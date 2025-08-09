#!/bin/bash

# Container 231: Jellyfin Media Server
# Base: Ubuntu Server 24.04 LTS
# Purpose: Open source media streaming server

echo "Creating Container 231: Jellyfin Media Server on Proxmox..."

# SSH into Proxmox and create container
ssh root@192.168.122.9 << 'EOF'

# Create container with Ubuntu template
pct create 231 /var/lib/vz/template/cache/ubuntu-24.04-standard_24.04-2_amd64.tar.zst \
  --hostname jellyfin \
  --memory 4096 \
  --cores 2 \
  --net0 name=eth0,bridge=vmbr0,ip=192.168.122.231/24,gw=192.168.122.1 \
  --storage local-lvm \
  --rootfs local-lvm:16 \
  --unprivileged 1 \
  --features nesting=1

# Add bind mounts for media and config
pct set 231 --mp0 /mnt/data/media/movies,mp=/media/movies,ro=1
pct set 231 --mp1 /mnt/data/media/tv,mp=/media/tv,ro=1
pct set 231 --mp2 /mnt/data/media/music,mp=/media/music,ro=1
pct set 231 --mp3 /mnt/data/configs/jellyfin,mp=/config

# Start container
pct start 231

# Wait for container to boot
sleep 15

# Install Jellyfin Media Server
pct exec 231 -- bash -c '
# Update system
apt update && apt upgrade -y

# Install dependencies
apt install -y curl wget gnupg2 software-properties-common ca-certificates

# Add Jellyfin repository
curl -fsSL https://repo.jellyfin.org/jellyfin_team.gpg.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/jellyfin.gpg
echo "deb [arch=$( dpkg --print-architecture )] https://repo.jellyfin.org/ubuntu $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/jellyfin.list

# Install Jellyfin
apt update
apt install -y jellyfin

# Create necessary directories
mkdir -p /config /media/{movies,tv,music}

# Configure Jellyfin directories
systemctl stop jellyfin
sed -i "s|JELLYFIN_DATA_DIRECTORY=.*|JELLYFIN_DATA_DIRECTORY=\"/config\"|" /etc/default/jellyfin
sed -i "s|JELLYFIN_CONFIG_DIRECTORY=.*|JELLYFIN_CONFIG_DIRECTORY=\"/config/config\"|" /etc/default/jellyfin
sed -i "s|JELLYFIN_LOG_DIRECTORY=.*|JELLYFIN_LOG_DIRECTORY=\"/config/log\"|" /etc/default/jellyfin
sed -i "s|JELLYFIN_CACHE_DIRECTORY=.*|JELLYFIN_CACHE_DIRECTORY=\"/config/cache\"|" /etc/default/jellyfin

# Set proper permissions
chown -R jellyfin:jellyfin /config
mkdir -p /config/{config,log,cache}
chown -R jellyfin:jellyfin /config

# Install additional packages for hardware transcoding
apt install -y ffmpeg intel-media-va-driver-non-free

# Enable and start Jellyfin
systemctl enable jellyfin
systemctl start jellyfin

# Configure basic firewall
ufw --force enable
ufw allow 8096/tcp
ufw allow 8920/tcp
ufw allow 7359/udp
ufw allow 1900/udp

echo "Jellyfin Media Server installation completed"
echo "Service status:"
systemctl status jellyfin --no-pager -l
'

echo "Container 231 (Jellyfin) setup completed!"
echo "Access: http://192.168.122.231:8096"

EOF

echo "Container 231 (Jellyfin) created successfully on Proxmox!"
