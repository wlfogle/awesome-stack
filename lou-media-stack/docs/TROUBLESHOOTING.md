# Troubleshooting Guide - Lou Media Stack

## 🚨 Common Issues and Solutions

### VPN (Gluetun) Issues

#### Issue: Gluetun constantly restarting
**Symptoms:**
- Container exits with error codes
- DNS resolution failures
- Empty SERVER_COUNTRIES variable

**Solutions:**
```bash
# Check VPN configuration
docker logs gluetun --tail 50

# For WireGuard custom provider, remove SERVER_COUNTRIES
# Edit .env file and ensure these settings:
VPN_TYPE=wireguard
VPN_SERVICE_PROVIDER=custom
# Remove or comment out SERVER_COUNTRIES line

# Test VPN connection
docker exec gluetun wget -qO- ifconfig.me
```

#### Issue: Downloads not working through VPN
**Symptoms:**
- Deluge can't connect to trackers
- Timeout errors in download clients

**Solutions:**
```bash
# Verify VPN is working
docker exec gluetun curl -s ifconfig.me

# Check if download client is properly routed
docker exec deluge curl -s ifconfig.me
# Should show same IP as Gluetun

# Restart VPN-dependent services
docker-compose restart gluetun deluge jackett
```

### Database Issues

#### Issue: PostgreSQL connection failures
**Symptoms:**
- Authentik can't connect to database
- Services fail to start with database errors

**Solutions:**
```bash
# Check PostgreSQL status
docker logs postgres --tail 20

# Test database connection
docker exec postgres psql -U postgres -c "SELECT version();"

# Recreate database if corrupted
docker-compose stop postgres
docker volume rm mediastack_postgres_data
docker-compose up -d postgres
```

#### Issue: Valkey/Redis connection issues
**Symptoms:**
- Session data not persisting
- Cache-related errors

**Solutions:**
```bash
# Test Valkey connection
docker exec valkey redis-cli ping

# Check memory usage
docker exec valkey redis-cli info memory

# Clear cache if needed
docker exec valkey redis-cli flushall
```

### Service Configuration Issues

#### Issue: API keys not working
**Symptoms:**
- Services can't communicate
- Authentication failures between services

**Solutions:**
```bash
# Extract API keys from configs
grep -r "ApiKey\|API_KEY\|api_key" extracted-configs/

# Common locations for API keys:
# Sonarr: /config/config.xml
# Radarr: /config/config.xml
# Jackett: /config/Jackett/ServerConfig.json

# Regenerate API keys if needed
docker exec sonarr cat /config/config.xml | grep ApiKey
```

#### Issue: Services can't reach each other
**Symptoms:**
- Network connection refused
- DNS resolution failures

**Solutions:**
```bash
# Check Docker networks
docker network ls
docker network inspect lou-media-stack_default

# Test connectivity between services
docker exec sonarr ping deluge
docker exec radarr ping jackett

# Verify service names in configurations
# Use container names, not localhost
```

### Media Server Issues

#### Issue: Jellyfin/Plex not scanning libraries
**Symptoms:**
- New media not appearing
- Library scan failures

**Solutions:**
```bash
# Check file permissions
ls -la data/media/movies/
ls -la data/media/tv/

# Fix permissions
sudo chown -R 1000:1000 data/media/

# Force library scan
# Jellyfin: Dashboard -> Libraries -> Scan Library
# Plex: Settings -> Library -> Scan Library Files
```

#### Issue: Transcoding failures
**Symptoms:**
- Playback errors
- High CPU usage
- Stuttering video

**Solutions:**
```bash
# Check hardware acceleration
docker exec jellyfin ls /dev/dri/

# For Intel GPU:
# Add to docker-compose.yml:
devices:
  - /dev/dri:/dev/dri

# Check transcoding logs
docker logs jellyfin | grep -i transcode
```

### Download Client Issues

#### Issue: Deluge not receiving downloads
**Symptoms:**
- Sonarr/Radarr show "No download client available"
- Downloads stuck in queue

**Solutions:**
```bash
# Check Deluge daemon status
docker exec deluge deluge-console "info"

# Verify download client configuration:
# Host: deluge (container name)
# Port: 58846 (daemon port)
# Username: (leave empty)
# Password: (check Deluge settings)

# Test connection from Sonarr/Radarr
docker exec sonarr wget -qO- http://deluge:8112
```

#### Issue: Indexers not returning results
**Symptoms:**
- No search results in Sonarr/Radarr
- Jackett tests fail

**Solutions:**
```bash
# Check Jackett logs
docker logs jackett --tail 50

# Test indexers individually in Jackett
# Add FlareSolverr if CloudFlare protection

# Verify indexer URLs in Sonarr/Radarr:
# Format: http://jackett:9117/api/v2.0/indexers/INDEXER_ID/results/torznab/
```

