# Wayland Troubleshooting Log
**Date:** 2025-06-23T17:54:59Z
**System:** Garuda Linux with KDE Plasma 6.3.2
**Hardware:** Intel i9-13900HX + Intel iGPU + NVIDIA RTX 4080 Laptop GPU

## Problem Description
- Top panel flickering on and off in Wayland
- Random micro-freezes during system operation
- Hybrid GPU setup causing compositor sync issues

## System Information
- Operating System: Garuda Linux 
- KDE Plasma Version: 6.3.2
- KDE Frameworks Version: 6.11.0
- Qt Version: 6.8.2
- Kernel Version: 6.13.5-zen1-1-zen (64-bit)
- Graphics Platform: Wayland
- Processors: 32 × 13th Gen Intel® Core™ i9-13900HX
- Memory: 62.5 GiB of RAM
- Graphics Processor 1: Intel® Graphics
- Graphics Processor 2: NVIDIA GeForce RTX 4080 Laptop GPU
- Manufacturer: OriginPC
- Product Name: EON17-X

## Diagnostic Phase

### Step 1: GPU and Driver Information
**Command:** `lspci | grep -E "(VGA|3D|Display)" && nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv`

**Results:**
- Intel Raptor Lake-S UHD Graphics (rev 04) - Primary display controller
- NVIDIA GeForce RTX 4080 Laptop GPU - Secondary GPU
- NVIDIA Driver Version: 570.124.04
- NVIDIA VRAM: 12282 MiB

**Analysis:** Hybrid GPU setup confirmed. Driver versions appear current.

### Step 2: Wayland Environment Check
**Command:** `echo $XDG_SESSION_TYPE && echo $WAYLAND_DISPLAY && kwin_wayland --version`

**Results:**
- Session Type: wayland
- Wayland Display: wayland-0
- KWin Version: 6.3.2
- Warning: QThreadStorage thread cleanup issues detected

**Analysis:** Running on Wayland, KWin 6.3.2 active, some thread cleanup warnings.

### Step 3: Environment Variables
**Command:** `env | grep -E "(GDK|QT|WAYLAND|NVIDIA|__GL)" | sort`

**Results:**
- MOZ_ENABLE_WAYLAND=1 (Firefox Wayland support)
- QT_AUTO_SCREEN_SCALE_FACTOR=0 (Manual scaling)
- QT_WAYLAND_RECONNECT=1 (Qt Wayland reconnection)
- WAYLAND_DISPLAY=wayland-0

**Analysis:** Missing key NVIDIA-specific environment variables for Wayland.

### Step 4: Journal Error Analysis
**Command:** `journalctl --user -n 100 | grep -i -E "(kwin|wayland|nvidia|composit|freeze|flicker)"`

**Results:**
- DBus integration errors with KWin
- Teardown call handling issues
- Physical device detection: NVIDIA RTX 4080 Laptop GPU recognized

**Analysis:** KWin having DBus communication issues, but GPU detection working.

### Step 5: NVIDIA Wayland Support
**Command:** `cat /proc/driver/nvidia/version && ls -la /usr/lib*/libnvidia-allocator.so*`

**Results:**
- NVIDIA Driver: 570.124.04 (Very recent, good)
- GBM allocator libraries present (Wayland compatibility layer)
- Both 32-bit and 64-bit libraries available

**Analysis:** NVIDIA Wayland support is properly installed with GBM backend.

### Step 6: Display Configuration
**Command:** `kscreen-doctor -o && qdbus org.kde.KWin /KWin org.kde.kwin.Compositing.platformName`

**Results:**
- Display: eDP-2 (Internal laptop display)
- Resolution: 2560x1440@240Hz (High refresh rate display)
- Current mode: 1600x900@60Hz (Lower than native res)
- Scale: 1.15x (Fractional scaling active)
- VRR: Automatic (Variable refresh rate)
- KWin DBus interface unavailable (compositing query failed)

**Analysis:** HIGH REFRESH RATE + FRACTIONAL SCALING + HYBRID GPU = Problem! This combination is known to cause flickering and micro-freezes in Wayland.

## Root Cause Analysis
**Primary Issues Identified:**
1. **Fractional scaling (1.15x)** - Known to cause compositor issues
2. **High refresh rate (240Hz capable)** - Can cause sync problems with hybrid GPU
3. **Hybrid GPU switching** - Intel + NVIDIA coordination issues
4. **Missing NVIDIA environment variables** - Suboptimal GPU utilization
5. **KWin DBus communication errors** - Compositor instability

## Proposed Solutions

### Solution 1: Fix Fractional Scaling
**Approach:** Change from 1.15x to 1.0x (integer scaling)
**Rationale:** Fractional scaling causes significant compositor overhead and sync issues

### Solution 2: Configure NVIDIA Environment Variables
**Approach:** Add proper NVIDIA Wayland environment variables
**Variables to set:**
- `__GLX_VENDOR_LIBRARY_NAME=nvidia`
- `LIBVA_DRIVER_NAME=nvidia`
- `GBM_BACKEND=nvidia-drm`
- `__GL_GSYNC_ALLOWED=1`
- `__GL_VRR_ALLOWED=1`

### Solution 3: Optimize KWin Compositor Settings
**Approach:** Disable problematic compositor features
**Settings:**
- Force compositor to use NVIDIA GPU
- Disable animations for stability
- Set explicit rendering backend

### Solution 4: Display Configuration Optimization
**Approach:** Set optimal display mode and refresh rate
**Target:** Native resolution (2560x1440) at stable refresh rate

### Solution 5: Kernel Parameters for Hybrid GPU
**Approach:** Add kernel parameters for better hybrid GPU handling
**Parameters:** NVIDIA modeset and DRM settings

## Implementation Phase

/run/media/lou/Data/Lou Fogle/errors/wayland_monitor.sh
