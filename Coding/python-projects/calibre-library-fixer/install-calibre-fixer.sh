#!/bin/bash

# Calibre Library Fixer - Arch Linux Installation Script
# =====================================================
# Installs the Calibre Library Fixer GUI application system-wide on Arch Linux
#
# Author: AI Assistant
# Date: 2025-01-20
# License: MIT

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Application details
APP_NAME="Calibre Library Fixer"
APP_EXEC="calibre-library-fixer-gui"
APP_VERSION="2.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Installation paths
INSTALL_DIR="/usr/local/bin"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/pixmaps"
DOC_DIR="/usr/share/doc/calibre-library-fixer"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   Calibre Library Fixer v${APP_VERSION}${NC}"
echo -e "${BLUE}   Arch Linux Installation Script${NC}"
echo -e "${BLUE}=========================================${NC}"
echo

# Function to print status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root!"
        print_warning "Run it as a regular user - it will use sudo when needed."
        exit 1
    fi
}

# Check for required dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    local deps=("python" "python-pyqt6" "python-requests" "python-pip")
    
    for dep in "${deps[@]}"; do
        if ! pacman -Qi "$dep" &> /dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_warning "Missing dependencies: ${missing_deps[*]}"
        echo -n "Install missing dependencies? [Y/n]: "
        read -r response
        
        if [[ "$response" =~ ^[Nn]$ ]]; then
            print_error "Cannot proceed without dependencies."
            exit 1
        fi
        
        print_status "Installing dependencies with pacman..."
        sudo pacman -S --needed "${missing_deps[@]}"
    else
        print_status "All dependencies are satisfied."
    fi
}

# Verify source files exist
check_source_files() {
    print_status "Checking source files..."
    
    local required_files=(
        "$SCRIPT_DIR/calibre-library-fixer-gui.py"
    )
    
    for file in "${required_files[@]}"; do
        if [[ ! -f "$file" ]]; then
            print_error "Required file not found: $file"
            print_error "Make sure you're running this script from the directory containing the Calibre Library Fixer files."
            exit 1
        fi
    done
    
    print_status "Source files found."
}

# Create application directories
create_directories() {
    print_status "Creating application directories..."
    
    sudo mkdir -p "$INSTALL_DIR"
    sudo mkdir -p "$DESKTOP_DIR"
    sudo mkdir -p "$ICON_DIR"
    sudo mkdir -p "$DOC_DIR"
    
    print_status "Directories created."
}

# Install the main application
install_application() {
    print_status "Installing application files..."
    
    # Copy the main GUI script
    sudo cp "$SCRIPT_DIR/calibre-library-fixer-gui.py" "$INSTALL_DIR/$APP_EXEC"
    sudo chmod +x "$INSTALL_DIR/$APP_EXEC"
    
    # Copy CLI version if it exists
    if [[ -f "$SCRIPT_DIR/calibre-library-fixer.py" ]]; then
        sudo cp "$SCRIPT_DIR/calibre-library-fixer.py" "$INSTALL_DIR/calibre-library-fixer"
        sudo chmod +x "$INSTALL_DIR/calibre-library-fixer"
        print_status "CLI version installed as 'calibre-library-fixer'"
    fi
    
    # Copy enhanced version if it exists
    if [[ -f "$SCRIPT_DIR/calibre-library-fixer-enhanced.py" ]]; then
        sudo cp "$SCRIPT_DIR/calibre-library-fixer-enhanced.py" "$INSTALL_DIR/calibre-library-fixer-enhanced"
        sudo chmod +x "$INSTALL_DIR/calibre-library-fixer-enhanced"
        print_status "Enhanced CLI version installed as 'calibre-library-fixer-enhanced'"
    fi
    
    print_status "Application files installed."
}

# Create desktop entry
create_desktop_entry() {
    print_status "Creating desktop entry..."
    
    local desktop_file="$DESKTOP_DIR/calibre-library-fixer.desktop"
    
    sudo tee "$desktop_file" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Calibre Library Fixer
GenericName=Ebook Library Organizer
Comment=Organize your ebook library with standardized filenames
Exec=$APP_EXEC
Icon=calibre-library-fixer
Terminal=false
Categories=Office;Database;
Keywords=calibre;ebook;library;organize;filename;
StartupNotify=true
StartupWMClass=calibre-library-fixer-gui
MimeType=application/x-sqlite3;
EOF
    
    sudo chmod 644 "$desktop_file"
    print_status "Desktop entry created."
}

