# ArchForgePro

A comprehensive Arch Linux management tool with AI assistance, package management, system maintenance, and more.

## Features

### Main Tab Structure

1. **Clean Install Backup/Restore**
   - Backup: Create system backups with configurable options
   - Restore: Restore from existing backups with preview
   - Logs: View backup/restore operation logs

2. **Software Management**
   - Search Packages: Quick search, advanced search, search results, search history
   - Package Install: Single install, batch install, install queue, install history, install log
   - Build & Distribute: Package builder, distribution, build log
   - Windows Programs: Wine management, program installer, installed programs, wine prefixes, logs
   - Maintenance: Quick maintenance, system updates, package cache, system optimization, maintenance logs
   - Installed Packages: View and manage installed packages
   - Settings: Software management configuration

3. **RGB/Fan Control**
   - Keyboard: RGB lighting control with effects and profiles
   - Fans: Fan speed control with automatic and manual modes

4. **Kernel Tools**
   - Download: Download kernel sources
   - Configure: Kernel configuration management
   - Compile: Kernel compilation with options
   - Install: Install and manage compiled kernels

5. **AI Assistant**
   - Chat: Interactive AI chat interface
   - Recommendations: AI-powered system recommendations
   - Analysis: AI analysis of packages and system
   - Settings: AI configuration options

6. **Settings**
   - About: Application information and credits

## Building

### Prerequisites

- Qt6 (Core, Widgets)
- CMake >= 3.16
- C++17 compatible compiler

### Build Instructions

```bash
cd src
mkdir build
cd build
cmake ..
make
```

### Running

```bash
./ArchForgePro
```

## Project Structure

```
src/
├── main.cpp                              # Application entry point
├── mainwindow.h/.cpp                     # Main window
├── cleaninstallbackuprestore_widget.h/.cpp  # Backup/Restore functionality
├── softwaremanagement_widget.h/.cpp      # Software management
├── rgbfancontrol_widget.h/.cpp           # RGB and fan control
├── kerneltools_widget.h/.cpp             # Kernel tools
├── aiassistant_widget.h/.cpp             # AI assistant
├── settings_widget.h/.cpp                # Settings and about
├── aioptimizer.h/.cpp                    # AI optimization (existing)
├── ai_assistant_extracted.h              # AI assistant reference (existing)
└── CMakeLists.txt                        # Build configuration
```

## Development Status

This is currently a GUI stub implementation with the following status:

- ✅ Complete UI structure and navigation
- ✅ All main tabs and sub-tabs implemented
- ✅ Basic functionality stubs
- ⚠️ Backend functionality marked as TODO
- ⚠️ AI integration not yet connected
- ⚠️ System operations not yet implemented

## Next Steps

1. Implement backend functionality for each module
2. Add AI service integration
3. Implement system operations (package management, kernel tools, etc.)
4. Add configuration persistence
5. Implement logging and error handling
6. Add unit tests

## License

GPL v3.0

## Contributing

This is a Qt6/C++ GUI application for Arch Linux system management. Contributions welcome!
