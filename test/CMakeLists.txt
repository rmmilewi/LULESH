

include(FetchContent)

FetchContent_Declare(
  googletest
  SOURCE_DIR "D:/repos/googletest-1.17.0/googletest-1.17.0"
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(lulesh_test correctness_test.cc ../src/lulesh.cc)
target_link_libraries(lulesh_test GTest::gtest_main lulesh_lib ${LULESH_EXTERNAL_LIBS})
target_compile_definitions(lulesh_test PRIVATE TEST)

include(GoogleTest)
gtest_discover_tests(lulesh_test)
