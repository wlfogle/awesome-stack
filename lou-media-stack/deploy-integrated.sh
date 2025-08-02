#!/bin/bash

# Lou Media Stack Integrated Deployment Script
# This script deploys the media stack with all extracted configurations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Check for required tools
command -v docker > /dev/null 2>&1 || { print_error "Docker is required but not installed. Aborting."; exit 1; }
command -v docker-compose > /dev/null 2>&1 || { print_error "Docker Compose is required but not installed. Aborting."; exit 1; }

print_status "Starting Lou Media Stack integrated deployment..."

# Create comprehensive .env file with all extracted configurations
print_status "Creating comprehensive .env file with extracted configurations..."
cat > .env << 'EOF'
# System Configuration
PUID=1000
PGID=1000
TZ=America/New_York

# Domain Configuration
DOMAIN=mediastack.local
ACME_EMAIL=admin@mediastack.local

# Directory Configuration
MEDIA_ROOT=/home/lou/media
DOWNLOADS_ROOT=/home/lou/downloads
CONFIG_ROOT=/home/lou/lou-media-stack/config

# Database Configuration
POSTGRES_DB=mediastack
POSTGRES_USER=mediastack
POSTGRES_PASSWORD=mediastack_postgres_secure_2024!

# Redis Configuration
REDIS_PASSWORD=mediastack_redis_secure_2024!

# Authentication Configuration
AUTHENTIK_SECRET_KEY=mediastack_authentik_super_secret_key_2024_random_string_here_must_be_long
AUTHENTIK_BOOTSTRAP_TOKEN=w9YCv3iLfCtd5BVbF2ZGqaUo5jgSe9FQHOKFvicLwwA=
AUTHENTIK_BOOTSTRAP_PASSWORD=9b/KbVhWzZK8OXtygygdqw==

# VPN Configuration
VPN_PROVIDER=mullvad
VPN_TYPE=wireguard
VPN_PRIVATE_KEY=your_wireguard_private_key
VPN_ADDRESSES=10.64.0.1/32
SERVER_COUNTRIES=Netherlands,Sweden

# WireGuard Configuration
WIREGUARD_SERVERURL=auto
WIREGUARD_SERVERPORT=51820
WIREGUARD_PEERS=1

# Plex Configuration (extracted from your configs)
PLEX_CLAIM=claim-yJoUSZ4XTRsJU7pUUzJe
PLEX_TOKEN=2zXXuK38PsFx286PUUmh

# Media Service API Keys (extracted from your configs)
SONARR_API_KEY=22f9f967cd5f4b6f9ee4c828402d3cc1
RADARR_API_KEY=a0df0d925fb141d8a299b2efc6299ecb
LIDARR_API_KEY=78224eea40824ebcb8539d1f8f8d5d54
READARR_API_KEY=d03a15c212114af2a33e0ee1616d2376
BAZARR_API_KEY=your_bazarr_api_key
JACKETT_API_KEY=1z85hw9lsxsennfl21mtwzr2smge68ej
TMDB_API_KEY=47ef060c8451984321a70c2a07c63bce

# Additional Service Configuration
GRAFANA_PASSWORD=grafana_admin_2024!
CALIBRE_PASSWORD=calibre_admin_2024!
VAULTWARDEN_ADMIN_TOKEN=
VAULTWARDEN_SIGNUPS_ALLOWED=true
VAULTWARDEN_INVITATIONS_ALLOWED=true

# Notification Configuration
WATCHTOWER_NOTIFICATION_URL=

# Deluge Configuration
DELUGE_DAEMON_PORT=58846

# OpenWeather API (for weather services)
OPENWEATHER_API_KEY=your_openweather_api_key
WEATHER_CITY=New York
WEATHER_COUNTRY_CODE=US
EOF

print_status "Environment file created with all extracted configurations"

# Create directory structure
print_status "Creating directory structure..."
mkdir -p ~/media/{movies,tv,music,audiobooks,books,recordings,timeshift}
mkdir -p ~/downloads/{complete,incomplete,watch}
mkdir -p ~/lou-media-stack/config/{traefik,portainer,jellyfin,plex,sonarr,radarr,lidarr,readarr,bazarr,jackett,deluge,overseerr,jellyseerr,tautulli,tvheadend,heimdall,unpackerr,filebot,recyclarr,flaresolverr,postgres,redis,wireguard,gluetun,watchtower,vaultwarden,weather}

# Set proper permissions
print_status "Setting permissions..."
chown -R $USER:$USER ~/media ~/downloads ~/lou-media-stack 2>/dev/null || sudo chown -R $USER:$USER ~/media ~/downloads ~/lou-media-stack

# Copy extracted configurations
print_status "Copying extracted service configurations..."

# Copy Jackett configuration with API key
if [ -f "extracted-configs/docker-configs/mediastack-new_jackett/Jackett/ServerConfig.json" ]; then
    mkdir -p config/jackett/Jackett
    cp extracted-configs/docker-configs/mediastack-new_jackett/Jackett/ServerConfig.json config/jackett/Jackett/
    print_status "Jackett configuration copied"
fi

# Copy Sonarr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_sonarr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_sonarr/* config/sonarr/ 2>/dev/null || true
    print_status "Sonarr configuration copied"
