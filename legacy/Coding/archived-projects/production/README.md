# OriginPC Enhanced Control Center
Professional RGB control and system monitoring for OriginPC laptops

## Features
- Complete RGB keyboard control with advanced effects
- System monitoring and performance management
- Fan control and temperature monitoring
- Power management integration
- Automatic lid-close detection
- System tray integration
- Persistent effect profiles

## Installation

### Prerequisites
- Flatpak installed on your system
- KDE Platform runtime (5.15-23.08)

### Method 1: Direct Installation
```bash
# Install KDE Platform runtime if not already installed
flatpak install org.kde.Platform//5.15-23.08

# Install the application
flatpak install originpc-control-center.flatpak
```

### Method 2: Repository Installation
```bash
# Add the remote repository
flatpak remote-add --user originpc-rgb https://your-server.com/repo

# Install the application
flatpak install --user originpc-rgb org.originpc.ControlCenter
```

### Device Permissions Setup
Run the included setup script to configure device permissions:
```bash
sudo ./setup-permissions.sh
```

## Usage

### Starting the Application
```bash
# Normal start
flatpak run org.originpc.ControlCenter

# Start minimized
flatpak run org.originpc.ControlCenter --minimized

# Start in system tray
flatpak run org.originpc.ControlCenter --tray
```

### RGB Control
- Choose colors using the color picker or presets
- Apply effects to individual keys or groups
- Save and load custom effect profiles
- Use the system tray for quick access

### System Monitoring
- CPU and GPU temperature monitoring
- Fan speed control
- Power management profiles
- Memory and disk usage tracking

### Keyboard Profiles
The application includes several pre-configured profiles:
- Gaming Mode (WASD highlight)
- Professional Mode (subtle effects)
- Performance Mode (temperature-based)
- Custom profiles can be created and saved

## Advanced Configuration

### Effect Profiles
Custom effect profiles are stored in:
```
~/.var/app/org.originpc.ControlCenter/config/enhanced-originpc-control/profiles/
```

### System Integration
The application integrates with:
- System power management
- Lid close detection
- Fan control systems
- System notifications

### Autostart Configuration
To configure autostart:
1. Use your desktop environment's autostart settings
2. Add the application with `--minimized` or `--tray` flag
3. Or use the built-in autostart configuration

## Troubleshooting

### Device Access Issues
If you encounter permission issues:
1. Run the setup script again: `sudo ./setup-permissions.sh`
2. Reconnect your RGB devices
3. Verify you're in the 'input' group: `groups`

### Effect Not Working
1. Check device connections
2. Clear all effects: Right-click tray icon → Quick Clear All
3. Restart the application

### System Monitoring Issues
1. Verify required permissions are set
2. Check system sensor access
3. Install lm-sensors if needed

## Support
- Report issues on GitHub
- Check the wiki for advanced configurations
- Join our community for support

## License
This software is licensed under MIT License. See LICENSE file for details.

# OriginPC Control Center Documentation

## 1. Program Overview

OriginPC Control Center is a comprehensive utility designed specifically for OriginPC laptops running Linux. The application provides complete RGB keyboard control along with extensive system monitoring and management capabilities. Built with a professional dark theme UI, the software integrates seamlessly with various Linux desktop environments and includes system tray functionality for background operation.

The application is designed to address the limited RGB control options for OriginPC laptops under Linux, with special focus on EON17-X models. It features enhanced keyboard clearing routines that specifically target problematic keys (like the keypad plus key) that often retain color settings after clearing.

## 2. Main Features

### RGB Lighting Control
- **Complete Keyboard Control**: Individual key and key group RGB control
- **Enhanced Key Clearing**: Special algorithms for clearing stubborn keys (keypad plus)
- **Dynamic Effects**: Multiple lighting effects including wave patterns, breathing, and reactive typing
- **Persistent Settings**: Effects and colors persist across application restarts
- **Gaming Mode**: Quick setup for gaming-focused key highlights

