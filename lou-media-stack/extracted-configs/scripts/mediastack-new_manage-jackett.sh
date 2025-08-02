#!/bin/bash

# Jackett Management and Optimization Script
# This script helps manage indexers, monitor health, and optimize performance

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
JACKETT_URL="http://192.168.12.204:9117"
JACKETT_API_KEY=""
FLARESOLVERR_URL="http://192.168.12.204:8191"
CONFIG_DIR="/home/lou/mediastack-new/config/jackett"
BACKUP_DIR="/home/lou/mediastack-new/backups/jackett"

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if Jackett is running
check_jackett_status() {
    log_info "Checking Jackett status..."
    
    if curl -s -o /dev/null -w "%{http_code}" "$JACKETT_URL" | grep -q "200\|301\|302"; then
        log_success "Jackett is accessible"
        return 0
    else
        log_error "Jackett is not accessible at $JACKETT_URL"
        return 1
    fi
}

# Function to get API key from container
get_api_key() {
    log_info "Retrieving Jackett API key..."
    
    # Try to get API key from container logs
    api_key=$(docker logs mediastack_jackett 2>&1 | grep -i "api key" | tail -1 | grep -oE '[a-zA-Z0-9]{32}' || echo "")
    
    if [[ -z "$api_key" ]]; then
        # Try to get from config file
        if [[ -f "/mnt/sda1/config/appdata/jackett/config/ServerConfig.json" ]]; then
            api_key=$(grep -o '"APIKey":"[^"]*' /mnt/sda1/config/appdata/jackett/config/ServerConfig.json | cut -d'"' -f4)
        fi
    fi
    
    if [[ -n "$api_key" ]]; then
        JACKETT_API_KEY="$api_key"
        log_success "API key retrieved: ${api_key:0:8}...${api_key: -8}"
        return 0
    else
        log_warning "Could not retrieve API key automatically"
        log_info "Please check the Jackett web interface for the API key"
        return 1
    fi
}

# Function to test API connectivity
test_api_connectivity() {
    if [[ -z "$JACKETT_API_KEY" ]]; then
        log_error "No API key available. Please set JACKETT_API_KEY"
        return 1
    fi
    
    log_info "Testing API connectivity..."
    
    response=$(curl -s -o /dev/null -w "%{http_code}" \
        -H "X-Api-Key: $JACKETT_API_KEY" \
        "$JACKETT_URL/api/v2.0/server/config")
    
    if [[ "$response" == "200" ]]; then
        log_success "API connectivity successful"
        return 0
    else
        log_error "API connectivity failed (HTTP $response)"
        return 1
    fi
}

# Function to list all indexers
list_indexers() {
    log_info "Listing all configured indexers..."
    
    if [[ -z "$JACKETT_API_KEY" ]]; then
        log_error "API key required for indexer listing"
        return 1
    fi
    
    response=$(curl -s -H "X-Api-Key: $JACKETT_API_KEY" \
        "$JACKETT_URL/api/v2.0/indexers")
    
    if [[ $? -eq 0 ]]; then
        echo "$response" | jq -r '.[] | "\(.id): \(.name) - \(.description) (\(.language))"' 2>/dev/null || echo "$response"
        log_success "Indexer list retrieved"
    else
        log_error "Failed to retrieve indexer list"
        return 1
    fi
}

# Function to check indexer health
check_indexer_health() {
    local indexer_id="$1"
    
    if [[ -z "$indexer_id" ]]; then
        log_error "Indexer ID required for health check"
        return 1
    fi
    
    log_info "Checking health of indexer: $indexer_id"
    
    # Test search functionality
    response=$(curl -s -w "%{http_code}" \
        -H "X-Api-Key: $JACKETT_API_KEY" \
        "$JACKETT_URL/api/v2.0/indexers/$indexer_id/results/torznab/api?t=search&q=test")
    
    http_code="${response: -3}"
    
    if [[ "$http_code" == "200" ]]; then
        log_success "Indexer $indexer_id is healthy"
        return 0
    else
        log_warning "Indexer $indexer_id may have issues (HTTP $http_code)"
        return 1
    fi
}

