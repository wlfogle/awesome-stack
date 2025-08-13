#!/usr/bin/env python3
"""
Secure Secrets Management System
Handles encryption, storage, and management of sensitive data
"""

import os
import json
import base64
import hashlib
from pathlib import Path
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC

class SecretsManager:
    def __init__(self, master_password=None):
        self.secrets_dir = Path.home() / ".config" / "secrets"
        self.secrets_dir.mkdir(parents=True, exist_ok=True)
        
        self.master_key_file = self.secrets_dir / "master.key"
        self.secrets_file = self.secrets_dir / "secrets.enc"
        
        self.master_password = master_password or os.getenv("SECRETS_MASTER_PASSWORD")
        if not self.master_password:
            self.master_password = self.prompt_master_password()
        
        self.cipher = self._get_cipher()
        
    def prompt_master_password(self):
        """Prompt for master password"""
        import getpass
        return getpass.getpass("Enter master password for secrets: ")
    
    def _derive_key(self, password, salt):
        """Derive encryption key from password"""
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=32,
            salt=salt,
            iterations=100000,
        )
        return base64.urlsafe_b64encode(kdf.derive(password.encode()))
    
    def _get_cipher(self):
        """Get or create cipher for encryption/decryption"""
        if self.master_key_file.exists():
            # Load existing salt
            with open(self.master_key_file, 'rb') as f:
                salt = f.read()
        else:
            # Generate new salt
            salt = os.urandom(16)
            with open(self.master_key_file, 'wb') as f:
                f.write(salt)
            os.chmod(self.master_key_file, 0o600)
        
        key = self._derive_key(self.master_password, salt)
        return Fernet(key)
    
    def encrypt_data(self, data):
        """Encrypt data"""
        if isinstance(data, dict):
            data = json.dumps(data)
        if isinstance(data, str):
            data = data.encode()
        return self.cipher.encrypt(data)
    
    def decrypt_data(self, encrypted_data):
        """Decrypt data"""
        try:
            decrypted = self.cipher.decrypt(encrypted_data)
            return decrypted.decode()
        except Exception as e:
            print(f"Decryption failed: {e}")
            return None
    
    def save_secrets(self, secrets_dict):
        """Save encrypted secrets"""
        try:
            encrypted = self.encrypt_data(secrets_dict)
            with open(self.secrets_file, 'wb') as f:
                f.write(encrypted)
            os.chmod(self.secrets_file, 0o600)
            return True
        except Exception as e:
            print(f"Failed to save secrets: {e}")
            return False
    
    def load_secrets(self):
        """Load and decrypt secrets"""
        try:
            if not self.secrets_file.exists():
                return {}
            
            with open(self.secrets_file, 'rb') as f:
                encrypted = f.read()
            
            decrypted = self.decrypt_data(encrypted)
            if decrypted:
                return json.loads(decrypted)
        except Exception as e:
            print(f"Failed to load secrets: {e}")
        
        return {}
    
    def set_secret(self, category, key, value):
        """Set a secret value"""
        secrets = self.load_secrets()
        
        if category not in secrets:
            secrets[category] = {}
        
        secrets[category][key] = value
        
        return self.save_secrets(secrets)
    
    def get_secret(self, category, key, default=None):
        """Get a secret value"""
        secrets = self.load_secrets()
        return secrets.get(category, {}).get(key, default)
    
    def delete_secret(self, category, key=None):
        """Delete a secret or entire category"""
        secrets = self.load_secrets()
        
        if category in secrets:
            if key:
                secrets[category].pop(key, None)
                if not secrets[category]:  # Remove empty category
                    del secrets[category]
            else:
                del secrets[category]
            
            return self.save_secrets(secrets)
        
        return False
    
    def list_secrets(self):
        """List all secret categories and keys (not values)"""
        secrets = self.load_secrets()
        result = {}
        
        for category, items in secrets.items():
            result[category] = list(items.keys()) if isinstance(items, dict) else ['<encrypted>']
        
        return result
    
    def export_secrets(self, output_file, include_values=False):
        """Export secrets to file"""
        try:
            if include_values:
                data = self.load_secrets()
            else:
                data = self.list_secrets()
            
            with open(output_file, 'w') as f:
                json.dump(data, f, indent=2)
            
            os.chmod(output_file, 0o600)
            return True
        except Exception as e:
            print(f"Failed to export secrets: {e}")
            return False
    
    def import_secrets(self, input_file):
        """Import secrets from file"""
        try:
            with open(input_file, 'r') as f:
                data = json.load(f)
            
            return self.save_secrets(data)
        except Exception as e:
            print(f"Failed to import secrets: {e}")
            return False
    
    def change_master_password(self, new_password):
        """Change master password"""
        try:
            # Load secrets with old password
            secrets = self.load_secrets()
            
            # Update password and regenerate cipher
            self.master_password = new_password
            
            # Remove old key file
            if self.master_key_file.exists():
                self.master_key_file.unlink()
            
            # Create new cipher
            self.cipher = self._get_cipher()
            
            # Save secrets with new password
            return self.save_secrets(secrets)
            
        except Exception as e:
            print(f"Failed to change master password: {e}")
            return False

