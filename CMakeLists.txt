cmake_minimum_required(VERSION 3.16)
project(SystemDashboardPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_executable(cpp-system-dashboard-pro
    src/main.cpp
    src/system_monitor.cpp
    src/task_manager.cpp
    src/ui.cpp
)

target_include_directories(cpp-system-dashboard-pro PRIVATE ${CURSES_INCLUDE_DIR})
target_link_libraries(cpp-system-dashboard-pro PRIVATE ${CURSES_LIBRARIES})

target_compile_options(cpp-system-dashboard-pro PRIVATE -Wall -Wextra -Wpedantic)
