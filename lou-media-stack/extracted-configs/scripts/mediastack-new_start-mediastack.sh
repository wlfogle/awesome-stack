#!/bin/bash

# MediaStack Priority Startup Script
# Based on MEDIASTACK_PRIORITY_GUIDE.md

echo "Starting MediaStack using Priority Guide Order..."
echo "=================================================="

# Change to config directory
cd /home/lou/mediastack-new/config

# Check if files exist
if [ ! -f "docker-compose.yml" ]; then
    echo "Error: docker-compose.yml not found!"
    exit 1
fi

if [ ! -f ".env" ]; then
    echo "Error: .env file not found!"
    exit 1
fi

# Create Docker network
echo "Creating Docker network..."
docker network create mediastack 2>/dev/null || echo "Network already exists"

echo ""
echo "=== Phase 1: Core Infrastructure ==="
echo ""

# Phase 1: Core Infrastructure (Start in order)
echo "1. Starting VPN (Gluetun) - CRITICAL FIRST..."
docker compose up gluetun -d
sleep 10

echo "2. Starting Database (PostgreSQL)..."
docker compose up postgres -d
sleep 15

echo "3. Starting Cache (Valkey/Redis)..."
docker compose up valkey -d
sleep 10

echo "4. Starting Authentication (Authentik)..."
docker compose up authentik authentik-worker -d
sleep 15

echo "5. Starting Reverse Proxy (Traefik)..."
docker compose up traefik -d
sleep 10

echo ""
echo "=== Phase 2: Essential Media Services ==="
echo ""

echo "6. Starting Indexer Management (Jackett)..."
docker compose up jackett -d
sleep 10

echo "7. Starting Download Client (Deluge)..."
docker compose up deluge -d
sleep 10

echo "8. Starting Media Management (Sonarr, Radarr, Lidarr)..."
docker compose up sonarr radarr lidarr -d
sleep 15

echo ""
echo "=== Phase 3: Media Servers ==="
echo ""

echo "9. Starting Media Servers (Jellyfin, Plex)..."
docker compose up jellyfin plex -d
sleep 10

echo "10. Starting Subtitles (Bazarr)..."
docker compose up bazarr -d
sleep 10

echo ""
echo "=== Phase 4: Enhancement Services ==="
echo ""

echo "11. Starting Enhancement Services..."
docker compose up tvapp2 portainer heimdall homarr jellyseerr overseerr flaresolverr unpackerr tdarr -d

echo ""
echo "=== MediaStack Started Successfully! ==="
echo ""

# Show running containers
echo "Running Services:"
echo "=================="
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

echo ""
echo "🔥 PHASE 1 CONFIGURATION REQUIRED:"
echo "=================================="
echo "Before using the stack, configure these services in order:"
echo ""
echo "1. VPN Test: docker exec gluetun wget -qO- ifconfig.me"
echo "2. Traefik Dashboard: http://localhost:8080"
echo "3. Authentik Setup: http://localhost:9000 (admin/change_this_bootstrap_pass_ABC!)"
echo "4. Database: PostgreSQL running on port 5432"
echo ""
echo "🎯 MEDIA SERVICES ACCESS:"
echo "========================"
echo "- Jackett: http://localhost:9117 (Configure indexers first)"
echo "- Deluge: http://localhost:8112 (Default password: deluge)"
echo "- Sonarr: http://localhost:8989"
echo "- Radarr: http://localhost:7878"
echo "- Lidarr: http://localhost:8686"
echo "- Jellyfin: http://localhost:8096"
echo "- Plex: http://localhost:32400"
echo "- Bazarr: http://localhost:6767"
echo ""
echo "📚 Follow the configuration steps in MEDIASTACK_PRIORITY_GUIDE.md"
echo "   Located at: /home/lou/MEDIASTACK_PRIORITY_GUIDE.md"
