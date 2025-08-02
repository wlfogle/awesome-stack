# Clevo Control Center for OriginPC EON17-X

A comprehensive Qt6-based control center specifically optimized for OriginPC EON17-X and other Clevo-based laptops running Linux. This application provides real RGB lighting control via hidraw1, fan management, performance profiles, and system information display.

## Features

- **Real RGB Lighting Control**: Direct hidraw1 interface for 4-zone RGB keyboard lighting with color selection and brightness control
- **Hardware Fan Management**: Real-time monitoring and manual control of CPU and GPU fans via hwmon interfaces
- **Performance Profiles**: Switch between Balanced, Performance, Quiet, and Custom profiles with thermal management
- **System Information**: Display detailed hardware specifications specific to OriginPC EON17-X
- **Modern UI**: Dark theme optimized for gaming systems with gradient headers and intuitive tabbed interface

## System Requirements

- **Laptop**: OriginPC EON17-X or compatible Clevo-based system
- **OS**: Linux distribution (tested on Garuda Linux with KDE Plasma 6.4.1)
- **Qt**: Qt6.9.1 or newer
- **Hardware**: Intel i9-13900HX, NVIDIA RTX 4080 Laptop GPU (or similar)
- **Kernel**: 6.15.3-zen1-1-zen or compatible
- **Dependencies**: CMake 3.16+, C++17 compatible compiler
- **Access**: /dev/hidraw1 access for RGB control, hwmon access for fan control

## Installation for Garuda Linux / Arch

### Dependencies

```bash
# Install Qt6 and build tools
sudo pacman -S qt6-base qt6-tools cmake gcc ninja

# Install optional dependencies for full functionality
sudo pacman -S cpupower nvidia-utils lm_sensors
```

### Quick Build & Install

```bash
# Navigate to project directory
cd "/run/media/lou/Data/Download/lou/Coding/Control Center"

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake -G Ninja ..
ninja

# Install system-wide (optional)
sudo ninja install
```

### Setup Hardware Access

```bash
# Add your user to input group for hidraw1 access
sudo usermod -a -G input $USER

# Create udev rules for RGB keyboard control
sudo tee /etc/udev/rules.d/99-clevo-control.rules > /dev/null <<EOF
# Clevo RGB Keyboard Control for OriginPC EON17-X
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1558", MODE="0666", GROUP="input"
SUBSYSTEM=="hidraw", KERNEL=="hidraw1", MODE="0666", GROUP="input"

# Hardware monitoring access
SUBSYSTEM=="hwmon", MODE="0644", GROUP="input"
SUBSYSTEM=="hwmon", ACTION=="add", RUN+="/bin/chmod -R 666 /sys/class/hwmon/%k"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Log out and back in, or reboot for group changes to take effect
```

## Hardware Support for OriginPC EON17-X

### RGB Keyboard (via /dev/hidraw1)
- 4-zone RGB control
- Per-zone color and brightness settings
- Gaming, Work, Rainbow, and Breathing presets
- Real-time color application

### Fan Control (via hwmon)
- CPU Fan monitoring and control
- GPU Fan monitoring and control
- Automatic and manual modes
- Temperature-based fan curves
- Real-time RPM and temperature display

### Performance Profiles
- **Balanced**: ondemand governor, 80°C thermal limit
- **Performance**: performance governor, 85°C thermal limit, max GPU performance
- **Quiet**: powersave governor, 70°C thermal limit
- **Custom**: User-defined settings

## Usage

### RGB Lighting
- Use the "RGB Lighting" tab to control keyboard backlighting
- Select colors for each zone individually
- Adjust brightness levels
- Apply preset lighting effects

### Fan Control
- Monitor real-time fan speeds and temperatures
- Toggle between automatic and manual fan control
- Adjust manual fan speeds with sliders
- **Warning**: Manual fan control can affect system stability

### Performance Profiles
- Choose from predefined performance profiles
- Each profile adjusts CPU governor and other system settings
- Custom profile allows user-defined configurations

### System Information
- View detailed hardware specifications
- Check BIOS and EC firmware versions
- Monitor system status

## Development

The application is structured with the following main components:

- `HardwareController`: Backend interface for hardware communication
- `RGBZoneWidget`: Individual RGB zone control widget
- `FanControlWidget`: Fan monitoring and control widget
- `PerformanceProfileWidget`: Performance profile selection widget
- `ClevoControlCenter`: Main application window

## License

Open Source - See individual file headers for specific licensing information.

## Contributing

Contributions are welcome! Please ensure compatibility with Qt6 and follow the existing code style.

## Disclaimer

This software is provided as-is. Hardware control features can potentially damage your system if used improperly. Use at your own risk and always monitor system temperatures when using manual fan control.