### System Monitoring & Management
- **Real-time Monitoring**: CPU, memory, disk usage and temperature monitoring
- **Comprehensive Temperature Tracking**: CPU, GPU, storage, and memory thermal monitoring
- **Fan Control**: Integration with NBFC and other fan control systems
- **Power Management**: TLP integration for power profiles (Performance, Balanced, Power Save)
- **Lid Monitoring**: Advanced lid state detection with automatic RGB clearing on lid close

### User Interface
- **Professional Dark Theme**: Modern, eye-friendly dark UI with high DPI scaling
- **System Tray Integration**: Minimize to tray for background operation
- **Tab-based Organization**: Organized interface with separate tabs for different functionality
- **Performance Monitoring**: Real-time graphical displays of system performance

## 3. Package-specific Information

### Debian/Ubuntu (.deb)
- **Package Name**: originpc-enhanced-control
- **Installation Path**: /usr/share/originpc-control/
- **Config Location**: ~/.config/enhanced-originpc-control/
- **Executable**: /usr/bin/originpc-control-center
- **Dependencies**: python3, python3-pyqt5, python3-psutil, python3-setuptools
- **Recommended**: python3-gputil, tlp, fancontrol
- **Systemd Service**: originpc-lid-monitor.service for lid state monitoring

### RPM-based (Fedora/RHEL)
- **Package Name**: originpc-enhanced-control
- **Installation Path**: /usr/share/originpc-control/
- **Config Location**: ~/.config/enhanced-originpc-control/
- **Executable**: /usr/bin/originpc-control-center
- **Dependencies**: python3, python3-qt5, python3-psutil
- **Recommended**: python3-gputil, tlp, nbfc
- **Systemd Service**: originpc-lid-monitor.service for lid state monitoring

### Arch Linux
- **Package Name**: originpc-enhanced-control
- **Installation Path**: /usr/share/originpc-control/
- **Config Location**: ~/.config/enhanced-originpc-control/
- **Executable**: /usr/bin/originpc-control-center
- **Dependencies**: python, python-pyqt5, python-psutil, systemd
- **Optional Dependencies**: python-gputil, tlp, nbfc, lm-sensors
- **Backup Configuration**: /etc/originpc-control/config.conf

### Flatpak
- **App ID**: org.originpc.ControlCenter
- **Runtime**: org.kde.Platform version 5.15-23.08
- **Sandbox Permissions**: Device access for RGB control, filesystem access for configuration
- **Config Location**: ~/.var/app/org.originpc.ControlCenter/config/enhanced-originpc-control/
- **Installation Command**: `flatpak install originpc-enhanced-control-5.1.0.flatpak`

### Snap
- **Package Name**: originpc-control-center
- **Base**: core22
- **Confinement**: strict
- **Interfaces**: hardware-observe, hidraw, raw-usb, system-observe, power-control, etc.
- **Installation Command**: `snap install originpc-control-center_1.0.0_amd64.snap --dangerous`

### AppImage
- **File Name**: OriginPC-Control-Center-5.1.0-x86_64.AppImage
- **Portability**: Runs on any Linux distribution without installation
- **Permissions**: Requires udev rules for RGB device access
- **Usage**: Mark as executable and run directly

### Universal Package (.tar.gz)
- **File Name**: originpc-enhanced-control-5.1.0-universal.tar.gz
- **Installation**: Includes install.sh script for setup
- **Flexibility**: Most customizable installation method
- **Requirements**: Requires manual dependency installation

## 4. System Requirements

### Hardware Requirements
- **Compatible Laptops**: OriginPC EON17-X and similar models with RGB keyboards
- **RGB Hardware**: Compatible HID devices (typically at /dev/hidraw0)
- **Minimum Specifications**:
  - Processor: Any modern x86_64 CPU
  - Memory: 2GB RAM minimum, 4GB recommended
  - Storage: 100MB free space for application and configuration

