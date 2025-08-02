#!/bin/bash
set -e

echo "🎬 Setting up Grandma's Media Center Solution"
echo "=============================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# Check if we're in the right directory
if [ ! -f "docker-compose.yml" ]; then
    print_error "Please run this script from your lou-media-stack directory"
    exit 1
fi

print_info "Setting up Grandmother Dashboard..."

# Create necessary directories
mkdir -p grandma-dashboard/static
mkdir -p grandma-dashboard/templates
mkdir -p firetv-app/app/src/main/res/drawable

print_status "Created directories"

# Check if API keys are set in .env
if [ ! -f ".env" ]; then
    print_warning "No .env file found. Creating template..."
    cat > .env.template << EOF
# Add these to your existing .env file:
RADARR_API_KEY=your_radarr_api_key_here
SONARR_API_KEY=your_sonarr_api_key_here
JACKETT_API_KEY=your_jackett_api_key_here

# Domain for your services
DOMAIN=your.domain.com

# Your network IP for Fire TV app
LOCAL_IP=192.168.1.100
EOF
    print_warning "Please add the API keys to your .env file before continuing"
    print_info "You can find API keys in each service's settings page"
fi

# Build the grandmother dashboard
print_info "Building grandmother dashboard container..."
if docker build -t grandma-dashboard ./grandma-dashboard/; then
    print_status "Dashboard container built successfully"
else
    print_error "Failed to build dashboard container"
    exit 1
fi

# Create Fire TV button styles
cat > firetv-app/app/src/main/res/drawable/tv_button_primary.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<selector xmlns:android="http://schemas.android.com/apk/res/android">
    <item android:state_focused="true">
        <shape android:shape="rectangle">
            <solid android:color="#4CAF50"/>
            <corners android:radius="12dp"/>
            <stroke android:width="4dp" android:color="#ffffff"/>
        </shape>
    </item>
    <item>
        <shape android:shape="rectangle">
            <solid android:color="#2E7D32"/>
            <corners android:radius="12dp"/>
        </shape>
    </item>
</selector>
EOF

cat > firetv-app/app/src/main/res/drawable/tv_button_secondary.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<selector xmlns:android="http://schemas.android.com/apk/res/android">
    <item android:state_focused="true">
        <shape android:shape="rectangle">
            <solid android:color="#2196F3"/>
            <corners android:radius="12dp"/>
            <stroke android:width="4dp" android:color="#ffffff"/>
        </shape>
    </item>
    <item>
        <shape android:shape="rectangle">
            <solid android:color="#1976D2"/>
            <corners android:radius="12dp"/>
        </shape>
    </item>
</selector>
EOF

cat > firetv-app/app/src/main/res/drawable/tv_button_accent.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<selector xmlns:android="http://schemas.android.com/apk/res/android">
    <item android:state_focused="true">
        <shape android:shape="rectangle">
            <solid android:color="#FF9800"/>
            <corners android:radius="12dp"/>
            <stroke android:width="4dp" android:color="#ffffff"/>
        </shape>
    </item>
    <item>
        <shape android:shape="rectangle">
            <solid android:color="#F57C00"/>
            <corners android:radius="12dp"/>
        </shape>
    </item>
</selector>
EOF

cat > firetv-app/app/src/main/res/drawable/tv_button_neutral.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<selector xmlns:android="http://schemas.android.com/apk/res/android">
    <item android:state_focused="true">
        <shape android:shape="rectangle">
            <solid android:color="#9E9E9E"/>
            <corners android:radius="12dp"/>
            <stroke android:width="4dp" android:color="#ffffff"/>
        </shape>
    </item>
    <item>
        <shape android:shape="rectangle">
            <solid android:color="#616161"/>
            <corners android:radius="12dp"/>
        </shape>
    </item>
</selector>
EOF

print_status "Created Fire TV button styles"

# Start the grandmother dashboard
print_info "Starting grandmother dashboard..."
if docker-compose up -d grandma-dashboard; then
    print_status "Grandmother dashboard started successfully"
    print_info "Dashboard available at: http://localhost:8600"
    
    # Wait for service to be ready
    sleep 10
    
    # Test if dashboard is accessible
    if curl -s http://localhost:8600 > /dev/null; then
        print_status "Dashboard is accessible and working!"
    else
        print_warning "Dashboard started but may need more time to initialize"
    fi
else
    print_error "Failed to start grandmother dashboard"
fi

echo ""
print_info "Creating installation guide..."

cat > GRANDMOTHER_SETUP.md << 'EOF'
# 🎬 Grandma's Media Center - Complete Setup Guide

## What You Now Have

### 1. 📱 Grandmother-Friendly Web Dashboard
**Access:** http://localhost:8600 (or http://your-server-ip:8600)

