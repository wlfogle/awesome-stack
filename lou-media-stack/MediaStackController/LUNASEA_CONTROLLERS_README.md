# LunaSea Media Stack Controllers

This repository contains multiple controllers for managing your media stack using LunaSea modules and API integrations.

## Available Controllers

### 1. Linux Controller (Python)
- **File**: `lunasea_linux_controller.py`
- **Description**: Comprehensive Python-based controller for Linux systems
- **Features**:
  - Service status monitoring
  - API integration with Sonarr, Radarr, Lidarr
  - Interactive mode
  - Dashboard view
  - LunaSea desktop app launcher

### 2. Android Controller (Kotlin)
- **File**: `app/src/main/java/com/mediastack/controller/lunasea/LunaSeaController.kt`
- **Description**: Android integration controller
- **Features**:
  - Integrates with existing Android MediaStackController app
  - Service monitoring through Android UI
  - API integration with media services

### 3. Shell Script Controller
- **File**: `luna_sea_controller.sh`
- **Description**: Basic bash script for service integration
- **Features**:
  - Simple service integration
  - Shell-based automation

### 4. Unified Launcher
- **File**: `launch_controller.sh`
- **Description**: Multi-platform launcher that detects your system and launches the appropriate controller
- **Features**:
  - Automatic platform detection
  - Unified command interface
  - Dependency management

## Quick Start

### Using the Unified Launcher (Recommended)

```bash
# Make the launcher executable
chmod +x launch_controller.sh

# Start interactive mode (auto-detects platform)
./launch_controller.sh

# Show service dashboard
./launch_controller.sh dashboard

# Refresh all services
./launch_controller.sh refresh

# Launch LunaSea desktop app
./launch_controller.sh launch

# Force Linux platform
./launch_controller.sh --platform linux

# Force Android platform
./launch_controller.sh --platform android
```

### Using Linux Controller Directly

```bash
# Make executable
chmod +x lunasea_linux_controller.py

# Interactive mode
./lunasea_linux_controller.py

# Show dashboard
./lunasea_linux_controller.py dashboard

# Refresh services
./lunasea_linux_controller.py refresh

# Launch LunaSea desktop
./lunasea_linux_controller.py launch
```

## Interactive Mode Features

When you run the controller in interactive mode, you'll see:

```
LunaSea Linux Controller - Interactive Mode
==================================================

Options:
1. Refresh all services
2. View service dashboard
3. Check specific service
4. Launch LunaSea desktop
5. Sonarr integration
6. Radarr integration
7. Lidarr integration
8. Exit
```

## Service Configuration

Services are configured in `services_config.json` with the following structure:

```json
{
  "services": [
    {
      "id": "sonarr",
      "name": "Sonarr",
      "description": "TV Series Management",
      "url": "http://192.168.12.204:8989",
      "port": 8989,
      "category": "content_management",
      "type": "TV Series",
      "api_key": "your_api_key_here",
      "has_web_ui": true,
      "supports_api": true,
      "icon": "sonarr"
    }
  ]
}
```

## Currently Supported Services

- **Jellyfin** (Media Server)
- **Plex** (Media Server)
- **Sonarr** (TV Series Management)
- **Radarr** (Movie Management)
- **Lidarr** (Music Management)
- **Bazarr** (Subtitles Management)
- **Jackett** (Indexer Management)
- **Deluge** (Torrent Client)
- **Overseerr** (Request Management for Plex)
- **Jellyseerr** (Request Management for Jellyfin)
- **Tautulli** (Plex Analytics)
- **Portainer** (Docker Management)

## LunaSea Integration

The controllers integrate with LunaSea modules extracted from the official LunaSea source code (`lunasea-11.0.0.tar.gz`). Key integrations include:

- **Sonarr Module**: Series management, system status, API v3
- **Radarr Module**: Movie management, system status, API v3
- **Lidarr Module**: Music management, system status, API v1
- **Tautulli Module**: Plex analytics integration
- **Dashboard Module**: Unified service overview

## Android APK Integration

The Android APK (`lunasea-android.apk`) provides:
- Mobile interface for service management
- Push notifications for service events
- Integration with existing MediaStackController app

## Requirements

### Linux Controller
- Python 3.6+
- `requests` library (usually pre-installed)
- Network access to your media services

### Android Controller
- Android SDK
- Kotlin support
- Existing MediaStackController app

### LunaSea Desktop
- Install LunaSea desktop app from [GitHub releases](https://github.com/JagandeepBrar/lunasea/releases)
- Or use the installed snap: `lunasea`

## API Keys

Make sure to configure your API keys in the service configuration:

- **Sonarr**: Settings → General → API Key
- **Radarr**: Settings → General → API Key
- **Lidarr**: Settings → General → API Key
- **Bazarr**: Settings → General → API Key

## Troubleshooting

### Service Not Responding
```bash
# Check specific service
./lunasea_linux_controller.py
# Then choose option 3 and enter service ID
```

### API Key Issues
- Verify API keys in service web interfaces
- Update `services_config.json` with correct keys
- Check firewall/network connectivity

### Python Dependencies
```bash
# Install requests if missing
python3 -m pip install --user requests
```

### LunaSea Desktop Not Found
```bash
# Install LunaSea snap
sudo snap install lunasea --dangerous /path/to/lunasea-linux-amd64.snap

# Or add to PATH
export PATH="/var/lib/snapd/snap/bin:$PATH"
```

## File Structure

```
MediaStackController/
├── lunasea_linux_controller.py      # Main Linux controller
├── launch_controller.sh             # Unified launcher
├── luna_sea_controller.sh           # Basic shell controller
├── services_config.json             # Service configuration
├── app/src/main/java/com/mediastack/controller/lunasea/
│   └── LunaSeaController.kt         # Android controller
├── lunasea-11.0.0/                  # LunaSea source code
└── LUNASEA_CONTROLLERS_README.md    # This file
```

## Development

To extend the controllers:

1. **Add New Service**: Update `services_config.json` and add integration methods
2. **Add New Platform**: Extend `launch_controller.sh` with new platform detection
3. **Add New Features**: Extend the Python controller with new API integrations

## Contributing

Feel free to submit issues and pull requests to improve the controllers.

## License

This project integrates with LunaSea (MIT License) and your existing MediaStackController project.
