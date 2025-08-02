#!/bin/bash

# Process temp ebook libraries with Calibre
# Usage: ./process-temp-books.sh [temp_folder_name]

MAIN_LIBRARY="/run/media/lou/Data1/Lou Fogle/Ebooks/Lou's Library"
TEMP_BASE="/run/media/lou/Data1/Lou Fogle/Ebooks"

# Folders to process
TEMP_FOLDERS=("New" "Temp" "Intake")

# If specific folder provided, use it
if [ "$1" ]; then
    TEMP_FOLDERS=("$1")
fi

# Function to process a folder
process_folder() {
    local folder="$1"
    local temp_path="$TEMP_BASE/$folder"
    
    echo "Processing: $temp_path"
    
    if [ ! -d "$temp_path" ]; then
        echo "Folder doesn't exist: $temp_path"
        return
    fi
    
    # Add books to main library using Calibre
    # This handles metadata extraction, series detection, and duplicate prevention
    docker exec -i mediastack_calibre-web calibredb add -r "$temp_path" --library-path="$MAIN_LIBRARY"
    
    # Optional: Move processed files to a 'processed' folder
    # mkdir -p "$temp_path/../processed-$(date +%Y%m%d)"
    # mv "$temp_path"/* "$temp_path/../processed-$(date +%Y%m%d)/"
}

# Process each temp folder
for folder in "${TEMP_FOLDERS[@]}"; do
    process_folder "$folder"
done

echo "Processing complete!"
