#!/bin/bash

# Universal Arch Installer C++/Qt6 Build Script
# This script builds the C++/Qt6 version of the Universal Arch Installer

set -e

PROJECT_NAME="UniversalArchInstaller"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_PREFIX="/usr/local"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check dependencies
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    if ! command_exists cmake; then
        missing_deps+=("cmake")
    fi
    
    if ! command_exists make; then
        missing_deps+=("make")
    fi
    
    if ! command_exists g++; then
        missing_deps+=("gcc")
    fi
    
    # Check for Qt6
    if ! pkg-config --exists Qt6Core; then
        missing_deps+=("qt6-base")
    fi
    
    if ! pkg-config --exists Qt6Widgets; then
        missing_deps+=("qt6-base")
    fi
    
    if ! pkg-config --exists Qt6Network; then
        missing_deps+=("qt6-base")
    fi
    
    if ! pkg-config --exists Qt6Sql; then
        missing_deps+=("qt6-base")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Installing missing dependencies..."
        
        if command_exists pacman; then
            # Arch Linux
            sudo pacman -S --needed --noconfirm cmake make gcc qt6-base qt6-tools
        elif command_exists apt; then
            # Debian/Ubuntu
            sudo apt update
            sudo apt install -y cmake make g++ qt6-base-dev qt6-tools-dev libqt6sql6-dev
        elif command_exists dnf; then
            # Fedora
            sudo dnf install -y cmake make gcc-c++ qt6-qtbase-devel qt6-qttools-devel
        else
            print_error "Please install the missing dependencies manually"
            exit 1
        fi
    fi
    
    print_success "All dependencies are available"
}

# Clean build directory
clean_build() {
    print_status "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    print_success "Build directory cleaned"
}

# Configure CMake
configure_cmake() {
    print_status "Configuring CMake..."
    cd "$BUILD_DIR"
    
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        "$PROJECT_ROOT"
    
    print_success "CMake configuration completed"
}

# Build the project
build_project() {
    print_status "Building project..."
    cd "$BUILD_DIR"
    
    # Use all available CPU cores
    local num_cores=$(nproc)
    make -j"$num_cores"
    
    print_success "Build completed successfully"
}

# Install the project
install_project() {
    print_status "Installing project..."
    cd "$BUILD_DIR"
    
    if [ "$EUID" -eq 0 ]; then
        make install
    else
        sudo make install
    fi
    
    print_success "Installation completed"
}

# Create desktop entry
create_desktop_entry() {
    print_status "Creating desktop entry..."
    
    local desktop_file="$HOME/.local/share/applications/universal-arch-installer.desktop"
    mkdir -p "$(dirname "$desktop_file")"
    
    cat > "$desktop_file" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Universal Arch Installer
Comment=Universal Arch Linux Package Installer
Exec=$INSTALL_PREFIX/bin/UniversalArchInstaller
Icon=universal-arch-installer
Terminal=false
Categories=System;PackageManager;
Keywords=package;installer;arch;linux;pacman;aur;
StartupNotify=true
EOF
    
    # Update desktop database
    if command_exists update-desktop-database; then
        update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
    fi
    
    print_success "Desktop entry created"
}

# Run tests (if any)
run_tests() {
    print_status "Running tests..."
    cd "$BUILD_DIR"
    
    if [ -f "test/test_universal_installer" ]; then
        ./test/test_universal_installer
        print_success "All tests passed"
    else
        print_warning "No tests found"
    fi
}

# Package for distribution
create_package() {
    print_status "Creating distribution package..."
    cd "$BUILD_DIR"
    
    # Create PKGBUILD for Arch Linux
    if command_exists makepkg; then
        print_status "Creating Arch Linux package..."
        # This would create a PKGBUILD and build a package
        print_warning "Package creation not implemented yet"
    fi
    
    # Create AppImage (if tools available)
    if command_exists linuxdeploy; then
        print_status "Creating AppImage..."
        print_warning "AppImage creation not implemented yet"
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Build script for Universal Arch Installer (C++/Qt6 version)"
    echo ""
    echo "Options:"
    echo "  -h, --help       Show this help message"
    echo "  -c, --clean      Clean build directory before building"
    echo "  -i, --install    Install after building"
    echo "  -t, --test       Run tests after building"
    echo "  -p, --package    Create distribution package"
    echo "  -d, --debug      Build in debug mode"
    echo "  --prefix PREFIX  Set installation prefix (default: /usr/local)"
    echo ""
    echo "Examples:"
    echo "  $0                    # Basic build"
    echo "  $0 -c -i             # Clean, build, and install"
    echo "  $0 --debug           # Debug build"
    echo "  $0 --prefix /usr     # Install to /usr"
}

# Main function
main() {
    local clean=false
    local install=false
    local run_tests_flag=false
    local create_package_flag=false
    local build_type="Release"
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -c|--clean)
                clean=true
                shift
                ;;
            -i|--install)
                install=true
                shift
                ;;
            -t|--test)
                run_tests_flag=true
                shift
                ;;
            -p|--package)
                create_package_flag=true
                shift
                ;;
            -d|--debug)
                build_type="Debug"
                shift
                ;;
            --prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    print_status "Starting build process for $PROJECT_NAME"
    print_status "Build type: $build_type"
    print_status "Install prefix: $INSTALL_PREFIX"
    
    # Execute build steps
    check_dependencies
    
    if [ "$clean" = true ]; then
        clean_build
    elif [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    configure_cmake
    build_project
    
    if [ "$run_tests_flag" = true ]; then
        run_tests
    fi
    
    if [ "$install" = true ]; then
        install_project
        create_desktop_entry
    fi
    
    if [ "$create_package_flag" = true ]; then
        create_package
    fi
    
    print_success "Build process completed successfully!"
    
    if [ "$install" = true ]; then
        print_status "You can now run the application with: $INSTALL_PREFIX/bin/UniversalArchInstaller"
        print_status "Or find it in your application menu as 'Universal Arch Installer'"
    else
        print_status "To run the application: $BUILD_DIR/bin/UniversalArchInstaller"
        print_status "To install: $0 --install"
    fi
}

# Run main function with all arguments
main "$@"
