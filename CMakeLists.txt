cmake_minimum_required(VERSION 3.0)
project(thread_pool)

set(CMAKE_CXX_STANDARD 17)

add_executable(thread_pool thread_pool.cpp main.cpp)
target_link_libraries(thread_pool gtest pthread)