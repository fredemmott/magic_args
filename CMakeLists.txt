cmake_minimum_required(VERSION 3.29)
project(magic_args VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(magic_args)

add_executable(test "test.cpp")
target_link_libraries(test magic_args)
