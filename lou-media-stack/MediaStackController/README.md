# MediaStack Controller - Android/Fire TV App

A comprehensive Android and Fire TV application for controlling your MediaStack services including Jellyfin, Plex, Sonarr, Radarr, and more.

## Features

### 📱 Android Features
- **Service Status Monitoring**: Real-time status checking for all your services
- **Media Search**: Search across Sonarr, Radarr, and Lidarr
- **Service Control**: Start, stop, restart services remotely
- **Log Viewing**: View Docker container logs
- **EPG Support**: Electronic Program Guide for TV recording
- **Modern UI**: Material Design with dark theme support

### 📺 Fire TV Features
- **10-foot UI**: Optimized for TV screens and remote control
- **D-pad Navigation**: Full remote control support
- **Service Grid**: Easy-to-navigate service tiles
- **Quick Actions**: Context menu for service operations
- **WebView Integration**: Direct service access within the app

## Installation

### Prerequisites
- Android Studio 4.2 or later
- Android SDK 21 (Android 5.0) or later
- Your MediaStack server running on your network

### Build Instructions

1. **Clone/Import the project**:
   ```bash
   # The project is already set up in /home/lou/MediaStackController
   cd /home/lou/MediaStackController
   ```

2. **Open in Android Studio**:
   - Launch Android Studio
   - Select "Open an existing project"
   - Navigate to `/home/lou/MediaStackController`
   - Let Android Studio sync the project

3. **Configure Server Address**:
   - Edit `app/src/main/java/com/mediastack/controller/viewmodels/MainViewModel.kt`
   - Update the server IP address from `192.168.12.204` to your server's IP
   - Update ports if you've modified them in your docker-compose

4. **Build and Run**:
   - Connect your Android device or Fire TV
   - Click the "Run" button in Android Studio
   - Select your target device

## Configuration

### Server Setup
Your MediaStack server should be accessible at the configured IP address with these default ports:

```
Service         Port    URL
Jellyfin        8096    http://YOUR_SERVER_IP:8096
Plex            32400   http://YOUR_SERVER_IP:32400
Sonarr          8989    http://YOUR_SERVER_IP:8989
Radarr          7878    http://YOUR_SERVER_IP:7878
Lidarr          8686    http://YOUR_SERVER_IP:8686
Jackett         9117    http://YOUR_SERVER_IP:9117
Deluge          8112    http://YOUR_SERVER_IP:8112
Bazarr          6767    http://YOUR_SERVER_IP:6767
Portainer       9000    http://YOUR_SERVER_IP:9000
Authentik       6080    http://YOUR_SERVER_IP:6080
Grafana         3800    http://YOUR_SERVER_IP:3800
Homarr          3200    http://YOUR_SERVER_IP:3200
```

### API Keys
Update the API keys in `MainViewModel.kt`:
- Sonarr API Key: `a0a1421101bb471a8db85f4affeb7410`
- Radarr API Key: `9088fc58d3da47b9b67feac5c83a279b`
- Bazarr API Key: `8c93513725bba49fea8fd0d3685e5ff2`

## Usage

### Android/Mobile Usage
1. **Launch the app** on your Android device
2. **View Services**: The main screen shows all your services with status indicators
3. **Control Services**: Tap on any service to open it or use the action buttons
4. **Search Media**: Use the search FAB to find movies, TV shows, or music
5. **View EPG**: Use the EPG FAB for TV guide and recording management

### Fire TV Usage
1. **Launch from Fire TV home** screen
2. **Navigate with D-pad**: Use arrow keys to move between services
3. **Select services**: Press SELECT/ENTER to open a service
4. **Quick actions**: Press MENU for service control options
5. **WebView**: Services open in full-screen WebView for easy control

## Development

### Project Structure
```
app/src/main/
├── java/com/mediastack/controller/
│   ├── MainActivity.kt              # Main Android activity
│   ├── FireTVActivity.kt           # Fire TV optimized activity
│   ├── models/                     # Data models
│   │   └── MediaService.kt         # Service definitions
│   ├── viewmodels/                 # MVVM ViewModels
│   │   └── MainViewModel.kt        # Main app logic
│   ├── network/                    # Network layer
│   │   └── MediaStackApiService.kt # API communication
│   └── adapters/                   # RecyclerView adapters
├── res/
│   ├── layout/                     # UI layouts
│   │   ├── activity_main.xml       # Mobile layout
│   │   └── activity_fire_tv.xml    # Fire TV layout
│   ├── values/                     # Resources
│   │   ├── strings.xml            # Text strings
│   │   └── colors.xml             # Color palette
│   └── drawable/                   # Icons and graphics
└── AndroidManifest.xml            # App configuration
```

### Extending the App

#### Adding New Services
1. Update `createDefaultServices()` in `MainViewModel.kt`
2. Add service icons to `res/drawable/`
3. Add service strings to `res/values/strings.xml`

#### Adding New Features
1. Create new activities/fragments as needed
2. Update `AndroidManifest.xml` with new activities
3. Add navigation logic to existing activities

## Remote Control API

To enable full remote control capabilities, you'll need to implement a REST API on your server. Here's a sample endpoint structure:

```bash
# Service Control
POST /api/services/{service_id}/restart
POST /api/services/{service_id}/start
POST /api/services/{service_id}/stop

# Logs
GET /api/services/{service_id}/logs?lines=100

# System Stats
GET /api/system/stats

# EPG
GET /api/epg/channels
GET /api/epg/programs/{channel_id}
POST /api/epg/record/{program_id}
```

## Troubleshooting

### Common Issues

1. **Services show as offline**:
   - Check your server IP address configuration
   - Ensure your device is on the same network
   - Verify service ports are correct

2. **Fire TV app won't install**:
   - Enable "Apps from Unknown Sources" in Fire TV settings
   - Use ADB to install: `adb install app-debug.apk`

3. **Remote control not working**:
   - Implement the REST API endpoints on your server
   - Check network connectivity
   - Verify API keys are correct

## Screenshots

### Android Interface
- Service grid with status indicators
- Modern Material Design
- Search and EPG integration

### Fire TV Interface
- 10-foot UI optimized for TV
- D-pad navigation
- Large, readable text and buttons

## License

This project is created for controlling your personal MediaStack deployment. Modify as needed for your specific setup.

## Contributing

Feel free to submit issues and enhancement requests. This app is designed to be easily customizable for different MediaStack configurations.

## Support

For issues related to:
- **MediaStack services**: Check your docker-compose logs
- **Android app**: Review logcat output in Android Studio
- **Fire TV installation**: Ensure developer options are enabled

---

**Note**: This app is designed to work with your existing MediaStack deployment. Make sure all services are properly configured and accessible before using the mobile app.
