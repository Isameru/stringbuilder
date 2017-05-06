cmake_minimum_required(VERSION 2.8)

project(stringbuilder C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED on)

set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
if(MSVC)
add_definitions(-DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0)
endif()
add_subdirectory(src/external/googletest EXLUCDE_FROM_ALL)
add_library(google::test ALIAS gtest_main)
include_directories(src/include)

add_subdirectory(src/tests)