### Software Requirements
- **Operating System**: Any modern Linux distribution (Debian, Ubuntu, Fedora, Arch, etc.)
- **Desktop Environment**: Any (GNOME, KDE, XFCE, etc.)
- **Python**: Python 3.8 or higher
- **Display Server**: X11 or Wayland
- **Dependencies**:
  - PyQt5 for GUI
  - psutil for system monitoring
  - [Optional] GPUtil for GPU monitoring
  - [Optional] TLP for power management
  - [Optional] NBFC for fan control

### Permission Requirements
- **RGB Device Access**: Read/write access to /dev/hidraw* devices
  - `sudo chmod 666 /dev/hidraw0` or add user to appropriate group
- **System Monitoring**: No special permissions required
- **Fan/Power Control**: May require sudo for certain operations

## 5. Key Components Documentation

### EnhancedRGBController
The core component for interacting with RGB keyboard hardware.

- **Key Features**:
  - Complete keyboard mapping with spatial layout
  - Multi-device support for different hardware configurations
  - Enhanced clearing algorithms targeting problematic keys
  - Advanced wave effects with multiple patterns
  - Key grouping for efficient control

### SystemDataUpdater
Thread-based system monitoring with comprehensive data collection.

- **Key Features**:
  - Real-time CPU, memory, disk usage monitoring
  - Temperature tracking for all components
  - Network monitoring capabilities
  - GPU monitoring through GPUtil if available
  - High performance with minimal system impact

### LidMonitor
Monitors laptop lid state to automatically clear RGB lighting on lid close.

- **Key Features**:
  - Multiple detection methods for maximum compatibility
  - Enhanced clearing on lid close
  - Special handling for EON17-X specific quirks
  - Persistent monitoring with self-recovery

### FanController
Integrated fan control and monitoring system.

- **Key Features**:
  - Multiple fan detection methods
  - NBFC integration for advanced fan control
  - Built-in fan speed display
  - Multiple control modes (Auto, Performance, Silent)

### PowerManager
Manages system power profiles for optimal performance/battery life.

- **Key Features**:
  - TLP integration for profile switching
  - Battery monitoring
  - AC adapter state detection
  - Profile switching (Performance, Balanced, Power Save)

### TemperatureMonitor
Comprehensive temperature monitoring for all system components.

- **Key Features**:
  - CPU core temperature monitoring
  - GPU temperature tracking
  - Storage (NVMe/SSD) temperature monitoring
  - Memory temperature monitoring
  - Multi-source data aggregation

## 6. Usage Instructions

### Installation

#### Debian/Ubuntu
```
sudo apt install ./originpc-enhanced-control_5.1.0_all.deb
```

#### Fedora/RHEL
```
sudo dnf install ./originpc-enhanced-control-5.1.0-1.noarch.rpm
```

#### Arch Linux
```
sudo pacman -U originpc-enhanced-control-5.1.0-1-any.pkg.tar.zst
```

#### Flatpak
```
flatpak install ./originpc-enhanced-control-5.1.0.flatpak
```

#### Snap
```
sudo snap install ./originpc-control-center_1.0.0_amd64.snap --dangerous
```

#### AppImage
```
chmod +x ./OriginPC-Control-Center-5.1.0-x86_64.AppImage
./OriginPC-Control-Center-5.1.0-x86_64.AppImage
```

#### Universal Package
```
tar -xzf originpc-enhanced-control-5.1.0-universal.tar.gz
cd originpc-enhanced-control
./universal/install.sh
```

### First Launch Setup

1. Ensure RGB device permissions are correct:
   ```
   sudo chmod 666 /dev/hidraw0
   ```
   Or add your user to the appropriate group:
   ```
   sudo usermod -a -G input,plugdev $USER
   ```
   (Log out and back in for group changes to take effect)

2. Launch the application:
   ```
   originpc-control-center
   ```
   
3. Check the connection status in the RGB Control tab to verify device access.

### RGB Control Usage

1. **Choose a Color**: Click "Choose Color" or use preset colors.
2. **Apply to Keys**:
   - Select a key group like "All Keys", "Function Keys", etc.
   - Apply color to the selected group
