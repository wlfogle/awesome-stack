#!/bin/bash

# Create download directory
mkdir -p /home/lou/mediastack-new/downloads

# Array of URLs from wanted.md
urls=(
"https://github.com/youegraillot/lidarr-on-steroids"
"https://github.com/RandomNinjaAtk/docker-radarr-extended"
"https://github.com/RandomNinjaAtk/docker-sonarr-extended"
"https://github.com/Flexget/Flexget"
"https://github.com/SickGear/SickGear"
"https://github.com/RandomNinjaAtk/arr-scripts"
"https://autobrr.com/"
"https://github.com/dan-online/autopulse"
"https://github.com/Cloudbox/autoscan"
"https://github.com/buildarr/buildarr"
"https://github.com/aetaric/checkrr"
"https://github.com/hrenard/cleanarr/"
"https://github.com/se1exin/Cleanarr"
"https://github.com/RiffSphere/Collectarr"
"https://github.com/TMD20/crossarr"
"https://github.com/FrenchGithubUser/Dasharr"
"https://github.com/ManiMatter/decluttarr"
"https://github.com/rfsbraz/deleterr"
"https://github.com/anandslab/deployarr"
"https://github.com/Adman1020/Elsewherr"
"https://github.com/onedr0p/exportarr"
"https://github.com/Flemmarr/Flemmarr"
"https://github.com/plexguide/Huntarr.io"
"https://github.com/Schaka/janitorr"
"https://github.com/angrycuban13/Just-A-Bunch-Of-Starr-Scripts"
"https://github.com/Kometa-Team/Kometa"
"https://github.com/nullable-eth/labelarr"
"https://github.com/Ombi-app/Ombi"
"https://github.com/p-hueber/prefetcharr"
"https://github.com/jamcalli/Pulsarr"
"https://github.com/Fazzani/Proxarr"
"https://github.com/giuseppe99barchetta/SuggestArr"
"https://github.com/l3uddz/traktarr"
"https://github.com/nylonee/watchlistarr"
"https://github.com/JCSynthTux/radarr_autodelete"
"https://github.com/JasonHHouse/gaps"
"https://github.com/Casvt/Plex-scripts"
"https://lazylibrarian.gitlab.io/"
"https://github.com/causefx/Organizr"
"https://github.com/pir8radio/DownloadViewarr"
"https://github.com/Monitorr/Monitorr"
"https://github.com/ThijmenGThN/swaparr"
)

# Change to download directory
cd /home/lou/mediastack-new/downloads

# Download each site to 2 levels depth
for url in "${urls[@]}"; do
    echo "Downloading: $url"
    
    # Extract site name for directory
    site_name=$(echo "$url" | sed 's|https://||' | sed 's|/|_|g' | sed 's|\.|-|g')
    
    # Run httrack with ZIP downloads and improved settings
    httrack "$url" -O "$site_name" -r2 -N0 -s0 -a -K0 -f -x \
        "+*.zip" "+*.tar.gz" "+*.tar.bz2" "+*.tgz" "+*.rar" "+*.7z" \
        "-*" "+*.html" "+*.htm" "+*.md" "+*.txt" "+*.json" "+*.yml" "+*.yaml" \
        "+*.py" "+*.js" "+*.sh" "+*.dockerfile" "+*.docker" \
        "--max-files=5000" "--max-time=300" "--robots=0"
    
    echo "Completed: $url"
    echo "---"
done

echo "All downloads completed!"
