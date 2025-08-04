# Vaultwarden Setup Documentation

## Domain Configuration

Setup for Vaultwarden running with Traefik:

- **Domain**: `vaultwarden.lou-fogle-media-stack.duckdns.org`
- **SSL/TLS**: Enabled via Let's Encrypt in Traefik

## Traefik Configuration

Traefik configuration file located at `/etc/traefik/dynamic/vaultwarden.yml`:

```yaml
http:
  routers:
    vaultwarden:
      rule: "Host(`vaultwarden.lou-fogle-media-stack.duckdns.org`)"
      service: vaultwarden
      tls:
        certResolver: letsencrypt
      
  services:
    vaultwarden:
      loadBalancer:
        servers:
          - url: "http://192.168.122.104:8080"
```

## Vaultwarden Service

- **Access URL**: `https://vaultwarden.lou-fogle-media-stack.duckdns.org`
- **Secure Context**: HTTPS provides Subtle Crypto API for cryptographic functions

## Steps to Access

1. Open FireDragon browser.
2. Navigate to `https://vaultwarden.lou-fogle-media-stack.duckdns.org`.
3. Accept any security exceptions if prompted.
4. Log in with your Vaultwarden credentials.

This setup ensures that the WebCrypto API is accessible, resolving previous login and functionality issues associated with non-secure contexts.
