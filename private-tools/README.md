# 🔐 Private Automation Tools

**⚠️ SENSITIVE CONTENT - PRIVATE BRANCH ONLY**

This directory contains automation scripts, credentials, and sensitive tools that should NEVER be committed to public repositories.

## 🛡️ Security Notice

- **Private Branch Only**: These tools are only available on the `private-automation` branch
- **Encrypted Secrets**: All sensitive data is encrypted using military-grade encryption
- **Local Only**: These scripts are designed to run on trusted local systems only
- **No Public Exposure**: Never commit API keys, passwords, or tokens to public repos

## 📁 Directory Structure

```
private-tools/
├── warp-automation/      # Warp Terminal auto-login tools
├── vpn-rotation/         # Advanced VPN IP rotation system
├── secrets/              # Encrypted secrets management
└── README.md            # This file
```

## 🔧 Tools Overview

### 1. **Warp Automation** (`warp-automation/`)

Automated authentication and session management for Warp Terminal.

**Features:**
- Automatic login with stored credentials
- Token refresh and session management
- Systemd service integration
- Auto-start on boot configuration

**Usage:**
```bash
cd warp-automation
python3 warp-auto-login.py --setup    # Initial setup
python3 warp-auto-login.py --service  # Install systemd service
python3 warp-auto-login.py --login    # Manual login
```

**Environment Variables:**
```bash
export WARP_EMAIL="your-email@example.com"
export WARP_PASSWORD="your-password"
export WARP_API_KEY="your-api-key"
```

### 2. **VPN Rotation** (`vpn-rotation/`)

Advanced multi-provider VPN IP rotation system for maximum privacy.

**Features:**
- Multiple VPN provider support (WireGuard, NordVPN, etc.)
- Automated IP rotation on schedule
- Stealth mode for aggressive rotation
- API-based provider management
- Rotation history and logging

**Usage:**
```bash
cd vpn-rotation
python3 advanced-vpn-rotation.py --setup      # Initial configuration
python3 advanced-vpn-rotation.py --rotate     # Single rotation
python3 advanced-vpn-rotation.py --stealth    # Stealth mode
python3 advanced-vpn-rotation.py --scheduled  # Scheduled rotation
```

**Supported Providers:**
- **WireGuard**: Custom server with API rotation
- **Custom Proxy**: Rotating residential proxies
- **NordVPN**: (Configuration required)
- **ExpressVPN**: (Configuration required)

### 3. **Secrets Management** (`secrets/`)

Military-grade encrypted secrets storage and management system.

**Features:**
- AES-256 encryption with PBKDF2 key derivation
- Master password protection
- Secure file permissions (600)
- Import/export functionality
- Category-based organization

**Usage:**
```bash
cd secrets
python3 secrets-manager.py --setup                    # Initial setup
python3 secrets-manager.py --set warp email user@example.com
python3 secrets-manager.py --get warp email
python3 secrets-manager.py --list                     # List all secrets
```

## 🚀 Quick Start

### Initial Setup

1. **Clone to private branch:**
   ```bash
   git checkout private-automation
   cd private-tools
   ```

2. **Install dependencies:**
   ```bash
   pip install cryptography requests
   ```

3. **Setup secrets manager:**
   ```bash
   cd secrets
   python3 secrets-manager.py --setup
   ```

4. **Configure Warp automation:**
   ```bash
   cd ../warp-automation
   # Set environment variables first
   export WARP_EMAIL="your-email@example.com"
   export WARP_PASSWORD="your-password"
   python3 warp-auto-login.py --setup
   ```

5. **Setup VPN rotation:**
   ```bash
   cd ../vpn-rotation
   python3 advanced-vpn-rotation.py --setup
   # Edit ~/.config/vpn-rotation/secrets.json with your credentials
   ```

### Systemd Services

Enable automatic operation with systemd:

```bash
# Warp auto-login
python3 warp-automation/warp-auto-login.py --service

# VPN rotation (scheduled)
sudo cp vpn-rotation/vpn-rotation.service /etc/systemd/system/
sudo systemctl enable --now vpn-rotation.service
```