# Create application icon
create_icon() {
    print_status "Creating application icon..."
    
    # Create a simple SVG icon
    local icon_file="$ICON_DIR/calibre-library-fixer.svg"
    
    sudo tee "$icon_file" > /dev/null << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="64" height="64" viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg">
  <!-- Book -->
  <rect x="10" y="15" width="35" height="45" rx="3" ry="3" fill="#4682B4" stroke="#2E5A87" stroke-width="2"/>
  <!-- Pages -->
  <rect x="12" y="17" width="31" height="41" rx="2" ry="2" fill="#FFFFFF"/>
  <!-- Text lines -->
  <line x1="16" y1="25" x2="39" y2="25" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="29" x2="39" y2="29" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="33" x2="39" y2="33" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="37" x2="39" y2="37" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="41" x2="39" y2="41" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="45" x2="39" y2="45" stroke="#CCCCCC" stroke-width="1"/>
  <line x1="16" y1="49" x2="32" y2="49" stroke="#CCCCCC" stroke-width="1"/>
  <!-- Gear (settings icon) -->
  <circle cx="50" cy="45" r="10" fill="#FFA500" stroke="#EB9100" stroke-width="1"/>
  <circle cx="50" cy="45" r="4" fill="#FFFFFF"/>
  <!-- Gear teeth -->
  <polygon points="50,33 52,35 48,35" fill="#FFA500"/>
  <polygon points="50,57 48,55 52,55" fill="#FFA500"/>
  <polygon points="38,45 40,43 40,47" fill="#FFA500"/>
  <polygon points="62,45 60,47 60,43" fill="#FFA500"/>
  <polygon points="41.86,36.86 43.27,35.45 44.69,38.28" fill="#FFA500"/>
  <polygon points="58.14,53.14 56.73,54.55 55.31,51.72" fill="#FFA500"/>
  <polygon points="58.14,36.86 56.73,35.45 55.31,38.28" fill="#FFA500"/>
  <polygon points="41.86,53.14 43.27,54.55 44.69,51.72" fill="#FFA500"/>
</svg>
EOF
    
    sudo chmod 644 "$icon_file"
    
    # Also create a PNG version for better compatibility
    if command -v convert &> /dev/null; then
        sudo convert "$icon_file" "$ICON_DIR/calibre-library-fixer.png"
        print_status "Application icon created (SVG and PNG)."
    else
        print_status "Application icon created (SVG only)."
        print_warning "Install ImageMagick for PNG icon generation: sudo pacman -S imagemagick"
    fi
}

# Create documentation
create_documentation() {
    print_status "Creating documentation..."
    
    # Create README
    sudo tee "$DOC_DIR/README.md" > /dev/null << 'EOF'
# Calibre Library Fixer v2.0

## Overview
Calibre Library Fixer is a powerful tool for organizing your Calibre ebook library with standardized filenames. It features advanced metadata detection, AI-enhanced filename parsing, and external database lookups.

## Features
- 📚 Scan and analyze Calibre libraries
- 🧠 AI-enhanced filename parsing
- 🔍 External metadata lookups (Google Books, Open Library, Barnes & Noble, etc.)
- 📋 Preview changes before applying
- 🛡️ Safety features with backups
- ⚙️ Configurable settings
- 📝 Comprehensive logging

## Usage

### GUI Version
Launch the graphical interface:
```bash
calibre-library-fixer-gui
```

### CLI Version (if installed)
Run from command line:
```bash
calibre-library-fixer /path/to/calibre/library
```

### Enhanced CLI Version (if installed)
Run the enhanced version with advanced features:
```bash
calibre-library-fixer-enhanced /path/to/calibre/library --dry-run
```

## Configuration
The GUI version stores settings automatically. For CLI versions, see the built-in help:
```bash
calibre-library-fixer --help
```

## Support
This is an open-source tool. Please report issues or contribute improvements.

## License
MIT License - see source code for details.
EOF
    
    # Create uninstall script
    sudo tee "$DOC_DIR/uninstall.sh" > /dev/null << 'EOF'
#!/bin/bash
# Uninstall script for Calibre Library Fixer

echo "Uninstalling Calibre Library Fixer..."

# Remove application files
sudo rm -f /usr/local/bin/calibre-library-fixer-gui
sudo rm -f /usr/local/bin/calibre-library-fixer
sudo rm -f /usr/local/bin/calibre-library-fixer-enhanced

# Remove desktop entry
sudo rm -f /usr/share/applications/calibre-library-fixer.desktop

# Remove icons
sudo rm -f /usr/share/pixmaps/calibre-library-fixer.svg
sudo rm -f /usr/share/pixmaps/calibre-library-fixer.png

# Remove documentation
sudo rm -rf /usr/share/doc/calibre-library-fixer

echo "Uninstallation complete."
echo "Note: User settings and logs are preserved in ~/.config/CalibreFixer/"
EOF
    
    sudo chmod +x "$DOC_DIR/uninstall.sh"
    
    print_status "Documentation created."
}

