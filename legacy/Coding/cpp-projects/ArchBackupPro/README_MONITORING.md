# ArchBackupPro Real-time Monitoring Daemon

A comprehensive real-time monitoring system for ArchBackupPro that continuously watches for system changes and provides intelligent backup suggestions.

## Features

### Monitoring Capabilities

1. **Package Management Monitoring**
   - Tracks changes to installed packages using SHA256 hashing
   - Detects recent package installations
   - Logs package list changes

2. **Configuration File Monitoring**
   - Monitors `/etc` and `~/.config` directories
   - Detects configuration changes within 10-minute windows
   - Logs modified configuration files

3. **System Resource Monitoring**
   - CPU usage monitoring (warns if >80%)
   - Memory usage monitoring (warns if >80%)
   - Disk usage monitoring (warns if >80%)

4. **System Service Monitoring**
   - Monitors systemd service failures
   - Reports failed services with details

5. **Backup Suggestions**
   - Tracks backup history
   - Suggests backups if >7 days since last backup
   - Provides intelligent backup timing recommendations

## Implementation

### C++ Daemon (Recommended)

The modern implementation uses C++ for better performance and system integration:

- **Files**: `monitoringclass.h/cpp`, `monitoring_daemon.cpp`
- **Build System**: CMake
- **Dependencies**: C++17, filesystem, threads
- **Features**: Proper daemonization, signal handling, PID management

### Bash Script (Legacy)

A simple bash script implementation for quick setup:

- **File**: `archbackuppro-monitor`
- **Features**: Basic monitoring with shell commands

## Installation

### C++ Daemon Installation

```bash
# Install dependencies (Arch Linux)
sudo pacman -S cmake gcc

# Build and install
sudo ./install-cpp-monitor.sh
```

### Manual Build

```bash
cd src
mkdir build-monitoring
cd build-monitoring
cp ../CMakeLists_monitoring.txt ./CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release .
ninja  # or make if available

# Install manually
sudo cp bin/archbackuppro-monitoring-daemon /usr/local/bin/
sudo cp ../archbackuppro-monitoring-daemon.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable archbackuppro-monitoring-daemon
sudo systemctl start archbackuppro-monitoring-daemon
```

### Bash Script Installation

```bash
sudo ./install-monitor.sh
```

## Usage

### Systemd Service Commands

```bash
# Check status
systemctl status archbackuppro-monitoring-daemon

# View real-time logs
journalctl -u archbackuppro-monitoring-daemon -f

# View monitoring log file
tail -f /var/log/archbackuppro/monitor.log

# Start/stop/restart
systemctl start archbackuppro-monitoring-daemon
systemctl stop archbackuppro-monitoring-daemon
systemctl restart archbackuppro-monitoring-daemon

# Enable/disable auto-start
systemctl enable archbackuppro-monitoring-daemon
systemctl disable archbackuppro-monitoring-daemon
```

### Direct Execution

```bash
# Run in foreground (for testing)
/usr/local/bin/archbackuppro-monitoring-daemon --foreground

# Run as daemon
/usr/local/bin/archbackuppro-monitoring-daemon --daemon

# Show help
/usr/local/bin/archbackuppro-monitoring-daemon --help

# Show version
/usr/local/bin/archbackuppro-monitoring-daemon --version
```

## Configuration

### Monitoring Intervals

- **Main Loop**: 5 minutes (300 seconds)
- **Configuration Changes**: 10-minute detection window
- **Resource Thresholds**: CPU/Memory >80%, Disk >80%

### File Locations

- **Log File**: `/var/log/archbackuppro/monitor.log`
- **Data Directory**: `/var/lib/archbackuppro/`
- **PID File**: `/run/archbackuppro/monitor.pid`
- **Service File**: `/etc/systemd/system/archbackuppro-monitoring-daemon.service`

### Security Features

- **systemd Hardening**: PrivateTmp, ProtectHome, ProtectSystem
- **Resource Limits**: 256MB memory limit, 25% CPU quota
- **Minimal Privileges**: Read-only access except for log/data directories

## Log Output Examples

```
[2024-06-25 21:15:00] INFO: ArchBackupPro monitoring daemon started (PID: 12345)
[2024-06-25 21:15:00] INFO: Logging to /var/log/archbackuppro/monitor.log
[2024-06-25 21:15:00] INFO: Data directory: /var/lib/archbackuppro
[2024-06-25 21:20:00] CHANGE: Package list has changed
[2024-06-25 21:20:00] INFO: Recent package installation detected
[2024-06-25 21:25:00] CHANGE: 3 configuration files modified in /etc
[2024-06-25 21:30:00] WARNING: High CPU usage: 85.2%
[2024-06-25 21:35:00] SUGGESTION: Last backup was 8 days ago, consider running a backup
```

## Architecture

### C++ Class Structure

```cpp
class MonitoringClass {
private:
    void monitorPackages();      // Package change detection
    void monitorConfigs();       // Configuration monitoring
    void monitorResources();     // System resource monitoring
    void monitorServices();      // systemd service monitoring
    void checkBackupSuggestions(); // Backup timing suggestions
    void logMessage();           // Centralized logging
};
```

### Daemon Features

- **Double-fork daemonization** for proper background execution
- **Signal handling** for graceful shutdown (SIGTERM, SIGINT, SIGHUP)
- **PID file management** to prevent multiple instances
- **Automatic directory creation** for logs and data
- **Exception handling** with proper cleanup

## Troubleshooting

### Common Issues

1. **Permission Denied**: Ensure running as root or with appropriate sudo privileges
2. **Service Won't Start**: Check systemd journal for detailed error messages
3. **High Resource Usage**: Monitor logs for excessive warning messages

### Debug Commands

```bash
# Check service status in detail
systemctl status archbackuppro-monitoring-daemon -l

# View recent logs
journalctl -u archbackuppro-monitoring-daemon --since "1 hour ago"

# Test daemon in foreground
sudo /usr/local/bin/archbackuppro-monitoring-daemon --foreground

# Check file permissions
ls -la /var/log/archbackuppro/
ls -la /var/lib/archbackuppro/
```

## Development

### Adding New Monitoring Features

1. Add new monitoring method to `MonitoringClass`
2. Call method in main monitoring loop
3. Update CMakeLists.txt if needed
4. Rebuild and test

### Code Structure

- **Header**: `monitoringclass.h` - Class definition
- **Implementation**: `monitoringclass.cpp` - Core monitoring logic
- **Daemon**: `monitoring_daemon.cpp` - Main executable with daemonization
- **Build**: `CMakeLists_monitoring.txt` - Build configuration

## Performance

- **Memory Usage**: <50MB typical, 256MB limit
- **CPU Usage**: <5% typical, 25% limit
- **Disk I/O**: Minimal, mostly log writes
- **Network**: None (local monitoring only)

## Future Enhancements

- Integration with ArchBackupPro GUI for real-time status
- Email/notification system for critical events
- Web dashboard for remote monitoring
- Machine learning for intelligent backup scheduling
- Integration with cloud backup services
