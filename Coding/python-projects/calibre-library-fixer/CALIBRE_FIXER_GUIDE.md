# Calibre Library Scanner and Filename Fixer

A comprehensive Python tool to scan your Calibre library, read metadata from the database, and fix filenames according to a standardized structure.

## Filename Structure

The program organizes filenames using this structure:
```
author-title-series-series1-series2-series3-series4
```

Where:
- **author**: First author name (sanitized)
- **title**: Book title (sanitized)  
- **series**: Main series name + formatted number (e.g., "HarryPotter01")
- **series1-4**: Custom series columns from your Calibre library

## Features

✅ **Metadata Reading**: Reads directly from Calibre's SQLite database  
✅ **Custom Series Support**: Handles up to 4 custom series columns  
✅ **Safe Mode**: Dry-run mode to preview changes before applying  
✅ **Filename Sanitization**: Removes/replaces problematic characters  
✅ **Database Updates**: Updates Calibre database with new paths  
✅ **Comprehensive Logging**: Detailed logs and error reporting  
✅ **Auto-Detection**: Can find your Calibre library automatically  
✅ **Backup Safety**: Creates detailed reports of all changes  

## Installation

The script requires Python 3.6+ (already installed on most modern systems) and uses only standard library modules.

```bash
# Make executable
chmod +x /home/lou/calibre-library-fixer.py

# Check if it works
python3 /home/lou/calibre-library-fixer.py --help
```

## Usage Examples

### 1. Preview Changes (Safe Mode)
```bash
# Auto-detect library and preview changes
python3 /home/lou/calibre-library-fixer.py --auto --dry-run

# Specify library path and preview
python3 /home/lou/calibre-library-fixer.py --library "~/Calibre Library" --dry-run
```

### 2. Actually Fix Filenames
```bash
# Auto-detect and fix (with confirmation)
python3 /home/lou/calibre-library-fixer.py --auto --fix

# Specify library and fix
python3 /home/lou/calibre-library-fixer.py --library "~/Calibre Library" --fix
```

### 3. Generate Reports
```bash
# Save detailed report to file
python3 /home/lou/calibre-library-fixer.py --auto --dry-run --report calibre_report.txt

# Verbose logging for debugging
python3 /home/lou/calibre-library-fixer.py --auto --dry-run --verbose
```

## Command Line Options

```
--library, -l PATH    Path to Calibre library directory
--auto, -a           Auto-detect Calibre library location
--dry-run            Preview changes without making them (default)
--fix                Actually make the changes (disables dry-run)
--report, -r FILE    Save report to file
--verbose, -v        Enable verbose logging
--help, -h           Show help message
```

## Example Output

### Dry Run Mode
```
🔍 DRY RUN MODE - No changes will be made
Use --fix to actually make changes

Found Calibre library: /home/lou/Calibre Library
2025-06-20 20:05:00 - INFO - Initialized Calibre fixer for: /home/lou/Calibre Library
2025-06-20 20:05:00 - INFO - Mode: DRY RUN
2025-06-20 20:05:00 - INFO - Found 1247 books in library

Book ID 42:
  Current: Brandon Sanderson/The Way of Kings (123)
  New:     Brandon Sanderson-The Way of Kings-The Stormlight Archive01
  Author:  Brandon Sanderson
  Title:   The Way of Kings
  Series:  The Stormlight Archive #1.0

💡 Found 342 files that need renaming.
   Use --fix to make the changes.
```

### Live Mode
```
⚠️  LIVE MODE - Changes will be made to your library
Are you sure you want to continue? (yes/no): yes

Book ID 42:
  Current: Brandon Sanderson/The Way of Kings (123)
  New:     Brandon Sanderson-The Way of Kings-The Stormlight Archive01
  ✅ Renamed successfully

✅ Successfully processed 342 files.
```

## How It Works

1. **Database Connection**: Connects to Calibre's `metadata.db` SQLite database
2. **Metadata Extraction**: Queries book information including:
   - Title, authors, series information
   - Custom series columns (series1, series2, etc.)
   - File formats and paths
3. **Filename Generation**: Creates new filenames using the specified structure
4. **Sanitization**: Removes problematic characters for filesystem compatibility
5. **Directory Renaming**: Renames book directories to match new structure
6. **Database Updates**: Updates Calibre database with new paths

## Custom Series Support

The tool automatically detects custom series columns in your Calibre library:

```
# If you have custom columns like:
- series1: "Universe"
- series2: "Timeline" 
- series3: "Arc"
- series4: "Subseries"

# The filename becomes:
Isaac Asimov-Foundation-Foundation01-Robot Universe-Galactic Empire-Foundation Saga-
```

## Safety Features

### Backup Recommendations
Before running with `--fix`:
1. **Backup your Calibre library** (entire directory)
2. **Close Calibre** completely
3. **Run dry-run first** to preview changes
4. **Check the log file** for any errors

### Built-in Safety
- **Dry-run default**: Never makes changes unless explicitly told
- **Confirmation prompt**: Asks for confirmation in live mode  
- **Duplicate detection**: Won't overwrite existing directories
- **Database rollback**: Can manually revert database changes if needed
- **Detailed logging**: Complete audit trail of all operations

## Troubleshooting

### Common Issues

**1. "Calibre library not found"**
```bash
# Try auto-detection
python3 /home/lou/calibre-library-fixer.py --auto --dry-run

# Or specify exact path
python3 /home/lou/calibre-library-fixer.py --library "/exact/path/to/Calibre Library" --dry-run
```

**2. "Database is locked"**
- Close Calibre application completely
- Wait a few seconds and try again

**3. "Permission denied"**
```bash
# Check file permissions
ls -la /path/to/calibre/library/

# Fix permissions if needed
chmod 755 /path/to/calibre/library/
```

**4. Custom series not detected**
- Check if custom columns are properly configured in Calibre
- Use `--verbose` flag to see detailed column detection

### Log Files

The program creates detailed logs:
- `calibre_fixer.log` - Complete operation log
- Custom report files with `--report` option

### Recovery

If something goes wrong:
1. **Restore from backup** (recommended)
2. **Manual database fix**:
   ```sql
   -- Example to revert a single book path
   UPDATE books SET path = 'old_path' WHERE id = book_id;
   ```

## Advanced Usage

### Batch Processing
```bash
# Process multiple libraries
for lib in ~/Calibre*; do
  python3 /home/lou/calibre-library-fixer.py --library "$lib" --dry-run
done
```

### Integration with Scripts
```python
from calibre_library_fixer import CalibreLibraryFixer

# Use in your own scripts
fixer = CalibreLibraryFixer("/path/to/library", dry_run=True)
summary = fixer.scan_and_fix()
```

## Requirements

- **Python 3.6+** (standard on modern Linux)
- **SQLite3** (included with Python)
- **Read/write access** to Calibre library directory
- **Calibre should be closed** during filename changes

## File Location

The script is installed at: `/home/lou/calibre-library-fixer.py`

You can move it anywhere or create a symlink for easier access:
```bash
# Create symlink for system-wide access
sudo ln -s /home/lou/calibre-library-fixer.py /usr/local/bin/calibre-fixer

# Then use anywhere
calibre-fixer --auto --dry-run
```

---

**Version**: 1.0  
**Created**: 2025-06-20  
**License**: MIT
