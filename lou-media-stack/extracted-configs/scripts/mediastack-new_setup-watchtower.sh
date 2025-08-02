#!/bin/bash

# Watchtower Configuration Setup Script
# This script helps automate the setup of Watchtower with enhanced features

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
WATCHTOWER_CONTAINER="mediastack_watchtower"
DOMAIN="${DOMAIN:-mediastack.local}"
CONFIG_DIR="/home/lou/mediastack-new/config/watchtower"
DATA_ROOT="/home/lou/mediastack-new/appdata"
COMPOSE_FILE="/home/lou/mediastack-new/config/docker-compose.yml"

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

# Function to check if Watchtower is running
check_watchtower_status() {
    log_info "Checking Watchtower status..."
    
    if docker ps --format "table {{.Names}}" | grep -q "$WATCHTOWER_CONTAINER"; then
        log_success "Watchtower is running"
        return 0
    else
        log_warning "Watchtower is not running"
        return 1
    fi
}

# Function to create directory structure
create_directories() {
    log_info "Creating directory structure..."
    
    directories=(
        "${DATA_ROOT}/watchtower/config"
        "${DATA_ROOT}/watchtower/logs"
        "/home/lou/mediastack-new/backups/watchtower"
    )
    
    for dir in "${directories[@]}"; do
        mkdir -p "$dir"
        log_success "Created directory: $dir"
    done
}

# Function to generate API token
generate_api_token() {
    log_info "Generating Watchtower API token..."
    
    # Generate a random token
    token=$(openssl rand -hex 32)
    
    # Save to .env file
    if grep -q "WATCHTOWER_API_TOKEN" "${CONFIG_DIR}/../.env" 2>/dev/null; then
        sed -i "s/WATCHTOWER_API_TOKEN=.*/WATCHTOWER_API_TOKEN=$token/" "${CONFIG_DIR}/../.env"
    else
        echo "WATCHTOWER_API_TOKEN=$token" >> "${CONFIG_DIR}/../.env"
    fi
    
    log_success "API token generated and saved to .env file"
    log_info "Token: $token"
}

# Function to create notification configuration
create_notification_config() {
    log_info "Creating notification configuration..."
    
    cat > "${DATA_ROOT}/watchtower/config/notifications.yml" << EOF
# Watchtower Notification Configuration

# Slack Configuration
slack:
  enabled: true
  webhook_url: \${SLACK_WEBHOOK_URL}
  channel: "#mediastack-updates"
  username: "Watchtower"
  icon_emoji: ":whale:"
  
  # Message templates
  templates:
    update_success: |
      :white_check_mark: *Container Updated Successfully*
      
      *Container:* {{.Container}}
      *From:* {{.From}}
      *To:* {{.To}}
      *Time:* {{.Time}}
      
    update_failed: |
      :x: *Container Update Failed*
      
      *Container:* {{.Container}}
      *Error:* {{.Error}}
      *Time:* {{.Time}}
      
    update_summary: |
      :bar_chart: *Daily Update Summary*
      
      *Updated:* {{.Updated}}
      *Failed:* {{.Failed}}
      *Skipped:* {{.Skipped}}
      *Date:* {{.Date}}

# Discord Configuration
discord:
  enabled: false
  webhook_url: \${DISCORD_WEBHOOK_URL}
  username: "Watchtower"
  
# Email Configuration
email:
  enabled: false
  smtp_server: \${SMTP_SERVER}
  smtp_port: 587
  username: \${SMTP_USERNAME}
  password: \${SMTP_PASSWORD}
  from: "watchtower@${DOMAIN}"
  to: ["admin@${DOMAIN}"]
  
# Gotify Configuration
gotify:
  enabled: false
  server: "https://gotify.${DOMAIN}"
  token: \${GOTIFY_TOKEN}
EOF
    
    log_success "Notification configuration created"
}

