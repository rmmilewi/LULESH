#!/bin/bash
# Script to run all LULESH tests

set -e  # Exit on error

echo "===== LULESH Test Suite ====="
echo "Running all tests..."

# Check if LULESH is built
if [ ! -f "../build/lulesh2.0" ]; then
    echo "LULESH executable not found. Building LULESH..."
    cd ..
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DWITH_MPI=Off ..
    make
    cd ../tests
fi

# Run correctness tests
echo -e "\n===== Running Correctness Tests ====="
python3 run_tests.py

# Run performance tests
echo -e "\n===== Running Performance Tests ====="
python3 run_tests.py --performance

# Run scaling tests
echo -e "\n===== Running Scaling Tests ====="
python3 scaling_test.py --test-type size --size 10 --iterations 5

# Run thread scaling tests
echo -e "\n===== Running Thread Scaling Tests ====="
python3 scaling_test.py --test-type thread --size 10 --iterations 5

# Run convergence tests
echo -e "\n===== Running Convergence Tests ====="
python3 convergence_test.py --size 10 --max-iterations 20 --step 2

# Run memory tests
echo -e "\n===== Running Memory Tests ====="
python3 memory_test.py --size 10 --iterations 5

# Check if MPI is available
if command -v mpirun &> /dev/null; then
    # Check if MPI version of LULESH is built
    if [ ! -f "../build_mpi/lulesh2.0" ]; then
        echo "MPI version of LULESH not found. Building MPI version..."
        cd ..
        mkdir -p build_mpi
        cd build_mpi
        cmake -DCMAKE_BUILD_TYPE=Release -DWITH_MPI=On -DMPI_CXX_COMPILER=`which mpicxx` ..
        make
        cd ../tests
    fi
    
    # Run MPI tests
    echo -e "\n===== Running MPI Tests ====="
    python3 run_mpi_tests.py
else
    echo -e "\n===== Skipping MPI Tests (MPI not available) ====="
fi

echo -e "\n===== All tests completed ====="