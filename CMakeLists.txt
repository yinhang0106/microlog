cmake_minimum_required(VERSION 3.22)

project(microlog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(microlog INTERFACE)
target_include_directories(microlog INTERFACE include)

add_subdirectory(test)
