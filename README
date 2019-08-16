# reLULESH: A Refactor of LULESH

This is a demonstration of common software refactoring techniques applied to scientific C++ code. Refactoring is the act of improving the design of software without changing its behavior. 

The object of our study is LULESH, a widely-known proxy code for shock hydrodynamics developed at Lawrence Livermore National Laboratory. This project (reLULESH) is a fork of LULESH 2.0.1, and here we apply techniques from Bob Martin's magnum opus *Clean Code: A Handbook of Agile Software Craftsmanship*. We will provide a table of performance comparisons to showcase the cost-benefit trade-offs of performing different kinds of refactoring.

#### Contact Information

Feel free to open an issue if you have any questions or comments!

Want to get in touch with the original developers? Contact Ian Karlin (<karlin1@llnl.gov>) and/or Rob Neely (<neely4@llnl.gov>).

#### References
*Martin, Robert C. Clean code: a handbook of agile software craftsmanship. Pearson Education, 2009.*

*I. Karlin, J. Keasler, R. Neely. LULESH 2.0 Updates and Changes. August 2013, pages 1-9*

#### Note
reLULESH represents a personal effort to learn and apply the principles and practices of software refactoring. I make no guarantees about the quality or correctness of the code provided here.

### Build Instructions

A Makefile and a CMake build system are provided.

#### Building with CMake

Create a build directory and run cmake. Example:

```bash
  $ mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=Release -DMPI_CXX_COMPILER=`which mpicxx` ..
```

CMake variables:

```cmake
  CMAKE_BUILD_TYPE      "Debug", "Release", or "RelWithDebInfo"

  CMAKE_CXX_COMPILER    Path to the C++ compiler
  MPI_CXX_COMPILER      Path to the MPI C++ compiler

  WITH_MPI=On|Off       Build with MPI (Default: On)
  WITH_OPENMP=On|Off    Build with OpenMP support (Default: On)
  WITH_SILO=On|Off      Build with support for SILO. (Default: Off).
  
  SILO_DIR              Path to SILO library (only needed when WITH_SILO is "On")
```