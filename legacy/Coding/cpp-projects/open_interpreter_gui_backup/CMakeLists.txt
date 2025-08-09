cmake_minimum_required(VERSION 3.5)
project(OpenInterpreterGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(OpenInterpreterGUI
    main.cpp
    mainwindow.h
    chatwidget.cpp
)

include_directories(${CMAKE_SOURCE_DIR})

target_link_libraries(OpenInterpreterGUI Qt6::Widgets)
