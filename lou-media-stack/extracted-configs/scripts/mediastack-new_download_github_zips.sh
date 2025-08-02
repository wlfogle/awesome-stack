#!/bin/bash

# Create download directory
mkdir -p /home/lou/mediastack-new/downloads/github-zips

# Function to check if repo already has a ZIP file
check_repo_exists() {
    local repo="$1"
    local safe_name=$(echo "$repo" | sed 's|/|_|g')
    
    # Check if any ZIP file exists for this repo
    if [ -f "${safe_name}_main.zip" ] || [ -f "${safe_name}_master.zip" ] || [ -f "${safe_name}_dev.zip" ]; then
        return 0  # exists
    else
        return 1  # does not exist
    fi
}

# Initialize counters
downloaded=0
skipped=0
failed=0
total=${#repos[@]}

# Array of GitHub repositories
repos=(
"youegraillot/lidarr-on-steroids"
"RandomNinjaAtk/docker-radarr-extended"
"RandomNinjaAtk/docker-sonarr-extended"
"Flexget/Flexget"
"SickGear/SickGear"
"RandomNinjaAtk/arr-scripts"
"dan-online/autopulse"
"Cloudbox/autoscan"
"buildarr/buildarr"
"aetaric/checkrr"
"hrenard/cleanarr"
"se1exin/Cleanarr"
"RiffSphere/Collectarr"
"TMD20/crossarr"
"FrenchGithubUser/Dasharr"
"ManiMatter/decluttarr"
"rfsbraz/deleterr"
"anandslab/deployarr"
"Adman1020/Elsewherr"
"onedr0p/exportarr"
"Flemmarr/Flemmarr"
"plexguide/Huntarr.io"
"Schaka/janitorr"
"angrycuban13/Just-A-Bunch-Of-Starr-Scripts"
"Kometa-Team/Kometa"
"nullable-eth/labelarr"
"Ombi-app/Ombi"
"p-hueber/prefetcharr"
"jamcalli/Pulsarr"
"Fazzani/Proxarr"
"giuseppe99barchetta/SuggestArr"
"l3uddz/traktarr"
"nylonee/watchlistarr"
"JCSynthTux/radarr_autodelete"
"JasonHHouse/gaps"
"Casvt/Plex-scripts"
"causefx/Organizr"
"pir8radio/DownloadViewarr"
"Monitorr/Monitorr"
"ThijmenGThN/swaparr"
)

# Change to download directory
cd /home/lou/mediastack-new/downloads/github-zips

# Download ZIP files from each repository
for repo in "${repos[@]}"; do
    # Check if repo already has a ZIP file
    if check_repo_exists "$repo"; then
        echo "Skipping $repo (already downloaded)"
        skipped=$((skipped + 1))
        continue
    fi

    echo "Downloading ZIP from: $repo"
    
    # Create safe directory name
    safe_name=$(echo "$repo" | sed 's|/|_|g')
    
    # Download main branch ZIP
    wget -O "${safe_name}_main.zip" "https://github.com/${repo}/archive/refs/heads/main.zip" 2>/dev/null
    
    # If main doesn't exist, try master
    if [ ! -s "${safe_name}_main.zip" ]; then
        rm -f "${safe_name}_main.zip"
        wget -O "${safe_name}_master.zip" "https://github.com/${repo}/archive/refs/heads/master.zip" 2>/dev/null
    fi
    
    # If neither exist, try dev
    if [ ! -s "${safe_name}_master.zip" ] && [ ! -s "${safe_name}_main.zip" ]; then
        rm -f "${safe_name}_master.zip"
        wget -O "${safe_name}_dev.zip" "https://github.com/${repo}/archive/refs/heads/dev.zip" 2>/dev/null
    fi
    
    # Check if any download was successful
    if [ -s "${safe_name}_main.zip" ] || [ -s "${safe_name}_master.zip" ] || [ -s "${safe_name}_dev.zip" ]; then
        echo "✓ Successfully downloaded ZIP for $repo"
        downloaded=$((downloaded + 1))
    else
        echo "✗ Failed to download ZIP for $repo"
        failed=$((failed + 1))
        rm -f "${safe_name}_"*.zip
    fi

    echo "---"
done

echo "GitHub ZIP downloads completed!"
echo "Total repositories: $total"
echo "Downloaded: $downloaded"
echo "Skipped: $skipped"
echo "Failed: $failed"
echo "Files saved to: /home/lou/mediastack-new/downloads/github-zips/"
