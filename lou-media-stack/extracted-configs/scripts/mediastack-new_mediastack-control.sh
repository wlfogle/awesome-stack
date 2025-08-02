#!/bin/bash

# MediaStack Control Center
# Advanced management script for your media stack

set -e

MEDIASTACK_DIR="/home/lou/mediastack-new"
COMPOSE_FILE="$MEDIASTACK_DIR/docker-compose.yml"
ENV_FILE="$MEDIASTACK_DIR/.env"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}    MediaStack Control Center${NC}"
    echo -e "${BLUE}================================${NC}"
}

# Function to check if Docker is running
check_docker() {
    if ! docker info >/dev/null 2>&1; then
        print_error "Docker is not running or not accessible"
        exit 1
    fi
}

# Function to show service status
show_status() {
    print_header
    echo -e "${CYAN}Service Status:${NC}"
    echo
    
    cd "$MEDIASTACK_DIR"
    docker-compose ps --format "table {{.Name}}\t{{.Status}}\t{{.Ports}}" | head -20
    echo
    echo -e "${PURPLE}Use 'mediastack-control.sh status-full' for complete list${NC}"
}

# Function to show full status
show_full_status() {
    print_header
    echo -e "${CYAN}Complete Service Status:${NC}"
    echo
    
    cd "$MEDIASTACK_DIR"
    docker-compose ps
}

# Function to show logs
show_logs() {
    if [ -z "$2" ]; then
        print_error "Usage: $0 logs <service_name>"
        exit 1
    fi
    
    cd "$MEDIASTACK_DIR"
    docker-compose logs -f --tail=50 "$2"
}

# Function to restart service
restart_service() {
    if [ -z "$2" ]; then
        print_error "Usage: $0 restart <service_name>"
        exit 1
    fi
    
    print_status "Restarting $2..."
    cd "$MEDIASTACK_DIR"
    docker-compose restart "$2"
    print_status "$2 restarted successfully"
}

# Function to start all services
start_all() {
    print_status "Starting all MediaStack services..."
    cd "$MEDIASTACK_DIR"
    docker-compose up -d
    print_status "All services started"
}

# Function to stop all services
stop_all() {
    print_status "Stopping all MediaStack services..."
    cd "$MEDIASTACK_DIR"
    docker-compose down
    print_status "All services stopped"
}

# Function to update services
update_services() {
    print_status "Updating MediaStack services..."
    cd "$MEDIASTACK_DIR"
    docker-compose pull
    docker-compose up -d
    print_status "Services updated and restarted"
}

# Function to show resource usage
show_resources() {
    print_header
    echo -e "${CYAN}Resource Usage:${NC}"
    echo
    
    echo -e "${PURPLE}Docker System Info:${NC}"
    docker system df
    echo
    
    echo -e "${PURPLE}Container Resource Usage:${NC}"
    docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}" | head -20
}

# Function to cleanup unused resources
cleanup() {
    print_status "Cleaning up unused Docker resources..."
    docker system prune -f
    docker volume prune -f
    docker network prune -f
    print_status "Cleanup completed"
}

# Function to backup configurations
backup_configs() {
    BACKUP_DIR="/home/lou/mediastack-backups/$(date +%Y%m%d_%H%M%S)"
    print_status "Creating backup at $BACKUP_DIR..."
    
    mkdir -p "$BACKUP_DIR"
    cp -r "$MEDIASTACK_DIR/config" "$BACKUP_DIR/"
    cp -r "$MEDIASTACK_DIR/appdata" "$BACKUP_DIR/"
    cp "$MEDIASTACK_DIR/docker-compose.yml" "$BACKUP_DIR/"
    cp "$MEDIASTACK_DIR/.env" "$BACKUP_DIR/"
    
    print_status "Backup created at $BACKUP_DIR"
}

