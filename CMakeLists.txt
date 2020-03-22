
cmake_minimum_required(VERSION 3.3...3.10)

find_package(Doxygen
    OPTIONAL_COMPONENTS dot mscgen dia)

project(stringbuilder
    VERSION     1.1
    DESCRIPTION "Fast, efficient and convenient alternative to std::stringstream and std::string concatenation."
    LANGUAGES   CXX)

add_library(stringbuilder INTERFACE)
target_compile_features(stringbuilder INTERFACE cxx_std_11)
target_include_directories(stringbuilder INTERFACE include)

if(CMAKE_PROJECT_NAME STREQUAL stringbuilder AND DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_HTML YES)
    doxygen_add_docs(stringbuilder.doc include/stringbuilder.h ALL)
endif()

# Enable Code Coverage
# TODO: Make it right.
if(CODE_COVERAGE)
    if(MSVC)
        message(WARNING "Code Coverage on Visual Studio not supported")
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL stringbuilder)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()
