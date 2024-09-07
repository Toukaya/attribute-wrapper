# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

cmake_minimum_required(VERSION 3.28)
project(test LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  option(CXX_STANDARD_REQUIRED "Require c++ standard" YES)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_EXTENSIONS NO)
endif()

enable_testing()

find_package(Catch2 3 REQUIRED)

add_executable(test attr_test.cpp)
target_link_libraries(test PRIVATE Catch2::Catch2WithMain attr)