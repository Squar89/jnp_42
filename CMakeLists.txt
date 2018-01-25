cmake_minimum_required(VERSION 3.5.1)
project(jnp_42)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2 -std=c++1z")

add_executable(jnp_42 42.cc)