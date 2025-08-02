# IPTV Proxy Setup Guide

*Source: `/home/lou/Downloads/iptv-proxy-3.7.2/README.md`*

## Description

Iptv-Proxy is a project to proxyfie an m3u file and to proxyfie an Xtream iptv service (client API).

### M3U and M3U8

M3U service convert an iptv m3u file into a web proxy server. It's transform all the original tracks to an new url pointing on the proxy.

### Xtream code client api

proxy on Xtream code (client API)
support live, vod, series and full epg 🚀

### M3u Example

Original iptv m3u file

```m3u
#EXTM3U
#EXTINF:-1 tvg-ID="examplechanel1.com" tvg-name="chanel1" tvg-logo="http://ch.xyz/logo1.png" group-title="USA HD",CHANEL1-HD
http://iptvexample.net:1234/12/test/1
#EXTINF:-1 tvg-ID="examplechanel2.com" tvg-name="chanel2" tvg-logo="http://ch.xyz/logo2.png" group-title="USA HD",CHANEL2-HD
http://iptvexample.net:1234/13/test/2
```

What M3U proxy IPTV do:
- convert chanels url to new endpoints
- convert original m3u file with new routes pointing to the proxy

### Start proxy server example

```bash
iptv-proxy --m3u-url http://example.com/get.php?username=user\&password=pass\&type=m3u_plus\&output=m3u8 \\
             --port 8080 \\
             --hostname proxyexample.com \\
             --user test \\
             --password passwordtest
```

This gives you an m3u file on endpoint:
`http://proxyserver.com:8080/iptv.m3u?username=test\&password=passwordtest`

### With Docker

```yaml
version: "3"
services:
  iptv-proxy:
    image: ghcr.io/gibby/iptv-proxy:latest
    container_name: "iptv-proxy"
    restart: unless-stopped
    ports:
      - "8080:8080"
    environment:
      M3U_URL: /root/iptv/iptv.m3u
      PORT: 8080
      HOSTNAME: localhost
      GIN_MODE: release
      # Xtream-code proxy configuration
      XTREAM_USER: xtream_user
      XTREAM_PASSWORD: xtream_password
      XTREAM_BASE_URL: "http://example.com:1234"
      USER: test
      PASSWORD: testpassword
    volumes:
      - ./iptv:/root/iptv
```

### TLS with Traefik Integration

```yaml
services:
  iptv-proxy:
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.iptv-proxy.rule=Host(`iptv.yourdomain.com`)"
      - "traefik.http.routers.iptv-proxy.entrypoints=websecure"
      - "traefik.http.routers.iptv-proxy.tls.certresolver=letsencrypt"
      - "traefik.http.services.iptv-proxy.loadbalancer.server.port=8080"
    environment:
      HTTPS: 1
      ADVERTISED_PORT: 443
      HOSTNAME: iptv.yourdomain.com
```

## Integration with MediaStack

This IPTV proxy can be integrated into your media stack to:
- Provide unified IPTV streaming endpoints
- Proxy external IPTV services through your infrastructure
- Enable authentication and access control for IPTV streams
- Support both M3U playlists and Xtream API services

