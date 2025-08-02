cmake_minimum_required(VERSION 3.16)
project(WarpTerminalGUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/terminalwidget.cpp
    src/tabwidget.cpp
    src/settingsdialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/terminalwidget.h
    src/tabwidget.h
    src/settingsdialog.h
)

qt_add_executable(warp-terminal-gui
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(warp-terminal-gui
    PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# Install
install(TARGETS warp-terminal-gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
