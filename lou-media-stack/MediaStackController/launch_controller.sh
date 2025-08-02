#!/usr/bin/env bash

# LunaSea Media Stack Controller Launcher
# This script detects the platform and launches the appropriate controller

set -e

# Function to detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        CYGWIN*)    echo "windows";;
        MINGW*)     echo "windows";;
        *)          echo "unknown";;
    esac
}

# Function to check if LunaSea is installed
check_lunasea() {
    if command -v lunasea &> /dev/null; then
        echo "LunaSea desktop application found"
        return 0
    else
        echo "LunaSea desktop application not found"
        return 1
    fi
}

# Function to check if Python is available
check_python() {
    if command -v python3 &> /dev/null; then
        echo "Python 3 found"
        return 0
    else
        echo "Python 3 not found"
        return 1
    fi
}

# Function to install Python dependencies
install_python_deps() {
    echo "Installing Python dependencies..."
    python3 -m pip install --user requests
}

# Function to launch Linux controller
launch_linux_controller() {
    echo "Launching Linux controller..."
    
    # Check if Python dependencies are installed
    if ! python3 -c "import requests" 2>/dev/null; then
        echo "Required Python packages not found. Installing..."
        install_python_deps
    fi
    
    # Make the Python controller executable
    chmod +x ./lunasea_linux_controller.py
    
    # Launch the controller
    ./lunasea_linux_controller.py "$@"
}

# Function to launch Android controller (via Android Studio or ADB)
launch_android_controller() {
    echo "Launching Android controller..."
    
    # Check if the Android project can be built
    if [ -f "./gradlew" ]; then
        echo "Building Android project..."
        ./gradlew assembleDebug
        
        # Check if ADB is available to install
        if command -v adb &> /dev/null; then
            echo "Installing APK on connected device..."
            adb install -r ./app/build/outputs/apk/debug/app-debug.apk
        else
            echo "ADB not found. Please install the APK manually from ./app/build/outputs/apk/debug/"
        fi
    else
        echo "Android project not found or not properly configured"
    fi
}

# Function to launch LunaSea desktop app
launch_lunasea_app() {
    echo "Launching LunaSea desktop application..."
    if check_lunasea; then
        lunasea &
    else
        echo "LunaSea desktop app not installed. Please install it first."
        echo "You can download it from: https://github.com/JagandeepBrar/lunasea/releases"
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] [COMMAND]"
    echo ""
    echo "Options:"
    echo "  -p, --platform PLATFORM    Force platform (linux, android, desktop)"
    echo "  -h, --help                 Show this help message"
    echo ""
    echo "Commands:"
    echo "  refresh                    Refresh all services"
    echo "  dashboard                  Show service dashboard"
    echo "  launch                     Launch LunaSea desktop"
    echo "  interactive                Start interactive mode (default)"
    echo ""
    echo "Examples:"
    echo "  $0                         Start interactive mode"
    echo "  $0 refresh                 Refresh all services"
    echo "  $0 --platform linux dashboard"
    echo "  $0 --platform android"
}

# Main function
main() {
    local platform=""
    local command="interactive"
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -p|--platform)
                platform="$2"
                shift 2
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            refresh|dashboard|launch|interactive)
                command="$1"
                shift
                ;;
            *)
                echo "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Auto-detect platform if not specified
    if [ -z "$platform" ]; then
        platform=$(detect_platform)
    fi
    
    echo "Detected platform: $platform"
    echo "Command: $command"
    echo "----------------------------------------"
    
    # Launch appropriate controller
    case "$platform" in
        linux)
            if check_python; then
                launch_linux_controller "$command"
            else
                echo "Python 3 is required for Linux controller"
                exit 1
            fi
            ;;
        android)
            launch_android_controller
            ;;
        desktop)
            launch_lunasea_app
            ;;
        *)
            echo "Unsupported platform: $platform"
            echo "Supported platforms: linux, android, desktop"
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"
