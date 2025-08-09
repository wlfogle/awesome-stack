# OriginPC Enhanced Control Center - Installation Guide

## 🚀 Quick Installation

### Ubuntu/Debian (.deb)
```bash
# Download and install
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control_5.1.0_all.deb
sudo apt install ./originpc-enhanced-control_5.1.0_all.deb

# Launch
originpc-control-center
```

### Fedora/RHEL/CentOS (.rpm)
```bash
# Download and install
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control-5.1.0-1.noarch.rpm
sudo dnf install ./originpc-enhanced-control-5.1.0-1.noarch.rpm

# Launch
originpc-control-center
```

### Arch Linux/Manjaro (AUR)
```bash
# Using yay
yay -S originpc-enhanced-control

# Manual from AUR
git clone https://aur.archlinux.org/originpc-enhanced-control.git
cd originpc-enhanced-control
makepkg -si
```

### Flatpak
```bash
# Install from Flathub
flatpak install flathub org.originpc.ControlCenter

# Launch
flatpak run org.originpc.ControlCenter

# Grant hardware access (required for RGB control)
flatpak override --device=all org.originpc.ControlCenter

# Grant system monitoring access
flatpak override --filesystem=/proc/acpi:ro --filesystem=/sys/class/hwmon:ro org.originpc.ControlCenter
```

### Snap
```bash
# Install from Snap Store
sudo snap install originpc-control-center

# Launch
originpc-control-center

# Connect required interfaces for hardware access
sudo snap connect originpc-control-center:hardware-observe
sudo snap connect originpc-control-center:hidraw
sudo snap connect originpc-control-center:raw-usb

# Connect system monitoring interfaces
sudo snap connect originpc-control-center:system-observe
sudo snap connect originpc-control-center:hardware-observe
```

### AppImage
```bash
# Download AppImage
wget https://github.com/loufogle/originpc-control-center/releases/latest/download/OriginPC_Control_Center-1.0.0-x86_64.AppImage

# Make executable
chmod +x OriginPC_Control_Center-1.0.0-x86_64.AppImage

# Run
./OriginPC_Control_Center-1.0.0-x86_64.AppImage

# Note: You may need to configure udev rules for RGB device access
sudo tee /etc/udev/rules.d/60-originpc-rgb.rules << 'EOF'
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0x1044", ATTRS{idProduct}=="0x7a39", MODE="0666"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Universal (Any Linux)
```bash
# Download and extract
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control-5.1.0-universal.tar.gz
tar -xzf originpc-enhanced-control-5.1.0-universal.tar.gz
cd originpc-enhanced-control

# Install
sudo ./universal/install.sh

# Launch
originpc-control-center
```

## 🔧 Manual Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install python3 python3-pip python3-pyqt5 python3-psutil

# Fedora/CentOS/RHEL
sudo dnf install python3 python3-pip python3-qt5 python3-psutil

# Arch/Manjaro
sudo pacman -S python python-pip python-pyqt5 python-psutil

# openSUSE
sudo zypper install python3 python3-pip python3-qt5 python3-psutil
```

### Install from Source
```bash
# Clone repository
git clone https://github.com/user/originpc-enhanced-control.git
cd originpc-enhanced-control

# Run universal installer
sudo ./universal/install.sh
```

## 🎮 Usage

### Desktop Application
```bash
# Launch main GUI
originpc-control-center

# Launch minimized to system tray
originpc-control-center --tray

# Launch minimized
originpc-control-center --minimized
```

### Command Line Tools
```bash
# Fix KP_Plus cyan issue (main fix)
originpc-rgb-fix

# Clear all RGB lighting
originpc-rgb-clear

# System monitoring
originpc-monitor
```

### System Services
```bash
# Enable automatic lid monitoring
sudo systemctl enable --now originpc-lid-monitor

# Check service status
systemctl status originpc-lid-monitor

# View service logs
journalctl -u originpc-lid-monitor -f
```

## 🐛 Troubleshooting

### RGB Control Not Working

#### Check Device Access
```bash
# List HID devices
ls -la /dev/hidraw*

# Check permissions
lsusb | grep -i rgb
```

#### Fix Permissions (Temporary)
```bash
sudo chmod 666 /dev/hidraw0
```

