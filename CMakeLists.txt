cmake_minimum_required(VERSION ${CMAKE_VERSION})
project(cpp_hashset)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(tests)

add_executable(basic_test
        Hashset.hpp
        tests/basic.cpp
)

add_executable(advanced_test
        Hashset.hpp
        tests/advanced.cpp
)

add_executable(extreme_test
        Hashset.hpp
        tests/extreme.cpp
)