#!/bin/bash

# Clevo Control Center Setup Script for OriginPC EON17-X
# This script sets up the build environment and installs the application

set -e

echo "🎮 Clevo Control Center Setup for OriginPC EON17-X"
echo "================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Check if running on Arch-based system
if ! command -v pacman &> /dev/null; then
    print_error "This script is designed for Arch-based systems (like Garuda Linux)"
    print_error "Please install dependencies manually and build with cmake"
    exit 1
fi

print_status "Detected Arch-based system"

# Check for Qt6 and development tools
print_status "Installing dependencies..."
sudo pacman -S --needed qt6-base qt6-tools cmake gcc ninja cpupower nvidia-utils lm_sensors

# Check if hidraw1 exists
print_status "Checking hardware access..."
if [ -e "/dev/hidraw1" ]; then
    print_success "Found /dev/hidraw1 for RGB control"
else
    print_warning "/dev/hidraw1 not found - RGB control may not work"
fi

# Check hwmon devices
hwmon_count=$(ls /sys/class/hwmon/ 2>/dev/null | wc -l)
if [ "$hwmon_count" -gt 0 ]; then
    print_success "Found $hwmon_count hwmon devices for fan control"
else
    print_warning "No hwmon devices found - fan control may not work"
fi

# Build the application
print_status "Building Clevo Control Center..."
mkdir -p build
cd build

print_status "Configuring with CMake..."
cmake -G Ninja ..

print_status "Compiling..."
ninja

print_success "Build completed successfully!"

# Setup hardware access
print_status "Setting up hardware access..."

# Add user to input group
print_status "Adding user $USER to input group..."
sudo usermod -a -G input $USER

# Create udev rules
print_status "Creating udev rules..."
sudo tee /etc/udev/rules.d/99-clevo-control.rules > /dev/null <<EOF
# Clevo RGB Keyboard Control for OriginPC EON17-X
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1558", MODE="0666", GROUP="input"
SUBSYSTEM=="hidraw", KERNEL=="hidraw1", MODE="0666", GROUP="input"

# Hardware monitoring access
SUBSYSTEM=="hwmon", MODE="0644", GROUP="input"
SUBSYSTEM=="hwmon", ACTION=="add", RUN+="/bin/chmod -R 666 /sys/class/hwmon/%k"
EOF

# Reload udev rules
print_status "Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

# Check if user wants to install system-wide
echo ""
read -p "Install system-wide? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Installing system-wide..."
    sudo ninja install
    print_success "System-wide installation completed!"
    
    echo ""
    print_success "You can now run: ClevoControlCenter"
else
    print_success "Local build completed!"
    echo ""
    print_success "You can run: ./ClevoControlCenter"
fi

# Final instructions
echo ""
echo "=========================================="
print_success "Setup completed!"
echo ""
print_warning "IMPORTANT: You need to log out and back in (or reboot) for group changes to take effect."
echo ""
print_status "To test RGB access after logging back in:"
echo "  ls -la /dev/hidraw1"
echo ""
print_status "To test fan control:"
echo "  find /sys/class/hwmon -name 'fan*_input'"
echo ""
print_status "To run the application:"
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "  ClevoControlCenter"
else
    echo "  cd build && ./ClevoControlCenter"
fi
echo ""
print_warning "Monitor system temperatures when using manual fan control!"
echo "=========================================="
