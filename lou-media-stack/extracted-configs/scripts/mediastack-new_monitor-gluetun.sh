#!/bin/bash

# Gluetun VPN Health Monitor
# This script monitors the Gluetun VPN connection and provides detailed health information

set -e

GLUETUN_CONTAINER="mediastack_gluetun"
GLUETUN_HTTP_PORT="8000"
GLUETUN_IP="localhost"
LOG_FILE="/var/log/gluetun-monitor.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

check_container_status() {
    local status=$(docker inspect -f '{{.State.Status}}' "$GLUETUN_CONTAINER" 2>/dev/null || echo "not found")
    local health=$(docker inspect -f '{{.State.Health.Status}}' "$GLUETUN_CONTAINER" 2>/dev/null || echo "no health check")
    
    echo -e "${BLUE}Container Status:${NC}"
    echo -e "  Status: $status"
    echo -e "  Health: $health"
    echo ""
    
    if [[ "$status" != "running" ]]; then
        echo -e "${RED}❌ Container is not running!${NC}"
        return 1
    fi
    
    if [[ "$health" == "unhealthy" ]]; then
        echo -e "${YELLOW}⚠️  Container is unhealthy!${NC}"
        return 1
    fi
    
    echo -e "${GREEN}✅ Container is running and healthy${NC}"
    return 0
}

check_vpn_connection() {
    echo -e "${BLUE}VPN Connection Status:${NC}"
    
    # Check if Gluetun HTTP API is available
    if ! curl -s --max-time 5 "http://$GLUETUN_IP:$GLUETUN_HTTP_PORT/v1/openvpn/status" >/dev/null 2>&1; then
        echo -e "${RED}❌ Cannot reach Gluetun HTTP API${NC}"
        return 1
    fi
    
    # Get VPN status
    local vpn_status=$(curl -s --max-time 5 "http://$GLUETUN_IP:$GLUETUN_HTTP_PORT/v1/openvpn/status" | jq -r '.status // "unknown"' 2>/dev/null || echo "unknown")
    echo -e "  VPN Status: $vpn_status"
    
    # Get public IP
    local public_ip=$(curl -s --max-time 5 "http://$GLUETUN_IP:$GLUETUN_HTTP_PORT/v1/publicip/ip" | jq -r '.public_ip // "unknown"' 2>/dev/null || echo "unknown")
    echo -e "  Public IP: $public_ip"
    
    # Get port forwarding status
    local port_forwarding=$(curl -s --max-time 5 "http://$GLUETUN_IP:$GLUETUN_HTTP_PORT/v1/openvpn/portforwarded" | jq -r '.port // "none"' 2>/dev/null || echo "none")
    echo -e "  Port Forwarding: $port_forwarding"
    
    echo ""
    
    if [[ "$vpn_status" == "running" ]]; then
        echo -e "${GREEN}✅ VPN is connected and running${NC}"
        return 0
    else
        echo -e "${RED}❌ VPN is not connected properly${NC}"
        return 1
    fi
}

check_dns_resolution() {
    echo -e "${BLUE}DNS Resolution Test:${NC}"
    
    # Test DNS resolution inside container
    local dns_test=$(docker exec "$GLUETUN_CONTAINER" nslookup google.com 2>/dev/null | grep -q "Address" && echo "working" || echo "failed")
    echo -e "  DNS Resolution: $dns_test"
    
    if [[ "$dns_test" == "working" ]]; then
        echo -e "${GREEN}✅ DNS resolution is working${NC}"
        return 0
    else
        echo -e "${RED}❌ DNS resolution failed${NC}"
        return 1
    fi
}

check_kill_switch() {
    echo -e "${BLUE}Kill Switch Status:${NC}"
    
    # Check if firewall is enabled
    local firewall_status=$(docker exec "$GLUETUN_CONTAINER" sh -c 'if [ -f /tmp/gluetun/firewall ]; then echo "enabled"; else echo "disabled"; fi' 2>/dev/null || echo "unknown")
    echo -e "  Firewall: $firewall_status"
    
    # Check iptables rules
    local iptables_rules=$(docker exec "$GLUETUN_CONTAINER" iptables -L -n 2>/dev/null | grep -c "DROP\|REJECT" || echo "0")
    echo -e "  Firewall Rules: $iptables_rules active rules"
    
    echo ""
    
    if [[ "$firewall_status" == "enabled" ]] && [[ "$iptables_rules" -gt "0" ]]; then
        echo -e "${GREEN}✅ Kill switch is active${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠️  Kill switch status unclear${NC}"
        return 1
    fi
}

