# LULESH Test Suite

This directory contains tests for the LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics) proxy application.

## Overview

The test suite includes:

1. **Correctness Tests**: Verify that LULESH produces the expected results for various configurations
2. **Performance Tests**: Measure the performance of LULESH with different problem sizes and parameters
3. **Regression Tests**: Ensure that code changes don't negatively impact results or performance

## Running Tests

Before running tests, make sure you have built LULESH:

```bash
cd /path/to/LULESH
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_MPI=Off ..
make
```

To run the tests:

```bash
cd /path/to/LULESH/tests
python3 run_tests.py
```

### Command Line Options

- `--performance`: Run performance tests in addition to correctness tests
- `--test <test_name>`: Run a specific test by name

Example:
```bash
python3 run_tests.py --performance
python3 run_tests.py --test small_problem
```

## Test Categories

### Correctness Tests

These tests verify that LULESH produces the expected results:

1. **small_problem**: Small problem size (10^3) with 10 iterations
2. **medium_problem**: Medium problem size (20^3) with 10 iterations
3. **regions_test**: Testing with 5 regions
4. **balance_test**: Testing with balance factor 2
5. **cost_test**: Testing with cost factor 2

### Performance Tests

These tests measure the performance of LULESH:

1. **perf_small**: Performance test with small problem size (30^3)
2. **perf_medium**: Performance test with medium problem size (50^3)

## Adding New Tests

To add a new test, edit the `run_tests.py` file and add a new entry to the `TEST_CASES` or `PERFORMANCE_TESTS` list:

```python
{
    "name": "your_test_name",
    "args": ["-s", "10", "-i", "10", "-other", "args"],
    "description": "Description of your test",
    "expected_energy": 1.234e+05,  # Optional for correctness tests
    "tolerance": 1e-5,             # Optional, defaults to 1e-5
}
```

## Recommended Additional Tests

Here are some additional tests that could be implemented:

1. **MPI Tests**: Test LULESH with different numbers of MPI ranks
2. **OpenMP Tests**: Test LULESH with different numbers of OpenMP threads
3. **Memory Usage Tests**: Monitor memory usage during execution
4. **Convergence Tests**: Verify that results converge as expected with increasing iterations
5. **Boundary Condition Tests**: Test different boundary conditions
6. **Stress Tests**: Run with very large problem sizes to test stability
7. **Reproducibility Tests**: Verify that results are reproducible across runs