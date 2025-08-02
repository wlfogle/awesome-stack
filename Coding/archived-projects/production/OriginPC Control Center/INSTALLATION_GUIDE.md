# OriginPC Enhanced Control Center v5.1.0 - Installation Guide

## Package Overview

This production release includes the following packages:

✅ **Available Packages:**
- `originpc-enhanced-control-5.1.0-corrected.deb` - Debian/Ubuntu package
- `originpc-enhanced-control-5.1.0-corrected.rpm` - Red Hat/Fedora/SUSE package  
- `originpc-enhanced-control-5.1.0-corrected.pkg.tar.zst` - Arch Linux package
- `OriginPC-Enhanced-Control-5.1.0.AppImage` - Universal Linux AppImage

❌ **Missing Packages (require manual building):**
- Snap package (configuration available in packaging/snap/)
- Flatpak package (configuration available in packaging/flatpak/)

## Installation Instructions

### Debian/Ubuntu (.deb)
```bash
# Install package
sudo dpkg -i originpc-enhanced-control-5.1.0-corrected.deb

# Install dependencies if needed
sudo apt-get install -f

# Verify installation
originpc-enhanced-control --version
```

### Red Hat/Fedora/CentOS (.rpm)
```bash
# Install package
sudo rpm -i originpc-enhanced-control-5.1.0-corrected.rpm

# Or using dnf/yum
sudo dnf install originpc-enhanced-control-5.1.0-corrected.rpm

# Verify installation
originpc-enhanced-control --version
```

### Arch Linux (.pkg.tar.zst)
```bash
# Install package
sudo pacman -U originpc-enhanced-control-5.1.0-corrected.pkg.tar.zst

# Verify installation
originpc-enhanced-control --version
```

### Universal AppImage
```bash
# Make executable
chmod +x OriginPC-Enhanced-Control-5.1.0.AppImage

# Run directly
./OriginPC-Enhanced-Control-5.1.0.AppImage

# Optional: Install system-wide
sudo mv OriginPC-Enhanced-Control-5.1.0.AppImage /usr/local/bin/originpc-enhanced-control
```

## System Requirements

- **Operating System:** Linux (any distribution)
- **Python:** 3.6+ 
- **Dependencies:**
  - python3-pyqt5 (Qt5 Python bindings)
  - python3-psutil (System monitoring)
  - python3 (Python runtime)

**Optional Dependencies:**
- python3-gputil (GPU monitoring)
- lm-sensors (Enhanced temperature monitoring)
- fancontrol (Advanced fan control)
- tlp (Power management integration)

## Post-Installation Setup

### 1. Device Permissions
The package automatically configures udev rules for RGB device access. If manual setup is needed:

```bash
# Create udev rule
sudo tee /etc/udev/rules.d/99-originpc-rgb.rules << EOF
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1b1c", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0c45", MODE="0666"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 2. Enable Lid Monitor Service (Optional)
For automatic RGB clearing when laptop lid is closed:

```bash
# Enable for current user
sudo systemctl enable originpc-lid-monitor@$USER.service
sudo systemctl start originpc-lid-monitor@$USER.service
```

### 3. Launch Application
```bash
# Command line
originpc-enhanced-control

# Or find "OriginPC Enhanced Control Center" in your application menu
```

## Features

### ✨ Core Features
- **Complete RGB Control:** Individual key control with 16.7M colors
- **Advanced Effects:** Wave, radial, breathing, rainbow, and custom patterns
- **System Monitoring:** Real-time CPU, memory, temperature, and fan monitoring
- **Lid Monitoring:** Automatic RGB clearing when laptop is closed
- **Power Management:** TLP integration for optimal battery life
- **System Tray:** Background operation with tray integration
- **Professional UI:** Dark theme with high DPI scaling support

### 🚀 Enhanced Features (v5.1)
- **AI Effects Engine:** Intelligent color theory and adaptive learning
- **Hardware Optimizations:** Advanced threading and performance tuning
- **System Intelligence:** Predictive analytics and resource management
- **Enhanced Monitoring:** Comprehensive sensor data collection
- **Professional Interface:** Nyx-inspired system monitoring

## Configuration

Configuration files are stored in:
- `~/.config/enhanced-originpc-control/`

Key configuration files:
- `settings.json` - Application settings
- `profiles.json` - RGB profiles and effects
- `effect_state.json` - Persistent effect state
- `ai_usage.json` - AI enhancement usage data

## Troubleshooting

### Common Issues

**1. Permission Denied for RGB Device**
```bash
# Check device permissions
ls -la /dev/hidraw*
# Should show mode 666 (readable/writable by all)

# Manually set permissions if needed
sudo chmod 666 /dev/hidraw0
```

**2. Python Dependencies Missing**
```bash
# Install missing dependencies
sudo apt install python3-pyqt5 python3-psutil  # Debian/Ubuntu
sudo dnf install python3-qt5 python3-psutil    # Fedora
sudo pacman -S python-pyqt5 python-psutil      # Arch
```

**3. Application Won't Start**
```bash
# Run with debug output
originpc-enhanced-control --debug

# Check system logs
journalctl -f | grep originpc
```

**4. Effects Not Persisting**
- Ensure configuration directory has write permissions
- Check if lid monitor service is interfering
- Verify udev rules are properly loaded

### Getting Help

- Check application logs in `~/.config/enhanced-originpc-control/`
- Use `--debug` flag for verbose output
- Ensure all dependencies are installed
- Verify hardware compatibility

## Hardware Compatibility

**Supported OriginPC Models:**
- Neuron series laptops
- Millennium series laptops  
- Other OriginPC laptops with RGB keyboards

**RGB Controller Support:**
- Vendor ID: 1b1c (OriginPC)
- Vendor ID: 0c45 (Generic RGB controllers)

## Uninstallation

### Debian/Ubuntu
```bash
sudo apt remove originpc-enhanced-control
```

### Red Hat/Fedora
```bash
sudo rpm -e originpc-enhanced-control
```

### Arch Linux
```bash
sudo pacman -R originpc-enhanced-control
```

### AppImage
```bash
# Simply delete the AppImage file
rm /usr/local/bin/originpc-enhanced-control
```

### Clean Configuration
```bash
# Remove configuration files
rm -rf ~/.config/enhanced-originpc-control/
```

---

**Version:** 5.1.0-corrected  
**Build Date:** 2025-06-20  
**Package Contents Verified:** ✅ Correct enhanced source code included
