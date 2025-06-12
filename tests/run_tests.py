#!/usr/bin/env python3
"""
Test framework for LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics)
This script runs various test cases for LULESH and verifies the results.
"""

import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
import time

# Define the path to the LULESH executable
LULESH_PATH = Path("../build/lulesh2.0")

# Define test cases
TEST_CASES = [
    {
        "name": "small_problem",
        "args": ["-s", "10", "-i", "10"],
        "description": "Small problem size (10^3) with 10 iterations",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "medium_problem",
        "args": ["-s", "20", "-i", "10"],
        "description": "Medium problem size (20^3) with 10 iterations",
        "expected_energy": 2.505353e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "regions_test",
        "args": ["-s", "10", "-i", "10", "-r", "5"],
        "description": "Testing with 5 regions",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "balance_test",
        "args": ["-s", "10", "-i", "10", "-b", "2"],
        "description": "Testing with balance factor 2",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
    {
        "name": "cost_test",
        "args": ["-s", "10", "-i", "10", "-c", "2"],
        "description": "Testing with cost factor 2",
        "expected_energy": 2.596764e+05,
        "tolerance": 1e-5,
    },
]

# Performance test cases
PERFORMANCE_TESTS = [
    {
        "name": "perf_small",
        "args": ["-s", "30", "-i", "20"],
        "description": "Performance test with small problem size",
    },
    {
        "name": "perf_medium",
        "args": ["-s", "50", "-i", "10"],
        "description": "Performance test with medium problem size",
    },
]

def run_test(test_case):
    """Run a single test case and return the results"""
    print(f"Running test: {test_case['name']} - {test_case['description']}")
    
    cmd = [str(LULESH_PATH)] + test_case["args"]
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

def run_all_tests():
    """Run all defined test cases"""
    print("=" * 80)
    print("LULESH Test Suite")
    print("=" * 80)
    
    # Check if LULESH executable exists
    if not LULESH_PATH.exists():
        print(f"ERROR: LULESH executable not found at {LULESH_PATH}")
        print("Please build LULESH first using the instructions in the README")
        return False
    
    # Run correctness tests
    print("\nRunning correctness tests:")
    print("-" * 40)
    
    passed = 0
    failed = 0
    
    for test in TEST_CASES:
        if run_test(test):
            passed += 1
        else:
            failed += 1
        print("-" * 40)
    
    # Run performance tests if requested
    if args.performance:
        print("\nRunning performance tests:")
        print("-" * 40)
        
        for test in PERFORMANCE_TESTS:
            run_test(test)
            print("-" * 40)
    
    # Print summary
    print("\nTest Summary:")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    print(f"Total: {passed + failed}")
    
    return failed == 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run LULESH tests")
    parser.add_argument("--performance", action="store_true", help="Run performance tests")
    parser.add_argument("--test", help="Run a specific test by name")
    args = parser.parse_args()
    
    if args.test:
        # Run a specific test
        test_found = False
        for test in TEST_CASES + (PERFORMANCE_TESTS if args.performance else []):
            if test["name"] == args.test:
                run_test(test)
                test_found = True
                break
        
        if not test_found:
            print(f"ERROR: Test '{args.test}' not found")
            sys.exit(1)
    else:
        # Run all tests
        success = run_all_tests()
        sys.exit(0 if success else 1)