cmake_minimum_required(VERSION 3.22)
project(shared_ptr)

find_package(GTest REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

include_directories(include)
include_directories(${GTEST_INCLUDE_DIRS})

enable_testing()

add_executable(
        main_test
        test/shared_ptr_test.cpp
)
target_link_libraries(main_test GTest::gtest_main)
gtest_discover_tests(main_test)