# Function to create update policies
create_update_policies() {
    log_info "Creating update policies..."
    
    cat > "${DATA_ROOT}/watchtower/config/update-policies.yml" << EOF
# Update Policies Configuration

# Service Categories
categories:
  critical:
    description: "Critical infrastructure - manual updates only"
    auto_update: false
    monitor_only: true
    services:
      - mediastack_postgres
      - mediastack_valkey
      - mediastack_gluetun
      - mediastack_authentik_server
      - mediastack_authentik_worker
      - mediastack_traefik
      - mediastack_portainer
      
  high_priority:
    description: "Arr services - cautious auto-update"
    auto_update: true
    stop_timeout: 30
    dependencies: ["mediastack_postgres"]
    services:
      - mediastack_sonarr
      - mediastack_radarr
      - mediastack_lidarr
      
  medium_priority:
    description: "Media services - auto-update allowed"
    auto_update: true
    stop_timeout: 60
    services:
      - mediastack_jellyfin
      - mediastack_plex
      - mediastack_deluge
      - mediastack_jackett
      
  low_priority:
    description: "Monitoring and utilities - safe auto-update"
    auto_update: true
    stop_timeout: 30
    services:
      - mediastack_prometheus
      - mediastack_grafana
      - mediastack_heimdall
      - mediastack_homarr
      - mediastack_homepage
      - mediastack_tautulli
      - mediastack_flaresolverr
      - mediastack_unpackerr
      - mediastack_watchtower

# Update Schedule
schedule:
  maintenance_window:
    start: "02:00"
    end: "04:00"
    timezone: "America/New_York"
    
  update_frequency:
    critical: "manual"
    high_priority: "daily"
    medium_priority: "daily"
    low_priority: "daily"
    
  cron_schedule: "0 0 2 * * *"  # Daily at 2 AM
  
# Rollback Configuration
rollback:
  enabled: true
  max_rollback_attempts: 3
  rollback_timeout: 300
  
# Health Checks
health_checks:
  enabled: true
  timeout: 30
  retries: 3
  interval: 10
EOF
    
    log_success "Update policies created"
}

# Function to create monitoring configuration
create_monitoring_config() {
    log_info "Creating monitoring configuration..."
    
    cat > "${DATA_ROOT}/watchtower/config/monitoring.yml" << EOF
# Monitoring Configuration

# Prometheus Metrics
prometheus:
  enabled: true
  port: 8080
  path: "/v1/metrics"
  
  # Custom metrics
  custom_metrics:
    - name: "watchtower_update_duration"
      type: "histogram"
      help: "Time taken to update containers"
      buckets: [1, 5, 10, 30, 60, 300, 600]
      
    - name: "watchtower_container_status"
      type: "gauge"
      help: "Container status (0=stopped, 1=running, 2=updating)"
      
    - name: "watchtower_last_check"
      type: "gauge"
      help: "Timestamp of last update check"

# Grafana Dashboard
grafana:
  dashboard_id: "watchtower-updates"
  refresh_interval: "5m"
  
  panels:
    - title: "Updates Today"
      type: "stat"
      span: 3
      
    - title: "Success Rate"
      type: "stat"
      span: 3
      
    - title: "Update Timeline"
      type: "graph"
      span: 6
      
    - title: "Container Status"
      type: "table"
      span: 6
      
    - title: "Failed Updates"
      type: "logs"
      span: 6

# Alerting Rules
alerts:
  - name: "WatchtowerUpdateFailed"
    condition: "rate(watchtower_containers_failed_total[1h]) > 0"
    duration: "5m"
    severity: "warning"
    
  - name: "WatchtowerDown"
    condition: "up{job=\"watchtower\"} == 0"
    duration: "5m"
    severity: "critical"
    
  - name: "WatchtowerLongUpdate"
    condition: "histogram_quantile(0.95, rate(watchtower_update_duration_seconds_bucket[5m])) > 300"
    duration: "10m"
    severity: "warning"

# Log Configuration
logging:
  level: "info"
  format: "json"
  output: "/var/log/watchtower/watchtower.log"
  
  # Log rotation
  rotation:
    max_size: "100MB"
    max_age: "7d"
    max_backups: 10
    compress: true
EOF
    
    log_success "Monitoring configuration created"
}

