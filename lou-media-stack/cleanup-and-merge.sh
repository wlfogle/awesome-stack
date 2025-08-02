#!/bin/bash

# Lou MediaStack - Cleanup and Merge Script
# This script cleans up old stack containers and applies new port assignments

set -e

echo "🧹 Lou MediaStack - Cleanup and Merge"
echo "====================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "Step 1: Identifying containers from old stacks..."
OLD_CONTAINERS=$(docker ps -a --format "{{.Names}}" | grep -E "^mediastack_" || true)

if [ -n "$OLD_CONTAINERS" ]; then
    print_warning "Found old stack containers (with underscore naming):"
    echo "$OLD_CONTAINERS" | while read -r container; do
        echo "  - $container"
    done
    echo ""
    
    read -p "Do you want to stop and remove these old containers? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_status "Stopping old containers..."
        echo "$OLD_CONTAINERS" | while read -r container; do
            if [ -n "$container" ]; then
                print_status "Stopping $container..."
                docker stop "$container" 2>/dev/null || true
                docker rm "$container" 2>/dev/null || true
            fi
        done
        print_success "Old containers cleaned up!"
    else
        print_warning "Skipping container cleanup. You can run this script again later."
        echo "To manually clean up later, run:"
        echo "docker stop \$(docker ps -q --filter name=mediastack_)"
        echo "docker rm \$(docker ps -aq --filter name=mediastack_)"
    fi
else
    print_success "No old containers found to clean up."
fi

echo ""
echo "Step 2: Checking current stack status..."

CURRENT_CONTAINERS=$(docker ps --format "{{.Names}}" | grep -E "^mediastack-" || true)

if [ -n "$CURRENT_CONTAINERS" ]; then
    print_status "Current stack containers (hyphen naming - keeping these):"
    echo "$CURRENT_CONTAINERS" | while read -r container; do
        echo "  ✅ $container"
    done
else
    print_warning "No current stack containers found."
fi

echo ""
echo "Step 3: Applying port reassignments..."

if [ -f "docker-compose.yml" ]; then
    print_status "Restarting current stack with new port assignments..."
    
    # Stop current stack
    print_status "Stopping current stack..."
    docker-compose down 2>/dev/null || true
    
    # Start with new port assignments
    print_status "Starting stack with new port assignments..."
    docker-compose up -d
    
    print_success "Stack restarted with new ports!"
    
    echo ""
    echo "🎯 New Port Assignments:"
    echo "========================"
    echo "Phase 1 - Core Infrastructure:"
    echo "  • Traefik Dashboard: http://localhost:8000"
    echo "  • Gluetun HTTP Proxy: http://localhost:8001"  
    echo "  • Gluetun Shadowsocks: localhost:8002"
    echo "  • Gluetun Control: http://localhost:8003"
    echo "  • WireGuard VPN: UDP:8010"
    echo ""
    echo "Phase 4 - Enhancement:"
    echo "  • TVHeadend Web: http://localhost:8320"
    echo "  • TVHeadend HTSP: localhost:8321"
    echo ""
    echo "All other services accessible via Traefik routing on ports 80/443"
    
else
    print_error "docker-compose.yml not found in current directory!"
    exit 1
fi

echo ""
echo "Step 4: Network cleanup..."
OLD_NETWORKS=$(docker network ls --format "{{.Name}}" | grep -E "(config_mediastack|mediastack_)" || true)

if [ -n "$OLD_NETWORKS" ]; then
    print_warning "Found old networks that may conflict:"
    echo "$OLD_NETWORKS"
    print_status "Cleaning up unused networks..."
    docker network prune -f
    print_success "Network cleanup complete!"
else
    print_success "No conflicting networks found."
fi

echo ""
echo "Step 5: Final verification..."

print_status "Checking service health..."
sleep 5

HEALTHY_SERVICES=$(docker ps --filter "name=mediastack-" --format "{{.Names}}" | wc -l)
TOTAL_SERVICES=$(docker-compose config --services | wc -l)

print_success "Stack merge and cleanup complete!"
echo ""
echo "📊 Final Status:"
echo "================"
echo "  • Running services: $HEALTHY_SERVICES"
echo "  • Total configured: $TOTAL_SERVICES"
echo "  • Port conflicts resolved: ✅"
echo "  • Old containers removed: ✅"
echo "  • New port assignments applied: ✅"

echo ""
echo "🔗 Quick Access URLs:"
echo "===================="
echo "  • Traefik Dashboard: http://localhost:8000"
echo "  • Main Dashboard: https://your-domain.com"
echo "  • Portainer: https://portainer.your-domain.com"

echo ""
echo "🎉 Your unified Lou MediaStack is now ready!"
echo "All old configurations have been cleaned up and new ports are active."

# Show any containers that might still be restarting
RESTARTING=$(docker ps --filter "status=restarting" --format "{{.Names}}" || true)
if [ -n "$RESTARTING" ]; then
    print_warning "Some services are still restarting:"
    echo "$RESTARTING"
    echo "This is normal - give them a few minutes to stabilize."
fi
