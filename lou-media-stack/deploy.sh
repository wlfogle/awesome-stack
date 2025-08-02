#!/bin/bash

# Lou Media Stack Deployment Script
# This script sets up the directory structure and deploys the media stack

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
command -v docker >/dev/null 2>&1 || { print_error "Docker is required but not installed. Aborting."; exit 1; }
command -v docker-compose >/dev/null 2>&1 || { print_error "Docker Compose is required but not installed. Aborting."; exit 1; }

print_status "Starting Lou Media Stack deployment..."

# Create directory structure
print_status "Creating directory structure..."

# Main directories
mkdir -p ~/media/{movies,tv,music,audiobooks,books}
mkdir -p ~/downloads/{complete,incomplete,watch}
mkdir -p ~/lou-media-stack/config/{traefik,portainer,jellyfin,plex,sonarr,radarr,lidarr,readarr,bazarr,jackett,deluge,overseerr,jellyseerr,tautulli,tvheadend,heimdall,unpackerr,filebot,recyclarr,flaresolvr,postgres,redis,wireguard,gluetun,watchtower}

# Set proper permissions
print_status "Setting permissions..."
sudo chown -R $USER:$USER ~/media
sudo chown -R $USER:$USER ~/downloads
sudo chown -R $USER:$USER ~/lou-media-stack

# Check if .env file exists
if [ ! -f ".env" ]; then
    print_warning ".env file not found. Creating from template..."
    cp .env.example .env
    print_warning "Please edit .env file with your configuration before continuing."
    print_warning "Pay special attention to:"
    print_warning "- DOMAIN and ACME_EMAIL settings"
    print_warning "- VPN configuration (VPN_PROVIDER, VPN_PRIVATE_KEY, etc.)"
    print_warning "- Database passwords"
    print_warning "- Plex claim token"
    echo
    read -p "Press Enter after you've configured the .env file..."
fi


# Source environment variables
source .env

# Create Traefik configuration
print_status "Creating Traefik configuration..."
cat > config/traefik/traefik.yml << EOF
global:
  checkNewVersion: false
  sendAnonymousUsage: false

api:
  dashboard: true
  insecure: false

entryPoints:
  web:
    address: ":80"
    http:
      redirections:
        entrypoint:
          to: websecure
          scheme: https
          permanent: true
  websecure:
    address: ":443"

certificatesResolvers:
  letsencrypt:
    acme:
      email: ${ACME_EMAIL}
      storage: /acme.json
      httpChallenge:
        entryPoint: web

providers:
  docker:
    endpoint: "unix:///var/run/docker.sock"
    exposedByDefault: false
    network: mediastack
EOF

# Create Traefik acme.json file
touch config/traefik/acme.json
chmod 600 config/traefik/acme.json

# Create initial Jackett configuration
print_status "Creating Jackett configuration..."
mkdir -p config/jackett/Jackett
cat > config/jackett/Jackett/ServerConfig.json << EOF
{
  "Port": 9117,
  "AllowExternal": true,
  "APIKey": "$(openssl rand -hex 16)",
  "AdminPassword": "",
  "InstanceId": "$(uuidgen)",
  "BlackholeDir": "/downloads/watch",
  "UpdateDisabled": false,
  "UpdatePrerelease": false,
  "BasePathOverride": "",
  "CacheEnabled": true,
  "CacheTtl": 2100,
  "CacheMaxResultsPerIndexer": 1000,
  "FlareSolverrUrl": "http://flaresolverr:8191/",
  "OmdbApiKey": "",
  "OmdbApiUrl": "https://www.omdbapi.com/"
}
EOF

# Create FlareSolverr configuration
print_status "Creating FlareSolverr configuration..."
mkdir -p config/flaresolverr

