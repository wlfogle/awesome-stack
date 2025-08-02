#!/bin/bash

# Extract and Consolidate MediaStack Configurations
# This script gathers all valuable configuration materials from previous setups

echo "🔍 Extracting MediaStack Configuration Materials..."
echo "================================================="

# Define source directories
SOURCE_DIRS=(
    "/home/lou/mediastack-new"
    "/home/lou/MEDIASTACK_UNIFIED"
    "/home/lou/MEDIASTACK_NEW"
    "/home/lou/Media_Stack"
    "/home/lou/arr_stack"
    "/mnt/sda1"
    "/mnt/sdb1"
)

# Define target directories
TARGET_DIR="/home/lou/lou-media-stack"
EXTRACTED_DIR="$TARGET_DIR/extracted-configs"
BACKUP_DIR="$TARGET_DIR/config-backups"

# Create target directories
mkdir -p "$EXTRACTED_DIR"/{env-files,docker-configs,api-keys,service-configs,json-configs,scripts}
mkdir -p "$BACKUP_DIR"

echo "📁 Created extraction directories"

# Function to extract API keys and tokens from files
extract_api_keys() {
    local source_dir="$1"
    local output_file="$EXTRACTED_DIR/api-keys/extracted-keys.txt"
    
    echo "🔑 Extracting API keys from $source_dir..."
    
    # Search for common API key patterns
    find "$source_dir" -type f \( -name "*.xml" -o -name "*.json" -o -name "*.env" -o -name "*.yaml" -o -name "*.yml" \) 2>/dev/null | while read -r file; do
        if [[ -r "$file" ]]; then
            # Extract various API key patterns
            grep -H -i "api.*key\|token\|secret\|password" "$file" 2>/dev/null | grep -v "example\|placeholder" >> "$output_file" 2>/dev/null
        fi
    done
}

