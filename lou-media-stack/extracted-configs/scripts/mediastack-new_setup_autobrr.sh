#!/bin/bash

# Autobrr Setup Script for Public Trackers
# This script configures Autobrr with API keys from running services

echo "🚀 Setting up Autobrr with public tracker configuration..."

# Wait for services to be ready
echo "⏳ Waiting for services to start..."
sleep 10

# Function to get API key from service
get_api_key() {
    local service=$1
    local port=$2
    local endpoint=$3
    
    echo "📡 Getting API key for $service..."
    
    # Try to get API key from service
    local api_key=$(curl -s "http://localhost:$port$endpoint" | grep -o '"apiKey":"[^"]*' | cut -d'"' -f4)
    
    if [ -z "$api_key" ]; then
        echo "⚠️  Could not get API key for $service automatically"
        echo "   Please configure manually in Autobrr web interface"
        return 1
    else
        echo "✅ API key found for $service: ${api_key:0:10}..."
        echo "$api_key"
        return 0
    fi
}

# Create basic configuration directory
mkdir -p /mnt/sda1/config/autobrr

# Get API keys (these will need to be configured manually in the web interface)
echo "📋 API Keys needed for configuration:"
echo "   - Radarr: http://localhost:7878 -> Settings -> General -> API Key"
echo "   - Sonarr: http://localhost:8989 -> Settings -> General -> API Key"  
echo "   - Lidarr: http://localhost:8686 -> Settings -> General -> API Key"
echo ""

# Create a simple tracker configuration for commonly available public trackers
cat > /mnt/sda1/config/autobrr/public_trackers.json << 'EOF'
{
  "public_trackers": [
    {
      "name": "YTS",
      "irc_server": "irc.rizon.net",
      "port": 6697,
      "ssl": true,
      "channel": "#yts",
      "categories": ["Movies"],
      "notes": "Movies only, good for 720p/1080p"
    },
    {
      "name": "EZTV",
      "irc_server": "irc.rizon.net", 
      "port": 6697,
      "ssl": true,
      "channel": "#eztv",
      "categories": ["TV"],
      "notes": "TV shows, reliable releases"
    },
    {
      "name": "TGx",
      "irc_server": "irc.rizon.net",
      "port": 6697,
      "ssl": true,
      "channel": "#tgx",
      "categories": ["Movies", "TV", "Games"],
      "notes": "General tracker, good variety"
    },
    {
      "name": "Nyaa",
      "irc_server": "irc.rizon.net",
      "port": 6697,
      "ssl": true,
      "channel": "#nyaa",
      "categories": ["Anime"],
      "notes": "Anime content"
    }
  ]
}
EOF

echo "✅ Public tracker configuration created at /mnt/sda1/config/autobrr/public_trackers.json"

# Create basic filters
cat > /mnt/sda1/config/autobrr/basic_filters.json << 'EOF'
{
  "filters": [
    {
      "name": "Movies 1080p Quality",
      "enabled": true,
      "trackers": ["*"],
      "categories": ["Movies"],
      "resolutions": ["1080p"],
      "sources": ["BluRay", "WEB-DL", "WEBRip"],
      "exclude_keywords": ["CAM", "TS", "HDCAM", "HDTS", "WORKPRINT"],
      "min_size": "1GB",
      "max_size": "15GB",
      "action": "radarr"
    },
    {
      "name": "TV Shows HD",
      "enabled": true,
      "trackers": ["*"],
      "categories": ["TV"],
      "resolutions": ["1080p", "720p"],
      "sources": ["BluRay", "WEB-DL", "WEBRip"],
      "exclude_keywords": ["CAM", "TS", "HDCAM"],
      "min_size": "200MB",
      "max_size": "8GB",
      "action": "sonarr"
    },
    {
      "name": "Music Lossless",
      "enabled": true,
      "trackers": ["*"],
      "categories": ["Music"],
      "formats": ["FLAC", "APE", "ALAC"],
      "exclude_keywords": ["MP3", "AAC", "128", "192"],
      "min_size": "50MB",
      "max_size": "2GB",
      "action": "lidarr"
    }
  ]
}
EOF

echo "✅ Basic filters created at /mnt/sda1/config/autobrr/basic_filters.json"

# Set proper permissions
chown -R 1000:1000 /mnt/sda1/config/autobrr/

echo ""
echo "🎉 Autobrr setup complete!"
echo ""
echo "📋 Next steps:"
echo "1. Access Autobrr at: http://localhost:7474"
echo "2. Create admin account"
echo "3. Go to Settings -> IRC and add public tracker servers"
echo "4. Go to Settings -> Apps and add your *arr services with API keys"
echo "5. Go to Filters and import the basic filters"
echo ""
echo "🌐 Public tracker IRC servers to add:"
echo "   - irc.rizon.net:6697 (SSL) - channels: #yts, #eztv, #tgx, #nyaa"
echo "   - irc.p2p-network.net:6667 - channels: #announce"
echo ""
echo "⚠️  Remember: Use VPN for all torrent activity!"

EOF
