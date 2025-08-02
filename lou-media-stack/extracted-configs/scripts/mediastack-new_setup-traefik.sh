#!/bin/bash

# Traefik Configuration Setup Script
# This script helps automate the setup of Traefik with SSL, security, and monitoring

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
TRAEFIK_URL="http://192.168.12.204:8080"
DOMAIN="${DOMAIN:-mediastack.local}"
ACME_EMAIL="${ACME_EMAIL:-admin@${DOMAIN}}"
CONFIG_DIR="/home/lou/mediastack-new/config/traefik"
DATA_ROOT="/home/lou/mediastack-new/appdata"

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

# Function to check if Traefik is running
check_traefik_status() {
    log_info "Checking Traefik status..."
    
    if curl -s -o /dev/null -w "%{http_code}" "${TRAEFIK_URL}/ping" | grep -q "200"; then
        log_success "Traefik is running and healthy"
        return 0
    else
        log_error "Traefik is not accessible at ${TRAEFIK_URL}"
        return 1
    fi
}

# Function to create directory structure
create_directories() {
    log_info "Creating directory structure..."
    
    directories=(
        "${DATA_ROOT}/traefik/config"
        "${DATA_ROOT}/traefik/letsencrypt"
        "${DATA_ROOT}/traefik/logs"
        "${DATA_ROOT}/traefik/certs"
        "/home/lou/mediastack-new/backups/traefik"
    )
    
    for dir in "${directories[@]}"; do
        mkdir -p "$dir"
        log_success "Created directory: $dir"
    done
}

# Function to set up SSL certificates
setup_ssl() {
    log_info "Setting up SSL certificate configuration..."
    
    # Create acme.json file with correct permissions
    touch "${DATA_ROOT}/traefik/letsencrypt/acme.json"
    chmod 600 "${DATA_ROOT}/traefik/letsencrypt/acme.json"
    
    log_success "SSL certificate storage configured"
    log_info "ACME email set to: ${ACME_EMAIL}"
}

# Function to create middleware configuration
create_middleware_config() {
    log_info "Creating middleware configuration..."
    
    cat > "${DATA_ROOT}/traefik/config/middleware.yml" << EOF
# Middleware Configuration
http:
  middlewares:
    # Rate Limiting
    default-ratelimit:
      rateLimit:
        burst: 100
        average: 50
        period: 1m
        
    api-ratelimit:
      rateLimit:
        burst: 20
        average: 10
        period: 1m
        
    # Security Headers
    security-headers:
      headers:
        accessControlAllowMethods:
          - GET
          - OPTIONS
          - PUT
          - POST
          - DELETE
        accessControlMaxAge: 100
        hostsProxyHeaders:
          - "X-Forwarded-Host"
        referrerPolicy: "same-origin"
        customRequestHeaders:
          X-Forwarded-Proto: "https"
        customResponseHeaders:
          X-Robots-Tag: "noindex,nofollow,nosnippet,noarchive,notranslate,noimageindex"
          server: ""
        sslRedirect: true
        sslHost: "${DOMAIN}"
        sslForceHost: true
        stsSeconds: 31536000
        stsIncludeSubdomains: true
        stsPreload: true
        forceSTSHeader: true
        frameDeny: true
        contentTypeNosniff: true
        browserXssFilter: true
        isDevelopment: false
        
    # Basic Auth (generate password with: htpasswd -nb user password)
    basic-auth:
      basicAuth:
        users:
          - "admin:\$2y\$10\$..."  # Replace with actual hash
          
    # IP Whitelist
    ip-whitelist:
      ipWhiteList:
        sourceRange:
          - "192.168.0.0/16"
          - "10.0.0.0/8"
          - "172.16.0.0/12"
          
    # Authentik Forward Auth
    authentik:
      forwardAuth:
        address: "http://mediastack_authentik_server:9000/outpost.goauthentik.io/auth/traefik"
        trustForwardHeader: true
        authResponseHeaders:
          - Remote-User
          - Remote-Name
          - Remote-Email
          - Remote-Groups
          
    # Compression
    compression:
      compress: {}
      
    # CORS
    cors:
      headers:
        accessControlAllowOriginList:
          - "https://${DOMAIN}"
          - "https://*.${DOMAIN}"
        accessControlAllowMethods:
          - GET
          - OPTIONS
          - PUT
          - POST
          - DELETE
        accessControlAllowHeaders:
          - "*"
        accessControlExposeHeaders:
          - "*"
        accessControlMaxAge: 100
        addVaryHeader: true
EOF
    
    log_success "Middleware configuration created"
}