# Function to copy configuration files
copy_configs() {
    local source_dir="$1"
    local source_name=$(basename "$source_dir")
    
    echo "📋 Processing $source_name..."
    
    if [[ ! -d "$source_dir" ]]; then
        echo "⚠️  Directory $source_dir not found, skipping..."
        return
    fi
    
    # Create backup directory for this source
    mkdir -p "$BACKUP_DIR/$source_name"
    
    # Copy .env files
    find "$source_dir" -name "*.env" -type f 2>/dev/null | while read -r file; do
        if [[ -r "$file" ]]; then
            cp "$file" "$EXTRACTED_DIR/env-files/${source_name}_$(basename "$file")" 2>/dev/null
            echo "✅ Copied $(basename "$file") from $source_name"
        fi
    done
    
    # Copy Docker Compose files
    find "$source_dir" -name "docker-compose*.yml" -o -name "docker-compose*.yaml" -o -name "compose*.yml" -type f 2>/dev/null | while read -r file; do
        if [[ -r "$file" ]]; then
            cp "$file" "$EXTRACTED_DIR/docker-configs/${source_name}_$(basename "$file")" 2>/dev/null
            echo "✅ Copied $(basename "$file") from $source_name"
        fi
    done
    
    # Copy JSON configuration files
    find "$source_dir" -name "*.json" -type f 2>/dev/null | while read -r file; do
        if [[ -r "$file" ]]; then
            # Skip node_modules and other irrelevant directories
            if [[ "$file" != *"node_modules"* && "$file" != *"cache"* ]]; then
                relative_path=$(realpath --relative-to="$source_dir" "$file")
                mkdir -p "$EXTRACTED_DIR/json-configs/$source_name/$(dirname "$relative_path")"
                cp "$file" "$EXTRACTED_DIR/json-configs/$source_name/$relative_path" 2>/dev/null
                echo "✅ Copied JSON: $relative_path from $source_name"
            fi
        fi
    done
    
    # Copy service configuration directories
    for service_dir in "$source_dir"/appdata/*/; do
        if [[ -d "$service_dir" ]]; then
            service_name=$(basename "$service_dir")
            mkdir -p "$EXTRACTED_DIR/service-configs/$source_name/$service_name"
            
            # Copy important config files
            find "$service_dir" -name "*.xml" -o -name "*.json" -o -name "*.yaml" -o -name "*.yml" -o -name "*.conf" -o -name "*.cfg" -type f 2>/dev/null | while read -r config_file; do
                if [[ -r "$config_file" ]]; then
                    relative_path=$(realpath --relative-to="$service_dir" "$config_file")
                    mkdir -p "$EXTRACTED_DIR/service-configs/$source_name/$service_name/$(dirname "$relative_path")"
                    cp "$config_file" "$EXTRACTED_DIR/service-configs/$source_name/$service_name/$relative_path" 2>/dev/null
                    echo "✅ Copied $service_name config: $relative_path"
                fi
            done
        fi
    done
    
    # Copy scripts
    find "$source_dir" -name "*.sh" -type f 2>/dev/null | while read -r script; do
        if [[ -r "$script" ]]; then
            cp "$script" "$EXTRACTED_DIR/scripts/${source_name}_$(basename "$script")" 2>/dev/null
            echo "✅ Copied script: $(basename "$script") from $source_name"
        fi
    done
    
    # Extract API keys from this directory
    extract_api_keys "$source_dir"
}

# Process each source directory
for source_dir in "${SOURCE_DIRS[@]}"; do
    copy_configs "$source_dir"
done

echo ""
echo "🔍 Searching for specific valuable items..."

# Search for Plex tokens specifically
echo "📺 Looking for Plex tokens..."
find "${SOURCE_DIRS[@]}" -type f \( -name "*.xml" -o -name "*.json" -o -name "*.env" \) -exec grep -l "PlexOnlineToken\|X-Plex-Token\|plex.*token" {} \; 2>/dev/null | while read -r file; do
    if [[ -r "$file" ]]; then
        echo "🎬 Found Plex token in: $file"
        grep "PlexOnlineToken\|X-Plex-Token\|plex.*token" "$file" >> "$EXTRACTED_DIR/api-keys/plex-tokens.txt" 2>/dev/null
    fi
done

# Search for Sonarr/Radarr/Lidarr API keys
echo "🎯 Looking for *arr service API keys..."
find "${SOURCE_DIRS[@]}" -name "config.xml" -type f 2>/dev/null | while read -r config_file; do
    if [[ -r "$config_file" ]]; then
        service_path=$(dirname "$config_file")
        service_name=$(basename "$(dirname "$service_path")")
        
        api_key=$(grep -o '<ApiKey>[^<]*' "$config_file" 2>/dev/null | sed 's/<ApiKey>//')
        if [[ -n "$api_key" ]]; then
            echo "🔑 Found $service_name API key: $api_key"
            echo "$service_name: $api_key" >> "$EXTRACTED_DIR/api-keys/arr-api-keys.txt"
        fi
    fi
done

# Search for Jackett API keys
echo "🕷️  Looking for Jackett API keys..."
find "${SOURCE_DIRS[@]}" -name "ServerConfig.json" -type f 2>/dev/null | while read -r jackett_config; do
    if [[ -r "$jackett_config" ]]; then
        api_key=$(grep -o '"APIKey":"[^"]*' "$jackett_config" 2>/dev/null | sed 's/"APIKey":"//')
        if [[ -n "$api_key" ]]; then
            echo "🔍 Found Jackett API key: $api_key"
            echo "Jackett: $api_key" >> "$EXTRACTED_DIR/api-keys/jackett-api-keys.txt"
        fi
    fi
done

# Create summary report
echo ""
echo "📊 Creating summary report..."
SUMMARY_FILE="$EXTRACTED_DIR/extraction-summary.txt"

cat > "$SUMMARY_FILE" << EOF
MediaStack Configuration Extraction Summary
==========================================
Generated: $(date)

Directories processed:
$(for dir in "${SOURCE_DIRS[@]}"; do echo "  - $dir"; done)

Files extracted:
  - Environment files: $(find "$EXTRACTED_DIR/env-files" -type f 2>/dev/null | wc -l)
  - Docker Compose files: $(find "$EXTRACTED_DIR/docker-configs" -type f 2>/dev/null | wc -l)
  - JSON configurations: $(find "$EXTRACTED_DIR/json-configs" -type f 2>/dev/null | wc -l)
  - Service configurations: $(find "$EXTRACTED_DIR/service-configs" -type f 2>/dev/null | wc -l)
  - Scripts: $(find "$EXTRACTED_DIR/scripts" -type f 2>/dev/null | wc -l)

API Keys found:
  - Plex tokens: $(grep -c "token" "$EXTRACTED_DIR/api-keys/plex-tokens.txt" 2>/dev/null || echo "0")
  - Arr service keys: $(wc -l < "$EXTRACTED_DIR/api-keys/arr-api-keys.txt" 2>/dev/null || echo "0")
  - Jackett keys: $(wc -l < "$EXTRACTED_DIR/api-keys/jackett-api-keys.txt" 2>/dev/null || echo "0")

Next steps:
1. Review extracted configurations in: $EXTRACTED_DIR
2. Consolidate environment variables
3. Merge Docker Compose configurations
4. Update API keys in new setup
5. Test service configurations

EOF

echo "✅ Configuration extraction complete!"
echo "📁 Extracted files are in: $EXTRACTED_DIR"
echo "📋 Summary report: $SUMMARY_FILE"
echo ""
echo "🚀 Run the consolidation script next to merge configurations:"
echo "   ./scripts/consolidate-configs.sh"
