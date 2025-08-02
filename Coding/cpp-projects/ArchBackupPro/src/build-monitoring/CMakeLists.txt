cmake_minimum_required(VERSION 3.16)

project(ArchBackupProMonitoring VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Source files for monitoring daemon
set(MONITORING_SOURCES
    ../monitoringclass.cpp
    ../monitoring_daemon.cpp
)

# Header files for monitoring daemon
set(MONITORING_HEADERS
    ../monitoringclass.h
)

# Add include directory
include_directories(..)

# Create the monitoring daemon executable
add_executable(archbackuppro-monitoring-daemon ${MONITORING_SOURCES} ${MONITORING_HEADERS})

# Link libraries
target_link_libraries(archbackuppro-monitoring-daemon
    Threads::Threads
)

# Set target properties
set_target_properties(archbackuppro-monitoring-daemon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install rules
install(TARGETS archbackuppro-monitoring-daemon
    RUNTIME DESTINATION bin
)

# Install service file and script
install(FILES ../archbackuppro-monitor.service
    DESTINATION /etc/systemd/system/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES ../archbackuppro-monitor
    DESTINATION /usr/local/bin/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
