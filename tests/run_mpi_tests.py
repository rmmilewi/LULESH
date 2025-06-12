#!/usr/bin/env python3
"""
MPI Test framework for LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics)
This script runs various MPI test cases for LULESH and verifies the results.
"""

import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
import time

# Define the path to the LULESH executable (MPI version)
LULESH_PATH = Path("../build/lulesh2.0")

# Define MPI test cases
MPI_TEST_CASES = [
    {
        "name": "mpi_2_ranks",
        "ranks": 2,
        "args": ["-s", "10", "-i", "10"],
        "description": "Small problem size (10^3) with 10 iterations on 2 MPI ranks",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "mpi_4_ranks",
        "ranks": 4,
        "args": ["-s", "10", "-i", "10"],
        "description": "Small problem size (10^3) with 10 iterations on 4 MPI ranks",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "mpi_8_ranks",
        "ranks": 8,
        "args": ["-s", "10", "-i", "10"],
        "description": "Small problem size (10^3) with 10 iterations on 8 MPI ranks",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
]

# MPI Performance test cases
MPI_PERFORMANCE_TESTS = [
    {
        "name": "mpi_perf_small",
        "ranks": 4,
        "args": ["-s", "30", "-i", "20"],
        "description": "Performance test with small problem size on 4 MPI ranks",
    },
    {
        "name": "mpi_perf_medium",
        "ranks": 8,
        "args": ["-s", "50", "-i", "10"],
        "description": "Performance test with medium problem size on 8 MPI ranks",
    },
]

def run_mpi_test(test_case):
    """Run a single MPI test case and return the results"""
    print(f"Running test: {test_case['name']} - {test_case['description']}")
    
    ranks = test_case["ranks"]
    cmd = ["mpirun", "-np", str(ranks), str(LULESH_PATH)] + test_case["args"]
    start_time = time.time()
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        end_time = time.time()
        
        # Extract the final energy from the output
        energy_match = re.search(r"Final Origin Energy =\s+(\d+\.\d+e[+-]\d+)", result.stdout)
        if energy_match:
            final_energy = float(energy_match.group(1))
        else:
            print(f"ERROR: Could not extract final energy from output")
            return False
            
        # Extract the elapsed time
        time_match = re.search(r"Elapsed time\s+=\s+(\d+\.\d+)", result.stdout)
        if time_match:
            elapsed_time = float(time_match.group(1))
        else:
            elapsed_time = end_time - start_time
            
        # Extract FOM (Figure of Merit)
        fom_match = re.search(r"FOM\s+=\s+(\d+\.\d+)", result.stdout)
        fom = float(fom_match.group(1)) if fom_match else None
        
        # Check if the result matches the expected value (if provided)
        if "expected_energy" in test_case:
            expected = test_case["expected_energy"]
            tolerance = test_case.get("tolerance", 1e-5)
            rel_diff = abs((final_energy - expected) / expected)
            
            if rel_diff <= tolerance:
                print(f"✅ PASS: Final energy {final_energy} matches expected {expected} (relative diff: {rel_diff})")
                test_passed = True
            else:
                print(f"❌ FAIL: Final energy {final_energy} does not match expected {expected} (relative diff: {rel_diff})")
                test_passed = False
        else:
            # For performance tests, we just report the results
            test_passed = True
            
        # Print performance metrics
        print(f"Elapsed time: {elapsed_time:.4f} seconds")
        if fom:
            print(f"Figure of Merit: {fom:.2f} (z/s)")
            
        return test_passed
        
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Test failed with return code {e.returncode}")
        print(f"STDOUT: {e.stdout}")
        print(f"STDERR: {e.stderr}")
        return False

def check_mpi_available():
    """Check if MPI is available on the system"""
    try:
        result = subprocess.run(["mpirun", "--version"], capture_output=True, text=True)
        return result.returncode == 0
    except FileNotFoundError:
        return False

def run_all_mpi_tests():
    """Run all defined MPI test cases"""
    print("=" * 80)
    print("LULESH MPI Test Suite")
    print("=" * 80)
    
    # Check if LULESH executable exists
    if not LULESH_PATH.exists():
        print(f"ERROR: LULESH executable not found at {LULESH_PATH}")
        print("Please build LULESH first using the instructions in the README")
        return False
    
    # Check if MPI is available
    if not check_mpi_available():
        print("ERROR: MPI is not available on this system")
        print("Please install MPI (e.g., OpenMPI or MPICH) and rebuild LULESH with MPI support")
        return False
    
    # Run correctness tests
    print("\nRunning MPI correctness tests:")
    print("-" * 40)
    
    passed = 0
    failed = 0
    
    for test in MPI_TEST_CASES:
        if run_mpi_test(test):
            passed += 1
        else:
            failed += 1
        print("-" * 40)
    
    # Run performance tests if requested
    if args.performance:
        print("\nRunning MPI performance tests:")
        print("-" * 40)
        
        for test in MPI_PERFORMANCE_TESTS:
            run_mpi_test(test)
            print("-" * 40)
    
    # Print summary
    print("\nTest Summary:")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    print(f"Total: {passed + failed}")
    
    return failed == 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run LULESH MPI tests")
    parser.add_argument("--performance", action="store_true", help="Run performance tests")
    parser.add_argument("--test", help="Run a specific test by name")
    args = parser.parse_args()
    
    if args.test:
        # Run a specific test
        test_found = False
        for test in MPI_TEST_CASES + (MPI_PERFORMANCE_TESTS if args.performance else []):
            if test["name"] == args.test:
                run_mpi_test(test)
                test_found = True
                break
        
        if not test_found:
            print(f"ERROR: Test '{args.test}' not found")
            sys.exit(1)
    else:
        # Run all tests
        success = run_all_mpi_tests()
        sys.exit(0 if success else 1)