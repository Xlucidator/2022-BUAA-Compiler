cmake_minimum_required(VERSION 3.13)
project(Compiler)

set(CMAKE_CXX_STANDARD 14)
aux_source_directory(./src SRC_LIST)

add_executable(Compiler ${SRC_LIST})