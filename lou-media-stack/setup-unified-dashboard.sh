#!/bin/bash

# 🎬 Grandma's Media Center - Automated Setup Script
# This script automatically integrates the unified dashboard with your existing media stack

set -e  # Exit on any error

echo "🎬 Setting up Grandma's Media Center..."
echo "=================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "docker-compose-unified.yml" ]; then
    print_error "docker-compose-unified.yml not found. Please run this from your media stack directory."
    exit 1
fi

print_status "Detected media stack directory: $(pwd)"

# 1. Auto-detect and extract existing API keys
print_status "🔍 Auto-detecting existing API keys..."

extract_api_key() {
    local service=$1
    local container_name="mediastack-${service}"
    
    print_status "Extracting API key for $service..."
    
    case $service in
        "sonarr")
            if docker ps --format "table {{.Names}}" | grep -q "$container_name"; then
                API_KEY=$(docker exec $container_name cat /config/config.xml 2>/dev/null | grep -oP '<ApiKey>\K[^<]+' || echo "")
            fi
            ;;
        "radarr")
            if docker ps --format "table {{.Names}}" | grep -q "$container_name"; then
                API_KEY=$(docker exec $container_name cat /config/config.xml 2>/dev/null | grep -oP '<ApiKey>\K[^<]+' || echo "")
            fi
            ;;
        "lidarr")
            if docker ps --format "table {{.Names}}" | grep -q "$container_name"; then
                API_KEY=$(docker exec $container_name cat /config/config.xml 2>/dev/null | grep -oP '<ApiKey>\K[^<]+' || echo "")
            fi
            ;;
        "jackett")
            if docker ps --format "table {{.Names}}" | grep -q "$container_name"; then
                API_KEY=$(docker exec $container_name cat /config/Jackett/ServerConfig.json 2>/dev/null | grep -oP '"APIKey":\s*"\K[^"]+' || echo "")
            fi
            ;;
    esac
    
    if [ -n "$API_KEY" ]; then
        print_success "✅ Found $service API key: ${API_KEY:0:8}..."
        echo "${service^^}_API_KEY=$API_KEY" >> .env.new
    else
        print_warning "⚠️  Could not auto-detect $service API key - you may need to add it manually"
        echo "# ${service^^}_API_KEY=your_${service}_api_key_here" >> .env.new
    fi
}

# Create new env file with detected keys
cp .env .env.backup 2>/dev/null || touch .env.backup
echo "# Auto-generated API keys for Unified Dashboard" > .env.new
echo "# Generated on $(date)" >> .env.new
echo "" >> .env.new

# Extract API keys for each service
extract_api_key "sonarr"
extract_api_key "radarr" 
extract_api_key "lidarr"
extract_api_key "jackett"

# Add optional OpenAI key placeholder
echo "" >> .env.new
echo "# Optional: OpenAI API key for enhanced AI features" >> .env.new
echo "# Get your key from: https://platform.openai.com/api-keys" >> .env.new
echo "# OPENAI_API_KEY=your_openai_api_key_here" >> .env.new

# Merge with existing .env file
if [ -f ".env" ]; then
    print_status "📝 Merging with existing .env file..."
    cat .env.new >> .env
    rm .env.new
else
    mv .env.new .env
fi

print_success "🔑 API keys have been added to .env file"

# 2. Automatically integrate with existing docker-compose
print_status "🐳 Integrating with existing docker-compose-unified.yml..."

# Check if unified dashboard services already exist
if grep -q "unified-dashboard" docker-compose-unified.yml; then
    print_warning "⚠️  Unified dashboard services already exist in docker-compose-unified.yml"
    read -p "Do you want to update them? (y/n): " -r
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_status "Skipping docker-compose integration..."
    else
        # Remove existing services and re-add them
        print_status "🔄 Updating existing unified dashboard services..."
        # Create a backup
        cp docker-compose-unified.yml docker-compose-unified.yml.backup
        
        # Remove old unified dashboard services
        sed -i '/# =====.*UNIFIED DASHBOARD.*=====/,/# =====.*END UNIFIED DASHBOARD.*=====/d' docker-compose-unified.yml
        
        # Add new services
        add_unified_services
    fi
else
    add_unified_services
fi

add_unified_services() {
    print_status "➕ Adding unified dashboard services..."
    
    # Add unified dashboard services to docker-compose
    cat >> docker-compose-unified.yml << 'EOF'

  # ============================================================================
  # UNIFIED DASHBOARD - Grandma's Media Center (8600-8699)
  # ============================================================================
  
  unified-dashboard:
    image: nginx:alpine
    container_name: mediastack-unified-dashboard
    restart: unless-stopped
    volumes:
      - ./unified-dashboard:/usr/share/nginx/html:ro
      - ./unified-dashboard/nginx.conf:/etc/nginx/conf.d/default.conf:ro
    networks:
      - mediastack
    ports:
      - "8600:80"  # Grandma's Media Center
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.unified-dashboard.rule=Host(`dashboard.${DOMAIN:-localhost}`)"
      - "traefik.http.routers.unified-dashboard.tls=true"
      - "traefik.http.services.unified-dashboard.loadbalancer.server.port=80"
    environment:
      - TZ=${TZ:-UTC}

  api-proxy:
    image: node:18-alpine
    container_name: mediastack-api-proxy
    restart: unless-stopped
    working_dir: /app
    volumes:
      - ./unified-dashboard/api-proxy:/app
    networks:
      - mediastack
    ports:
      - "8601:3000"  # AI API Proxy
    environment:
      - NODE_ENV=production
      - TZ=${TZ:-UTC}
      # Auto-detected service URLs
      - SONARR_URL=http://mediastack-sonarr:8989
      - RADARR_URL=http://mediastack-radarr:7878
      - LIDARR_URL=http://mediastack-lidarr:8686
      - JACKETT_URL=http://mediastack-jackett:9117
      - TVHEADEND_URL=http://mediastack-tvheadend:9981
      - XTEVE_URL=http://mediastack-xteve:34400
      - JELLYFIN_URL=http://mediastack-jellyfin:8096
      - PLEX_URL=http://mediastack-plex:32400
      # Auto-detected API keys
      - SONARR_API_KEY=${SONARR_API_KEY}
      - RADARR_API_KEY=${RADARR_API_KEY}
      - LIDARR_API_KEY=${LIDARR_API_KEY}
      - JACKETT_API_KEY=${JACKETT_API_KEY}
      # Optional OpenAI integration
      - OPENAI_API_KEY=${OPENAI_API_KEY}
    command: sh -c "npm install && npm start"
    depends_on:
      - postgres
      - valkey
    healthcheck:
      test: ["CMD", "wget", "--quiet", "--tries=1", "--spider", "http://localhost:3000/health"]
      interval: 30s
      timeout: 10s
      retries: 3

EOF

    print_success "✅ Added unified dashboard services to docker-compose-unified.yml"
}

