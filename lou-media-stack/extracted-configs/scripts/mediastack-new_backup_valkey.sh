#!/bin/bash

# Valkey Backup Script
# Automated backup of Valkey data volume with timestamped tarballs and cleanup

# Configuration
BACKUP_DIR="/run/media/lou/StorageOne/config/valkey"
CONTAINER_NAME="valkey"
VOLUME_NAME="valkey_data"
RETENTION_DAYS=7
LOG_FILE="/home/lou/valkey_backup.log"

# Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

# Function to log messages
log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Function to check if container is running
check_container() {
    if ! docker ps --format "table {{.Names}}" | grep -q "^$CONTAINER_NAME$"; then
        log_message "ERROR: Container $CONTAINER_NAME is not running"
        exit 1
    fi
}

# Function to create backup
create_backup() {
    local timestamp=$(date '+%Y%m%d_%H%M%S')
    local backup_file="$BACKUP_DIR/valkey_backup_$timestamp.tar.gz"
    
    log_message "Starting backup of Valkey data volume"
    
    # Create backup using docker run with volume mount
    if docker run --rm \
        -v "$VOLUME_NAME":/data:ro \
        -v "$BACKUP_DIR":/backup \
        alpine:latest \
        tar -czf "/backup/valkey_backup_$timestamp.tar.gz" -C /data .; then
        
        log_message "Backup created successfully: $backup_file"
        
        # Verify backup file exists and has content
        if [ -f "$backup_file" ] && [ -s "$backup_file" ]; then
            local file_size=$(du -h "$backup_file" | cut -f1)
            log_message "Backup file size: $file_size"
            return 0
        else
            log_message "ERROR: Backup file is empty or missing"
            return 1
        fi
    else
        log_message "ERROR: Failed to create backup"
        return 1
    fi
}

# Function to clean up old backups
cleanup_old_backups() {
    log_message "Cleaning up backups older than $RETENTION_DAYS days"
    
    local deleted_count=0
    while IFS= read -r -d '' file; do
        rm "$file"
        log_message "Deleted old backup: $(basename "$file")"
        ((deleted_count++))
    done < <(find "$BACKUP_DIR" -name "valkey_backup_*.tar.gz" -type f -mtime +$RETENTION_DAYS -print0)
    
    if [ $deleted_count -eq 0 ]; then
        log_message "No old backups to clean up"
    else
        log_message "Cleaned up $deleted_count old backup(s)"
    fi
}

# Function to show backup status
show_backup_status() {
    log_message "Current backup status:"
    
    local backup_count=$(find "$BACKUP_DIR" -name "valkey_backup_*.tar.gz" -type f | wc -l)
    local total_size=$(du -sh "$BACKUP_DIR" 2>/dev/null | cut -f1)
    
    log_message "Total backups: $backup_count"
    log_message "Total backup size: $total_size"
    
    if [ $backup_count -gt 0 ]; then
        log_message "Recent backups:"
        find "$BACKUP_DIR" -name "valkey_backup_*.tar.gz" -type f -printf "%TY-%Tm-%Td %TH:%TM - %f (%s bytes)\n" | sort -r | head -5 | while read line; do
            log_message "  $line"
        done
    fi
}

# Main execution
main() {
    log_message "=== Valkey Backup Script Started ==="
    
    # Check if container is running
    check_container
    
    # Create backup
    if create_backup; then
        log_message "Backup completed successfully"
    else
        log_message "Backup failed"
        exit 1
    fi
    
    # Clean up old backups
    cleanup_old_backups
    
    # Show backup status
    show_backup_status
    
    log_message "=== Valkey Backup Script Completed ==="
}

# Execute main function
main "$@"
