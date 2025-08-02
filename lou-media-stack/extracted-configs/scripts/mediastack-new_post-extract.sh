#!/bin/bash

# Post-Extraction Script for Unpackerr
# Handles permissions, cleanup, and notifications after extraction

set -e

# Configuration
SCRIPT_NAME="post-extract"
LOG_FILE="/var/log/unpackerr/post-extract.log"
EXTRACT_PATH="/downloads/extracted"
COMPLETED_PATH="/downloads/completed"
MEDIA_PATH="/mnt/sda1/media"
PUID=${PUID:-1000}
PGID=${PGID:-1000}

# Logging function
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') [$SCRIPT_NAME] $1" | tee -a "$LOG_FILE"
}

# Error handling
handle_error() {
    log "ERROR: $1"
    exit 1
}

# Main execution
main() {
    log "Starting post-extraction processing"
    
    # Get arguments from Unpackerr
    EXTRACTED_PATH="$1"
    ORIGINAL_PATH="$2"
    
    if [[ -z "$EXTRACTED_PATH" ]]; then
        handle_error "No extraction path provided"
    fi
    
    log "Processing extracted files at: $EXTRACTED_PATH"
    
    # Fix permissions
    log "Fixing permissions for extracted files"
    if [[ -d "$EXTRACTED_PATH" ]]; then
        chown -R "$PUID:$PGID" "$EXTRACTED_PATH" || log "WARNING: Failed to change ownership"
        find "$EXTRACTED_PATH" -type f -exec chmod 644 {} \; || log "WARNING: Failed to set file permissions"
        find "$EXTRACTED_PATH" -type d -exec chmod 755 {} \; || log "WARNING: Failed to set directory permissions"
    fi
    
    # Remove empty directories
    log "Cleaning up empty directories"
    find "$EXTRACTED_PATH" -type d -empty -delete 2>/dev/null || true
    
    # Remove unwanted files
    log "Removing unwanted files"
    find "$EXTRACTED_PATH" -type f \( -name "*.nfo" -o -name "*.txt" -o -name "*.url" -o -name "*.jpg" -o -name "*.jpeg" -o -name "*.png" -o -name "*.gif" -o -name "*.exe" -o -name "*.msi" -o -name "*.bat" -o -name "*.cmd" \) -delete 2>/dev/null || true
    
    # Remove sample files
    log "Removing sample files"
    find "$EXTRACTED_PATH" -type f -name "*sample*" -delete 2>/dev/null || true
    find "$EXTRACTED_PATH" -type f -name "*SAMPLE*" -delete 2>/dev/null || true
    
    # Remove subtitle files if not wanted
    if [[ "$REMOVE_SUBTITLES" == "true" ]]; then
        log "Removing subtitle files"
        find "$EXTRACTED_PATH" -type f \( -name "*.srt" -o -name "*.sub" -o -name "*.idx" -o -name "*.ass" -o -name "*.ssa" -o -name "*.vtt" \) -delete 2>/dev/null || true
    fi
    
    # Send notification
    if command -v curl &> /dev/null && [[ -n "$WEBHOOK_URL" ]]; then
        log "Sending notification webhook"
        curl -X POST "$WEBHOOK_URL" \
            -H "Content-Type: application/json" \
            -d "{\"text\":\"Unpackerr: Successfully extracted files from $ORIGINAL_PATH\"}" \
            2>/dev/null || log "WARNING: Failed to send webhook notification"
    fi
    
    # Update file statistics
    if [[ -d "$EXTRACTED_PATH" ]]; then
        FILE_COUNT=$(find "$EXTRACTED_PATH" -type f | wc -l)
        TOTAL_SIZE=$(du -sh "$EXTRACTED_PATH" | cut -f1)
        log "Extraction complete: $FILE_COUNT files, $TOTAL_SIZE total"
    fi
    
    log "Post-extraction processing completed successfully"
}

# Create log directory if it doesn't exist
mkdir -p "$(dirname "$LOG_FILE")"

# Execute main function
main "$@"
