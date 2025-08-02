# Universal Arch Linux Installer

![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Python](https://img.shields.io/badge/python-3.8+-yellow.svg)
![Arch Linux](https://img.shields.io/badge/Arch%20Linux-compatible-brightgreen.svg)

A comprehensive, AI-enhanced package installer for Arch Linux that unifies all package managers and system tools into one powerful interface.

## 🚀 Features

### 📦 Multi-Source Package Management
- **pacman** - Official Arch repositories
- **AUR Helpers** - yay, paru, pikaur, trizen, aurman, aura, pakku
- **Universal Packages** - Flatpak, Snap
- **Python Packages** - pip, pipx
- **Conda/Mamba** - conda-forge repositories
- **Git Repositories** - Direct git installation
- **Local Packages** - Local package installation
- **AppImages** - Portable application support

### 🤖 AI-Powered Features
- **Smart Package Search** - AI-enhanced search with intelligent ranking
- **Automatic Categorization** - Packages automatically categorized by AI
- **Intelligent Recommendations** - AI suggests packages based on your system
- **Request Tracking** - Built-in AI request budget management (150 requests)

### 🛠️ System Maintenance Integration
- **Cylon** - System maintenance and optimization
- **Topgrade** - Universal update tool
- **Reflector** - Automatic mirror optimization
- **Paccache** - Package cache cleaning
- **System Updates** - Unified system update interface

### 🖥️ Dual Interface
- **Qt6 GUI** - Modern, intuitive graphical interface
- **CLI Interface** - Powerful command-line interface
- **Interactive Mode** - Step-by-step package management

### 📊 Advanced Features
- **SQLite Database** - Local package information caching
- **Installation History** - Complete installation tracking
- **Package Export/Import** - Backup and restore package lists
- **Smart Filtering** - Filter packages by source, category, etc.
- **Comprehensive Logging** - Detailed operation logs

## 🔧 Installation

### Quick Install
```bash
cd /home/lou/universal-arch-installer
chmod +x install.sh
./install.sh
```

### Manual Installation
1. **Install Dependencies**:
   ```bash
   # Install PyQt6
   pip install --user PyQt6
   
   # Install additional package managers
   yay -S trizen aurman aura-bin pakku topgrade-bin
   ```

2. **Make Executable**:
   ```bash
   chmod +x universal_arch_installer_qt.py
   ```

3. **Create Symlink** (optional):
   ```bash
   sudo ln -sf $(pwd)/universal_arch_installer_qt.py /usr/local/bin/uai
   ```

## 📖 Usage

### GUI Mode
```bash
# Launch graphical interface
uai --gui
# or
python universal_arch_installer_qt.py --gui
```

### CLI Mode
```bash
# Search for packages
uai --search firefox

# Search with AI enhancement
uai --search "video editor" --ai

# System maintenance
uai --maintenance system_update
uai --maintenance clean_cache
uai --maintenance update_mirrors
uai --maintenance cylon

# Interactive mode
uai --interactive
```

### Command Line Options
```
usage: universal_arch_installer_qt.py [-h] [-g] [-s] [-i] [--ai] 
                                      [-m {update_mirrors,clean_cache,system_update,cylon}] 
                                      [package]

positional arguments:
  package               Package name to install/search

optional arguments:
  -h, --help            show this help message and exit
  -g, --gui             Launch GUI interface
  -s, --search          Search for packages
  -i, --interactive     Interactive CLI mode
  --ai                  Use AI enhancements
  -m, --maintenance     Run maintenance operation
```

## 🎯 GUI Interface

### Main Tabs
1. **Search Packages** - Multi-source package search with AI ranking
2. **Installed Packages** - View and manage installed packages
3. **System Maintenance** - System maintenance and optimization tools
4. **AI Assistant** - AI-powered features and request tracking
5. **Settings** - Configure preferences and package manager priority

### Search Features
- **Real-time Search** - Background search across all repositories
- **AI Enhancement Toggle** - Enable/disable AI-powered features
- **Smart Categorization** - Packages automatically categorized
- **One-Click Install** - Install packages directly from search results

### System Maintenance
- **Update Mirrors** - Optimize pacman mirrors with reflector
- **Clean Package Cache** - Remove old package files
- **Full System Update** - Update all packages from all sources
- **Cylon Maintenance** - Run comprehensive system maintenance

## 🤖 AI Features

### Smart Search
The AI enhancement provides:
- **Intelligent Ranking** - Exact matches and popularity-based sorting
- **Category Detection** - Automatic package categorization
- **Relevance Scoring** - Smart relevance calculation
- **Source Prioritization** - Prefer official repositories

### Request Tracking
- **Budget Management** - 150 AI requests per session
- **Usage Logging** - All AI requests are logged with timestamps
- **Remaining Counter** - Real-time tracking of available requests

### Categories
- Development
- System
- Multimedia
- Games
- Internet
- Office
- Graphics
- Education
- Science
- Utilities

## 🔧 System Tools Integration

### Detected Package Managers
The installer automatically detects and integrates:
- ✅ **pacman** - Always available on Arch Linux
- ✅ **yay** - Most popular AUR helper
- ✅ **paru** - Fast AUR helper written in Rust
- ✅ **pikaur** - Review-based AUR helper
- ✅ **trizen** - Lightweight AUR helper
- ✅ **aurman** - Feature-rich AUR helper
- ✅ **aura** - Multi-lingual package manager
- ✅ **pakku** - Pacman-like AUR helper
- ✅ **cylon** - System maintenance tool
- ✅ **pamac** - Manjaro's package manager
- ✅ **bauh** - Universal package manager GUI
- ✅ **octopi** - Pacman frontend

### Maintenance Tools
- **cylon** - System cleanup and optimization
- **topgrade** - Universal system updater
- **reflector** - Mirror optimization
- **paccache** - Package cache management
- **bleachbit** - System cleaner
- **stacer** - System optimizer
- **timeshift** - System snapshots

## 📁 File Structure

```
universal-arch-installer/
├── universal_arch_installer_qt.py    # Main application
├── universal-arch-installer.py       # Original CLI version
├── install.sh                        # Installation script
├── requirements.txt                  # Python dependencies
├── ai_requests_log.txt               # AI usage tracking
└── README.md                         # This file

~/.config/universal-arch-installer/
├── packages.db                       # SQLite package database
└── installer.log                     # Application logs
```

## 🗂️ Database Schema

### Packages Table
- Package information cache
- Installation status tracking
- Metadata storage

### Install History Table
- Complete installation history
- Success/failure tracking
- Error message logging

## ⚙️ Configuration

### Settings (GUI)
- **Preferred AUR Helper** - Choose your preferred AUR helper
- **Auto-update Package Lists** - Automatically refresh package lists
- **Enable AI Features** - Toggle AI-powered features
- **Auto-categorize Packages** - Automatic package categorization

### Environment Variables
- `UAI_LOG_LEVEL` - Set logging level (DEBUG, INFO, WARNING, ERROR)
- `UAI_AI_REQUESTS` - Override AI request limit (default: 150)

## 🔍 Examples

### Search Examples
```bash
# Basic search
uai --search firefox

# AI-enhanced search
uai --search "video editing software" --ai

# Search development tools
uai --search python --ai
```

### Maintenance Examples
```bash
# Update system mirrors
uai --maintenance update_mirrors

# Clean package cache
uai --maintenance clean_cache

# Full system update
uai --maintenance system_update

# Run cylon maintenance
uai --maintenance cylon
```

## 🚨 Troubleshooting

### Common Issues

1. **PyQt6 not found**
   ```bash
   pip install --user PyQt6
   ```

2. **Permission denied for system operations**
   - Ensure sudo access for system maintenance
   - Check user permissions

3. **AUR helper not detected**
   ```bash
   # Install yay or paru
   yay -S paru-bin
   ```

4. **AI requests exhausted**
   - Check `ai_requests_log.txt` for usage
   - Wait for next session or modify limit

### Logs
- **Application logs**: `~/.config/universal-arch-installer/installer.log`
- **AI requests**: `~/universal-arch-installer/ai_requests_log.txt`

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on Arch Linux
5. Submit a pull request

## 📝 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

- **Arch Linux** - The amazing distribution this tool is built for
- **PyQt6** - Excellent GUI framework
- **AUR Helpers** - yay, paru, and all other maintainers
- **Package Maintainers** - All the developers maintaining packages
- **Community** - Arch Linux community for inspiration and support

## 📊 AI Request Tracking

Current session AI usage is tracked in `ai_requests_log.txt`:
- **Total Budget**: 150 requests per session
- **Current Usage**: Tracked in real-time
- **Request History**: Complete log with timestamps
- **Remaining Requests**: Displayed in GUI status bar

## 🔮 Future Enhancements

- **ML Package Recommendations** - Machine learning-based suggestions
- **System Health Monitoring** - Real-time system health tracking
- **Package Vulnerability Scanning** - Security vulnerability detection
- **Automated Backup Solutions** - Integrated system backup tools
- **Package Dependency Visualization** - Graphical dependency trees
- **Remote Package Management** - Manage multiple Arch systems

---

**Happy Package Managing! 🎉**

*Made with ❤️ for the Arch Linux community*
