#!/bin/bash

# Grandma-Friendly TV Stack Configuration Script
# This configures a simple, integrated TV solution that "just works"

set -e

echo "🏠 Setting up Grandma-Friendly TV Stack..."

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

# Check if .env exists
if [[ ! -f .env ]]; then
    print_error "No .env file found! Please create one first."
    exit 1
fi

# Load environment variables
source .env

print_info "Configuring TV stack for domain: ${DOMAIN}"

# Create required directories
print_status "Creating directory structure..."
mkdir -p "${MEDIA_ROOT}/recordings"
mkdir -p "${MEDIA_ROOT}/timeshift" 
mkdir -p "${MEDIA_ROOT}/xteve_guide"

# Set proper permissions
sudo chown -R ${PUID}:${PGID} "${MEDIA_ROOT}/recordings" "${MEDIA_ROOT}/timeshift" "${MEDIA_ROOT}/xteve_guide"

print_status "Directory structure created"

# Start the simplified TV stack
print_status "Starting TV services..."
docker-compose -f docker-compose-unified.yml up -d xteve

# Wait for services to start
sleep 10

# Check service status
if docker ps | grep -q "mediastack-xteve"; then
    print_status "xTeve is running"
else
    print_error "xTeve failed to start"
    exit 1
fi

print_info "==============================================="
print_info "🎉 GRANDMA-FRIENDLY TV STACK IS READY!"
print_info "==============================================="
echo
print_info "Your TV hub is available at:"
echo -e "  📺 Main TV Interface: ${GREEN}https://tv.${DOMAIN}${NC}"
echo -e "  🌐 Direct Access: ${GREEN}http://localhost:8220${NC}"
echo

print_info "SIMPLE SETUP GUIDE:"
echo "1. 📡 Add your IPTV playlists (M3U files) to xTeve"
echo "2. 🔗 Add your EPG sources (XMLTV files) for channel guide"
echo "3. ⚙️  Map channels and enable the ones you want"
echo "4. 📱 Add the xTeve URL to Plex/Jellyfin as a TV source"
echo

print_info "xTeve will handle:"
echo "  ✅ IPTV channels and EPG"
echo "  ✅ Channel filtering and mapping"
echo "  ✅ Buffer management"
echo "  ✅ Integration with Plex/Jellyfin"
echo

print_info "For Plex/Jellyfin integration, use:"
echo -e "  📺 M3U URL: ${GREEN}http://mediastack-xteve:34400/m3u/xteve.m3u${NC}"
echo -e "  📅 EPG URL: ${GREEN}http://mediastack-xteve:34400/xmltv/xteve.xml${NC}"
echo

print_warning "First-time setup:"
echo "1. Visit https://tv.${DOMAIN} to configure xTeve"
echo "2. Go through the setup wizard"
echo "3. Add your IPTV providers and EPG sources"
echo "4. Configure your media servers (Plex/Jellyfin)"

echo
print_status "TV Stack configuration complete! 🎉"
