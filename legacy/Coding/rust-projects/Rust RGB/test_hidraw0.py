#!/usr/bin/env python3
"""
HIDraw0 Monitor - Test touchpad data
"""
import os
import sys
import time

def monitor_hidraw0():
    """Monitor /dev/hidraw0 for touchpad data"""
    device_path = "/dev/hidraw0"
    
    print(f"🔍 Monitoring {device_path} for touchpad activity...")
    print("Move your finger on the touchpad and I'll show the data:")
    print("=" * 60)
    
    try:
        with open(device_path, 'rb') as device:
            packet_count = 0
            while packet_count < 50:  # Monitor for 50 packets
                try:
                    # Read raw data from hidraw0
                    data = device.read(64)  # Read up to 64 bytes
                    if data:
                        packet_count += 1
                        timestamp = time.strftime("%H:%M:%S")
                        
                        # Convert to hex for easy reading
                        hex_data = ' '.join([f'{b:02x}' for b in data])
                        
                        print(f"[{timestamp}] Packet {packet_count:2d}: {hex_data}")
                        print(f"                    Length: {len(data)} bytes")
                        print(f"                    ASCII:  {''.join([chr(b) if 32 <= b <= 126 else '.' for b in data])}")
                        print("-" * 60)
                        
                except KeyboardInterrupt:
                    print("\n🛑 Monitoring stopped by user")
                    break
                except Exception as e:
                    print(f"Read error: {e}")
                    time.sleep(0.1)
                    
    except PermissionError:
        print(f"❌ Permission denied. Run with sudo:")
        print(f"   sudo python3 {sys.argv[0]}")
        return False
    except FileNotFoundError:
        print(f"❌ Device {device_path} not found")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    print(f"\n✅ Monitoring completed. Captured {packet_count} packets.")
    return True

if __name__ == "__main__":
    print("🔧 HIDraw0 Touchpad Monitor")
    print("=" * 30)
    
    # Check if device exists
    if not os.path.exists("/dev/hidraw0"):
        print("❌ /dev/hidraw0 does not exist")
        sys.exit(1)
    
    # Check permissions
    if not os.access("/dev/hidraw0", os.R_OK):
        print("⚠️  No read permission for /dev/hidraw0")
        print("   Try running with sudo")
    
    monitor_hidraw0()
