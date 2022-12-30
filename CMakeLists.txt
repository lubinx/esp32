cmake_minimum_required(VERSION 3.16)
include(./cmake/idf.cmake)

project(kernel)

idf_build(
    COMPONENTS
        # ...
    SRCS
        "main.cpp"
)
