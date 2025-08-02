# Homarr User Dashboard Configuration

## 🏠 **USER DASHBOARD SETUP**

### **Access Homarr**
- URL: http://localhost:8341
- Purpose: Beautiful user-friendly dashboard for daily use

### **Initial Setup Steps:**

1. **First Access**
   - Go to http://localhost:8341
   - Click "Get Started" or the settings icon
   - No authentication required initially (family-friendly)

2. **Dashboard Configuration**

   **Media Services (Primary Focus):**
   - **Jellyfin** → http://localhost:8200 (Main media server)
   - **Plex** → http://localhost:8201 (Alternative media server)
   - **Audiobookshelf** → http://localhost:8210 (Audiobooks)
   - **Calibre Web** → http://localhost:8211 (eBooks)

   **Request & Discovery:**
   - **Overseerr** → http://localhost:8310 (Request movies/TV)
   - **Jellyseerr** → http://localhost:8311 (Jellyfin requests)

   **TV & Live Content:**
   - **Channels DVR** → http://localhost:8220 (Live TV)
   - **TVHeadend** → http://localhost:8320 (TV Backend)

### **Recommended Widget Layout:**

```
🎬 MEDIA CENTERS
┌─────────────────┐ ┌─────────────────┐
│    JELLYFIN     │ │      PLEX       │
│   (Movies/TV)   │ │   (Alternative) │
└─────────────────┘ └─────────────────┘

📚 BOOKS & AUDIO  
┌─────────────────┐ ┌─────────────────┐
│ AUDIOBOOKSHELF  │ │   CALIBRE WEB   │
│   (Audiobooks)  │ │     (eBooks)    │
└─────────────────┘ └─────────────────┘

📡 REQUESTS & TV
┌─────────────────┐ ┌─────────────────┐
│    OVERSEERR    │ │  CHANNELS DVR   │
│   (Requests)    │ │    (Live TV)    │
└─────────────────┘ └─────────────────┘
```

### **Widgets to Enable:**

**System Info Widget:**
- Shows server status
- CPU/Memory usage
- Docker container status

**Weather Widget:**
- Add your location for weather display
- Nice visual element for family dashboard

**Calendar Widget (Optional):**
- Shows upcoming releases
- Can integrate with media services

**RSS/News Widget (Optional):**
- Movie/TV news feeds
- Entertainment updates

### **Family-Friendly Features:**

1. **Simple Layout:** Focus on media consumption
2. **Large Icons:** Easy to identify services
3. **Status Indicators:** Show if services are running
4. **No Admin Tools:** Hide technical/admin functions
5. **Custom Backgrounds:** Add family photos or themes

### **Service Categories:**

```json
{
  "categories": [
    {
      "name": "📺 Watch",
      "services": ["Jellyfin", "Plex", "Channels DVR"]
    },
    {
      "name": "📚 Read/Listen", 
      "services": ["Audiobookshelf", "Calibre Web"]
    },
    {
      "name": "🎯 Request",
      "services": ["Overseerr", "Jellyseerr"]
    }
  ]
}
```

### **Advanced Customization:**
- **Themes:** Light/dark mode toggle
- **Custom CSS:** Brand colors, fonts
- **Service Health:** Automatic status checking
- **Quick Actions:** Direct links to popular content
