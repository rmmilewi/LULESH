# LULESH Test Plan

This document outlines a comprehensive test plan for the LULESH (Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics) proxy application.

## 1. Introduction

LULESH is a proxy application that represents simplified 3D Lagrangian hydrodynamics on an unstructured mesh. It is designed to be representative of more complex hydrodynamics codes while being simple enough to study and modify. This test plan aims to ensure that LULESH functions correctly and efficiently across various configurations and environments.

## 2. Test Categories

### 2.1 Correctness Tests

These tests verify that LULESH produces the expected results for various configurations:

| Test ID | Description | Expected Outcome |
|---------|-------------|------------------|
| C1 | Small problem size (10^3) with 10 iterations | Final energy matches reference value |
| C2 | Medium problem size (20^3) with 10 iterations | Final energy matches reference value |
| C3 | Testing with 5 regions | Final energy matches reference value |
| C4 | Testing with balance factor 2 | Final energy matches reference value |
| C5 | Testing with cost factor 2 | Final energy matches reference value |

### 2.2 Performance Tests

These tests measure the performance of LULESH with different problem sizes and parameters:

| Test ID | Description | Metrics |
|---------|-------------|---------|
| P1 | Small problem size (30^3) with 20 iterations | Execution time, FOM |
| P2 | Medium problem size (50^3) with 10 iterations | Execution time, FOM |

### 2.3 Scaling Tests

These tests evaluate how LULESH scales with different problem sizes and thread counts:

| Test ID | Description | Metrics |
|---------|-------------|---------|
| S1 | Problem size scaling (10^3 to 80^3) | Execution time vs. problem size |
| S2 | Thread scaling (1 to max cores) | Speedup, efficiency |

### 2.4 MPI Tests

These tests verify that LULESH works correctly with MPI:

| Test ID | Description | Expected Outcome |
|---------|-------------|------------------|
| M1 | Small problem size (10^3) with 2 MPI ranks | Final energy matches reference value |
| M2 | Small problem size (10^3) with 4 MPI ranks | Final energy matches reference value |
| M3 | Small problem size (10^3) with 8 MPI ranks | Final energy matches reference value |

### 2.5 Convergence Tests

These tests verify that LULESH converges to a stable solution:

| Test ID | Description | Metrics |
|---------|-------------|---------|
| CV1 | Convergence test with increasing iterations | Relative change between iterations |

### 2.6 Memory Tests

These tests monitor the memory usage of LULESH:

| Test ID | Description | Metrics |
|---------|-------------|---------|
| MM1 | Memory usage with different problem sizes | Peak RSS, Peak VMS |

## 3. Test Environment

### 3.1 Hardware Requirements

- CPU: Multi-core processor (at least 4 cores recommended)
- Memory: At least 8GB RAM for small to medium problem sizes, 16GB+ for larger problems
- Storage: 1GB free space

### 3.2 Software Requirements

- Operating System: Linux (recommended), macOS, or Windows with WSL
- Compiler: GCC 4.8+ or compatible C++ compiler
- Build System: CMake 3.1+
- MPI Implementation: OpenMPI or MPICH (for MPI tests)
- Python 3.6+ (for test scripts)
- Python Packages: matplotlib, numpy, psutil (for analysis and visualization)

## 4. Test Procedures

### 4.1 Building LULESH

```bash
cd /path/to/LULESH
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_MPI=Off ..
make
```

For MPI version:
```bash
cd /path/to/LULESH
mkdir -p build_mpi && cd build_mpi
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_MPI=On -DMPI_CXX_COMPILER=`which mpicxx` ..
make
```

### 4.2 Running Tests

#### Correctness Tests
```bash
cd /path/to/LULESH/tests
python3 run_tests.py
```

#### Performance Tests
```bash
cd /path/to/LULESH/tests
python3 run_tests.py --performance
```

#### Scaling Tests
```bash
cd /path/to/LULESH/tests
python3 scaling_test.py
```

#### MPI Tests
```bash
cd /path/to/LULESH/tests
python3 run_mpi_tests.py
```

#### Convergence Tests
```bash
cd /path/to/LULESH/tests
python3 convergence_test.py
```

#### Memory Tests
```bash
cd /path/to/LULESH/tests
python3 memory_test.py --scaling
```

## 5. Test Reporting

Each test script generates CSV files and plots (when matplotlib is available) with the test results. These files are saved in the tests directory with timestamps in the filenames.

## 6. Continuous Integration

For continuous integration, the following tests should be run automatically on each commit:

1. Correctness tests (run_tests.py)
2. A subset of performance tests with small problem sizes
3. Basic MPI tests with 2 ranks

## 7. Regression Testing

When making changes to LULESH, the following regression tests should be run:

1. All correctness tests
2. Performance tests to ensure performance hasn't degraded
3. MPI tests to ensure parallel functionality still works

## 8. Future Test Enhancements

1. **Fault Injection Tests**: Introduce artificial faults to test robustness
2. **Boundary Condition Tests**: Test different boundary conditions
3. **Reproducibility Tests**: Verify that results are reproducible across runs
4. **Cross-Platform Tests**: Test on different operating systems and hardware
5. **Compiler Optimization Tests**: Test with different compiler optimization levels

## 9. Conclusion

This test plan provides a comprehensive approach to testing LULESH. By following this plan, developers can ensure that LULESH functions correctly and efficiently across various configurations and environments.