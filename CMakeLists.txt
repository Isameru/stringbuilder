cmake_minimum_required(VERSION 2.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(stringbuilder C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
#elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED on)

set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
if(MSVC)
add_definitions(-DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0)
endif()
set(BUILD_GTEST on)
set(BUILD_GMOCK off)
add_subdirectory(src/external/googletest EXLUCDE_FROM_ALL)
add_library(google::test ALIAS gtest_main)

set_target_properties(gtest gtest_main PROPERTIES FOLDER "Google Test")

# Enable Code Coverage
if(CODE_COVERAGE)
    if(MSVC)
        message(WARNING "Code Coverage on Visual Studio not supported")
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
    endif()
endif()

if(PERF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-exceptions -fno-rtti")
endif()

include_directories(src/include)

add_subdirectory(src/tests)
