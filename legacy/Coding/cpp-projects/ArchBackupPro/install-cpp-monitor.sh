#!/bin/bash

# ArchBackupPro C++ Monitoring Daemon Installation Script
# This script builds and installs the C++ monitoring daemon as a systemd service

set -e

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root (use sudo)"
    exit 1
fi

echo "Installing ArchBackupPro Real-time Monitoring Daemon (C++)..."

# Check for required dependencies
echo "Checking dependencies..."
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Please install build-essential or equivalent."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Please install cmake."
    exit 1
fi

# Navigate to source directory
cd src

# Create build directory
echo "Creating build directory..."
mkdir -p build-monitoring
cd build-monitoring

# Configure with CMake
echo "Configuring build with CMake..."
cmake -f ../CMakeLists_monitoring.txt -DCMAKE_BUILD_TYPE=Release ..

# Build the daemon
echo "Building monitoring daemon..."
make -j$(nproc)

# Install the executable
echo "Installing monitoring daemon executable..."
cp archbackuppro-monitoring-daemon /usr/local/bin/
chmod +x /usr/local/bin/archbackuppro-monitoring-daemon

# Go back to project root
cd ../..

# Copy the systemd service file
echo "Installing systemd service..."
cp archbackuppro-monitoring-daemon.service /etc/systemd/system/

# Create necessary directories
echo "Creating system directories..."
mkdir -p /var/log/archbackuppro
mkdir -p /var/lib/archbackuppro

# Set appropriate permissions
echo "Setting permissions..."
chown root:root /usr/local/bin/archbackuppro-monitoring-daemon
chown root:root /etc/systemd/system/archbackuppro-monitoring-daemon.service
chmod 644 /etc/systemd/system/archbackuppro-monitoring-daemon.service

# Stop old bash service if running
if systemctl is-active --quiet archbackuppro-monitor.service; then
    echo "Stopping old bash monitoring service..."
    systemctl stop archbackuppro-monitor.service
    systemctl disable archbackuppro-monitor.service
fi

# Reload systemd daemon
echo "Reloading systemd daemon..."
systemctl daemon-reload

# Enable the service to start at boot
echo "Enabling service to start at boot..."
systemctl enable archbackuppro-monitoring-daemon.service

# Start the service
echo "Starting monitoring service..."
systemctl start archbackuppro-monitoring-daemon.service

# Check service status
echo ""
echo "Installation complete! Service status:"
systemctl status archbackuppro-monitoring-daemon.service --no-pager

echo ""
echo "C++ Monitoring daemon has been installed and started successfully!"
echo ""
echo "Useful commands:"
echo "  Check status:     systemctl status archbackuppro-monitoring-daemon"
echo "  View logs:        journalctl -u archbackuppro-monitoring-daemon -f"
echo "  View monitor log: tail -f /var/log/archbackuppro/monitor.log"
echo "  Stop service:     systemctl stop archbackuppro-monitoring-daemon"
echo "  Restart service:  systemctl restart archbackuppro-monitoring-daemon"
echo "  Disable service:  systemctl disable archbackuppro-monitoring-daemon"
echo ""
echo "Test foreground mode:"
echo "  /usr/local/bin/archbackuppro-monitoring-daemon --foreground"
echo ""
