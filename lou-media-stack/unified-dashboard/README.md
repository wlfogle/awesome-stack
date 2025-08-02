# 🎬 Grandma's Media Center - AI-Powered Unified Dashboard

A simple, user-friendly interface that integrates with your entire media stack. Perfect for users of all ages!

## ✨ Features

- **🤖 AI-Powered Search**: Natural language search that understands what you want
- **🔍 Predictive Suggestions**: Auto-complete search suggestions based on your history
- **📺 Unified EPG**: Watch and record from TVHeadend, Xteve, and HDHomeRun
- **📥 One-Click Downloads**: Automatically searches and downloads across all your *arr services
- **🎯 Smart Instructions**: Clear, step-by-step guidance on how to watch your content
- **🏠 Simple Interface**: Large buttons and clear text designed for accessibility

## 🚀 Quick Start

1. **Add to your existing docker-compose.yml**:

```yaml
# Add these services to your existing docker-compose.yml
  unified-dashboard:
    image: nginx:alpine
    container_name: mediastack-unified-dashboard
    restart: unless-stopped
    volumes:
      - ./unified-dashboard:/usr/share/nginx/html:ro
      - ./unified-dashboard/nginx.conf:/etc/nginx/conf.d/default.conf:ro
    networks:
      - mediastack
    ports:
      - "8600:80"  # Access at http://localhost:8600
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.unified-dashboard.rule=Host(\`dashboard.\${DOMAIN}\`)"
      - "traefik.http.routers.unified-dashboard.tls=true"
      - "traefik.http.services.unified-dashboard.loadbalancer.server.port=80"

  api-proxy:
    image: node:18-alpine
    container_name: mediastack-api-proxy
    restart: unless-stopped
    working_dir: /app
    volumes:
      - ./unified-dashboard/api-proxy:/app
    networks:
      - mediastack
    ports:
      - "8601:3000"
    environment:
      - NODE_ENV=production
      # Your service URLs (these should work with your existing setup)
      - SONARR_URL=http://mediastack-sonarr:8989
      - RADARR_URL=http://mediastack-radarr:7878
      - LIDARR_URL=http://mediastack-lidarr:8686
      - JACKETT_URL=http://mediastack-jackett:9117
      - TVHEADEND_URL=http://mediastack-tvheadend:9981
      - XTEVE_URL=http://mediastack-xteve:34400
      - JELLYFIN_URL=http://mediastack-jellyfin:8096
      - PLEX_URL=http://mediastack-plex:32400
      # API Keys from your .env file
      - SONARR_API_KEY=\${SONARR_API_KEY}
      - RADARR_API_KEY=\${RADARR_API_KEY}
      - LIDARR_API_KEY=\${LIDARR_API_KEY}
      - JACKETT_API_KEY=\${JACKETT_API_KEY}
      # Optional: OpenAI API key for enhanced AI features
      - OPENAI_API_KEY=\${OPENAI_API_KEY}
    command: sh -c "npm install && npm start"
```

2. **Add API keys to your .env file**:

```bash
# Add these to your existing .env file
SONARR_API_KEY=your_sonarr_api_key_here
RADARR_API_KEY=your_radarr_api_key_here
LIDARR_API_KEY=your_lidarr_api_key_here
JACKETT_API_KEY=your_jackett_api_key_here

# Optional: For enhanced AI features
OPENAI_API_KEY=your_openai_api_key_here
```

3. **Start the services**:

```bash
cd /home/lou/lou-media-stack
docker-compose -f docker-compose-unified.yml up unified-dashboard api-proxy -d
```

## 🎯 How to Use

### For Grandma (or anyone who wants it simple):

1. **Open your web browser** and go to: `http://localhost:8600`

2. **Type what you want to watch** in the big search box:
   - "funny movies from the 90s"
   - "cooking shows"
   - "batman movies"
   - "classical music"

3. **The AI will understand** what you mean and show you options

4. **Click "Download This"** on anything you like

5. **Follow the instructions** it gives you on where to watch it

### TV Guide & Recording:

1. **Scroll down to "TV Guide"** to see what's on now
2. **Click "Watch Now"** to watch live TV
3. **Click "Record"** to record a show
4. **Click "Schedule"** to set up future recordings

## 🤖 AI Features

### Smart Search Understanding:
- "scary movies" → Automatically searches horror films
- "shows like friends" → Finds similar sitcoms  
- "new releases 2024" → Shows recent content
- "disney movies for kids" → Family-friendly content

### Predictive Suggestions:
- Types ahead as you search
- Learns from your preferences
- Suggests alternatives if nothing found
- Fixes common typos automatically

### Natural Language:
- "I want something funny to watch tonight"
- "Find me action movies with explosions"
- "Show me documentaries about space"

## 🔧 Configuration

### API Keys Setup:

1. **Find your API keys**:
   - **Sonarr**: Settings → General → API Key
   - **Radarr**: Settings → General → API Key  
   - **Lidarr**: Settings → General → API Key
   - **Jackett**: Dashboard → Copy API Key

2. **Add them to your .env file** (see Quick Start above)

### Service URLs:
The default URLs should work with your existing docker-compose setup. If you have custom service names, update them in the docker-compose.yml.

### Optional OpenAI Integration:
For enhanced AI features, get an API key from OpenAI and add it to your .env file. The system works fine without it using built-in AI.

## 🌟 What Makes It Special

- **No Technical Knowledge Required**: Just type and click
- **Learns Your Preferences**: Gets smarter as you use it
- **Unified Interface**: Everything in one place
- **Accessible Design**: Large text, clear buttons, simple layout
- **Instant Results**: AI-powered search finds what you want fast
- **Automatic Downloads**: Handles all the technical stuff behind the scenes

## 🏠 Perfect for:

- **Grandparents**: Simple, clear interface
- **Family Use**: Everyone can find what they want
- **Non-Technical Users**: No need to understand Sonarr, Radarr, etc.
- **Quick Access**: One place for all your media needs

## 📱 Access Points:

- **Main Dashboard**: `http://localhost:8600`
- **Direct to Jellyfin**: `http://localhost:8200`  
- **Direct to Plex**: `http://localhost:8201`
- **TV Guide**: Built into the dashboard
- **HDHomeRun**: Auto-detected and integrated

## 🎉 That's It!

Just open the dashboard and start typing what you want to watch. The AI will take care of the rest!

---

*Built with ❤️ for easy media consumption*
