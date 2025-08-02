# Python Projects Collection

This directory contains various Python scripts and projects related to Proxmox, Home Assistant, media stack management, and system utilities.

## Main Scripts

### Media Stack & Container Management
- **`qbt_port_update.py`** - Updates qBittorrent port forwarding from Gluetun container
- **`qbt_port_update_lxc.py`** - LXC container version of the qBittorrent port updater
- **`qcow2_manager.py`** - QCOW2 disk image management utility for Proxmox VMs

### Home Assistant & Alexa Integration
- **`simple_bridge.py`** - Simple Alexa bridge for Home Assistant using Philips Hue emulation
- **`update_token.py`** - Utility to update Home Assistant tokens in bridge configurations

### System Utilities
- **`universal_arch_installer_optimized.py`** - Universal Arch Linux installer with optimizations

## Project Directories

### `calibre-library-fixer/`
Complete Calibre library management and fixing tools with GUI and CLI interfaces.

### `originpc-control/`
OriginPC hardware control utilities including RGB control, hardware optimizations, and system monitoring.

### `pimox/`
Proxmox utilities and conversation logging tools.

### `lou/`
Duplicate directory that contains copies of some projects (should be cleaned up).

## Usage Notes

Most scripts are standalone and can be executed directly. Some require specific environment variables or configuration files:

- Port update scripts need Gluetun and qBittorrent container configurations
- Alexa bridge needs Home Assistant URL and token configuration
- System utilities may require root privileges

## Related Directories

- `/home/lou/awesome_stack/scripts/` - Shell scripts for infrastructure management
- `/home/lou/awesome_stack/Coding/rust-projects/` - Rust implementations
- `/home/lou/awesome_stack/lou-media-stack/` - Main media stack configuration