# Function to show quick access URLs
show_urls() {
    print_header
    echo -e "${CYAN}Quick Access URLs:${NC}"
    echo
    
    echo -e "${GREEN}Control & Management:${NC}"
    echo "Homepage Dashboard: http://localhost:3002"
    echo "Traefik Dashboard: http://localhost:8080"
    echo "Authentik: http://localhost:9000"
    echo
    
    echo -e "${GREEN}Media Servers:${NC}"
    echo "Jellyfin: http://localhost:8096"
    echo "Plex: http://localhost:32400"
    echo "Emby: http://localhost:8097"
    echo
    
    echo -e "${GREEN}Download Management:${NC}"
    echo "Deluge: http://localhost:8112"
    echo "Jackett: http://localhost:9117"
    echo "Prowlarr: http://localhost:9696"
    echo
    
    echo -e "${GREEN}Content Management:${NC}"
    echo "Sonarr: http://localhost:8989"
    echo "Radarr: http://localhost:7878"
    echo "Lidarr: http://localhost:8686"
    echo "Bazarr: http://localhost:6767"
    echo
    
    echo -e "${GREEN}IPTV & Live TV:${NC}"
    echo "TVApp2: http://localhost:8088"
    echo "Channels DVR: http://localhost:8089"
    echo "TVHeadend: http://localhost:9981"
    echo
    
    echo -e "${GREEN}Monitoring:${NC}"
    echo "Tautulli: http://localhost:8181"
    echo "Grafana: http://localhost:3000"
    echo "Prometheus: http://localhost:9090"
}

# Function to show health check
health_check() {
    print_header
    echo -e "${CYAN}Health Check:${NC}"
    echo
    
    cd "$MEDIASTACK_DIR"
    
    # Check critical services
    critical_services=("traefik" "authentik" "postgres" "jellyfin" "homepage")
    
    for service in "${critical_services[@]}"; do
        if docker-compose ps "$service" | grep -q "Up"; then
            echo -e "${GREEN}✓${NC} $service is running"
        else
            echo -e "${RED}✗${NC} $service is not running"
        fi
    done
    
    echo
    echo -e "${PURPLE}Storage Usage:${NC}"
    df -h /mnt/mediastack-ssd /mnt/mediastack-usb 2>/dev/null || echo "External storage not mounted"
    
    echo
    echo -e "${PURPLE}Network Status:${NC}"
    docker network ls | grep mediastack
}

# Function to show interactive menu
show_menu() {
    print_header
    echo -e "${CYAN}Available Commands:${NC}"
    echo
    echo "  start           - Start all services"
    echo "  stop            - Stop all services"
    echo "  restart <name>  - Restart specific service"
    echo "  status          - Show service status (first 20)"
    echo "  status-full     - Show complete service status"
    echo "  logs <name>     - Show logs for specific service"
    echo "  update          - Update and restart services"
    echo "  resources       - Show resource usage"
    echo "  cleanup         - Clean up unused Docker resources"
    echo "  backup          - Backup configurations"
    echo "  urls            - Show quick access URLs"
    echo "  health          - Run health check"
    echo "  menu            - Show this menu"
    echo
    echo -e "${PURPLE}Examples:${NC}"
    echo "  $0 restart jellyfin"
    echo "  $0 logs sonarr"
    echo "  $0 status"
}

# Main script logic
main() {
    check_docker
    
    case "${1:-menu}" in
        "start")
            start_all
            ;;
        "stop")
            stop_all
            ;;
        "restart")
            restart_service "$@"
            ;;
        "status")
            show_status
            ;;
        "status-full")
            show_full_status
            ;;
        "logs")
            show_logs "$@"
            ;;
        "update")
            update_services
            ;;
        "resources")
            show_resources
            ;;
        "cleanup")
            cleanup
            ;;
        "backup")
            backup_configs
            ;;
        "urls")
            show_urls
            ;;
        "health")
            health_check
            ;;
        "menu"|*)
            show_menu
            ;;
    esac
}

# Run main function
main "$@"
