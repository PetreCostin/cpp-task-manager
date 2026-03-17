cmake_minimum_required(VERSION 3.10)
project(cpp_system_dashboard)

set(CMAKE_CXX_STANDARD 17)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(cpp-system-dashboard
    src/main.cpp
)

target_link_libraries(cpp-system-dashboard ${CURSES_LIBRARIES})
