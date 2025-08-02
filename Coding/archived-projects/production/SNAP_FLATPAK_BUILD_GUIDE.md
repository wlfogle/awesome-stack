# Snap & Flatpak Manual Build Guide

## Overview

Due to system constraints during automated building, the snap and flatpak packages could not be automatically generated. However, the source configurations are available and can be built manually on appropriate systems.

## Available Source Packages

### ✅ Snap Source Package
- **File:** `originpc-enhanced-control-5.1.0-snap-source.tar.gz`
- **Contains:** Complete snapcraft.yaml configuration + source files
- **Status:** Ready for manual building

### ✅ Flatpak Source Package  
- **File:** `originpc-enhanced-control-5.1.0-flatpak-source.tar.gz`
- **Contains:** Flatpak manifest files + source files
- **Status:** Ready for manual building (requires dependency fixes)

## Manual Build Instructions

### Building Snap Package

#### Prerequisites
```bash
sudo apt install snapd snapcraft
# OR
sudo dnf install snapd snapcraft
# OR  
sudo pacman -S snapd snapcraft
```

#### Build Steps
```bash
# Extract source package
tar -xzf originpc-enhanced-control-5.1.0-snap-source.tar.gz
cd snap/

# Build the snap package
snapcraft --destructive-mode

# Alternative: Build in clean environment
snapcraft clean
snapcraft
```

#### Expected Output
- `originpc-enhanced-control_5.1.0_amd64.snap`

### Building Flatpak Package

#### Prerequisites
```bash
# Install flatpak-builder and add flathub
sudo apt install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install required runtimes
flatpak install --user flathub org.freedesktop.Platform//23.08
flatpak install --user flathub org.freedesktop.Sdk//23.08
```

#### Build Steps
```bash
# Extract source package
tar -xzf originpc-enhanced-control-5.1.0-flatpak-source.tar.gz
cd flatpak/

# Build the flatpak (use updated manifest)
flatpak-builder --user --install-deps-from=flathub --repo=repo build org.originpc.ControlCenter-updated.yml

# Create .flatpak bundle
flatpak build-bundle repo org.originpc.ControlCenter.flatpak org.originpc.ControlCenter
```

#### Expected Output
- `org.originpc.ControlCenter.flatpak`

## Known Issues & Solutions

### Snap Build Issues

**Issue 1: Native builds not supported**
```
Solution: Use --destructive-mode flag or build in container
```

**Issue 2: Permission errors with gnome extensions**
```
Solution: Use the simplified snapcraft-simple.yaml configuration
```

**Issue 3: Build in distrobox container**
```bash
# If available, use distrobox with Ubuntu/Debian container
distrobox enter ubuntu-packaging
cd /path/to/snap/source
snapcraft
```

### Flatpak Build Issues

**Issue 1: PyQt5 dependency resolution**
```
Error: sip<7,>=6.6.2 not found
Solution: Use system PyQt5 packages instead of pip installation
```

**Issue 2: KDE SDK end-of-life**
```
Warning: org.kde.Sdk is end-of-life
Solution: Use org.freedesktop.Platform runtime (already implemented in updated manifest)
```

**Issue 3: Missing dependencies**
```bash
# Install all required dependencies
flatpak install --user flathub org.freedesktop.Platform.GL.default
flatpak install --user flathub org.freedesktop.Platform.Locale
```

## Alternative Distribution Methods

### Snap Store Submission
1. Build the snap package successfully
2. Create Snap Store developer account
3. Upload via: `snapcraft upload originpc-enhanced-control_5.1.0_amd64.snap`
4. Submit for review

### Flatpak Hub Submission  
1. Build and test the flatpak package
2. Fork https://github.com/flathub/flathub
3. Submit pull request with manifest
4. Follow Flathub review process

### Direct Distribution
Both packages can be distributed directly to users:
- **Snap:** `sudo snap install --dangerous originpc-enhanced-control_5.1.0_amd64.snap`
- **Flatpak:** `flatpak install --user org.originpc.ControlCenter.flatpak`

## Configuration Files Included

### Snap Configuration
- `snapcraft.yaml` - Original configuration  
- `snapcraft-simple.yaml` - Simplified configuration without gnome extensions
- All Python source files included

### Flatpak Configuration
- `org.originpc.ControlCenter.yml` - Original KDE-based manifest
- `org.originpc.ControlCenter-updated.yml` - Updated freedesktop runtime manifest  
- All Python source files included

## Recommendations

### For Immediate Use
1. **AppImage** (already built): Universal compatibility, no packaging system required
2. **DEB/RPM/PKG** (already built): Native package manager support

### For Store Distribution
1. **Snap**: Build manually on Ubuntu system with snapcraft
2. **Flatpak**: Build manually with updated freedesktop runtime

### Build Environment Recommendations
- **Snap**: Ubuntu 20.04+ with snapcraft installed
- **Flatpak**: Any Linux with flatpak-builder and flathub access

## Source Verification

Both source packages contain the **correct enhanced source code v5.1.0**:
- ✅ `enhanced-professional-control-center.py` (139,387 bytes)
- ✅ All AI enhancement modules included
- ✅ All optimization and system intelligence modules
- ✅ Complete build configurations

---

**Status:** Source packages ready for manual building  
**Alternative:** Use AppImage for universal compatibility  
**Next Steps:** Manual build on appropriate development system