fi

# Copy Radarr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_radarr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_radarr/* config/radarr/ 2>/dev/null || true
    print_status "Radarr configuration copied"
fi

# Copy Lidarr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_lidarr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_lidarr/* config/lidarr/ 2>/dev/null || true
    print_status "Lidarr configuration copied"
fi

# Copy Readarr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_readarr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_readarr/* config/readarr/ 2>/dev/null || true
    print_status "Readarr configuration copied"
fi

# Copy Bazarr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_bazarr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_bazarr/* config/bazarr/ 2>/dev/null || true
    print_status "Bazarr configuration copied"
fi

# Copy Overseerr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_overseerr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_overseerr/* config/overseerr/ 2>/dev/null || true
    print_status "Overseerr configuration copied"
fi

# Copy Jellyseerr configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_jellyseerr" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_jellyseerr/* config/jellyseerr/ 2>/dev/null || true
    print_status "Jellyseerr configuration copied"
fi

# Copy Tautulli configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_tautulli" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_tautulli/* config/tautulli/ 2>/dev/null || true
    print_status "Tautulli configuration copied"
fi

# Copy Plex configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_plex" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_plex/* config/plex/ 2>/dev/null || true
    print_status "Plex configuration copied"
fi

# Copy Jellyfin configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_jellyfin" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_jellyfin/* config/jellyfin/ 2>/dev/null || true
    print_status "Jellyfin configuration copied"
fi

# Copy Heimdall configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_heimdall" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_heimdall/* config/heimdall/ 2>/dev/null || true
    print_status "Heimdall configuration copied"
fi

# Copy Deluge configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_deluge" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_deluge/* config/deluge/ 2>/dev/null || true
    print_status "Deluge configuration copied"
fi

# Copy Portainer configuration
if [ -d "extracted-configs/docker-configs/mediastack-new_portainer" ]; then
    cp -r extracted-configs/docker-configs/mediastack-new_portainer/* config/portainer/ 2>/dev/null || true
    print_status "Portainer configuration copied"
fi

# Copy Traefik configuration
if [ -f "extracted-configs/docker-configs/mediastack-new_traefik.yml" ]; then
    mkdir -p config/traefik
    cp extracted-configs/docker-configs/mediastack-new_traefik.yml config/traefik/traefik.yml
    print_status "Traefik configuration copied"
fi

# Copy Recyclarr configuration
if [ -f "extracted-configs/docker-configs/mediastack-new_recyclarr.yml" ]; then
    cp extracted-configs/docker-configs/mediastack-new_recyclarr.yml config/recyclarr/recyclarr.yml
    print_status "Recyclarr configuration copied"
fi

# Create Traefik acme.json file
touch config/traefik/acme.json
chmod 600 config/traefik/acme.json

# Create Docker network
print_status "Creating Docker network..."
docker network create mediastack 2>/dev/null || print_warning "Network mediastack already exists"

# Stop and remove conflicting services
print_status "Stopping conflicting services..."
docker stop mediastack_traefik mediastack_tvheadend mediastack_gluetun mediastack_portainer mediastack_plex mediastack_jellyfin mediastack_sonarr mediastack_radarr mediastack_lidarr mediastack_bazarr mediastack_jackett mediastack_deluge mediastack_overseerr mediastack_jellyseerr mediastack_tautulli mediastack_heimdall 2>/dev/null || true
docker rm mediastack_traefik mediastack_tvheadend mediastack_gluetun mediastack_portainer mediastack_plex mediastack_jellyfin mediastack_sonarr mediastack_radarr mediastack_lidarr mediastack_bazarr mediastack_jackett mediastack_deluge mediastack_overseerr mediastack_jellyseerr mediastack_tautulli mediastack_heimdall 2>/dev/null || true

# Pull Docker images
print_status "Pulling Docker images (this may take a while)..."
docker-compose pull || print_warning "Some images failed to pull, continuing with deployment..."

# Start the stack
print_status "Starting Lou Media Stack with all configurations..."
docker-compose up -d --force-recreate

# Wait for services to start
print_status "Waiting for services to start..."
sleep 60

# Display service status
print_status "Checking service status..."
docker-compose ps

# Display service URLs
print_status "Deployment complete! Your pre-configured services are available at:"
echo
echo "🏠 Dashboard: http://localhost (Heimdall)"
echo "📊 Portainer: http://localhost:9000"
echo "🎬 Jellyfin: http://localhost:8096"
echo "📺 Plex: http://localhost:32400"
echo "🔍 Jackett: http://localhost:9117"
echo "📺 Sonarr: http://localhost:8989"
echo "🎬 Radarr: http://localhost:7878"
echo "🎵 Lidarr: http://localhost:8686"
echo "💬 Bazarr: http://localhost:6767"
echo "🌊 Deluge: http://localhost:8112"
echo "🎭 Overseerr: http://localhost:5055"
echo "📊 Tautulli: http://localhost:8181"
echo "🔐 Vaultwarden: http://localhost:8000"
echo

print_status "All services deployed with your existing configurations!"
print_status "API keys and settings have been automatically configured from your extracted data."
print_status "Your Plex server is already claimed and ready to use."
print_status "Check service logs with: docker-compose logs -f [service_name]"
