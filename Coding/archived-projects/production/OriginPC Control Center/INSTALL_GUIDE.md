# OriginPC Enhanced Control Center - Complete Installation Guide

## 📦 Available Packages (All Include AI Enhancements)

**Current Version: 5.1.0** - Complete Enhanced Edition with AI Features

### Package Files:
- **Arch Linux**: `originpc-enhanced-control-5.1.0-corrected.pkg.tar.zst` (120KB)
- **Debian/Ubuntu**: `originpc-enhanced-control-5.1.0-corrected.deb` (71KB)  
- **RHEL/Fedora/SUSE**: `originpc-enhanced-control-5.1.0-corrected.rpm` (73KB)

---

## 🚀 Quick Installation

### For Garuda Linux (Arch-based):
```bash
cd /home/lou/rgb-project/packaging
sudo pacman -U originpc-enhanced-control-5.1.0-corrected.pkg.tar.zst
```

### For Other Distributions:

**Debian/Ubuntu/Mint:**
```bash
cd /home/lou/rgb-project/packaging
sudo dpkg -i originpc-enhanced-control-5.1.0-corrected.deb
sudo apt-get install -f  # Fix any missing dependencies
```

**RHEL/CentOS/Fedora:**
```bash
cd /home/lou/rgb-project/packaging
sudo rpm -i originpc-enhanced-control-5.1.0-corrected.rpm
# Or for Fedora:
sudo dnf install originpc-enhanced-control-5.1.0-corrected.rpm
```

**openSUSE:**
```bash
cd /home/lou/rgb-project/packaging
sudo zypper install originpc-enhanced-control-5.1.0-corrected.rpm
```

---

## 📋 Prerequisites

### Required Dependencies:
- **Python 3.8+**
- **PyQt5** (`python3-pyqt5`)
- **psutil** (`python3-psutil`)

### Optional Dependencies (for enhanced features):
- **GPUtil** (`python-gputil`) - GPU monitoring
- **sensors** (`lm-sensors`) - Hardware monitoring
- **NBFC** - Fan control (for laptops)
- **TLP** - Power management

### Install Dependencies:

**Arch Linux/Garuda:**
```bash
sudo pacman -S python python-pyqt5 python-psutil
# Optional:
sudo pacman -S python-gputil lm-sensors tlp
```

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install python3 python3-pyqt5 python3-psutil
# Optional:
sudo apt install python3-gputil lm-sensors tlp
```

**RHEL/Fedora:**
```bash
sudo dnf install python3 python3-qt5 python3-psutil
# Optional:
sudo dnf install python3-gputil lm-sensors tlp
```

---

## 🛠️ Post-Installation Setup

### 1. RGB Device Permissions:
The package automatically configures udev rules, but if needed:
```bash
sudo chmod 666 /dev/hidraw0
# Or reload udev rules:
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 2. Enable Hardware Monitoring:
```bash
# Detect hardware sensors
sudo sensors-detect
# Answer 'YES' to all questions

# Load sensor modules
sudo modprobe coretemp  # CPU temperatures
sudo modprobe it87      # Motherboard sensors
```

### 3. Optional: Enable TLP Power Management:
```bash
sudo systemctl enable tlp
sudo systemctl start tlp
```

---

## 🎮 Running the Application

### Command Line:
```bash
originpc-enhanced-control
```

### Desktop Application:
- Look for "OriginPC Enhanced Control Center" in your applications menu
- Or run from the command line with options:

```bash
# Start normally
originpc-enhanced-control

# Start minimized
originpc-enhanced-control --minimized

# Start to system tray
originpc-enhanced-control --tray
```

---

## ✨ Complete Feature List

### 🎨 RGB Control:
- **Individual key control** - Set any key to any color
- **Group controls** - WASD, arrow keys, function keys, etc.
- **Advanced effects** - Rainbow waves, breathing, gaming mode
- **Color presets** - Quick access to common colors
- **Persistent settings** - Remembers your preferences

### 🧠 AI Enhancements:
- **Color Theory Engine** - Generates harmonious color palettes
- **Adaptive Learning** - Learns your RGB preferences over time  
- **Mood-based Colors** - Colors that match your workflow
- **Smart Patterns** - AI-generated lighting effects

### 🔧 System Integration:
- **Fan Control** - NBFC integration for laptop fans
- **Power Management** - TLP integration and CPU scaling
- **Temperature Monitoring** - Real-time CPU/GPU/storage temps
- **Performance Analytics** - Predictive system monitoring

### 💻 Laptop Features:
- **Lid Monitoring** - Auto-clear RGB when lid closes
- **EON17-X Optimized** - Special support for OriginPC laptops
- **Multi-device Support** - Works with various RGB keyboards

### 🖥️ Interface:
- **Professional Dark Theme** - Easy on the eyes
- **System Tray Integration** - Run in background
- **Real-time Monitoring** - Live system stats
- **Tabbed Interface** - Organized controls

---

## 🔧 Performance Optimizations

### Included Optimizations:
- **RGB Command Batching** - Faster color changes
- **System Information Caching** - Reduced CPU usage
- **Multi-device Support** - Better hardware compatibility  
- **Error Recovery** - Automatic problem resolution
- **Memory Management** - Efficient resource usage

---

## 🚨 Troubleshooting

### RGB Not Working:
```bash
# Check device permissions
ls -la /dev/hidraw*
sudo chmod 666 /dev/hidraw0

# Check if device exists
lsusb | grep -i origin
```

### Application Won't Start:
```bash
# Check dependencies
python3 -c "import PyQt5; import psutil; print('Dependencies OK')"

# Run with debug output
originpc-enhanced-control --verbose
```

### Permission Issues:
```bash
# Ensure user is in correct groups
sudo usermod -a -G input,plugdev $USER
# Log out and back in
```

### Fan Control Not Working:
```bash
# Install NBFC for laptop fan control
# Check NBFC documentation for your laptop model
```

---

## 📁 File Locations

### Application Files:
- **Main executable**: `/usr/bin/originpc-enhanced-control`
- **Source code**: `/usr/share/originpc-control/src/`
- **Desktop entry**: `/usr/share/applications/originpc-enhanced-control.desktop`

### Configuration:
- **User config**: `~/.config/enhanced-originpc-control/`
- **Settings**: `~/.config/enhanced-originpc-control/settings.json`
- **AI data**: `~/.config/enhanced-originpc-control/ai_data/`

### System Files:
- **Udev rules**: `/etc/udev/rules.d/99-originpc-rgb.rules`
- **Service files**: `/etc/systemd/system/originpc-lid-monitor.service`

---

## 🔄 Uninstallation

### Arch Linux:
```bash
sudo pacman -R originpc-enhanced-control
```

### Debian/Ubuntu:
```bash
sudo apt remove originpc-enhanced-control
```

### RHEL/Fedora:
```bash
sudo rpm -e originpc-enhanced-control
# Or: sudo dnf remove originpc-enhanced-control
```

### Clean User Data:
```bash
rm -rf ~/.config/enhanced-originpc-control/
```

---

## 📞 Support

- **Compatible Hardware**: OriginPC EON17-X, other RGB keyboards
- **Tested Distributions**: Garuda Linux, Ubuntu, Fedora, openSUSE
- **Python Version**: 3.8+ required
- **Architecture**: x86_64 (Intel/AMD)

---

**Version**: 5.1.0 Enhanced Edition with AI Features  
**Package Date**: 2025-06-20  
**Total Features**: 50+ advanced features included
