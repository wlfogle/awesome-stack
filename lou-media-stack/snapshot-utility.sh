#!/bin/bash

# Lou Media Stack Snapshot Utility
# Creates snapshots of the entire stack state for easy rollback

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SNAPSHOTS_DIR="$SCRIPT_DIR/snapshots"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SNAPSHOT_NAME="mediastack_${TIMESTAMP}"
SNAPSHOT_PATH="$SNAPSHOTS_DIR/$SNAPSHOT_NAME"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

create_snapshot() {
    local name="$1"
    if [ -n "$name" ]; then
        SNAPSHOT_NAME="mediastack_${name}_${TIMESTAMP}"
        SNAPSHOT_PATH="$SNAPSHOTS_DIR/$SNAPSHOT_NAME"
    fi

    log "Creating snapshot: $SNAPSHOT_NAME"
    
    # Create snapshot directory
    mkdir -p "$SNAPSHOT_PATH"
    
    # Create snapshot metadata
    cat > "$SNAPSHOT_PATH/snapshot_info.json" << EOF
{
    "snapshot_name": "$SNAPSHOT_NAME",
    "timestamp": "$TIMESTAMP",
    "creation_date": "$(date -Iseconds)",
    "hostname": "$(hostname)",
    "user": "$(whoami)",
    "directory": "$SCRIPT_DIR",
    "docker_version": "$(docker --version 2>/dev/null || echo 'Not available')",
    "compose_version": "$(docker compose version 2>/dev/null || echo 'Not available')"
}
EOF

    # 1. Backup Docker Compose files
    log "Backing up Docker Compose files..."
    mkdir -p "$SNAPSHOT_PATH/compose"
    find "$SCRIPT_DIR" -name "docker-compose*.yml" -o -name "docker-compose*.yaml" | while read -r file; do
        cp "$file" "$SNAPSHOT_PATH/compose/" 2>/dev/null || warn "Could not backup $file"
    done

    # 2. Backup environment and config files
    log "Backing up configuration files..."
    mkdir -p "$SNAPSHOT_PATH/configs"
    
    # Copy common config files
    for pattern in "*.env" ".env*" "*.conf" "*.config" "*.json" "*.yml" "*.yaml" "*.toml" "*.ini"; do
        find "$SCRIPT_DIR" -maxdepth 2 -name "$pattern" -type f | while read -r file; do
            relative_path=$(realpath --relative-to="$SCRIPT_DIR" "$file")
            target_dir="$SNAPSHOT_PATH/configs/$(dirname "$relative_path")"
            mkdir -p "$target_dir"
            cp "$file" "$target_dir/" 2>/dev/null
        done
    done

    # 3. Capture current Docker state
    log "Capturing Docker container state..."
    mkdir -p "$SNAPSHOT_PATH/docker_state"
    
    # Running containers
    docker ps --format "table {{.Names}}\t{{.Image}}\t{{.Status}}\t{{.Ports}}" > "$SNAPSHOT_PATH/docker_state/running_containers.txt" 2>/dev/null
    
    # All containers (including stopped)
    docker ps -a --format "table {{.Names}}\t{{.Image}}\t{{.Status}}\t{{.CreatedAt}}" > "$SNAPSHOT_PATH/docker_state/all_containers.txt" 2>/dev/null
    
    # Container inspect data for mediastack containers
    docker ps -a --filter "name=mediastack" --format "{{.Names}}" | while read -r container; do
        if [ -n "$container" ]; then
            docker inspect "$container" > "$SNAPSHOT_PATH/docker_state/${container}_inspect.json" 2>/dev/null
        fi
    done

    # 4. Backup Docker volumes (metadata only)
    log "Capturing Docker volumes information..."
    mkdir -p "$SNAPSHOT_PATH/volumes"
    docker volume ls --format "table {{.Name}}\t{{.Driver}}\t{{.Scope}}" > "$SNAPSHOT_PATH/volumes/volumes_list.txt" 2>/dev/null
    
    # Volume inspect data for mediastack volumes
    docker volume ls --filter "name=mediastack" --format "{{.Name}}" | while read -r volume; do
        if [ -n "$volume" ]; then
            docker volume inspect "$volume" > "$SNAPSHOT_PATH/volumes/${volume}_inspect.json" 2>/dev/null
        fi
    done

    # 5. Backup networks
    log "Capturing Docker networks..."
    mkdir -p "$SNAPSHOT_PATH/networks"
    docker network ls --format "table {{.Name}}\t{{.Driver}}\t{{.Scope}}" > "$SNAPSHOT_PATH/networks/networks_list.txt" 2>/dev/null
    
    # Network inspect data for mediastack networks
    docker network ls --filter "name=mediastack" --format "{{.Name}}" | while read -r network; do
        if [ -n "$network" ]; then
            docker network inspect "$network" > "$SNAPSHOT_PATH/networks/${network}_inspect.json" 2>/dev/null
        fi
    done

    # 6. Backup critical volume data (selective)
    log "Backing up critical configuration volumes..."
    mkdir -p "$SNAPSHOT_PATH/volume_backups"
    
    # Create a script to backup specific volume contents
    cat > "$SNAPSHOT_PATH/backup_volumes.sh" << 'VOLEOF'
#!/bin/bash
# Volume backup script - run manually for full volume backups
SNAPSHOT_PATH="$1"

# Backup small config volumes (avoid large media volumes)
for volume in $(docker volume ls --filter "name=mediastack" --format "{{.Name}}"); do
    volume_info=$(docker volume inspect "$volume" 2>/dev/null)
    if [ $? -eq 0 ]; then
        # Only backup if it's likely a config volume (not media storage)
        if [[ "$volume" =~ (config|data|settings|db|database)$ ]] && [[ ! "$volume" =~ (media|movies|tv|downloads|books|music)$ ]]; then
            echo "Backing up volume: $volume"
            mkdir -p "$SNAPSHOT_PATH/volume_backups/$volume"
            docker run --rm -v "$volume:/source:ro" -v "$SNAPSHOT_PATH/volume_backups/$volume:/backup" alpine tar czf "/backup/data.tar.gz" -C /source .
        fi
    fi
done
VOLEOF
    chmod +x "$SNAPSHOT_PATH/backup_volumes.sh"

    # 7. Create restoration script
    log "Creating restoration script..."
    cat > "$SNAPSHOT_PATH/restore.sh" << RESEOF
#!/bin/bash

# Restore script for snapshot: $SNAPSHOT_NAME
# Created: $(date)

SNAPSHOT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
ORIGINAL_DIR="$SCRIPT_DIR"

echo "Restoring snapshot: $SNAPSHOT_NAME"
echo "WARNING: This will stop current containers and replace configurations!"
echo "Press Ctrl+C within 10 seconds to cancel..."
sleep 10

# Stop current mediastack containers
echo "Stopping current mediastack containers..."
docker compose -f "\$ORIGINAL_DIR/docker-compose.yml" down 2>/dev/null || true

# Backup current state before restore
if [ -d "\$ORIGINAL_DIR" ]; then
    backup_name="pre_restore_backup_\$(date +%Y%m%d_%H%M%S)"
    mkdir -p "\$ORIGINAL_DIR/snapshots/\$backup_name"
    cp "\$ORIGINAL_DIR/docker-compose.yml" "\$ORIGINAL_DIR/snapshots/\$backup_name/" 2>/dev/null || true
    cp -r "\$ORIGINAL_DIR"/*.env "\$ORIGINAL_DIR/snapshots/\$backup_name/" 2>/dev/null || true
fi

# Restore compose files
echo "Restoring Docker Compose files..."
cp "\$SNAPSHOT_DIR/compose"/* "\$ORIGINAL_DIR/" 2>/dev/null

# Restore config files
echo "Restoring configuration files..."
if [ -d "\$SNAPSHOT_DIR/configs" ]; then
    cd "\$SNAPSHOT_DIR/configs"
    find . -type f | while read -r file; do
        target_file="\$ORIGINAL_DIR/\$file"
        mkdir -p "\$(dirname "\$target_file")"
        cp "\$file" "\$target_file" 2>/dev/null
    done
    cd - > /dev/null
fi

# Restore volumes (if backup exists)
if [ -d "\$SNAPSHOT_DIR/volume_backups" ]; then
    echo "Restoring volume data..."
    for volume_backup in "\$SNAPSHOT_DIR/volume_backups"/*; do
        if [ -d "\$volume_backup" ]; then
            volume_name="\$(basename "\$volume_backup")"
            if [ -f "\$volume_backup/data.tar.gz" ]; then
                echo "Restoring volume: \$volume_name"
                docker run --rm -v "\$volume_name:/target" -v "\$volume_backup:/backup" alpine sh -c "cd /target && tar xzf /backup/data.tar.gz"
            fi
        fi
    done
fi

echo "Snapshot restored. You can now start your services with:"
echo "docker compose -f \$ORIGINAL_DIR/docker-compose.yml up -d"
RESEOF
    chmod +x "$SNAPSHOT_PATH/restore.sh"

    # 8. Create snapshot summary
    log "Creating snapshot summary..."
    cat > "$SNAPSHOT_PATH/README.md" << READEOF
# Media Stack Snapshot: $SNAPSHOT_NAME

Created: $(date)
Directory: $SCRIPT_DIR

## Contents

- **compose/**: Docker Compose files
- **configs/**: Environment and configuration files
- **docker_state/**: Container state information
- **volumes/**: Volume information and metadata
- **networks/**: Network configuration
- **volume_backups/**: Critical volume data backups
- **restore.sh**: Restoration script
- **backup_volumes.sh**: Manual volume backup script

## Restore Instructions

1. Run the restore script: \`./restore.sh\`
2. Or manually copy files and restart services

## Manual Volume Backup

To backup volume data: \`./backup_volumes.sh "$SNAPSHOT_PATH"\`

## Container State at Snapshot

\`\`\`
$(docker ps --filter "name=mediastack" --format "table {{.Names}}\t{{.Image}}\t{{.Status}}" 2>/dev/null)
\`\`\`
READEOF

    success "Snapshot created successfully: $SNAPSHOT_PATH"
    log "To restore this snapshot later, run: $SNAPSHOT_PATH/restore.sh"
    log "To backup volume data, run: $SNAPSHOT_PATH/backup_volumes.sh $SNAPSHOT_PATH"
}

list_snapshots() {
    log "Available snapshots:"
    if [ -d "$SNAPSHOTS_DIR" ]; then
        for snapshot in "$SNAPSHOTS_DIR"/mediastack_*; do
            if [ -d "$snapshot" ]; then
                snapshot_name=$(basename "$snapshot")
                if [ -f "$snapshot/snapshot_info.json" ]; then
                    creation_date=$(grep '"creation_date"' "$snapshot/snapshot_info.json" | cut -d'"' -f4)
                    echo "  $snapshot_name (Created: $creation_date)"
                else
                    echo "  $snapshot_name"
                fi
                echo "    Path: $snapshot"
                echo "    Restore: $snapshot/restore.sh"
                echo
            fi
        done
    else
        warn "No snapshots directory found"
    fi
}

restore_snapshot() {
    local snapshot_name="$1"
    if [ -z "$snapshot_name" ]; then
        error "Please specify a snapshot name"
        return 1
    fi

    local snapshot_path="$SNAPSHOTS_DIR/$snapshot_name"
    if [ ! -d "$snapshot_path" ]; then
        error "Snapshot not found: $snapshot_path"
        return 1
    fi

    if [ -f "$snapshot_path/restore.sh" ]; then
        log "Executing restore script for: $snapshot_name"
        bash "$snapshot_path/restore.sh"
    else
        error "Restore script not found in snapshot"
        return 1
    fi
}

cleanup_snapshots() {
    local keep_days="$1"
    if [ -z "$keep_days" ]; then
        keep_days=30
    fi

    log "Cleaning up snapshots older than $keep_days days..."
    find "$SNAPSHOTS_DIR" -type d -name "mediastack_*" -mtime +$keep_days | while read -r old_snapshot; do
        warn "Removing old snapshot: $(basename "$old_snapshot")"
        rm -rf "$old_snapshot"
    done
}

# Main script logic
case "$1" in
    "create")
        create_snapshot "$2"
        ;;
    "list")
        list_snapshots
        ;;
    "restore")
        restore_snapshot "$2"
        ;;
    "cleanup")
        cleanup_snapshots "$2"
        ;;
    *)
        echo "Lou Media Stack Snapshot Utility"
        echo
        echo "Usage: $0 {create|list|restore|cleanup} [options]"
        echo
        echo "Commands:"
        echo "  create [name]     Create a new snapshot (optional custom name)"
        echo "  list             List all available snapshots"
        echo "  restore <name>   Restore a specific snapshot"
        echo "  cleanup [days]   Remove snapshots older than X days (default: 30)"
        echo
        echo "Examples:"
        echo "  $0 create                    # Create snapshot with timestamp"
        echo "  $0 create before_upgrade     # Create named snapshot"
        echo "  $0 list                      # Show all snapshots"
        echo "  $0 restore mediastack_20241219_142315"
        echo "  $0 cleanup 7                 # Remove snapshots older than 7 days"
        exit 1
        ;;
esac