3. **Create Effects**:
   - Navigate to the Effects tab
   - Select an effect like "Rainbow Wave" or "Breathing"
   - Click "Start" to begin the effect

### System Monitoring Usage

1. **View System Data**: The left panel shows real-time CPU, memory usage and temperatures.
2. **Fan Control**:
   - Navigate to the System tab
   - Choose a fan mode: Auto, Silent, or Performance
   - Click "Fan GUI" for detailed fan control
3. **Power Management**:
   - Select a power profile: Performance, Balanced, or Power Save
   - View power information including battery and AC status
4. **Temperature Monitoring**:
   - View detailed temperature data for all components
   - Launch the dedicated temperature monitor for comprehensive view

### Advanced Features

1. **Lid Monitoring**:
   - Lid monitoring starts automatically
   - RGB lighting clears automatically when lid is closed
   - Test the clearing functionality using the "Test Clear" button

2. **System Tray**:
   - Minimize to tray using the tray button
   - Access quick actions from the tray icon context menu
   - Double-click the tray icon to restore the application

3. **Command Line Options**:
   ```
   originpc-control-center --minimized  # Start minimized
   originpc-control-center --tray       # Start to system tray
   ```

### Troubleshooting

1. **RGB Not Working**:
   - Check device permissions: `ls -l /dev/hidraw*`
   - Ensure your user has access to the device
   - Try running with sudo to verify permission issues

2. **Fan Control Not Working**:
   - Ensure NBFC is installed and configured
   - Check if your system supports fan control
   - Try using external fan control applications

3. **Lid Monitoring Issues**:
   - Verify your laptop supports ACPI lid status
   - Check permissions for /proc/acpi/button/lid/
   - Test with manual triggers using `/tmp/test_lid_closed`

4. **Persistent Problems**:
   - Check application logs in ~/.config/enhanced-originpc-control/
   - Verify all dependencies are installed
   - Try running from terminal to see debug output

## License

This software is licensed under the MIT License. See the LICENSE file for details.

---

*Documentation for OriginPC Control Center version 5.1.0*
*Updated: June 20, 2025*

# OriginPC Enhanced Professional Control Center

