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
