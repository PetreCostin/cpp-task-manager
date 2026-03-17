cmake_minimum_required(VERSION 3.16)
project(SystemMonitorAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(agent
    main.cpp
    system_monitor.cpp
)

target_compile_options(agent PRIVATE -Wall -Wextra -Wpedantic)