# Create Deluge configuration
print_status "Creating Deluge configuration..."
mkdir -p config/deluge
cat > config/deluge/core.conf << EOF
{
  "file_version": 1,
  "format_version": 1
}{
  "add_paused": false,
  "allow_remote": true,
  "auto_managed": true,
  "cache_expiry": 60,
  "cache_size": 512,
  "copy_torrent_file": true,
  "daemon_port": ${DELUGE_DAEMON_PORT:-58846},
  "del_copy_torrent_file": true,
  "download_location": "/downloads/incomplete",
  "enabled_plugins": ["AutoAdd", "Label"],
  "enc_in_policy": 1,
  "enc_level": 2,
  "enc_out_policy": 1,
  "ignore_limits_on_local_network": true,
  "info_sent": 0.0,
  "listen_ports": [6881, 6891],
  "max_active_downloading": 3,
  "max_active_limit": 8,
  "max_active_seeding": 5,
  "max_connections_global": 200,
  "max_connections_per_torrent": 60,
  "max_download_speed": -1.0,
  "max_half_open_connections": 50,
  "max_upload_slots_global": 4,
  "max_upload_slots_per_torrent": 3,
  "max_upload_speed": -1.0,
  "move_completed": true,
  "move_completed_path": "/downloads/complete",
  "natpmp": true,
  "path": "/downloads/incomplete",
  "plugins_location": "/config/plugins",
  "queue_new_to_top": false,
  "random_outgoing_ports": true,
  "random_port": true,
  "remove_seed_at_ratio": false,
  "seed_time_limit": 180,
  "seed_time_ratio_limit": 7.0,
  "share_ratio_limit": 2.0,
  "stop_seed_at_ratio": false,
  "stop_seed_ratio": 2.0,
  "torrentfiles_location": "/config/torrents",
  "upnp": true,
  "utpex": true
}
EOF

# Create Docker network
print_status "Creating Docker network..."
docker network create mediastack 2>/dev/null || print_warning "Network mediastack already exists"

# Pull Docker images
print_status "Pulling Docker images (this may take a while)..."
docker-compose pull || print_warning "Some images failed to pull, continuing with deployment..."

# Stop and remove conflicting services
print_status "Stopping conflicting services..."
docker stop mediastack_traefik mediastack_tvheadend mediastack_gluetun 2>/dev/null || true
docker rm mediastack_traefik mediastack_tvheadend mediastack_gluetun 2>/dev/null || true

# Start the stack
print_status "Starting Lou Media Stack..."
docker-compose up -d --force-recreate

# Wait for services to start
print_status "Waiting for services to start..."
sleep 30

# Display service URLs
print_status "Deployment complete! Your services are available at:"
echo
echo "🏠 Dashboard: https://heimdall.${DOMAIN}"
echo "📊 Portainer: https://portainer.${DOMAIN}"
echo "🎬 Jellyfin: https://jellyfin.${DOMAIN}"
echo "📺 Plex: https://plex.${DOMAIN}"
echo "🔍 Jackett: https://jackett.${DOMAIN}"
echo "📺 Sonarr: https://sonarr.${DOMAIN}"
echo "🎬 Radarr: https://radarr.${DOMAIN}"
echo "🎵 Lidarr: https://lidarr.${DOMAIN}"
echo "📚 Readarr: https://readarr.${DOMAIN}"
echo "💬 Bazarr: https://bazarr.${DOMAIN}"
echo "🌊 Deluge: https://deluge.${DOMAIN}"
echo "🎭 Overseerr: https://overseerr.${DOMAIN}"
echo "📊 Tautulli: https://tautulli.${DOMAIN}"
echo "📺 TVHeadend: https://tvheadend.${DOMAIN}"
echo
print_status "Initial setup tips:"
echo "1. Configure your indexers in Jackett first"
echo "2. Set up your *arr applications (Sonarr, Radarr, etc.)"
echo "3. Configure Overseerr to connect to your *arr apps"
echo "4. Add your media libraries to Jellyfin and Plex"
echo
print_warning "Don't forget to configure your VPN settings in Gluetun!"
print_status "Check service logs with: docker-compose logs -f [service_name]"