# Function to create backup script
create_backup_script() {
    log_info "Creating backup script..."
    
    cat > "${DATA_ROOT}/watchtower/backup-watchtower.sh" << 'EOF'
#!/bin/bash
# Watchtower Backup Script

BACKUP_DIR="/home/lou/mediastack-new/backups/watchtower"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DATA_ROOT="/home/lou/mediastack-new/appdata"

mkdir -p "$BACKUP_DIR"

# Backup configuration
echo "Backing up Watchtower configuration..."
tar -czf "$BACKUP_DIR/watchtower_config_$TIMESTAMP.tar.gz" -C "$DATA_ROOT" watchtower/config/

# Backup logs (last 7 days)
echo "Backing up logs..."
find "$DATA_ROOT/watchtower/logs" -name "*.log" -mtime -7 -exec tar -czf "$BACKUP_DIR/watchtower_logs_$TIMESTAMP.tar.gz" {} + 2>/dev/null || echo "No logs to backup"

# Backup container state
echo "Backing up container information..."
docker inspect mediastack_watchtower > "$BACKUP_DIR/watchtower_container_info_$TIMESTAMP.json"

# Cleanup old backups (keep last 30 days)
find "$BACKUP_DIR" -name "watchtower_*" -type f -mtime +30 -delete

echo "Backup completed: $BACKUP_DIR"
EOF
    
    chmod +x "${DATA_ROOT}/watchtower/backup-watchtower.sh"
    log_success "Backup script created"
}

# Function to create log rotation configuration
create_log_rotation() {
    log_info "Creating log rotation configuration..."
    
    cat > "${DATA_ROOT}/watchtower/config/logrotate.conf" << EOF
# Watchtower Log Rotation Configuration

${DATA_ROOT}/watchtower/logs/watchtower.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 root root
    postrotate
        docker kill -s USR1 mediastack_watchtower 2>/dev/null || true
    endscript
}

${DATA_ROOT}/watchtower/logs/update.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 644 root root
}
EOF
    
    log_success "Log rotation configuration created"
}

# Function to update environment variables
update_environment_variables() {
    log_info "Updating environment variables..."
    
    ENV_FILE="${CONFIG_DIR}/../.env"
    
    # Add Watchtower-specific variables
    vars=(
        "WATCHTOWER_API_TOKEN="
        "SLACK_WEBHOOK_URL="
        "DISCORD_WEBHOOK_URL="
        "SMTP_SERVER="
        "SMTP_USERNAME="
        "SMTP_PASSWORD="
        "GOTIFY_TOKEN="
    )
    
    for var in "${vars[@]}"; do
        if ! grep -q "^${var%=}" "$ENV_FILE" 2>/dev/null; then
            echo "$var" >> "$ENV_FILE"
            log_info "Added $var to environment file"
        fi
    done
    
    log_success "Environment variables updated"
}

# Function to apply container labels
apply_container_labels() {
    log_info "Applying container labels..."
    
    # Check if docker-compose.yml exists
    if [[ ! -f "$COMPOSE_FILE" ]]; then
        log_error "Docker compose file not found at $COMPOSE_FILE"
        return 1
    fi
    
    log_info "Container labels should be applied manually to your docker-compose.yml"
    log_info "Reference: ${CONFIG_DIR}/service-labels.yml"
    
    # Create a patch file for easy application
    cat > "${CONFIG_DIR}/apply-labels.patch" << 'EOF'
# Apply these labels to your services in docker-compose.yml

# Critical services (no auto-update)
postgres:
  labels:
    - "watchtower.enable=false"
    - "watchtower.monitor-only=true"

# Media services (auto-update allowed)
jellyfin:
  labels:
    - "watchtower.enable=true"
    - "watchtower.stop-timeout=30s"

# See service-labels.yml for complete configuration
EOF
    
    log_success "Label reference created at ${CONFIG_DIR}/apply-labels.patch"
}

# Function to test configuration
test_configuration() {
    log_info "Testing Watchtower configuration..."
    
    # Test if Watchtower is responding
    if docker ps --format "table {{.Names}}" | grep -q "$WATCHTOWER_CONTAINER"; then
        log_success "Watchtower container is running"
        
        # Test API endpoint if available
        if curl -s -f "http://localhost:8088/v1/health" >/dev/null 2>&1; then
            log_success "API endpoint is accessible"
        else
            log_warning "API endpoint not accessible (may need restart)"
        fi
    else
        log_warning "Watchtower container is not running"
    fi
    
    # Test configuration files
    config_files=(
        "${DATA_ROOT}/watchtower/config/notifications.yml"
        "${DATA_ROOT}/watchtower/config/update-policies.yml"
        "${DATA_ROOT}/watchtower/config/monitoring.yml"
    )
    
    for file in "${config_files[@]}"; do
        if [[ -f "$file" ]]; then
            log_success "Configuration file exists: $(basename "$file")"
        else
            log_error "Configuration file missing: $file"
        fi
    done
    
    return 0
}

