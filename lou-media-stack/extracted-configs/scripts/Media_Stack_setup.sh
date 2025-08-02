#!/bin/bash

echo "Setting up *arr Media Stack..."

# Create directories with proper permissions
sudo chown -R $(id -u):$(id -g) .
chmod +x setup.sh

# Start the services
echo "Starting Docker containers..."
docker-compose up -d

echo ""
echo "Media Stack Setup Complete!"
echo ""
echo "Access your services at:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎬 Jellyfin (Media Server):      http://localhost:8096"
echo "📺 Sonarr (TV Shows):           http://localhost:8989"
echo "🎭 Radarr (Movies):             http://localhost:7878"
echo "🔍 Prowlarr (Indexers):         http://localhost:9696"
echo "⬇️  qBittorrent (Downloads):     http://localhost:8080"
echo "📝 Bazarr (Subtitles):          http://localhost:6767"
echo "🔧 Jackett (Alt Indexers):      http://localhost:9117"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⚠️  IMPORTANT SETUP STEPS:"
echo "1. Configure qBittorrent first (default login: admin/adminadmin)"
echo "2. Set up indexers in Prowlarr"
echo "3. Connect Sonarr and Radarr to Prowlarr"
echo "4. Configure download client in Sonarr/Radarr"
echo "5. Add media libraries to Jellyfin"
echo ""
echo "💡 Check the README.md for detailed setup instructions!"
