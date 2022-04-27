cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_FLAGS "-std=gnu++17 -Wall -Wextra")

project(sk_router)

add_executable(sk_router src/main.cpp src/router.cpp src/vector_entry.cpp src/utilities.cpp)
