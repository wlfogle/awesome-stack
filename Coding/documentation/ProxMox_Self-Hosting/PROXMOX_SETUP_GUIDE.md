# 🚀 Proxmox VE VM & Cluster Setup Guide

## ✅ **Status: VM Created Successfully!**

Your Proxmox VE virtual machine has been created and is currently running. Here's everything you need to complete the setup.

## 📊 **Current Configuration**

- **VM Name**: proxmox-selfhost
- **Target IP**: 192.168.0.65
- **Cluster Master**: 192.168.0.64 (existing)
- **VM Status**: Running and ready for installation
- **VNC Access**: vnc://localhost:5902

## 🎯 **Next Steps**

### 1. 🖥️ Complete Proxmox Installation

**Connect to VM via VNC:**
```bash
# Install a VNC viewer if you don't have one
sudo pacman -S remmina  # or tigervnc, vinagre

# Connect to: vnc://localhost:5902
```

**During Proxmox Installation:**
- Accept license agreements
- Select target disk (100GB virtio disk)
- Set timezone and keyboard layout
- Create root password and admin email
- **Network Configuration**: Use DHCP initially (we'll change this later)
- Hostname: `proxmox`
- Domain: `local`

### 2. 📡 Configure Network for Cluster Access

After installation, the VM will have an IP in the 192.168.122.x range. To join the cluster at 192.168.0.64, you need to configure it for the 192.168.0.x network.

**Option A: Reconfigure VM Network (Recommended)**

1. Shut down the VM:
```bash
sudo virsh shutdown proxmox-selfhost
```

2. Edit VM configuration to use bridged networking:
```bash
sudo virsh edit proxmox-selfhost
```

3. Change the network section from:
```xml
<interface type='network'>
  <source network='default'/>
```

to:
```xml
<interface type='direct'>
  <source dev='enp4s0' mode='bridge'/>
  <model type='virtio'/>
</interface>
```

4. Start the VM:
```bash
sudo virsh start proxmox-selfhost
```

**Option B: Configure Static IP in VM**

Access the VM console and edit network configuration:
```bash
# In the VM, edit network interfaces
nano /etc/network/interfaces
```

Use the configuration from `vm-network-config.txt`:
```
auto lo
iface lo inet loopback

iface ens3 inet manual

auto vmbr0
iface vmbr0 inet static
        address 192.168.0.65/24
        gateway 192.168.0.1
        bridge-ports ens3
        bridge-stp off
        bridge-fd 0
```

### 3. 🏗️ Set Up Cluster

**Step 1: Create cluster on master node (192.168.0.64)**
```bash
ssh root@192.168.0.64
pvecm create homelab-cluster
```

**Step 2: Join new node to cluster**
```bash
# SSH to your new VM (once it has IP 192.168.0.65)
ssh root@192.168.0.65
pvecm add 192.168.0.64
```

**Step 3: Verify cluster**
```bash
pvecm status
pvecm nodes
```

## 🛠️ **VM Management Commands**

```bash
# Check VM status
sudo virsh list --all

# Start VM
sudo virsh start proxmox-selfhost

# Stop VM
sudo virsh shutdown proxmox-selfhost

# Force stop VM
sudo virsh destroy proxmox-selfhost

# Console access
sudo virsh console proxmox-selfhost

# VNC access
vnc://localhost:5902

# Get VM IP
sudo virsh domifaddr proxmox-selfhost
```

## 🌐 **Network Information**

- **Current VM Network**: 192.168.122.x (NAT)
- **Target Network**: 192.168.0.65 (bridged)
- **Cluster Master**: 192.168.0.64
- **Your Garuda Host**: 192.168.0.184

## 🔧 **Optimization for Self-Hosting**

The VM is configured with:
- **8GB RAM** (adjustable)
- **4 vCPUs** (adjustable)
- **100GB Storage** (thin provisioned)
- **VirtIO drivers** for optimal performance

### Adjust Resources if Needed:
```bash
# Stop VM first
sudo virsh shutdown proxmox-selfhost

# Edit configuration
sudo virsh edit proxmox-selfhost

# Modify memory (in KB):
<memory unit='KiB'>8388608</memory>  # 8GB

# Modify CPUs:
<vcpu placement='static'>4</vcpu>

# Start VM
sudo virsh start proxmox-selfhost
```

## 🔍 **Troubleshooting**

### VM Won't Start
```bash
sudo virsh start proxmox-selfhost --console
sudo journalctl -u libvirtd
```

### Network Issues
```bash
# Check libvirt networks
sudo virsh net-list --all
sudo virsh net-start default

# Check VM network config
sudo virsh domiflist proxmox-selfhost
```

### Performance Issues
```bash
# Check host resources
htop
free -h

# Optimize VM
sudo virsh edit proxmox-selfhost
# Add: <cpu mode='host-passthrough'/>
```

## 📚 **Useful Scripts Created**

- `create-proxmox-vm-final.sh` - VM creation script
- `configure-proxmox-vm.sh` - Post-installation configuration
- `setup-cluster.sh` - Cluster setup automation
- `vm-network-config.txt` - Network configuration template

## 🎯 **Expected Results**

Once complete, you'll have:
1. ✅ Proxmox VE VM running at 192.168.0.65
2. ✅ VM joined to cluster with 192.168.0.64
3. ✅ Unified cluster management interface
4. ✅ High-performance self-hosting platform

## 🌐 **Web Access**

After configuration:
- **New Node**: https://192.168.0.65:8006
- **Master Node**: https://192.168.0.64:8006
- **Cluster View**: Available from either interface

Your Proxmox cluster will be ready for self-hosting applications, VMs, and containers! 🎉
