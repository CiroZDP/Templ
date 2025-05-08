# Probably overkill for a simple project, but I'd rather this than a "compile.sh" script :/
cmake_minimum_required(VERSION 3.10)
project(templ)
set(CMAKE_CXX_STANDARD 20)

add_executable(templ main.cpp)