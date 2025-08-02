cmake_minimum_required(VERSION 3.16)
project(OpenInterpreterGUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components with error checking
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)
find_package(Qt6 OPTIONAL_COMPONENTS TextToSpeech)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found! Please install Qt6 development packages.")
endif()

# Verify individual components
if(NOT Qt6Core_FOUND)
    message(FATAL_ERROR "Qt6Core not found!")
endif()
if(NOT Qt6Widgets_FOUND)
    message(FATAL_ERROR "Qt6Widgets not found!")
endif()
if(NOT Qt6Network_FOUND)
    message(FATAL_ERROR "Qt6Network not found!")
endif()
if(NOT Qt6Concurrent_FOUND)
    message(FATAL_ERROR "Qt6Concurrent not found!")
endif()

# Qt6 MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/chatwidget.cpp
    src/codeeditor.cpp
    src/modelconfigwidget.cpp
    src/filemanagerwidget.cpp
    src/interpreterworker.cpp
    src/aiassistantmanager.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/chatwidget.h
    src/codeeditor.h
    src/modelconfigwidget.h
    src/filemanagerwidget.h
    src/interpreterworker.h
    src/aiassistantmanager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# Link optional components
if(Qt6TextToSpeech_FOUND)
    target_link_libraries(${PROJECT_NAME} Qt6::TextToSpeech)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_TEXTTOSPEECH)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
