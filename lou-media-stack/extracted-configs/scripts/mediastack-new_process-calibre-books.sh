#!/bin/bash

# Process temp ebook folders into Calibre libraries
# This script adds books from temp folders to your main "Lou's Library" while preserving metadata

MAIN_LIBRARY="/run/media/lou/Data1/Lou Fogle/Ebooks/Lou's Library"
TEMP_BASE="/run/media/lou/Data1/Lou Fogle/Ebooks"

# Temp folders to process
TEMP_FOLDERS=("New" "Temp" "Intake")

# Function to process a folder
process_folder() {
    local folder="$1"
    local temp_path="$TEMP_BASE/$folder"
    
    echo "Processing: $temp_path"
    
    if [ ! -d "$temp_path" ]; then
        echo "Folder doesn't exist: $temp_path"
        return
    fi
    
    # Count files to process
    file_count=$(find "$temp_path" -type f \( -name "*.epub" -o -name "*.mobi" -o -name "*.azw*" -o -name "*.pdf" -o -name "*.lit" -o -name "*.fb2" \) | wc -l)
    
    if [ "$file_count" -eq 0 ]; then
        echo "No ebook files found in $temp_path"
        return
    fi
    
    echo "Found $file_count ebook files to process"
    
    # Use calibredb to add books (preserves metadata and handles duplicates)
    docker exec -i mediastack_calibre-web calibredb add --recurse "$temp_path" --library-path="$MAIN_LIBRARY" --duplicates
    
    # Create processed folder with timestamp
    processed_folder="$TEMP_BASE/processed-$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$processed_folder/$folder"
    
    # Move processed files
    echo "Moving processed files to: $processed_folder/$folder"
    mv "$temp_path"/* "$processed_folder/$folder/" 2>/dev/null || echo "Some files may have been skipped"
    
    echo "Processed $folder - moved files to $processed_folder/$folder"
}

# Main execution
echo "Starting Calibre library processing..."
echo "Main library: $MAIN_LIBRARY"
echo "=================================================="

# Process each temp folder
for folder in "${TEMP_FOLDERS[@]}"; do
    process_folder "$folder"
    echo "=================================================="
done

echo "Processing complete!"
echo "Access your library at: http://localhost:8084"
