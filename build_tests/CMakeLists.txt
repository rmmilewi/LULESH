cmake_minimum_required(VERSION 3.10)
project(LULESH_TESTS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define USE_MPI=0 for tests
add_definitions(-DUSE_MPI=0)

# Find GTest package (includes GMock)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include parent directory for header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Add source files - only include what we need for testing
set(LULESH_SOURCES
    ../lulesh-util.cc
)

# Add test executable
add_executable(lulesh_tests ../lulesh_tests.cpp ${LULESH_SOURCES})

# Link against GTest libraries (includes GMock)
target_link_libraries(lulesh_tests ${GTEST_LIBRARIES} pthread)

# Add test
add_test(NAME LuleshTests COMMAND lulesh_tests)

# Enable testing
enable_testing()