cmake_minimum_required(VERSION 3.16)
include(./cmake/idf.cmake)

project(kernel)

idf_build(
    COMPONENTS
        # "mbedtls"
        # "esp_event"
        # "lwip"
    SRCS
        "main.cpp"
)
