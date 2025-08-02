#!/bin/bash

echo "Switching to X11 session..."

# Set the default session to X11 in SDDM config
sudo mkdir -p /etc/sddm.conf.d/

# Create SDDM config to default to X11
sudo tee /etc/sddm.conf.d/default-x11.conf > /dev/null << EOF
[General]
Session=plasma
DisplayServer=x11

[X11]
Session=plasma
EOF

# Also set user's default session
echo "plasma" > ~/.dmrc

# Set environment variable for next login
echo "export XDG_SESSION_TYPE=x11" >> ~/.profile

echo "X11 configuration complete!"
echo "Please log out and log back in to use X11 session."
echo "You can also reboot to ensure all changes take effect."

# Optional: Kill current Wayland session to force restart
read -p "Do you want to restart your session now? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Restarting session..."
    loginctl terminate-user $(whoami)
fi
