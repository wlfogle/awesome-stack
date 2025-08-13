#!/usr/bin/env python3
"""
Advanced VPN IP Rotation System
Handles multiple VPN providers, API keys, and automated rotation
"""

import os
import json
import time
import random
import requests
import subprocess
from datetime import datetime, timedelta
from pathlib import Path

# Configuration
CONFIG_DIR = Path.home() / ".config" / "vpn-rotation"
SECRETS_FILE = CONFIG_DIR / "secrets.json"
ROTATION_LOG = CONFIG_DIR / "rotation.log"
PROVIDERS_CONFIG = CONFIG_DIR / "providers.json"

class VPNRotationManager:
    def __init__(self):
        self.config_dir = CONFIG_DIR
        self.config_dir.mkdir(parents=True, exist_ok=True)
        
        self.secrets = self.load_secrets()
        self.providers = self.load_providers()
        self.current_provider = None
        self.rotation_history = []
        
    def load_secrets(self):
        """Load API keys and credentials securely"""
        try:
            if SECRETS_FILE.exists():
                with open(SECRETS_FILE, 'r') as f:
                    secrets = json.load(f)
                # Decrypt if needed
                return secrets
        except Exception as e:
            self.log(f"Error loading secrets: {e}")
        
        # Default empty secrets
        return {
            "nordvpn": {
                "username": os.getenv("NORDVPN_USER", ""),
                "password": os.getenv("NORDVPN_PASS", ""),
                "api_key": os.getenv("NORDVPN_API_KEY", "")
            },
            "expressvpn": {
                "activation_code": os.getenv("EXPRESS_ACTIVATION_CODE", ""),
                "api_key": os.getenv("EXPRESS_API_KEY", "")
            },
            "surfshark": {
                "username": os.getenv("SURFSHARK_USER", ""),
                "password": os.getenv("SURFSHARK_PASS", ""),
                "api_key": os.getenv("SURFSHARK_API_KEY", "")
            },
            "wireguard": {
                "server_key": os.getenv("WG_SERVER_PRIVATE_KEY", ""),
                "api_endpoint": os.getenv("WG_API_ENDPOINT", ""),
                "auth_token": os.getenv("WG_AUTH_TOKEN", "")
            },
            "custom_proxy": {
                "api_key": os.getenv("PROXY_API_KEY", ""),
                "endpoint": os.getenv("PROXY_ENDPOINT", ""),
                "auth_user": os.getenv("PROXY_USER", ""),
                "auth_pass": os.getenv("PROXY_PASS", "")
            }
        }
    
    def save_secrets(self):
        """Save secrets securely"""
        try:
            with open(SECRETS_FILE, 'w') as f:
                json.dump(self.secrets, f, indent=2)
            os.chmod(SECRETS_FILE, 0o600)
        except Exception as e:
            self.log(f"Error saving secrets: {e}")
    
    def load_providers(self):
        """Load VPN provider configurations"""
        default_providers = {
            "wireguard": {
                "name": "WireGuard",
                "type": "wireguard",
                "priority": 1,
                "locations": ["us-east", "us-west", "eu-central", "asia-pacific"],
                "rotation_interval": 3600,  # 1 hour
                "enabled": True
            },
            "nordvpn": {
                "name": "NordVPN",
                "type": "openvpn",
                "priority": 2,
                "locations": ["us", "uk", "de", "jp", "au"],
                "rotation_interval": 7200,  # 2 hours
                "enabled": False
            },
            "custom_proxy": {
                "name": "Custom Rotating Proxy",
                "type": "proxy",
                "priority": 3,
                "locations": ["global"],
                "rotation_interval": 1800,  # 30 minutes
                "enabled": True
            }
        }
        
        try:
            if PROVIDERS_CONFIG.exists():
                with open(PROVIDERS_CONFIG, 'r') as f:
                    return json.load(f)
        except Exception as e:
            self.log(f"Error loading providers: {e}")
        
        return default_providers
    
    def log(self, message):
        """Log rotation events"""
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        log_entry = f"[{timestamp}] {message}\n"
        
        try:
            with open(ROTATION_LOG, 'a') as f:
                f.write(log_entry)
        except Exception as e:
            print(f"Logging error: {e}")
        
        print(log_entry.strip())
    
    def get_current_ip(self):
        """Get current external IP address"""
        try:
            response = requests.get('https://api.ipify.org?format=json', timeout=10)
            return response.json().get('ip')
        except Exception as e:
            self.log(f"Error getting current IP: {e}")
            return None
    
    def rotate_wireguard_ip(self):
        """Rotate WireGuard IP using server API"""
        try:
            wg_secrets = self.secrets.get('wireguard', {})\n            api_endpoint = wg_secrets.get('api_endpoint')\n            auth_token = wg_secrets.get('auth_token')\n            \n            if not api_endpoint or not auth_token:\n                self.log(\"WireGuard API credentials not configured\")\n                return False\n            \n            headers = {\n                'Authorization': f'Bearer {auth_token}',\n                'Content-Type': 'application/json'\n            }\n            \n            # Request new IP from WireGuard server\n            response = requests.post(\n                f'{api_endpoint}/rotate',\n                headers=headers,\n                json={'client': 'garuda-host'},\n                timeout=15\n            )\n            \n            if response.status_code == 200:\n                result = response.json()\n                new_ip = result.get('new_ip')\n                config = result.get('config')\n                \n                # Update local WireGuard config\n                if config:\n                    with open('/etc/wireguard/wg0.conf', 'w') as f:\n                        f.write(config)\n                    \n                    # Restart WireGuard\n                    subprocess.run(['systemctl', 'restart', 'wg-quick@wg0'])\n                    \n                    self.log(f\"WireGuard IP rotated to: {new_ip}\")\n                    return True\n            \n            self.log(f\"WireGuard rotation failed: {response.status_code}\")\n            return False\n            \n        except Exception as e:\n            self.log(f\"WireGuard rotation error: {e}\")\n            return False\n    \n    def rotate_proxy_endpoint(self):\n        \"\"\"Rotate custom proxy endpoint\"\"\"\n        try:\n            proxy_secrets = self.secrets.get('custom_proxy', {})\n            api_key = proxy_secrets.get('api_key')\n            endpoint = proxy_secrets.get('endpoint')\n            \n            if not api_key or not endpoint:\n                self.log(\"Proxy API credentials not configured\")\n                return False\n            \n            headers = {\n                'X-API-Key': api_key,\n                'Content-Type': 'application/json'\n            }\n            \n            # Request new proxy endpoint\n            response = requests.post(\n                f'{endpoint}/rotate',\n                headers=headers,\n                json={'type': 'residential'},\n                timeout=15\n            )\n            \n            if response.status_code == 200:\n                result = response.json()\n                new_proxy = result.get('proxy_url')\n                \n                # Update system proxy settings\n                self.update_system_proxy(new_proxy)\n                \n                self.log(f\"Proxy rotated to: {new_proxy}\")\n                return True\n            \n            self.log(f\"Proxy rotation failed: {response.status_code}\")\n            return False\n            \n        except Exception as e:\n            self.log(f\"Proxy rotation error: {e}\")\n            return False\n    \n    def update_system_proxy(self, proxy_url):\n        \"\"\"Update system-wide proxy settings\"\"\"\n        try:\n            # Set environment variables\n            os.environ['HTTP_PROXY'] = proxy_url\n            os.environ['HTTPS_PROXY'] = proxy_url\n            \n            # Update proxy for current session\n            proxy_config = {\n                'http': proxy_url,\n                'https': proxy_url\n            }\n            \n            # You can extend this to update browser settings, etc.\n            self.log(f\"System proxy updated: {proxy_url}\")\n            \n        except Exception as e:\n            self.log(f\"Error updating system proxy: {e}\")\n    \n    def perform_rotation(self, provider_name=None):\n        \"\"\"Perform IP rotation using specified or best provider\"\"\"\n        if not provider_name:\n            # Auto-select best provider\n            enabled_providers = {\n                k: v for k, v in self.providers.items() \n                if v.get('enabled', False)\n            }\n            \n            if not enabled_providers:\n                self.log(\"No enabled providers found\")\n                return False\n            \n            # Sort by priority\n            provider_name = min(enabled_providers.keys(), \n                              key=lambda x: enabled_providers[x].get('priority', 999))\n        \n        provider = self.providers.get(provider_name)\n        if not provider:\n            self.log(f\"Provider {provider_name} not found\")\n            return False\n        \n        old_ip = self.get_current_ip()\n        success = False\n        \n        if provider['type'] == 'wireguard':\n            success = self.rotate_wireguard_ip()\n        elif provider['type'] == 'proxy':\n            success = self.rotate_proxy_endpoint()\n        # Add more provider types as needed\n        \n        if success:\n            time.sleep(5)  # Wait for connection to establish\n            new_ip = self.get_current_ip()\n            \n            self.rotation_history.append({\n                'timestamp': datetime.now().isoformat(),\n                'provider': provider_name,\n                'old_ip': old_ip,\n                'new_ip': new_ip,\n                'success': True\n            })\n            \n            self.log(f\"Rotation successful: {old_ip} -> {new_ip} via {provider_name}\")\n            return True\n        else:\n            self.rotation_history.append({\n                'timestamp': datetime.now().isoformat(),\n                'provider': provider_name,\n                'old_ip': old_ip,\n                'new_ip': old_ip,\n                'success': False\n            })\n            \n            self.log(f\"Rotation failed for provider: {provider_name}\")\n            return False\n    \n    def stealth_mode_rotation(self):\n        \"\"\"Aggressive rotation for maximum privacy\"\"\"\n        self.log(\"Entering stealth mode - aggressive rotation\")\n        \n        # Rotate every 10-30 minutes randomly\n        while True:\n            self.perform_rotation()\n            \n            # Random delay between 10-30 minutes\n            delay = random.randint(600, 1800)\n            self.log(f\"Next rotation in {delay//60} minutes\")\n            time.sleep(delay)\n    \n    def scheduled_rotation(self):\n        \"\"\"Run scheduled rotation based on provider intervals\"\"\"\n        self.log(\"Starting scheduled rotation service\")\n        \n        last_rotations = {}\n        \n        while True:\n            current_time = datetime.now()\n            \n            for provider_name, config in self.providers.items():\n                if not config.get('enabled', False):\n                    continue\n                \n                interval = config.get('rotation_interval', 3600)\n                last_rotation = last_rotations.get(provider_name)\n                \n                if (not last_rotation or \n                    (current_time - last_rotation).seconds >= interval):\n                    \n                    if self.perform_rotation(provider_name):\n                        last_rotations[provider_name] = current_time\n            \n            # Check every 5 minutes\n            time.sleep(300)\n    \n    def test_all_providers(self):\n        \"\"\"Test connectivity for all enabled providers\"\"\"\n        self.log(\"Testing all providers...\")\n        \n        results = {}\n        for provider_name, config in self.providers.items():\n            if not config.get('enabled', False):\n                continue\n            \n            self.log(f\"Testing {provider_name}...\")\n            success = self.perform_rotation(provider_name)\n            results[provider_name] = success\n            \n            if success:\n                self.log(f\"✅ {provider_name} working\")\n            else:\n                self.log(f\"❌ {provider_name} failed\")\n            \n            time.sleep(10)  # Brief delay between tests\n        \n        return results\n\ndef main():\n    import argparse\n    \n    parser = argparse.ArgumentParser(description=\"Advanced VPN IP Rotation\")\n    parser.add_argument('--rotate', action='store_true', help='Perform single rotation')\n    parser.add_argument('--stealth', action='store_true', help='Enter stealth mode')\n    parser.add_argument('--scheduled', action='store_true', help='Run scheduled rotation')\n    parser.add_argument('--test', action='store_true', help='Test all providers')\n    parser.add_argument('--provider', help='Use specific provider')\n    parser.add_argument('--setup', action='store_true', help='Setup configuration')\n    \n    args = parser.parse_args()\n    \n    vpn_manager = VPNRotationManager()\n    \n    if args.setup:\n        vpn_manager.save_secrets()\n        print(\"Configuration setup complete. Edit secrets.json with your credentials.\")\n    elif args.test:\n        vpn_manager.test_all_providers()\n    elif args.stealth:\n        vpn_manager.stealth_mode_rotation()\n    elif args.scheduled:\n        vpn_manager.scheduled_rotation()\n    elif args.rotate:\n        vpn_manager.perform_rotation(args.provider)\n    else:\n        print(\"Advanced VPN Rotation Manager\")\n        print(\"Usage: python3 advanced-vpn-rotation.py [--rotate|--stealth|--scheduled|--test]\")\n\nif __name__ == \"__main__\":\n    main()
