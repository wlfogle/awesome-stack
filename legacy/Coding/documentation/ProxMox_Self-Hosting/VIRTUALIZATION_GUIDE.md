# 🚀 Virtualization & Containerization Setup Guide

Your Garuda Linux system has been optimized for virtualization, containerization, and remote system access. This guide covers all installed tools and their usage.

## 📊 System Overview

- **Hardware**: 32 CPU cores, 62GB RAM, VT-x enabled
- **Host OS**: Garuda Linux 
- **Host IP**: 192.168.12.172
- **Proxmox VE**: 192.168.0.4

## 🛠️ Installed Tools

### Virtualization
- **QEMU/KVM**: Full virtualization with hardware acceleration
- **libvirt**: VM management framework
- **virt-manager**: GUI for VM management
- **quickemu**: Easy VM creation for various OSes
- **quickgui**: GUI frontend for quickemu
- **box64**: x86_64 emulation layer

### Containerization
- **Docker**: Container runtime and ecosystem
- **Podman**: Daemonless container engine
- **distrobox**: Integration layer for containers
- **BoxBuddy**: GUI manager for distrobox

### Remote Access
- **SSH**: Secure shell access (enabled)
- **Proxmox VE**: Remote hypervisor management

## 🎯 Quick Start Commands

### Container Operations

```bash
# Docker
docker run -it ubuntu bash                    # Run Ubuntu container
docker ps                                     # List running containers
docker images                                 # List images

# Podman (Docker alternative)
podman run -it fedora bash                    # Run Fedora container
podman ps -a                                  # List all containers

# Distrobox (seamless integration)
distrobox create --name ubuntu --image ubuntu:latest
distrobox enter ubuntu                        # Enter container
distrobox list                               # List containers

# BoxBuddy (GUI)
flatpak run io.github.dvlv.boxbuddyrs        # Launch GUI
```

### Virtual Machines

```bash
# LibVirt/KVM
virsh list --all                             # List VMs
virt-manager                                  # Launch VM manager GUI
virsh start vm-name                          # Start VM
virsh shutdown vm-name                       # Shutdown VM

# QuickEMU (easy VM creation)
quickgui                                      # Launch GUI
quickemu --vm ubuntu-22.04.conf             # Run Ubuntu VM
```

### Remote Access

```bash
# Connect to Proxmox VE
ssh root@192.168.0.4                         # SSH to Proxmox
pve                                          # Alias for above
pve-web                                      # Open web interface

# Your system SSH access
ssh lou@192.168.12.172                       # From remote systems
```

## 📦 Container Examples

### Development Environment

```bash
# Create Ubuntu development container
distrobox create --name dev-ubuntu --image ubuntu:22.04
distrobox enter dev-ubuntu

# Inside container:
sudo apt update && sudo apt install -y build-essential git vim nodejs npm python3
```

### Fedora Container for RPM Development

```bash
# Create Fedora container
distrobox create --name dev-fedora --image fedora:latest
distrobox enter dev-fedora

# Inside container:
sudo dnf install -y @development-tools git vim nodejs npm python3
```

### Alpine Linux (minimal)

```bash
# Lightweight container
docker run -it alpine sh
podman run -it alpine sh
```

## 🎮 Virtual Machine Examples

### Windows 11 VM with QuickEMU

```bash
# Download and create Windows 11 VM
quickget windows 11
quickemu --vm windows-11.conf
```

### Ubuntu Desktop VM

```bash
# Download and create Ubuntu VM
quickget ubuntu 22.04
quickemu --vm ubuntu-22.04.conf
```

### Custom VM with virt-manager

1. Launch `virt-manager`
2. Click "Create new virtual machine"
3. Choose ISO or network install
4. Configure CPU, RAM, storage
5. Install guest OS

## 🔧 System Optimizations Applied

### Performance Tunings
- CPU governor set to performance mode
- Memory management optimized for VMs (vm.dirty_ratio = 5)
- Network buffers increased for VM traffic
- KSM (Kernel Same-page Merging) enabled
- I/O scheduler optimized for SSDs
- BBR congestion control enabled

### Security & Access
- SSH daemon enabled and configured
- User added to libvirt, docker, and kvm groups
- Firewall configured for necessary services

## 🌐 Networking Configuration

### Default Networks
- **libvirt default**: 192.168.122.0/24 (NAT)
- **docker0**: 172.17.0.0/16
- **podman**: 10.88.0.0/16

### Port Forwarding
```bash
# Forward VM port to host
virsh edit vm-name
# Add portforward section in XML
```

## 📁 Important Directories

```
/var/lib/libvirt/images/          # VM disk images
~/.local/share/containers/        # Podman data
/var/lib/docker/                  # Docker data
~/.local/share/distrobox/         # Distrobox containers
```

## 🚀 Optimization Script

Run the optimization script to apply performance tweaks:

```bash
./virt-optimize.sh
```

This script applies:
- CPU performance optimizations
- Memory management tuning
- Network optimizations
- Container registry configuration
- Pre-pulls common container images

## 🛡️ Security Considerations

### Container Security
```bash
# Run containers with limited privileges
podman run --user 1000:1000 --security-opt no-new-privileges alpine

# Scan images for vulnerabilities
docker scan image-name
```

### VM Security
- Use secure boot when possible
- Keep guest additions/tools updated
- Regular security updates
- Network segmentation with VLANs

## 🔍 Troubleshooting

### Container Issues
```bash
# Check container logs
docker logs container-name
podman logs container-name

# Clean up resources
docker system prune -af
podman system prune -af
```

### VM Issues
```bash
# Check libvirt logs
sudo journalctl -u libvirtd

# Verify hardware virtualization
lscpu | grep Virtualization
lsmod | grep kvm
```

### Network Issues
```bash
# Check virtual networks
virsh net-list --all
ip link show

# Restart networking
sudo systemctl restart libvirtd
```

## 📚 Useful Aliases (Auto-loaded)

The system includes helpful aliases:
- `pve` - SSH to Proxmox VE
- `pve-web` - Open Proxmox web interface
- `dbox` - Distrobox shortcut
- `d` - Docker shortcut
- `p` - Podman shortcut
- `qe` - QuickEMU
- `qeg` - QuickGUI

## 🎯 Next Steps

1. **Run optimization script**: `./virt-optimize.sh`
2. **Test Proxmox access**: `pve` or `pve-web`
3. **Create first container**: `distrobox create --name test --image ubuntu`
4. **Create first VM**: Launch `quickgui` or `virt-manager`
5. **Explore BoxBuddy**: `flatpak run io.github.dvlv.boxbuddyrs`

## 📖 Additional Resources

- [libvirt Documentation](https://libvirt.org/docs.html)
- [Docker Documentation](https://docs.docker.com/)
- [Podman Documentation](https://docs.podman.io/)
- [Distrobox GitHub](https://github.com/89luca89/distrobox)
- [QuickEMU GitHub](https://github.com/quickemu-project/quickemu)
- [Proxmox VE Documentation](https://pve.proxmox.com/pve-docs/)

Your system is now fully optimized for virtualization and containerization workflows! 🎉
