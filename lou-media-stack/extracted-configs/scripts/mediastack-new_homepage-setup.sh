#!/bin/bash

# Homepage Setup Script for MediaStack
# This script helps configure and start the Homepage dashboard service

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="$SCRIPT_DIR/config/homepage"
ENV_FILE="$CONFIG_DIR/.env"

echo "🏠 Homepage Configuration Script"
echo "================================"

# Check if Homepage service exists in docker-compose
if ! grep -q "homepage:" "$SCRIPT_DIR/docker-compose.yml"; then
    echo "❌ Homepage service not found in docker-compose.yml"
    echo "Please add the Homepage service to your docker-compose.yml file first."
    exit 1
fi

# Create config directory if it doesn't exist
mkdir -p "$CONFIG_DIR"

echo "📁 Config directory: $CONFIG_DIR"

# Function to prompt for API key
prompt_for_key() {
    local service_name="$1"
    local var_name="$2"
    local current_value=$(grep "^$var_name=" "$ENV_FILE" 2>/dev/null | cut -d'=' -f2)
    
    if [[ "$current_value" == "your_${service_name,,}_"* ]] || [[ -z "$current_value" ]]; then
        echo -n "Enter $service_name API key (or press Enter to skip): "
        read -r new_value
        if [[ -n "$new_value" ]]; then
            sed -i "s/^$var_name=.*/$var_name=$new_value/" "$ENV_FILE"
            echo "✅ Updated $service_name API key"
        else
            echo "⏭️  Skipped $service_name API key"
        fi
    else
        echo "✅ $service_name API key already configured"
    fi
}

# Check if .env file exists
if [[ ! -f "$ENV_FILE" ]]; then
    echo "❌ Environment file not found at $ENV_FILE"
    echo "Please create the .env file first."
    exit 1
fi

echo ""
echo "🔑 API Key Configuration"
echo "========================"
echo "Configure API keys for your services (you can skip any and configure later):"
echo ""

# Configure important API keys
prompt_for_key "Jellyfin" "HOMEPAGE_VAR_JELLYFIN_API_KEY"
prompt_for_key "Plex" "HOMEPAGE_VAR_PLEX_API_KEY"
prompt_for_key "Sonarr" "HOMEPAGE_VAR_SONARR_API_KEY"
prompt_for_key "Radarr" "HOMEPAGE_VAR_RADARR_API_KEY"
prompt_for_key "Lidarr" "HOMEPAGE_VAR_LIDARR_API_KEY"
prompt_for_key "Prowlarr" "HOMEPAGE_VAR_PROWLARR_API_KEY"
prompt_for_key "Jackett" "HOMEPAGE_VAR_JACKETT_API_KEY"
prompt_for_key "Overseerr" "HOMEPAGE_VAR_OVERSEERR_API_KEY"
prompt_for_key "Tautulli" "HOMEPAGE_VAR_TAUTULLI_API_KEY"
prompt_for_key "Portainer" "HOMEPAGE_VAR_PORTAINER_API_KEY"

echo ""
echo "🚀 Starting Homepage Service"
echo "============================="

# Start Homepage service
if docker-compose -f "$SCRIPT_DIR/docker-compose.yml" up -d homepage; then
    echo "✅ Homepage service started successfully!"
    echo ""
    echo "🌐 Access your Homepage dashboard at:"
    echo "   - Local: http://localhost:3000"
    echo "   - Traefik: https://homepage.your-domain.com"
    echo ""
    echo "📚 Additional Configuration:"
    echo "   - Edit services: $CONFIG_DIR/services.yaml"
    echo "   - Update API keys: $CONFIG_DIR/.env"
    echo "   - View logs: docker-compose logs -f homepage"
else
    echo "❌ Failed to start Homepage service"
    exit 1
fi

echo ""
echo "💡 Tips:"
echo "   - API keys can be found in each service's settings/configuration page"
echo "   - Services without API keys will still appear but won't show live data"
echo "   - Restart Homepage after updating API keys: docker-compose restart homepage"
echo "   - Check Homepage logs for any connection issues"
echo ""
echo "🎉 Homepage setup complete!"
