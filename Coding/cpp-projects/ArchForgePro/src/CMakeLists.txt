cmake_minimum_required(VERSION 3.16)

project(ArchForgePro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include Qt6 directories
get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt")

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    cleaninstallbackuprestore_widget.cpp
    softwaremanagement_widget.cpp
    rgbfancontrol_widget.cpp
    kerneltools_widget.cpp
    aiassistant_widget.cpp
    settings_widget.cpp
    # ArchBackupPro core components
    backupmanager.cpp
    restoremanager.cpp
    packagemanager.cpp
    settingsmanager.cpp
    aioptimizer.cpp
)

# Header files
set(HEADERS
    mainwindow.h
    cleaninstallbackuprestore_widget.h
    softwaremanagement_widget.h
    rgbfancontrol_widget.h
    kerneltools_widget.h
    aiassistant_widget.h
    settings_widget.h
    # ArchBackupPro core components
    backupmanager.h
    restoremanager.h
    packagemanager.h
    settingsmanager.h
    aioptimizer.h
)

# Create the executable
add_executable(ArchForgePro ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(ArchForgePro
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
)

# Set target properties
set_target_properties(ArchForgePro PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Install rules
install(TARGETS ArchForgePro
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
