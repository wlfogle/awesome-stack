# Media Stack Snapshot: mediastack_20250719_183439

Created: Sat Jul 19 06:34:40 PM EDT 2025
Directory: /home/lou/lou-media-stack

## Contents

- **compose/**: Docker Compose files
- **configs/**: Environment and configuration files
- **docker_state/**: Container state information
- **volumes/**: Volume information and metadata
- **networks/**: Network configuration
- **volume_backups/**: Critical volume data backups
- **restore.sh**: Restoration script
- **backup_volumes.sh**: Manual volume backup script

## Restore Instructions

1. Run the restore script: `./restore.sh`
2. Or manually copy files and restart services

## Manual Volume Backup

To backup volume data: `./backup_volumes.sh "/home/lou/lou-media-stack/snapshots/mediastack_20250719_183439"`

## Container State at Snapshot

```
NAMES                                 IMAGE                                      STATUS
mediastack-deluge                     linuxserver/deluge                         Up 20 minutes
mediastack-tvheadend                  linuxserver/tvheadend                      Up 20 minutes
mediastack-traefik                    traefik:v3.0                               Up 20 minutes
mediastack-wireguard                  linuxserver/wireguard                      Up 20 minutes
mediastack-gluetun                    qmcgaw/gluetun                             Restarting (1) 48 seconds ago
mediastack-weather-dashboard          node:18-alpine                             Up 20 minutes
mediastack-weather-web                nginx:alpine                               Up 5 hours
mediastack-weather                    felddy/weewx:5                             Up 5 hours
mediastack-jellyfin                   jellyfin/jellyfin                          Up 26 hours (healthy)
mediastack-plex                       plexinc/pms-docker                         Up 26 hours (healthy)
mediastack-radarr                     linuxserver/radarr                         Up 26 hours
mediastack-lidarr                     linuxserver/lidarr                         Up 26 hours
mediastack-bazarr                     linuxserver/bazarr                         Up 26 hours
mediastack-filebot                    rednoah/filebot:latest                     Restarting (1) 32 seconds ago
mediastack-heimdall                   linuxserver/heimdall                       Up 26 hours
mediastack-sonarr                     linuxserver/sonarr                         Up 26 hours
mediastack-jackett                    linuxserver/jackett                        Up 26 hours
mediastack-overseerr                  sctx/overseerr:latest                      Up 26 hours
mediastack-recyclarr                  recyclarr/recyclarr:latest                 Up 26 hours
mediastack-portainer                  portainer/portainer-ce:latest              Up 26 hours
mediastack-vaultwarden                vaultwarden/server:latest                  Up 26 hours (healthy)
mediastack-postgres                   postgres:15                                Up 26 hours (healthy)
mediastack-tautulli                   linuxserver/tautulli                       Up 26 hours
mediastack-jellyseerr                 fallenbagel/jellyseerr:latest              Up 26 hours
mediastack-flaresolverr               flaresolverr/flaresolverr:latest           Up 26 hours
mediastack-redis                      redis:alpine                               Up 26 hours
mediastack-unpackerr                  golift/unpackerr                           Up 26 hours
mediastack-watchtower                 containrrr/watchtower                      Up 26 hours (healthy)
mediastack_authentik_server           ghcr.io/goauthentik/server:latest          Up 2 days (healthy)
mediastack_authentik_worker           ghcr.io/goauthentik/server:latest          Up 2 days (healthy)
mediastack_ai_recommendation_engine   python:3.11-slim                           Restarting (1) 30 seconds ago
mediastack_ai_storage_optimizer       python:3.11-slim                           Restarting (1) 31 seconds ago
mediastack_traefik_certs_dumper       ldez/traefik-certs-dumper:latest           Up 2 days
mediastack_valkey_exporter            oliver006/redis_exporter:latest            Up 2 days (unhealthy)
mediastack_flaresolverr               ghcr.io/flaresolverr/flaresolverr:latest   Up 2 days
mediastack_epgstation                 yasuoza/epgstation:2.6.9                   Restarting (1) 8 seconds ago
mediastack_headplane                  ghcr.io/tale/headplane:latest              Restarting (1) 21 seconds ago
mediastack_postgres                   postgres:16-alpine                         Up 2 days (healthy)
mediastack_headscale                  headscale/headscale:latest                 Restarting (1) 7 seconds ago
mediastack_valkey                     valkey/valkey:7-alpine                     Up 2 days (healthy)
mediastack_ddns_updater               qmcgaw/ddns-updater:latest                 Restarting (1) 48 seconds ago
mediastack_janitorr                   ghcr.io/schaka/janitorr:latest             Restarting (1) 11 seconds ago
```
