cmake_minimum_required(VERSION 3.16)

project(Qt6SimpleApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt6_standard_project_setup()

add_executable(Qt6SimpleApp main.cpp)

target_link_libraries(Qt6SimpleApp Qt6::Core Qt6::Widgets)