# Function to create services configuration
create_services_config() {
    log_info "Creating services configuration..."
    
    cat > "${DATA_ROOT}/traefik/config/services.yml" << EOF
# Services Configuration
http:
  routers:
    # Traefik Dashboard
    traefik-dashboard:
      rule: "Host(\`traefik.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: api@internal
      middlewares:
        - authentik
        - security-headers
      tls:
        certResolver: letsencrypt
    
    # Jellyfin
    jellyfin:
      rule: "Host(\`jellyfin.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: jellyfin
      middlewares:
        - authentik
        - security-headers
        - default-ratelimit
        - compression
      tls:
        certResolver: letsencrypt
        
    # Sonarr
    sonarr:
      rule: "Host(\`sonarr.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: sonarr
      middlewares:
        - authentik
        - security-headers
        - api-ratelimit
      tls:
        certResolver: letsencrypt
        
    # Radarr
    radarr:
      rule: "Host(\`radarr.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: radarr
      middlewares:
        - authentik
        - security-headers
        - api-ratelimit
      tls:
        certResolver: letsencrypt
        
    # Grafana
    grafana:
      rule: "Host(\`grafana.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: grafana
      middlewares:
        - authentik
        - security-headers
        - default-ratelimit
      tls:
        certResolver: letsencrypt
        
    # Prometheus
    prometheus:
      rule: "Host(\`prometheus.${DOMAIN}\`)"
      entryPoints:
        - websecure
      service: prometheus
      middlewares:
        - authentik
        - security-headers
        - api-ratelimit
      tls:
        certResolver: letsencrypt

  services:
    # Jellyfin Service
    jellyfin:
      loadBalancer:
        servers:
          - url: "http://mediastack_jellyfin:8096"
        healthCheck:
          path: "/health"
          interval: "10s"
          timeout: "5s"
          
    # Sonarr Service
    sonarr:
      loadBalancer:
        servers:
          - url: "http://mediastack_sonarr:8989"
        healthCheck:
          path: "/api/v3/system/status"
          interval: "30s"
          timeout: "10s"
          
    # Radarr Service
    radarr:
      loadBalancer:
        servers:
          - url: "http://mediastack_radarr:7878"
        healthCheck:
          path: "/api/v3/system/status"
          interval: "30s"
          timeout: "10s"
          
    # Grafana Service
    grafana:
      loadBalancer:
        servers:
          - url: "http://mediastack_grafana:3000"
        healthCheck:
          path: "/api/health"
          interval: "30s"
          timeout: "10s"
          
    # Prometheus Service
    prometheus:
      loadBalancer:
        servers:
          - url: "http://mediastack_prometheus:9090"
        healthCheck:
          path: "/-/healthy"
          interval: "30s"
          timeout: "10s"
EOF
    
    log_success "Services configuration created"
}

# Function to create TLS configuration
create_tls_config() {
    log_info "Creating TLS configuration..."
    
    cat > "${DATA_ROOT}/traefik/config/tls.yml" << EOF
# TLS Configuration
tls:
  options:
    default:
      sslStrategies:
        - "tls.SniStrict"
      minVersion: "VersionTLS12"
      maxVersion: "VersionTLS13"
      cipherSuites:
        - "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384"
        - "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305"
        - "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256"
        - "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384"
        - "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305"
        - "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256"
      curvePreferences:
        - CurveP521
        - CurveP384
        - CurveP256
        - X25519
        
    # Strict TLS for sensitive services
    strict:
      minVersion: "VersionTLS13"
      cipherSuites:
        - "TLS_AES_256_GCM_SHA384"
        - "TLS_CHACHA20_POLY1305_SHA256"
        - "TLS_AES_128_GCM_SHA256"
      curvePreferences:
        - X25519
        - CurveP256
EOF
    
    log_success "TLS configuration created"
}

# Function to create monitoring configuration
setup_monitoring() {
    log_info "Setting up monitoring configuration..."
    
    # Create Prometheus rules
    cat > "${DATA_ROOT}/traefik/config/prometheus-rules.yml" << EOF
groups:
  - name: traefik
    rules:
      - alert: TraefikHighResponseTime
        expr: histogram_quantile(0.95, rate(traefik_service_request_duration_seconds_bucket[5m])) > 1
        for: 5m
        labels:
          severity: warning
          service: traefik
        annotations:
          summary: "High response time detected"
          description: "95th percentile response time is {{ \$value }}s for service {{ \$labels.service }}"
          
      - alert: TraefikHighErrorRate
        expr: rate(traefik_service_requests_total{code=~"5.."}[5m]) > 0.1
        for: 2m
        labels:
          severity: critical
          service: traefik
        annotations:
          summary: "High error rate detected"
          description: "Error rate is {{ \$value | humanizePercentage }} for service {{ \$labels.service }}"
          
      - alert: TraefikServiceDown
        expr: traefik_service_server_up == 0
        for: 1m
        labels:
          severity: critical
          service: traefik
        annotations:
          summary: "Service backend is down"
          description: "Backend {{ \$labels.url }} for service {{ \$labels.service }} is down"
EOF
    
    log_success "Monitoring configuration created"
}

