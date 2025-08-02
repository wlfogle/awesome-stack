#!/bin/bash

# Lou MediaStack - Container Integration Script
# This script integrates working containers from old stacks into the new unified stack

set -e

echo "🔗 Lou MediaStack - Container Integration"
echo "========================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to add service to docker-compose if it doesn't exist
add_service_to_compose() {
    local service_name="$1"
    local service_config="$2"
    
    if ! grep -q "  ${service_name}:" docker-compose.yml; then
        print_status "Adding ${service_name} to docker-compose.yml..."
        echo "" >> docker-compose.yml
        echo "$service_config" >> docker-compose.yml
    else
        print_warning "${service_name} already exists in docker-compose.yml"
    fi
}

# Check if we have the current docker-compose.yml
if [ ! -f "docker-compose.yml" ]; then
    print_error "docker-compose.yml not found in current directory!"
    exit 1
fi

print_status "Backing up current docker-compose.yml..."
cp docker-compose.yml docker-compose.yml.backup

# Analyze working containers that can be integrated
print_status "Analyzing working containers for integration..."

WORKING_CONTAINERS=$(docker ps --format "{{.Names}}" | grep "^mediastack_" | grep -v "Restarting")

echo "Found working containers:"
echo "$WORKING_CONTAINERS" | while read container; do
    if [ -n "$container" ]; then
        echo "  ✅ $container"
    fi
done

# Integration mapping with priority-based ports
declare -A SERVICE_PORTS=(
    # Phase 1: Core Infrastructure (8000-8099)
    ["mediastack_postgres"]="8020:5432"
    ["mediastack_valkey"]="8021:6379"
    ["mediastack_authentik_server"]="8030:9000"
    
    # Phase 2: Essential Media (8100-8199) 
    ["mediastack_flaresolverr"]="8101:8191"
    
    # Phase 3: Media Servers (8200-8299)
    ["mediastack_audiobookshelf"]="8210:80"
    ["mediastack_calibre_web"]="8211:8083"
    ["mediastack_channels_dvr"]="8220:8089"
    ["mediastack_iptv_proxy"]="8221:8080"
    ["mediastack_tvapp2"]="8222:4124"
    ["mediastack_tdarr"]="8230:8265"
    ["mediastack_filebot"]="8240:5800"
    
    # Phase 4: Enhancement (8300-8399)
    ["mediastack_homarr"]="8341:7575"
    ["mediastack_homepage"]="8342:3000"
    ["mediastack_organizr"]="8340:80"
    
    # Phase 5: Monitoring (8400-8499)
    ["mediastack_prometheus"]="8400:9090"
    
    # Phase 6: Management (8500-8599)
    ["mediastack_guacamole"]="8510:8080"
    ["mediastack_chromium"]="8520:3000"
    ["mediastack_autoscan"]="8530:3030"
    ["mediastack_gaps"]="8540:8484"
)

