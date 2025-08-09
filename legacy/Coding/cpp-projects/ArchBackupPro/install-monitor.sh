#!/bin/bash

# ArchBackupPro Monitoring Daemon Installation Script
# This script installs the monitoring daemon as a systemd service

set -e

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root (use sudo)"
    exit 1
fi

echo "Installing ArchBackupPro Real-time Monitoring Daemon..."

# Copy the monitoring script to the system location
echo "Installing monitoring daemon script..."
cp archbackuppro-monitor /usr/local/bin/
chmod +x /usr/local/bin/archbackuppro-monitor

# Copy the systemd service file
echo "Installing systemd service..."
cp archbackuppro-monitor.service /etc/systemd/system/

# Create necessary directories
echo "Creating system directories..."
mkdir -p /var/log/archbackuppro
mkdir -p /var/lib/archbackuppro

# Set appropriate permissions
echo "Setting permissions..."
chown root:root /usr/local/bin/archbackuppro-monitor
chown root:root /etc/systemd/system/archbackuppro-monitor.service
chmod 644 /etc/systemd/system/archbackuppro-monitor.service

# Reload systemd daemon
echo "Reloading systemd daemon..."
systemctl daemon-reload

# Enable the service to start at boot
echo "Enabling service to start at boot..."
systemctl enable archbackuppro-monitor.service

# Start the service
echo "Starting monitoring service..."
systemctl start archbackuppro-monitor.service

# Check service status
echo ""
echo "Installation complete! Service status:"
systemctl status archbackuppro-monitor.service --no-pager

echo ""
echo "Monitoring daemon has been installed and started successfully!"
echo ""
echo "Useful commands:"
echo "  Check status:     systemctl status archbackuppro-monitor"
echo "  View logs:        journalctl -u archbackuppro-monitor -f"
echo "  View monitor log: tail -f /var/log/archbackuppro/monitor.log"
echo "  Stop service:     systemctl stop archbackuppro-monitor"
echo "  Restart service:  systemctl restart archbackuppro-monitor"
echo "  Disable service:  systemctl disable archbackuppro-monitor"
echo ""
