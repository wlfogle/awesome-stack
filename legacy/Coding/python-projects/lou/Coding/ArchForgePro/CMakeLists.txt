cmake_minimum_required(VERSION 3.25)
project(ArchForgePro VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)

qt6_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/backupmanager.cpp
    src/backupmanager.h
    src/packagemanager.cpp
    src/packagemanager.h
    src/settingsmanager.cpp
    src/settingsmanager.h
    src/aioptimizer.cpp
    src/aioptimizer.h
    src/restoremanager.cpp
    src/restoremanager.h
    src/backupjob.cpp
    src/backupjob.h
)

# Add resources
qt6_add_resources(SOURCES resources/resources.qrc)

qt6_add_executable(ArchForgePro ${SOURCES})

target_link_libraries(ArchForgePro PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Concurrent
)

# Install target
install(TARGETS ArchForgePro DESTINATION bin)

# Desktop file
configure_file(
    ${CMAKE_SOURCE_DIR}/archforgepro.desktop.in
    ${CMAKE_BINARY_DIR}/archforgepro.desktop
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/archforgepro.desktop 
        DESTINATION share/applications)