# Function to restart Watchtower
restart_watchtower() {
    log_info "Restarting Watchtower container..."
    
    if docker restart "$WATCHTOWER_CONTAINER" 2>/dev/null; then
        log_success "Watchtower restarted successfully"
        
        # Wait for container to be ready
        sleep 10
        
        if docker ps --format "table {{.Names}}" | grep -q "$WATCHTOWER_CONTAINER"; then
            log_success "Watchtower is running after restart"
        else
            log_error "Watchtower failed to start after restart"
            return 1
        fi
    else
        log_error "Failed to restart Watchtower"
        return 1
    fi
}

# Function to show webhook test commands
show_webhook_tests() {
    log_info "Webhook test commands:"
    
    echo
    echo "Test Slack webhook:"
    echo "curl -X POST -H 'Content-type: application/json' \\"
    echo "  --data '{\"text\":\"Test notification from Watchtower\"}' \\"
    echo "  \$SLACK_WEBHOOK_URL"
    
    echo
    echo "Test Discord webhook:"
    echo "curl -X POST -H 'Content-type: application/json' \\"
    echo "  --data '{\"content\":\"Test notification from Watchtower\"}' \\"
    echo "  \$DISCORD_WEBHOOK_URL"
    
    echo
    echo "Test Watchtower API (after restart):"
    echo "curl -H 'Authorization: Bearer \$WATCHTOWER_API_TOKEN' \\"
    echo "  http://localhost:8088/v1/updates"
}

# Function to create systemd service for log rotation
create_systemd_logrotate() {
    log_info "Creating systemd service for log rotation..."
    
    cat > /tmp/watchtower-logrotate.service << EOF
[Unit]
Description=Watchtower Log Rotation
After=docker.service

[Service]
Type=oneshot
ExecStart=/usr/sbin/logrotate -f ${DATA_ROOT}/watchtower/config/logrotate.conf
User=root

[Install]
WantedBy=multi-user.target
EOF
    
    cat > /tmp/watchtower-logrotate.timer << EOF
[Unit]
Description=Run Watchtower log rotation daily
Requires=watchtower-logrotate.service

[Timer]
OnCalendar=daily
Persistent=true

[Install]
WantedBy=timers.target
EOF
    
    log_success "Systemd service files created in /tmp/"
    log_info "To install: sudo cp /tmp/watchtower-logrotate.* /etc/systemd/system/"
    log_info "Then: sudo systemctl enable --now watchtower-logrotate.timer"
}

# Main execution
main() {
    log_info "Starting Watchtower enhanced configuration setup..."
    
    # Check current status
    check_watchtower_status
    
    # Create directory structure
    create_directories
    
    # Generate API token
    generate_api_token
    
    # Create configuration files
    create_notification_config
    create_update_policies
    create_monitoring_config
    create_backup_script
    create_log_rotation
    
    # Update environment
    update_environment_variables
    
    # Apply container labels
    apply_container_labels
    
    # Create systemd service
    create_systemd_logrotate
    
    # Test configuration
    if test_configuration; then
        log_success "Configuration test passed"
    else
        log_error "Configuration test failed"
        exit 1
    fi
    
    # Ask user if they want to restart Watchtower
    echo
    read -p "Do you want to restart Watchtower to apply changes? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        restart_watchtower
    fi
    
    log_success "Watchtower configuration completed!"
    log_info "Next steps:"
    log_info "1. Configure webhook URLs in .env file"
    log_info "2. Apply container labels to docker-compose.yml"
    log_info "3. Set up monitoring dashboards"
    log_info "4. Test notifications"
    log_info "5. Schedule regular backups"
    
    echo
    log_info "Configuration files created in: ${DATA_ROOT}/watchtower/config/"
    log_info "Backup script: ${DATA_ROOT}/watchtower/backup-watchtower.sh"
    log_info "Service labels reference: ${CONFIG_DIR}/service-labels.yml"
    
    # Show webhook test commands
    show_webhook_tests
}

# Run if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
