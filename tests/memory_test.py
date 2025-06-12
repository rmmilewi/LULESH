#!/usr/bin/env python3
"""
Memory Usage Test for LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics)
This script monitors the memory usage of LULESH with different problem sizes.
"""

import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
import time
import psutil
import threading
import csv
from datetime import datetime

# Define the path to the LULESH executable
LULESH_PATH = Path("../build/lulesh2.0")

class MemoryMonitor:
    """Class to monitor memory usage of a process"""
    
    def __init__(self, pid, interval=0.1):
        self.pid = pid
        self.interval = interval
        self.memory_usage = []
        self.running = False
        self.thread = None
        
    def start(self):
        """Start monitoring memory usage"""
        self.running = True
        self.thread = threading.Thread(target=self._monitor)
        self.thread.daemon = True
        self.thread.start()
        
    def stop(self):
        """Stop monitoring memory usage"""
        self.running = False
        if self.thread:
            self.thread.join()
            
    def _monitor(self):
        """Monitor memory usage of the process"""
        try:
            process = psutil.Process(self.pid)
            while self.running:
                try:
                    # Get memory info
                    mem_info = process.memory_info()
                    self.memory_usage.append({
                        'timestamp': time.time(),
                        'rss': mem_info.rss,  # Resident Set Size
                        'vms': mem_info.vms,  # Virtual Memory Size
                    })
                    time.sleep(self.interval)
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    # Process has terminated
                    break
        except psutil.NoSuchProcess:
            # Process doesn't exist
            pass
            
    def get_peak_memory(self):
        """Get the peak memory usage in MB"""
        if not self.memory_usage:
            return 0, 0
            
        peak_rss = max(entry['rss'] for entry in self.memory_usage)
        peak_vms = max(entry['vms'] for entry in self.memory_usage)
        
        return peak_rss / (1024 * 1024), peak_vms / (1024 * 1024)
        
    def save_to_csv(self, filename):
        """Save memory usage data to a CSV file"""
        if not self.memory_usage:
            return
            
        with open(filename, 'w', newline='') as csvfile:
            fieldnames = ['timestamp', 'rss_mb', 'vms_mb']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for entry in self.memory_usage:
                writer.writerow({
                    'timestamp': entry['timestamp'],
                    'rss_mb': entry['rss'] / (1024 * 1024),
                    'vms_mb': entry['vms'] / (1024 * 1024),
                })

def run_memory_test(problem_size, iterations):
    """Run LULESH and monitor its memory usage"""
    print(f"Running memory test with problem size {problem_size}^3 and {iterations} iterations")
    
    cmd = [str(LULESH_PATH), "-s", str(problem_size), "-i", str(iterations)]
    
    # Start the process
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Start monitoring memory usage
    monitor = MemoryMonitor(process.pid)
    monitor.start()
    
    # Wait for the process to complete
    stdout, stderr = process.communicate()
    
    # Stop monitoring
    monitor.stop()
    
    # Check if the process completed successfully
    if process.returncode != 0:
        print(f"ERROR: LULESH failed with return code {process.returncode}")
        print(f"STDOUT: {stdout}")
        print(f"STDERR: {stderr}")
        return None
    
    # Extract the final energy from the output
    energy_match = re.search(r"Final Origin Energy =\s+(\d+\.\d+e[+-]\d+)", stdout)
    final_energy = float(energy_match.group(1)) if energy_match else None
    
    # Extract the elapsed time
    time_match = re.search(r"Elapsed time\s+=\s+(\d+\.\d+)", stdout)
    elapsed_time = float(time_match.group(1)) if time_match else None
    
    # Get peak memory usage
    peak_rss, peak_vms = monitor.get_peak_memory()
    
    # Save memory usage data to CSV
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"lulesh_memory_s{problem_size}_i{iterations}_{timestamp}.csv"
    monitor.save_to_csv(csv_filename)
    
    # Print results
    print(f"Test completed:")
    print(f"  Problem size: {problem_size}^3")
    print(f"  Iterations: {iterations}")
    if final_energy:
        print(f"  Final energy: {final_energy}")
    if elapsed_time:
        print(f"  Elapsed time: {elapsed_time} seconds")
    print(f"  Peak RSS memory usage: {peak_rss:.2f} MB")
    print(f"  Peak VMS memory usage: {peak_vms:.2f} MB")
    print(f"  Memory usage data saved to: {csv_filename}")
    
    return {
        'problem_size': problem_size,
        'iterations': iterations,
        'final_energy': final_energy,
        'elapsed_time': elapsed_time,
        'peak_rss_mb': peak_rss,
        'peak_vms_mb': peak_vms,
        'csv_file': csv_filename,
    }

