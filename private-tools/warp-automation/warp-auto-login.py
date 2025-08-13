#!/usr/bin/env python3
"""
Warp Auto-Login Automation
Handles automatic authentication and session management for Warp Terminal
"""

import os
import json
import time
import requests
import subprocess
from pathlib import Path

# Configuration
WARP_CONFIG_DIR = Path.home() / ".config" / "warp-automation"
WARP_TOKEN_FILE = WARP_CONFIG_DIR / "warp_token.json"
WARP_SESSION_FILE = WARP_CONFIG_DIR / "warp_session.json"

# API Configuration (Replace with actual endpoints)
WARP_API_BASE = "https://api.warp.dev"
LOGIN_ENDPOINT = f"{WARP_API_BASE}/auth/login"
REFRESH_ENDPOINT = f"{WARP_API_BASE}/auth/refresh"
VALIDATE_ENDPOINT = f"{WARP_API_BASE}/auth/validate"

class WarpAutoLogin:
    def __init__(self):
        self.config_dir = WARP_CONFIG_DIR
        self.token_file = WARP_TOKEN_FILE
        self.session_file = WARP_SESSION_FILE
        
        # Ensure config directory exists
        self.config_dir.mkdir(parents=True, exist_ok=True)
        
        # Load credentials from environment or config
        self.email = os.getenv('WARP_EMAIL')
        self.password = os.getenv('WARP_PASSWORD')
        self.api_key = os.getenv('WARP_API_KEY')
        
    def load_stored_token(self):
        """Load stored authentication token"""
        try:
            if self.token_file.exists():
                with open(self.token_file, 'r') as f:
                    return json.load(f)
        except Exception as e:
            print(f"Error loading token: {e}")
        return None
    
    def save_token(self, token_data):
        """Save authentication token securely"""
        try:
            with open(self.token_file, 'w') as f:
                json.dump(token_data, f, indent=2)
            # Secure the file
            os.chmod(self.token_file, 0o600)
        except Exception as e:
            print(f"Error saving token: {e}")
    
    def login(self):
        """Perform initial login"""
        if not self.email or not self.password:
            print("❌ WARP_EMAIL and WARP_PASSWORD environment variables required")
            return None
            
        try:
            payload = {
                "email": self.email,
                "password": self.password
            }
            
            response = requests.post(LOGIN_ENDPOINT, json=payload, timeout=10)
            
            if response.status_code == 200:
                token_data = response.json()
                token_data['timestamp'] = time.time()
                self.save_token(token_data)
                print("✅ Warp login successful")
                return token_data
            else:
                print(f"❌ Login failed: {response.status_code}")
                return None
                
        except Exception as e:
            print(f"❌ Login error: {e}")
            return None
    
    def refresh_token(self, token_data):
        """Refresh authentication token"""
        try:
            headers = {
                "Authorization": f"Bearer {token_data.get('access_token')}",
                "Content-Type": "application/json"
            }
            
            payload = {
                "refresh_token": token_data.get('refresh_token')
            }
            
            response = requests.post(REFRESH_ENDPOINT, json=payload, headers=headers, timeout=10)
            
            if response.status_code == 200:
                new_token = response.json()
                new_token['timestamp'] = time.time()
                self.save_token(new_token)
                print("✅ Token refreshed successfully")
                return new_token
            else:
                print(f"❌ Token refresh failed: {response.status_code}")
                return None
                
        except Exception as e:
            print(f"❌ Token refresh error: {e}")
            return None
    
    def validate_token(self, token_data):
        """Validate current token"""
        try:
            headers = {
                "Authorization": f"Bearer {token_data.get('access_token')}",
                "Content-Type": "application/json"
            }
            
            response = requests.get(VALIDATE_ENDPOINT, headers=headers, timeout=10)
            return response.status_code == 200
            
        except Exception as e:
            print(f"❌ Token validation error: {e}")
            return False
    
    def is_token_expired(self, token_data):
        """Check if token is expired"""
        if not token_data:
            return True
            
        timestamp = token_data.get('timestamp', 0)
        expires_in = token_data.get('expires_in', 3600)  # Default 1 hour
        
        return time.time() > (timestamp + expires_in - 300)  # Refresh 5 min early
    
    def ensure_authenticated(self):
        """Ensure we have a valid authentication token"""
        token_data = self.load_stored_token()
        
        # If no token or token is expired, try to refresh or re-login
        if not token_data or self.is_token_expired(token_data):
            if token_data and token_data.get('refresh_token'):
                print("🔄 Refreshing token...")
                token_data = self.refresh_token(token_data)
            
            if not token_data:
                print("🔑 Performing fresh login...")
                token_data = self.login()
        
        # Validate token
        if token_data and not self.validate_token(token_data):
            print("🔄 Token invalid, re-authenticating...")
            token_data = self.login()
        
        return token_data
    
    def start_warp_with_auth(self):
        """Start Warp terminal with authentication"""
        token_data = self.ensure_authenticated()
        
        if not token_data:
            print("❌ Failed to authenticate with Warp")
            return False
        
        try:
            # Set environment variables for Warp
            env = os.environ.copy()
            env['WARP_AUTH_TOKEN'] = token_data.get('access_token')
            env['WARP_USER_ID'] = token_data.get('user_id', '')
            
            # Launch Warp terminal
            subprocess.Popen(['warp-terminal'], env=env, 
                           stdout=subprocess.DEVNULL, 
                           stderr=subprocess.DEVNULL)
            
            print("✅ Warp terminal launched with authentication")
            return True
            
        except Exception as e:
            print(f"❌ Failed to launch Warp: {e}")
            return False
    
    def setup_auto_login(self):
        """Setup automatic login on system startup"""
        desktop_file = Path.home() / ".config/autostart/warp-auto-login.desktop"
        script_path = Path(__file__).absolute()
        
        desktop_content = f"""[Desktop Entry]
Type=Application
Name=Warp Auto Login
Comment=Automatic Warp Terminal Authentication
Exec=python3 {script_path} --auto-start
Hidden=false
NoDisplay=true
X-GNOME-Autostart-enabled=true
"""
        
        desktop_file.parent.mkdir(parents=True, exist_ok=True)
        with open(desktop_file, 'w') as f:
            f.write(desktop_content)
        
        print("✅ Auto-login setup complete")
    
    def create_systemd_service(self):
        """Create systemd user service for persistent authentication"""
        service_file = Path.home() / ".config/systemd/user/warp-auto-login.service"
        script_path = Path(__file__).absolute()
        
        service_content = f"""[Unit]
Description=Warp Auto Login Service
After=network-online.target

[Service]
Type=simple
ExecStart=python3 {script_path} --daemon
Restart=always
RestartSec=30
Environment=HOME={Path.home()}

[Install]
WantedBy=default.target
"""
        
        service_file.parent.mkdir(parents=True, exist_ok=True)
        with open(service_file, 'w') as f:
            f.write(service_content)
        
        # Enable and start the service
        subprocess.run(['systemctl', '--user', 'daemon-reload'])
        subprocess.run(['systemctl', '--user', 'enable', 'warp-auto-login.service'])
        subprocess.run(['systemctl', '--user', 'start', 'warp-auto-login.service'])
        
        print("✅ Systemd service created and started")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Warp Auto-Login Automation")
    parser.add_argument('--login', action='store_true', help='Perform login')
    parser.add_argument('--start-warp', action='store_true', help='Start Warp with auth')
    parser.add_argument('--auto-start', action='store_true', help='Auto-start mode')
    parser.add_argument('--daemon', action='store_true', help='Run as daemon')
    parser.add_argument('--setup', action='store_true', help='Setup auto-login')
    parser.add_argument('--service', action='store_true', help='Create systemd service')
    
    args = parser.parse_args()
    
    warp_login = WarpAutoLogin()
    
    if args.setup:
        warp_login.setup_auto_login()
    elif args.service:
        warp_login.create_systemd_service()
    elif args.login:
        warp_login.ensure_authenticated()
    elif args.start_warp:
        warp_login.start_warp_with_auth()
    elif args.daemon:
        # Daemon mode - keep authentication fresh
        while True:
            warp_login.ensure_authenticated()
            time.sleep(1800)  # Check every 30 minutes
    elif args.auto_start:
        # Auto-start mode - authenticate and launch Warp
        warp_login.start_warp_with_auth()
    else:
        print("Warp Auto-Login Tool")
        print("Usage: python3 warp-auto-login.py [--login|--start-warp|--setup|--service]")

if __name__ == "__main__":
    main()
