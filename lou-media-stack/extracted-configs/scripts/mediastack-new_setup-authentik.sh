#!/bin/bash

# Authentik Configuration Setup Script
# This script helps automate the setup of Authentik with MFA, proxies, and monitoring

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
AUTHENTIK_URL="http://192.168.12.204:9000"
AUTHENTIK_API_TOKEN=""
DOMAIN="${DOMAIN:-mediastack.local}"

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

# Function to check if Authentik is running
check_authentik_status() {
    log_info "Checking Authentik status..."
    
    if curl -s -o /dev/null -w "%{http_code}" "${AUTHENTIK_URL}/-/health/live/" | grep -q "200"; then
        log_success "Authentik is running and healthy"
        return 0
    else
        log_error "Authentik is not accessible at ${AUTHENTIK_URL}"
        return 1
    fi
}

# Function to create API token
create_api_token() {
    log_info "Creating API token..."
    log_warning "Please create an API token manually in Authentik:"
    log_warning "1. Go to ${AUTHENTIK_URL}/if/admin/#/core/tokens"
    log_warning "2. Click 'Create Token'"
    log_warning "3. Set identifier to 'automation-token'"
    log_warning "4. Set user to your admin user"
    log_warning "5. Copy the token and set it in the AUTHENTIK_API_TOKEN variable"
    echo
    read -p "Press Enter after creating the token..."
}

# Function to check API connectivity
check_api_connectivity() {
    if [[ -z "$AUTHENTIK_API_TOKEN" ]]; then
        log_error "AUTHENTIK_API_TOKEN not set. Please set it first."
        return 1
    fi
    
    log_info "Testing API connectivity..."
    
    response=$(curl -s -o /dev/null -w "%{http_code}" \
        -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
        "${AUTHENTIK_URL}/api/v3/core/users/")
    
    if [[ "$response" == "200" ]]; then
        log_success "API connectivity successful"
        return 0
    else
        log_error "API connectivity failed (HTTP $response)"
        return 1
    fi
}

# Function to create groups
create_groups() {
    log_info "Creating user groups..."
    
    groups=(
        "mediastack-admins:MediaStack Administrators:true"
        "mediastack-users:MediaStack Users:false"
        "mediastack-monitoring:MediaStack Monitoring:false"
        "mediastack-media:MediaStack Media Access:false"
    )
    
    for group_info in "${groups[@]}"; do
        IFS=':' read -r name display_name is_superuser <<< "$group_info"
        
        log_info "Creating group: $display_name"
        
        curl -s -X POST "${AUTHENTIK_URL}/api/v3/core/groups/" \
            -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
            -H "Content-Type: application/json" \
            -d "{
                \"name\": \"$name\",
                \"is_superuser\": $is_superuser,
                \"attributes\": {
                    \"description\": \"$display_name\"
                }
            }" > /dev/null
        
        log_success "Created group: $display_name"
    done
}

# Function to configure MFA
configure_mfa() {
    log_info "Configuring MFA settings..."
    
    # Create TOTP authenticator stage
    log_info "Creating TOTP authenticator stage..."
    curl -s -X POST "${AUTHENTIK_URL}/api/v3/stages/authenticator_totp/" \
        -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
        -H "Content-Type: application/json" \
        -d '{
            "name": "TOTP Setup",
            "friendly_name": "Time-based One-Time Password",
            "digits": 6,
            "issuer": "MediaStack Authentik"
        }' > /dev/null
    
    # Create WebAuthn authenticator stage
    log_info "Creating WebAuthn authenticator stage..."
    curl -s -X POST "${AUTHENTIK_URL}/api/v3/stages/authenticator_webauthn/" \
        -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
        -H "Content-Type: application/json" \
        -d '{
            "name": "WebAuthn Setup",
            "friendly_name": "Security Key / Biometric",
            "user_verification": "preferred",
            "authenticator_attachment": "platform",
            "resident_key_requirement": "discouraged"
        }' > /dev/null
    
    log_success "MFA configuration completed"
}

# Function to create password policy
create_password_policy() {
    log_info "Creating password policy..."
    
    curl -s -X POST "${AUTHENTIK_URL}/api/v3/policies/password/" \
        -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
        -H "Content-Type: application/json" \
        -d '{
            "name": "Strong Password Policy",
            "execution_logging": true,
            "password_field": "password",
            "amount_uppercase": 1,
            "amount_lowercase": 1,
            "amount_digits": 1,
            "amount_symbols": 1,
            "length_min": 12,
            "error_message": "Password must be at least 12 characters with uppercase, lowercase, digit, and symbol",
            "hibp_allowed_count": 0,
            "zxcvbn_score_threshold": 3
        }' > /dev/null
    
    log_success "Password policy created"
}

# Function to create reputation policy
create_reputation_policy() {
    log_info "Creating reputation policy for brute force protection..."
    
    curl -s -X POST "${AUTHENTIK_URL}/api/v3/policies/reputation/" \
        -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
        -H "Content-Type: application/json" \
        -d '{
            "name": "Brute Force Protection",
            "execution_logging": true,
            "check_ip": true,
            "check_username": true,
            "threshold": 5,
            "duration": 300
        }' > /dev/null
    
    log_success "Reputation policy created"
}