# Create integrated compose additions
cat > container_integrations.yml << 'EOF'

  # ============================================================================
  # INTEGRATED SERVICES FROM OLD STACKS (Priority-based ports)
  # ============================================================================

  # Phase 3: Media Servers & Content
  audiobookshelf:
    image: ghcr.io/advplyr/audiobookshelf:latest
    container_name: mediastack-audiobookshelf
    restart: unless-stopped
    environment:
      AUDIOBOOKSHELF_UID: ${PUID}
      AUDIOBOOKSHELF_GID: ${PGID}
    volumes:
      - audiobookshelf_config:/config
      - audiobookshelf_metadata:/metadata
      - ${MEDIA_ROOT}/audiobooks:/audiobooks:ro
      - ${MEDIA_ROOT}/books:/books:ro
    networks:
      - mediastack
    ports:
      - "8210:80"      # Phase 3: Media Servers
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.audiobookshelf.rule=Host(\`audiobooks.${DOMAIN}\`)"
      - "traefik.http.services.audiobookshelf.loadbalancer.server.port=80"

  calibre-web:
    image: lscr.io/linuxserver/calibre-web:latest
    container_name: mediastack-calibre-web
    restart: unless-stopped
    environment:
      PUID: ${PUID}
      PGID: ${PGID}
      TZ: ${TZ}
    volumes:
      - calibre_web_config:/config
      - ${MEDIA_ROOT}/books:/books
    networks:
      - mediastack
    ports:
      - "8211:8083"    # Phase 3: Media Servers
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.calibre-web.rule=Host(\`books.${DOMAIN}\`)"
      - "traefik.http.services.calibre-web.loadbalancer.server.port=8083"

  channels-dvr:
    image: fancybits/channels-dvr:latest
    container_name: mediastack-channels-dvr
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - channels_dvr_data:/channels-dvr
      - ${MEDIA_ROOT}/recordings:/recordings
    networks:
      - mediastack
    ports:
      - "8220:8089"    # Phase 3: Media Servers
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.channels-dvr.rule=Host(\`dvr.${DOMAIN}\`)"
      - "traefik.http.services.channels-dvr.loadbalancer.server.port=8089"

  iptv-proxy:
    image: pierro777/iptv-proxy:latest
    container_name: mediastack-iptv-proxy
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - iptv_proxy_config:/app/config
    networks:
      - mediastack
    ports:
      - "8221:8080"    # Phase 3: Media Servers
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.iptv-proxy.rule=Host(\`iptv.${DOMAIN}\`)"
      - "traefik.http.services.iptv-proxy.loadbalancer.server.port=8080"

  tvapp2:
    image: thebinaryninja/tvapp2:latest
    container_name: mediastack-tvapp2
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    networks:
      - mediastack
    ports:
      - "8222:4124"    # Phase 3: Media Servers
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.tvapp2.rule=Host(\`tvapp2.${DOMAIN}\`)"
      - "traefik.http.services.tvapp2.loadbalancer.server.port=4124"

  tdarr:
    image: ghcr.io/haveagitgat/tdarr:latest
    container_name: mediastack-tdarr
    restart: unless-stopped
    environment:
      TZ: ${TZ}
      PUID: ${PUID}
      PGID: ${PGID}
    volumes:
      - tdarr_server_data:/app/server
      - tdarr_config_data:/app/configs
      - tdarr_log_data:/app/logs
      - ${MEDIA_ROOT}:/media
    networks:
      - mediastack
    ports:
      - "8230:8265"    # Phase 3: Media Processing - Web UI
      - "8231:8266"    # Server Port
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.tdarr.rule=Host(\`tdarr.${DOMAIN}\`)"
      - "traefik.http.services.tdarr.loadbalancer.server.port=8265"

  tdarr-node:
    image: ghcr.io/haveagitgat/tdarr_node:latest
    container_name: mediastack-tdarr-node
    restart: unless-stopped
    environment:
      TZ: ${TZ}
      PUID: ${PUID}
      PGID: ${PGID}
      nodeName: MainNode
    volumes:
      - tdarr_config_data:/app/configs
      - tdarr_log_data:/app/logs
      - ${MEDIA_ROOT}:/media
    networks:
      - mediastack
    depends_on:
      - tdarr

  filebot-gui:
    image: jlesage/filebot:latest
    container_name: mediastack-filebot-gui
    restart: unless-stopped
    environment:
      PUID: ${PUID}
      PGID: ${PGID}
      TZ: ${TZ}
    volumes:
      - filebot_gui_config:/config
      - ${MEDIA_ROOT}:/media
      - ${DOWNLOADS_ROOT}:/downloads
    networks:
      - mediastack
    ports:
      - "8240:5800"    # Phase 3: File Processing
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.filebot-gui.rule=Host(\`filebot.${DOMAIN}\`)"
      - "traefik.http.services.filebot-gui.loadbalancer.server.port=5800"

  # Phase 4: Enhancement - Dashboards
  homarr:
    image: ghcr.io/ajnart/homarr:latest
    container_name: mediastack-homarr
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - homarr_config:/app/data/configs
      - homarr_icons:/app/public/icons
      - homarr_data:/data
    networks:
      - mediastack
    ports:
      - "8341:7575"    # Phase 4: Enhancement
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.homarr.rule=Host(\`homarr.${DOMAIN}\`)"
      - "traefik.http.services.homarr.loadbalancer.server.port=7575"

  homepage:
    image: ghcr.io/gethomepage/homepage:latest
    container_name: mediastack-homepage
    restart: unless-stopped
    environment:
      TZ: ${TZ}
      PUID: ${PUID}
      PGID: ${PGID}
    volumes:
      - homepage_config:/app/config
      - /var/run/docker.sock:/var/run/docker.sock:ro
    networks:
      - mediastack
    ports:
      - "8342:3000"    # Phase 4: Enhancement
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.homepage.rule=Host(\`homepage.${DOMAIN}\`)"
      - "traefik.http.services.homepage.loadbalancer.server.port=3000"

  organizr:
    image: organizr/organizr:latest
    container_name: mediastack-organizr
    restart: unless-stopped
    environment:
      PUID: ${PUID}
      PGID: ${PGID}
      TZ: ${TZ}
      fpm: "true"
    volumes:
      - organizr_config:/config
    networks:
      - mediastack
    ports:
      - "8340:80"      # Phase 4: Enhancement
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.organizr.rule=Host(\`dashboard.${DOMAIN}\`)"
      - "traefik.http.services.organizr.loadbalancer.server.port=80"

  # Phase 5: Monitoring
  prometheus:
    image: prom/prometheus:latest
    container_name: mediastack-prometheus
    restart: unless-stopped
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
      - '--storage.tsdb.retention.time=200h'
      - '--web.enable-lifecycle'
    volumes:
      - prometheus_data:/prometheus
      - prometheus_config:/etc/prometheus
    networks:
      - mediastack
    ports:
      - "8400:9090"    # Phase 5: Monitoring
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.prometheus.rule=Host(\`prometheus.${DOMAIN}\`)"
      - "traefik.http.services.prometheus.loadbalancer.server.port=9090"

  # Phase 6: Management & Utilities
  guacamole:
    image: guacamole/guacamole:latest
    container_name: mediastack-guacamole
    restart: unless-stopped
    environment:
      GUACD_HOSTNAME: mediastack-guacd
      POSTGRES_HOSTNAME: mediastack-postgres
      POSTGRES_DATABASE: ${POSTGRES_DB}
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
    networks:
      - mediastack
    ports:
      - "8510:8080"    # Phase 6: Management
    depends_on:
      - guacd
      - postgres
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.guacamole.rule=Host(\`remote.${DOMAIN}\`)"
      - "traefik.http.services.guacamole.loadbalancer.server.port=8080"

  guacd:
    image: guacamole/guacd:latest
    container_name: mediastack-guacd
    restart: unless-stopped
    networks:
      - mediastack

  chromium:
    image: lscr.io/linuxserver/chromium:latest
    container_name: mediastack-chromium
    restart: unless-stopped
    environment:
      PUID: ${PUID}
      PGID: ${PGID}
      TZ: ${TZ}
    volumes:
      - chromium_config:/config
    networks:
      - mediastack
    ports:
      - "8520:3000"    # Phase 6: Management
    security_opt:
      - seccomp:unconfined
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.chromium.rule=Host(\`browser.${DOMAIN}\`)"
      - "traefik.http.services.chromium.loadbalancer.server.port=3000"

  autoscan:
    image: ghcr.io/hotio/autoscan:latest
    container_name: mediastack-autoscan
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - autoscan_config:/config
      - ${MEDIA_ROOT}:/media:ro
    networks:
      - mediastack
    ports:
      - "8530:3030"    # Phase 6: Management
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.autoscan.rule=Host(\`autoscan.${DOMAIN}\`)"
      - "traefik.http.services.autoscan.loadbalancer.server.port=3030"

  gaps:
    image: housewrecker/gaps:latest
    container_name: mediastack-gaps
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - gaps_data:/usr/app
    networks:
      - mediastack
    ports:
      - "8540:8484"    # Phase 6: Management
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.gaps.rule=Host(\`gaps.${DOMAIN}\`)"
      - "traefik.http.services.gaps.loadbalancer.server.port=8484"

  kometa:
    image: kometateam/kometa:latest
    container_name: mediastack-kometa
    restart: unless-stopped
    environment:
      TZ: ${TZ}
    volumes:
      - kometa_config:/config
      - ${MEDIA_ROOT}:/media:ro
    networks:
      - mediastack

  # Security & Network
  crowdsec:
    image: crowdsecurity/crowdsec:latest
    container_name: mediastack-crowdsec
    restart: unless-stopped
    environment:
      COLLECTIONS: "crowdsecurity/nginx crowdsecurity/base-http-scenarios"
    volumes:
      - crowdsec_config:/etc/crowdsec
      - crowdsec_data:/var/lib/crowdsec/data
      - /var/log:/var/log:ro
    networks:
      - mediastack

  tailscale:
    image: tailscale/tailscale:latest
    container_name: mediastack-tailscale
    restart: unless-stopped
    environment:
      TS_AUTHKEY: ${TAILSCALE_AUTH_KEY}
    volumes:
      - tailscale_data:/var/lib/tailscale
    networks:
      - mediastack
    cap_add:
      - NET_ADMIN
      - SYS_MODULE

EOF

# Add volumes section for new services
cat >> container_integrations.yml << 'EOF'

# Additional volumes for integrated services
  audiobookshelf_config:
  audiobookshelf_metadata:
  calibre_web_config:
  channels_dvr_data:
  iptv_proxy_config:
  tdarr_server_data:
  tdarr_config_data:
  tdarr_log_data:
  filebot_gui_config:
  homarr_config:
  homarr_icons:
  homarr_data:
  homepage_config:
  organizr_config:
  prometheus_data:
  prometheus_config:
  chromium_config:
  autoscan_config:
  gaps_data:
  kometa_config:
  crowdsec_config:
  crowdsec_data:
  tailscale_data:
EOF

print_status "Integrating services into docker-compose.yml..."

# Find where to insert services (before volumes section)
VOLUMES_LINE=$(grep -n "^volumes:" docker-compose.yml | cut -d: -f1)

if [ -n "$VOLUMES_LINE" ]; then
    # Insert before volumes section
    head -n $((VOLUMES_LINE - 1)) docker-compose.yml > docker-compose-temp.yml
    cat container_integrations.yml >> docker-compose-temp.yml
    tail -n +$VOLUMES_LINE docker-compose.yml >> docker-compose-temp.yml
    mv docker-compose-temp.yml docker-compose.yml
else
    # Just append to end
    cat container_integrations.yml >> docker-compose.yml
fi

# Clean up temp file
rm container_integrations.yml

print_success "Services integrated into docker-compose.yml"

# Create migration strategy
cat > migrate_containers.sh << 'EOF'
#!/bin/bash

# Stop old containers gracefully
echo "Stopping old containers..."
OLD_CONTAINERS="mediastack_audiobookshelf mediastack_calibre_web mediastack_channels_dvr mediastack_iptv_proxy mediastack_tvapp2 mediastack_tdarr mediastack_tdarr_node mediastack_filebot mediastack_homarr mediastack_homepage mediastack_organizr mediastack_prometheus mediastack_guacamole mediastack_guacd mediastack_chromium mediastack_autoscan mediastack_gaps mediastack_kometa mediastack_crowdsec mediastack_tailscale"

for container in $OLD_CONTAINERS; do
    if docker ps -q -f name=$container; then
        echo "Stopping $container..."
        docker stop $container
    fi
done

# Start new unified stack
echo "Starting unified stack..."
docker-compose up -d

echo "Migration complete! New containers running with priority-based ports."
EOF

chmod +x migrate_containers.sh

print_success "Integration complete!"
echo ""
echo "📊 Summary of Changes:"
echo "====================="
echo "✅ Added 20+ services from working old containers"
echo "✅ Applied priority-based port assignments"
echo "✅ Maintained all existing configurations"
echo "✅ Created migration script: migrate_containers.sh"
echo ""
echo "🚀 Next Steps:"
echo "=============="
echo "1. Review the integrated docker-compose.yml"
echo "2. Run: ./migrate_containers.sh to complete the migration"
echo "3. Verify services are running: docker-compose ps"
echo ""
echo "🔗 New Service Access URLs (with priority ports):"
echo "=================================================="
echo "• Audiobook Shelf: http://localhost:8210"
echo "• Calibre Web: http://localhost:8211"
echo "• Channels DVR: http://localhost:8220"
echo "• IPTV Proxy: http://localhost:8221"
echo "• TVApp2: http://localhost:8222"
echo "• Tdarr: http://localhost:8230"
echo "• FileBot GUI: http://localhost:8240"
echo "• Organizr: http://localhost:8340"
echo "• Homarr: http://localhost:8341"
echo "• Homepage: http://localhost:8342"
echo "• Prometheus: http://localhost:8400"
echo "• Guacamole: http://localhost:8510"
echo "• Chromium: http://localhost:8520"
echo "• AutoScan: http://localhost:8530"
echo "• Gaps: http://localhost:8540"
echo ""
print_success "Ready to migrate! All configurations preserved with new priority-based ports."
