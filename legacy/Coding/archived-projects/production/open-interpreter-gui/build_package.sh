#!/bin/bash

# Build script for Open Interpreter GUI Arch Linux package
# This script builds the package and optionally installs it

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PACKAGE_NAME="open-interpreter-gui"
VERSION="1.0.0"
BUILD_DIR="/tmp/open-interpreter-gui-build"
PRODUCTION_DIR="/run/media/lou/Data/Download/lou/Coding/production"
SOURCE_DIR="$(pwd)"

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    log_info "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for makepkg
    if ! command -v makepkg >/dev/null 2>&1; then
        missing_deps+=("base-devel")
    fi
    
    # Check for Python build tools
    if ! python -c "import build" >/dev/null 2>&1; then
        missing_deps+=("python-build")
    fi
    
    if ! python -c "import installer" >/dev/null 2>&1; then
        missing_deps+=("python-installer")
    fi
    
    if ! python -c "import wheel" >/dev/null 2>&1; then
        missing_deps+=("python-wheel")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        log_info "Install with: sudo pacman -S ${missing_deps[*]}"
        exit 1
    fi
    
    log_success "All dependencies are available"
}

# Prepare build environment
prepare_build() {
    log_info "Preparing build environment..."
    
    # Clean previous build
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    
    # Copy source files to build directory
    cp -r "$SOURCE_DIR"/* "$BUILD_DIR/"
    
    # Ensure production directory exists
    mkdir -p "$PRODUCTION_DIR"
    
    log_success "Build environment prepared"
}

# Build the package
build_package() {
    log_info "Building package..."
    
    cd "$BUILD_DIR"
    
    # Generate checksums (skip for local build)
    sed -i "s/sha256sums=('SKIP')/sha256sums=('SKIP')/" PKGBUILD
    
    # Build the package
    makepkg -s --noconfirm
    
    if [ $? -ne 0 ]; then
        log_error "Package build failed"
        exit 1
    fi
    
    log_success "Package built successfully"
}

# Move package to production directory
move_package() {
    log_info "Moving package to production directory..."
    
    cd "$BUILD_DIR"
    
    # Find the built package
    PACKAGE_FILE=$(ls ${PACKAGE_NAME}-${VERSION}-*.pkg.tar.zst 2>/dev/null | head -1)
    
    if [ -z "$PACKAGE_FILE" ]; then
        log_error "Built package file not found"
        exit 1
    fi
    
    # Move to production directory
    mv "$PACKAGE_FILE" "$PRODUCTION_DIR/"
    
    log_success "Package moved to $PRODUCTION_DIR/$PACKAGE_FILE"
    echo "Package location: $PRODUCTION_DIR/$PACKAGE_FILE"
}

# Install the package
install_package() {
    log_info "Installing package..."
    
    PACKAGE_FILE=$(ls "$PRODUCTION_DIR"/${PACKAGE_NAME}-${VERSION}-*.pkg.tar.zst 2>/dev/null | head -1)
    
    if [ -z "$PACKAGE_FILE" ]; then
        log_error "Package file not found in production directory"
        exit 1
    fi
    
    # Install with pacman
    sudo pacman -U "$PACKAGE_FILE" --noconfirm
    
    if [ $? -eq 0 ]; then
        log_success "Package installed successfully"
        log_info "You can now run 'open-interpreter-gui' from the command line or find it in your applications menu"
    else
        log_error "Package installation failed"
        exit 1
    fi
}

# Clean up build directory
cleanup() {
    if [ -d "$BUILD_DIR" ]; then
        log_info "Cleaning up build directory..."
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    fi
}

# Main execution
main() {
    log_info "Starting Open Interpreter GUI package build process..."
    echo "================================"
    echo "Package: $PACKAGE_NAME"
    echo "Version: $VERSION"
    echo "Source: $SOURCE_DIR"
    echo "Production: $PRODUCTION_DIR"
    echo "================================"
    echo
    
    # Parse arguments
    BUILD_ONLY=false
    INSTALL_ONLY=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-only)
                BUILD_ONLY=true
                shift
                ;;
            --install-only)
                INSTALL_ONLY=true
                shift
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --build-only    Only build the package, don't install"
                echo "  --install-only  Only install existing package"
                echo "  -h, --help      Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    if [ "$INSTALL_ONLY" = true ]; then
        install_package
        return
    fi
    
    # Build process
    check_dependencies
    prepare_build
    build_package
    move_package
    
    if [ "$BUILD_ONLY" = false ]; then
        install_package
    fi
    
    cleanup
    
    log_success "Process completed successfully!"
    
    if [ "$BUILD_ONLY" = true ]; then
        log_info "Package is ready at: $PRODUCTION_DIR"
        log_info "To install later, run: sudo pacman -U $PRODUCTION_DIR/${PACKAGE_NAME}-${VERSION}-*.pkg.tar.zst"
    else
        log_info "Open Interpreter GUI is now installed and ready to use!"
        log_info "Start it with: open-interpreter-gui"
    fi
}

# Trap cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"