# Function to test all indexers
test_all_indexers() {
    log_info "Testing all configured indexers..."
    
    if [[ -z "$JACKETT_API_KEY" ]]; then
        log_error "API key required for indexer testing"
        return 1
    fi
    
    # Get list of indexers
    indexers=$(curl -s -H "X-Api-Key: $JACKETT_API_KEY" \
        "$JACKETT_URL/api/v2.0/indexers" | jq -r '.[].id' 2>/dev/null)
    
    if [[ -z "$indexers" ]]; then
        log_error "No indexers found or API error"
        return 1
    fi
    
    local healthy=0
    local unhealthy=0
    
    for indexer in $indexers; do
        if check_indexer_health "$indexer"; then
            ((healthy++))
        else
            ((unhealthy++))
        fi
    done
    
    log_info "Health check summary:"
    log_success "Healthy indexers: $healthy"
    if [[ $unhealthy -gt 0 ]]; then
        log_warning "Unhealthy indexers: $unhealthy"
    fi
}

# Function to check FlareSolverr status
check_flaresolverr_status() {
    log_info "Checking FlareSolverr status..."
    
    response=$(curl -s -X POST -H "Content-Type: application/json" \
        -d '{"cmd": "sessions.list"}' \
        "$FLARESOLVERR_URL/v1" 2>/dev/null)
    
    if [[ $? -eq 0 ]] && echo "$response" | grep -q '"status":"ok"'; then
        log_success "FlareSolverr is working properly"
        return 0
    else
        log_error "FlareSolverr is not responding correctly"
        return 1
    fi
}

# Function to backup configuration
backup_configuration() {
    log_info "Backing up Jackett configuration..."
    
    mkdir -p "$BACKUP_DIR"
    
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    
    # Backup configuration directory
    if [[ -d "/mnt/sda1/config/appdata/jackett" ]]; then
        tar -czf "$BACKUP_DIR/jackett_config_$TIMESTAMP.tar.gz" \
            -C "/mnt/sda1/config/appdata" jackett/
        log_success "Configuration backed up to: $BACKUP_DIR/jackett_config_$TIMESTAMP.tar.gz"
    else
        log_warning "Configuration directory not found for backup"
    fi
    
    # Export indexer list via API
    if [[ -n "$JACKETT_API_KEY" ]]; then
        curl -s -H "X-Api-Key: $JACKETT_API_KEY" \
            "$JACKETT_URL/api/v2.0/indexers" > "$BACKUP_DIR/indexers_$TIMESTAMP.json"
        log_success "Indexer list exported to: $BACKUP_DIR/indexers_$TIMESTAMP.json"
    fi
    
    # Cleanup old backups (keep last 30 days)
    find "$BACKUP_DIR" -name "jackett_*" -type f -mtime +30 -delete
    find "$BACKUP_DIR" -name "indexers_*" -type f -mtime +30 -delete
    
    log_success "Backup completed"
}