### Monitoring Issues

#### Issue: Prometheus/Grafana not starting
**Symptoms:**
- Monitoring containers restarting
- No metrics data

**Solutions:**
```bash
# Check Prometheus config
docker logs prometheus --tail 20

# Verify config file exists
ls -la appdata/prometheus/

# Check Grafana permissions
sudo chown -R 472:472 appdata/grafana/

# Test Prometheus targets
curl http://localhost:9090/targets
```

### Authentication Issues

#### Issue: Authentik not loading
**Symptoms:**
- Login page not accessible
- SSL certificate errors

**Solutions:**
```bash
# Check Authentik logs
docker logs authentik-server --tail 50

# Verify database connection
docker exec authentik-server python -c "from django.db import connection; connection.cursor()"

# Reset admin password
docker exec authentik-server ak create-admin-group
```

## 🔧 Diagnostic Commands

### System Health Check
```bash
# Check all service status
docker-compose ps

# Check resource usage
docker stats --no-stream

# Check disk space
df -h
du -sh appdata/*/
```

### Network Diagnostics
```bash
# Check network connectivity
docker exec sonarr ping -c 3 deluge
docker exec radarr ping -c 3 jackett
docker exec deluge ping -c 3 google.com

# Check DNS resolution
docker exec sonarr nslookup deluge
docker exec gluetun nslookup google.com
```

### Log Analysis
```bash
# View recent logs for all services
docker-compose logs --tail=50

# Monitor logs in real-time
docker-compose logs -f

# Check specific service logs
docker logs jellyfin --since 1h
docker logs gluetun --since 30m
```

### Configuration Validation
```bash
# Check Docker Compose syntax
docker-compose config

# Validate environment variables
docker-compose config | grep -i "error\|warning"

# Check file permissions
find appdata/ -type f -name "*.xml" -o -name "*.json" | head -10 | xargs ls -la
```

## 📊 Performance Troubleshooting

### High CPU Usage
```bash
# Identify resource-heavy containers
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}"

# Common causes:
# - Transcoding (Jellyfin/Plex)
# - Indexing (Sonarr/Radarr)
# - Extraction (Downloads)
```

### High Memory Usage
```bash
# Check memory usage per container
docker stats --no-stream --format "table {{.Container}}\t{{.MemUsage}}\t{{.MemPerc}}"

# Check for memory leaks
docker exec postgres psql -U postgres -c "SELECT pg_size_pretty(pg_database_size('authentik'));"
```

### Disk Space Issues
```bash
# Check disk usage
du -sh data/media/*
du -sh appdata/*

# Find large files
find data/media/ -type f -size +5G -exec ls -lh {} \;

# Clean up downloads
rm -rf data/torrents/incomplete/*
```

## 🛠️ Recovery Procedures

### Complete Stack Reset
```bash
# Stop all services
docker-compose down

# Remove containers (keeps volumes)
docker-compose down --rmi local

# Start fresh
docker-compose up -d --force-recreate
```

### Database Recovery
```bash
# Backup current database
docker exec postgres pg_dump -U postgres authentik > authentik_backup.sql

# Restore from backup
docker exec -i postgres psql -U postgres authentik < authentik_backup.sql
```

### Configuration Restore
```bash
# Restore from extracted configs
cp extracted-configs/service-configs/mediastack-new/sonarr/config.xml appdata/sonarr/
cp extracted-configs/service-configs/mediastack-new/radarr/config.xml appdata/radarr/

# Fix permissions
sudo chown -R 1000:1000 appdata/
```

## 📞 Getting Help

### Log Collection
```bash
# Collect logs for support
mkdir -p debug-logs
docker-compose logs > debug-logs/all-services.log
docker logs gluetun > debug-logs/gluetun.log
docker logs postgres > debug-logs/postgres.log
```

### System Information
```bash
# Gather system info
docker version > debug-logs/system-info.txt
docker-compose version >> debug-logs/system-info.txt
uname -a >> debug-logs/system-info.txt
df -h >> debug-logs/system-info.txt
```

### Configuration Backup
```bash
# Backup current configuration
tar -czf mediastack-debug-$(date +%Y%m%d).tar.gz \
  docker-compose.yml \
  .env \
  appdata/ \
  debug-logs/
```

## 🔍 Common Error Messages

### "Connection refused"
- Check if target service is running
- Verify network connectivity
- Ensure correct port numbers

### "Permission denied"
- Check file/directory permissions
- Verify PUID/PGID settings
- Run: `sudo chown -R 1000:1000 appdata/`

### "Database connection failed"
- Check PostgreSQL container status
- Verify database credentials
- Check network connectivity

### "VPN connection failed"
- Verify VPN credentials
- Check server availability
- Review firewall settings

Remember: Always check logs first (`docker logs <service>`) and ensure services are started in the correct order (VPN → Database → Services).
