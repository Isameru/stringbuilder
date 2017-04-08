# MIT License
#
# Copyright (c) 2017 Isameru
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 2.8)

project(stringbuilder)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
add_subdirectory(src/external/googletest EXLUCDE_FROM_ALL)
target_include_directories(gtest_main PUBLIC src/external/googletest/include)
add_library(google::test ALIAS gtest_main)

include_directories(src/include)

add_subdirectory(src/tests)