# Function to optimize performance
optimize_performance() {
    log_info "Optimizing Jackett performance..."
    
    # Check container resource usage
    cpu_usage=$(docker stats mediastack_jackett --no-stream --format "table {{.CPUPerc}}" | tail -1 | sed 's/%//')
    memory_usage=$(docker stats mediastack_jackett --no-stream --format "table {{.MemUsage}}" | tail -1)
    
    log_info "Current resource usage:"
    log_info "CPU: ${cpu_usage}%"
    log_info "Memory: $memory_usage"
    
    # Restart container if resource usage is high
    if (( $(echo "$cpu_usage > 80" | bc -l 2>/dev/null || echo "0") )); then
        log_warning "High CPU usage detected, consider restarting container"
    fi
    
    # Clear cache directory
    docker exec mediastack_jackett rm -rf /tmp/jackett-cache/* 2>/dev/null || true
    log_success "Cache cleared"
    
    # Check log file size
    log_size=$(docker exec mediastack_jackett du -sh /app/logs 2>/dev/null | cut -f1 || echo "unknown")
    log_info "Log directory size: $log_size"
    
    log_success "Performance optimization completed"
}

# Function to generate monitoring report
generate_monitoring_report() {
    log_info "Generating monitoring report..."
    
    REPORT_FILE="$BACKUP_DIR/jackett_report_$(date +%Y%m%d_%H%M%S).txt"
    
    cat > "$REPORT_FILE" << EOF
Jackett Monitoring Report
Generated: $(date)

=== SERVICE STATUS ===
Jackett URL: $JACKETT_URL
Service Status: $(curl -s -o /dev/null -w "%{http_code}" "$JACKETT_URL")
FlareSolverr Status: $(curl -s -o /dev/null -w "%{http_code}" "$FLARESOLVERR_URL")

=== CONTAINER STATS ===
$(docker stats mediastack_jackett --no-stream)

=== INDEXER COUNT ===
Total Indexers: $(curl -s -H "X-Api-Key: $JACKETT_API_KEY" "$JACKETT_URL/api/v2.0/indexers" 2>/dev/null | jq length 2>/dev/null || echo "N/A")

=== RECENT LOGS ===
$(docker logs mediastack_jackett --tail 20)

=== DISK USAGE ===
Config Directory: $(du -sh /mnt/sda1/config/appdata/jackett 2>/dev/null || echo "N/A")
Backup Directory: $(du -sh $BACKUP_DIR 2>/dev/null || echo "N/A")

EOF
    
    log_success "Monitoring report generated: $REPORT_FILE"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo
    echo "Commands:"
    echo "  status          - Check Jackett and FlareSolverr status"
    echo "  list-indexers   - List all configured indexers"
    echo "  test-indexers   - Test all indexers for health"
    echo "  backup          - Backup configuration and indexer list"
    echo "  optimize        - Optimize performance and clear cache"
    echo "  report          - Generate monitoring report"
    echo "  api-key         - Retrieve and display API key"
    echo
    echo "Options:"
    echo "  --api-key KEY   - Set API key manually"
    echo "  --help          - Show this help message"
    echo
    echo "Examples:"
    echo "  $0 status"
    echo "  $0 test-indexers"
    echo "  $0 backup"
    echo "  $0 --api-key YOUR_KEY test-indexers"
}

# Main execution
main() {
    local command="$1"
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --api-key)
                JACKETT_API_KEY="$2"
                shift 2
                ;;
            --help)
                show_usage
                exit 0
                ;;
            *)
                command="$1"
                shift
                ;;
        esac
    done
    
    # Check if Jackett is running
    if ! check_jackett_status; then
        log_error "Jackett is not running or not accessible"
        exit 1
    fi
    
    # Get API key if not provided
    if [[ -z "$JACKETT_API_KEY" ]]; then
        get_api_key
    fi
    
    # Execute command
    case "$command" in
        status)
            check_jackett_status
            check_flaresolverr_status
            if [[ -n "$JACKETT_API_KEY" ]]; then
                test_api_connectivity
            fi
            ;;
        list-indexers)
            list_indexers
            ;;
        test-indexers)
            test_all_indexers
            ;;
        backup)
            backup_configuration
            ;;
        optimize)
            optimize_performance
            ;;
        report)
            generate_monitoring_report
            ;;
        api-key)
            if [[ -n "$JACKETT_API_KEY" ]]; then
                echo "API Key: $JACKETT_API_KEY"
            else
                log_error "Could not retrieve API key"
                exit 1
            fi
            ;;
        "")
            log_info "No command specified. Use --help for usage information."
            show_usage
            ;;
        *)
            log_error "Unknown command: $command"
            show_usage
            exit 1
            ;;
    esac
}

# Run if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