def run_memory_scaling_tests(max_size=50, step=10, iterations=10):
    """Run memory tests with increasing problem sizes"""
    print("=" * 80)
    print("LULESH Memory Scaling Tests")
    print("=" * 80)
    
    results = []
    
    for size in range(step, max_size + 1, step):
        result = run_memory_test(size, iterations)
        if result:
            results.append(result)
        print("-" * 40)
    
    # Save summary to CSV
    if results:
        summary_file = f"lulesh_memory_scaling_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(summary_file, 'w', newline='') as csvfile:
            fieldnames = ['problem_size', 'iterations', 'final_energy', 
                         'elapsed_time', 'peak_rss_mb', 'peak_vms_mb', 'csv_file']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for result in results:
                writer.writerow(result)
                
        print(f"\nMemory scaling summary saved to: {summary_file}")
        
        # Try to generate a plot if matplotlib is available
        try:
            import matplotlib.pyplot as plt
            
            # Extract data for plotting
            sizes = [r['problem_size'] for r in results]
            rss_values = [r['peak_rss_mb'] for r in results]
            vms_values = [r['peak_vms_mb'] for r in results]
            
            # Create the plot
            plt.figure(figsize=(10, 6))
            plt.plot(sizes, rss_values, 'o-', label='RSS (Resident Set Size)')
            plt.plot(sizes, vms_values, 's-', label='VMS (Virtual Memory Size)')
            plt.xlabel('Problem Size (N for N^3)')
            plt.ylabel('Memory Usage (MB)')
            plt.title('LULESH Memory Scaling')
            plt.legend()
            plt.grid(True)
            
            # Add theoretical scaling line (N^3)
            if len(sizes) > 1:
                # Use the first point to calibrate
                scale_factor = rss_values[0] / (sizes[0]**3)
                theoretical_sizes = range(step, max_size + 1, step)
                theoretical_values = [scale_factor * (s**3) for s in theoretical_sizes]
                plt.plot(theoretical_sizes, theoretical_values, 'k--', label='Theoretical N^3 Scaling')
                plt.legend()
            
            # Save the plot
            plot_file = f"lulesh_memory_scaling_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png"
            plt.savefig(plot_file)
            print(f"Memory scaling plot saved to: {plot_file}")
            
        except ImportError:
            print("WARNING: matplotlib is not installed. Plot was not generated.")
            print("Install matplotlib with: pip install matplotlib")
    
    return results

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test LULESH memory usage")
    parser.add_argument("--size", type=int, default=30, help="Problem size for single test (default: 30)")
    parser.add_argument("--iterations", type=int, default=10, help="Number of iterations (default: 10)")
    parser.add_argument("--scaling", action="store_true", help="Run scaling tests with increasing problem sizes")
    parser.add_argument("--max-size", type=int, default=50, help="Maximum problem size for scaling tests (default: 50)")
    parser.add_argument("--step", type=int, default=10, help="Step size for scaling tests (default: 10)")
    args = parser.parse_args()
    
    # Check if psutil is available
    try:
        import psutil
    except ImportError:
        print("ERROR: psutil is not installed. This script requires psutil to monitor memory usage.")
        print("Install psutil with: pip install psutil")
        sys.exit(1)
    
    # Check if LULESH executable exists
    if not LULESH_PATH.exists():
        print(f"ERROR: LULESH executable not found at {LULESH_PATH}")
        print("Please build LULESH first using the instructions in the README")
        sys.exit(1)
    
    if args.scaling:
        # Run scaling tests
        run_memory_scaling_tests(args.max_size, args.step, args.iterations)
    else:
        # Run a single test
        run_memory_test(args.size, args.iterations)