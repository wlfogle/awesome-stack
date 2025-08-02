#!/bin/bash

# Restore script for snapshot: mediastack_20250719_183439
# Created: Sat Jul 19 06:34:40 PM EDT 2025

SNAPSHOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORIGINAL_DIR="/home/lou/lou-media-stack"

echo "Restoring snapshot: mediastack_20250719_183439"
echo "WARNING: This will stop current containers and replace configurations!"
echo "Press Ctrl+C within 10 seconds to cancel..."
sleep 10

# Stop current mediastack containers
echo "Stopping current mediastack containers..."
docker compose -f "$ORIGINAL_DIR/docker-compose.yml" down 2>/dev/null || true

# Backup current state before restore
if [ -d "$ORIGINAL_DIR" ]; then
    backup_name="pre_restore_backup_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$ORIGINAL_DIR/snapshots/$backup_name"
    cp "$ORIGINAL_DIR/docker-compose.yml" "$ORIGINAL_DIR/snapshots/$backup_name/" 2>/dev/null || true
    cp -r "$ORIGINAL_DIR"/*.env "$ORIGINAL_DIR/snapshots/$backup_name/" 2>/dev/null || true
fi

# Restore compose files
echo "Restoring Docker Compose files..."
cp "$SNAPSHOT_DIR/compose"/* "$ORIGINAL_DIR/" 2>/dev/null

# Restore config files
echo "Restoring configuration files..."
if [ -d "$SNAPSHOT_DIR/configs" ]; then
    cd "$SNAPSHOT_DIR/configs"
    find . -type f | while read -r file; do
        target_file="$ORIGINAL_DIR/$file"
        mkdir -p "$(dirname "$target_file")"
        cp "$file" "$target_file" 2>/dev/null
    done
    cd - > /dev/null
fi

# Restore volumes (if backup exists)
if [ -d "$SNAPSHOT_DIR/volume_backups" ]; then
    echo "Restoring volume data..."
    for volume_backup in "$SNAPSHOT_DIR/volume_backups"/*; do
        if [ -d "$volume_backup" ]; then
            volume_name="$(basename "$volume_backup")"
            if [ -f "$volume_backup/data.tar.gz" ]; then
                echo "Restoring volume: $volume_name"
                docker run --rm -v "$volume_name:/target" -v "$volume_backup:/backup" alpine sh -c "cd /target && tar xzf /backup/data.tar.gz"
            fi
        fi
    done
fi

echo "Snapshot restored. You can now start your services with:"
echo "docker compose -f $ORIGINAL_DIR/docker-compose.yml up -d"
