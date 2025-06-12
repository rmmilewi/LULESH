#!/usr/bin/env python3
"""
Scaling Test for LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics)
This script tests the scaling properties of LULESH with different problem sizes and thread counts.
"""

import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
import time
import csv
from datetime import datetime

# Define the path to the LULESH executable
LULESH_PATH = Path("../build/lulesh2.0")

def run_lulesh(size, iterations, threads=None):
    """Run LULESH with the given problem size, iterations, and thread count"""
    env = os.environ.copy()
    if threads is not None:
        env["OMP_NUM_THREADS"] = str(threads)
    
    cmd = [str(LULESH_PATH), "-s", str(size), "-i", str(iterations), "-q"]
    
    start_time = time.time()
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True, env=env)
        end_time = time.time()
        
        # Extract the final energy from the output
        energy_match = re.search(r"Final Origin Energy =\s+(\d+\.\d+e[+-]\d+)", result.stdout)
        final_energy = float(energy_match.group(1)) if energy_match else None
        
        # Extract the elapsed time from the output
        time_match = re.search(r"Elapsed time\s+=\s+(\d+\.\d+)", result.stdout)
        elapsed_time = float(time_match.group(1)) if time_match else (end_time - start_time)
        
        # Extract FOM (Figure of Merit)
        fom_match = re.search(r"FOM\s+=\s+(\d+\.\d+)", result.stdout)
        fom = float(fom_match.group(1)) if fom_match else None
        
        return {
            'success': True,
            'final_energy': final_energy,
            'elapsed_time': elapsed_time,
            'fom': fom,
        }
        
    except subprocess.CalledProcessError as e:
        print(f"ERROR: LULESH failed with return code {e.returncode}")
        print(f"STDOUT: {e.stdout}")
        print(f"STDERR: {e.stderr}")
        return {
            'success': False,
            'error': str(e),
        }

def test_problem_size_scaling(iterations=10, threads=None):
    """Test how LULESH scales with problem size"""
    print("=" * 80)
    print("LULESH Problem Size Scaling Test")
    if threads is not None:
        print(f"Using {threads} OpenMP threads")
    print("=" * 80)
    
    # Define problem sizes to test
    sizes = [10, 20, 30, 40, 50, 60, 70, 80]
    results = []
    
    for size in sizes:
        print(f"Running with problem size {size}^3...", end="", flush=True)
        result = run_lulesh(size, iterations, threads)
        
        if result['success']:
            print(f" Completed in {result['elapsed_time']:.2f} seconds")
            results.append({
                'problem_size': size,
                'iterations': iterations,
                'threads': threads,
                'elapsed_time': result['elapsed_time'],
                'final_energy': result['final_energy'],
                'fom': result['fom'],
            })
        else:
            print(f" Failed")
    
    # Save results to CSV
    if results:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"lulesh_size_scaling_{timestamp}.csv"
        
        with open(csv_filename, 'w', newline='') as csvfile:
            fieldnames = ['problem_size', 'iterations', 'threads', 
                         'elapsed_time', 'final_energy', 'fom']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for result in results:
                writer.writerow(result)
                
        print(f"\nSize scaling results saved to: {csv_filename}")
        
        # Try to generate a plot if matplotlib is available
        try:
            import matplotlib.pyplot as plt
            import numpy as np
            
            # Extract data for plotting
            sizes = [r['problem_size'] for r in results]
            times = [r['elapsed_time'] for r in results]
            
            # Create the plot
            plt.figure(figsize=(10, 6))
            
            # Plot actual times
            plt.plot(sizes, times, 'o-', label='Measured Time')
            
            # Plot theoretical O(N^3) scaling
            if len(sizes) > 1:
                # Use the first point to calibrate
                scale_factor = times[0] / (sizes[0]**3)
                theoretical_sizes = np.array(sizes)
                theoretical_times = scale_factor * (theoretical_sizes**3)
                plt.plot(theoretical_sizes, theoretical_times, 'k--', label='Theoretical O(N^3)')
            
            plt.xlabel('Problem Size (N for N^3)')
            plt.ylabel('Execution Time (seconds)')
            plt.title('LULESH Problem Size Scaling')
            plt.legend()
            plt.grid(True)
            
            # Add log-log plot as inset
            if len(sizes) > 1:
                ax_inset = plt.axes([0.55, 0.2, 0.3, 0.3])
                ax_inset.loglog(sizes, times, 'o-', label='Measured')
                ax_inset.loglog(theoretical_sizes, theoretical_times, 'k--', label='O(N^3)')
                ax_inset.set_xlabel('Size (log)')
                ax_inset.set_ylabel('Time (log)')
                ax_inset.grid(True)
            
            # Save the plot
            plot_filename = f"lulesh_size_scaling_{timestamp}.png"
            plt.savefig(plot_filename)
            print(f"Size scaling plot saved to: {plot_filename}")
            
        except ImportError:
            print("WARNING: matplotlib is not installed. Plot was not generated.")
            print("Install matplotlib with: pip install matplotlib")
    
    return results

