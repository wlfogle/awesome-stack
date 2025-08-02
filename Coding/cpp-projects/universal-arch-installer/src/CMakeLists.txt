cmake_minimum_required(VERSION 3.16)
project(UniversalArchInstaller VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    searchtab.cpp
    installtab.cpp
    packagemanager.cpp
    searchthread.cpp
    packageinfo.cpp
)

# Header files
set(HEADERS
    mainwindow.h
    packagemanager.h
    searchthread.h
    packageinfo.h
)

# Create executable
add_executable(universal-arch-installer ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(universal-arch-installer 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Concurrent
)

# Compiler flags
target_compile_options(universal-arch-installer PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Install target
install(TARGETS universal-arch-installer
    RUNTIME DESTINATION bin
)