# Function to create applications and providers
create_applications() {
    log_info "Creating applications and providers..."
    
    # Array of applications (name:slug:internal_host:port)
    apps=(
        "Jellyfin:jellyfin:mediastack_jellyfin:8096"
        "Plex:plex:mediastack_plex:32400"
        "Sonarr:sonarr:mediastack_sonarr:8989"
        "Radarr:radarr:mediastack_radarr:7878"
        "Lidarr:lidarr:mediastack_lidarr:8686"
        "Grafana:grafana:mediastack_grafana:3000"
        "Prometheus:prometheus:mediastack_prometheus:9090"
        "Portainer:portainer:mediastack_portainer:9000"
        "Traefik:traefik:mediastack_traefik:8080"
    )
    
    for app_info in "${apps[@]}"; do
        IFS=':' read -r name slug internal_host port <<< "$app_info"
        
        log_info "Creating provider for $name..."
        
        # Create provider
        provider_response=$(curl -s -X POST "${AUTHENTIK_URL}/api/v3/providers/proxy/" \
            -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
            -H "Content-Type: application/json" \
            -d "{
                \"name\": \"$name Proxy\",
                \"authorization_flow\": \"default-provider-authorization-implicit-consent\",
                \"external_host\": \"https://$slug.$DOMAIN\",
                \"internal_host\": \"http://$internal_host:$port\",
                \"mode\": \"forward_single\",
                \"skip_path_regex\": \"^/api/.*\"
            }")
        
        provider_id=$(echo "$provider_response" | jq -r '.pk')
        
        if [[ "$provider_id" != "null" ]]; then
            log_info "Creating application for $name..."
            
            # Create application
            curl -s -X POST "${AUTHENTIK_URL}/api/v3/core/applications/" \
                -H "Authorization: Bearer $AUTHENTIK_API_TOKEN" \
                -H "Content-Type: application/json" \
                -d "{
                    \"name\": \"$name\",
                    \"slug\": \"$slug\",
                    \"provider\": $provider_id,
                    \"launch_url\": \"https://$slug.$DOMAIN\",
                    \"group\": \"MediaStack\"
                }" > /dev/null
            
            log_success "Created application: $name"
        else
            log_error "Failed to create provider for $name"
        fi
    done
}

# Function to enable monitoring
enable_monitoring() {
    log_info "Enabling monitoring and metrics..."
    
    # Create Prometheus monitoring configuration
    cat > /tmp/prometheus-authentik.yml << EOF
scrape_configs:
  - job_name: 'authentik'
    static_configs:
      - targets: ['192.168.12.204:9000']
    metrics_path: '/-/metrics/'
    scrape_interval: 30s
    scrape_timeout: 10s
EOF
    
    log_success "Monitoring configuration created at /tmp/prometheus-authentik.yml"
    log_info "Add this to your Prometheus configuration"
}

# Function to create backup script
create_backup_script() {
    log_info "Creating backup script..."
    
    cat > /tmp/authentik-backup.sh << 'EOF'
#!/bin/bash
# Authentik Backup Script

BACKUP_DIR="/home/lou/mediastack-new/backups/authentik"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$BACKUP_DIR"

# Backup PostgreSQL database
echo "Backing up Authentik database..."
docker exec mediastack_postgres pg_dump -U authentik authentik > "$BACKUP_DIR/authentik_db_$TIMESTAMP.sql"

# Backup configuration (if any custom configs exist)
if [[ -d "/home/lou/mediastack-new/config/authentik" ]]; then
    echo "Backing up Authentik configuration..."
    tar -czf "$BACKUP_DIR/authentik_config_$TIMESTAMP.tar.gz" -C /home/lou/mediastack-new/config authentik/
fi

# Cleanup old backups (keep last 7 days)
find "$BACKUP_DIR" -name "authentik_*" -type f -mtime +7 -delete

echo "Backup completed: $BACKUP_DIR"
EOF
    
    chmod +x /tmp/authentik-backup.sh
    log_success "Backup script created at /tmp/authentik-backup.sh"
}

# Main execution
main() {
    log_info "Starting Authentik configuration setup..."
    
    # Check if Authentik is running
    if ! check_authentik_status; then
        log_error "Authentik is not running. Please start it first."
        exit 1
    fi
    
    # Check for required tools
    if ! command -v jq &> /dev/null; then
        log_error "jq is required but not installed. Please install it first."
        exit 1
    fi
    
    # If no API token, help user create one
    if [[ -z "$AUTHENTIK_API_TOKEN" ]]; then
        create_api_token
    fi
    
    # Test API connectivity
    if ! check_api_connectivity; then
        log_error "Cannot connect to Authentik API. Please check your token."
        exit 1
    fi
    
    # Main configuration steps
    create_groups
    configure_mfa
    create_password_policy
    create_reputation_policy
    create_applications
    enable_monitoring
    create_backup_script
    
    log_success "Authentik configuration completed!"
    log_info "Next steps:"
    log_info "1. Configure DNS/reverse proxy for your applications"
    log_info "2. Set up SSL certificates"
    log_info "3. Test login flows"
    log_info "4. Configure monitoring dashboards"
    log_info "5. Set up regular backups"
    
    echo
    log_info "Access your Authentik admin panel at: ${AUTHENTIK_URL}/if/admin/"
    log_info "View your applications at: ${AUTHENTIK_URL}/if/user/"
}

# Run if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
