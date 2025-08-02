#!/bin/bash

# WireGuard Client Configuration Generator
# Usage: ./generate-client-configs.sh [client-name] [optional-endpoint]

set -e

# Configuration
SERVER_CONFIG="/etc/wireguard/wg0.conf"
WIREGUARD_DIR="/etc/wireguard"
CLIENT_DIR="/home/lou/wireguard-vpn/clients"
SERVER_PUBLIC_KEY=$(cat server_public_key)
SERVER_ENDPOINT="${2:-172.59.82.13:51820}"  # Use provided endpoint or default to current public IP
VPN_SUBNET="10.200.200.0/24"
DNS_SERVERS="1.1.1.1, 8.8.8.8"

# Create client directory if it doesn't exist
mkdir -p "$CLIENT_DIR"

# Function to get next available IP
get_next_ip() {
    local used_ips=$(sudo grep -o "AllowedIPs = 10\.200\.200\.[0-9]\+/32" "$SERVER_CONFIG" | grep -o "[0-9]\+/32" | grep -o "[0-9]\+" | sort -n)
    local next_ip=2
    for ip in $used_ips; do
        if [ "$ip" -eq "$next_ip" ]; then
            next_ip=$((next_ip + 1))
        else
            break
        fi
    done
    echo "$next_ip"
}

# Function to generate client config
generate_client_config() {
    local client_name="$1"
    local client_ip="$2"
    local private_key="$3"
    local public_key="$4"
    
    cat > "$CLIENT_DIR/${client_name}.conf" <<EOF
[Interface]
PrivateKey = $private_key
Address = 10.200.200.$client_ip/24
DNS = $DNS_SERVERS

[Peer]
PublicKey = $SERVER_PUBLIC_KEY
Endpoint = $SERVER_ENDPOINT
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
EOF

    # Generate QR code for mobile devices
    if command -v qrencode &> /dev/null; then
        qrencode -t ansiutf8 < "$CLIENT_DIR/${client_name}.conf" > "$CLIENT_DIR/${client_name}-qr.txt"
        echo "QR code saved to: $CLIENT_DIR/${client_name}-qr.txt"
    fi
}

# Function to add peer to server config
add_peer_to_server() {
    local client_name="$1"
    local client_ip="$2"
    local public_key="$3"
    
    # Check if peer already exists
    if sudo grep -q "# $client_name" "$SERVER_CONFIG"; then
        echo "Warning: Client '$client_name' already exists in server config"
        return 1
    fi
    
    # Add peer to server config
    sudo tee -a "$SERVER_CONFIG" > /dev/null <<EOF

[Peer]
# $client_name
PublicKey = $public_key
AllowedIPs = 10.200.200.$client_ip/32
EOF
}

# Function to create client
create_client() {
    local client_name="$1"
    
    if [[ -z "$client_name" ]]; then
        echo "Usage: $0 <client-name> [endpoint]"
        echo "Example: $0 smartphone"
        echo "Example: $0 laptop mydomain.com:51820"
        exit 1
    fi
    
    # Validate client name
    if [[ ! "$client_name" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "Error: Client name must contain only alphanumeric characters, hyphens, and underscores"
        exit 1
    fi
    
    # Check if client already exists
    if [[ -f "$CLIENT_DIR/${client_name}.conf" ]]; then
        echo "Error: Client '$client_name' already exists"
        exit 1
    fi
    
    # Generate keys
    local private_key=$(wg genkey)
    local public_key=$(echo "$private_key" | wg pubkey)
    
    # Get next available IP
    local client_ip=$(get_next_ip)
    
    echo "Creating client: $client_name"
    echo "Assigned IP: 10.200.200.$client_ip"
    echo "Server endpoint: $SERVER_ENDPOINT"
    
    # Generate client config
    generate_client_config "$client_name" "$client_ip" "$private_key" "$public_key"
    
    # Add peer to server config
    if add_peer_to_server "$client_name" "$client_ip" "$public_key"; then
        echo "Client configuration created: $CLIENT_DIR/${client_name}.conf"
        
        # Restart WireGuard to apply changes
        sudo systemctl reload wg-quick@wg0
        
        echo "WireGuard server reloaded with new client configuration"
        echo ""
        echo "To connect this client:"
        echo "1. Copy the config file to your device: $CLIENT_DIR/${client_name}.conf"
        if [[ -f "$CLIENT_DIR/${client_name}-qr.txt" ]]; then
            echo "2. Or scan the QR code: $CLIENT_DIR/${client_name}-qr.txt"
        fi
        echo "3. Import the configuration into your WireGuard client"
        echo ""
        echo "Client configuration:"
        echo "===================="
        cat "$CLIENT_DIR/${client_name}.conf"
        echo "===================="
        
        if [[ -f "$CLIENT_DIR/${client_name}-qr.txt" ]]; then
            echo ""
            echo "QR Code for mobile devices:"
            cat "$CLIENT_DIR/${client_name}-qr.txt"
        fi
    else
        # Clean up if adding to server failed
        rm -f "$CLIENT_DIR/${client_name}.conf"
        rm -f "$CLIENT_DIR/${client_name}-qr.txt"
        exit 1
    fi
}

# Function to list clients
list_clients() {
    echo "Current WireGuard clients:"
    echo "========================="
    if [[ -d "$CLIENT_DIR" ]] && [[ "$(ls -A "$CLIENT_DIR"/*.conf 2>/dev/null)" ]]; then
        for config in "$CLIENT_DIR"/*.conf; do
            if [[ -f "$config" ]]; then
                local client_name=$(basename "$config" .conf)
                local client_ip=$(grep "Address" "$config" | cut -d' ' -f3 | cut -d'/' -f1)
                echo "  $client_name - $client_ip"
            fi
        done
    else
        echo "  No clients configured"
    fi
    echo ""
    echo "Server status:"
    sudo wg show
}

# Function to remove client
remove_client() {
    local client_name="$1"
    
    if [[ -z "$client_name" ]]; then
        echo "Usage: $0 remove <client-name>"
        exit 1
    fi
    
    if [[ ! -f "$CLIENT_DIR/${client_name}.conf" ]]; then
        echo "Error: Client '$client_name' not found"
        exit 1
    fi
    
    # Remove client config files
    rm -f "$CLIENT_DIR/${client_name}.conf"
    rm -f "$CLIENT_DIR/${client_name}-qr.txt"
    
    # Remove from server config
    sudo sed -i "/# $client_name/,/^$/d" "$SERVER_CONFIG"
    
    # Restart WireGuard
    sudo systemctl reload wg-quick@wg0
    
    echo "Client '$client_name' removed successfully"
}

# Main script logic
case "${1:-create}" in
    "list")
        list_clients
        ;;
    "remove")
        remove_client "$2"
        ;;
    "create"|*)
        if [[ "$1" == "create" ]]; then
            create_client "$2"
        else
            create_client "$1"
        fi
        ;;
esac
