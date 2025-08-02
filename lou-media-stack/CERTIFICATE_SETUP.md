# Self-Signed Certificate Setup for MediaStack

Your MediaStack now uses a self-signed certificate for HTTPS connections. Here's how to access your services:

## 🌟 Quick Access
**Weather Station**: https://weather.mediastack.local

## 🔒 Accepting the Certificate

### Method 1: Accept in Browser (Recommended)
1. Go to `https://weather.mediastack.local` in your browser
2. You'll see a security warning about an "untrusted certificate"
3. Click "Advanced" or "Show Advanced"
4. Click "Accept Risk and Continue" or "Proceed to weather.mediastack.local (unsafe)"
5. The certificate will be remembered for this domain

### Method 2: Import Certificate into Browser
1. Go to your browser settings → Security → Manage Certificates
2. Import the certificate file: `/home/lou/lou-media-stack/certs/mediastack.crt`
3. Mark it as trusted for websites

### Method 3: Add Certificate to System (Firefox users)
Firefox uses its own certificate store, so you may need to:
1. Type `about:config` in Firefox
2. Set `security.tls.insecure_fallback_hosts` to `weather.mediastack.local`

## 🔧 Certificate Details
- **Certificate File**: `/home/lou/lou-media-stack/certs/mediastack.crt`
- **Valid For**: All *.mediastack.local domains
- **Expires**: 365 days from creation
- **Type**: Self-signed RSA 2048-bit

## 🚀 What's Working
- ✅ HTTPS encryption is active
- ✅ Automatic HTTP → HTTPS redirect
- ✅ Weather station serving live data
- ✅ All mediastack.local domains covered

## 📍 All Available Services
Once you accept the certificate, all these services will work with HTTPS:
- https://weather.mediastack.local - Weather Station
- https://dashboard.mediastack.local - Main Dashboard
- https://jellyfin.mediastack.local - Jellyfin Media Server
- https://plex.mediastack.local - Plex Media Server
- https://sonarr.mediastack.local - TV Show Management
- https://radarr.mediastack.local - Movie Management
- https://lidarr.mediastack.local - Music Management
- https://bazarr.mediastack.local - Subtitle Management
- https://overseerr.mediastack.local - Request Management
- https://tautulli.mediastack.local - Plex Analytics
- And many more...

## 🔄 Certificate Renewal
The certificate is valid for 1 year. When it expires, regenerate it with:
```bash
cd /home/lou/lou-media-stack
openssl req -new -x509 -key certs/mediastack.key -out certs/mediastack.crt -days 365 -config certs/mediastack.conf -extensions v3_req
```

## 🆘 Troubleshooting
If you're still seeing certificate errors:
1. Clear your browser cache and cookies
2. Restart your browser completely
3. Try in an incognito/private window
4. Use the direct IP if needed: https://127.0.0.1 (with host header issues)

Your weather station is now fully functional with secure HTTPS! 🎉
