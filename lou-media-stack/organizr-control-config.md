# Organizr Control Dashboard Configuration

## 🎛️ **ADMIN CONTROL DASHBOARD SETUP**

### **Access Organizr**
- URL: http://localhost:8340
- Purpose: Admin control center with embedded services

### **Initial Setup Steps:**

1. **First Time Setup**
   - Go to http://localhost:8340
   - Click "Setup" 
   - Choose Admin username/password
   - Select SQLite database (simpler for single admin)

2. **Tab Configuration for Control Dashboard**
   
   **System Monitoring Tabs:**
   - **Prometheus** → http://localhost:8400
   - **Portainer** → http://localhost:8500
   - **Traefik** → http://localhost:8000
   
   **Service Management:**
   - **Authentik** → http://localhost:8030
   - **Vaultwarden** → http://localhost:8550
   - **Guacamole** → http://localhost:8510
   
   **Media Control:**
   - **Sonarr** → http://localhost:8110
   - **Radarr** → http://localhost:8111 
   - **Lidarr** → http://localhost:8112
   - **Jackett** → http://localhost:8100
   - **Deluge** → Via Gluetun (use VPN proxy)
   
   **Analytics & Logs:**
   - **Tautulli** → http://localhost:8330
   - **Bazarr** → http://localhost:8300

3. **Advanced Configuration**
   
   **Theme:** Dark theme recommended for admin use
   **Layout:** Horizontal tabs with sidebar for categories
   **Auth:** Enable admin authentication
   **iFrame:** Enable for embedding services

### **Recommended Tab Organization:**

```
🔧 SYSTEM CONTROL
├── Prometheus (Monitoring)
├── Portainer (Containers) 
├── Traefik (Proxy)
└── Authentik (Auth)

🎬 MEDIA MANAGEMENT  
├── Sonarr (TV)
├── Radarr (Movies)
├── Lidarr (Music)
├── Jackett (Indexers)
└── Deluge (Downloads)

📊 ANALYTICS
├── Tautulli (Usage)
├── Bazarr (Subtitles)
└── Gaps (Collection)

🛡️ SECURITY & TOOLS
├── Vaultwarden (Passwords)
├── Guacamole (Remote)
└── Chromium (Browser)
```

### **Security Settings:**
- Enable HTTPS redirect through Traefik
- Set admin-only access for control functions
- Configure session timeouts appropriately
