# ARR STACK FOR DUMMIES - COMPLETE CONFIGURATION GUIDE

## Table of Contents
1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Environment Setup](#environment-setup)
4. [Service Configuration](#service-configuration)
5. [Integration Setup](#integration-setup)
6. [Troubleshooting](#troubleshooting)
7. [Maintenance](#maintenance)

---

## Overview

The Arr Stack is a collection of services that work together to automate media downloading and management:

- **Gluetun**: VPN client for secure downloading
- **Traefik**: Reverse proxy for web interface access
- **Authentik**: Authentication service
- **Jackett**: Indexer aggregator (finds torrents)
- **Deluge**: Torrent client (downloads torrents)
- **Sonarr**: TV show management
- **Radarr**: Movie management
- **Lidarr**: Music management
- **Jellyfin**: Media server for streaming
- **Bazarr**: Subtitle management

---

## Prerequisites

### Directory Structure
```
/home/lou/mediastack-new/
├── config/
│   ├── .env
│   ├── docker-compose.yml
│   └── various service configs/
├── media/
│   ├── movies/
│   ├── tv/
│   └── music/
└── downloads/
    ├── complete/
    └── incomplete/
```

### Required Information
- VPN provider credentials (Wireguard keys)
- Domain name (if using external access)
- Strong passwords for all services

---

## Environment Setup

### 1. Update .env File
Location: `/home/lou/mediastack-new/config/.env`

```env
# VPN Configuration
WIREGUARD_PRIVATE_KEY=your_private_key_here
WIREGUARD_PUBLIC_KEY=your_public_key_here
VPN_SERVICE_PROVIDER=your_vpn_provider
VPN_TYPE=wireguard

# Database
POSTGRES_PASSWORD=your_strong_postgres_password

# Authentik
AUTHENTIK_SECRET_KEY=your_authentik_secret_key_here
AUTHENTIK_BOOTSTRAP_PASSWORD=change_this_bootstrap_pass_ABC!

# Timezone
TZ=America/New_York

# User/Group IDs
PUID=1000
PGID=1000

# Paths
MEDIA_ROOT=/home/lou/mediastack-new/media
DOWNLOADS_ROOT=/home/lou/mediastack-new/downloads
CONFIG_ROOT=/home/lou/mediastack-new/config
```

### 2. Generate Secure Keys
```bash
# Generate Authentik secret key
python3 -c 'from secrets import token_urlsafe; print(token_urlsafe(50))'

# Generate strong passwords
openssl rand -base64 32
```

---

## Service Configuration

### PHASE 1: Core Infrastructure

#### 1. Gluetun VPN Configuration

**Purpose**: Secure all download traffic through VPN

**Access**: No web interface, monitor via logs

**Configuration**:
```yaml
# In docker-compose.yml
gluetun:
  environment:
    - VPN_SERVICE_PROVIDER=your_provider
    - VPN_TYPE=wireguard
    - WIREGUARD_PRIVATE_KEY=${WIREGUARD_PRIVATE_KEY}
    - WIREGUARD_PUBLIC_KEY=${WIREGUARD_PUBLIC_KEY}
    - WIREGUARD_ADDRESSES=10.x.x.x/32
    - SERVER_COUNTRIES=Netherlands,Switzerland
    - FIREWALL_OUTBOUND_SUBNETS=192.168.1.0/24
```

**Testing**:
```bash
# Check VPN status
docker exec gluetun curl -s https://ipinfo.io/ip

# Should return VPN IP, not your real IP
```

#### 2. Traefik Reverse Proxy

**Purpose**: Manage web access to all services with SSL

**Access**: https://traefik.yourdomain.com (if configured)

**Configuration**:
```yaml
# traefik.yml
api:
  dashboard: true
  debug: true

entryPoints:
  web:
    address: ":80"
    http:
      redirections:
        entryPoint:
          to: websecure
          scheme: https
  websecure:
    address: ":443"

certificatesResolvers:
  letsencrypt:
    acme:
      email: your-email@example.com
      storage: acme.json
      httpChallenge:
        entryPoint: web
```

**Labels for Services**:
```yaml
labels:
  - "traefik.enable=true"
  - "traefik.http.routers.service-name.rule=Host(`service.yourdomain.com`)"
  - "traefik.http.routers.service-name.tls=true"
  - "traefik.http.routers.service-name.tls.certresolver=letsencrypt"
```

#### 3. Authentik Authentication

**Purpose**: Single sign-on for all services

**Access**: https://auth.yourdomain.com or http://localhost:9000

**Initial Setup**:
1. Navigate to Authentik web interface
2. Login with:
   - Email: `akadmin`
   - Password: `change_this_bootstrap_pass_ABC!`
3. **IMMEDIATELY** change the password
4. Create user accounts for family members

**Configuration Steps**:
1. **Create Users**:
   - Go to Directory → Users
   - Click "Create" → "User"
   - Fill in username, email, first/last name
   - Set strong password
   - Add to appropriate groups

2. **Create Groups**:
   - Go to Directory → Groups
   - Create groups like "Media Admins", "Media Users"
   - Assign permissions accordingly

3. **Create Applications**:
   - Go to Applications → Applications
   - Create app for each service (Sonarr, Radarr, etc.)
   - Use Generic OAuth2/OpenID provider

---

### PHASE 2: Media Services

#### 4. Jackett Indexer Configuration

**Purpose**: Aggregates torrent indexers for the Arr services

**Access**: http://localhost:9117

**Initial Setup**:
1. Navigate to Jackett web interface
2. Click "Add indexer"
3. **Public Indexers** (no login required):
   - 1337x
   - RARBG (if available)
   - The Pirate Bay
   - EZTV
   - YTS

4. **Private Indexers** (requires accounts):
   - Create accounts on private trackers
   - Add each indexer with your credentials
   - Test each indexer after adding

**Configuration**:
```yaml
# In Jackett settings
Admin password: set_strong_password_here
Base URL: /jackett (if using reverse proxy)
Server port: 9117
```

**API Key**: Copy this for use in Sonarr/Radarr/Lidarr

#### 5. Deluge Torrent Client

**Purpose**: Downloads torrents found by Jackett

**Access**: http://localhost:8112

**Initial Setup**:
1. Default password: `deluge`
2. **IMMEDIATELY** change password:
   - Preferences → Interface → Password
   - Set strong password

**Detailed Configuration**:

**Downloads Tab**:
```
Download to: /downloads/incomplete
Move completed to: /downloads/complete
Copy of .torrent files to: /downloads/torrents
```

**Network Tab**:
```
Incoming Port: 58846 (or random)
☑ Use Random Port
☑ UPnP
☑ NAT-PMP
```

**Bandwidth Tab**:
```
Max Download Speed: -1 (unlimited)
Max Upload Speed: 1000 (adjust based on your connection)
Max Connections: 200
Max Upload Connections: 50
```

**Interface Tab**:
```
☑ Enable Web Interface
Port: 8112
☑ Enable Password
Password: your_strong_password
```

**Daemon Tab**:
```
☑ Allow Remote Connections
Port: 58846
```

**Plugins Tab**:
Enable these plugins:
- ☑ Label
- ☑ Execute
- ☑ Scheduler

**Labels Setup**:
Create labels for organization:
- sonarr (for TV shows)
- radarr (for movies)
- lidarr (for music)
- manual (for manual downloads)

#### 6. Sonarr TV Show Management

**Purpose**: Automates TV show downloading and organization

**Access**: http://localhost:8989

**Initial Setup Wizard**:
1. **Media Management**:
   ```
   ☑ Rename Episodes
   ☑ Replace Illegal Characters
   Standard Episode Format: {Series Title} - S{season:00}E{episode:00} - {Episode Title} {Quality Full}
   Season Folder Format: Season {season:00}
   Series Folder Format: {Series Title} ({Series Year})
   ```

2. **Profiles**:
   - Keep default profiles or customize
   - Recommended: Create "HD" profile (720p+)

3. **Quality Definitions**:
   - Adjust size limits per quality
   - Recommended: 1GB for 720p, 3GB for 1080p per hour

**Detailed Configuration**:

**Media Management Settings**:
```
Root Folders: /tv

Episode Naming:
☑ Rename Episodes
☑ Replace Illegal Characters
Standard Episode Format: {Series Title} - S{season:00}E{episode:00} - {Episode Title} {Quality Full}
Daily Episode Format: {Series Title} - {Air-Date} - {Episode Title} {Quality Full}
Anime Episode Format: {Series Title} - S{season:00}E{episode:00} - {Episode Title} {Quality Full}
Series Folder Format: {Series Title} ({Series Year})
Season Folder Format: Season {season:00}
Specials Folder Format: Specials

Multi-Episode Style: Range

Folders:
☑ Create empty series folders
☑ Delete empty folders
☑ Skip Free Space Check
☑ Use Hardlinks instead of Copy

Importing:
☑ Enable Completed Download Handling
☑ Remove Completed Downloads
☑ Automatically Reprocess Failed Downloads

File Management:
☑ Ignore Deleted Episodes
☑ Download Propers and Repacks
Propers and Repacks: Do not prefer
```

**Indexer Configuration**:
1. Go to Settings → Indexers
2. Click "+" to add Jackett
3. Select "Torznab"
4. Configure:
   ```
   Name: Jackett
   URL: http://jackett:9117/api/v2.0/indexers/all/results/torznab/
   API Key: [copy from Jackett]
   Categories: 5000,5030,5040 (TV)
   ```

**Download Client Configuration**:
1. Go to Settings → Download Clients
2. Click "+" to add Deluge
3. Configure:
   ```
   Name: Deluge
   Enable: ☑
   Host: deluge
   Port: 8112
   Password: [your deluge password]
   Category: sonarr
   ```

**Quality Profiles**:
Create custom profiles:
1. **HD Profile**:
   - Allowed: HDTV-720p, HDTV-1080p, WEB-720p, WEB-1080p
   - Cutoff: WEB-1080p

2. **SD Profile**:
   - Allowed: SDTV, DVD
   - Cutoff: DVD

#### 7. Radarr Movie Management

**Purpose**: Automates movie downloading and organization

**Access**: http://localhost:7878

**Configuration** (similar to Sonarr):

**Media Management**:
```
Root Folders: /movies

Movie Naming:
☑ Rename Movies
☑ Replace Illegal Characters
Standard Movie Format: {Movie Title} ({Release Year}) {Quality Full}
Movie Folder Format: {Movie Title} ({Release Year})

Folders:
☑ Create empty movie folders
☑ Delete empty folders
☑ Skip Free Space Check
☑ Use Hardlinks instead of Copy

Importing:
☑ Enable Completed Download Handling
☑ Remove Completed Downloads
☑ Automatically Reprocess Failed Downloads
```

**Indexer Configuration**:
```
Name: Jackett
URL: http://jackett:9117/api/v2.0/indexers/all/results/torznab/
API Key: [copy from Jackett]
Categories: 2000,2010,2020,2030,2040,2045,2050,2060 (Movies)
```

**Download Client**:
```
Name: Deluge
Host: deluge
Port: 8112
Password: [your deluge password]
Category: radarr
```

#### 8. Lidarr Music Management

**Purpose**: Automates music downloading and organization

**Access**: http://localhost:8686

**Configuration**:

**Media Management**:
```
Root Folders: /music

Track Naming:
☑ Rename Tracks
☑ Replace Illegal Characters
Standard Track Format: {Artist Name} - {Album Title} - {track:00} - {Track Title}
Artist Folder Format: {Artist Name}
Album Folder Format: {Album Title} ({Release Year})
```

**Indexer Configuration**:
```
Name: Jackett
URL: http://jackett:9117/api/v2.0/indexers/all/results/torznab/
API Key: [copy from Jackett]
Categories: 3000,3010,3020,3030,3040 (Audio)
```

---

### PHASE 3: Media Servers

#### 9. Jellyfin Media Server

**Purpose**: Stream your media collection

**Access**: http://localhost:8096

**Initial Setup**:
1. Create admin account
2. Add media libraries:
   - Movies: /media/movies
   - TV Shows: /media/tv
   - Music: /media/music

**Detailed Configuration**:

**Library Setup**:
1. **Movies Library**:
   ```
   Content Type: Movies
   Folder: /media/movies
   ☑ Enable real-time monitoring
   ☑ Enable chapter image extraction
   ```

2. **TV Shows Library**:
   ```
   Content Type: TV Shows
   Folder: /media/tv
   ☑ Enable real-time monitoring
   ☑ Automatically add to collection
   ```

3. **Music Library**:
   ```
   Content Type: Music
   Folder: /media/music
   ☑ Enable real-time monitoring
   ```

**Server Settings**:
```
Server name: Your Media Server
Cache path: /config/cache
Metadata path: /config/metadata
Log file path: /config/logs

Network:
☑ Enable automatic port mapping
☑ Enable remote access
Base URL: (leave empty for local)
```

**Transcoding Settings**:
```
Transcoding thread count: 0 (auto)
☑ Enable hardware acceleration (if supported)
☑ Enable hardware encoding
```

#### 10. Bazarr Subtitle Management

**Purpose**: Automatically download subtitles

**Access**: http://localhost:6767

**Configuration**:

**Languages**:
1. Add your preferred languages
2. Set default language
3. Configure hearing impaired settings

**Providers**:
Enable subtitle providers:
- OpenSubtitles
- Subscene
- TVSubtitles
- Podnapisi

**Sonarr Integration**:
```
Address: http://sonarr:8989
API Key: [copy from Sonarr]
Base URL: (leave empty)
```

**Radarr Integration**:
```
Address: http://radarr:7878
API Key: [copy from Radarr]
Base URL: (leave empty)
```

---

## Integration Setup

### Connecting Services Together

#### 1. Jackett → Arr Services
- Copy Jackett API key
- Add Jackett as indexer in each Arr service
- Use internal Docker network URLs

#### 2. Deluge → Arr Services
- Configure Deluge as download client
- Set up categories for each service
- Enable completed download handling

#### 3. Arr Services → Jellyfin
- Set up media folders correctly
- Enable real-time monitoring
- Configure library refresh intervals

#### 4. Bazarr → Arr Services
- Connect to Sonarr and Radarr
- Configure subtitle preferences
- Set up automatic subtitle downloading

### API Keys and URLs

**Internal Docker Network URLs**:
```
Jackett: http://jackett:9117
Deluge: http://deluge:8112
Sonarr: http://sonarr:8989
Radarr: http://radarr:7878
Lidarr: http://lidarr:8686
Bazarr: http://bazarr:6767
```

**External Access URLs** (if using Traefik):
```
Sonarr: https://sonarr.yourdomain.com
Radarr: https://radarr.yourdomain.com
Lidarr: https://lidarr.yourdomain.com
Jackett: https://jackett.yourdomain.com
Jellyfin: https://jellyfin.yourdomain.com
```

---

## Troubleshooting

### Common Issues

#### VPN Not Working
```bash
# Check VPN status
docker exec gluetun curl -s https://ipinfo.io/ip

# Check logs
docker logs gluetun

# Restart VPN
docker restart gluetun
```

#### Download Client Issues
```bash
# Check if Deluge is accessible
curl http://localhost:8112

# Check container logs
docker logs deluge

# Verify network connectivity
docker exec sonarr ping deluge
```

#### Indexer Problems
```bash
# Test indexer in Jackett
# Check Jackett logs
docker logs jackett

# Verify indexer is working in Arr services
# Check Arr logs for indexer errors
```

#### Media Not Showing in Jellyfin
```bash
# Check file permissions
ls -la /media/movies/
ls -la /media/tv/

# Force library scan
# In Jellyfin: Dashboard → Libraries → Scan Library
```

### Log Locations
```bash
# View logs for specific service
docker logs [service_name]

# Follow logs in real-time
docker logs -f [service_name]

# View last 100 lines
docker logs --tail 100 [service_name]
```

---

## Maintenance

### Regular Tasks

#### Weekly
- Check VPN status
- Review failed downloads
- Clear completed downloads
- Update indexers if needed

#### Monthly
- Update Docker images
- Clean up old logs
- Review storage usage
- Check for new releases

#### Quarterly
- Full system backup
- Review and update passwords
- Update SSL certificates
- Performance optimization

### Backup Strategy

#### Critical Data to Backup
```bash
# Configuration files
/home/lou/mediastack-new/config/

# Database backups
docker exec postgres pg_dump -U user database > backup.sql

# Media metadata
/home/lou/mediastack-new/config/jellyfin/
```

#### Backup Script
```bash
#!/bin/bash
BACKUP_DIR="/home/lou/backups/mediastack-$(date +%Y%m%d)"
mkdir -p $BACKUP_DIR

# Backup configs
cp -r /home/lou/mediastack-new/config $BACKUP_DIR/

# Backup database
docker exec postgres pg_dump -U user database > $BACKUP_DIR/database.sql

# Compress backup
tar -czf $BACKUP_DIR.tar.gz $BACKUP_DIR/
rm -rf $BACKUP_DIR/
```

### Update Procedure

#### Update Single Service
```bash
# Stop service
docker stop [service_name]

# Pull latest image
docker pull [image_name]

# Start service
docker start [service_name]
```

#### Update All Services
```bash
# In your mediastack directory
docker-compose pull
docker-compose up -d
```

---

## Security Best Practices

### Password Management
- Use unique, strong passwords for all services
- Enable two-factor authentication where available
- Regular password rotation
- Use a password manager

### Network Security
- Keep services on internal network
- Use VPN for all download traffic
- Regular security updates
- Monitor access logs

### Access Control
- Use Authentik for centralized authentication
- Implement role-based access
- Regular user account reviews
- Secure API keys

---

## Performance Optimization

### Storage
- Use SSD for OS and configs
- Use HDD for media storage
- Implement proper file permissions
- Regular disk cleanup

### Network
- Monitor bandwidth usage
- Optimize download speeds
- Use quality profiles appropriately
- Implement proper caching

### System Resources
- Monitor CPU and RAM usage
- Optimize Docker container limits
- Regular system maintenance
- Use hardware acceleration where possible

---

## Useful Commands

### Docker Management
```bash
# View all containers
docker ps -a

# View container logs
docker logs [container_name]

# Restart container
docker restart [container_name]

# Update container
docker-compose pull [service_name]
docker-compose up -d [service_name]

# Clean up unused images
docker system prune -a
```

### System Monitoring
```bash
# Check disk usage
df -h

# Check system resources
htop

# Monitor network
nethogs

# Check VPN status
docker exec gluetun curl -s https://ipinfo.io/ip
```

### File Management
```bash
# Check media permissions
ls -la /media/

# Fix permissions
sudo chown -R 1000:1000 /media/
sudo chmod -R 755 /media/

# Check storage usage
du -sh /media/*
```

---

## Conclusion

This guide provides comprehensive configuration for your entire media stack. Follow the phases in order, take your time with each service, and don't hesitate to check logs when issues arise. The key to a successful setup is patience and methodical configuration.

Remember to:
1. Change all default passwords
2. Set up proper authentication
3. Configure VPN correctly
4. Test each service before moving to the next
5. Set up regular backups
6. Monitor system performance

Your media stack will provide years of automated media management once properly configured!

---

## Quick Reference

### Service Ports
- Gluetun: No web interface
- Traefik: 80, 443, 8080 (dashboard)
- Authentik: 9000, 9443
- Jackett: 9117
- Deluge: 8112
- Sonarr: 8989
- Radarr: 7878
- Lidarr: 8686
- Jellyfin: 8096
- Bazarr: 6767

### Default Passwords
- Deluge: `deluge`
- Authentik: `change_this_bootstrap_pass_ABC!`
- Others: Set during initial setup

### Important URLs
- Jackett Torznab: `http://jackett:9117/api/v2.0/indexers/all/results/torznab/`
- Deluge Daemon: `http://deluge:8112`

### Categories
- Movies: 2000,2010,2020,2030,2040,2045,2050,2060
- TV: 5000,5030,5040
- Music: 3000,3010,3020,3030,3040

---

*Last updated: $(date)*
