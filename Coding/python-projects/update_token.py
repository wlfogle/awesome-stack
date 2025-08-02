#!/usr/bin/env python
"""
Simple script to update Home Assistant token in the bridge
"""

import os

print("🔑 Home Assistant Token Updater")
print("================================")
print("")
print("1. Get your HA token from: http://192.168.122.113:8123")
print("2. Profile → Long-Lived Access Tokens → Create Token")
print("3. Enter the token below:")
print("")

token = input("Enter your HA token: ").strip()

if not token:
    print("❌ No token provided!")
    exit(1)

# Read the bridge script
try:
    with open('/sdcard/alexa-bridge/simple_bridge.py', 'r') as f:
        content = f.read()
except FileNotFoundError:
    print("❌ Bridge script not found!")
    exit(1)

# Replace the token
updated_content = content.replace('YOUR_HA_TOKEN_HERE', token)

# Write to home directory  
with open(os.path.expanduser('~/alexa_bridge.py'), 'w') as f:
    f.write(updated_content)

print("✅ Token updated successfully!")
print("🚀 Now run: python alexa_bridge.py")
