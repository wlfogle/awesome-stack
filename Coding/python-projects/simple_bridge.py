#!/usr/bin/env python
"""
Simple Alexa Bridge for Home Assistant
Emulates Philips Hue devices for local Alexa discovery
"""

import socket
import struct
import json
import requests
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading
import time

# Configuration
HA_URL = "http://192.168.122.113:8123"
HA_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJiNDYxNzBiNGM3NDU0MTYzODAzMTdkYTkwN2ZmNzU4MCIsImlhdCI6MTc1Mzg4MjQ2NiwiZXhwIjoyMDY5MjQyNDY2fQ.GUjQR5KBwjxuiNu1oYK6wqt9QZ1ckQ2sp7C1RfKDi_M"
BRIDGE_IP = "192.168.122.133"    # VM-613 IP address

# Device definitions (your Home Assistant scripts)
DEVICES = {
    "movie night": {"port": 12340, "script": "movie_night"},
    "system status": {"port": 12341, "script": "system_status"},
    "entertainment mode": {"port": 12342, "script": "entertainment_mode"},
    "restart plex": {"port": 12343, "script": "restart_plex"},
    "gaming mode": {"port": 12344, "script": "gaming_mode"},
    "restart jellyfin": {"port": 12345, "script": "restart_jellyfin"},
    "scan plex library": {"port": 12346, "script": "scan_plex_library"},
    "check downloads": {"port": 12347, "script": "check_downloads"},
    "ai assistant status": {"port": 12349, "script": "ai_assistant_status"},
    "emergency status": {"port": 12350, "script": "emergency_status"},
    "check storage space": {"port": 12351, "script": "check_storage_space"},
    "maintenance mode": {"port": 12352, "script": "maintenance_mode"},
    "bedtime mode": {"port": 12354, "script": "bedtime_mode"}
}

class AlexaBridge:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
    def start_discovery_server(self):
        """Start SSDP discovery server for Alexa"""
        multicast_group = '239.255.255.250'
        server_address = ('', 1900)
        
        # Bind to the server address
        self.sock.bind(server_address)
        
        # Tell the OS to add the socket to the multicast group
        group = socket.inet_aton(multicast_group)
        mreq = struct.pack('4sL', group, socket.INADDR_ANY)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        
        print(f"🎤 Alexa Discovery Server started on {multicast_group}:1900")
        
        while True:
            try:
                data, address = self.sock.recvfrom(1024)
                message = data.decode('utf-8')
                
                if 'M-SEARCH' in message and 'upnp:rootdevice' in message:
                    print(f"📡 Discovery request from {address[0]}")
                    self.send_discovery_response(address[0])
                    
            except Exception as e:
                print(f"❌ Discovery error: {e}")
                
    def send_discovery_response(self, requester_ip):
        """Send SSDP discovery response"""
        response = (
            "HTTP/1.1 200 OK\r\n"
            "CACHE-CONTROL: max-age=86400\r\n"
            f"DATE: {time.strftime('%a, %d %b %Y %H:%M:%S GMT')}\r\n"
            "EXT:\r\n"
            f"LOCATION: http://{BRIDGE_IP}:9080/description.xml\r\n"
            "SERVER: Linux/3.14.0 UPnP/1.0 IpBridge/1.26.0\r\n"
            "ST: upnp:rootdevice\r\n"
            "USN: uuid:2f402f80-da50-11e1-9b23-001788255acc\r\n\r\n"
        )
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(response.encode(), (requester_ip, 1900))
        sock.close()

class HueEmulator(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/description.xml':
            self.send_description()
        elif '/api/' in self.path and '/lights' in self.path:
            self.send_lights_list()
        else:
            self.send_response(404)
            self.end_headers()
            
    def do_PUT(self):
        if '/api/' in self.path and '/state' in self.path:
            self.handle_device_control()
        else:
            self.send_response(404)
            self.end_headers()
            
    def send_description(self):
        """Send UPnP description XML"""
        xml = f'''<?xml version="1.0" encoding="UTF-8"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
    <device>
        <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
        <friendlyName>Philips hue ({BRIDGE_IP})</friendlyName>
        <manufacturer>Royal Philips Electronics</manufacturer>
        <modelName>Philips hue bridge 2012</modelName>
        <modelNumber>929000226503</modelNumber>
        <serialNumber>001788255acc</serialNumber>
        <UDN>uuid:2f402f80-da50-11e1-9b23-001788255acc</UDN>
    </device>
</root>'''
        
        self.send_response(200)
        self.send_header('Content-Type', 'application/xml')
        self.end_headers()
        self.wfile.write(xml.encode())
        
    def send_lights_list(self):
        """Send list of available 'lights' (our HA scripts)"""
        lights = {}
        for i, (name, config) in enumerate(DEVICES.items(), 1):
            lights[str(i)] = {
                "name": name,
                "state": {"on": False, "reachable": True},
                "type": "Dimmable light",
                "modelid": "LWB004",
                "manufacturername": "Philips",
                "uniqueid": f"00:17:88:01:00:00:00:{i:02x}"
            }
            
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(lights).encode())
        
    def handle_device_control(self):
        """Handle device on/off commands"""
        path_parts = self.path.split('/')
        light_id = path_parts[3] if len(path_parts) > 3 else None
        
        if light_id and light_id.isdigit():
            device_names = list(DEVICES.keys())
            device_index = int(light_id) - 1
            
            if 0 <= device_index < len(device_names):
                device_name = device_names[device_index]
                script_name = DEVICES[device_name]["script"]
                
                print(f"🎯 Executing: {device_name} -> {script_name}")
                self.call_home_assistant_script(script_name)
                
        response = [{"success": {f"/lights/{light_id}/state/on": True}}]
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(response).encode())
        
    def call_home_assistant_script(self, script_name):
        """Call Home Assistant script"""
        url = f"{HA_URL}/api/services/script/{script_name}"
        headers = {
            "Authorization": f"Bearer {HA_TOKEN}",
            "Content-Type": "application/json"
        }
        
        try:
            response = requests.post(url, headers=headers, json={})
            print(f"✅ Home Assistant call: {response.status_code}")
        except Exception as e:
            print(f"❌ Home Assistant error: {e}")

def main():
    print("🚀 Starting Simple Alexa Bridge...")
    print(f"📱 Bridge IP: {BRIDGE_IP}")
    print(f"🏠 Home Assistant: {HA_URL}")
    print(f"🎤 Devices configured: {len(DEVICES)}")
    
    # Start discovery server in background
    bridge = AlexaBridge()
    discovery_thread = threading.Thread(target=bridge.start_discovery_server)
    discovery_thread.daemon = True
    discovery_thread.start()
    
    # Start HTTP server for Hue API emulation
    httpd = HTTPServer(('0.0.0.0', 9080), HueEmulator)
    print("🌐 HTTP Server started on port 9080")
    print("🎯 Ready for Alexa discovery!")
    print("💡 Say: 'Alexa, discover devices'")
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Bridge stopped")
        httpd.shutdown()

if __name__ == "__main__":
    main()
