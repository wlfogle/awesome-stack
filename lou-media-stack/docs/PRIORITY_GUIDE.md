# MediaStack Priority Configuration Guide

## 🚀 Critical Setup Order

This guide prioritizes services by configuration order - what needs to be set up first for your MediaStack to function properly.

### Phase 1: Core Infrastructure (Configure First)

#### 1. Gluetun VPN (CRITICAL - Configure First)
**Why First:** All download traffic routes through this VPN container.

**Setup:**
```bash
# Test VPN connection
docker exec gluetun wget -qO- ifconfig.me
# Should show VPN IP, not your real IP
```

**Configuration:**
- Update `.env` with VPN credentials
- For WireGuard: Set `VPN_TYPE=wireguard`
- For custom providers: Remove `SERVER_COUNTRIES` if empty

#### 2. Traefik Reverse Proxy
**Why Second:** Handles all web access and SSL certificates.

**Setup:**
```bash
# Create Traefik directories
mkdir -p appdata/traefik/{data,logs}
touch appdata/traefik/data/acme.json
chmod 600 appdata/traefik/data/acme.json
```

#### 3. Authentik Authentication
**Why Third:** Provides SSO and security for all services.

**Setup:**
```bash
# Generate secrets
echo "AUTHENTIK_SECRET_KEY=$(openssl rand -base64 32)"
echo "AUTHENTIK_BOOTSTRAP_PASSWORD=$(openssl rand -base64 16)"
```

#### 4. PostgreSQL & Valkey
**Database backends for Authentik and caching.**

**Verification:**
```bash
# Check PostgreSQL
docker exec postgres psql -U postgres -c "SELECT version();"

# Check Valkey
docker exec valkey redis-cli ping
```

### Phase 2: Essential Media Services

#### 5. Jackett (Indexer Manager) - PRIORITY
**Why Critical:** Provides torrent indexers to Sonarr/Radarr/Lidarr.

**Configuration:**
1. Access: `http://localhost:9117`
2. Add public indexers: 1337x, RARBG, ThePirateBay
3. Add private trackers with credentials
4. Note API Key for integration

#### 6. Deluge (Download Client)
**Main torrent client for downloads.**

**Configuration:**
1. Access: `http://localhost:8112`
2. Default password: `deluge`
3. Set download paths:
   - Download to: `/data/torrents/incomplete`
   - Move completed to: `/data/torrents/complete`

#### 7. Sonarr/Radarr/Lidarr (Media Management)
**Automated media acquisition.**

**Configuration Order:**
1. **Sonarr (TV Shows):**
   - Root folder: `/data/media/tv`
   - Connect to Jackett indexers
   - Add Deluge as download client

2. **Radarr (Movies):**
   - Root folder: `/data/media/movies`
   - Same indexers and download client

3. **Lidarr (Music):**
   - Root folder: `/data/media/music`
   - Configure for lossless quality

### Phase 3: Media Servers & Enhancement

#### 8. Jellyfin/Plex (Media Servers)
**Primary streaming services.**

**Jellyfin Setup:**
1. Access: `http://localhost:8096`
2. Add media libraries:
   - Movies: `/data/media/movies`
   - TV Shows: `/data/media/tv`
   - Music: `/data/media/music`

**Plex Setup:**
1. Access: `http://localhost:32400`
2. Claim server with Plex account
3. Add same media libraries

#### 9. Bazarr (Subtitles)
**Automated subtitle downloading.**

**Configuration:**
1. Access: `http://localhost:6767`
2. Connect to Sonarr/Radarr with API keys
3. Add subtitle providers:
   - OpenSubtitles
   - Subscene
   - Addic7ed

### Phase 4: Monitoring & Dashboards

#### 10. Portainer (Docker Management)
**Container management interface.**

**Setup:**
1. Access: `http://localhost:9000`
2. Create admin account
3. Monitor container health

#### 11. Grafana/Prometheus (Monitoring)
**System metrics and visualization.**

**Configuration:**
1. Grafana: `http://localhost:3000`
2. Add Prometheus data source
3. Import Docker dashboards

#### 12. Heimdall/Homepage (Dashboards)
**Service access dashboards.**

**Setup:**
- Add tiles for each service
- Configure API integrations
- Set up service monitoring

## 🔧 Quick Configuration Commands

### Start Services in Order
```bash
# Phase 1: Infrastructure
docker-compose up -d postgres valkey gluetun traefik authentik

# Wait for initialization
sleep 30

# Phase 2: Media Services
docker-compose up -d jackett deluge sonarr radarr lidarr

# Phase 3: Media Servers
docker-compose up -d jellyfin plex bazarr

# Phase 4: Everything else
docker-compose up -d --remove-orphans
```

### Verify Each Phase
```bash
# Check VPN
docker exec gluetun curl -s ifconfig.me

# Check databases
docker exec postgres psql -U postgres -c "SELECT version();"
docker exec valkey redis-cli ping

# Check API access
curl -s "http://localhost:7878/api/v3/system/status"
curl -s "http://localhost:8989/api/v3/system/status"
```

## 🚨 Common Issues by Phase

### Phase 1 Issues
- **Gluetun not connecting:** Check VPN credentials
- **Traefik SSL errors:** Verify domain configuration
- **Authentik database errors:** Ensure PostgreSQL is running

### Phase 2 Issues
- **Jackett no results:** Check indexer configuration
- **Deluge not downloading:** Verify VPN connection
- **Arr services not finding media:** Check folder permissions

### Phase 3 Issues
- **Jellyfin/Plex not scanning:** Check media folder paths
- **Bazarr no subtitles:** Verify API connections

## 📋 Pre-Flight Checklist

Before starting:
- [ ] VPN credentials configured
- [ ] Directory structure created
- [ ] Environment variables set
- [ ] Docker daemon running
- [ ] Network connectivity verified

## 🔗 Service Dependencies

```
PostgreSQL + Valkey
    ↓
Authentik
    ↓
Gluetun → Traefik
    ↓
Jackett → Deluge
    ↓
Sonarr/Radarr/Lidarr
    ↓
Jellyfin/Plex
    ↓
Bazarr
    ↓
Monitoring Services
```

## 🎯 Success Criteria

Each phase should achieve:
- **Phase 1:** All containers healthy, VPN working
- **Phase 2:** Downloads working, indexers responding
- **Phase 3:** Media servers accessible, subtitles downloading
- **Phase 4:** Full monitoring and dashboard access

Follow this order to ensure smooth deployment and avoid configuration conflicts!