**Complete RGB lighting and system management suite for OriginPC EON17-X laptops on Linux**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python 3.8+](https://img.shields.io/badge/python-3.8+-blue.svg)](https://www.python.org/downloads/)
[![Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://www.kernel.org/)

## 🎯 Features

### 🎨 RGB Lighting Control
- **Full keyboard RGB control** with per-key customization
- **Advanced effects**: Rainbow waves, breathing, static colors, gaming mode
- **Persistent effects** that survive reboots and system events
- **Professional UI** with real-time color picker and presets

### 🖥️ System Management (Nyx-inspired)
- **Real-time system monitoring**: CPU, memory, temperature, GPU
- **Fan control integration** with NBFC support
- **Power management** with TLP integration
- **System tray integration** for background operation

### 🔧 Advanced Features
- **Lid monitoring** with automatic RGB clearing on closure
- **KP_Plus cyan fix** - resolves stubborn hardware persistence issue
- **Multiple device support** (/dev/hidraw0-3)
- **Professional dark theme** with high DPI scaling
- **Comprehensive logging** and error handling

## 🚀 Quick Install

### Ubuntu/Debian
```bash
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control.deb
sudo apt install ./originpc-enhanced-control.deb
```

### Fedora/RHEL/CentOS
```bash
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control.rpm
sudo dnf install ./originpc-enhanced-control.rpm
```

### Arch Linux/Manjaro
```bash
yay -S originpc-enhanced-control
# or from AUR
git clone https://aur.archlinux.org/originpc-enhanced-control.git
cd originpc-enhanced-control && makepkg -si
```

### Universal (Any Linux)
```bash
wget https://github.com/user/originpc-enhanced-control/releases/latest/download/originpc-enhanced-control-universal.tar.gz
tar -xzf originpc-enhanced-control-universal.tar.gz
cd originpc-enhanced-control && sudo ./install.sh
```

## 🔧 Manual Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update && sudo apt install python3 python3-pip python3-pyqt5 python3-psutil

# Fedora
sudo dnf install python3 python3-pip python3-qt5 python3-psutil

# Arch
sudo pacman -S python python-pip python-pyqt5 python-psutil

# Universal (pip)
pip3 install PyQt5 psutil
```

### Install from Source
```bash
git clone https://github.com/user/originpc-enhanced-control.git
cd originpc-enhanced-control
sudo ./install.sh
```

## 🎮 Usage

### Desktop Application
```bash
# Launch GUI
originpc-control-center

# Launch minimized to tray
originpc-control-center --tray

# Launch minimized
originpc-control-center --minimized
```

### Command Line Tools
```bash
# Fix KP_Plus cyan issue
originpc-rgb-fix

# Clear all RGB
originpc-rgb-clear

# System monitoring
originpc-monitor
```

### Systemd Services
```bash
# Enable lid monitoring service
sudo systemctl enable --now originpc-lid-monitor

# Check service status
systemctl status originpc-lid-monitor
```

## 🐛 Troubleshooting

### Common Issues

#### RGB Not Working
```bash
# Check device permissions
ls -la /dev/hidraw*

# Fix permissions
sudo chmod 666 /dev/hidraw0

# Add udev rule for permanent fix
sudo tee /etc/udev/rules.d/99-originpc-rgb.rules << 'EOF'
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1038", MODE="0666"
EOF
sudo udevadm control --reload-rules
```

#### KP_Plus Cyan Issue
```bash
# Manual fix (run after lid opens)
originpc-rgb-fix

# Check if lid service is running
systemctl status originpc-lid-monitor
```

#### Dependencies Missing
```bash
# Install missing dependencies
sudo apt install python3-pyqt5 python3-psutil  # Ubuntu/Debian
sudo dnf install python3-qt5 python3-psutil    # Fedora
sudo pacman -S python-pyqt5 python-psutil      # Arch
```

## 📦 Package Contents

### Core Applications
- `originpc-control-center` - Main GUI application
- `originpc-rgb-fix` - KP_Plus cyan fix utility
- `originpc-rgb-clear` - General RGB clearing tool
- `originpc-monitor` - System monitoring utility

### Services
- `originpc-lid-monitor.service` - Automatic lid monitoring
- `originpc-rgb-daemon.service` - Background RGB management

### Configuration
- `/etc/originpc-control/` - System configuration
- `~/.config/originpc-control/` - User configuration
- `/var/log/originpc-control/` - Application logs

## 🔒 Security

### Permissions Required
- **USB HID access** (`/dev/hidraw*`) - For RGB control
- **System monitoring** - For temperature/fan data
- **Systemd services** - For lid monitoring (optional)

### Privacy
- **No telemetry** - Application runs completely offline
- **Local storage only** - All settings stored locally
- **Open source** - Full source code available for review

## 🤝 Contributing

### Development Setup
```bash
git clone https://github.com/user/originpc-enhanced-control.git
cd originpc-enhanced-control
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 enhanced-professional-control-center.py
```

### Building Packages
```bash
# Build all packages
./build-packages.sh

# Build specific distribution
./build-packages.sh debian
./build-packages.sh rpm
./build-packages.sh arch
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🏆 Credits

- **Original concept**: OriginPC RGB control research
- **UI inspiration**: Nyx system monitor design principles
- **Testing**: EON17-X laptop community
- **Dependencies**: PyQt5, psutil, Python ecosystem

## 🆘 Support

### Issues
- [GitHub Issues](https://github.com/user/originpc-enhanced-control/issues)
- [Reddit r/OriginPC](https://reddit.com/r/OriginPC)

### Documentation
- [Wiki](https://github.com/user/originpc-enhanced-control/wiki)
- [FAQ](docs/FAQ.md)
- [Hardware Compatibility](docs/COMPATIBILITY.md)

---

**Made with ❤️ for the OriginPC Linux community**

