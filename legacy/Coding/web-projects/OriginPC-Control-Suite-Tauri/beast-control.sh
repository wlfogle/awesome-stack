#!/bin/bash

echo "🔥 OriginPC EON17-X Beast Control Center 🔥"
echo "========================================"
echo ""

# Show current system status
echo "📊 Current System Status:"
echo "   RAM: $(free -h | grep '^Mem:' | awk '{print $3 "/" $2}')"
echo "   CPU: $(grep -c ^processor /proc/cpuinfo) cores @ $(cat /proc/cpuinfo | grep MHz | head -1 | awk '{print $4}')MHz"
echo "   GPU: $(lspci | grep VGA | wc -l) GPUs detected"
echo ""

# Show running VMs
echo "🖥️ Running VMs:"
sudo virsh list --state-running | grep -v "Id.*Name.*State" | grep -v "^$" | grep -v "^-"
echo ""

# Show available modes
echo "🎯 Available Beast Modes:"
echo "   1) 🎮 Gaming Mode        - 40GB RAM, 20 cores to gaming"  
echo "   2) 🏠 Self-Hosting Mode  - 32GB RAM, 16 cores to Proxmox cluster"
echo "   3) 🔥 Hybrid Mode        - Gaming + Self-hosting together!"
echo "   4) 💻 Development Mode   - Balanced for coding + testing"
echo "   5) 🛑 All Stop          - Stop all VMs, maximum host power"
echo "   6) 📊 System Monitor    - Show detailed resource usage"
echo "   7) 🏠 Proxmox Cluster   - Manage Proxmox cluster status"
echo ""

read -p "Select mode (1-7): " choice

case $choice in
    1)
        echo "🎮 Activating Gaming Beast Mode..."
        sudo virsh shutdown proxmox-selfhost 2>/dev/null
        ./start-gaming-vm.sh
        ;;
    2)  
        echo "🏠 Activating Self-Hosting Beast Mode..."
        sudo virsh shutdown win10-gaming 2>/dev/null
        ./self-hosting-mode.sh
        ;;
    3)
        echo "🔥 Activating HYBRID Beast Mode..."
        echo "   Your system is about to become a DATACENTER!"
        
        # Start gaming VM with reduced resources for hybrid
        echo "🎮 Starting gaming VM (32GB mode)..."
        # Modify gaming VM for hybrid mode
        sudo virsh setmaxmem win10-gaming 33554432k --config  # 32GB
        sudo virsh setmem win10-gaming 33554432k --config
        ./start-gaming-vm.sh
        
        echo "🏠 Starting self-hosting VM (20GB mode)..."
        sleep 3
        ./self-hosting-mode.sh
        
        echo "🔥 HYBRID MODE ACTIVE!"
        echo "   - Gaming: 32GB RAM, cores 2-9+18-25"
        echo "   - Self-hosting: 20GB RAM, cores 10-15+26-31" 
        echo "   - Host: 12GB RAM, cores 0-1+16-17"
        echo "   You now have BOTH gaming and self-hosting!"
        ;;
    4)
        echo "💻 Activating Development Mode..."
        sudo virsh shutdown win10-gaming 2>/dev/null
        sudo virsh shutdown proxmox-selfhost 2>/dev/null
        
        # Start lighter development VMs
        echo "   - Host gets maximum resources for development"
        echo "   - All VMs stopped for maximum coding power"
        echo "   - Use 'beast-control.sh' to start specific workloads"
        ;;
    5)
        echo "🛑 Stopping all VMs..."
        sudo virsh shutdown win10-gaming 2>/dev/null
        sudo virsh shutdown proxmox-selfhost 2>/dev/null
        sudo virsh shutdown win10-test 2>/dev/null
        ./stop-gaming-vm.sh 2>/dev/null
        echo "   All VMs stopped. Host has full 64GB RAM + 32 cores!"
        ;;
    6)
        echo "📊 System Resource Monitor:"
        echo ""
        echo "Memory Usage:"
        free -h
        echo ""
        echo "CPU Usage:"
        top -bn1 | grep "Cpu(s)" 
        echo ""
        echo "VM Status:"
        sudo virsh list --all
        echo ""
        echo "GPU Status:"
        lspci -k | grep -A3 VGA
        echo ""
        echo "Storage Usage:"
        df -h | grep -E "(nvme|sda)"
        ;;
    7)
        echo "🏠 Proxmox Cluster Management:"
        echo ""
        echo "📱 Pi Node Status (192.168.0.64):"
        if ping -c 1 192.168.0.64 &>/dev/null; then
            echo "   ✅ Pi online"
            ssh pimox "pvecm status" 2>/dev/null || echo "   ❌ Cluster not accessible"
        else
            echo "   ❌ Pi offline"
        fi
        echo ""
        echo "🔥 Beast Node Status (192.168.0.65):"
        if sudo virsh list --state-running | grep -q proxmox-selfhost; then
            echo "   ✅ VM running"
            if ping -c 1 192.168.0.65 &>/dev/null; then
                echo "   ✅ Network accessible"
                ssh root@192.168.0.65 "pvecm status" 2>/dev/null || echo "   ❌ Cluster not joined"
            else
                echo "   ❌ Network not accessible"
            fi
        else
            echo "   ❌ VM not running"
        fi
        echo ""
        echo "🔧 Quick Actions:"
        echo "   - Start Proxmox VM: sudo virsh start proxmox-selfhost"
        echo "   - Join cluster: ./add-to-cluster.sh"
        echo "   - Web interface Pi: https://192.168.0.64:8006"
        echo "   - Web interface VM: https://192.168.0.65:8006"
        ;;
    *)
        echo "Invalid choice!"
        ;;
esac

echo ""
echo "🔥 Beast Control Center - Ready for next command!"
echo "   Your OriginPC EON17-X is optimized and ready! 🚀"
