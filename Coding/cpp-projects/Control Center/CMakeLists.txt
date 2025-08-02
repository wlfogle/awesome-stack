cmake_minimum_required(VERSION 3.16)
project(ClevoControlCenter VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)

# Add executable
add_executable(ClevoControlCenter
    main.cpp
)

# Link Qt6 libraries
target_link_libraries(ClevoControlCenter PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)

# Set compiler flags for better performance and debugging
target_compile_options(ClevoControlCenter PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Install rules
install(TARGETS ClevoControlCenter
    DESTINATION bin
)

# Create desktop entry
set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/clevo-control-center.desktop")
file(WRITE ${DESKTOP_FILE}
"[Desktop Entry]
Name=Clevo Control Center
Comment=Hardware control for Clevo-based laptops
Exec=${CMAKE_INSTALL_PREFIX}/bin/ClevoControlCenter
Icon=preferences-system
Terminal=false
Type=Application
Categories=System;Settings;
")

install(FILES ${DESKTOP_FILE}
    DESTINATION share/applications
)

# Create udev rules for hidraw access
set(UDEV_RULES_FILE "${CMAKE_CURRENT_BINARY_DIR}/99-clevo-control.rules")
file(WRITE ${UDEV_RULES_FILE}
"# Clevo RGB Keyboard Control
SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"1558\", MODE=\"0666\", GROUP=\"input\"
SUBSYSTEM==\"hidraw\", KERNEL==\"hidraw1\", MODE=\"0666\", GROUP=\"input\"

# Hardware monitoring access
SUBSYSTEM==\"hwmon\", MODE=\"0644\", GROUP=\"input\"
SUBSYSTEM==\"hwmon\", ACTION==\"add\", RUN+=\"/bin/chmod -R 666 /sys/class/hwmon/%k\"
")

install(FILES ${UDEV_RULES_FILE}
    DESTINATION /etc/udev/rules.d
    COMPONENT udev-rules
)

# Print post-install instructions
install(CODE "message(STATUS \"\nClevo Control Center installed successfully!\n\nTo use the application:\n1. Add your user to the 'input' group: sudo usermod -a -G input $USER\n2. Reload udev rules: sudo udevadm control --reload-rules\n3. Restart your system or reconnect the device\n4. Run: ClevoControlCenter\n\nFor RGB control, ensure /dev/hidraw1 is accessible.\nFor fan control, ensure hwmon devices are accessible.\n\")")