# Function to create backup script
create_backup_script() {
    log_info "Creating backup script..."
    
    cat > "${DATA_ROOT}/traefik/backup-traefik.sh" << 'EOF'
#!/bin/bash
# Traefik Backup Script

BACKUP_DIR="/home/lou/mediastack-new/backups/traefik"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DATA_ROOT="/home/lou/mediastack-new/appdata"

mkdir -p "$BACKUP_DIR"

# Backup configurations
echo "Backing up Traefik configuration..."
tar -czf "$BACKUP_DIR/traefik_config_$TIMESTAMP.tar.gz" -C "$DATA_ROOT" traefik/config/

# Backup SSL certificates
echo "Backing up SSL certificates..."
tar -czf "$BACKUP_DIR/traefik_certs_$TIMESTAMP.tar.gz" -C "$DATA_ROOT" traefik/letsencrypt/

# Backup logs (last 7 days)
echo "Backing up logs..."
find "$DATA_ROOT/traefik/logs" -name "*.log" -mtime -7 -exec tar -czf "$BACKUP_DIR/traefik_logs_$TIMESTAMP.tar.gz" {} +

# Cleanup old backups (keep last 30 days)
find "$BACKUP_DIR" -name "traefik_*" -type f -mtime +30 -delete

echo "Backup completed: $BACKUP_DIR"
EOF
    
    chmod +x "${DATA_ROOT}/traefik/backup-traefik.sh"
    log_success "Backup script created"
}

# Function to test configuration
test_configuration() {
    log_info "Testing Traefik configuration..."
    
    # Test API endpoint
    if curl -s "${TRAEFIK_URL}/api/overview" | grep -q "routers"; then
        log_success "API endpoint is working"
    else
        log_error "API endpoint test failed"
        return 1
    fi
    
    # Test metrics endpoint
    if curl -s "${TRAEFIK_URL}/metrics" | grep -q "traefik_"; then
        log_success "Metrics endpoint is working"
    else
        log_error "Metrics endpoint test failed"
        return 1
    fi
    
    # Test ping endpoint
    if curl -s "${TRAEFIK_URL}/ping" | grep -q "OK"; then
        log_success "Ping endpoint is working"
    else
        log_error "Ping endpoint test failed"
        return 1
    fi
    
    return 0
}

# Function to restart Traefik
restart_traefik() {
    log_info "Restarting Traefik container..."
    
    if docker restart mediastack_traefik; then
        log_success "Traefik restarted successfully"
        
        # Wait for Traefik to be ready
        sleep 10
        
        if check_traefik_status; then
            log_success "Traefik is running after restart"
        else
            log_error "Traefik failed to start after restart"
            return 1
        fi
    else
        log_error "Failed to restart Traefik"
        return 1
    fi
}

# Function to generate htpasswd hash
generate_htpasswd() {
    log_info "Generating htpasswd hash..."
    
    if command -v htpasswd &> /dev/null; then
        echo "Enter username for basic auth:"
        read -r username
        echo "Enter password for basic auth:"
        read -rs password
        
        hash=$(htpasswd -nb "$username" "$password")
        log_success "Generated hash: $hash"
        log_info "Add this to your middleware configuration"
    else
        log_warning "htpasswd not found. Install apache2-utils package"
        log_info "Alternative: use online htpasswd generator"
    fi
}

# Main execution
main() {
    log_info "Starting Traefik configuration setup..."
    
    # Check if Traefik is running
    if ! check_traefik_status; then
        log_error "Traefik is not running. Please start it first."
        exit 1
    fi
    
    # Create directory structure
    create_directories
    
    # Set up SSL
    setup_ssl
    
    # Create configuration files
    create_middleware_config
    create_services_config
    create_tls_config
    setup_monitoring
    create_backup_script
    
    # Test configuration
    if test_configuration; then
        log_success "Configuration test passed"
    else
        log_error "Configuration test failed"
        exit 1
    fi
    
    # Ask user if they want to restart Traefik
    echo
    read -p "Do you want to restart Traefik to apply changes? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        restart_traefik
    fi
    
    log_success "Traefik configuration completed!"
    log_info "Next steps:"
    log_info "1. Update your DNS records to point to your server"
    log_info "2. Test SSL certificates: curl -I https://jellyfin.${DOMAIN}"
    log_info "3. Configure monitoring dashboards"
    log_info "4. Set up log rotation"
    log_info "5. Test rate limiting and security headers"
    
    echo
    log_info "Access your Traefik dashboard at: https://traefik.${DOMAIN}"
    log_info "Monitor metrics at: ${TRAEFIK_URL}/metrics"
    log_info "Check API at: ${TRAEFIK_URL}/api/overview"
    
    # Optional: Generate htpasswd hash
    echo
    read -p "Do you want to generate htpasswd hash for basic auth? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        generate_htpasswd
    fi
}

# Run if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
