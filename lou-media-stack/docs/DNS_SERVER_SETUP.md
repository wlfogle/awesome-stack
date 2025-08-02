# Technitium DNS Server Setup Documentation

*Source: `/home/lou/Documents/Network-Setup/Technitium-DNS-Server-Setup.md`*

## Overview

Successfully installed and configured Technitium DNS Server on Pimox (Proxmox on Raspberry Pi) as a powerful alternative to Pi-hole. This provides network-wide ad blocking, enhanced DNS resolution, and advanced DNS server capabilities.

## System Information

- **Host System:** Pimox (Proxmox VE 8.3.3 on Raspberry Pi)
- **IP Address:** 192.168.12.213
- **Installation Method:** Docker Container
- **Installation Date:** July 8, 2025

## Installation Details

### Prerequisites

```bash
# DNS resolution fix
echo "nameserver 8.8.8.8" > /etc/resolv.conf

# Docker installation
apt update
apt install -y docker.io docker-compose
systemctl enable --now docker
```

### Technitium DNS Container

```bash
docker run -d --name technitium-dns \
  --restart=unless-stopped \
  --hostname=technitium-dns \
  -p 5380:5380 \
  -p 53:53/udp \
  -p 53:53/tcp \
  -v /opt/technitium-dns:/etc/dns \
  technitium/dns-server:latest
```

## Access Information

### Web Interface
- **URL:** http://192.168.12.213:5380
- **Default Login:** admin/admin (change immediately)
- **Features:** Configuration, monitoring, statistics, blocklist management

### DNS Server
- **IP:** 192.168.12.213
- **Port:** 53 (UDP/TCP)
- **Status:** ✅ Active and responding

## Configuration Summary

### Basic DNS Settings
- **Allow Recursion:** ✅ Enabled
- **Recursion Access:** Allow Only For Private Networks
- **Server Name:** technitium-dns
- **Domain:** technitium-dns.local

### Forwarders Configured
- Primary: 1.1.1.1 (Cloudflare)
- Secondary: 1.0.0.1 (Cloudflare)
- Backup: 8.8.8.8, 8.8.4.4 (Google)

### Ad Blocking Configuration
- **Status:** ✅ Enabled and working
- **Blocklists Applied:**
  - StevenBlack's hosts file
  - SomeoneWhoCares hosts
  - AdGuard DNS Filter

## Key Advantages Over Pi-hole

### Technical Improvements
- ✅ **Full authoritative DNS server** (not just forwarding)
- ✅ **Apple Private Relay compatibility**
- ✅ **Complete DNS record type support** (A, AAAA, CNAME, MX, TXT, PTR, NS, SRV, etc.)
- ✅ **Built-in recursive resolver**
- ✅ **Better caching mechanisms**

### Security & Privacy Features
- ✅ **DNS-over-HTTPS (DoH) support**
- ✅ **DNS-over-TLS (DoT) support**
- ✅ **DNSSEC validation**
- ✅ **Enhanced query logging**
- ✅ **Advanced filtering options**

### Management Features
- ✅ **Zone management capabilities**
- ✅ **Conditional forwarding**
- ✅ **API access for automation**
- ✅ **Better statistics and analytics**
- ✅ **Regex-based filtering**

## Network Integration

### Router Configuration
To enable network-wide DNS filtering:
1. Access your router's admin panel
2. Navigate to DNS settings
3. Set Primary DNS: `192.168.12.213`
4. Set Secondary DNS: `8.8.8.8` (backup)
5. Save and restart router

### Individual Device Configuration
- **Windows:** Network Adapter Settings → IPv4 Properties → DNS
- **macOS:** System Preferences → Network → Advanced → DNS
- **iOS/Android:** WiFi Settings → DNS Configuration
- **Linux:** `/etc/resolv.conf` or NetworkManager

## Testing Results

### DNS Resolution Test
```bash
nslookup google.com 192.168.12.213
# Result: ✅ Successfully resolved multiple IPv4 and IPv6 addresses
```

### Ad Blocking Test
```bash
nslookup doubleclick.net 192.168.12.213
# Result: ✅ NXDOMAIN (blocked successfully)
```

## Maintenance Commands

### Docker Management
```bash
# Check container status
docker ps

# View logs
docker logs technitium-dns

# Restart container
docker restart technitium-dns

# Update container
docker pull technitium/dns-server:latest
docker stop technitium-dns
docker rm technitium-dns
# Re-run creation command with new image
```

### DNS Testing
```bash
# Test DNS resolution
nslookup [domain] 192.168.12.213
dig @192.168.12.213 [domain]

# Test ad blocking
nslookup doubleclick.net 192.168.12.213
```

## Integration with MediaStack

This DNS server provides essential network infrastructure for your media stack:
- **Ad blocking** for cleaner streaming interfaces
- **Custom DNS records** for local service discovery
- **Enhanced privacy** for all media stack communications
- **Improved performance** through intelligent caching
- **Local domain resolution** for internal services
