cmake_minimum_required(VERSION 3.31.0 FATAL_ERROR)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN "/opt/gcc-15")

project("HastyRadical" VERSION 0.1 LANGUAGES CXX)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

message("Hello ${pybind11_DIR}")
# Add the unsupported compiler flag explicitly
add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:-allow-unsupported-compiler>)

add_executable(HastyRadical
    "src/main.cpp"
)
target_compile_features(HastyRadical
    PRIVATE cxx_std_23
    INTERFACE cxx_std_23)

# Define project root directory for file paths
target_compile_definitions(HastyRadical PRIVATE 
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

set_target_properties(HastyRadical PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_SCAN_FOR_MODULES ON
    CXX_MODULE_STD ON
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(HastyRadical PRIVATE -ftime-trace)
endif()

#target_link_libraries(HastyRadical PRIVATE c++ c++abi)

include(FetchContent)

target_sources(HastyRadical
    PUBLIC 
        FILE_SET CXX_MODULES FILES
            "src/containers.cppm"
            "src/mult_test.cppm"
            "src/radlib.cppm"
            "src/test_class.cppm"
            "src/threadpool.cppm"
            "src/tests.cppm"
)

