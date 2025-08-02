#!/bin/bash
# Unified OTA/IPTV EPG Startup Script

set -e

echo "Starting Unified OTA/IPTV EPG System..."
echo "======================================"

# Change to config directory
cd /home/lou/mediastack-new/config

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "Error: Docker is not running. Please start Docker first."
    exit 1
fi

# Update unified playlist and EPG
echo "Updating unified playlist and EPG..."
python3 unified-epg-setup.py

# Start Enhanced PseudoTV server with Plex integration
echo "Starting Enhanced PseudoTV server with Plex integration..."
python3 pseudotv-plex-server.py &
PSEUDOTV_PID=$!
echo "Enhanced PseudoTV server started with PID: $PSEUDOTV_PID"

# Start Docker services
echo "Starting Docker services..."
docker-compose up -d

# Wait for services to start
echo "Waiting for services to start..."
sleep 10

# Check service status
echo "Checking service status..."
docker-compose ps

echo ""
echo "Unified EPG System Started!"
echo "=========================="
echo ""
echo "Access Points:"
echo "- Unified Playlist: http://localhost:8888/playlist.m3u"
echo "- Unified EPG: http://localhost:8888/epg.xml"
echo "- TVApp2 Web UI: http://localhost:8888"
echo "- TVHeadend Web UI: http://localhost:9981"
echo "- Jellyfin Web UI: http://localhost:8096"
echo "- PseudoTV Server: http://localhost:8890"
echo "- Homepage Dashboard: http://localhost:3001"
echo ""
echo "Configuration:"
echo "- Total channels: $(grep -c '#EXTINF' /home/lou/mediastack-new/data/tvapp2/playlists/unified_playlist.m3u)"
echo "- OTA channels: $(grep -c 'OTA Channels' /home/lou/mediastack-new/data/tvapp2/playlists/unified_playlist.m3u)"
echo "- IPTV channels: $(grep -c 'group-title=' /home/lou/mediastack-new/data/tvapp2/playlists/unified_playlist.m3u | grep -v 'OTA Channels')"
echo "- EPG size: $(ls -lh /home/lou/mediastack-new/data/tvapp2/epg/unified_epg.xml | awk '{print $5}')"
echo ""
echo "Next steps:"
echo "1. Configure TVHeadend at http://localhost:9981 (see tvheadend_unified_setup.txt)"
echo "2. Configure Jellyfin Live TV at http://localhost:8096 (see jellyfin_livetv_setup.txt)"
echo "3. Test OTA channels and IPTV streams"
echo ""
echo "To update channels and EPG, run: python3 unified-epg-setup.py"
