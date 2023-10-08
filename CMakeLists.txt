cmake_minimum_required(VERSION 3.0)
project(youdis VERSION 0.0.1)
 
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_compile_options(-Wall)

include_directories(include)

add_executable(youdis
  main.cpp
)

