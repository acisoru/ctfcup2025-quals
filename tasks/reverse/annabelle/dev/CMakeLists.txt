cmake_minimum_required(VERSION 3.15)
project(annabelle C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_executable(${PROJECT_NAME} src/main.c)
