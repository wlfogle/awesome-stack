#!/bin/bash

# Fix script for auxiliary services with restart issues
# Addresses permissions, missing configs, and startup problems

DOCKER_COMPOSE_FILE="/home/lou/lou-media-stack/docker-compose.yml"
DATA_DIR="/home/lou/lou-media-stack/data"

echo "=== Media Stack Auxiliary Services Fix Script ==="
echo "Fixing permission, configuration, and startup issues..."

# Create necessary directories with proper permissions
echo "Creating missing directories..."
mkdir -p "${DATA_DIR}/filebot/config"
mkdir -p "${DATA_DIR}/headscale/config"
mkdir -p "${DATA_DIR}/headplane/config"
mkdir -p "${DATA_DIR}/ddns-updater"
mkdir -p "${DATA_DIR}/janitorr"
mkdir -p "${DATA_DIR}/epgstation/config"
mkdir -p "${DATA_DIR}/epgstation/data"
mkdir -p "${DATA_DIR}/ai-services/recommendation-engine"
mkdir -p "${DATA_DIR}/ai-services/storage-optimizer"
mkdir -p "${DATA_DIR}/flexget/config"

# Set proper ownership (assuming user ID 1000)
echo "Setting proper ownership..."
sudo chown -R 1000:1000 "${DATA_DIR}"

# Fix HeadScale configuration
echo "Setting up HeadScale configuration..."
if [ ! -f "${DATA_DIR}/headscale/config/config.yaml" ]; then
cat > "${DATA_DIR}/headscale/config/config.yaml" << 'EOF'
server_url: https://headscale.lou-media-stack.local
listen_addr: 0.0.0.0:8080
metrics_listen_addr: 127.0.0.1:9090
grpc_listen_addr: 0.0.0.0:50443
grpc_allow_insecure: false
private_key_path: /etc/headscale/private.key
noise:
  private_key_path: /etc/headscale/noise_private.key
prefixes:
  v4: 100.64.0.0/10
  v6: fd7a:115c:a1e0::/48
database:
  type: sqlite3
  sqlite3:
    path: /etc/headscale/db.sqlite
derp:
  server:
    enabled: false
  urls:
    - https://controlplane.tailscale.com/derpmap/default
  auto_update_enabled: true
  update_frequency: 24h
log:
  level: info
acme:
  url: https://acme-v02.api.letsencrypt.org/directory
  email: ""
  tls_letsencrypt_hostname: ""
  tls_letsencrypt_cache_dir: /var/lib/headscale/cache
  tls_letsencrypt_challenge_type: HTTP-01
  tls_letsencrypt_listen: ":http"
  tls_cert_path: ""
  tls_key_path: ""
unix_socket: /var/run/headscale/headscale.sock
unix_socket_permission: "0770"
logtail:
  enabled: false
randomize_client_port: false
EOF
fi

# Generate HeadScale private keys if they don't exist
if [ ! -f "${DATA_DIR}/headscale/private.key" ]; then
    echo "Generating HeadScale private key..."
    docker run --rm -v "${DATA_DIR}/headscale:/etc/headscale" headscale/headscale:latest headscale generate private-key
fi

# Fix FileBot configuration
echo "Setting up FileBot configuration..."
if [ ! -f "${DATA_DIR}/filebot/config/filebot.conf" ]; then
cat > "${DATA_DIR}/filebot/config/filebot.conf" << 'EOF'
# FileBot Configuration
-Dfilebot.license=/config/filebot.psm
-Djava.net.useSystemProxies=false
-Dfile.encoding=UTF-8
-Dsun.jnu.encoding=UTF-8
EOF
fi

# Fix DDNS Updater configuration
echo "Setting up DDNS Updater configuration..."
if [ ! -f "${DATA_DIR}/ddns-updater/config.json" ]; then
cat > "${DATA_DIR}/ddns-updater/config.json" << 'EOF'
{
  "settings": [
    {
      "provider": "cloudflare",
      "domain": "lou-media-stack.local",
      "host": "@",
      "ttl": 600
    }
  ]
}
EOF
fi

