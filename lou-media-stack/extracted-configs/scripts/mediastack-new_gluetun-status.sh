#!/bin/bash

# Simple Gluetun VPN Status Check
# Quick status overview of your Gluetun VPN connection

GLUETUN_CONTAINER="mediastack_gluetun"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🔐 Gluetun VPN Status${NC}"
echo "===================="

# Container Status
STATUS=$(docker inspect -f '{{.State.Status}}' "$GLUETUN_CONTAINER" 2>/dev/null || echo "not found")
HEALTH=$(docker inspect -f '{{.State.Health.Status}}' "$GLUETUN_CONTAINER" 2>/dev/null || echo "no health check")

if [[ "$STATUS" == "running" ]]; then
    echo -e "Container: ${GREEN}✅ Running${NC}"
else
    echo -e "Container: ${RED}❌ Not Running${NC}"
fi

if [[ "$HEALTH" == "healthy" ]]; then
    echo -e "Health: ${GREEN}✅ Healthy${NC}"
elif [[ "$HEALTH" == "starting" ]]; then
    echo -e "Health: ${YELLOW}⏳ Starting${NC}"
else
    echo -e "Health: ${RED}❌ Unhealthy${NC}"
fi

# VPN Connection Status
echo ""
echo -e "${BLUE}VPN Connection:${NC}"

# Try to get public IP through the VPN
PUBLIC_IP=$(docker exec "$GLUETUN_CONTAINER" wget -qO- http://checkip.amazonaws.com 2>/dev/null | head -1)
if [[ -n "$PUBLIC_IP" ]]; then
    echo -e "Public IP: ${GREEN}$PUBLIC_IP${NC}"
    echo -e "Status: ${GREEN}✅ Connected${NC}"
else
    echo -e "Public IP: ${RED}Unknown${NC}"
    echo -e "Status: ${RED}❌ Not Connected${NC}"
fi

# Check if kill switch is active
echo ""
echo -e "${BLUE}Security:${NC}"
FIREWALL_RULES=$(docker exec "$GLUETUN_CONTAINER" iptables -L -n 2>/dev/null | grep -c "DROP\|REJECT" || echo "0")
if [[ "$FIREWALL_RULES" -gt "0" ]]; then
    echo -e "Kill Switch: ${GREEN}✅ Active ($FIREWALL_RULES rules)${NC}"
else
    echo -e "Kill Switch: ${YELLOW}⚠️ Status unclear${NC}"
fi

# Services using VPN
echo ""
echo -e "${BLUE}Services via VPN:${NC}"
for service in deluge jackett flaresolverr; do
    SERVICE_STATUS=$(docker inspect -f '{{.State.Status}}' "mediastack_$service" 2>/dev/null || echo "not found")
    if [[ "$SERVICE_STATUS" == "running" ]]; then
        echo -e "$service: ${GREEN}✅ Running${NC}"
    else
        echo -e "$service: ${RED}❌ Not Running${NC}"
    fi
done

echo ""
echo -e "${BLUE}Last updated: $(date)${NC}"
