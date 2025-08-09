#!/bin/bash

# Container 230: Plex Media Server
# Base: Ubuntu Server 24.04 LTS
# Purpose: Media streaming server with transcoding capabilities

echo "Creating Container 230: Plex Media Server..."

# Create container with Ubuntu Server base
lxc launch ubuntu:24.04 230-plex

# Wait for container to be ready
echo "Waiting for container to be ready..."
sleep 10

# Configure container settings
lxc config set 230-plex limits.cpu 4
lxc config set 230-plex limits.memory 8GB
lxc config set 230-plex security.nesting true
lxc config set 230-plex security.privileged false

# Add storage mounts for media
lxc config device add 230-plex media-movies disk source=/mnt/data/media/movies path=/media/movies readonly=true
lxc config device add 230-plex media-tv disk source=/mnt/data/media/tv path=/media/tv readonly=true
lxc config device add 230-plex media-music disk source=/mnt/data/media/music path=/media/music readonly=true
lxc config device add 230-plex plex-config disk source=/mnt/data/configs/plex path=/config

# Set static IP
lxc network attach lxdbr0 230-plex eth0
lxc config device set 230-plex eth0 ipv4.address 192.168.122.230

# Install and configure Plex
lxc exec 230-plex -- bash -c '
apt update && apt upgrade -y

# Install dependencies
apt install -y curl wget gnupg2 software-properties-common

# Add Plex repository
curl https://downloads.plex.tv/plex-keys/PlexSign.key | gpg --dearmor | tee /usr/share/keyrings/plex-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/plex-archive-keyring.gpg] https://downloads.plex.tv/repo/deb public main" | tee /etc/apt/sources.list.d/plexmediaserver.list

# Install Plex Media Server
apt update
apt install -y plexmediaserver

# Configure Plex directories
mkdir -p /config /media/{movies,tv,music}
chown -R plex:plex /config /media

# Configure Plex to use custom config directory
systemctl stop plexmediaserver
sed -i "s|PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR=.*|PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR=/config|" /etc/default/plexmediaserver
sed -i "s|PLEX_MEDIA_SERVER_HOME=.*|PLEX_MEDIA_SERVER_HOME=/usr/lib/plexmediaserver|" /etc/default/plexmediaserver
sed -i "s|PLEX_MEDIA_SERVER_USER=.*|PLEX_MEDIA_SERVER_USER=plex|" /etc/default/plexmediaserver

# Enable and start Plex
systemctl enable plexmediaserver
systemctl start plexmediaserver

# Install additional codecs for transcoding
apt install -y ffmpeg intel-media-va-driver-non-free

# Configure firewall
ufw allow 32400/tcp
ufw allow 3005/tcp
ufw allow 8324/tcp
ufw allow 32469/tcp
ufw allow 1900/udp
ufw allow 32410:32414/udp

echo "Plex Media Server installation completed"
echo "Access Plex at: http://192.168.122.230:32400/web"
echo "Complete setup through the web interface"
'

echo "Container 230 (Plex) created successfully!"
echo "Access: http://192.168.122.230:32400/web"