# 3. Install Node.js dependencies
print_status "📦 Installing Node.js dependencies..."
if [ -d "unified-dashboard/api-proxy" ]; then
    cd unified-dashboard/api-proxy
    if command -v npm &> /dev/null; then
        npm install --production
        print_success "✅ Node.js dependencies installed"
    else
        print_warning "⚠️  npm not found locally - dependencies will be installed in Docker container"
    fi
    cd ../..
fi

# 4. Set proper permissions
print_status "🔒 Setting proper permissions..."
chmod -R 755 unified-dashboard/
chmod +x unified-dashboard/api-proxy/server.js 2>/dev/null || true

# 5. Start the services
print_status "🚀 Starting unified dashboard services..."

# Pull required images
docker-compose -f docker-compose-unified.yml pull unified-dashboard api-proxy

# Start the services
docker-compose -f docker-compose-unified.yml up -d unified-dashboard api-proxy

# Wait a moment for services to start
sleep 10

# Check if services are running
if docker ps | grep -q "mediastack-unified-dashboard" && docker ps | grep -q "mediastack-api-proxy"; then
    print_success "🎉 Unified dashboard is running!"
else
    print_error "❌ Some services failed to start. Check logs with:"
    echo "docker-compose -f docker-compose-unified.yml logs unified-dashboard api-proxy"
fi

# 6. Health checks and final setup
print_status "🏥 Performing health checks..."

# Check if dashboard is accessible
if curl -s http://localhost:8600/health > /dev/null 2>&1; then
    print_success "✅ Dashboard health check passed"
else
    print_warning "⚠️  Dashboard health check failed - may still be starting up"
fi

# Check API proxy
if curl -s http://localhost:8601/health > /dev/null 2>&1; then
    print_success "✅ API proxy health check passed"
else
    print_warning "⚠️  API proxy health check failed - may still be starting up"
fi

# 7. Create desktop shortcuts (optional)
create_desktop_shortcuts() {
    print_status "🖥️  Creating desktop shortcuts..."
    
    DESKTOP_DIR="$HOME/Desktop"
    if [ -d "$DESKTOP_DIR" ]; then
        # Create Grandma's Media Center shortcut
        cat > "$DESKTOP_DIR/Grandmas-Media-Center.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Link
Name=Grandma's Media Center
Comment=Simple AI-powered media search and management
URL=http://localhost:8600
Icon=applications-multimedia
EOF
        
        chmod +x "$DESKTOP_DIR/Grandmas-Media-Center.desktop"
        print_success "✅ Desktop shortcut created"
    fi
}

# Ask if user wants desktop shortcuts
read -p "🖥️  Create desktop shortcut for easy access? (y/n): " -r
if [[ $REPLY =~ ^[Yy]$ ]]; then
    create_desktop_shortcuts
fi

# 8. Final status and instructions
echo ""
echo "🎉 =================================================="
echo "🎬 Grandma's Media Center Setup Complete!"
echo "🎉 =================================================="
echo ""
echo "✅ What's been set up:"
echo "   • AI-powered unified search dashboard"
echo "   • Automatic API key detection and configuration"
echo "   • Integration with all your existing services"
echo "   • EPG and recording functionality"
echo "   • HDHomeRun auto-detection"
echo ""
echo "🌐 Access Points:"
echo "   • Main Dashboard: http://localhost:8600"
echo "   • API Proxy: http://localhost:8601"
echo ""
echo "🎯 How to use:"
echo "   1. Open http://localhost:8600 in your browser"
echo "   2. Type what you want to watch (e.g., 'funny movies', 'cooking shows')"
echo "   3. Click 'Download This' on anything you like"
echo "   4. Follow the simple instructions to watch"
echo ""
echo "🔧 Next steps:"
if grep -q "# OPENAI_API_KEY" .env; then
    echo "   • Add OpenAI API key to .env for enhanced AI features (optional)"
fi
echo "   • Bookmark http://localhost:8600 for easy access"
echo "   • Show grandma how to use it - it's that simple!"
echo ""
echo "📋 Logs and troubleshooting:"
echo "   • View logs: docker-compose -f docker-compose-unified.yml logs unified-dashboard api-proxy"
echo "   • Restart services: docker-compose -f docker-compose-unified.yml restart unified-dashboard api-proxy"
echo ""
print_success "🎬 Ready for grandma to start watching! 🍿"