# Fix Janitorr configuration
echo "Setting up Janitorr configuration..."
if [ ! -f "${DATA_DIR}/janitorr/config.yaml" ]; then
cat > "${DATA_DIR}/janitorr/config.yaml" << 'EOF'
# Janitorr Configuration
general:
  log_level: INFO
  dry_run: true
  
services:
  sonarr:
    enabled: true
    url: http://sonarr:8989
    api_key: ""
  radarr:
    enabled: true
    url: http://radarr:7878
    api_key: ""
    
cleanup:
  free_space_threshold: 10
  max_file_age_days: 90
EOF
fi

# Fix EPGStation configuration
echo "Setting up EPGStation configuration..."
if [ ! -f "${DATA_DIR}/epgstation/config/config.yml" ]; then
cat > "${DATA_DIR}/epgstation/config/config.yml" << 'EOF'
port: 8888
mirakurunPath: http://mirakurun:40772/
dbtype: sqlite3
sqlite:
  extensions:
    - '/usr/local/lib/spatialite'
  db: '/app/data/epgstation.db'
epgUpdateIntervalTime: 10
conflictPriority: 1
recPriority: 2
streamFilePath: /app/streamfiles
recordedFormat: '%YEAR%年%MONTH%月%DAY%日%HOUR%時%MIN%分%SEC%秒-%TITLE%'
recordedFileExtension: .m2ts
recorded:
  - name: recorded
    path: /app/recorded
thumbnails:
  - name: thumbnail
    path: /app/thumbnail
EOF
fi

# Fix FlexGet configuration
echo "Setting up FlexGet configuration..."
if [ ! -f "${DATA_DIR}/flexget/config/config.yml" ]; then
cat > "${DATA_DIR}/flexget/config/config.yml" << 'EOF'
web_server:
  bind: 0.0.0.0
  port: 5050

schedules:
  - tasks: '*'
    interval:
      minutes: 30

tasks:
  sample_task:
    rss: https://example.com/feed.xml
    accept_all: yes
    download: /downloads
EOF
fi

# Stop problematic services
echo "Stopping problematic services..."
docker-compose -f "$DOCKER_COMPOSE_FILE" stop \
  filebot \
  headscale \
  headplane \
  ddns-updater \
  janitorr \
  epgstation \
  ai-recommendation-engine \
  ai-storage-optimizer \
  flexget

# Remove containers to force rebuild
echo "Removing containers to force clean start..."
docker-compose -f "$DOCKER_COMPOSE_FILE" rm -f \
  filebot \
  headscale \
  headplane \
  ddns-updater \
  janitorr \
  epgstation \
  ai-recommendation-engine \
  ai-storage-optimizer \
  flexget

# Pull latest images
echo "Pulling latest images..."
docker-compose -f "$DOCKER_COMPOSE_FILE" pull \
  filebot \
  headscale \
  headplane \
  ddns-updater \
  janitorr

# Set final permissions
echo "Setting final permissions..."
sudo chmod -R 755 "${DATA_DIR}"
sudo chown -R 1000:1000 "${DATA_DIR}"

# Start services one by one with delay
echo "Starting services with delays..."

echo "Starting HeadScale..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d headscale
sleep 10

echo "Starting HeadPlane..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d headplane
sleep 5

echo "Starting FileBot..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d filebot
sleep 5

echo "Starting DDNS Updater..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d ddns-updater
sleep 5

echo "Starting Janitorr..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d janitorr
sleep 5

echo "Starting FlexGet..."
docker-compose -f "$DOCKER_COMPOSE_FILE" up -d flexget
sleep 5

echo "=== Fix script completed ==="
echo "Checking service status..."
docker ps --format "table {{.Names}}\t{{.Status}}" | grep -E "(filebot|headscale|headplane|ddns|janitorr|flexget)"

echo ""
echo "To check logs for specific services, use:"
echo "docker-compose logs -f <service_name>"
echo ""
echo "Services that may need manual configuration:"
echo "- FileBot: May need license file"
echo "- DDNS Updater: Needs API credentials"
echo "- Janitorr: Needs Sonarr/Radarr API keys"
