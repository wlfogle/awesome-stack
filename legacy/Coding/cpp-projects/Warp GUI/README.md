# Warp Terminal GUI

A modern, feature-rich terminal GUI application built with C++ and Qt6, inspired by Warp Terminal.

## Features

- **Modern Dark Theme**: Sleek dark interface with customizable colors
- **Multi-Tab Support**: Create, close, rename, and duplicate terminal tabs
- **AI Assistant Sidebar**: Integrated panel for AI-powered command assistance
- **File Explorer**: Browse project files with a built-in tree view
- **Customizable Settings**: Comprehensive settings dialog for fonts, colors, and behavior
- **Command History**: Navigate through command history with up/down arrows
- **Built-in Commands**: Support for `cd`, `clear`, and other terminal commands
- **Fish Shell Integration**: Optimized for Fish shell (configurable)

## Screenshots

The application features:
- A main window with tabbed terminal interface
- Sidebar with file explorer and AI assistant
- Settings dialog with multiple configuration tabs
- Modern styling with proper dark theme support

## Prerequisites

- Qt6 (Core, Widgets components)
- CMake 3.16 or higher
- C++17 compatible compiler
- Fish shell (recommended, but other shells work too)

## Installation

### On Arch Linux / Garuda Linux

```bash
# Install Qt6 and CMake
sudo pacman -S qt6-base cmake make gcc

# Clone or extract the project
cd "/run/media/lou/Data/Download/lou/Coding/Warp GUI"

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run the application
./warp-terminal-gui
```

### On Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install qt6-base-dev cmake build-essential

# Build process (same as above)
mkdir build
cd build
cmake ..
make
./warp-terminal-gui
```

## Usage

### Basic Operations

- **New Tab**: Ctrl+Shift+T or File → New Tab
- **Close Tab**: Ctrl+W or File → Close Tab
- **Toggle Sidebar**: Ctrl+B or View → Toggle Sidebar
- **Settings**: Ctrl+, or File → Settings

### Terminal Features

- **Command History**: Use Up/Down arrows to navigate command history
- **Copy/Paste**: Ctrl+C/Ctrl+V (Ctrl+C interrupts running processes)
- **Clear Terminal**: Ctrl+L or type `clear`
- **Change Directory**: Use `cd` command with tab completion

### AI Assistant

The sidebar includes an AI assistant panel where you can:
- Ask for command suggestions
- View command execution history
- Get help with terminal operations

### File Explorer

The file tree shows:
- Current working directory structure
- Project files and folders
- Quick navigation to different directories

## Configuration

### Settings Dialog

Access via File → Settings or Ctrl+, to configure:

**General Tab:**
- Default shell (fish, bash, zsh, sh)
- Working directory
- Startup behavior
- Command history limit

**Appearance Tab:**
- Font family and size
- Color scheme (Dark, Light, Custom)
- Custom colors for background, text, and prompt
- Window opacity

**Terminal Tab:**
- Tab size for indentation
- Line wrapping
- Cursor appearance
- Scrollback buffer size

### Shell Configuration

The application works best with Fish shell but supports:
- Fish (default)
- Bash
- Zsh
- Sh

## Architecture

The application is structured with the following components:

- **MainWindow**: Main application window with menu, toolbar, and status bar
- **TabWidget**: Manages multiple terminal tabs with context menus
- **TerminalWidget**: Individual terminal instances with process management
- **SettingsDialog**: Comprehensive settings interface

### Key Classes

- `MainWindow`: Central application hub
- `TabWidget`: Tab management and navigation
- `TerminalWidget`: Terminal emulation and process execution
- `SettingsDialog`: Configuration management

## Building from Source

```bash
# Ensure you have Qt6 development packages
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Development

### Adding Features

1. **New Terminal Features**: Extend `TerminalWidget` class
2. **UI Enhancements**: Modify `MainWindow` layout and styling
3. **Settings Options**: Add new options to `SettingsDialog`
4. **Themes**: Extend color scheme system in settings

### Code Style

- C++17 standard
- Qt naming conventions
- Header guards for all header files
- Proper memory management with Qt parent-child system

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Troubleshooting

### Common Issues

**Qt6 not found:**
```bash
# Make sure Qt6 is installed and in PATH
export CMAKE_PREFIX_PATH=/usr/lib/qt6/cmake
```

**Build errors:**
```bash
# Clean build directory
rm -rf build/*
cmake ..
make clean && make
```

**Runtime issues:**
- Ensure Fish shell is installed: `sudo pacman -S fish`
- Check terminal permissions
- Verify working directory exists

## Future Enhancements

- Syntax highlighting for command output
- Plugin system for extensions
- Integrated terminal multiplexer support
- Advanced AI integration
- Custom keybinding configuration
- Session management and restoration

---

Built with ❤️ using Qt6 and C++
