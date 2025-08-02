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