# Update desktop database
update_desktop_database() {
    print_status "Updating desktop database..."
    
    if command -v update-desktop-database &> /dev/null; then
        sudo update-desktop-database "$DESKTOP_DIR"
    fi
    
    if command -v gtk-update-icon-cache &> /dev/null; then
        sudo gtk-update-icon-cache "$ICON_DIR" -f -t 2>/dev/null || true
    fi
    
    print_status "Desktop database updated."
}

# Create system service (optional)
create_service() {
    echo -n "Create a system service for automated library scanning? [y/N]: "
    read -r response
    
    if [[ "$response" =~ ^[Yy]$ ]]; then
        print_status "Creating system service..."
        
        local service_file="/etc/systemd/system/calibre-library-fixer.service"
        
        sudo tee "$service_file" > /dev/null << 'EOF'
[Unit]
Description=Calibre Library Fixer - Automated Scanning
After=network.target

[Service]
Type=oneshot
User=nobody
ExecStart=/usr/local/bin/calibre-library-fixer-enhanced --auto-detect --dry-run --log
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF
        
        # Create timer for regular execution
        local timer_file="/etc/systemd/system/calibre-library-fixer.timer"
        
        sudo tee "$timer_file" > /dev/null << 'EOF'
[Unit]
Description=Run Calibre Library Fixer weekly
Requires=calibre-library-fixer.service

[Timer]
OnCalendar=weekly
Persistent=true

[Install]
WantedBy=timers.target
EOF
        
        sudo systemctl daemon-reload
        print_status "System service created (disabled by default)."
        print_warning "To enable: sudo systemctl enable --now calibre-library-fixer.timer"
    fi
}

# Main installation function
main() {
    echo -e "${BLUE}Starting installation...${NC}"
    echo
    
    check_root
    check_source_files
    check_dependencies
    create_directories
    install_application
    create_desktop_entry
    create_icon
    create_documentation
    update_desktop_database
    create_service
    
    echo
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}   Installation Complete!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    echo
    print_status "Calibre Library Fixer has been installed successfully."
    echo
    echo -e "${BLUE}How to run:${NC}"
    echo -e "  • GUI: ${YELLOW}calibre-library-fixer-gui${NC}"
    echo -e "  • Or search for 'Calibre Library Fixer' in your application menu"
    echo
    if [[ -f "$INSTALL_DIR/calibre-library-fixer" ]]; then
        echo -e "  • CLI: ${YELLOW}calibre-library-fixer /path/to/library${NC}"
    fi
    if [[ -f "$INSTALL_DIR/calibre-library-fixer-enhanced" ]]; then
        echo -e "  • Enhanced CLI: ${YELLOW}calibre-library-fixer-enhanced /path/to/library${NC}"
    fi
    echo
    echo -e "${BLUE}Documentation:${NC} $DOC_DIR"
    echo -e "${BLUE}Uninstall:${NC} sudo $DOC_DIR/uninstall.sh"
    echo
    print_status "Installation log saved to: /var/log/calibre-library-fixer-install.log"
    
    # Save installation log
    {
        echo "Calibre Library Fixer v$APP_VERSION"
        echo "Installed on: $(date)"
        echo "Installed by: $USER"
        echo "Installation directory: $INSTALL_DIR"
        echo "Desktop entry: $DESKTOP_DIR/calibre-library-fixer.desktop"
        echo "Documentation: $DOC_DIR"
    } | sudo tee /var/log/calibre-library-fixer-install.log > /dev/null
}

# Run main installation
main "$@"
