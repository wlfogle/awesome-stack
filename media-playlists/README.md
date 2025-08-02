# Media Playlists

This directory contains IPTV M3U playlists extracted from various sources for use with media streaming applications.

## Files

### `wuffylist3.m3u` (177 KB)
- **Source**: Extracted from 1Pix Media app (via Wuffy Player integration)
- **Content**: 764+ premium US TV channels including:
  - Major networks (ABC, CBS, NBC, FOX)
  - Premium channels (HBO, Showtime, Starz)
  - Cable networks (CNN, ESPN, Discovery, A&E, AMC)
  - Specialty channels (Adult Swim, Animal Planet, BBC America)
- **Format**: Standard M3U8 with EPG metadata
- **Stream URLs**: `http://63.141.251.250/jamzone/isg.php?id=XXXXX&type=stream.m3u8`

### `Custom Playlist.m3u` (101 KB)  
- **Source**: Fire TV (bedroom) - Custom curated channels
- **Content**: Mix of live TV streams and specialty channels
- **Includes**: Louisville local channels, cable networks, international streams

### `Lou.m3u` (192 KB)
- **Source**: Fire TV (bedroom) - Personal playlist
- **Content**: Larger collection with diverse channel lineup
- **Format**: Extended M3U with detailed metadata

## Usage

These playlists are compatible with:
- **Jellyfin** (Media server with live TV support)
- **Plex** (With Plex Pass for live TV)
- **VLC Media Player**
- **Kodi** (with IPTV Simple Client)
- **Any M3U8-compatible player**

## Integration

### For Media Stack:
1. Copy desired M3U files to your media server
2. Configure IPTV/Live TV addon with file path
3. Set up EPG (Electronic Program Guide) if supported

### For Jellyfin:
```bash
# Copy to Jellyfin config directory
cp wuffylist3.m3u /var/lib/jellyfin/config/
# Configure in Jellyfin Dashboard > Live TV > Tuner Devices
```

### For Docker Compose:
```yaml
volumes:
  - ./media-playlists:/config/playlists:ro
```

## Notes

- Stream availability may vary over time
- Some streams may require specific geographic regions
- Always respect content licensing and terms of service
- Extracted on: August 2, 2025

## Sources

- **1Pix Media**: Android app with Wuffy Player integration
- **Wuffy Player**: Media player with built-in channel database
- **Fire TV**: Sideloaded playlist files from bedroom device
