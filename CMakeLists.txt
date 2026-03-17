cmake_minimum_required(VERSION 3.16)
project(CppDistributedDashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ system-monitor agent
add_subdirectory(agent)
