cmake_minimum_required(VERSION 3.25)
project(ArchBackupPro VERSION 0.0.1 LANGUAGES CXX)

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
    src/restoremanager.cpp
    src/restoremanager.h
    src/backupjob.cpp
    src/backupjob.h
    src/monitoringmanager.cpp
    src/monitoringmanager.h
)

# Add resources
qt6_add_resources(SOURCES resources/resources.qrc)

qt6_add_executable(ArchBackupPro ${SOURCES})

target_link_libraries(ArchBackupPro PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Concurrent
)

# Install target
install(TARGETS ArchBackupPro DESTINATION bin)

# Desktop file
configure_file(
    ${CMAKE_SOURCE_DIR}/archbackuppro.desktop.in
    ${CMAKE_BINARY_DIR}/archbackuppro.desktop
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/archbackuppro.desktop 
        DESTINATION share/applications)

# Install monitoring daemon files
install(FILES archbackuppro-monitor
    DESTINATION usr/local/bin/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES archbackuppro-monitor.service
    DESTINATION etc/systemd/system/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(FILES install-monitor.sh
    DESTINATION share/archbackuppro/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