**Features:**
- **Large buttons and text** - Easy to see and click
- **Simple search** - Just type "funny movies" or "cooking shows"
- **One-click downloads** - Search, click "Download This", done!
- **Clear instructions** - Shows exactly where to find downloaded content
- **System status** - Shows if everything is working
- **Built-in help** - Explains how to use everything

### 2. 📺 Fire TV App (Ready to Build)
**Location:** `firetv-app/` folder

**Features:**
- **Remote-friendly navigation** - Works perfectly with Fire TV remote
- **Large TV interface** - Optimized for living room viewing
- **Direct dashboard access** - Opens your web dashboard in full screen
- **Quick media library access** - One-click to Jellyfin, Live TV, etc.
- **Connection status** - Shows if server is reachable

## 🚀 Quick Start

### For Grandma (Web Dashboard):
1. Open web browser
2. Go to: http://your-server-ip:8600
3. Type what you want to watch (e.g., "funny movies")
4. Click "Download This" or "Watch Now"
5. Follow the simple instructions

### For Fire TV:
1. Build the APK (see Android Studio setup below)
2. Sideload to Fire TV
3. Launch "Grandma's Media Center"
4. Use remote to navigate large buttons
5. Everything opens full-screen

## 📋 What Grandmother Can Do

### Search & Find Content:
- Type: "cooking shows" → Gets cooking-related content
- Type: "batman movies" → Shows all Batman films
- Type: "funny" → Finds comedies
- Type: "british" → Finds British content

### Download New Content:
1. Search for what she wants
2. Click the big "Download This" button
3. Get a clear message: "Batman will be ready in 20 minutes"
4. Instructions tell her exactly where to find it when ready

### Watch Existing Content:
- Click "Watch Movies & TV" → Opens full media library
- Click "Live TV" → Watch/record live television
- Click "Books" → Digital book library

### Get Help:
- Click "Help & Instructions" → Clear guide on using everything
- Status bar shows if everything is working
- Simple error messages if something goes wrong

## 🔧 Technical Setup

### Required API Keys:
Add these to your `.env` file:
```bash
RADARR_API_KEY=your_radarr_api_key
SONARR_API_KEY=your_sonarr_api_key
JACKETT_API_KEY=your_jackett_api_key
DOMAIN=your.domain.com
```

### Fire TV App Development:
1. Install Android Studio
2. Open project in `firetv-app/` folder
3. Update server IP in MainActivity.java (line 25)
4. Build APK: Build → Build Bundle(s) / APK(s) → Build APK(s)
5. Sideload using ADB or Apps2Fire

### Network Access:
- Dashboard: Port 8600
- Make sure firewall allows access from grandmother's devices
- Consider setting up DNS name instead of IP address

## 🎯 Why This Works for Grandma

### Web Dashboard:
✅ **No technical terms** - "Download This" not "Add to Radarr queue"  
✅ **Clear feedback** - Shows exactly what's happening  
✅ **Simple language** - "Your movie will be ready in 20 minutes"  
✅ **Error handling** - "Try different keywords" not "API timeout"  
✅ **Visual status** - Green checkmarks, clear instructions  

### Fire TV App:
✅ **Remote-only navigation** - No touchscreen required  
✅ **Large buttons** - Easy to see from across the room  
✅ **Simple layout** - 4 big buttons, that's it  
✅ **Immediate feedback** - Shows when connecting, loading, etc.  
✅ **Full-screen experience** - No confusing menus or small text  

## 📞 Support Instructions for Family

When grandmother needs help:
1. Check status at: http://your-server:8600/api/status
2. Look at dashboard - status bar shows what's happening
3. Common issues:
   - "Can't find anything" → Check API keys in .env
   - "Download failed" → Check if services are running
   - "Nothing appears" → Check if downloads folder is mounted

## 🎊 Success Metrics

Your grandmother solution is working when:
- She can find and download content without calling for help
- Downloads actually complete and appear where expected  
- She feels confident using it independently
- The Fire TV app launches reliably from the couch
- Error messages make sense to non-technical users

---

**Need help?** Check logs: `docker logs mediastack-grandma-dashboard`  
**Want to customize?** Edit `grandma-dashboard/app.py` and rebuild
EOF

print_status "Setup guide created: GRANDMOTHER_SETUP.md"

echo ""
echo "🎉 Grandmother's Media Center Setup Complete!"
echo "=============================================="
echo ""
print_status "✅ Web Dashboard: http://localhost:8600"
print_status "✅ Fire TV App: Ready to build in firetv-app/"
print_status "✅ Setup Guide: GRANDMOTHER_SETUP.md"
echo ""
print_info "Next Steps:"
echo "1. Add your API keys to .env file"
echo "2. Test the web dashboard at http://localhost:8600"
echo "3. Build the Fire TV APK using Android Studio"
echo "4. Give grandmother a simple demo!"
echo ""
print_warning "Remember: This is designed for ACTUAL grandmother use!"
print_warning "Test everything with a non-technical person first."