def test_thread_scaling(problem_size=30, iterations=10):
    """Test how LULESH scales with the number of OpenMP threads"""
    print("=" * 80)
    print(f"LULESH Thread Scaling Test (Problem Size: {problem_size}^3)")
    print("=" * 80)
    
    # Get the number of available CPU cores
    import multiprocessing
    max_cores = multiprocessing.cpu_count()
    
    # Define thread counts to test (powers of 2 up to max_cores)
    thread_counts = [1]
    count = 2
    while count <= max_cores:
        thread_counts.append(count)
        count *= 2
    
    # Add max_cores if it's not already in the list
    if max_cores not in thread_counts:
        thread_counts.append(max_cores)
    
    # Sort the list
    thread_counts.sort()
    
    results = []
    baseline_time = None
    
    for threads in thread_counts:
        print(f"Running with {threads} threads...", end="", flush=True)
        result = run_lulesh(problem_size, iterations, threads)
        
        if result['success']:
            elapsed_time = result['elapsed_time']
            print(f" Completed in {elapsed_time:.2f} seconds")
            
            # Calculate speedup relative to single thread
            if threads == 1:
                baseline_time = elapsed_time
                speedup = 1.0
            else:
                speedup = baseline_time / elapsed_time if baseline_time else None
                
            # Calculate efficiency
            efficiency = (speedup / threads) if speedup else None
            
            results.append({
                'problem_size': problem_size,
                'iterations': iterations,
                'threads': threads,
                'elapsed_time': elapsed_time,
                'speedup': speedup,
                'efficiency': efficiency,
                'final_energy': result['final_energy'],
                'fom': result['fom'],
            })
        else:
            print(f" Failed")
    
    # Save results to CSV
    if results:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"lulesh_thread_scaling_{timestamp}.csv"
        
        with open(csv_filename, 'w', newline='') as csvfile:
            fieldnames = ['problem_size', 'iterations', 'threads', 
                         'elapsed_time', 'speedup', 'efficiency',
                         'final_energy', 'fom']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for result in results:
                writer.writerow(result)
                
        print(f"\nThread scaling results saved to: {csv_filename}")
        
        # Try to generate a plot if matplotlib is available
        try:
            import matplotlib.pyplot as plt
            
            # Extract data for plotting
            threads = [r['threads'] for r in results]
            speedups = [r['speedup'] for r in results]
            efficiencies = [r['efficiency'] for r in results]
            
            # Create the plot
            plt.figure(figsize=(12, 6))
            
            # Speedup plot
            plt.subplot(1, 2, 1)
            plt.plot(threads, speedups, 'o-', label='Measured Speedup')
            plt.plot(threads, threads, 'k--', label='Ideal Speedup')
            plt.xlabel('Number of Threads')
            plt.ylabel('Speedup')
            plt.title('LULESH Strong Scaling: Speedup')
            plt.legend()
            plt.grid(True)
            
            # Efficiency plot
            plt.subplot(1, 2, 2)
            plt.plot(threads, efficiencies, 'o-', label='Parallel Efficiency')
            plt.axhline(y=1.0, color='k', linestyle='--', label='Ideal Efficiency')
            plt.xlabel('Number of Threads')
            plt.ylabel('Efficiency')
            plt.title('LULESH Strong Scaling: Efficiency')
            plt.legend()
            plt.grid(True)
            
            plt.tight_layout()
            
            # Save the plot
            plot_filename = f"lulesh_thread_scaling_{timestamp}.png"
            plt.savefig(plot_filename)
            print(f"Thread scaling plot saved to: {plot_filename}")
            
        except ImportError:
            print("WARNING: matplotlib is not installed. Plot was not generated.")
            print("Install matplotlib with: pip install matplotlib")
    
    return results

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test LULESH scaling properties")
    parser.add_argument("--size", type=int, default=30, help="Problem size for thread scaling test (default: 30)")
    parser.add_argument("--iterations", type=int, default=10, help="Number of iterations (default: 10)")
    parser.add_argument("--threads", type=int, help="Number of OpenMP threads for size scaling test")
    parser.add_argument("--test-type", choices=["size", "thread", "both"], default="both",
                       help="Type of scaling test to run (default: both)")
    args = parser.parse_args()
    
    # Check if LULESH executable exists
    if not LULESH_PATH.exists():
        print(f"ERROR: LULESH executable not found at {LULESH_PATH}")
        print("Please build LULESH first using the instructions in the README")
        sys.exit(1)
    
    if args.test_type in ["size", "both"]:
        test_problem_size_scaling(args.iterations, args.threads)
        
    if args.test_type in ["thread", "both"]:
        test_thread_scaling(args.size, args.iterations)