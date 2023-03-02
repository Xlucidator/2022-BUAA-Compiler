cmake_minimum_required(VERSION 3.13)
project(Compiler)

set(CMAKE_CXX_STANDARD 11)

aux_source_directory(./src/ SRC_LIST)
aux_source_directory(./src/lexer SRC_LIST)
aux_source_directory(./src/parser SRC_LIST)
    aux_source_directory(./src/parser/errorhandler SRC_LIST)
    aux_source_directory(./src/parser/irbuilder SRC_LIST)
aux_source_directory(./src/generator SRC_LIST)
aux_source_directory(./src/tool SRC_LIST)

add_executable(Compiler ${SRC_LIST})