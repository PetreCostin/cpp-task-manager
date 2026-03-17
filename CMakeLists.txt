cmake_minimum_required(VERSION 3.16)
project(TaskManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

add_executable(task-manager src/main.cpp src/task_manager.cpp)

target_include_directories(task-manager PRIVATE ${CURSES_INCLUDE_DIR} src)
target_link_libraries(task-manager PRIVATE ${CURSES_LIBRARIES} Threads::Threads)

target_compile_options(task-manager PRIVATE -Wall -Wextra -Wpedantic)
