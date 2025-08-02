cmake_minimum_required(VERSION 3.16)
project(ArchForge-RGB-Control)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Enable Qt's automatic moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    src/main.cpp
    src/rgbcommandbatcher.cpp
    src/mainwindow.cpp
    src/fancontroller.cpp
    src/spatialeffects.cpp
    src/powermanager.cpp
)

# Header files
set(HEADERS
    src/rgbcommandbatcher.h
    src/mainwindow.h
    src/fancontroller.h
    src/spatialeffects.h
    src/powermanager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
