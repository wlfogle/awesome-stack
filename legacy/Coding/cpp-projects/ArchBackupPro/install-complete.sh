#!/bin/bash

# ArchBackupPro Complete Installation Script
# This script builds and installs ArchBackupPro with the monitoring daemon

set -e

echo "========================================"
echo "ArchBackupPro Complete Installation"
echo "========================================"

# Check if running as root for daemon installation
if [[ $EUID -eq 0 ]]; then
    echo "Running as root - will install system-wide"
    INSTALL_USER_ONLY=false
else
    echo "Running as user - will require sudo for monitoring daemon"
    INSTALL_USER_ONLY=false
fi

# Check for required dependencies
echo "Checking dependencies..."

MISSING_DEPS=()

if ! command -v g++ &> /dev/null; then
    MISSING_DEPS+=("g++")
fi

if ! command -v cmake &> /dev/null; then
    MISSING_DEPS+=("cmake")
fi

if ! command -v qmake6 &> /dev/null && ! command -v qmake-qt6 &> /dev/null; then
    MISSING_DEPS+=("qt6-base")
fi

if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null; then
    MISSING_DEPS+=("ninja or make")
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "ERROR: Missing dependencies: ${MISSING_DEPS[*]}"
    echo "Please install them first:"
    echo "  sudo pacman -S base-devel cmake qt6-base ninja"
    exit 1
fi

echo "All dependencies found!"

# Build monitoring daemon
echo ""
echo "Building monitoring daemon..."
cd src
mkdir -p build-monitoring
cd build-monitoring

# Copy and configure CMake file
cp ../CMakeLists_monitoring.txt ./CMakeLists.txt

# Update paths in CMakeLists.txt
sed -i 's|../monitoringclass.cpp|../monitoringclass.cpp|g' CMakeLists.txt
sed -i 's|../monitoring_daemon.cpp|../monitoring_daemon.cpp|g' CMakeLists.txt
sed -i 's|../monitoringclass.h|../monitoringclass.h|g' CMakeLists.txt

# Build
cmake -DCMAKE_BUILD_TYPE=Release .
if command -v ninja &> /dev/null; then
    ninja
else
    make -j$(nproc)
fi

echo "Monitoring daemon built successfully!"

# Go back to project root
cd ../..

# Build main application
echo ""
echo "Building main application..."
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
if command -v ninja &> /dev/null; then
    ninja
else
    make -j$(nproc)
fi

echo "Main application built successfully!"

# Go back to project root
cd ..

# Install monitoring daemon
echo ""
echo "Installing monitoring daemon..."

if [[ $EUID -ne 0 ]]; then
    echo "Installing monitoring daemon with sudo..."
    
    # Create installation script for sudo
    cat > temp_install_daemon.sh << 'EOF'
#!/bin/bash
set -e

# Copy daemon executable
cp src/build-monitoring/bin/archbackuppro-monitoring-daemon /usr/local/bin/
chmod +x /usr/local/bin/archbackuppro-monitoring-daemon

# Copy service file
cp archbackuppro-monitoring-daemon.service /etc/systemd/system/
chmod 644 /etc/systemd/system/archbackuppro-monitoring-daemon.service

# Create directories
mkdir -p /var/log/archbackuppro
mkdir -p /var/lib/archbackuppro
mkdir -p /run/archbackuppro

# Reload and enable service
systemctl daemon-reload
systemctl enable archbackuppro-monitoring-daemon

echo "Monitoring daemon installed successfully!"
EOF

    chmod +x temp_install_daemon.sh
    sudo ./temp_install_daemon.sh
    rm temp_install_daemon.sh
else
    # Direct installation as root
    cp src/build-monitoring/bin/archbackuppro-monitoring-daemon /usr/local/bin/
    chmod +x /usr/local/bin/archbackuppro-monitoring-daemon
    
    cp archbackuppro-monitoring-daemon.service /etc/systemd/system/
    chmod 644 /etc/systemd/system/archbackuppro-monitoring-daemon.service
    
    mkdir -p /var/log/archbackuppro
    mkdir -p /var/lib/archbackuppro
    mkdir -p /run/archbackuppro
    
    systemctl daemon-reload
    systemctl enable archbackuppro-monitoring-daemon
fi

# Install main application
echo ""
echo "Installing main application..."

if [[ $EUID -ne 0 ]]; then
    echo "Installing main application with sudo..."
    sudo cp build/ArchBackupPro /usr/local/bin/
    sudo chmod +x /usr/local/bin/ArchBackupPro
else
    cp build/ArchBackupPro /usr/local/bin/
    chmod +x /usr/local/bin/ArchBackupPro
fi

# Create desktop entry
echo ""
echo "Creating desktop entry..."

DESKTOP_CONTENT="[Desktop Entry]
Name=ArchBackupPro
Comment=Comprehensive backup and restore solution for Arch Linux
Exec=/usr/local/bin/ArchBackupPro
Icon=archbackuppro
Terminal=false
Type=Application
Categories=System;Utility;
StartupNotify=true"

if [[ $EUID -ne 0 ]]; then
    # Install for current user
    mkdir -p ~/.local/share/applications
    echo "$DESKTOP_CONTENT" > ~/.local/share/applications/archbackuppro.desktop
    chmod +x ~/.local/share/applications/archbackuppro.desktop
else
    # Install system-wide
    echo "$DESKTOP_CONTENT" > /usr/share/applications/archbackuppro.desktop
    chmod +x /usr/share/applications/archbackuppro.desktop
fi

# Start monitoring daemon
echo ""
echo "Starting monitoring daemon..."
if systemctl is-active --quiet archbackuppro-monitoring-daemon; then
    echo "Monitoring daemon is already running."
else
    if [[ $EUID -ne 0 ]]; then
        sudo systemctl start archbackuppro-monitoring-daemon
    else
        systemctl start archbackuppro-monitoring-daemon
    fi
    echo "Monitoring daemon started."
fi

# Installation complete
echo ""
echo "========================================"
echo "Installation completed successfully!"
echo "========================================"
echo ""
echo "Applications installed:"
echo "  - ArchBackupPro: /usr/local/bin/ArchBackupPro"
echo "  - Monitoring Daemon: /usr/local/bin/archbackuppro-monitoring-daemon"
echo ""
echo "Services:"
echo "  - archbackuppro-monitoring-daemon.service (enabled and started)"
echo ""
echo "You can now:"
echo "  1. Launch ArchBackupPro from applications menu"
echo "  2. Run from terminal: ArchBackupPro"
echo "  3. Check monitoring status: systemctl status archbackuppro-monitoring-daemon"
echo "  4. View monitoring logs: tail -f /var/log/archbackuppro/monitor.log"
echo ""
echo "The monitoring daemon will automatically start with the main application"
echo "and provide real-time system monitoring and backup suggestions."
echo ""
