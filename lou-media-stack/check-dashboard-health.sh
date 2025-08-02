#!/bin/bash

# 🩺 Grandma's Media Center Health Check
# Quick script to verify everything is working

echo "🩺 Checking Grandma's Media Center Health..."
echo "============================================"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_service() {
    local name=$1
    local url=$2
    local container=$3
    
    echo -n "Checking $name... "
    
    # Check if container is running
    if docker ps --format "table {{.Names}}" | grep -q "$container"; then
        echo -n "Container Running ✓ "
        
        # Check if service responds
        if curl -s "$url" > /dev/null 2>&1; then
            echo -e "${GREEN}Service Healthy ✅${NC}"
        else
            echo -e "${YELLOW}Service Not Responding ⚠️${NC}"
        fi
    else
        echo -e "${RED}Container Stopped ❌${NC}"
    fi
}

# Check main services
check_service "Unified Dashboard" "http://localhost:8600" "mediastack-unified-dashboard"
check_service "AI API Proxy" "http://localhost:8601/health" "mediastack-api-proxy"

echo ""
echo "🌐 Quick Access Links:"
echo "• Main Dashboard: http://localhost:8600"
echo "• Jellyfin: http://localhost:8200"
echo "• Plex: http://localhost:8201"
echo ""

# Check if dashboard is accessible
if curl -s http://localhost:8600 > /dev/null 2>&1; then
    echo -e "${GREEN}🎉 Grandma's Media Center is ready to use!${NC}"
    echo "Just open http://localhost:8600 and start searching!"
else
    echo -e "${RED}❌ Dashboard not accessible. Try running:${NC}"
    echo "docker-compose -f docker-compose-unified.yml restart unified-dashboard api-proxy"
fi
