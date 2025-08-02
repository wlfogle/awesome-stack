#!/bin/bash

# PostgreSQL Automated Backup Script
# Performs daily backups of all PostgreSQL databases with rotation

set -e

POSTGRES_CONTAINER="mediastack_postgres"
POSTGRES_USER="authentik"
BACKUP_DIR="/home/lou/mediastack-new/backups/postgresql"
RETENTION_DAYS=30
DATE=$(date +%Y%m%d_%H%M%S)
LOG_FILE="/home/lou/mediastack-new/logs/postgres-backup.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

create_backup_directory() {
    mkdir -p "$BACKUP_DIR"
    if [[ ! -d "$BACKUP_DIR" ]]; then
        log_message "ERROR: Failed to create backup directory: $BACKUP_DIR"
        exit 1
    fi
}

check_postgres_health() {
    log_message "Checking PostgreSQL health..."
    
    if ! docker exec "$POSTGRES_CONTAINER" pg_isready -U "$POSTGRES_USER" &>/dev/null; then
        log_message "ERROR: PostgreSQL is not ready"
        exit 1
    fi
    
    log_message "PostgreSQL is healthy"
}

backup_database() {
    local db_name="$1"
    local backup_file="$BACKUP_DIR/${db_name}_${DATE}.sql"
    local compressed_file="$backup_file.gz"
    
    log_message "Starting backup of database: $db_name"
    
    # Create SQL dump
    if docker exec "$POSTGRES_CONTAINER" pg_dump -U "$POSTGRES_USER" -d "$db_name" > "$backup_file"; then
        # Compress the backup
        if gzip "$backup_file"; then
            log_message "SUCCESS: Database $db_name backed up to $compressed_file"
            
            # Get backup size
            local backup_size=$(du -h "$compressed_file" | cut -f1)
            log_message "Backup size: $backup_size"
            
            return 0
        else
            log_message "ERROR: Failed to compress backup for $db_name"
            rm -f "$backup_file"
            return 1
        fi
    else
        log_message "ERROR: Failed to backup database $db_name"
        rm -f "$backup_file"
        return 1
    fi
}

backup_all_databases() {
    log_message "Starting backup of all databases..."
    
    # Get list of databases
    local databases=$(docker exec "$POSTGRES_CONTAINER" psql -U "$POSTGRES_USER" -t -c "SELECT datname FROM pg_database WHERE datistemplate = false;" | grep -v "^$" | sed 's/^ *//')
    
    local backup_count=0
    local failed_count=0
    
    for db in $databases; do
        if backup_database "$db"; then
            ((backup_count++))
        else
            ((failed_count++))
        fi
    done
    
    log_message "Backup summary: $backup_count successful, $failed_count failed"
    
    if [[ $failed_count -gt 0 ]]; then
        return 1
    fi
    
    return 0
}

cleanup_old_backups() {
    log_message "Cleaning up backups older than $RETENTION_DAYS days..."
    
    local deleted_count=0
    
    # Find and delete old backups
    if [[ -d "$BACKUP_DIR" ]]; then
        while IFS= read -r -d '' file; do
            rm -f "$file"
            ((deleted_count++))
            log_message "Deleted old backup: $(basename "$file")"
        done < <(find "$BACKUP_DIR" -name "*.sql.gz" -mtime +$RETENTION_DAYS -print0)
    fi
    
    log_message "Cleanup completed: $deleted_count old backups removed"
}

create_backup_summary() {
    local summary_file="$BACKUP_DIR/backup_summary_${DATE}.txt"
    
    {
        echo "PostgreSQL Backup Summary"
        echo "========================"
        echo "Date: $(date)"
        echo "Backup Directory: $BACKUP_DIR"
        echo "Retention Period: $RETENTION_DAYS days"
        echo ""
        echo "Database Backups:"
        ls -lh "$BACKUP_DIR"/*_${DATE}.sql.gz 2>/dev/null || echo "No backups created"
        echo ""
        echo "Disk Usage:"
        du -sh "$BACKUP_DIR"
        echo ""
        echo "Available Space:"
        df -h "$BACKUP_DIR"
    } > "$summary_file"
    
    log_message "Backup summary created: $summary_file"
}

verify_backups() {
    log_message "Verifying backup integrity..."
    
    local verification_failed=0
    
    for backup in "$BACKUP_DIR"/*_${DATE}.sql.gz; do
        if [[ -f "$backup" ]]; then
            if gzip -t "$backup" &>/dev/null; then
                log_message "VERIFIED: $(basename "$backup")"
            else
                log_message "ERROR: Backup verification failed for $(basename "$backup")"
                ((verification_failed++))
            fi
        fi
    done
    
    if [[ $verification_failed -eq 0 ]]; then
        log_message "All backups verified successfully"
        return 0
    else
        log_message "ERROR: $verification_failed backups failed verification"
        return 1
    fi
}

main() {
    log_message "=== PostgreSQL Backup Script Started ==="
    
    # Ensure log directory exists
    mkdir -p "$(dirname "$LOG_FILE")"
    
    # Check if log directory is writable
    if [[ ! -w "$(dirname "$LOG_FILE")" ]]; then
        LOG_FILE="/tmp/postgres-backup.log"
        log_message "Log directory not writable, using temporary log file: $LOG_FILE"
    fi
    
    create_backup_directory
    check_postgres_health
    
    if backup_all_databases; then
        cleanup_old_backups
        verify_backups
        create_backup_summary
        log_message "=== PostgreSQL Backup Script Completed Successfully ==="
        exit 0
    else
        log_message "=== PostgreSQL Backup Script Failed ==="
        exit 1
    fi
}

# Handle script arguments
case "${1:-backup}" in
    "backup")
        main
        ;;
    "restore")
        if [[ -z "$2" ]]; then
            echo "Usage: $0 restore <backup_file> [database_name]"
            exit 1
        fi
        
        backup_file="$2"
        target_db="${3:-authentik}"
        
        if [[ -f "$backup_file" ]]; then
            log_message "Restoring $backup_file to database $target_db"
            
            # Extract if compressed
            if [[ "$backup_file" == *.gz ]]; then
                temp_file="/tmp/$(basename "$backup_file" .gz)"
                gunzip -c "$backup_file" > "$temp_file"
                backup_file="$temp_file"
            fi
            
            # Restore database
            docker exec -i "$POSTGRES_CONTAINER" psql -U "$POSTGRES_USER" -d "$target_db" < "$backup_file"
            
            # Clean up temp file
            [[ -f "$temp_file" ]] && rm -f "$temp_file"
            
            log_message "Database restore completed"
        else
            log_message "ERROR: Backup file not found: $backup_file"
            exit 1
        fi
        ;;
    "list")
        echo "Available backups:"
        ls -lh "$BACKUP_DIR"/*.sql.gz 2>/dev/null || echo "No backups found"
        ;;
    *)
        echo "Usage: $0 [backup|restore|list]"
        echo "  backup  - Create backup of all databases (default)"
        echo "  restore - Restore from backup file"
        echo "  list    - List available backups"
        exit 1
        ;;
esac