## ⚙️ Configuration Files

### Secrets Configuration
Located at `~/.config/secrets/secrets.enc` (encrypted)

### VPN Rotation Config
Located at `~/.config/vpn-rotation/`
- `secrets.json` - API keys and credentials
- `providers.json` - Provider configurations
- `rotation.log` - Rotation history

### Warp Configuration
Located at `~/.config/warp-automation/`
- `warp_token.json` - Authentication tokens
- `warp_session.json` - Session data

## 🔒 Security Best Practices

### Secrets Management
- Always use encrypted storage for sensitive data
- Never commit real credentials to version control
- Use environment variables for temporary secrets
- Regularly rotate API keys and passwords

### File Permissions
```bash
chmod 600 ~/.config/secrets/*
chmod 600 ~/.config/vpn-rotation/secrets.json
chmod 700 ~/.config/warp-automation/
```

### Network Security
- Only use trusted networks for initial setup
- Verify VPN connections before transmitting sensitive data
- Monitor rotation logs for suspicious activity

## 🚨 Emergency Procedures

### Compromised Credentials
1. **Immediate Actions:**
   ```bash
   # Stop all automation services
   systemctl --user stop warp-auto-login.service
   sudo systemctl stop vpn-rotation.service
   
   # Clear stored tokens
   rm -rf ~/.config/warp-automation/*.json
   rm -rf ~/.config/vpn-rotation/secrets.json
   ```

2. **Reset and Rotate:**
   - Change all passwords and API keys
   - Generate new VPN configurations
   - Re-encrypt secrets with new master password

### Service Recovery
```bash
# Check service status
systemctl --user status warp-auto-login.service
sudo systemctl status vpn-rotation.service

# View logs
journalctl --user -u warp-auto-login.service -f
sudo journalctl -u vpn-rotation.service -f

# Restart services
systemctl --user restart warp-auto-login.service
sudo systemctl restart vpn-rotation.service
```

## 📊 Monitoring and Logging

### Log Locations
- **VPN Rotation**: `~/.config/vpn-rotation/rotation.log`
- **Warp Automation**: `journalctl --user -u warp-auto-login.service`
- **Secrets Manager**: Local stderr output

### Health Checks
```bash
# Check current IP
curl -s https://api.ipify.org

# Test VPN rotation
python3 vpn-rotation/advanced-vpn-rotation.py --test

# Verify Warp authentication
python3 warp-automation/warp-auto-login.py --login
```

## 🔧 Troubleshooting

### Common Issues

**Warp Login Fails:**
- Check environment variables
- Verify API endpoints
- Check network connectivity

**VPN Rotation Stuck:**
- Check provider API credentials
- Verify network interfaces
- Review rotation logs

**Secrets Decryption Failed:**
- Verify master password
- Check file permissions
- Ensure encryption key file exists

### Reset Procedures
```bash
# Full reset (USE WITH CAUTION)
rm -rf ~/.config/warp-automation/
rm -rf ~/.config/vpn-rotation/
rm -rf ~/.config/secrets/

# Re-run setup
python3 secrets/secrets-manager.py --setup
python3 warp-automation/warp-auto-login.py --setup
python3 vpn-rotation/advanced-vpn-rotation.py --setup
```

## 📋 Dependencies

```bash
pip install -r requirements.txt
```

**Required packages:**
- `cryptography` - Encryption/decryption
- `requests` - HTTP API calls
- `psutil` - System monitoring
- `schedule` - Task scheduling (optional)

## 🤝 Contributing

**IMPORTANT**: Never commit sensitive data to version control.

1. Always work on the `private-automation` branch
2. Use placeholder values in example configurations
3. Test thoroughly in isolated environments
4. Document security considerations

## ⚠️ Disclaimer

These tools are for authorized use only. Users are responsible for:
- Complying with all applicable laws and regulations
- Securing their own credentials and API keys
- Using VPN services in accordance with their terms of service
- Maintaining proper operational security

---

**Last Updated**: August 13, 2025  
**Security Review**: Required before each release  
**Branch**: `private-automation` (NEVER merge to main)