def setup_default_secrets():
    """Setup default secrets structure"""
    secrets_manager = SecretsManager()
    
    default_secrets = {
        "warp": {
            "email": "your-warp-email@example.com",
            "password": "your-warp-password",
            "api_key": "your-warp-api-key"
        },
        "vpn": {
            "wireguard_server_key": "your-wireguard-private-key",
            "wireguard_api_endpoint": "https://your-server.com/api",
            "wireguard_auth_token": "your-auth-token"
        },
        "github": {
            "personal_access_token": "your-github-pat",
            "username": "your-github-username"
        },
        "api_keys": {
            "openai": "your-openai-api-key",
            "claude": "your-claude-api-key",
            "proxy_service": "your-proxy-api-key"
        },
        "credentials": {
            "ssh_private_key": "path-to-your-ssh-key",
            "sudo_password": "your-sudo-password"
        }
    }
    
    return secrets_manager.save_secrets(default_secrets)

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Secure Secrets Manager")
    parser.add_argument('--set', nargs=3, metavar=('CATEGORY', 'KEY', 'VALUE'), 
                       help='Set a secret value')
    parser.add_argument('--get', nargs=2, metavar=('CATEGORY', 'KEY'), 
                       help='Get a secret value')
    parser.add_argument('--delete', nargs='1-2', metavar=('CATEGORY', 'KEY'), 
                       help='Delete a secret or category')
    parser.add_argument('--list', action='store_true', help='List all secrets')
    parser.add_argument('--export', metavar='FILE', help='Export secrets to file')
    parser.add_argument('--import', metavar='FILE', help='Import secrets from file')
    parser.add_argument('--setup', action='store_true', help='Setup default secrets')
    parser.add_argument('--change-password', action='store_true', 
                       help='Change master password')
    
    args = parser.parse_args()
    
    try:
        if args.setup:
            if setup_default_secrets():
                print("✅ Default secrets setup complete. Edit with real values.")
            else:
                print("❌ Failed to setup default secrets")
                
        elif args.change_password:
            secrets_manager = SecretsManager()
            new_password = input("Enter new master password: ")
            if secrets_manager.change_master_password(new_password):
                print("✅ Master password changed successfully")
            else:
                print("❌ Failed to change master password")
                
        elif args.set:
            secrets_manager = SecretsManager()
            category, key, value = args.set
            if secrets_manager.set_secret(category, key, value):
                print(f"✅ Secret {category}.{key} set successfully")
            else:
                print(f"❌ Failed to set secret {category}.{key}")
                
        elif args.get:
            secrets_manager = SecretsManager()
            category, key = args.get
            value = secrets_manager.get_secret(category, key)
            if value:
                print(f"{category}.{key}: {value}")
            else:
                print(f"Secret {category}.{key} not found")
                
        elif args.list:
            secrets_manager = SecretsManager()
            secrets = secrets_manager.list_secrets()
            print("Available secrets:")
            for category, keys in secrets.items():
                print(f"  {category}:")
                for key in keys:
                    print(f"    - {key}")
                    
        elif getattr(args, 'export', None):
            secrets_manager = SecretsManager()
            include_values = input("Include secret values? (y/N): ").lower() == 'y'
            if secrets_manager.export_secrets(args.export, include_values):
                print(f"✅ Secrets exported to {args.export}")
            else:
                print("❌ Failed to export secrets")
                
        elif getattr(args, 'import', None):
            secrets_manager = SecretsManager()
            if secrets_manager.import_secrets(getattr(args, 'import')):
                print("✅ Secrets imported successfully")
            else:
                print("❌ Failed to import secrets")
                
        else:
            print("Secure Secrets Manager")
            print("Usage: python3 secrets-manager.py [--setup|--set|--get|--list|--export|--import]")
            
    except KeyboardInterrupt:
        print("\n❌ Operation cancelled")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()
