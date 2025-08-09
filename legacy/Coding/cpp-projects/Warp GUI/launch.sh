#!/bin/bash

# Warp Terminal GUI Launch Script
echo "🚀 Launching Warp Terminal GUI..."

# Change to the build directory
cd "$(dirname "$0")/build"

# Check if executable exists
if [ ! -f "./warp-terminal-gui" ]; then
    echo "❌ Error: warp-terminal-gui executable not found!"
    echo "Please run 'mkdir build && cd build && cmake .. && make' first"
    exit 1
fi

# Launch the application
echo "✅ Starting application..."
./warp-terminal-gui &

# Get the PID
APP_PID=$!

echo "📱 Application launched with PID: $APP_PID"
echo "🖥️  Check your desktop for the GUI window"
echo ""
echo "To stop the application later, run:"
echo "   pkill -f warp-terminal-gui"
echo ""
echo "Application features:"
echo "  • Multi-tab terminal interface"
echo "  • AI assistant sidebar"  
echo "  • File explorer"
echo "  • Comprehensive settings"
echo "  • Dark theme"
echo ""
echo "✨ Enjoy using Warp Terminal GUI!"