#### Fix Permissions (Permanent)
```bash
# Add user to groups
sudo usermod -a -G dialout,plugdev $USER

# Create udev rule
sudo tee /etc/udev/rules.d/99-originpc-rgb.rules << 'EOF'
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1038", MODE="0666", GROUP="users"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0c45", MODE="0666", GROUP="users"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Log out and back in
```

### KP_Plus Cyan Issue

The KP_Plus key lighting up cyan is a **hardware-level issue**:

```bash
# Manual fix (run after lid opens)
originpc-rgb-fix

# Alternative minimal fix
python3 /usr/share/originpc-control/src/originpc-rgb-fix.py
```

**Why this happens:**
- Hardware firmware default state
- Cannot be prevented, only cleared after it appears
- Wait 10-30 seconds after opening lid before running fix

### Dependencies Missing

#### Python Dependencies
```bash
# Install via pip (if system packages unavailable)
pip3 install PyQt5 psutil setuptools

# Or use virtual environment
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

#### System Dependencies
```bash
# Ubuntu/Debian
sudo apt install python3-dev build-essential

# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install python3-devel

# Arch
sudo pacman -S base-devel python-pip
```

### Service Issues

#### Lid Monitor Not Working
```bash
# Check service status
systemctl status originpc-lid-monitor

# Restart service
sudo systemctl restart originpc-lid-monitor

# Check logs for errors
journalctl -u originpc-lid-monitor --since "10 minutes ago"
```

#### Manual Service Start
```bash
# Test service manually
sudo python3 /usr/share/originpc-control/src/lid-monitor-daemon.py
```

### Application Crashes

#### Check Dependencies
```bash
# Test PyQt5
python3 -c "from PyQt5.QtWidgets import QApplication; print('PyQt5 OK')"

# Test psutil
python3 -c "import psutil; print('psutil OK')"
```

#### Run with Debug
```bash
# Run with verbose output
python3 /usr/share/originpc-control/src/enhanced-professional-control-center.py --debug
```

#### Check Logs
```bash
# Application logs
tail -f /var/log/originpc-control/app.log

# System logs
journalctl -f | grep originpc
```

## 🔍 Advanced Configuration

### Custom RGB Device
```bash
# Edit configuration
sudo nano /etc/originpc-control/config.conf

# Add custom device path
RGB_DEVICE=/dev/hidraw1
```

### Performance Tuning
```bash
# Reduce monitoring frequency
echo "MONITOR_INTERVAL=2000" | sudo tee -a /etc/originpc-control/config.conf

# Disable GPU monitoring if not needed
echo "ENABLE_GPU_MONITOR=false" | sudo tee -a /etc/originpc-control/config.conf
```

### Network Access (Optional)
```bash
# For remote monitoring (advanced users)
echo "ENABLE_NETWORK=true" | sudo tee -a /etc/originpc-control/config.conf
echo "NETWORK_PORT=8080" | sudo tee -a /etc/originpc-control/config.conf
```

## 📦 Package Building

### Build All Packages
```bash
./build-packages.sh
```

### Build Specific Package
```bash
./build-packages.sh debian    # .deb package
./build-packages.sh rpm       # .rpm package  
./build-packages.sh arch      # .pkg.tar.xz package
./build-packages.sh flatpak   # Flatpak package
./build-packages.sh snap      # Snap package
./build-packages.sh appimage  # AppImage package
./build-packages.sh universal # .tar.gz package
```

### Requirements for Building
```bash
# Debian package
sudo apt install dpkg-dev

# RPM package  
sudo dnf install rpm-build

# Arch package
sudo pacman -S base-devel

# Flatpak package
sudo apt install flatpak flatpak-builder # Debian/Ubuntu
sudo dnf install flatpak flatpak-builder # Fedora

# Snap package
sudo apt install snapcraft # Debian/Ubuntu
sudo dnf install snapd # Fedora

# AppImage package
sudo apt install wget fuse # Dependencies for AppImage building
```

## 🆘 Getting Help

### GitHub Issues
https://github.com/user/originpc-enhanced-control/issues

### Community Support
- Reddit: r/OriginPC
- Discord: OriginPC Linux Community

### Documentation
- Wiki: https://github.com/user/originpc-enhanced-control/wiki
- FAQ: docs/FAQ.md

---

**Installation successful? Run `originpc-control-center` to get started!** 🎉

