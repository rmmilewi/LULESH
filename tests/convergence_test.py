#!/usr/bin/env python3
"""
Convergence Test for LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics)
This script tests the convergence properties of LULESH by running with increasing iterations.
"""

import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
import time
import matplotlib.pyplot as plt
import numpy as np

# Define the path to the LULESH executable
LULESH_PATH = Path("../build/lulesh2.0")

def run_lulesh(size, iterations):
    """Run LULESH with the given problem size and iterations"""
    cmd = [str(LULESH_PATH), "-s", str(size), "-i", str(iterations), "-q"]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        
        # Extract the final energy from the output
        energy_match = re.search(r"Final Origin Energy =\s+(\d+\.\d+e[+-]\d+)", result.stdout)
        if energy_match:
            final_energy = float(energy_match.group(1))
            return final_energy
        else:
            print(f"ERROR: Could not extract final energy from output")
            return None
            
    except subprocess.CalledProcessError as e:
        print(f"ERROR: LULESH failed with return code {e.returncode}")
        print(f"STDOUT: {e.stdout}")
        print(f"STDERR: {e.stderr}")
        return None

def test_convergence(problem_size=10, max_iterations=100, step=5):
    """Test the convergence of LULESH by running with increasing iterations"""
    print(f"Testing convergence for problem size {problem_size}^3")
    print(f"Running from 1 to {max_iterations} iterations with step {step}")
    
    iterations = list(range(1, max_iterations + 1, step))
    energies = []
    
    for i in iterations:
        print(f"Running with {i} iterations...", end="", flush=True)
        energy = run_lulesh(problem_size, i)
        if energy is not None:
            energies.append(energy)
            print(f" Final energy: {energy}")
        else:
            print(f" Failed")
            energies.append(None)
    
    # Filter out None values
    valid_data = [(i, e) for i, e in zip(iterations, energies) if e is not None]
    if not valid_data:
        print("No valid data points collected")
        return False
    
    valid_iterations, valid_energies = zip(*valid_data)
    
    # Calculate convergence metrics
    if len(valid_energies) > 1:
        # Calculate relative changes between consecutive iterations
        rel_changes = [abs((valid_energies[i] - valid_energies[i-1]) / valid_energies[i-1]) 
                      for i in range(1, len(valid_energies))]
        
        print("\nConvergence Analysis:")
        print(f"Initial energy: {valid_energies[0]}")
        print(f"Final energy: {valid_energies[-1]}")
        print(f"Maximum relative change: {max(rel_changes)}")
        print(f"Final relative change: {rel_changes[-1]}")
        
        # Plot the results
        plt.figure(figsize=(10, 6))
        
        # Energy vs Iterations
        plt.subplot(2, 1, 1)
        plt.plot(valid_iterations, valid_energies, 'o-')
        plt.xlabel('Iterations')
        plt.ylabel('Final Origin Energy')
        plt.title(f'LULESH Convergence Test (Problem Size: {problem_size}^3)')
        plt.grid(True)
        
        # Relative Change vs Iterations
        plt.subplot(2, 1, 2)
        plt.semilogy(valid_iterations[1:], rel_changes, 'o-')
        plt.xlabel('Iterations')
        plt.ylabel('Relative Change')
        plt.title('Convergence Rate (Relative Change Between Consecutive Iterations)')
        plt.grid(True)
        
        plt.tight_layout()
        
        # Save the plot
        output_file = f"lulesh_convergence_s{problem_size}.png"
        plt.savefig(output_file)
        print(f"\nConvergence plot saved to {output_file}")
        
        # Check if the solution has converged
        if rel_changes[-1] < 1e-6:
            print("\nThe solution appears to have converged.")
            return True
        else:
            print("\nThe solution has not yet converged to the desired tolerance.")
            return False
    else:
        print("Not enough data points to analyze convergence")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test LULESH convergence")
    parser.add_argument("--size", type=int, default=10, help="Problem size (default: 10)")
    parser.add_argument("--max-iterations", type=int, default=100, help="Maximum number of iterations (default: 100)")
    parser.add_argument("--step", type=int, default=5, help="Step size for iterations (default: 5)")
    args = parser.parse_args()
    
    # Check if matplotlib is available
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("WARNING: matplotlib is not installed. Plots will not be generated.")
        print("Install matplotlib with: pip install matplotlib")
    
    # Check if LULESH executable exists
    if not LULESH_PATH.exists():
        print(f"ERROR: LULESH executable not found at {LULESH_PATH}")
        print("Please build LULESH first using the instructions in the README")
        sys.exit(1)
    
    # Run the convergence test
    success = test_convergence(args.size, args.max_iterations, args.step)
    sys.exit(0 if success else 1)