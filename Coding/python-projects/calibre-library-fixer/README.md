# Calibre Library Fixer

A comprehensive GUI and CLI tool for organizing Calibre library filenames with standardized naming conventions.

## Features

- **GUI Interface**: Modern PyQt6-based graphical interface with dark theme
- **CLI Interface**: Command-line interface for scripting and automation
- **Smart Filename Generation**: Standardized format: `author-title-series-series1-series2-series3-series4`
- **Dry Run Mode**: Preview changes before applying them
- **Auto-Detection**: Automatically find your Calibre library
- **Progress Tracking**: Real-time progress updates and detailed logging
- **Safe Operations**: Backup options and confirmation dialogs
- **Custom Series Support**: Handle custom series columns from Calibre

## Installation

### From Arch User Repository (AUR)

```bash
# Using an AUR helper like yay
yay -S calibre-library-fixer

# Or using makepkg directly
git clone https://aur.archlinux.org/calibre-library-fixer.git
cd calibre-library-fixer
makepkg -si
```

### From Source

```bash
# Clone the repository
git clone https://github.com/example/calibre-library-fixer.git
cd calibre-library-fixer

# Install dependencies
sudo pacman -S python python-pyqt6 calibre

# Install the package
pip install .
```

## Usage

### GUI Application

Launch the graphical interface:

```bash
calibre-library-fixer-gui
```

Or use the desktop entry from your application menu.

### Command Line Interface

```bash
# Preview changes (dry run)
calibre-library-fixer --library ~/Calibre\ Library --dry-run

# Actually make changes
calibre-library-fixer --library ~/Calibre\ Library --fix

# Auto-detect library location
calibre-library-fixer --auto --dry-run

# Save detailed report
calibre-library-fixer --auto --fix --report changes.txt
```

## Filename Format

The tool organizes filenames according to this standardized format:

```
author-title-series-series1-series2-series3-series4
```

Where:
- **author**: First author's name (sanitized for filesystem)
- **title**: Book title (sanitized for filesystem)
- **series**: Main series name and number (e.g., "Foundation01")
- **series1-4**: Custom series columns from Calibre (if configured)

## Safety Features

- **Dry Run Mode**: Preview all changes before applying them
- **Database Backup**: Option to backup Calibre database before changes
- **Detailed Logging**: Complete audit trail of all operations
- **Validation**: Ensures target directories don't already exist
- **Confirmation Dialogs**: Multiple confirmation steps for safety

## Requirements

- Python 3.8 or higher
- PyQt6
- Calibre (for library metadata access)
- SQLite3 (included with Python)

## Development

### Setting up Development Environment

```bash
# Clone repository
git clone https://github.com/example/calibre-library-fixer.git
cd calibre-library-fixer

# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install development dependencies
pip install -e ".[dev]"

# Run tests
pytest

# Format code
black calibre_library_fixer/
isort calibre_library_fixer/

# Type checking
mypy calibre_library_fixer/
```

### Building Package

```bash
# Build wheel
python -m build

# Install locally
pip install dist/*.whl
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

If you encounter any issues or have questions:

1. Check the [Issues](https://github.com/example/calibre-library-fixer/issues) page
2. Create a new issue with detailed information about your problem
3. Include your system information and Calibre version

## Changelog

### Version 2.0.0
- Complete rewrite with PyQt6 GUI
- Improved error handling and logging
- Better filename sanitization
- Auto-detection of Calibre libraries
- Enhanced progress tracking
- Added packaging for Arch Linux

### Version 1.0.0
- Initial CLI-only release
- Basic filename fixing functionality
- Dry run mode
- Custom series support