check_services_through_vpn() {
    echo -e "${BLUE}Services Using VPN:${NC}"
    
    # Check services that should be running through VPN
    local services=("deluge" "jackett" "flaresolverr")
    local all_healthy=true
    
    for service in "${services[@]}"; do
        local container_name="mediastack_$service"
        local status=$(docker inspect -f '{{.State.Status}}' "$container_name" 2>/dev/null || echo "not found")
        
        if [[ "$status" == "running" ]]; then
            echo -e "  $service: ${GREEN}✅ Running${NC}"
        else
            echo -e "  $service: ${RED}❌ Not running${NC}"
            all_healthy=false
        fi
    done
    
    echo ""
    
    if [[ "$all_healthy" == true ]]; then
        echo -e "${GREEN}✅ All VPN services are running${NC}"
        return 0
    else
        echo -e "${RED}❌ Some VPN services are not running${NC}"
        return 1
    fi
}

show_connection_details() {
    echo -e "${BLUE}Connection Details:${NC}"
    
    # Get detailed connection info
    local connection_info=$(curl -s --max-time 5 "http://$GLUETUN_IP:$GLUETUN_HTTP_PORT/v1/openvpn/settings" 2>/dev/null | jq '.' 2>/dev/null || echo '{}')
    
    if [[ "$connection_info" != '{}' ]]; then
        echo -e "  Server: $(echo "$connection_info" | jq -r '.server // "unknown"')"
        echo -e "  Protocol: $(echo "$connection_info" | jq -r '.protocol // "unknown"')"
        echo -e "  Cipher: $(echo "$connection_info" | jq -r '.cipher // "unknown"')"
    else
        echo -e "  ${YELLOW}Connection details not available${NC}"
    fi
    
    echo ""
}

restart_gluetun() {
    echo -e "${YELLOW}Restarting Gluetun container...${NC}"
    docker restart "$GLUETUN_CONTAINER"
    
    # Wait for container to be healthy
    echo -e "${YELLOW}Waiting for container to become healthy...${NC}"
    local timeout=60
    local counter=0
    
    while [[ $counter -lt $timeout ]]; do
        local health=$(docker inspect -f '{{.State.Health.Status}}' "$GLUETUN_CONTAINER" 2>/dev/null || echo "no health check")
        if [[ "$health" == "healthy" ]]; then
            echo -e "${GREEN}✅ Container is healthy again${NC}"
            return 0
        fi
        sleep 2
        ((counter += 2))
    done
    
    echo -e "${RED}❌ Container failed to become healthy within $timeout seconds${NC}"
    return 1
}

main() {
    echo -e "${BLUE}=== Gluetun VPN Health Monitor ===${NC}"
    echo "$(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    local all_checks_passed=true
    
    # Run all checks
    check_container_status || all_checks_passed=false
    check_vpn_connection || all_checks_passed=false
    check_dns_resolution || all_checks_passed=false
    check_kill_switch || all_checks_passed=false
    check_services_through_vpn || all_checks_passed=false
    show_connection_details
    
    # Show recent logs
    echo -e "${BLUE}Recent Logs (last 5 lines):${NC}"
    docker logs "$GLUETUN_CONTAINER" --tail 5 2>/dev/null | sed 's/^/  /'
    echo ""
    
    if [[ "$all_checks_passed" == true ]]; then
        echo -e "${GREEN}🎉 All checks passed! VPN is working correctly.${NC}"
        log_message "Health check passed - all systems operational"
        exit 0
    else
        echo -e "${RED}❌ Some checks failed. VPN may not be working correctly.${NC}"
        log_message "Health check failed - issues detected"
        
        # Auto-restart if requested
        if [[ "$1" == "--auto-restart" ]]; then
            restart_gluetun
        else
            echo -e "${YELLOW}Run with --auto-restart to automatically restart on failure${NC}"
        fi
        
        exit 1
    fi
}

# Check if jq is available
if ! command -v jq &> /dev/null; then
    echo -e "${YELLOW}⚠️  jq is not installed. Installing...${NC}"
    if command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm jq
    elif command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y jq
    else
        echo -e "${RED}❌ Cannot install jq. Please install it manually.${NC}"
        exit 1
    fi
fi

# Run main function
main "$@"
