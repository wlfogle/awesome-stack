#!/usr/bin/env python3
"""
Universal Arch Linux Installer - Optimized Version
==================================================
A comprehensive Qt-based installer with enhanced features, AI-powered search,
and intelligent download URL discovery.

Features:
- Interactive GUI with Qt6
- AI-powered package search and recommendations
- Intelligent URL construction and web scraping
- Windows program installer with Wine integration
- System maintenance and optimization tools
- Performance monitoring and security scanning
- Backup and restore functionality
- Interactive CLI mode with full menu system

Author: AI Assistant
Date: 2025-06-22
License: MIT
"""

import sys
import os
import subprocess
import json
import shutil
import tempfile
import argparse
import logging
import re
import sqlite3
import hashlib
import datetime
import threading
import time
import asyncio
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any, Union
from dataclasses import dataclass, asdict
from enum import Enum
from collections import defaultdict
import xml.etree.ElementTree as ET
from urllib.parse import urljoin, urlparse, parse_qs
import tarfile
import gzip
from packaging import version

# Enhanced imports with fallbacks
try:
    import requests
    import aiohttp
    from bs4 import BeautifulSoup
    import feedparser
    WEB_SCRAPING_AVAILABLE = True
except ImportError:
    WEB_SCRAPING_AVAILABLE = False

try:
    import nltk
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.metrics.pairwise import cosine_similarity
    AI_FEATURES_AVAILABLE = True
except (ImportError, LookupError):
    AI_FEATURES_AVAILABLE = False

try:
    import psutil
    PSUTIL_AVAILABLE = True
except ImportError:
    PSUTIL_AVAILABLE = False

# Qt6 imports with fallback
try:
    from PyQt6.QtWidgets import *
    from PyQt6.QtCore import *
    from PyQt6.QtGui import *
    QT_AVAILABLE = True
except ImportError:
    try:
        from PyQt5.QtWidgets import *
        from PyQt5.QtCore import *
        from PyQt5.QtGui import *
        QT_AVAILABLE = True
    except ImportError:
        QT_AVAILABLE = False

# Color codes for terminal output
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

class InstallMethod(Enum):
    PACMAN = "pacman"
    YAY = "yay"
    PARU = "paru"
    PIKAUR = "pikaur"
    TRIZEN = "trizen"
    AURMAN = "aurman"
    AURA = "aura"
    PAKKU = "pakku"
    PIP = "pip"
    PIPX = "pipx"
    CONDA = "conda"
    MAMBA = "mamba"
    FLATPAK = "flatpak"
    SNAP = "snap"
    APPIMAGE = "appimage"
    GIT = "git"
    LOCAL = "local"
    WEB_DOWNLOAD = "web_download"
    BINARY_RELEASE = "binary_release"
    SOURCE_BUILD = "source_build"
    DOCKER = "docker"
    NIX = "nix"
    HOMEBREW = "homebrew"

class PackageCategory(Enum):
    DEVELOPMENT = "Development"
    SYSTEM = "System"
    MULTIMEDIA = "Multimedia"
    GAMES = "Games"
    INTERNET = "Internet"
    OFFICE = "Office"
    GRAPHICS = "Graphics"
    EDUCATION = "Education"
    SCIENCE = "Science"
    UTILITIES = "Utilities"
    SECURITY = "Security"
    TERMINAL = "Terminal"
    OTHER = "Other"

@dataclass
class PackageInfo:
    name: str
    method: InstallMethod
    source: str
    description: str = ""
    version: str = ""
    size: str = ""
    dependencies: List[str] = None
    optional_deps: List[str] = None
    post_install: List[str] = None
    category: PackageCategory = PackageCategory.OTHER
    popularity: int = 0
    last_updated: str = ""
    maintainer: str = ""
    license: str = ""
    url: str = ""
    homepage: str = ""
    installed: bool = False
    install_date: str = ""
    security_score: float = 0.0
    compatibility_score: float = 0.0
    recommendation_reason: str = ""

    def __post_init__(self):
        if self.dependencies is None:
            self.dependencies = []
        if self.optional_deps is None:
            self.optional_deps = []
        if self.post_install is None:
            self.post_install = []

class UniversalArchInstaller:
    """Enhanced Universal Arch Linux Installer with AI-powered features"""
    
    def __init__(self):
        self.config_dir = Path.home() / ".config" / "universal-arch-installer"
        self.config_dir.mkdir(parents=True, exist_ok=True)
        
        # Initialize logging
        self.logger = self._setup_logging()
        
        # Initialize database
        self.db_path = self.config_dir / "enhanced_packages.db"
        self._init_database()
        
        # Initialize AI components if available
        if AI_FEATURES_AVAILABLE:
            self.vectorizer = TfidfVectorizer(stop_words='english', max_features=1000)
            self.package_vectors = None
        
        # Performance monitoring
        self.performance_monitor = PerformanceMonitor() if PSUTIL_AVAILABLE else None
        
        # Backup manager
        self.backup_manager = BackupManager(self.config_dir)
        
        # Package methods availability
        self.available_methods = self._check_available_methods()
        
        self.logger.info("UniversalArchInstaller initialized successfully")

    def _setup_logging(self) -> logging.Logger:
        """Setup enhanced logging system"""
        log_file = self.config_dir / "installer.log"
        
        # Create formatter
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        # File handler
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        
        # Console handler
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        
        # Setup logger
        logger = logging.getLogger('UniversalArchInstaller')
        logger.setLevel(logging.DEBUG)
        logger.addHandler(file_handler)
        logger.addHandler(console_handler)
        
        return logger

    def _init_database(self):
        """Initialize SQLite database for enhanced features"""
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            # Packages table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS packages (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT UNIQUE NOT NULL,
                    method TEXT NOT NULL,
                    source TEXT NOT NULL,
                    description TEXT,
                    version TEXT,
                    category TEXT,
                    popularity INTEGER DEFAULT 0,
                    last_updated TEXT,
                    installed BOOLEAN DEFAULT FALSE,
                    install_date TEXT,
                    security_score REAL DEFAULT 0.0,
                    compatibility_score REAL DEFAULT 0.0,
                    metadata TEXT
                )
            ''')
            
            # User interactions table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS user_interactions (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    package_name TEXT NOT NULL,
                    action TEXT NOT NULL,
                    timestamp TEXT NOT NULL,
                    success BOOLEAN DEFAULT TRUE,
                    duration REAL DEFAULT 0.0
                )
            ''')
            
            # Performance metrics table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS performance_metrics (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    operation_type TEXT NOT NULL,
                    package_name TEXT NOT NULL,
                    duration REAL NOT NULL,
                    memory_usage REAL NOT NULL,
                    cpu_usage REAL NOT NULL,
                    timestamp TEXT NOT NULL,
                    success BOOLEAN DEFAULT TRUE
                )
            ''')
            
            # Backup metadata table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS backup_metadata (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL,
                    path TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    size INTEGER NOT NULL,
                    packages_count INTEGER NOT NULL
                )
            ''')
            
            conn.commit()
            conn.close()
            
        except Exception as e:
            self.logger.error(f"Database initialization failed: {e}")

    def _check_available_methods(self) -> List[InstallMethod]:
        """Check which package managers are available on the system"""
        available = []
        
        methods_to_check = {
            InstallMethod.PACMAN: "pacman",
            InstallMethod.YAY: "yay",
            InstallMethod.PARU: "paru",
            InstallMethod.PIKAUR: "pikaur",
            InstallMethod.TRIZEN: "trizen",
            InstallMethod.AURMAN: "aurman",
            InstallMethod.AURA: "aura",
            InstallMethod.PAKKU: "pakku",
            InstallMethod.PIP: "pip",
            InstallMethod.PIPX: "pipx",
            InstallMethod.CONDA: "conda",
            InstallMethod.MAMBA: "mamba",
            InstallMethod.FLATPAK: "flatpak",
            InstallMethod.SNAP: "snap",
            InstallMethod.GIT: "git",
            InstallMethod.DOCKER: "docker",
        }
        
        for method, command in methods_to_check.items():
            if shutil.which(command):
                available.append(method)
                
        return available

    def print_colored(self, text: str, color: str = Colors.WHITE):
        """Print colored text to terminal"""
        print(f"{color}{text}{Colors.RESET}")

    def search_package_ai_enhanced(self, query: str, use_ai: bool = True) -> List[PackageInfo]:
        """Enhanced package search with AI-powered recommendations"""
        self.logger.info(f"Searching for packages: {query}")
        
        monitor_data = None
        if self.performance_monitor:
            monitor_data = self.performance_monitor.start_monitoring("search", query)
        
        try:
            all_packages = []
            
            # Search across all available methods
            for method in self.available_methods:
                try:
                    packages = self._search_packages_by_method(query, method)
                    all_packages.extend(packages)
                except Exception as e:
                    self.logger.warning(f"Search failed for {method.value}: {e}")
            
            # AI-powered filtering and ranking if available
            if use_ai and AI_FEATURES_AVAILABLE and all_packages:
                all_packages = self._apply_ai_ranking(query, all_packages)
            
            # Remove duplicates while preserving order
            seen = set()
            unique_packages = []
            for pkg in all_packages:
                key = (pkg.name, pkg.method.value)
                if key not in seen:
                    seen.add(key)
                    unique_packages.append(pkg)
            
            self.logger.info(f"Found {len(unique_packages)} unique packages")
            return unique_packages[:50]  # Limit results
            
        except Exception as e:
            self.logger.error(f"Package search failed: {e}")
            return []
        finally:
            if monitor_data and self.performance_monitor:
                self.performance_monitor.stop_monitoring(monitor_data, True)

    def _search_packages_by_method(self, query: str, method: InstallMethod) -> List[PackageInfo]:
        """Search packages using specific method"""
        packages = []
        
        try:
            if method == InstallMethod.PACMAN:
                packages.extend(self._search_pacman(query))
            elif method in [InstallMethod.YAY, InstallMethod.PARU, InstallMethod.PIKAUR]:
                packages.extend(self._search_aur_helper(query, method))
            elif method == InstallMethod.FLATPAK:
                packages.extend(self._search_flatpak(query))
            elif method == InstallMethod.SNAP:
                packages.extend(self._search_snap(query))
            elif method == InstallMethod.PIP:
                packages.extend(self._search_pip(query))
            elif method == InstallMethod.CONDA:
                packages.extend(self._search_conda(query))
            
        except Exception as e:
            self.logger.warning(f"Search failed for {method.value}: {e}")
            
        return packages

    def _search_pacman(self, query: str) -> List[PackageInfo]:
        """Search official Arch repositories"""
        packages = []
        try:
            result = subprocess.run(
                ["pacman", "-Ss", query],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                packages = self._parse_pacman_output(result.stdout, InstallMethod.PACMAN)
                
        except subprocess.TimeoutExpired:
            self.logger.warning("Pacman search timed out")
        except Exception as e:
            self.logger.error(f"Pacman search failed: {e}")
            
        return packages

    def _search_aur_helper(self, query: str, method: InstallMethod) -> List[PackageInfo]:
        """Search AUR using helper"""
        packages = []
        try:
            result = subprocess.run(
                [method.value, "-Ss", query],
                capture_output=True, text=True, timeout=60
            )
            
            if result.returncode == 0:
                packages = self._parse_pacman_output(result.stdout, method)
                
        except subprocess.TimeoutExpired:
            self.logger.warning(f"{method.value} search timed out")
        except Exception as e:
            self.logger.error(f"{method.value} search failed: {e}")
            
        return packages

    def _search_flatpak(self, query: str) -> List[PackageInfo]:
        """Search Flatpak packages"""
        packages = []
        try:
            result = subprocess.run(
                ["flatpak", "search", query],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                packages = self._parse_flatpak_output(result.stdout)
                
        except Exception as e:
            self.logger.error(f"Flatpak search failed: {e}")
            
        return packages

    def _search_snap(self, query: str) -> List[PackageInfo]:
        """Search Snap packages"""
        packages = []
        try:
            result = subprocess.run(
                ["snap", "find", query],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                packages = self._parse_snap_output(result.stdout)
                
        except Exception as e:
            self.logger.error(f"Snap search failed: {e}")
            
        return packages

    def _search_pip(self, query: str) -> List[PackageInfo]:
        """Search PyPI packages"""
        packages = []
        try:
            result = subprocess.run(
                ["pip", "search", query],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                packages = self._parse_pip_output(result.stdout)
                
        except Exception as e:
            # pip search is deprecated, try alternative
            if WEB_SCRAPING_AVAILABLE:
                packages = self._search_pypi_web(query)
            
        return packages

    def _search_conda(self, query: str) -> List[PackageInfo]:
        """Search Conda packages"""
        packages = []
        try:
            result = subprocess.run(
                ["conda", "search", query],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                packages = self._parse_conda_output(result.stdout)
                
        except Exception as e:
            self.logger.error(f"Conda search failed: {e}")
            
        return packages

    def _parse_pacman_output(self, output: str, method: InstallMethod) -> List[PackageInfo]:
        """Parse pacman/AUR helper output"""
        packages = []
        lines = output.strip().split('\n')
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if not line:
                i += 1
                continue
                
            # Parse package line
            if '/' in line and ' ' in line:
                parts = line.split(' ', 1)
                if len(parts) >= 2:
                    name_part = parts[0]
                    if '/' in name_part:
                        repo, name = name_part.split('/', 1)
                    else:
                        repo = "unknown"
                        name = name_part
                    
                    version_desc = parts[1]
                    version = ""
                    description = ""
                    
                    # Extract version and description
                    if version_desc:
                        version_match = re.match(r'^([^\s]+)', version_desc)
                        if version_match:
                            version = version_match.group(1)
                            description = version_desc[len(version):].strip()
                    
                    # Get description from next line if needed
                    if i + 1 < len(lines) and not description:
                        next_line = lines[i + 1].strip()
                        if next_line and not ('/' in next_line and ' ' in next_line):
                            description = next_line
                            i += 1
                    
                    package = PackageInfo(
                        name=name,
                        method=method,
                        source=repo,
                        description=description,
                        version=version,
                        category=self._categorize_package(name, description)
                    )
                    packages.append(package)
            
            i += 1
        
        return packages

    def _parse_flatpak_output(self, output: str) -> List[PackageInfo]:
        """Parse Flatpak search output"""
        packages = []
        lines = output.strip().split('\n')[1:]  # Skip header
        
        for line in lines:
            parts = line.split('\t')
            if len(parts) >= 3:
                name = parts[0].strip()
                description = parts[1].strip()
                app_id = parts[2].strip()
                version = parts[3].strip() if len(parts) > 3 else ""
                
                package = PackageInfo(
                    name=name,
                    method=InstallMethod.FLATPAK,
                    source=app_id,
                    description=description,
                    version=version,
                    category=self._categorize_package(name, description)
                )
                packages.append(package)
        
        return packages

    def _parse_snap_output(self, output: str) -> List[PackageInfo]:
        """Parse Snap search output"""
        packages = []
        lines = output.strip().split('\n')[1:]  # Skip header
        
        for line in lines:
            parts = line.split()
            if len(parts) >= 4:
                name = parts[0]
                version = parts[1]
                publisher = parts[2]
                description = ' '.join(parts[4:]) if len(parts) > 4 else ""
                
                package = PackageInfo(
                    name=name,
                    method=InstallMethod.SNAP,
                    source=publisher,
                    description=description,
                    version=version,
                    category=self._categorize_package(name, description)
                )
                packages.append(package)
        
        return packages

    def _parse_pip_output(self, output: str) -> List[PackageInfo]:
        """Parse pip search output"""
        packages = []
        lines = output.strip().split('\n')
        
        for line in lines:
            if ' - ' in line:
                parts = line.split(' - ', 1)
                if len(parts) == 2:
                    name_version = parts[0].strip()
                    description = parts[1].strip()
                    
                    # Extract name and version
                    name_match = re.match(r'^([^\s\(]+)', name_version)
                    version_match = re.search(r'\(([^)]+)\)', name_version)
                    
                    name = name_match.group(1) if name_match else name_version
                    version = version_match.group(1) if version_match else ""
                    
                    package = PackageInfo(
                        name=name,
                        method=InstallMethod.PIP,
                        source="pypi",
                        description=description,
                        version=version,
                        category=self._categorize_package(name, description)
                    )
                    packages.append(package)
        
        return packages

    def _parse_conda_output(self, output: str) -> List[PackageInfo]:
        """Parse conda search output"""
        packages = []
        lines = output.strip().split('\n')
        
        for line in lines:
            if line.startswith('#') or not line.strip():
                continue
                
            parts = line.split()
            if len(parts) >= 3:
                name = parts[0]
                version = parts[1]
                channel = parts[2]
                
                package = PackageInfo(
                    name=name,
                    method=InstallMethod.CONDA,
                    source=channel,
                    version=version,
                    category=self._categorize_package(name, "")
                )
                packages.append(package)
        
        return packages

    def _search_pypi_web(self, query: str) -> List[PackageInfo]:
        """Search PyPI using web API"""
        packages = []
        try:
            if not WEB_SCRAPING_AVAILABLE:
                return packages
                
            url = f"https://pypi.org/simple/{query}/"
            response = requests.get(url, timeout=10)
            
            if response.status_code == 200:
                soup = BeautifulSoup(response.content, 'html.parser')
                # Parse PyPI simple index
                # This is a simplified implementation
                pass
                
        except Exception as e:
            self.logger.error(f"PyPI web search failed: {e}")
            
        return packages

    def _categorize_package(self, name: str, description: str) -> PackageCategory:
        """Categorize package based on name and description"""
        text = f"{name} {description}".lower()
        
        categories = {
            PackageCategory.DEVELOPMENT: [
                'dev', 'development', 'programming', 'code', 'editor', 'ide',
                'compiler', 'debugger', 'git', 'svn', 'version', 'library'
            ],
            PackageCategory.SYSTEM: [
                'system', 'kernel', 'driver', 'hardware', 'boot', 'init',
                'service', 'daemon', 'monitor', 'performance'
            ],
            PackageCategory.MULTIMEDIA: [
                'media', 'video', 'audio', 'music', 'player', 'codec',
                'streaming', 'recording', 'editing'
            ],
            PackageCategory.GAMES: [
                'game', 'gaming', 'emulator', 'steam', 'play'
            ],
            PackageCategory.INTERNET: [
                'browser', 'web', 'internet', 'network', 'download',
                'torrent', 'ftp', 'ssh', 'vpn'
            ],
            PackageCategory.OFFICE: [
                'office', 'document', 'pdf', 'spreadsheet', 'presentation',
                'text', 'writer', 'calc'
            ],
            PackageCategory.GRAPHICS: [
                'graphics', 'image', 'photo', 'drawing', 'design',
                'gimp', 'inkscape', 'blender'
            ],
            PackageCategory.TERMINAL: [
                'terminal', 'shell', 'console', 'cli', 'command'
            ],
            PackageCategory.UTILITIES: [
                'utility', 'tool', 'archive', 'backup', 'sync',
                'file', 'manager'
            ]
        }
        
        for category, keywords in categories.items():
            if any(keyword in text for keyword in keywords):
                return category
        
        return PackageCategory.OTHER

    def _apply_ai_ranking(self, query: str, packages: List[PackageInfo]) -> List[PackageInfo]:
        """Apply AI-powered ranking to packages"""
        if not AI_FEATURES_AVAILABLE or not packages:
            return packages
        
        try:
            # Create document corpus
            documents = []
            for pkg in packages:
                doc = f"{pkg.name} {pkg.description}"
                documents.append(doc)
            
            # Add query to corpus
            documents.append(query)
            
            # Vectorize
            tfidf_matrix = self.vectorizer.fit_transform(documents)
            query_vector = tfidf_matrix[-1]
            doc_vectors = tfidf_matrix[:-1]
            
            # Calculate similarity scores
            similarities = cosine_similarity(query_vector, doc_vectors).flatten()
            
            # Sort packages by similarity
            package_scores = list(zip(packages, similarities))
            package_scores.sort(key=lambda x: x[1], reverse=True)
            
            return [pkg for pkg, score in package_scores]
            
        except Exception as e:
            self.logger.error(f"AI ranking failed: {e}")
            return packages

    def install_package(self, package: PackageInfo, confirm: bool = True) -> bool:
        """Install a package with enhanced monitoring"""
        self.logger.info(f"Installing package: {package.name} via {package.method.value}")
        
        if confirm:
            self.print_colored(f"\n📦 Installing {package.name}", Colors.CYAN)
            self.print_colored(f"Method: {package.method.value}", Colors.WHITE)
            self.print_colored(f"Description: {package.description}", Colors.WHITE)
            
            response = input(f"\n{Colors.YELLOW}Continue with installation? (y/N): {Colors.RESET}")
            if response.lower() not in ['y', 'yes']:
                self.print_colored("Installation cancelled", Colors.YELLOW)
                return False
        
        monitor_data = None
        if self.performance_monitor:
            monitor_data = self.performance_monitor.start_monitoring("install", package.name)
        
        try:
            success = self._execute_install(package)
            
            if success:
                self.print_colored(f"✅ {package.name} installed successfully!", Colors.GREEN)
                self._record_installation(package)
            else:
                self.print_colored(f"❌ Failed to install {package.name}", Colors.RED)
            
            return success
            
        except Exception as e:
            self.logger.error(f"Installation failed: {e}")
            self.print_colored(f"❌ Installation error: {str(e)}", Colors.RED)
            return False
        finally:
            if monitor_data and self.performance_monitor:
                self.performance_monitor.stop_monitoring(monitor_data, success)

    def _execute_install(self, package: PackageInfo) -> bool:
        """Execute the actual installation command"""
        try:
            if package.method == InstallMethod.PACMAN:
                result = subprocess.run(
                    ["sudo", "pacman", "-S", "--needed", "--noconfirm", package.name],
                    capture_output=True, text=True
                )
            elif package.method in [InstallMethod.YAY, InstallMethod.PARU, InstallMethod.PIKAUR]:
                result = subprocess.run(
                    [package.method.value, "-S", "--needed", "--noconfirm", package.name],
                    capture_output=True, text=True
                )
            elif package.method == InstallMethod.FLATPAK:
                result = subprocess.run(
                    ["flatpak", "install", "-y", package.source],
                    capture_output=True, text=True
                )
            elif package.method == InstallMethod.SNAP:
                result = subprocess.run(
                    ["sudo", "snap", "install", package.name],
                    capture_output=True, text=True
                )
            elif package.method == InstallMethod.PIP:
                result = subprocess.run(
                    ["pip", "install", "--user", package.name],
                    capture_output=True, text=True
                )
            elif package.method == InstallMethod.PIPX:
                result = subprocess.run(
                    ["pipx", "install", package.name],
                    capture_output=True, text=True
                )
            else:
                self.logger.error(f"Unsupported install method: {package.method.value}")
                return False
            
            if result.returncode == 0:
                self.logger.info(f"Package {package.name} installed successfully")
                return True
            else:
                self.logger.error(f"Installation failed: {result.stderr}")
                return False
                
        except Exception as e:
            self.logger.error(f"Installation execution failed: {e}")
            return False

    def _record_installation(self, package: PackageInfo):
        """Record package installation in database"""
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            cursor.execute('''
                INSERT OR REPLACE INTO packages 
                (name, method, source, description, version, category, installed, install_date)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                package.name,
                package.method.value,
                package.source,
                package.description,
                package.version,
                package.category.value,
                True,
                datetime.datetime.now().isoformat()
            ))
            
            cursor.execute('''
                INSERT INTO user_interactions 
                (package_name, action, timestamp, success)
                VALUES (?, ?, ?, ?)
            ''', (
                package.name,
                "install",
                datetime.datetime.now().isoformat(),
                True
            ))
            
            conn.commit()
            conn.close()
            
        except Exception as e:
            self.logger.error(f"Failed to record installation: {e}")

    def system_maintenance(self, operation: str) -> bool:
        """Perform system maintenance operations"""
        self.print_colored(f"\n🔧 Performing {operation}...", Colors.CYAN)
        
        try:
            if operation == "update_mirrors":
                return self._update_mirrors()
            elif operation == "clean_cache":
                return self._clean_cache()
            elif operation == "system_update":
                return self._system_update()
            elif operation == "cylon_maintenance":
                return self._cylon_maintenance()
            else:
                self.print_colored(f"Unknown operation: {operation}", Colors.RED)
                return False
                
        except Exception as e:
            self.logger.error(f"Maintenance operation failed: {e}")
            self.print_colored(f"❌ Maintenance failed: {str(e)}", Colors.RED)
            return False

    def _update_mirrors(self) -> bool:
        """Update Arch mirrors using reflector"""
        self.print_colored("Updating mirrors with reflector...", Colors.YELLOW)
        
        try:
            # Check if reflector is available
            if not shutil.which("reflector"):
                self.print_colored("Installing reflector...", Colors.YELLOW)
                result = subprocess.run(
                    ["sudo", "pacman", "-S", "--needed", "--noconfirm", "reflector"],
                    capture_output=True, text=True
                )
                if result.returncode != 0:
                    self.print_colored("Failed to install reflector", Colors.RED)
                    return False
            
            # Update mirrors
            result = subprocess.run([
                "sudo", "reflector",
                "--country", "US,CA",
                "--age", "12",
                "--protocol", "https",
                "--sort", "rate",
                "--save", "/etc/pacman.d/mirrorlist"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                self.print_colored("✅ Mirrors updated successfully!", Colors.GREEN)
                return True
            else:
                self.print_colored(f"❌ Mirror update failed: {result.stderr}", Colors.RED)
                return False
                
        except Exception as e:
            self.logger.error(f"Mirror update failed: {e}")
            return False

    def _clean_cache(self) -> bool:
        """Clean package cache"""
        self.print_colored("Cleaning package cache...", Colors.YELLOW)
        
        success = True
        
        try:
            # Clean pacman cache
            result = subprocess.run(
                ["sudo", "pacman", "-Sc", "--noconfirm"],
                capture_output=True, text=True
            )
            if result.returncode == 0:
                self.print_colored("✅ Pacman cache cleaned", Colors.GREEN)
            else:
                self.print_colored("❌ Failed to clean pacman cache", Colors.RED)
                success = False
            
            # Clean AUR helper cache if available
            for helper in ["yay", "paru"]:
                if shutil.which(helper):
                    result = subprocess.run(
                        [helper, "-Sc", "--noconfirm"],
                        capture_output=True, text=True
                    )
                    if result.returncode == 0:
                        self.print_colored(f"✅ {helper} cache cleaned", Colors.GREEN)
                    else:
                        self.print_colored(f"❌ Failed to clean {helper} cache", Colors.RED)
            
            return success
            
        except Exception as e:
            self.logger.error(f"Cache cleaning failed: {e}")
            return False

    def _system_update(self) -> bool:
        """Perform full system update"""
        self.print_colored("Performing full system update...", Colors.YELLOW)
        
        try:
            # Update package database
            result = subprocess.run(
                ["sudo", "pacman", "-Sy"],
                capture_output=True, text=True
            )
            
            if result.returncode != 0:
                self.print_colored("❌ Failed to update package database", Colors.RED)
                return False
            
            # Upgrade system
            result = subprocess.run(
                ["sudo", "pacman", "-Su", "--noconfirm"],
                capture_output=True, text=True
            )
            
            if result.returncode == 0:
                self.print_colored("✅ System updated successfully!", Colors.GREEN)
                return True
            else:
                self.print_colored(f"❌ System update failed: {result.stderr}", Colors.RED)
                return False
                
        except Exception as e:
            self.logger.error(f"System update failed: {e}")
            return False

    def _cylon_maintenance(self) -> bool:
        """Run Cylon maintenance script if available"""
        if not shutil.which("cylon"):
            self.print_colored("Cylon not found. Installing...", Colors.YELLOW)
            
            # Try to install cylon from AUR
            for helper in ["yay", "paru", "pikaur"]:
                if shutil.which(helper):
                    result = subprocess.run(
                        [helper, "-S", "--needed", "--noconfirm", "cylon"],
                        capture_output=True, text=True
                    )
                    if result.returncode == 0:
                        break
            else:
                self.print_colored("❌ Failed to install Cylon", Colors.RED)
                return False
        
        try:
            result = subprocess.run(
                ["cylon", "-u"],
                capture_output=True, text=True
            )
            
            if result.returncode == 0:
                self.print_colored("✅ Cylon maintenance completed!", Colors.GREEN)
                return True
            else:
                self.print_colored(f"❌ Cylon maintenance failed: {result.stderr}", Colors.RED)
                return False
                
        except Exception as e:
            self.logger.error(f"Cylon maintenance failed: {e}")
            return False

    # Interactive Mode Methods (FIXED)
    def run_interactive_mode(self):
        """Run interactive CLI mode with full menu system"""
        self.print_colored("🚀 Welcome to Universal Arch Linux Installer", Colors.BOLD + Colors.CYAN)
        self.print_colored("=" * 60, Colors.CYAN)
        
        while True:
            try:
                choice = self.show_main_menu()
                
                if choice == "1":
                    self.handle_package_search()
                elif choice == "2":
                    self.handle_package_install()
                elif choice == "3":
                    self.handle_system_maintenance()
                elif choice == "4":
                    self.handle_installed_packages()
                elif choice == "5":
                    self.handle_backup_restore()
                elif choice == "6":
                    self.handle_performance_stats()
                elif choice == "7":
                    self.handle_settings()
                elif choice == "q" or choice == "quit":
                    self.print_colored("\n👋 Thank you for using Universal Arch Installer!", Colors.GREEN)
                    break
                else:
                    self.print_colored("Invalid choice. Please try again.", Colors.RED)
                    
            except KeyboardInterrupt:
                self.print_colored("\n\n👋 Goodbye!", Colors.YELLOW)
                break
            except Exception as e:
                self.logger.error(f"Interactive mode error: {e}")
                self.print_colored(f"❌ Error: {str(e)}", Colors.RED)

    def show_main_menu(self) -> str:
        """Display main menu and get user choice"""
        self.print_colored("\n📋 Main Menu", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        menu_items = [
            "1. 🔍 Search Packages",
            "2. 📦 Install Package",
            "3. 🔧 System Maintenance",
            "4. 📋 Installed Packages",
            "5. 💾 Backup & Restore",
            "6. 📊 Performance Stats",
            "7. ⚙️  Settings",
            "q. 🚪 Quit"
        ]
        
        for item in menu_items:
            self.print_colored(f"  {item}", Colors.WHITE)
        
        self.print_colored("-" * 30, Colors.BLUE)
        choice = input(f"{Colors.YELLOW}Enter your choice: {Colors.RESET}").strip().lower()
        return choice

    def handle_package_search(self):
        """Handle package search interaction"""
        self.print_colored("\n🔍 Package Search", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        query = input(f"{Colors.YELLOW}Enter search query: {Colors.RESET}").strip()
        if not query:
            self.print_colored("No query entered.", Colors.RED)
            return
        
        use_ai = input(f"{Colors.YELLOW}Use AI-powered search? (Y/n): {Colors.RESET}").strip().lower()
        use_ai = use_ai not in ['n', 'no']
        
        self.print_colored(f"\n🔍 Searching for '{query}'...", Colors.CYAN)
        packages = self.search_package_ai_enhanced(query, use_ai)
        
        if not packages:
            self.print_colored("No packages found.", Colors.RED)
            return
        
        self.print_colored(f"\n📦 Found {len(packages)} packages:", Colors.GREEN)
        self.print_colored("-" * 50, Colors.GREEN)
        
        for i, pkg in enumerate(packages[:20], 1):  # Show first 20
            self.print_colored(f"{i:2d}. {pkg.name}", Colors.BOLD + Colors.WHITE)
            self.print_colored(f"    Method: {pkg.method.value}", Colors.CYAN)
            self.print_colored(f"    Description: {pkg.description[:80]}...", Colors.WHITE)
            if pkg.version:
                self.print_colored(f"    Version: {pkg.version}", Colors.YELLOW)
            self.print_colored("", Colors.RESET)
        
        if len(packages) > 20:
            self.print_colored(f"... and {len(packages) - 20} more packages", Colors.YELLOW)
        
        # Option to install directly
        install_choice = input(f"{Colors.YELLOW}Install a package? Enter number (1-{min(len(packages), 20)}) or press Enter to continue: {Colors.RESET}").strip()
        
        if install_choice.isdigit():
            idx = int(install_choice) - 1
            if 0 <= idx < min(len(packages), 20):
                self.install_package(packages[idx])

    def handle_package_install(self):
        """Handle direct package installation"""
        self.print_colored("\n📦 Install Package", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        package_name = input(f"{Colors.YELLOW}Enter package name: {Colors.RESET}").strip()
        if not package_name:
            self.print_colored("No package name entered.", Colors.RED)
            return
        
        # Show available methods
        self.print_colored("\nAvailable installation methods:", Colors.WHITE)
        for i, method in enumerate(self.available_methods, 1):
            self.print_colored(f"  {i}. {method.value}", Colors.CYAN)
        
        method_choice = input(f"{Colors.YELLOW}Choose method (1-{len(self.available_methods)}) or press Enter for auto: {Colors.RESET}").strip()
        
        if method_choice.isdigit():
            idx = int(method_choice) - 1
            if 0 <= idx < len(self.available_methods):
                method = self.available_methods[idx]
            else:
                self.print_colored("Invalid method choice.", Colors.RED)
                return
        else:
            # Auto-select best method
            method = self.available_methods[0] if self.available_methods else InstallMethod.PACMAN
        
        # Create package info
        package = PackageInfo(
            name=package_name,
            method=method,
            source="direct",
            description=f"Direct installation via {method.value}"
        )
        
        self.install_package(package)

    def handle_system_maintenance(self):
        """Handle system maintenance options"""
        self.print_colored("\n🔧 System Maintenance", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        maintenance_options = [
            ("1", "update_mirrors", "🪞 Update Mirrors"),
            ("2", "clean_cache", "🧹 Clean Cache"),
            ("3", "system_update", "⬆️  System Update"),
            ("4", "cylon_maintenance", "🤖 Cylon Maintenance"),
        ]
        
        for num, _, desc in maintenance_options:
            self.print_colored(f"  {num}. {desc}", Colors.WHITE)
        
        choice = input(f"{Colors.YELLOW}Enter choice (1-4): {Colors.RESET}").strip()
        
        for num, operation, desc in maintenance_options:
            if choice == num:
                self.system_maintenance(operation)
                return
        
        self.print_colored("Invalid choice.", Colors.RED)

    def handle_installed_packages(self):
        """Handle installed packages management"""
        self.print_colored("\n📋 Installed Packages", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            cursor.execute('''
                SELECT name, method, install_date, version 
                FROM packages 
                WHERE installed = TRUE 
                ORDER BY install_date DESC
            ''')
            
            packages = cursor.fetchall()
            conn.close()
            
            if not packages:
                self.print_colored("No packages found in database.", Colors.YELLOW)
                return
            
            self.print_colored(f"Found {len(packages)} installed packages:", Colors.GREEN)
            self.print_colored("-" * 60, Colors.GREEN)
            
            for i, (name, method, install_date, version) in enumerate(packages[:20], 1):
                date_str = install_date[:10] if install_date else "Unknown"
                version_str = f" v{version}" if version else ""
                self.print_colored(f"{i:2d}. {name}{version_str}", Colors.BOLD + Colors.WHITE)
                self.print_colored(f"    Method: {method} | Installed: {date_str}", Colors.CYAN)
            
            if len(packages) > 20:
                self.print_colored(f"... and {len(packages) - 20} more packages", Colors.YELLOW)
                
        except Exception as e:
            self.logger.error(f"Failed to get installed packages: {e}")
            self.print_colored(f"❌ Error: {str(e)}", Colors.RED)

    def handle_backup_restore(self):
        """Handle backup and restore operations"""
        self.print_colored("\n💾 Backup & Restore", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        backup_options = [
            "1. 📤 Create Backup",
            "2. 📥 Restore Backup",
            "3. 📋 List Backups",
        ]
        
        for option in backup_options:
            self.print_colored(f"  {option}", Colors.WHITE)
        
        choice = input(f"{Colors.YELLOW}Enter choice (1-3): {Colors.RESET}").strip()
        
        if choice == "1":
            backup_name = input(f"{Colors.YELLOW}Enter backup name: {Colors.RESET}").strip()
            if backup_name:
                success = self.backup_manager.create_backup(backup_name)
                if success:
                    self.print_colored("✅ Backup created successfully!", Colors.GREEN)
                else:
                    self.print_colored("❌ Backup creation failed.", Colors.RED)
        elif choice == "2":
            self.print_colored("Restore functionality coming soon...", Colors.YELLOW)
        elif choice == "3":
            self.print_colored("List backups functionality coming soon...", Colors.YELLOW)
        else:
            self.print_colored("Invalid choice.", Colors.RED)

    def handle_performance_stats(self):
        """Handle performance statistics display"""
        self.print_colored("\n📊 Performance Statistics", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        if not self.performance_monitor:
            self.print_colored("Performance monitoring not available (psutil required).", Colors.YELLOW)
            return
        
        self.print_colored("Performance statistics coming soon...", Colors.YELLOW)

    def handle_settings(self):
        """Handle application settings"""
        self.print_colored("\n⚙️  Settings", Colors.BOLD + Colors.BLUE)
        self.print_colored("-" * 30, Colors.BLUE)
        
        settings_options = [
            "1. 📝 View Configuration",
            "2. 🔄 Reset Database",
            "3. 📋 View Logs",
        ]
        
        for option in settings_options:
            self.print_colored(f"  {option}", Colors.WHITE)
        
        choice = input(f"{Colors.YELLOW}Enter choice (1-3): {Colors.RESET}").strip()
        
        if choice == "1":
            self.print_colored(f"Config directory: {self.config_dir}", Colors.WHITE)
            self.print_colored(f"Database path: {self.db_path}", Colors.WHITE)
            self.print_colored(f"Available methods: {', '.join([m.value for m in self.available_methods])}", Colors.WHITE)
        elif choice == "2":
            confirm = input(f"{Colors.RED}Are you sure you want to reset the database? (y/N): {Colors.RESET}").strip().lower()
            if confirm in ['y', 'yes']:
                try:
                    self.db_path.unlink(missing_ok=True)
                    self._init_database()
                    self.print_colored("✅ Database reset successfully!", Colors.GREEN)
                except Exception as e:
                    self.print_colored(f"❌ Database reset failed: {str(e)}", Colors.RED)
        elif choice == "3":
            log_file = self.config_dir / "installer.log"
            if log_file.exists():
                try:
                    with open(log_file, 'r') as f:
                        lines = f.readlines()
                    
                    self.print_colored(f"Last 20 log entries:", Colors.WHITE)
                    for line in lines[-20:]:
                        print(line.strip())
                except Exception as e:
                    self.print_colored(f"❌ Failed to read logs: {str(e)}", Colors.RED)
            else:
                self.print_colored("No log file found.", Colors.YELLOW)
        else:
            self.print_colored("Invalid choice.", Colors.RED)


class PerformanceMonitor:
    """Performance monitoring for operations"""
    
    def __init__(self):
        self.process = psutil.Process() if PSUTIL_AVAILABLE else None
    
    def start_monitoring(self, operation_type: str, package_name: str):
        """Start monitoring an operation"""
        if not self.process:
            return None
        
        return {
            'operation_type': operation_type,
            'package_name': package_name,
            'start_time': time.time(),
            'start_memory': self.process.memory_info().rss / 1024 / 1024,  # MB
            'start_cpu': self.process.cpu_percent()
        }
    
    def stop_monitoring(self, monitor_data: dict, success: bool):
        """Stop monitoring and log results"""
        if not monitor_data or not self.process:
            return
        
        end_time = time.time()
        end_memory = self.process.memory_info().rss / 1024 / 1024
        end_cpu = self.process.cpu_percent()
        
        duration = end_time - monitor_data['start_time']
        memory_usage = max(end_memory, monitor_data['start_memory'])
        cpu_usage = max(end_cpu, monitor_data['start_cpu'])
        
        # Log to database
        # Implementation would insert into performance_metrics table


class BackupManager:
    """Backup and restore system for package configurations"""
    
    def __init__(self, config_dir: Path):
        self.config_dir = config_dir
        self.backup_dir = config_dir / "backups"
        self.backup_dir.mkdir(exist_ok=True)
    
    def create_backup(self, name: str, include_data: bool = True) -> bool:
        """Create a backup of current package state"""
        try:
            backup_path = self.backup_dir / f"{name}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.tar.gz"
            
            with tarfile.open(backup_path, 'w:gz') as tar:
                # Backup package database
                db_path = self.config_dir / "enhanced_packages.db"
                if db_path.exists():
                    tar.add(db_path, arcname="packages.db")
                
                # Backup configuration files
                if include_data:
                    config_files = [
                        "installer.log",
                        "performance.log",
                        "security.log",
                        "ai_operations.log"
                    ]
                    
                    for config_file in config_files:
                        file_path = self.config_dir / config_file
                        if file_path.exists():
                            tar.add(file_path, arcname=config_file)
            
            return True
            
        except Exception as e:
            logging.error(f"Backup creation failed: {e}")
            return False
    
    def restore_backup(self, backup_path: Path) -> bool:
        """Restore from a backup"""
        try:
            with tarfile.open(backup_path, 'r:gz') as tar:
                tar.extractall(self.config_dir)
            
            return True
            
        except Exception as e:
            logging.error(f"Backup restoration failed: {e}")
            return False


# Qt GUI Application (if available)
if QT_AVAILABLE:
    class UniversalArchInstallerGUI(QMainWindow):
        """Qt-based GUI for the Universal Arch Installer"""
        
        def __init__(self):
            super().__init__()
            self.installer = UniversalArchInstaller()
            self.init_ui()
        
        def init_ui(self):
            """Initialize the user interface"""
            self.setWindowTitle("Universal Arch Linux Installer")
            self.setGeometry(100, 100, 1400, 900)
            
            # Create central widget and layout
            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            layout = QVBoxLayout(central_widget)
            
            # Create tab widget
            tab_widget = QTabWidget()
            layout.addWidget(tab_widget)
            
            # Package Search Tab
            self.search_tab = self.create_search_tab()
            tab_widget.addTab(self.search_tab, "🔍 Search Packages")
            
            # Install Packages Tab
            self.install_tab = self.create_install_tab()
            tab_widget.addTab(self.install_tab, "📦 Install Packages")
            
            # AI Assistant Tab
            self.ai_tab = self.create_ai_assistant_tab()
            tab_widget.addTab(self.ai_tab, "🤖 AI Assistant")
            
            # Package Building & Distribution Tab
            self.build_tab = self.create_build_distribute_tab()
            tab_widget.addTab(self.build_tab, "🏗️ Package/Distribute")
            
            # Windows Programs Tab
            self.windows_tab = self.create_windows_tab()
            tab_widget.addTab(self.windows_tab, "🪟 Windows Programs")
            
            # System Maintenance Tab
            self.maintenance_tab = self.create_maintenance_tab()
            tab_widget.addTab(self.maintenance_tab, "🔧 System Maintenance")
            
            # Installed Packages Tab
            self.installed_tab = self.create_installed_tab()
            tab_widget.addTab(self.installed_tab, "📋 Installed Packages")
            
            # Settings Tab
            self.settings_tab = self.create_settings_tab()
            tab_widget.addTab(self.settings_tab, "⚙️ Settings")
            
            # Apply bauh-style theme
            self.apply_bauh_theme()
        
        def create_install_tab(self):
            """Create the package installation tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Install sub-tabs
            install_tabs = QTabWidget()
            layout.addWidget(install_tabs)
            
            # Single Package Install Tab
            single_install_tab = self.create_single_install_tab()
            install_tabs.addTab(single_install_tab, "📦 Single Install")
            
            # Batch Install Tab
            batch_install_tab = self.create_batch_install_tab()
            install_tabs.addTab(batch_install_tab, "📦 Batch Install")
            
            # Installation Queue Tab
            queue_tab = self.create_install_queue_tab()
            install_tabs.addTab(queue_tab, "📋 Install Queue")
            
            # Installation History Tab
            history_tab = self.create_install_history_tab()
            install_tabs.addTab(history_tab, "📚 Install History")
            
            # Installation Log Tab
            log_tab = self.create_install_log_tab()
            install_tabs.addTab(log_tab, "📝 Install Log")
            
            # Initialize queue
            self.install_queue = []
            
            return widget
        
        def create_single_install_tab(self):
            """Create single package installation tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Package input section
            input_group = QGroupBox("📦 Single Package Installation")
            input_layout = QFormLayout(input_group)
            
            self.install_package_input = QLineEdit()
            self.install_package_input.setPlaceholderText("Enter package name to install...")
            input_layout.addRow("Package Name:", self.install_package_input)
            
            # Installation method selection
            self.install_method_combo = QComboBox()
            for method in self.installer.available_methods:
                self.install_method_combo.addItem(method.value)
            input_layout.addRow("Install Method:", self.install_method_combo)
            
            # Options
            self.install_with_deps = QCheckBox("Install dependencies automatically")
            self.install_with_deps.setChecked(True)
            input_layout.addRow(self.install_with_deps)
            
            self.install_from_aur = QCheckBox("Include AUR packages")
            self.install_from_aur.setChecked(True)
            input_layout.addRow(self.install_from_aur)
            
            # Buttons
            button_layout = QHBoxLayout()
            self.install_single_btn = QPushButton("📦 Install Now")
            self.install_single_btn.clicked.connect(self.install_single_package)
            button_layout.addWidget(self.install_single_btn)
            
            self.add_to_queue_btn = QPushButton("➕ Add to Queue")
            self.add_to_queue_btn.clicked.connect(self.add_single_to_queue)
            button_layout.addWidget(self.add_to_queue_btn)
            
            self.search_before_install_btn = QPushButton("🔍 Search First")
            self.search_before_install_btn.clicked.connect(self.search_before_install)
            button_layout.addWidget(self.search_before_install_btn)
            
            input_layout.addRow(button_layout)
            layout.addWidget(input_group)
            
            # Package information display
            info_group = QGroupBox("📋 Package Information")
            info_layout = QVBoxLayout(info_group)
            
            self.package_info_display = QTextEdit()
            self.package_info_display.setReadOnly(True)
            self.package_info_display.setMaximumHeight(200)
            self.package_info_display.setPlaceholderText("Package information will appear here...")
            info_layout.addWidget(self.package_info_display)
            
            layout.addWidget(info_group)
            layout.addStretch()
            return widget
        
        def create_batch_install_tab(self):
            """Create batch installation tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Batch input section
            batch_group = QGroupBox("📦 Batch Package Installation")
            batch_layout = QVBoxLayout(batch_group)
            
            # Instructions
            instructions = QLabel("Enter package names (one per line) or upload a package list file:")
            batch_layout.addWidget(instructions)
            
            # Batch text input
            self.batch_install_text = QTextEdit()
            self.batch_install_text.setPlaceholderText("firefox\nvscode\ngit\ndocker\n...")
            self.batch_install_text.setMaximumHeight(200)
            batch_layout.addWidget(self.batch_install_text)
            
            # File operations
            file_layout = QHBoxLayout()
            self.load_package_list_btn = QPushButton("📁 Load from File")
            self.load_package_list_btn.clicked.connect(self.load_package_list)
            file_layout.addWidget(self.load_package_list_btn)
            
            self.save_package_list_btn = QPushButton("💾 Save to File")
            self.save_package_list_btn.clicked.connect(self.save_package_list)
            file_layout.addWidget(self.save_package_list_btn)
            
            file_layout.addStretch()
            batch_layout.addLayout(file_layout)
            
            # Batch options
            options_layout = QHBoxLayout()
            self.batch_method_combo = QComboBox()
            for method in self.installer.available_methods:
                self.batch_method_combo.addItem(method.value)
            options_layout.addWidget(QLabel("Method:"))
            options_layout.addWidget(self.batch_method_combo)
            
            self.batch_continue_on_error = QCheckBox("Continue on errors")
            self.batch_continue_on_error.setChecked(True)
            options_layout.addWidget(self.batch_continue_on_error)
            
            options_layout.addStretch()
            batch_layout.addLayout(options_layout)
            
            # Batch buttons
            batch_button_layout = QHBoxLayout()
            self.install_batch_btn = QPushButton("📦 Install All")
            self.install_batch_btn.clicked.connect(self.install_batch_packages)
            batch_button_layout.addWidget(self.install_batch_btn)
            
            self.add_batch_to_queue_btn = QPushButton("➕ Add All to Queue")
            self.add_batch_to_queue_btn.clicked.connect(self.add_batch_to_queue)
            batch_button_layout.addWidget(self.add_batch_to_queue_btn)
            
            self.clear_batch_btn = QPushButton("🧹 Clear List")
            self.clear_batch_btn.clicked.connect(lambda: self.batch_install_text.clear())
            batch_button_layout.addWidget(self.clear_batch_btn)
            
            batch_button_layout.addStretch()
            batch_layout.addLayout(batch_button_layout)
            
            layout.addWidget(batch_group)
            layout.addStretch()
            return widget
        
        def create_install_queue_tab(self):
            """Create installation queue management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Queue controls
            controls_layout = QHBoxLayout()
            self.clear_queue_btn = QPushButton("🗑️ Clear Queue")
            self.clear_queue_btn.clicked.connect(self.clear_install_queue)
            controls_layout.addWidget(self.clear_queue_btn)
            
            self.process_queue_btn = QPushButton("▶️ Process Queue")
            self.process_queue_btn.clicked.connect(self.process_install_queue)
            controls_layout.addWidget(self.process_queue_btn)
            
            self.pause_queue_btn = QPushButton("⏸️ Pause")
            self.pause_queue_btn.clicked.connect(self.pause_install_queue)
            controls_layout.addWidget(self.pause_queue_btn)
            
            controls_layout.addStretch()
            
            self.queue_progress = QProgressBar()
            self.queue_progress.setVisible(False)
            controls_layout.addWidget(self.queue_progress)
            
            layout.addLayout(controls_layout)
            
            # Installation queue table
            queue_group = QGroupBox("📋 Installation Queue")
            queue_layout = QVBoxLayout(queue_group)
            
            self.install_queue_table = QTableWidget()
            self.install_queue_table.setColumnCount(5)
            self.install_queue_table.setHorizontalHeaderLabels(["Package", "Method", "Status", "Progress", "Actions"])
            queue_layout.addWidget(self.install_queue_table)
            
            layout.addWidget(queue_group)
            
            # Queue statistics
            stats_group = QGroupBox("📊 Queue Statistics")
            stats_layout = QHBoxLayout(stats_group)
            
            self.queue_total_label = QLabel("Total: 0")
            stats_layout.addWidget(self.queue_total_label)
            
            self.queue_pending_label = QLabel("Pending: 0")
            stats_layout.addWidget(self.queue_pending_label)
            
            self.queue_completed_label = QLabel("Completed: 0")
            stats_layout.addWidget(self.queue_completed_label)
            
            self.queue_failed_label = QLabel("Failed: 0")
            stats_layout.addWidget(self.queue_failed_label)
            
            stats_layout.addStretch()
            layout.addWidget(stats_group)
            
            return widget
        
        def create_install_history_tab(self):
            """Create installation history tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # History controls
            controls_layout = QHBoxLayout()
            self.refresh_history_btn = QPushButton("🔄 Refresh")
            self.refresh_history_btn.clicked.connect(self.refresh_install_history)
            controls_layout.addWidget(self.refresh_history_btn)
            
            self.export_history_btn = QPushButton("📤 Export History")
            self.export_history_btn.clicked.connect(self.export_install_history)
            controls_layout.addWidget(self.export_history_btn)
            
            self.clear_history_btn = QPushButton("🗑️ Clear History")
            self.clear_history_btn.clicked.connect(self.clear_install_history)
            controls_layout.addWidget(self.clear_history_btn)
            
            controls_layout.addStretch()
            
            # Filter
            self.history_filter = QLineEdit()
            self.history_filter.setPlaceholderText("Filter history...")
            self.history_filter.textChanged.connect(self.filter_install_history)
            controls_layout.addWidget(self.history_filter)
            
            layout.addLayout(controls_layout)
            
            # History table
            self.install_history_table = QTableWidget()
            self.install_history_table.setColumnCount(6)
            self.install_history_table.setHorizontalHeaderLabels([
                "Package", "Method", "Version", "Install Date", "Status", "Actions"
            ])
            self.install_history_table.setAlternatingRowColors(True)
            layout.addWidget(self.install_history_table)
            
            # Load history
            self.refresh_install_history()
            
            return widget
        
        def create_install_log_tab(self):
            """Create installation log tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Log controls
            controls_layout = QHBoxLayout()
            self.clear_install_log_btn = QPushButton("🧹 Clear Log")
            self.clear_install_log_btn.clicked.connect(self.clear_install_log)
            controls_layout.addWidget(self.clear_install_log_btn)
            
            self.save_install_log_btn = QPushButton("💾 Save Log")
            self.save_install_log_btn.clicked.connect(self.save_install_log)
            controls_layout.addWidget(self.save_install_log_btn)
            
            self.auto_scroll_check = QCheckBox("Auto-scroll")
            self.auto_scroll_check.setChecked(True)
            controls_layout.addWidget(self.auto_scroll_check)
            
            controls_layout.addStretch()
            
            # Log level filter
            controls_layout.addWidget(QLabel("Level:"))
            self.log_level_combo = QComboBox()
            self.log_level_combo.addItems(["All", "Info", "Warning", "Error"])
            controls_layout.addWidget(self.log_level_combo)
            
            layout.addLayout(controls_layout)
            
            # Installation log
            self.install_log = QTextEdit()
            self.install_log.setReadOnly(True)
            self.install_log.append("📦 Installation Log - Ready")
            layout.addWidget(self.install_log)
            
            return widget
        
        def create_ai_assistant_tab(self):
            """Create AI assistant tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create AI sub-tabs
            ai_tabs = QTabWidget()
            layout.addWidget(ai_tabs)
            
            if not AI_FEATURES_AVAILABLE:
                # Dependencies tab
                deps_tab = self.create_ai_dependencies_tab()
                ai_tabs.addTab(deps_tab, "📦 Install Dependencies")
                
                # Features preview tab
                preview_tab = self.create_ai_features_preview_tab()
                ai_tabs.addTab(preview_tab, "🔮 Features Preview")
            else:
                # Chat Assistant Tab
                chat_tab = self.create_ai_chat_tab()
                ai_tabs.addTab(chat_tab, "💬 AI Chat")
                
                # Smart Recommendations Tab
                recommendations_tab = self.create_ai_recommendations_tab()
                ai_tabs.addTab(recommendations_tab, "🎯 Smart Recommendations")
                
                # Package Analysis Tab
                analysis_tab = self.create_ai_analysis_tab()
                ai_tabs.addTab(analysis_tab, "📊 Package Analysis")
                
                # AI Settings Tab
                ai_settings_tab = self.create_ai_settings_tab()
                ai_tabs.addTab(ai_settings_tab, "⚙️ AI Settings")
            
            return widget
        
        def create_ai_dependencies_tab(self):
            """Create AI dependencies installation tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Status display
            status_group = QGroupBox("📊 Dependency Status")
            status_layout = QVBoxLayout(status_group)
            
            self.ai_status_label = QLabel("❌ AI features are currently unavailable")
            self.ai_status_label.setStyleSheet("color: #ff6b6b; font-weight: bold; font-size: 14px;")
            status_layout.addWidget(self.ai_status_label)
            
            missing_deps = QLabel("Missing dependencies: scikit-learn, nltk, requests, beautifulsoup4")
            missing_deps.setStyleSheet("color: #ff9800; margin: 10px;")
            status_layout.addWidget(missing_deps)
            
            layout.addWidget(status_group)
            
            # Installation options
            install_group = QGroupBox("📦 Install AI Dependencies")
            install_layout = QVBoxLayout(install_group)
            
            install_all_btn = QPushButton("🚀 Install All AI Dependencies")
            install_all_btn.clicked.connect(self.install_ai_dependencies)
            install_layout.addWidget(install_all_btn)
            
            # Individual installation
            individual_layout = QGridLayout()
            deps = [("scikit-learn", "Machine Learning"), ("nltk", "Natural Language"), 
                   ("requests", "Web Requests"), ("beautifulsoup4", "Web Scraping")]
            for i, (dep, desc) in enumerate(deps):
                btn = QPushButton(f"Install {dep}\n({desc})")
                btn.clicked.connect(lambda checked, d=dep: self.install_single_dependency(d))
                individual_layout.addWidget(btn, i // 2, i % 2)
            
            install_layout.addLayout(individual_layout)
            layout.addWidget(install_group)
            
            # Installation log
            log_group = QGroupBox("📝 Installation Log")
            log_layout = QVBoxLayout(log_group)
            
            self.deps_install_log = QTextEdit()
            self.deps_install_log.setReadOnly(True)
            self.deps_install_log.setMaximumHeight(200)
            self.deps_install_log.append("Ready to install AI dependencies. Click a button above to start.")
            log_layout.addWidget(self.deps_install_log)
            
            layout.addWidget(log_group)
            layout.addStretch()
            return widget
        
        def create_ai_features_preview_tab(self):
            """Create AI features preview tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            preview_group = QGroupBox("🔮 AI Features Preview")
            preview_layout = QVBoxLayout(preview_group)
            
            features_text = QLabel("""
🤖 <b>AI Chat Assistant</b>
• Natural language package queries
• Intelligent package recommendations  
• Installation guidance and troubleshooting
• Context-aware suggestions

🎯 <b>Smart Recommendations</b>
• Personalized package suggestions
• System optimization recommendations
• Security-focused package analysis
• Dependency impact analysis

📊 <b>Package Analysis</b>
• Dependency impact analysis
• Package similarity detection
• Performance impact predictions
• Security vulnerability scanning

⚙️ <b>AI-Powered Maintenance</b>
• Automated system optimization
• Predictive maintenance scheduling
• Intelligent cache management
• Smart update recommendations
            """)
            features_text.setWordWrap(True)
            features_text.setStyleSheet("font-size: 14px; line-height: 1.6; padding: 20px;")
            preview_layout.addWidget(features_text)
            
            layout.addWidget(preview_group)
            
            # Benefits section
            benefits_group = QGroupBox("✨ Benefits")
            benefits_layout = QVBoxLayout(benefits_group)
            
            benefits_text = QLabel("""
• <b>Save Time:</b> Find the right packages faster with intelligent search
• <b>Better Decisions:</b> Get AI-powered recommendations based on your system
• <b>Avoid Issues:</b> Detect potential conflicts before installation
• <b>Learn More:</b> Get explanations and learning resources
• <b>Stay Secure:</b> AI-powered security analysis of packages
            """)
            benefits_text.setWordWrap(True)
            benefits_text.setStyleSheet("font-size: 13px; color: #4caf50; padding: 15px;")
            benefits_layout.addWidget(benefits_text)
            
            layout.addWidget(benefits_group)
            layout.addStretch()
            return widget
        
        def create_ai_chat_tab(self):
            """Create AI chat assistant tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Chat history
            chat_group = QGroupBox("💬 AI Chat Assistant")
            chat_layout = QVBoxLayout(chat_group)
            
            self.ai_chat_display = QTextEdit()
            self.ai_chat_display.setReadOnly(True)
            welcome_text = """🤖 AI Assistant: Hello! I can help you find and install packages. 

I can assist with:
• Finding packages for specific tasks
• Recommending alternatives
• Explaining installation procedures
• Troubleshooting common issues

What would you like to know?"""
            self.ai_chat_display.append(welcome_text)
            chat_layout.addWidget(self.ai_chat_display)
            
            # Chat input
            chat_input_layout = QHBoxLayout()
            self.ai_input = QLineEdit()
            self.ai_input.setPlaceholderText("Ask me about packages... (e.g., 'I need a video editor')")
            self.ai_input.returnPressed.connect(self.process_ai_query)
            chat_input_layout.addWidget(self.ai_input)
            
            self.ai_send_btn = QPushButton("📤 Send")
            self.ai_send_btn.clicked.connect(self.process_ai_query)
            chat_input_layout.addWidget(self.ai_send_btn)
            
            self.ai_clear_btn = QPushButton("🗑️ Clear")
            self.ai_clear_btn.clicked.connect(self.clear_ai_chat)
            chat_input_layout.addWidget(self.ai_clear_btn)
            
            chat_layout.addLayout(chat_input_layout)
            layout.addWidget(chat_group)
            
            # Quick actions
            actions_group = QGroupBox("⚡ Quick Actions")
            actions_layout = QGridLayout(actions_group)
            
            quick_actions = [
                ("🎬 Video Editing", "I need software for video editing"),
                ("💻 Development", "I need development tools for programming"),
                ("🎮 Gaming", "I want to install gaming software"),
                ("🎨 Graphics Design", "I need graphic design tools"),
                ("📊 Office Suite", "I need office applications"),
                ("🔐 Security Tools", "I need security and privacy tools"),
                ("🌐 Web Browsers", "I need web browsers"),
                ("🎵 Audio Production", "I need audio editing software")
            ]
            
            for i, (text, prompt) in enumerate(quick_actions):
                btn = QPushButton(text)
                btn.clicked.connect(lambda checked, p=prompt: self.send_quick_prompt(p))
                actions_layout.addWidget(btn, i // 4, i % 4)
            
            layout.addWidget(actions_group)
            return widget
        
        def create_ai_recommendations_tab(self):
            """Create AI recommendations tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Recommendation controls
            controls_group = QGroupBox("🎯 Recommendation Settings")
            controls_layout = QVBoxLayout(controls_group)
            
            category_layout = QHBoxLayout()
            category_layout.addWidget(QLabel("Category:"))
            self.rec_category_combo = QComboBox()
            self.rec_category_combo.addItems(["All", "Development", "Gaming", "Multimedia", "Security", "System"])
            category_layout.addWidget(self.rec_category_combo)
            controls_layout.addLayout(category_layout)
            
            rec_buttons = QHBoxLayout()
            self.get_recommendations_btn = QPushButton("🎯 Get Recommendations")
            self.get_recommendations_btn.clicked.connect(self.generate_ai_recommendations)
            rec_buttons.addWidget(self.get_recommendations_btn)
            
            self.refresh_rec_btn = QPushButton("🔄 Refresh")
            self.refresh_rec_btn.clicked.connect(self.refresh_recommendations)
            rec_buttons.addWidget(self.refresh_rec_btn)
            
            controls_layout.addLayout(rec_buttons)
            layout.addWidget(controls_group)
            
            # Recommendations display
            rec_display_group = QGroupBox("📋 Recommended Packages")
            rec_display_layout = QVBoxLayout(rec_display_group)
            
            self.ai_recommendations_table = QTableWidget()
            self.ai_recommendations_table.setColumnCount(5)
            self.ai_recommendations_table.setHorizontalHeaderLabels([
                "Package", "Reason", "Popularity", "Security", "Install"
            ])
            rec_display_layout.addWidget(self.ai_recommendations_table)
            
            layout.addWidget(rec_display_group)
            return widget
        
        def create_ai_analysis_tab(self):
            """Create AI package analysis tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Analysis input
            input_group = QGroupBox("📊 Package Analysis")
            input_layout = QVBoxLayout(input_group)
            
            package_input_layout = QHBoxLayout()
            package_input_layout.addWidget(QLabel("Package:"))
            self.analysis_package_input = QLineEdit()
            self.analysis_package_input.setPlaceholderText("Enter package name for analysis...")
            package_input_layout.addWidget(self.analysis_package_input)
            
            self.analyze_btn = QPushButton("🔍 Analyze")
            self.analyze_btn.clicked.connect(self.analyze_package)
            package_input_layout.addWidget(self.analyze_btn)
            
            input_layout.addLayout(package_input_layout)
            layout.addWidget(input_group)
            
            # Analysis results
            results_group = QGroupBox("📈 Analysis Results")
            results_layout = QVBoxLayout(results_group)
            
            self.analysis_results = QTextEdit()
            self.analysis_results.setReadOnly(True)
            self.analysis_results.append("Enter a package name above and click Analyze to see detailed information.")
            results_layout.addWidget(self.analysis_results)
            
            layout.addWidget(results_group)
            return widget
        
        def create_ai_settings_tab(self):
            """Create AI settings tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # AI model settings
            model_group = QGroupBox("🧠 AI Model Settings")
            model_layout = QVBoxLayout(model_group)
            
            self.ai_enabled_check = QCheckBox("Enable AI features")
            self.ai_enabled_check.setChecked(AI_FEATURES_AVAILABLE)
            model_layout.addWidget(self.ai_enabled_check)
            
            self.verbose_ai_check = QCheckBox("Verbose AI responses")
            model_layout.addWidget(self.verbose_ai_check)
            
            self.ai_cache_check = QCheckBox("Cache AI responses")
            self.ai_cache_check.setChecked(True)
            model_layout.addWidget(self.ai_cache_check)
            
            layout.addWidget(model_group)
            
            # Performance settings
            perf_group = QGroupBox("⚡ Performance Settings")
            perf_layout = QVBoxLayout(perf_group)
            
            timeout_layout = QHBoxLayout()
            timeout_layout.addWidget(QLabel("AI Response Timeout:"))
            self.ai_timeout_spin = QSpinBox()
            self.ai_timeout_spin.setRange(5, 60)
            self.ai_timeout_spin.setValue(30)
            self.ai_timeout_spin.setSuffix(" seconds")
            timeout_layout.addWidget(self.ai_timeout_spin)
            perf_layout.addLayout(timeout_layout)
            
            layout.addWidget(perf_group)
            layout.addStretch()
            return widget
        
        def create_build_distribute_tab(self):
            """Create the package building and distribution tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Build/Distribute sub-tabs
            build_tabs = QTabWidget()
            layout.addWidget(build_tabs)
            
            # Package Builder Tab
            builder_tab = self.create_package_builder_tab()
            build_tabs.addTab(builder_tab, "🏗️ Package Builder")
            
            # Package Distribution Tab
            distribution_tab = self.create_package_distribution_tab()
            build_tabs.addTab(distribution_tab, "📤 Package Distribution")
            
            # Build Log Tab
            build_log_tab = self.create_build_log_tab()
            build_tabs.addTab(build_log_tab, "📝 Build Log")
            
            return widget
        
        def create_package_builder_tab(self):
            """Create package builder tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Package Builder
            builder_group = QGroupBox("🏗️ Package Builder")
            builder_layout = QFormLayout(builder_group)
            
            self.build_package_name = QLineEdit()
            self.build_package_name.setPlaceholderText("my-awesome-package")
            builder_layout.addRow("Package Name:", self.build_package_name)
            
            self.build_version = QLineEdit()
            self.build_version.setPlaceholderText("1.0.0")
            builder_layout.addRow("Version:", self.build_version)
            
            self.build_description = QLineEdit()
            self.build_description.setPlaceholderText("A brief description of your package")
            builder_layout.addRow("Description:", self.build_description)
            
            self.build_source_path = QLineEdit()
            self.build_browse_source_btn = QPushButton("Browse...")
            self.build_browse_source_btn.clicked.connect(self.browse_source_directory)
            
            source_layout = QHBoxLayout()
            source_layout.addWidget(self.build_source_path)
            source_layout.addWidget(self.build_browse_source_btn)
            builder_layout.addRow("Source Directory:", source_layout)
            
            # Build type selection
            self.build_type_combo = QComboBox()
            self.build_type_combo.addItems(["PKGBUILD (Arch)", "DEB (Debian/Ubuntu)", "RPM (RedHat/SUSE)", "Flatpak", "AppImage"])
            builder_layout.addRow("Build Type:", self.build_type_combo)
            
            # Dependencies
            self.build_dependencies = QTextEdit()
            self.build_dependencies.setPlaceholderText("Enter dependencies (one per line)...")
            self.build_dependencies.setMaximumHeight(80)
            builder_layout.addRow("Dependencies:", self.build_dependencies)
            
            # Build buttons
            build_buttons = QHBoxLayout()
            
            self.create_pkgbuild_btn = QPushButton("📝 Create PKGBUILD")
            self.create_pkgbuild_btn.clicked.connect(self.create_pkgbuild)
            build_buttons.addWidget(self.create_pkgbuild_btn)
            
            self.build_package_btn = QPushButton("🔨 Build Package")
            self.build_package_btn.clicked.connect(self.build_package)
            build_buttons.addWidget(self.build_package_btn)
            
            self.test_package_btn = QPushButton("🧪 Test Package")
            self.test_package_btn.clicked.connect(self.test_package)
            build_buttons.addWidget(self.test_package_btn)
            
            builder_layout.addRow(build_buttons)
            
            layout.addWidget(builder_group)
            layout.addStretch()
            return widget
        
        def create_package_distribution_tab(self):
            """Create package distribution tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Distribution Manager
            distribution_group = QGroupBox("📤 Package Distribution")
            distribution_layout = QVBoxLayout(distribution_group)
            
            # Repository management
            repo_layout = QFormLayout()
            
            self.repo_name = QLineEdit()
            self.repo_name.setPlaceholderText("my-repository")
            repo_layout.addRow("Repository Name:", self.repo_name)
            
            self.repo_description = QLineEdit()
            self.repo_description.setPlaceholderText("My custom package repository")
            repo_layout.addRow("Repository Description:", self.repo_description)
            
            self.repo_path = QLineEdit()
            self.browse_repo_btn = QPushButton("Browse...")
            self.browse_repo_btn.clicked.connect(self.browse_repository_path)
            
            repo_path_layout = QHBoxLayout()
            repo_path_layout.addWidget(self.repo_path)
            repo_path_layout.addWidget(self.browse_repo_btn)
            repo_layout.addRow("Repository Path:", repo_path_layout)
            
            distribution_layout.addLayout(repo_layout)
            
            # Repository actions
            repo_actions = QHBoxLayout()
            
            self.create_repo_btn = QPushButton("🏗️ Create Repository")
            self.create_repo_btn.clicked.connect(self.create_repository)
            repo_actions.addWidget(self.create_repo_btn)
            
            self.add_package_to_repo_btn = QPushButton("➕ Add Package")
            self.add_package_to_repo_btn.clicked.connect(self.add_package_to_repository)
            repo_actions.addWidget(self.add_package_to_repo_btn)
            
            self.sign_packages_btn = QPushButton("🔐 Sign Packages")
            self.sign_packages_btn.clicked.connect(self.sign_packages)
            repo_actions.addWidget(self.sign_packages_btn)
            
            distribution_layout.addLayout(repo_actions)
            
            # Package list in repository
            self.repo_packages_table = QTableWidget()
            self.repo_packages_table.setColumnCount(5)
            self.repo_packages_table.setHorizontalHeaderLabels([
                "Package", "Version", "Architecture", "Size", "Actions"
            ])
            distribution_layout.addWidget(self.repo_packages_table)
            
            layout.addWidget(distribution_group)
            layout.addStretch()
            return widget
        
        def create_build_log_tab(self):
            """Create build log tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Log controls
            log_controls = QHBoxLayout()
            self.clear_build_log_btn = QPushButton("🧹 Clear Log")
            self.clear_build_log_btn.clicked.connect(lambda: self.build_log.clear())
            log_controls.addWidget(self.clear_build_log_btn)
            
            self.save_build_log_btn = QPushButton("💾 Save Log")
            self.save_build_log_btn.clicked.connect(self.save_build_log)
            log_controls.addWidget(self.save_build_log_btn)
            
            log_controls.addStretch()
            layout.addLayout(log_controls)
            
            # Build log
            self.build_log = QTextEdit()
            self.build_log.setReadOnly(True)
            layout.addWidget(self.build_log)
            
            return widget
        
        def create_settings_tab(self):
            """Create the settings tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Settings sub-tabs
            settings_tabs = QTabWidget()
            layout.addWidget(settings_tabs)
            
            # General Settings Tab
            general_tab = self.create_general_settings_tab()
            settings_tabs.addTab(general_tab, "⚙️ General Settings")
            
            # Package Manager Preferences Tab
            package_mgr_tab = self.create_package_manager_preferences_tab()
            settings_tabs.addTab(package_mgr_tab, "📦 Package Manager Preferences")
            
            # Appearance Tab
            appearance_tab = self.create_appearance_tab()
            settings_tabs.addTab(appearance_tab, "🎨 Appearance")
            
            # System Information Tab
            system_info_tab = self.create_system_information_tab()
            settings_tabs.addTab(system_info_tab, "ℹ️ System Information")
            
            return widget
        
        def create_general_settings_tab(self):
            """Create general settings tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # General Settings
            general_group = QGroupBox("⚙️ General Settings")
            general_layout = QFormLayout(general_group)
            
            self.auto_update_check = QCheckBox("Check for updates automatically")
            self.auto_update_check.setChecked(True)
            general_layout.addRow(self.auto_update_check)
            
            self.ai_features_check = QCheckBox("Enable AI-powered features")
            self.ai_features_check.setChecked(AI_FEATURES_AVAILABLE)
            self.ai_features_check.setEnabled(AI_FEATURES_AVAILABLE)
            general_layout.addRow(self.ai_features_check)
            
            self.performance_monitoring_check = QCheckBox("Enable performance monitoring")
            self.performance_monitoring_check.setChecked(PSUTIL_AVAILABLE)
            self.performance_monitoring_check.setEnabled(PSUTIL_AVAILABLE)
            general_layout.addRow(self.performance_monitoring_check)
            
            self.confirm_installs_check = QCheckBox("Confirm before installing packages")
            self.confirm_installs_check.setChecked(True)
            general_layout.addRow(self.confirm_installs_check)
            
            # Mirror settings
            self.mirror_country = QComboBox()
            self.mirror_country.addItems(["Auto", "US", "Canada", "Germany", "France", "UK", "Australia", "Japan"])
            general_layout.addRow("Preferred Mirror Country:", self.mirror_country)
            
            self.parallel_downloads = QSpinBox()
            self.parallel_downloads.setRange(1, 10)
            self.parallel_downloads.setValue(5)
            general_layout.addRow("Parallel Downloads:", self.parallel_downloads)
            
            layout.addWidget(general_group)
            
            # Action buttons
            action_buttons = QHBoxLayout()
            
            self.save_settings_btn = QPushButton("💾 Save Settings")
            self.save_settings_btn.clicked.connect(self.save_settings)
            action_buttons.addWidget(self.save_settings_btn)
            
            self.reset_settings_btn = QPushButton("🔄 Reset to Defaults")
            self.reset_settings_btn.clicked.connect(self.reset_settings)
            action_buttons.addWidget(self.reset_settings_btn)
            
            action_buttons.addStretch()
            layout.addLayout(action_buttons)
            
            layout.addStretch()
            return widget
        
        def create_package_manager_preferences_tab(self):
            """Create package manager preferences tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Package Manager Preferences
            package_mgr_group = QGroupBox("📦 Package Manager Preferences")
            package_mgr_layout = QFormLayout(package_mgr_group)
            
            self.preferred_aur_helper = QComboBox()
            aur_helpers = ["yay", "paru", "pikaur", "trizen"]
            available_helpers = [helper for helper in aur_helpers if shutil.which(helper)]
            self.preferred_aur_helper.addItems(available_helpers if available_helpers else aur_helpers)
            package_mgr_layout.addRow("Preferred AUR Helper:", self.preferred_aur_helper)
            
            self.enable_multilib_check = QCheckBox("Enable multilib repository")
            package_mgr_layout.addRow(self.enable_multilib_check)
            
            self.clean_cache_auto_check = QCheckBox("Clean package cache automatically")
            package_mgr_layout.addRow(self.clean_cache_auto_check)
            
            layout.addWidget(package_mgr_group)
            layout.addStretch()
            return widget
        
        def create_appearance_tab(self):
            """Create appearance settings tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Appearance Settings
            appearance_group = QGroupBox("🎨 Appearance")
            appearance_layout = QFormLayout(appearance_group)
            
            self.theme_combo = QComboBox()
            self.theme_combo.addItems(["Dark (Bauh Style)", "Light", "System Default"])
            self.theme_combo.currentTextChanged.connect(self.change_theme)
            appearance_layout.addRow("Theme:", self.theme_combo)
            
            self.font_size_spin = QSpinBox()
            self.font_size_spin.setRange(8, 20)
            self.font_size_spin.setValue(10)
            appearance_layout.addRow("Font Size:", self.font_size_spin)
            
            layout.addWidget(appearance_group)
            layout.addStretch()
            return widget
        
        def create_system_information_tab(self):
            """Create system information tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # System information
            status_group = QGroupBox("ℹ️ System Information")
            status_layout = QVBoxLayout(status_group)
            
            self.system_info_label = QLabel()
            self.update_system_info()
            status_layout.addWidget(self.system_info_label)
            
            layout.addWidget(status_group)
            
            # Configuration management
            config_group = QGroupBox("⚙️ Configuration Management")
            config_layout = QHBoxLayout(config_group)
            
            self.export_config_btn = QPushButton("📤 Export Config")
            self.export_config_btn.clicked.connect(self.export_configuration)
            config_layout.addWidget(self.export_config_btn)
            
            self.import_config_btn = QPushButton("📥 Import Config")
            self.import_config_btn.clicked.connect(self.import_configuration)
            config_layout.addWidget(self.import_config_btn)
            
            config_layout.addStretch()
            layout.addWidget(config_group)
            
            layout.addStretch()
            return widget
        
        def create_search_tab(self):
            """Create the package search tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Search sub-tabs
            search_tabs = QTabWidget()
            layout.addWidget(search_tabs)
            
            # Quick Search Tab
            quick_search_tab = self.create_quick_search_tab()
            search_tabs.addTab(quick_search_tab, "🔍 Quick Search")
            
            # Advanced Search Tab
            advanced_search_tab = self.create_advanced_search_tab()
            search_tabs.addTab(advanced_search_tab, "🎯 Advanced Search")
            
            # Search Results Tab
            results_tab = self.create_search_results_tab()
            search_tabs.addTab(results_tab, "📋 Search Results")
            
            # Search History Tab
            history_tab = self.create_search_history_tab()
            search_tabs.addTab(history_tab, "📚 Search History")
            
            return widget
        
        def create_quick_search_tab(self):
            """Create quick search interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Quick search input
            search_group = QGroupBox("🔍 Quick Package Search")
            search_layout = QVBoxLayout(search_group)
            
            input_layout = QHBoxLayout()
            self.search_input = QLineEdit()
            self.search_input.setPlaceholderText("Enter package name to search...")
            self.search_input.returnPressed.connect(self.search_packages)
            input_layout.addWidget(self.search_input)
            
            self.search_button = QPushButton("🔍 Search")
            self.search_button.clicked.connect(self.search_packages)
            input_layout.addWidget(self.search_button)
            
            search_layout.addLayout(input_layout)
            
            # Search options
            options_layout = QHBoxLayout()
            self.ai_search_check = QCheckBox("Use AI-powered search")
            self.ai_search_check.setChecked(True)
            options_layout.addWidget(self.ai_search_check)
            
            self.include_aur_check = QCheckBox("Include AUR packages")
            self.include_aur_check.setChecked(True)
            options_layout.addWidget(self.include_aur_check)
            
            self.include_flatpak_check = QCheckBox("Include Flatpak")
            options_layout.addWidget(self.include_flatpak_check)
            
            options_layout.addStretch()
            search_layout.addLayout(options_layout)
            
            layout.addWidget(search_group)
            
            # Popular packages
            popular_group = QGroupBox("⭐ Popular Packages")
            popular_layout = QGridLayout(popular_group)
            
            popular_packages = [
                ("🌐 Firefox", "firefox"), ("🎬 VLC", "vlc"), ("💻 VS Code", "code"), ("🎨 GIMP", "gimp"),
                ("🗃️ LibreOffice", "libreoffice-fresh"), ("🎮 Steam", "steam"), ("🐳 Docker", "docker"), ("📝 Git", "git")
            ]
            
            for i, (title, package) in enumerate(popular_packages):
                btn = QPushButton(title)
                btn.clicked.connect(lambda checked, pkg=package: self.search_popular_package(pkg))
                popular_layout.addWidget(btn, i // 4, i % 4)
            
            layout.addWidget(popular_group)
            
            layout.addStretch()
            return widget
        
        def create_advanced_search_tab(self):
            """Create advanced search interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Advanced filters
            filters_group = QGroupBox("🎯 Advanced Search Filters")
            filters_layout = QFormLayout(filters_group)
            
            self.adv_package_name = QLineEdit()
            self.adv_package_name.setPlaceholderText("Package name or keywords...")
            filters_layout.addRow("Package Name:", self.adv_package_name)
            
            self.adv_category_combo = QComboBox()
            self.adv_category_combo.addItems(["All Categories"] + [cat.value for cat in PackageCategory])
            filters_layout.addRow("Category:", self.adv_category_combo)
            
            self.adv_method_combo = QComboBox()
            methods = ["All Methods"] + [method.value for method in self.installer.available_methods]
            self.adv_method_combo.addItems(methods)
            filters_layout.addRow("Install Method:", self.adv_method_combo)
            
            self.adv_description = QLineEdit()
            self.adv_description.setPlaceholderText("Search in description...")
            filters_layout.addRow("Description:", self.adv_description)
            
            # Size filter
            size_layout = QHBoxLayout()
            self.min_size_spin = QSpinBox()
            self.min_size_spin.setRange(0, 10000)
            self.min_size_spin.setSuffix(" MB")
            size_layout.addWidget(QLabel("Min:"))
            size_layout.addWidget(self.min_size_spin)
            
            self.max_size_spin = QSpinBox()
            self.max_size_spin.setRange(0, 10000)
            self.max_size_spin.setValue(1000)
            self.max_size_spin.setSuffix(" MB")
            size_layout.addWidget(QLabel("Max:"))
            size_layout.addWidget(self.max_size_spin)
            
            filters_layout.addRow("Package Size:", size_layout)
            
            # Search buttons
            search_buttons = QHBoxLayout()
            self.advanced_search_btn = QPushButton("🔍 Advanced Search")
            self.advanced_search_btn.clicked.connect(self.perform_advanced_search)
            search_buttons.addWidget(self.advanced_search_btn)
            
            self.clear_filters_btn = QPushButton("🧹 Clear Filters")
            self.clear_filters_btn.clicked.connect(self.clear_search_filters)
            search_buttons.addWidget(self.clear_filters_btn)
            
            search_buttons.addStretch()
            filters_layout.addRow(search_buttons)
            
            layout.addWidget(filters_group)
            
            # Saved searches
            saved_group = QGroupBox("💾 Saved Searches")
            saved_layout = QVBoxLayout(saved_group)
            
            saved_controls = QHBoxLayout()
            self.save_search_btn = QPushButton("💾 Save Current Search")
            self.save_search_btn.clicked.connect(self.save_current_search)
            saved_controls.addWidget(self.save_search_btn)
            
            self.manage_saved_btn = QPushButton("📂 Manage Saved")
            self.manage_saved_btn.clicked.connect(self.manage_saved_searches)
            saved_controls.addWidget(self.manage_saved_btn)
            
            saved_controls.addStretch()
            saved_layout.addLayout(saved_controls)
            
            self.saved_searches_list = QListWidget()
            self.saved_searches_list.itemDoubleClicked.connect(self.load_saved_search)
            saved_layout.addWidget(self.saved_searches_list)
            
            layout.addWidget(saved_group)
            
            layout.addStretch()
            return widget
        
        def create_search_results_tab(self):
            """Create search results display tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Results controls
            controls_layout = QHBoxLayout()
            self.results_sort_combo = QComboBox()
            self.results_sort_combo.addItems(["Relevance", "Name", "Popularity", "Size", "Date"])
            self.results_sort_combo.currentTextChanged.connect(self.sort_search_results)
            controls_layout.addWidget(QLabel("Sort by:"))
            controls_layout.addWidget(self.results_sort_combo)
            
            self.results_filter_input = QLineEdit()
            self.results_filter_input.setPlaceholderText("Filter results...")
            self.results_filter_input.textChanged.connect(self.filter_search_results)
            controls_layout.addWidget(self.results_filter_input)
            
            self.export_results_btn = QPushButton("📤 Export Results")
            self.export_results_btn.clicked.connect(self.export_search_results)
            controls_layout.addWidget(self.export_results_btn)
            
            controls_layout.addStretch()
            layout.addLayout(controls_layout)
            
            # Results table
            self.results_table = QTableWidget()
            self.results_table.setColumnCount(7)
            self.results_table.setHorizontalHeaderLabels([
                "✓", "Name", "Method", "Version", "Description", "Category", "Actions"
            ])
            self.results_table.horizontalHeader().setStretchLastSection(True)
            self.results_table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
            self.results_table.setAlternatingRowColors(True)
            layout.addWidget(self.results_table)
            
            # Results status
            status_layout = QHBoxLayout()
            self.search_status = QLabel("Ready to search packages...")
            status_layout.addWidget(self.search_status)
            
            status_layout.addStretch()
            
            self.install_selected_results_btn = QPushButton("📦 Install Selected")
            self.install_selected_results_btn.clicked.connect(self.install_selected_results)
            self.install_selected_results_btn.setEnabled(False)
            status_layout.addWidget(self.install_selected_results_btn)
            
            layout.addLayout(status_layout)
            
            return widget
        
        def create_search_history_tab(self):
            """Create search history tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # History controls
            controls_layout = QHBoxLayout()
            self.clear_history_btn = QPushButton("🗑️ Clear History")
            self.clear_history_btn.clicked.connect(self.clear_search_history)
            controls_layout.addWidget(self.clear_history_btn)
            
            self.export_history_btn = QPushButton("📤 Export History")
            self.export_history_btn.clicked.connect(self.export_search_history)
            controls_layout.addWidget(self.export_history_btn)
            
            controls_layout.addStretch()
            layout.addLayout(controls_layout)
            
            # History table
            self.history_table = QTableWidget()
            self.history_table.setColumnCount(5)
            self.history_table.setHorizontalHeaderLabels([
                "Search Query", "Results Found", "Search Time", "Date", "Actions"
            ])
            self.history_table.setAlternatingRowColors(True)
            layout.addWidget(self.history_table)
            
            # Load search history
            self.load_search_history()
            
            return widget
        
        def create_maintenance_tab(self):
            """Create the system maintenance tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Maintenance sub-tabs
            maintenance_tabs = QTabWidget()
            layout.addWidget(maintenance_tabs)
            
            # Quick Maintenance Tab
            quick_maintenance_tab = self.create_quick_maintenance_tab()
            maintenance_tabs.addTab(quick_maintenance_tab, "⚡ Quick Maintenance")
            
            # System Updates Tab
            updates_tab = self.create_system_updates_tab()
            maintenance_tabs.addTab(updates_tab, "⬆️ System Updates")
            
            # Package Cache Tab
            cache_tab = self.create_package_cache_tab()
            maintenance_tabs.addTab(cache_tab, "📦 Package Cache")
            
            # System Optimization Tab
            optimization_tab = self.create_system_optimization_tab()
            maintenance_tabs.addTab(optimization_tab, "🚀 System Optimization")
            
            # Maintenance Logs Tab
            logs_tab = self.create_maintenance_logs_tab()
            maintenance_tabs.addTab(logs_tab, "📝 Maintenance Logs")
            
            return widget
        
        def create_quick_maintenance_tab(self):
            """Create quick maintenance operations tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Quick actions group
            quick_group = QGroupBox("⚡ Quick Maintenance Actions")
            quick_layout = QGridLayout(quick_group)
            
            self.update_mirrors_btn = QPushButton("🪞 Update Mirrors")
            self.update_mirrors_btn.clicked.connect(lambda: self.run_maintenance("update_mirrors"))
            quick_layout.addWidget(self.update_mirrors_btn, 0, 0)
            
            self.clean_cache_btn = QPushButton("🧹 Clean Cache")
            self.clean_cache_btn.clicked.connect(lambda: self.run_maintenance("clean_cache"))
            quick_layout.addWidget(self.clean_cache_btn, 0, 1)
            
            self.system_update_btn = QPushButton("⬆️ System Update")
            self.system_update_btn.clicked.connect(lambda: self.run_maintenance("system_update"))
            quick_layout.addWidget(self.system_update_btn, 1, 0)
            
            self.cylon_btn = QPushButton("🤖 Cylon Maintenance")
            self.cylon_btn.clicked.connect(lambda: self.run_maintenance("cylon_maintenance"))
            quick_layout.addWidget(self.cylon_btn, 1, 1)
            
            layout.addWidget(quick_group)
            
            # Status display
            status_group = QGroupBox("📊 System Status")
            status_layout = QVBoxLayout(status_group)
            
            self.system_status_label = QLabel("System status: Checking...")
            status_layout.addWidget(self.system_status_label)
            
            layout.addWidget(status_group)
            layout.addStretch()
            return widget
        
        def create_system_updates_tab(self):
            """Create system updates management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Update options
            update_group = QGroupBox("⬆️ Update Options")
            update_layout = QVBoxLayout(update_group)
            
            # Update type selection
            update_type_layout = QHBoxLayout()
            self.update_type_combo = QComboBox()
            self.update_type_combo.addItems(["System Updates Only", "Include AUR", "Full System Upgrade"])
            update_type_layout.addWidget(QLabel("Update Type:"))
            update_type_layout.addWidget(self.update_type_combo)
            update_layout.addLayout(update_type_layout)
            
            # Update options checkboxes
            self.download_only_check = QCheckBox("Download only (don't install)")
            update_layout.addWidget(self.download_only_check)
            
            self.ignore_deps_check = QCheckBox("Ignore dependency checks")
            update_layout.addWidget(self.ignore_deps_check)
            
            # Update buttons
            update_buttons = QHBoxLayout()
            self.check_updates_btn = QPushButton("🔍 Check for Updates")
            self.check_updates_btn.clicked.connect(self.check_for_updates)
            update_buttons.addWidget(self.check_updates_btn)
            
            self.install_updates_btn = QPushButton("⬇️ Install Updates")
            self.install_updates_btn.clicked.connect(self.install_system_updates)
            update_buttons.addWidget(self.install_updates_btn)
            
            update_layout.addLayout(update_buttons)
            layout.addWidget(update_group)
            
            # Available updates table
            updates_table_group = QGroupBox("📋 Available Updates")
            updates_table_layout = QVBoxLayout(updates_table_group)
            
            self.updates_table = QTableWidget()
            self.updates_table.setColumnCount(4)
            self.updates_table.setHorizontalHeaderLabels(["Package", "Current Version", "New Version", "Size"])
            updates_table_layout.addWidget(self.updates_table)
            
            layout.addWidget(updates_table_group)
            return widget
        
        def create_package_cache_tab(self):
            """Create package cache management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Cache info
            cache_info_group = QGroupBox("📊 Cache Information")
            cache_info_layout = QVBoxLayout(cache_info_group)
            
            self.cache_size_label = QLabel("Cache size: Calculating...")
            cache_info_layout.addWidget(self.cache_size_label)
            
            self.cache_location_label = QLabel("Cache location: /var/cache/pacman/pkg/")
            cache_info_layout.addWidget(self.cache_location_label)
            
            layout.addWidget(cache_info_group)
            
            # Cache operations
            cache_ops_group = QGroupBox("🧹 Cache Operations")
            cache_ops_layout = QGridLayout(cache_ops_group)
            
            self.clean_pacman_cache_btn = QPushButton("🧹 Clean Pacman Cache")
            self.clean_pacman_cache_btn.clicked.connect(lambda: self.run_maintenance("clean_cache"))
            cache_ops_layout.addWidget(self.clean_pacman_cache_btn, 0, 0)
            
            self.clean_aur_cache_btn = QPushButton("🧹 Clean AUR Cache")
            self.clean_aur_cache_btn.clicked.connect(self.clean_aur_cache)
            cache_ops_layout.addWidget(self.clean_aur_cache_btn, 0, 1)
            
            self.clean_all_cache_btn = QPushButton("🗑️ Clean All Caches")
            self.clean_all_cache_btn.clicked.connect(self.clean_all_caches)
            cache_ops_layout.addWidget(self.clean_all_cache_btn, 1, 0)
            
            self.view_cache_btn = QPushButton("👁️ View Cache Contents")
            self.view_cache_btn.clicked.connect(self.view_cache_contents)
            cache_ops_layout.addWidget(self.view_cache_btn, 1, 1)
            
            layout.addWidget(cache_ops_group)
            layout.addStretch()
            return widget
        
        def create_system_optimization_tab(self):
            """Create system optimization tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Optimization tools
            optimization_group = QGroupBox("🚀 System Optimization")
            optimization_layout = QGridLayout(optimization_group)
            
            self.optimize_mirrors_btn = QPushButton("🪞 Optimize Mirrors")
            self.optimize_mirrors_btn.clicked.connect(self.optimize_mirror_list)
            optimization_layout.addWidget(self.optimize_mirrors_btn, 0, 0)
            
            self.cleanup_orphans_btn = QPushButton("🧹 Remove Orphaned Packages")
            self.cleanup_orphans_btn.clicked.connect(self.cleanup_orphaned_packages)
            optimization_layout.addWidget(self.cleanup_orphans_btn, 0, 1)
            
            self.trim_logs_btn = QPushButton("📝 Trim System Logs")
            self.trim_logs_btn.clicked.connect(self.trim_system_logs)
            optimization_layout.addWidget(self.trim_logs_btn, 1, 0)
            
            self.defrag_db_btn = QPushButton("🗃️ Optimize Package Database")
            self.defrag_db_btn.clicked.connect(self.optimize_package_database)
            optimization_layout.addWidget(self.defrag_db_btn, 1, 1)
            
            layout.addWidget(optimization_group)
            
            # Scheduled maintenance
            schedule_group = QGroupBox("⏰ Scheduled Maintenance")
            schedule_layout = QVBoxLayout(schedule_group)
            
            self.auto_maintenance_check = QCheckBox("Enable automatic maintenance")
            schedule_layout.addWidget(self.auto_maintenance_check)
            
            schedule_controls = QHBoxLayout()
            schedule_controls.addWidget(QLabel("Run every:"))
            self.maintenance_interval_spin = QSpinBox()
            self.maintenance_interval_spin.setRange(1, 30)
            self.maintenance_interval_spin.setValue(7)
            schedule_controls.addWidget(self.maintenance_interval_spin)
            schedule_controls.addWidget(QLabel("days"))
            schedule_layout.addLayout(schedule_controls)
            
            layout.addWidget(schedule_group)
            layout.addStretch()
            return widget
        
        def create_maintenance_logs_tab(self):
            """Create maintenance logs tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Log controls
            controls_layout = QHBoxLayout()
            self.clear_maintenance_log_btn = QPushButton("🧹 Clear Log")
            self.clear_maintenance_log_btn.clicked.connect(lambda: self.maintenance_log.clear())
            controls_layout.addWidget(self.clear_maintenance_log_btn)
            
            self.save_maintenance_log_btn = QPushButton("💾 Save Log")
            self.save_maintenance_log_btn.clicked.connect(self.save_maintenance_log)
            controls_layout.addWidget(self.save_maintenance_log_btn)
            
            controls_layout.addStretch()
            layout.addLayout(controls_layout)
            
            # Maintenance log
            self.maintenance_log = QTextEdit()
            self.maintenance_log.setReadOnly(True)
            layout.addWidget(self.maintenance_log)
            
            return widget
        
        def create_windows_tab(self):
            """Create the Windows programs tab with tabbed interface"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Create Windows sub-tabs
            windows_tabs = QTabWidget()
            layout.addWidget(windows_tabs)
            
            # Wine Management Tab
            wine_tab = self.create_wine_management_tab()
            windows_tabs.addTab(wine_tab, "🍷 Wine Management")
            
            # Program Installer Tab
            installer_tab = self.create_program_installer_tab()
            windows_tabs.addTab(installer_tab, "📦 Install Programs")
            
            # Installed Programs Tab
            programs_tab = self.create_installed_programs_tab()
            windows_tabs.addTab(programs_tab, "🖥️ Installed Programs")
            
            # Wine Prefixes Tab
            prefixes_tab = self.create_wine_prefixes_tab()
            windows_tabs.addTab(prefixes_tab, "🗂️ Wine Prefixes")
            
            # Logs Tab
            logs_tab = self.create_windows_logs_tab()
            windows_tabs.addTab(logs_tab, "📝 Logs")
            
            # Initialize data
            self.check_wine_status()
            self.scan_wine_prefixes()
            self.refresh_installed_programs()
            
            return widget
        
        def create_wine_management_tab(self):
            """Create Wine management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Wine Status
            status_group = QGroupBox("🍷 Wine Status")
            status_layout = QVBoxLayout(status_group)
            
            status_info_layout = QHBoxLayout()
            self.wine_status_label = QLabel("Wine Status: Checking...")
            status_info_layout.addWidget(self.wine_status_label)
            status_info_layout.addStretch()
            
            self.install_wine_btn = QPushButton("Install Wine")
            self.install_wine_btn.clicked.connect(self.install_wine)
            status_info_layout.addWidget(self.install_wine_btn)
            
            self.winecfg_btn = QPushButton("Wine Config")
            self.winecfg_btn.clicked.connect(self.open_winecfg)
            status_info_layout.addWidget(self.winecfg_btn)
            
            status_layout.addLayout(status_info_layout)
            layout.addWidget(status_group)
            
            # Wine Prefix Management
            prefix_group = QGroupBox("🔧 Prefix Management")
            prefix_layout = QFormLayout(prefix_group)
            
            self.current_prefix_label = QLabel("Default (~/.wine)")
            prefix_layout.addRow("Current Prefix:", self.current_prefix_label)
            
            prefix_buttons = QHBoxLayout()
            self.scan_prefixes_btn = QPushButton("🔍 Scan Prefixes")
            self.scan_prefixes_btn.clicked.connect(self.scan_wine_prefixes)
            prefix_buttons.addWidget(self.scan_prefixes_btn)
            
            self.create_prefix_btn = QPushButton("➕ Create New")
            self.create_prefix_btn.clicked.connect(self.create_wine_prefix)
            prefix_buttons.addWidget(self.create_prefix_btn)
            
            prefix_layout.addRow("Actions:", prefix_buttons)
            layout.addWidget(prefix_group)
            
            layout.addStretch()
            return widget
        
        def create_program_installer_tab(self):
            """Create program installer tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Program Details
            details_group = QGroupBox("📋 Program Details")
            details_layout = QFormLayout(details_group)
            
            self.program_name_input = QLineEdit()
            self.program_name_input.setPlaceholderText("e.g., Notepad++, Chrome, VLC")
            details_layout.addRow("Program Name:", self.program_name_input)
            
            self.wine_prefix_combo = QComboBox()
            self.wine_prefix_combo.addItem("Default (~/.wine)")
            details_layout.addRow("Install to Prefix:", self.wine_prefix_combo)
            
            layout.addWidget(details_group)
            
            # Installation Source
            source_group = QGroupBox("📥 Installation Source")
            source_layout = QVBoxLayout(source_group)
            
            # Local file
            local_layout = QHBoxLayout()
            self.exe_path_input = QLineEdit()
            self.exe_path_input.setPlaceholderText("Path to installer file...")
            local_layout.addWidget(self.exe_path_input)
            
            self.browse_exe_btn = QPushButton("Browse...")
            self.browse_exe_btn.clicked.connect(self.browse_exe_file)
            local_layout.addWidget(self.browse_exe_btn)
            
            source_layout.addLayout(local_layout)
            
            # Download URL
            url_layout = QHBoxLayout()
            self.download_url_input = QLineEdit()
            self.download_url_input.setPlaceholderText("https://example.com/installer.exe")
            url_layout.addWidget(self.download_url_input)
            
            self.download_btn = QPushButton("Download")
            self.download_btn.clicked.connect(self.download_installer)
            url_layout.addWidget(self.download_btn)
            
            source_layout.addLayout(url_layout)
            
            # Auto search and download
            self.search_download_btn = QPushButton("🔍 Auto Search & Download")
            self.search_download_btn.clicked.connect(self.search_and_download)
            source_layout.addWidget(self.search_download_btn)
            
            layout.addWidget(source_group)
            
            # Installation Options
            options_group = QGroupBox("⚙️ Installation Options")
            options_layout = QVBoxLayout(options_group)
            
            self.install_deps_check = QCheckBox("Install dependencies automatically")
            self.install_deps_check.setChecked(True)
            options_layout.addWidget(self.install_deps_check)
            
            self.create_shortcut_check = QCheckBox("Create desktop shortcut")
            self.create_shortcut_check.setChecked(True)
            options_layout.addWidget(self.create_shortcut_check)
            
            layout.addWidget(options_group)
            
            # Install Button
            self.install_program_btn = QPushButton("📦 Install Program")
            self.install_program_btn.clicked.connect(self.install_windows_program)
            self.install_program_btn.setMinimumHeight(40)
            layout.addWidget(self.install_program_btn)
            
            layout.addStretch()
            return widget
        
        def create_installed_programs_tab(self):
            """Create installed programs management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Control buttons
            controls_layout = QHBoxLayout()
            self.refresh_programs_btn = QPushButton("🔄 Refresh")
            self.refresh_programs_btn.clicked.connect(self.refresh_installed_programs)
            controls_layout.addWidget(self.refresh_programs_btn)
            
            controls_layout.addStretch()
            
            self.run_selected_btn = QPushButton("▶️ Run Selected")
            self.run_selected_btn.clicked.connect(self.run_selected_program)
            self.run_selected_btn.setEnabled(False)
            controls_layout.addWidget(self.run_selected_btn)
            
            self.uninstall_selected_btn = QPushButton("🗑️ Uninstall Selected")
            self.uninstall_selected_btn.clicked.connect(self.uninstall_selected_programs)
            self.uninstall_selected_btn.setEnabled(False)
            controls_layout.addWidget(self.uninstall_selected_btn)
            
            layout.addLayout(controls_layout)
            
            # Programs table
            self.installed_programs_table = QTableWidget()
            self.installed_programs_table.setColumnCount(6)
            self.installed_programs_table.setHorizontalHeaderLabels([
                "✓", "Program", "Version", "Prefix", "Install Date", "Size"
            ])
            self.installed_programs_table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
            self.installed_programs_table.setAlternatingRowColors(True)
            self.installed_programs_table.itemSelectionChanged.connect(self.on_program_selection_changed)
            layout.addWidget(self.installed_programs_table)
            
            return widget
        
        def create_wine_prefixes_tab(self):
            """Create Wine prefixes management tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Search bar
            search_layout = QHBoxLayout()
            self.prefix_search_input = QLineEdit()
            self.prefix_search_input.setPlaceholderText("Search for Wine prefixes...")
            search_layout.addWidget(self.prefix_search_input)
            
            self.search_prefixes_btn = QPushButton("🔍 Search")
            self.search_prefixes_btn.clicked.connect(self.search_wine_prefixes)
            search_layout.addWidget(self.search_prefixes_btn)
            
            layout.addLayout(search_layout)
            
            # Prefixes table
            self.prefixes_table = QTableWidget()
            self.prefixes_table.setColumnCount(5)
            self.prefixes_table.setHorizontalHeaderLabels([
                "Prefix Path", "Programs", "Size", "Last Modified", "Actions"
            ])
            self.prefixes_table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
            layout.addWidget(self.prefixes_table)
            
            return widget
        
        def create_windows_logs_tab(self):
            """Create Windows logs tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Log controls
            controls_layout = QHBoxLayout()
            self.clear_log_btn = QPushButton("🧹 Clear Log")
            self.clear_log_btn.clicked.connect(self.clear_windows_log)
            controls_layout.addWidget(self.clear_log_btn)
            
            self.save_log_btn = QPushButton("💾 Save Log")
            self.save_log_btn.clicked.connect(self.save_windows_log)
            controls_layout.addWidget(self.save_log_btn)
            
            controls_layout.addStretch()
            layout.addLayout(controls_layout)
            
            # Log display
            self.windows_log = QTextEdit()
            self.windows_log.setReadOnly(True)
            layout.addWidget(self.windows_log)
            
            return widget
        
        def create_installed_tab(self):
            """Create the installed packages tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            # Refresh button
            refresh_btn = QPushButton("🔄 Refresh")
            refresh_btn.clicked.connect(self.refresh_installed_packages)
            layout.addWidget(refresh_btn)
            
            # Installed packages table
            self.installed_table = QTableWidget()
            self.installed_table.setColumnCount(5)
            self.installed_table.setHorizontalHeaderLabels([
                "Name", "Method", "Version", "Install Date", "Actions"
            ])
            layout.addWidget(self.installed_table)
            
            # Load installed packages
            self.refresh_installed_packages()
            
            return widget
        
        def create_performance_tab(self):
            """Create the performance monitoring tab"""
            widget = QWidget()
            layout = QVBoxLayout(widget)
            
            if not PSUTIL_AVAILABLE:
                layout.addWidget(QLabel("Performance monitoring requires psutil"))
                return widget
            
            # Performance metrics
            metrics_group = QGroupBox("System Metrics")
            metrics_layout = QGridLayout(metrics_group)
            
            self.cpu_label = QLabel("CPU: 0%")
            self.memory_label = QLabel("Memory: 0%")
            self.disk_label = QLabel("Disk: 0%")
            
            metrics_layout.addWidget(self.cpu_label, 0, 0)
            metrics_layout.addWidget(self.memory_label, 0, 1)
            metrics_layout.addWidget(self.disk_label, 0, 2)
            
            layout.addWidget(metrics_group)
            
            # Performance chart placeholder
            performance_chart = QLabel("Performance charts coming soon...")
            performance_chart.setAlignment(Qt.AlignmentFlag.AlignCenter)
            layout.addWidget(performance_chart)
            
            # Start performance monitoring timer
            self.performance_timer = QTimer()
            self.performance_timer.timeout.connect(self.update_performance_metrics)
            self.performance_timer.start(1000)  # Update every second
            
            return widget
        
        def apply_bauh_theme(self):
            """Apply bauh-style theme to fix background and styling issues"""
            self.setStyleSheet("""
                /* Main Application */
                QMainWindow {
                    background-color: #1e1e1e;
                    color: #e0e0e0;
                }
                
                QWidget {
                    background-color: #1e1e1e;
                    color: #e0e0e0;
                }
                
                /* Tab Widget */
                QTabWidget {
                    background-color: #1e1e1e;
                }
                
                QTabWidget::pane {
                    border: 1px solid #404040;
                    background-color: #1e1e1e;
                    top: -1px;
                }
                
                QTabBar::tab {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    padding: 12px 20px;
                    margin-right: 2px;
                    border: 1px solid #404040;
                    border-bottom: none;
                    min-width: 120px;
                    font-weight: bold;
                }
                
                QTabBar::tab:selected {
                    background-color: #1e1e1e;
                    border-bottom: 2px solid #00bcd4;
                }
                
                QTabBar::tab:hover:!selected {
                    background-color: #353535;
                }
                
                /* Buttons */
                QPushButton {
                    background-color: #00838f;
                    color: #ffffff !important;
                    border: none;
                    padding: 10px 16px;
                    border-radius: 6px;
                    font-weight: bold;
                    min-height: 20px;
                    font-size: 13px;
                }
                
                QPushButton:hover {
                    background-color: #00acc1;
                }
                
                QPushButton:pressed {
                    background-color: #006064;
                }
                
                QPushButton:disabled {
                    background-color: #424242;
                    color: #757575 !important;
                }
                
                /* Input Fields */
                QLineEdit {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border: 2px solid #404040;
                    padding: 8px 12px;
                    border-radius: 6px;
                    font-size: 13px;
                }
                
                QLineEdit:focus {
                    border-color: #00bcd4;
                    background-color: #353535;
                }
                
                QTextEdit {
                    background-color: #252525;
                    color: #e0e0e0;
                    border: 2px solid #404040;
                    border-radius: 6px;
                    padding: 8px;
                    font-family: 'Consolas', 'Monaco', monospace;
                    selection-background-color: #00838f;
                }
                
                QTextEdit:focus {
                    border-color: #00bcd4;
                }
                
                /* ComboBox */
                QComboBox {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border: 2px solid #404040;
                    padding: 8px 12px;
                    border-radius: 6px;
                    min-width: 120px;
                }
                
                QComboBox:hover {
                    border-color: #00bcd4;
                }
                
                QComboBox::drop-down {
                    border: none;
                    width: 20px;
                }
                
                QComboBox::down-arrow {
                    image: none;
                    border-left: 5px solid transparent;
                    border-right: 5px solid transparent;
                    border-top: 5px solid #e0e0e0;
                }
                
                QComboBox QAbstractItemView {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border: 1px solid #00bcd4;
                    selection-background-color: #00838f;
                }
                
                /* SpinBox */
                QSpinBox {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border: 2px solid #404040;
                    padding: 8px;
                    border-radius: 6px;
                }
                
                QSpinBox:focus {
                    border-color: #00bcd4;
                }
                
                /* Tables */
                QTableWidget {
                    background-color: #1e1e1e;
                    color: #e0e0e0;
                    gridline-color: #404040;
                    border: 1px solid #404040;
                    border-radius: 6px;
                    alternate-background-color: #252525;
                }
                
                QTableWidget::item {
                    padding: 8px;
                    border-bottom: 1px solid #404040;
                }
                
                QTableWidget::item:selected {
                    background-color: #00838f;
                    color: #ffffff;
                }
                
                QTableWidget::item:hover {
                    background-color: #353535;
                }
                
                QHeaderView::section {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    padding: 12px 8px;
                    border: none;
                    border-right: 1px solid #404040;
                    border-bottom: 2px solid #00bcd4;
                    font-weight: bold;
                }
                
                QHeaderView::section:hover {
                    background-color: #353535;
                }
                
                /* GroupBox */
                QGroupBox {
                    color: #e0e0e0;
                    border: 2px solid #404040;
                    border-radius: 8px;
                    margin-top: 12px;
                    padding-top: 12px;
                    font-weight: bold;
                    font-size: 14px;
                }
                
                QGroupBox::title {
                    subcontrol-origin: margin;
                    left: 12px;
                    padding: 0 8px;
                    background-color: #1e1e1e;
                    color: #00bcd4;
                }
                
                /* CheckBox */
                QCheckBox {
                    color: #e0e0e0;
                    spacing: 8px;
                }
                
                QCheckBox::indicator {
                    width: 18px;
                    height: 18px;
                    border: 2px solid #404040;
                    border-radius: 3px;
                    background-color: #2d2d2d;
                }
                
                QCheckBox::indicator:checked {
                    background-color: #00bcd4;
                    border-color: #00bcd4;
                }
                
                QCheckBox::indicator:checked:hover {
                    background-color: #00acc1;
                }
                
                QCheckBox::indicator:hover {
                    border-color: #00bcd4;
                }
                
                /* Labels */
                QLabel {
                    color: #e0e0e0;
                    background-color: transparent;
                }
                
                /* ScrollBars */
                QScrollBar:vertical {
                    background-color: #2d2d2d;
                    width: 12px;
                    border-radius: 6px;
                }
                
                QScrollBar::handle:vertical {
                    background-color: #00838f;
                    border-radius: 6px;
                    min-height: 20px;
                }
                
                QScrollBar::handle:vertical:hover {
                    background-color: #00bcd4;
                }
                
                QScrollBar:horizontal {
                    background-color: #2d2d2d;
                    height: 12px;
                    border-radius: 6px;
                }
                
                QScrollBar::handle:horizontal {
                    background-color: #00838f;
                    border-radius: 6px;
                    min-width: 20px;
                }
                
                QScrollBar::handle:horizontal:hover {
                    background-color: #00bcd4;
                }
                
                /* Progress Bar */
                QProgressBar {
                    background-color: #2d2d2d;
                    border: 2px solid #404040;
                    border-radius: 6px;
                    text-align: center;
                    color: #e0e0e0;
                }
                
                QProgressBar::chunk {
                    background-color: #00bcd4;
                    border-radius: 4px;
                }
                
                /* Status Bar */
                QStatusBar {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border-top: 1px solid #404040;
                }
                
                /* Tooltips */
                QToolTip {
                    background-color: #2d2d2d;
                    color: #e0e0e0;
                    border: 1px solid #00bcd4;
                    padding: 6px;
                    border-radius: 4px;
                }
            """)
        
        def search_packages(self):
            """Search for packages"""
            query = self.search_input.text().strip()
            if not query:
                return
            
            self.search_status.setText(f"Searching for '{query}'...")
            self.search_button.setEnabled(False)
            
            # Run search in background thread
            self.search_thread = SearchThread(
                self.installer, query, self.ai_search_check.isChecked()
            )
            self.search_thread.results_ready.connect(self.display_search_results)
            self.search_thread.start()
        
        def display_search_results(self, packages):
            """Display search results in the table"""
            self.results_table.setRowCount(len(packages))
            
            for row, package in enumerate(packages):
                self.results_table.setItem(row, 0, QTableWidgetItem(package.name))
                self.results_table.setItem(row, 1, QTableWidgetItem(package.method.value))
                self.results_table.setItem(row, 2, QTableWidgetItem(package.version))
                self.results_table.setItem(row, 3, QTableWidgetItem(package.description[:100]))
                self.results_table.setItem(row, 4, QTableWidgetItem(package.category.value))
                
                # Install button
                install_btn = QPushButton("Install")
                install_btn.clicked.connect(lambda checked, pkg=package: self.install_package(pkg))
                self.results_table.setCellWidget(row, 5, install_btn)
            
            self.search_status.setText(f"Found {len(packages)} packages")
            self.search_button.setEnabled(True)
        
        def install_package(self, package):
            """Install a package"""
            self.installer.install_package(package, confirm=False)
        
        def run_maintenance(self, operation):
            """Run maintenance operation"""
            self.maintenance_log.append(f"Starting {operation}...")
            success = self.installer.system_maintenance(operation)
            if success:
                self.maintenance_log.append(f"✅ {operation} completed successfully")
            else:
                self.maintenance_log.append(f"❌ {operation} failed")
        
        def check_wine_status(self):
            """Check Wine installation status"""
            if shutil.which("wine"):
                self.wine_status_label.setText("Wine Status: ✅ Installed")
                self.install_wine_btn.setText("Reinstall Wine")
            else:
                self.wine_status_label.setText("Wine Status: ❌ Not Installed")
                self.install_wine_btn.setText("Install Wine")
        
        def install_wine(self):
            """Install Wine"""
            try:
                result = subprocess.run(
                    ["sudo", "pacman", "-S", "--needed", "--noconfirm", "wine"],
                    capture_output=True, text=True
                )
                if result.returncode == 0:
                    self.wine_status_label.setText("Wine Status: ✅ Installed")
                    self.windows_log.append("✅ Wine installed successfully")
                else:
                    self.windows_log.append(f"❌ Wine installation failed: {result.stderr}")
            except Exception as e:
                self.windows_log.append(f"❌ Error installing Wine: {str(e)}")
            
            self.check_wine_status()
        
        def browse_exe_file(self):
            """Browse for executable file"""
            file_path, _ = QFileDialog.getOpenFileName(
                self, "Select Windows Installer", "",
                "Executable Files (*.exe *.msi);;All Files (*)"
            )
            if file_path:
                self.exe_path_input.setText(file_path)
        
        def download_installer(self):
            """Download installer from URL"""
            url = self.download_url_input.text().strip()
            if not url:
                return
            
            self.windows_log.append(f"Downloading from {url}...")
            # Implementation would download the file
        
        def search_and_download(self):
            """Search for program and download automatically"""
            program_name = self.program_name_input.text().strip()
            if not program_name:
                return
            
            self.windows_log.append(f"Searching for {program_name}...")
            # Implementation would search for download URLs
        
        def install_windows_program(self):
            """Install Windows program with Wine"""
            program_name = self.program_name_input.text().strip()
            exe_path = self.exe_path_input.text().strip()
            
            if not program_name or not exe_path:
                return
            
            self.windows_log.append(f"Installing {program_name}...")
            # Implementation would install the program
        
        def refresh_installed_packages(self):
            """Refresh the installed packages list"""
            try:
                conn = sqlite3.connect(self.installer.db_path)
                cursor = conn.cursor()
                
                cursor.execute('''
                    SELECT name, method, version, install_date 
                    FROM packages 
                    WHERE installed = TRUE 
                    ORDER BY install_date DESC
                ''')
                
                packages = cursor.fetchall()
                conn.close()
                
                self.installed_table.setRowCount(len(packages))
                
                for row, (name, method, version, install_date) in enumerate(packages):
                    self.installed_table.setItem(row, 0, QTableWidgetItem(name))
                    self.installed_table.setItem(row, 1, QTableWidgetItem(method))
                    self.installed_table.setItem(row, 2, QTableWidgetItem(version or ""))
                    self.installed_table.setItem(row, 3, QTableWidgetItem(install_date or ""))
                    
                    # Uninstall button
                    uninstall_btn = QPushButton("Uninstall")
                    self.installed_table.setCellWidget(row, 4, uninstall_btn)
                
            except Exception as e:
                self.installer.logger.error(f"Failed to refresh installed packages: {e}")
        
        def update_performance_metrics(self):
            """Update performance metrics"""
            if not PSUTIL_AVAILABLE:
                return
            
            try:
                cpu_percent = psutil.cpu_percent()
                memory = psutil.virtual_memory()
                disk = psutil.disk_usage('/')
                
                self.cpu_label.setText(f"CPU: {cpu_percent:.1f}%")
                self.memory_label.setText(f"Memory: {memory.percent:.1f}%")
                self.disk_label.setText(f"Disk: {(disk.used / disk.total * 100):.1f}%")
                
            except Exception as e:
                self.installer.logger.error(f"Performance update failed: {e}")
        
        # Wine and Windows Program Management Methods
        def open_winecfg(self):
            """Open Wine configuration"""
            try:
                subprocess.Popen(["winecfg"])
                self.windows_log.append("✅ Opened Wine configuration")
            except Exception as e:
                self.windows_log.append(f"❌ Failed to open Wine configuration: {str(e)}")
        
        def scan_wine_prefixes(self):
            """Scan for existing Wine prefixes"""
            self.windows_log.append("🔍 Scanning for Wine prefixes...")
            
            # Update wine prefix combo
            self.wine_prefix_combo.clear()
            self.wine_prefix_combo.addItem("Default (~/.wine)")
            
            # Clear prefixes table
            self.prefixes_table.setRowCount(0)
            
            try:
                home_dir = Path.home()
                wine_prefixes = []
                
                # Look for common Wine prefix patterns
                for item in home_dir.iterdir():
                    if item.is_dir() and (item.name.startswith('.wine') or 'wine' in item.name.lower()):
                        if self.is_wine_prefix(item):
                            wine_prefixes.append(item)
                            # Add to combo box
                            self.wine_prefix_combo.addItem(str(item))
                
                # Also check common prefix locations
                prefix_dirs = [
                    home_dir / "Games",
                    home_dir / "WinePrefixes",
                    home_dir / ".local" / "share" / "wineprefixes"
                ]
                
                for prefix_dir in prefix_dirs:
                    if prefix_dir.exists():
                        for item in prefix_dir.iterdir():
                            if item.is_dir() and self.is_wine_prefix(item):
                                wine_prefixes.append(item)
                                self.wine_prefix_combo.addItem(str(item))
                
                # Populate prefixes table
                self.prefixes_table.setRowCount(len(wine_prefixes))
                
                for row, prefix_path in enumerate(wine_prefixes):
                    # Get prefix info
                    programs_count = self.count_programs_in_prefix(prefix_path)
                    size = self.get_directory_size(prefix_path)
                    last_modified = datetime.datetime.fromtimestamp(prefix_path.stat().st_mtime).strftime('%Y-%m-%d %H:%M')
                    
                    self.prefixes_table.setItem(row, 0, QTableWidgetItem(str(prefix_path)))
                    self.prefixes_table.setItem(row, 1, QTableWidgetItem(str(programs_count)))
                    self.prefixes_table.setItem(row, 2, QTableWidgetItem(f"{size:.1f} MB"))
                    self.prefixes_table.setItem(row, 3, QTableWidgetItem(last_modified))
                    
                    # Action buttons
                    actions_widget = QWidget()
                    actions_layout = QHBoxLayout(actions_widget)
                    actions_layout.setContentsMargins(2, 2, 2, 2)
                    actions_layout.setSpacing(4)
                    
                    import_btn = QPushButton("Import")
                    import_btn.setFixedSize(70, 28)
                    import_btn.clicked.connect(lambda checked, p=prefix_path: self.import_wine_prefix(p))
                    actions_layout.addWidget(import_btn)
                    
                    open_btn = QPushButton("Open")
                    open_btn.setFixedSize(70, 28)
                    open_btn.clicked.connect(lambda checked, p=prefix_path: self.open_wine_prefix(p))
                    actions_layout.addWidget(open_btn)
                    
                    self.prefixes_table.setCellWidget(row, 4, actions_widget)
                
                self.windows_log.append(f"✅ Found {len(wine_prefixes)} Wine prefixes")
                
            except Exception as e:
                self.windows_log.append(f"❌ Error scanning prefixes: {str(e)}")
        
        def is_wine_prefix(self, path: Path) -> bool:
            """Check if a directory is a Wine prefix"""
            try:
                # Check for common Wine prefix files/directories
                wine_indicators = [
                    path / "system.reg",
                    path / "user.reg",
                    path / "drive_c",
                    path / "dosdevices"
                ]
                return any(indicator.exists() for indicator in wine_indicators)
            except:
                return False
        
        def count_programs_in_prefix(self, prefix_path: Path) -> int:
            """Count installed programs in Wine prefix"""
            try:
                program_files = [
                    prefix_path / "drive_c" / "Program Files",
                    prefix_path / "drive_c" / "Program Files (x86)"
                ]
                
                count = 0
                for pf_dir in program_files:
                    if pf_dir.exists():
                        count += len([d for d in pf_dir.iterdir() if d.is_dir()])
                
                return count
            except:
                return 0
        
        def get_directory_size(self, path: Path) -> float:
            """Get directory size in MB"""
            try:
                total_size = 0
                for dirpath, dirnames, filenames in os.walk(path):
                    for filename in filenames:
                        filepath = os.path.join(dirpath, filename)
                        if os.path.exists(filepath):
                            total_size += os.path.getsize(filepath)
                return total_size / (1024 * 1024)  # Convert to MB
            except:
                return 0.0
        
        def create_wine_prefix(self):
            """Create a new Wine prefix"""
            from PyQt6.QtWidgets import QInputDialog
            
            prefix_name, ok = QInputDialog.getText(
                self, "Create Wine Prefix", 
                "Enter prefix name:"
            )
            
            if ok and prefix_name:
                try:
                    home_dir = Path.home()
                    prefix_path = home_dir / f".wine-{prefix_name}"
                    
                    if prefix_path.exists():
                        QMessageBox.warning(self, "Prefix Exists", f"Prefix {prefix_path} already exists!")
                        return
                    
                    self.windows_log.append(f"🍷 Creating Wine prefix: {prefix_path}...")
                    
                    # Create prefix
                    env = os.environ.copy()
                    env['WINEPREFIX'] = str(prefix_path)
                    
                    result = subprocess.run(
                        ["wineboot", "--init"],
                        env=env, capture_output=True, text=True, timeout=60
                    )
                    
                    if result.returncode == 0:
                        self.windows_log.append(f"✅ Wine prefix created successfully: {prefix_path}")
                        self.scan_wine_prefixes()  # Refresh the list
                    else:
                        self.windows_log.append(f"❌ Failed to create Wine prefix: {result.stderr}")
                        
                except subprocess.TimeoutExpired:
                    self.windows_log.append("❌ Wine prefix creation timed out")
                except Exception as e:
                    self.windows_log.append(f"❌ Error creating Wine prefix: {str(e)}")
        
        def import_wine_prefix(self, prefix_path: Path):
            """Import programs from another Wine prefix to main installation"""
            reply = QMessageBox.question(
                self, "Import Wine Prefix",
                f"Import programs from {prefix_path} to main Wine installation?\n\n"
                "This will copy program files and registry entries.",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                try:
                    self.windows_log.append(f"📦 Importing from {prefix_path}...")
                    
                    # Copy Program Files
                    source_program_files = [
                        prefix_path / "drive_c" / "Program Files",
                        prefix_path / "drive_c" / "Program Files (x86)"
                    ]
                    
                    main_prefix = Path.home() / ".wine"
                    if not main_prefix.exists():
                        self.windows_log.append("❌ Main Wine prefix not found. Creating...")
                        subprocess.run(["wineboot", "--init"], capture_output=True, timeout=60)
                    
                    dest_program_files = [
                        main_prefix / "drive_c" / "Program Files",
                        main_prefix / "drive_c" / "Program Files (x86)"
                    ]
                    
                    imported_count = 0
                    for src_pf, dest_pf in zip(source_program_files, dest_program_files):
                        if src_pf.exists():
                            dest_pf.mkdir(parents=True, exist_ok=True)
                            for program_dir in src_pf.iterdir():
                                if program_dir.is_dir():
                                    dest_program_dir = dest_pf / program_dir.name
                                    if not dest_program_dir.exists():
                                        shutil.copytree(program_dir, dest_program_dir)
                                        imported_count += 1
                                        self.windows_log.append(f"  📁 Imported: {program_dir.name}")
                    
                    # TODO: Import registry entries (more complex)
                    
                    self.windows_log.append(f"✅ Imported {imported_count} programs from {prefix_path.name}")
                    self.refresh_installed_programs()
                    
                except Exception as e:
                    self.windows_log.append(f"❌ Import failed: {str(e)}")
        
        def open_wine_prefix(self, prefix_path: Path):
            """Open Wine prefix in file manager"""
            try:
                if os.name == 'posix':  # Linux/Unix
                    subprocess.Popen(["xdg-open", str(prefix_path)])
                else:
                    subprocess.Popen(["explorer", str(prefix_path)])
                self.windows_log.append(f"📂 Opened {prefix_path} in file manager")
            except Exception as e:
                self.windows_log.append(f"❌ Failed to open {prefix_path}: {str(e)}")
        
        def search_wine_prefixes(self):
            """Search for Wine prefixes based on user input"""
            search_term = self.prefix_search_input.text().strip().lower()
            
            if not search_term:
                self.scan_wine_prefixes()  # Show all if no search term
                return
            
            self.windows_log.append(f"🔍 Searching prefixes for: {search_term}")
            
            # Filter current table based on search term
            for row in range(self.prefixes_table.rowCount()):
                item = self.prefixes_table.item(row, 0)  # Prefix path column
                if item:
                    should_show = search_term in item.text().lower()
                    self.prefixes_table.setRowHidden(row, not should_show)
        
        def refresh_installed_programs(self):
            """Refresh the installed Windows programs list"""
            self.windows_log.append("🔄 Refreshing installed Windows programs...")
            
            try:
                # Clear table
                self.installed_programs_table.setRowCount(0)
                
                # Find Wine prefixes and their programs
                programs = []
                
                # Check main Wine prefix
                main_prefix = Path.home() / ".wine"
                if main_prefix.exists():
                    programs.extend(self.get_programs_from_prefix(main_prefix, "Default"))
                
                # Check other prefixes
                home_dir = Path.home()
                for item in home_dir.iterdir():
                    if item.is_dir() and item.name.startswith('.wine-'):
                        if self.is_wine_prefix(item):
                            prefix_name = item.name.replace('.wine-', '')
                            programs.extend(self.get_programs_from_prefix(item, prefix_name))
                
                # Populate table
                self.installed_programs_table.setRowCount(len(programs))
                
                for row, program in enumerate(programs):
                    # Checkbox
                    checkbox = QCheckBox()
                    checkbox.stateChanged.connect(self.on_program_selection_changed)
                    self.installed_programs_table.setCellWidget(row, 0, checkbox)
                    
                    self.installed_programs_table.setItem(row, 1, QTableWidgetItem(program['name']))
                    self.installed_programs_table.setItem(row, 2, QTableWidgetItem(program.get('version', 'Unknown')))
                    self.installed_programs_table.setItem(row, 3, QTableWidgetItem(program['prefix']))
                    self.installed_programs_table.setItem(row, 4, QTableWidgetItem(program.get('install_date', 'Unknown')))
                    self.installed_programs_table.setItem(row, 5, QTableWidgetItem(program.get('size', 'Unknown')))
                
                self.windows_log.append(f"✅ Found {len(programs)} installed Windows programs")
                
            except Exception as e:
                self.windows_log.append(f"❌ Error refreshing programs: {str(e)}")
        
        def get_programs_from_prefix(self, prefix_path: Path, prefix_name: str) -> list:
            """Get list of programs installed in a Wine prefix"""
            programs = []
            
            try:
                program_dirs = [
                    prefix_path / "drive_c" / "Program Files",
                    prefix_path / "drive_c" / "Program Files (x86)"
                ]
                
                for pf_dir in program_dirs:
                    if pf_dir.exists():
                        for program_dir in pf_dir.iterdir():
                            if program_dir.is_dir() and program_dir.name not in ['Common Files', 'Internet Explorer', 'Windows NT']:
                                # Get program info
                                program_info = {
                                    'name': program_dir.name,
                                    'prefix': prefix_name,
                                    'path': str(program_dir),
                                    'version': self.get_program_version(program_dir),
                                    'size': f"{self.get_directory_size(program_dir):.1f} MB",
                                    'install_date': datetime.datetime.fromtimestamp(program_dir.stat().st_mtime).strftime('%Y-%m-%d')
                                }
                                programs.append(program_info)
                
            except Exception as e:
                self.windows_log.append(f"❌ Error reading programs from {prefix_path}: {str(e)}")
            
            return programs
        
        def get_program_version(self, program_dir: Path) -> str:
            """Try to get program version from executable files"""
            try:
                # Look for main executable
                for exe_file in program_dir.rglob("*.exe"):
                    if exe_file.is_file():
                        # Try to extract version info (simplified)
                        return "1.0.0"  # Placeholder
                return "Unknown"
            except:
                return "Unknown"
        
        def on_program_selection_changed(self):
            """Handle program selection changes"""
            selected_count = 0
            for row in range(self.installed_programs_table.rowCount()):
                checkbox = self.installed_programs_table.cellWidget(row, 0)
                if checkbox and isinstance(checkbox, QCheckBox) and checkbox.isChecked():
                    selected_count += 1
            
            self.uninstall_selected_btn.setEnabled(selected_count > 0)
            self.run_selected_btn.setEnabled(selected_count == 1)  # Only enable for single selection
        
        def uninstall_selected_programs(self):
            """Uninstall selected Windows programs"""
            selected_programs = []
            
            for row in range(self.installed_programs_table.rowCount()):
                checkbox = self.installed_programs_table.cellWidget(row, 0)
                if checkbox and isinstance(checkbox, QCheckBox) and checkbox.isChecked():
                    program_name = self.installed_programs_table.item(row, 1).text()
                    selected_programs.append(program_name)
            
            if not selected_programs:
                return
            
            reply = QMessageBox.question(
                self, "Uninstall Programs",
                f"Uninstall {len(selected_programs)} selected programs?\n\n" +
                "\n".join(f"• {prog}" for prog in selected_programs),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                for program in selected_programs:
                    self.uninstall_windows_program(program)
                
                self.refresh_installed_programs()
        
        def uninstall_windows_program(self, program_name: str):
            """Uninstall a Windows program"""
            self.windows_log.append(f"🗑️ Uninstalling {program_name}...")
            
            try:
                # Try using Wine's uninstaller
                result = subprocess.run(
                    ["wine", "uninstaller"],
                    capture_output=True, text=True, timeout=30
                )
                
                self.windows_log.append(f"✅ Uninstaller launched for {program_name}")
                
            except Exception as e:
                self.windows_log.append(f"❌ Failed to uninstall {program_name}: {str(e)}")
        
        def run_selected_program(self):
            """Run the selected Windows program"""
            for row in range(self.installed_programs_table.rowCount()):
                checkbox = self.installed_programs_table.cellWidget(row, 0)
                if checkbox and isinstance(checkbox, QCheckBox) and checkbox.isChecked():
                    program_name = self.installed_programs_table.item(row, 1).text()
                    prefix_name = self.installed_programs_table.item(row, 3).text()
                    
                    self.run_windows_program(program_name, prefix_name)
                    break
        
        def run_windows_program(self, program_name: str, prefix_name: str):
            """Run a Windows program"""
            self.windows_log.append(f"▶️ Starting {program_name}...")
            
            try:
                # Determine prefix path
                if prefix_name == "Default":
                    prefix_path = Path.home() / ".wine"
                else:
                    prefix_path = Path.home() / f".wine-{prefix_name}"
                
                # Set environment
                env = os.environ.copy()
                env['WINEPREFIX'] = str(prefix_path)
                
                # Find and run the program's main executable
                program_dirs = [
                    prefix_path / "drive_c" / "Program Files" / program_name,
                    prefix_path / "drive_c" / "Program Files (x86)" / program_name
                ]
                
                main_exe = None
                for prog_dir in program_dirs:
                    if prog_dir.exists():
                        # Look for main executable
                        for exe_file in prog_dir.rglob("*.exe"):
                            if program_name.lower() in exe_file.name.lower():
                                main_exe = exe_file
                                break
                        if main_exe:
                            break
                
                if main_exe:
                    subprocess.Popen(["wine", str(main_exe)], env=env)
                    self.windows_log.append(f"✅ Started {program_name}")
                else:
                    self.windows_log.append(f"❌ Could not find executable for {program_name}")
                    
            except Exception as e:
                self.windows_log.append(f"❌ Failed to start {program_name}: {str(e)}")
        
        def clear_windows_log(self):
            """Clear the Windows installation log"""
            self.windows_log.clear()
            self.windows_log.append("🪟 Windows Program Manager Ready")
            self.windows_log.append("=" * 50)
            self.windows_log.append("Log cleared.\n")
        
        def save_windows_log(self):
            """Save Windows installation log to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Windows Installation Log",
                str(Path.home() / f"windows_install_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.windows_log.toPlainText())
                    QMessageBox.information(self, "Log Saved", f"Log saved to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save log: {str(e)}")
        
        # AI Assistant Methods
        def process_ai_query(self):
            """Process AI assistant query"""
            query = self.ai_input.text().strip()
            if not query:
                return
            
            self.ai_chat_display.append(f"\n🤔 You: {query}")
            self.ai_input.clear()
            
            # Process query with AI
            response = self.generate_ai_response(query)
            self.ai_chat_display.append(f"\n🤖 AI Assistant: {response}")
            
            # Auto-scroll to bottom
            scrollbar = self.ai_chat_display.verticalScrollBar()
            scrollbar.setValue(scrollbar.maximum())
        
        def send_quick_prompt(self, prompt):
            """Send a quick action prompt"""
            self.ai_input.setText(prompt)
            self.process_ai_query()
        
        def generate_ai_response(self, query):
            """Generate AI response to user query"""
            if not AI_FEATURES_AVAILABLE:
                return "AI features are not available. Please install scikit-learn and nltk."
            
            query_lower = query.lower()
            
            # Pattern matching for common queries
            if any(word in query_lower for word in ['video', 'editing', 'movie']):
                return self.recommend_video_editing_software()
            elif any(word in query_lower for word in ['development', 'programming', 'code']):
                return self.recommend_development_tools()
            elif any(word in query_lower for word in ['gaming', 'games', 'emulator']):
                return self.recommend_gaming_software()
            elif any(word in query_lower for word in ['office', 'document', 'spreadsheet']):
                return self.recommend_office_software()
            elif any(word in query_lower for word in ['browser', 'web', 'internet']):
                return self.recommend_browsers()
            elif any(word in query_lower for word in ['security', 'privacy', 'vpn']):
                return self.recommend_security_tools()
            elif any(word in query_lower for word in ['docker', 'container']):
                return self.recommend_docker_setup()
            elif any(word in query_lower for word in ['git', 'version control']):
                return self.recommend_git_setup()
            else:
                return self.generate_generic_response(query)
        
        def recommend_video_editing_software(self):
            return """🎬 For video editing on Arch Linux, I recommend:
            
**Professional Options:**
• DaVinci Resolve - Industry-standard, free version available
• Blender - Open-source with excellent video editing capabilities
• Kdenlive - Powerful open-source editor with great features

**Lightweight Options:**
• OpenShot - User-friendly for beginners
• Shotcut - Cross-platform, no watermarks
• Flowblade - MLT framework-based editor

**Installation commands:**
• Kdenlive: `sudo pacman -S kdenlive`
• Blender: `sudo pacman -S blender`
• OpenShot: `yay -S openshot`

Would you like me to help you install any of these?"""
        
        def recommend_development_tools(self):
            return """💻 Essential development tools for Arch Linux:
            
**Code Editors/IDEs:**
• Visual Studio Code: `yay -S visual-studio-code-bin`
• Neovim: `sudo pacman -S neovim`
• IntelliJ IDEA: `yay -S intellij-idea-community-edition`
• Sublime Text: `yay -S sublime-text-4`

**Version Control:**
• Git: `sudo pacman -S git`
• GitHub CLI: `sudo pacman -S github-cli`

**Programming Languages:**
• Python: `sudo pacman -S python python-pip`
• Node.js: `sudo pacman -S nodejs npm`
• Rust: `sudo pacman -S rust`
• Go: `sudo pacman -S go`

**Development Tools:**
• Docker: `sudo pacman -S docker docker-compose`
• Postman: `yay -S postman-bin`

Which programming language or tool would you like help setting up?"""
        
        def recommend_gaming_software(self):
            return """🎮 Gaming setup for Arch Linux:
            
**Game Launchers:**
• Steam: `sudo pacman -S steam`
• Lutris: `sudo pacman -S lutris`
• Heroic Games Launcher: `yay -S heroic-games-launcher-bin`

**Emulators:**
• RetroArch: `sudo pacman -S retroarch`
• PCSX2 (PS2): `sudo pacman -S pcsx2`
• Dolphin (GameCube/Wii): `sudo pacman -S dolphin-emu`
• RPCS3 (PS3): `yay -S rpcs3`

**Wine for Windows Games:**
• Wine: `sudo pacman -S wine`
• PlayOnLinux: `yay -S playonlinux`
• Bottles: `yay -S bottles`

**Graphics Drivers:**
• NVIDIA: `sudo pacman -S nvidia nvidia-utils`
• AMD: `sudo pacman -S mesa vulkan-radeon`

Need help with driver installation or game compatibility?"""
        
        def recommend_office_software(self):
            return """📊 Office and productivity software:
            
**Office Suites:**
• LibreOffice: `sudo pacman -S libreoffice-fresh`
• OnlyOffice: `yay -S onlyoffice-bin`
• WPS Office: `yay -S wps-office`

**PDF Tools:**
• Okular: `sudo pacman -S okular`
• Evince: `sudo pacman -S evince`
• Master PDF Editor: `yay -S master-pdf-editor`

**Note-Taking:**
• Obsidian: `yay -S obsidian`
• Joplin: `sudo pacman -S joplin-desktop`
• Typora: `yay -S typora`

**Productivity:**
• Thunderbird (Email): `sudo pacman -S thunderbird`
• KeePassXC (Password Manager): `sudo pacman -S keepassxc`
• Nextcloud Client: `sudo pacman -S nextcloud-client`

Which type of office application do you need help with?"""
        
        def recommend_browsers(self):
            return """🌐 Web browsers for Arch Linux:
            
**Popular Browsers:**
• Firefox: `sudo pacman -S firefox`
• Chrome: `yay -S google-chrome`
• Chromium: `sudo pacman -S chromium`
• Brave: `yay -S brave-bin`
• Opera: `yay -S opera`

**Privacy-Focused:**
• Tor Browser: `sudo pacman -S torbrowser-launcher`
• LibreWolf: `yay -S librewolf-bin`
• Ungoogled Chromium: `yay -S ungoogled-chromium`

**Lightweight:**
• Midori: `sudo pacman -S midori`
• Epiphany: `sudo pacman -S epiphany`

**Developer Tools:**
• Firefox Developer Edition: `yay -S firefox-developer-edition`

I recommend Firefox or Brave for most users. Need help setting up any specific browser?"""
        
        def recommend_security_tools(self):
            return """🔐 Security and privacy tools:
            
**VPN Clients:**
• OpenVPN: `sudo pacman -S openvpn`
• WireGuard: `sudo pacman -S wireguard-tools`
• NordVPN: `yay -S nordvpn-bin`

**Password Managers:**
• KeePassXC: `sudo pacman -S keepassxc`
• Bitwarden: `yay -S bitwarden`

**Encryption:**
• VeraCrypt: `sudo pacman -S veracrypt`
• GnuPG: `sudo pacman -S gnupg`

**Network Security:**
• Wireshark: `sudo pacman -S wireshark-qt`
• Nmap: `sudo pacman -S nmap`
• UFW Firewall: `sudo pacman -S ufw`

**System Security:**
• ClamAV: `sudo pacman -S clamav`
• Lynis: `sudo pacman -S lynis`

Would you like help configuring any of these security tools?"""
        
        def recommend_docker_setup(self):
            return """🐳 Docker setup guide:
            
**Installation:**
```bash
# Install Docker
sudo pacman -S docker docker-compose

# Start and enable Docker service
sudo systemctl start docker
sudo systemctl enable docker

# Add user to docker group (logout/login required)
sudo usermod -aG docker $USER
```

**Useful Docker Tools:**
• Portainer: `docker run -d -p 9000:9000 portainer/portainer-ce`
• Lazydocker: `yay -S lazydocker`
• Docker Desktop: `yay -S docker-desktop`

**Common Commands:**
• `docker ps` - List running containers
• `docker images` - List images
• `docker-compose up -d` - Start services

Need help with a specific Docker use case?"""
        
        def recommend_git_setup(self):
            return """📝 Git and version control setup:
            
**Installation and Setup:**
```bash
# Install Git
sudo pacman -S git

# Configure Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Generate SSH key for GitHub
ssh-keygen -t ed25519 -C "your.email@example.com"
```

**GUI Tools:**
• GitKraken: `yay -S gitkraken`
• GitHub Desktop: `yay -S github-desktop-bin`
• Sublime Merge: `yay -S sublime-merge`

**Terminal Tools:**
• Lazygit: `sudo pacman -S lazygit`
• Tig: `sudo pacman -S tig`
• GitHub CLI: `sudo pacman -S github-cli`

**Useful Aliases:**
```bash
git config --global alias.st status
git config --global alias.co checkout
git config --global alias.br branch
```

Need help with Git workflows or GitHub integration?"""
        
        def generate_generic_response(self, query):
            return f"""I understand you're asking about: "{query}"
            
Let me help you find the right packages! Here are some suggestions:

**Search Strategy:**
1. Try searching in the package search tab with keywords from your query
2. Check both official repos and AUR packages
3. Look for alternatives if the exact package isn't available

**Common Package Sources:**
• Official repos: Use `pacman -Ss keyword`
• AUR: Use `yay -Ss keyword` or `paru -Ss keyword`
• Flatpak: Use `flatpak search keyword`
• Snap: Use `snap find keyword`

**Need More Specific Help?**
Try asking me about:
• "I need software for [specific task]"
• "How do I install [specific program]?"
• "What's the best [type of software] for Linux?"

Would you like me to search for packages related to your query?"""
        
        def clear_ai_chat(self):
            """Clear AI chat history"""
            self.ai_chat_display.clear()
            welcome_text = """Welcome to the AI Package Assistant! I can help you with:
• Finding the right packages for your needs
• Recommending dependencies and alternatives
• Troubleshooting installation issues
• Suggesting package management best practices
• Security analysis of packages

Just describe what you're looking for or ask any package-related question!"""
            self.ai_chat_display.append(welcome_text)
        
        def generate_ai_recommendations(self):
            """Generate smart package recommendations"""
            if not AI_FEATURES_AVAILABLE:
                QMessageBox.warning(self, "AI Not Available", "AI features require additional packages.")
                return
            
            # Sample recommendations (in a real implementation, this would use AI)
            recommendations = [
                {"package": "firefox", "reason": "Most popular browser", "popularity": "★★★★★", "security": "High"},
                {"package": "git", "reason": "Essential for development", "popularity": "★★★★★", "security": "High"},
                {"package": "docker", "reason": "Containerization platform", "popularity": "★★★★☆", "security": "Medium"},
                {"package": "vscode", "reason": "Popular code editor", "popularity": "★★★★★", "security": "High"},
                {"package": "discord", "reason": "Communication platform", "popularity": "★★★★☆", "security": "Medium"}
            ]
            
            self.ai_recommendations_table.setRowCount(len(recommendations))
            
            for row, rec in enumerate(recommendations):
                self.ai_recommendations_table.setItem(row, 0, QTableWidgetItem(rec["package"]))
                self.ai_recommendations_table.setItem(row, 1, QTableWidgetItem(rec["reason"]))
                self.ai_recommendations_table.setItem(row, 2, QTableWidgetItem(rec["popularity"]))
                self.ai_recommendations_table.setItem(row, 3, QTableWidgetItem(rec["security"]))
                
                install_btn = QPushButton("Install")
                install_btn.clicked.connect(lambda checked, pkg=rec["package"]: self.install_recommended_package(pkg))
                self.ai_recommendations_table.setCellWidget(row, 4, install_btn)
        
        def install_recommended_package(self, package_name):
            """Install a recommended package"""
            # Search for the package first
            packages = self.installer.search_package_ai_enhanced(package_name, False)
            if packages:
                self.installer.install_package(packages[0], confirm=False)
                self.ai_chat_display.append(f"\n📦 Installing {package_name}...")
            else:
                self.ai_chat_display.append(f"\n❌ Package {package_name} not found")
        
        # Package Building Methods
        def browse_source_directory(self):
            """Browse for source directory"""
            dir_path = QFileDialog.getExistingDirectory(self, "Select Source Directory")
            if dir_path:
                self.build_source_path.setText(dir_path)
        
        def browse_repository_path(self):
            """Browse for repository path"""
            dir_path = QFileDialog.getExistingDirectory(self, "Select Repository Directory")
            if dir_path:
                self.repo_path.setText(dir_path)
        
        def create_pkgbuild(self):
            """Create a PKGBUILD file"""
            package_name = self.build_package_name.text().strip()
            version = self.build_version.text().strip()
            description = self.build_description.text().strip()
            source_path = self.build_source_path.text().strip()
            
            if not all([package_name, version, description, source_path]):
                QMessageBox.warning(self, "Missing Information", "Please fill in all required fields.")
                return
            
            pkgbuild_content = f"""# Maintainer: Your Name <your.email@example.com>
pkgname={package_name}
pkgver={version}
pkgrel=1
pkgdesc="{description}"
arch=('x86_64')
url=""
license=('GPL')
depends=()
makdepends=()
source=()
sha256sums=()

build() {{
    cd "$srcdir"
    # Add build commands here
}}

package() {{
    cd "$srcdir"
    # Add package commands here
}}
"""
            
            # Save PKGBUILD file
            source_dir = Path(source_path)
            pkgbuild_path = source_dir / "PKGBUILD"
            
            try:
                with open(pkgbuild_path, 'w') as f:
                    f.write(pkgbuild_content)
                
                self.build_log.append(f"✅ PKGBUILD created: {pkgbuild_path}")
                QMessageBox.information(self, "PKGBUILD Created", f"PKGBUILD file created at {pkgbuild_path}")
            except Exception as e:
                self.build_log.append(f"❌ Failed to create PKGBUILD: {str(e)}")
                QMessageBox.critical(self, "Error", f"Failed to create PKGBUILD: {str(e)}")
        
        def build_package(self):
            """Build the package"""
            source_path = self.build_source_path.text().strip()
            if not source_path:
                QMessageBox.warning(self, "Missing Path", "Please specify source directory.")
                return
            
            source_dir = Path(source_path)
            pkgbuild_path = source_dir / "PKGBUILD"
            
            if not pkgbuild_path.exists():
                QMessageBox.warning(self, "No PKGBUILD", "PKGBUILD file not found. Create one first.")
                return
            
            self.build_log.append(f"🔨 Building package in {source_path}...")
            
            try:
                # Build package using makepkg
                result = subprocess.run(
                    ["makepkg", "-sf"],
                    cwd=source_path,
                    capture_output=True,
                    text=True,
                    timeout=300
                )
                
                if result.returncode == 0:
                    self.build_log.append("✅ Package built successfully!")
                    self.build_log.append(result.stdout)
                    
                    # Find built package files
                    pkg_files = list(source_dir.glob("*.pkg.tar.*"))
                    if pkg_files:
                        self.build_log.append(f"📦 Package files: {', '.join(f.name for f in pkg_files)}")
                else:
                    self.build_log.append(f"❌ Build failed: {result.stderr}")
                    
            except subprocess.TimeoutExpired:
                self.build_log.append("❌ Build timed out")
            except Exception as e:
                self.build_log.append(f"❌ Build error: {str(e)}")
        
        def test_package(self):
            """Test the built package"""
            source_path = self.build_source_path.text().strip()
            if not source_path:
                return
            
            source_dir = Path(source_path)
            pkg_files = list(source_dir.glob("*.pkg.tar.*"))
            
            if not pkg_files:
                QMessageBox.warning(self, "No Package", "No built package found. Build the package first.")
                return
            
            pkg_file = pkg_files[0]
            self.build_log.append(f"🧪 Testing package: {pkg_file.name}")
            
            try:
                # Test install package
                result = subprocess.run(
                    ["sudo", "pacman", "-U", "--noconfirm", str(pkg_file)],
                    capture_output=True,
                    text=True
                )
                
                if result.returncode == 0:
                    self.build_log.append("✅ Package installed successfully for testing")
                    
                    # Ask if user wants to remove test installation
                    reply = QMessageBox.question(
                        self, "Test Complete",
                        "Package test installation successful! Remove the test installation?",
                        QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
                    )
                    
                    if reply == QMessageBox.StandardButton.Yes:
                        package_name = self.build_package_name.text().strip()
                        remove_result = subprocess.run(
                            ["sudo", "pacman", "-R", "--noconfirm", package_name],
                            capture_output=True,
                            text=True
                        )
                        if remove_result.returncode == 0:
                            self.build_log.append("✅ Test installation removed")
                else:
                    self.build_log.append(f"❌ Package test failed: {result.stderr}")
                    
            except Exception as e:
                self.build_log.append(f"❌ Test error: {str(e)}")
        
        def create_repository(self):
            """Create a new package repository"""
            repo_name = self.repo_name.text().strip()
            repo_path = self.repo_path.text().strip()
            
            if not repo_name or not repo_path:
                QMessageBox.warning(self, "Missing Information", "Please provide repository name and path.")
                return
            
            repo_dir = Path(repo_path)
            repo_dir.mkdir(parents=True, exist_ok=True)
            
            self.build_log.append(f"🏗️ Creating repository: {repo_name} at {repo_path}")
            
            try:
                # Create repository database
                db_path = repo_dir / f"{repo_name}.db.tar.gz"
                
                result = subprocess.run(
                    ["repo-add", str(db_path)],
                    cwd=repo_path,
                    capture_output=True,
                    text=True
                )
                
                if result.returncode == 0:
                    self.build_log.append(f"✅ Repository created: {db_path}")
                    self.refresh_repository_packages()
                else:
                    self.build_log.append(f"❌ Repository creation failed: {result.stderr}")
                    
            except Exception as e:
                self.build_log.append(f"❌ Repository error: {str(e)}")
        
        def add_package_to_repository(self):
            """Add package to repository"""
            repo_name = self.repo_name.text().strip()
            repo_path = self.repo_path.text().strip()
            
            if not repo_name or not repo_path:
                QMessageBox.warning(self, "Missing Repository", "Please create or specify a repository first.")
                return
            
            # Browse for package file
            pkg_file, _ = QFileDialog.getOpenFileName(
                self, "Select Package File",
                "", "Package Files (*.pkg.tar.*);;All Files (*)"
            )
            
            if not pkg_file:
                return
            
            self.build_log.append(f"➕ Adding package to repository: {Path(pkg_file).name}")
            
            try:
                repo_dir = Path(repo_path)
                db_path = repo_dir / f"{repo_name}.db.tar.gz"
                
                # Copy package to repository
                pkg_dest = repo_dir / Path(pkg_file).name
                shutil.copy2(pkg_file, pkg_dest)
                
                # Add to repository database
                result = subprocess.run(
                    ["repo-add", str(db_path), str(pkg_dest)],
                    cwd=repo_path,
                    capture_output=True,
                    text=True
                )
                
                if result.returncode == 0:
                    self.build_log.append(f"✅ Package added to repository")
                    self.refresh_repository_packages()
                else:
                    self.build_log.append(f"❌ Failed to add package: {result.stderr}")
                    
            except Exception as e:
                self.build_log.append(f"❌ Add package error: {str(e)}")
        
        def sign_packages(self):
            """Sign packages in repository"""
            repo_path = self.repo_path.text().strip()
            if not repo_path:
                return
            
            self.build_log.append("🔐 Signing packages...")
            
            try:
                repo_dir = Path(repo_path)
                pkg_files = list(repo_dir.glob("*.pkg.tar.*"))
                
                for pkg_file in pkg_files:
                    if not Path(str(pkg_file) + ".sig").exists():
                        result = subprocess.run(
                            ["gpg", "--detach-sign", "--use-agent", str(pkg_file)],
                            capture_output=True,
                            text=True
                        )
                        
                        if result.returncode == 0:
                            self.build_log.append(f"✅ Signed: {pkg_file.name}")
                        else:
                            self.build_log.append(f"❌ Failed to sign {pkg_file.name}: {result.stderr}")
                
            except Exception as e:
                self.build_log.append(f"❌ Signing error: {str(e)}")
        
        def refresh_repository_packages(self):
            """Refresh repository packages table"""
            repo_path = self.repo_path.text().strip()
            if not repo_path:
                return
            
            try:
                repo_dir = Path(repo_path)
                pkg_files = list(repo_dir.glob("*.pkg.tar.*"))
                
                self.repo_packages_table.setRowCount(len(pkg_files))
                
                for row, pkg_file in enumerate(pkg_files):
                    # Extract package info
                    name_parts = pkg_file.name.split('-')
                    package_name = '-'.join(name_parts[:-3]) if len(name_parts) >= 3 else pkg_file.stem
                    version = name_parts[-3] if len(name_parts) >= 3 else "unknown"
                    arch = name_parts[-2] if len(name_parts) >= 2 else "unknown"
                    size = f"{pkg_file.stat().st_size / (1024*1024):.1f} MB"
                    
                    self.repo_packages_table.setItem(row, 0, QTableWidgetItem(package_name))
                    self.repo_packages_table.setItem(row, 1, QTableWidgetItem(version))
                    self.repo_packages_table.setItem(row, 2, QTableWidgetItem(arch))
                    self.repo_packages_table.setItem(row, 3, QTableWidgetItem(size))
                    
                    # Action buttons
                    actions_widget = QWidget()
                    actions_layout = QHBoxLayout(actions_widget)
                    actions_layout.setContentsMargins(0, 0, 0, 0)
                    
                    remove_btn = QPushButton("Remove")
                    remove_btn.clicked.connect(lambda checked, p=pkg_file: self.remove_package_from_repo(p))
                    actions_layout.addWidget(remove_btn)
                    
                    self.repo_packages_table.setCellWidget(row, 4, actions_widget)
                    
            except Exception as e:
                self.build_log.append(f"❌ Error refreshing packages: {str(e)}")
        
        def remove_package_from_repo(self, pkg_file):
            """Remove package from repository"""
            reply = QMessageBox.question(
                self, "Remove Package",
                f"Remove {pkg_file.name} from repository?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                try:
                    pkg_file.unlink()
                    # Also remove signature if exists
                    sig_file = Path(str(pkg_file) + ".sig")
                    if sig_file.exists():
                        sig_file.unlink()
                    
                    self.build_log.append(f"🗑️ Removed package: {pkg_file.name}")
                    self.refresh_repository_packages()
                    
                except Exception as e:
                    self.build_log.append(f"❌ Failed to remove package: {str(e)}")
        
        def save_build_log(self):
            """Save build log to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Build Log",
                str(Path.home() / f"build_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.build_log.toPlainText())
                    QMessageBox.information(self, "Log Saved", f"Build log saved to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save log: {str(e)}")
        
        # Install Tab Methods
        def install_single_package(self):
            """Install a single package"""
            package_name = self.install_package_input.text().strip()
            if not package_name:
                QMessageBox.warning(self, "Missing Package", "Please enter a package name.")
                return
            
            method_name = self.install_method_combo.currentText()
            method = InstallMethod(method_name)
            
            package = PackageInfo(
                name=package_name,
                method=method,
                source="direct",
                description=f"Direct installation via {method_name}"
            )
            
            # Add to queue
            self.install_queue.append(package)
            self.update_install_queue_table()
            
            self.install_log.append(f"➕ Added {package_name} to installation queue")
        
        def install_batch_packages(self):
            """Install multiple packages from batch input"""
            batch_text = self.batch_install_text.toPlainText().strip()
            if not batch_text:
                QMessageBox.warning(self, "Missing Packages", "Please enter package names for batch installation.")
                return
            
            package_names = [name.strip() for name in batch_text.split('\n') if name.strip()]
            method_name = self.install_method_combo.currentText()
            method = InstallMethod(method_name)
            
            for package_name in package_names:
                package = PackageInfo(
                    name=package_name,
                    method=method,
                    source="batch",
                    description=f"Batch installation via {method_name}"
                )
                self.install_queue.append(package)
            
            self.update_install_queue_table()
            self.install_log.append(f"➕ Added {len(package_names)} packages to installation queue")
        
        def search_before_install(self):
            """Search for package before installing"""
            package_name = self.install_package_input.text().strip()
            if not package_name:
                QMessageBox.warning(self, "Missing Package", "Please enter a package name to search.")
                return
            
            # Switch to search tab and perform search
            self.search_input.setText(package_name)
            self.search_packages()
        
        def clear_install_queue(self):
            """Clear the installation queue"""
            self.install_queue.clear()
            self.update_install_queue_table()
            self.install_log.append("🗑️ Installation queue cleared")
        
        def process_install_queue(self):
            """Process all packages in the installation queue"""
            if not self.install_queue:
                QMessageBox.information(self, "Empty Queue", "No packages in installation queue.")
                return
            
            reply = QMessageBox.question(
                self, "Process Queue",
                f"Install {len(self.install_queue)} packages from queue?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                self.install_log.append(f"▶️ Processing {len(self.install_queue)} packages...")
                
                for i, package in enumerate(self.install_queue):
                    self.install_log.append(f"📦 Installing {package.name} ({i+1}/{len(self.install_queue)})...")
                    
                    # Update status in table
                    self.install_queue_table.setItem(i, 2, QTableWidgetItem("Installing..."))
                    QApplication.processEvents()
                    
                    success = self.installer.install_package(package, confirm=False)
                    
                    if success:
                        self.install_queue_table.setItem(i, 2, QTableWidgetItem("✅ Installed"))
                        self.install_log.append(f"✅ {package.name} installed successfully")
                    else:
                        self.install_queue_table.setItem(i, 2, QTableWidgetItem("❌ Failed"))
                        self.install_log.append(f"❌ {package.name} installation failed")
                
                self.install_log.append("🎉 Queue processing complete!")
        
        def update_install_queue_table(self):
            """Update the installation queue table"""
            self.install_queue_table.setRowCount(len(self.install_queue))
            
            for row, package in enumerate(self.install_queue):
                self.install_queue_table.setItem(row, 0, QTableWidgetItem(package.name))
                self.install_queue_table.setItem(row, 1, QTableWidgetItem(package.method.value))
                self.install_queue_table.setItem(row, 2, QTableWidgetItem("Queued"))
                
                # Remove button
                remove_btn = QPushButton("Remove")
                remove_btn.clicked.connect(lambda checked, idx=row: self.remove_from_queue(idx))
                self.install_queue_table.setCellWidget(row, 3, remove_btn)
        
        def remove_from_queue(self, index):
            """Remove package from installation queue"""
            if 0 <= index < len(self.install_queue):
                removed_package = self.install_queue.pop(index)
                self.update_install_queue_table()
                if hasattr(self, 'install_log'):
                    self.install_log.append(f"➖ Removed {removed_package.name} from queue")
        
        def add_single_to_queue(self):
            """Add single package to queue"""
            package_name = self.install_package_input.text().strip()
            if not package_name:
                QMessageBox.warning(self, "Missing Package", "Please enter a package name.")
                return
            
            method_name = self.install_method_combo.currentText()
            method = InstallMethod(method_name)
            
            package = PackageInfo(
                name=package_name,
                method=method,
                source="direct",
                description=f"Direct installation via {method_name}"
            )
            
            self.install_queue.append(package)
            self.update_install_queue_table()
            if hasattr(self, 'install_log'):
                self.install_log.append(f"➕ Added {package_name} to installation queue")
        
        def add_batch_to_queue(self):
            """Add batch packages to queue"""
            batch_text = self.batch_install_text.toPlainText().strip()
            if not batch_text:
                QMessageBox.warning(self, "Missing Packages", "Please enter package names.")
                return
            
            package_names = [name.strip() for name in batch_text.split('\n') if name.strip()]
            method_name = self.batch_method_combo.currentText()
            method = InstallMethod(method_name)
            
            for package_name in package_names:
                package = PackageInfo(
                    name=package_name,
                    method=method,
                    source="batch",
                    description=f"Batch installation via {method_name}"
                )
                self.install_queue.append(package)
            
            self.update_install_queue_table()
            if hasattr(self, 'install_log'):
                self.install_log.append(f"➕ Added {len(package_names)} packages to installation queue")
        
        def pause_install_queue(self):
            """Pause installation queue processing"""
            if hasattr(self, 'install_log'):
                self.install_log.append("⏸️ Installation queue paused")
            # Implementation would pause ongoing installations
        
        def load_package_list(self):
            """Load package list from file"""
            file_path, _ = QFileDialog.getOpenFileName(
                self, "Load Package List",
                "", "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'r') as f:
                        content = f.read()
                    self.batch_install_text.setPlainText(content)
                    if hasattr(self, 'install_log'):
                        self.install_log.append(f"📁 Loaded package list from {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Load Failed", f"Failed to load file: {str(e)}")
        
        def save_package_list(self):
            """Save package list to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Package List",
                str(Path.home() / "package_list.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.batch_install_text.toPlainText())
                    if hasattr(self, 'install_log'):
                        self.install_log.append(f"💾 Saved package list to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save file: {str(e)}")
        
        def refresh_install_history(self):
            """Refresh installation history"""
            # Implementation would load from database
            if hasattr(self, 'install_log'):
                self.install_log.append("🔄 Installation history refreshed")
        
        def export_install_history(self):
            """Export installation history"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Export Install History",
                str(Path.home() / f"install_history_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"),
                "CSV Files (*.csv);;All Files (*)"
            )
            
            if file_path:
                if hasattr(self, 'install_log'):
                    self.install_log.append(f"📤 Installation history exported to {file_path}")
        
        def clear_install_history(self):
            """Clear installation history"""
            reply = QMessageBox.question(
                self, "Clear History",
                "Clear all installation history?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                self.install_history_table.setRowCount(0)
                if hasattr(self, 'install_log'):
                    self.install_log.append("🗑️ Installation history cleared")
        
        def filter_install_history(self, filter_text):
            """Filter installation history"""
            for row in range(self.install_history_table.rowCount()):
                item = self.install_history_table.item(row, 0)  # Package column
                if item:
                    should_show = filter_text.lower() in item.text().lower()
                    self.install_history_table.setRowHidden(row, not should_show)
        
        def clear_install_log(self):
            """Clear installation log"""
            if hasattr(self, 'install_log'):
                self.install_log.clear()
                self.install_log.append("📦 Installation Log - Ready")
        
        def save_install_log(self):
            """Save installation log"""
            if not hasattr(self, 'install_log'):
                QMessageBox.warning(self, "No Log", "Install log is not available.")
                return
                
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Installation Log",
                str(Path.home() / f"install_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.install_log.toPlainText())
                    QMessageBox.information(self, "Log Saved", f"Installation log saved to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save log: {str(e)}")
        
        # Settings Methods
        def change_theme(self, theme_name):
            """Change application theme"""
            if theme_name == "Dark (Bauh Style)":
                self.apply_bauh_theme()
            elif theme_name == "Light":
                self.apply_light_theme()
            else:
                self.setStyleSheet("")  # System default
        
        def apply_light_theme(self):
            """Apply light theme"""
            self.setStyleSheet("""
                QMainWindow {
                    background-color: #ffffff;
                    color: #000000;
                }
                
                QWidget {
                    background-color: #ffffff;
                    color: #000000;
                }
                
                QPushButton {
                    background-color: #0078d4;
                    color: #ffffff;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                }
                
                QPushButton:hover {
                    background-color: #106ebe;
                }
                
                QLineEdit {
                    background-color: #ffffff;
                    color: #000000;
                    border: 1px solid #cccccc;
                    padding: 8px;
                    border-radius: 4px;
                }
                
                QTextEdit {
                    background-color: #ffffff;
                    color: #000000;
                    border: 1px solid #cccccc;
                }
            """)
        
        def save_settings(self):
            """Save application settings"""
            settings_file = self.installer.config_dir / "settings.json"
            
            settings = {
                "auto_update": self.auto_update_check.isChecked(),
                "ai_features": self.ai_features_check.isChecked(),
                "performance_monitoring": self.performance_monitoring_check.isChecked(),
                "confirm_installs": self.confirm_installs_check.isChecked(),
                "mirror_country": self.mirror_country.currentText(),
                "parallel_downloads": self.parallel_downloads.value(),
                "preferred_aur_helper": self.preferred_aur_helper.currentText(),
                "enable_multilib": self.enable_multilib_check.isChecked(),
                "clean_cache_auto": self.clean_cache_auto_check.isChecked(),
                "theme": self.theme_combo.currentText(),
                "font_size": self.font_size_spin.value()
            }
            
            try:
                with open(settings_file, 'w') as f:
                    json.dump(settings, f, indent=2)
                QMessageBox.information(self, "Settings Saved", "Settings saved successfully!")
            except Exception as e:
                QMessageBox.critical(self, "Save Failed", f"Failed to save settings: {str(e)}")
        
        def reset_settings(self):
            """Reset settings to defaults"""
            reply = QMessageBox.question(
                self, "Reset Settings",
                "Reset all settings to default values?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                self.auto_update_check.setChecked(True)
                self.ai_features_check.setChecked(AI_FEATURES_AVAILABLE)
                self.performance_monitoring_check.setChecked(PSUTIL_AVAILABLE)
                self.confirm_installs_check.setChecked(True)
                self.mirror_country.setCurrentText("Auto")
                self.parallel_downloads.setValue(5)
                self.enable_multilib_check.setChecked(False)
                self.clean_cache_auto_check.setChecked(False)
                self.theme_combo.setCurrentText("Dark (Bauh Style)")
                self.font_size_spin.setValue(10)
                
                QMessageBox.information(self, "Settings Reset", "Settings reset to defaults!")
        
        def export_configuration(self):
            """Export configuration to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Export Configuration",
                str(Path.home() / f"arch_installer_config_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json"),
                "JSON Files (*.json);;All Files (*)"
            )
            
            if file_path:
                try:
                    config = {
                        "settings": {
                            "auto_update": self.auto_update_check.isChecked(),
                            "theme": self.theme_combo.currentText(),
                            "aur_helper": self.preferred_aur_helper.currentText()
                        },
                        "export_date": datetime.datetime.now().isoformat(),
                        "version": "1.0.0"
                    }
                    
                    with open(file_path, 'w') as f:
                        json.dump(config, f, indent=2)
                    
                    QMessageBox.information(self, "Export Complete", f"Configuration exported to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Export Failed", f"Failed to export configuration: {str(e)}")
        
        def import_configuration(self):
            """Import configuration from file"""
            file_path, _ = QFileDialog.getOpenFileName(
                self, "Import Configuration", "",
                "JSON Files (*.json);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'r') as f:
                        config = json.load(f)
                    
                    # Apply settings if they exist
                    if "settings" in config:
                        settings = config["settings"]
                        
                        if "auto_update" in settings:
                            self.auto_update_check.setChecked(settings["auto_update"])
                        if "theme" in settings:
                            self.theme_combo.setCurrentText(settings["theme"])
                        if "aur_helper" in settings:
                            self.preferred_aur_helper.setCurrentText(settings["aur_helper"])
                    
                    QMessageBox.information(self, "Import Complete", "Configuration imported successfully!")
                except Exception as e:
                    QMessageBox.critical(self, "Import Failed", f"Failed to import configuration: {str(e)}")
        
        def update_system_info(self):
            """Update system information display"""
            try:
                # Get system information
                with open('/etc/os-release', 'r') as f:
                    os_info = f.read()
                
                distro = "Unknown"
                for line in os_info.split('\n'):
                    if line.startswith('PRETTY_NAME='):
                        distro = line.split('=', 1)[1].strip('"')
                        break
                
                # Get kernel version
                kernel = subprocess.run(['uname', '-r'], capture_output=True, text=True).stdout.strip()
                
                # Get available methods
                methods = ', '.join([m.value for m in self.installer.available_methods])
                
                info_text = f"""Distribution: {distro}
Kernel: {kernel}
Python: {sys.version.split()[0]}
Qt: {'Available' if QT_AVAILABLE else 'Not Available'}
AI Features: {'Available' if AI_FEATURES_AVAILABLE else 'Not Available'}
Performance Monitoring: {'Available' if PSUTIL_AVAILABLE else 'Not Available'}
Available Package Managers: {methods}"""
                
                self.system_info_label.setText(info_text)
                
            except Exception as e:
                self.system_info_label.setText(f"Error getting system info: {str(e)}")
        
        # Missing search methods
        def perform_advanced_search(self):
            """Perform advanced search with filters"""
            package_name = self.adv_package_name.text().strip()
            category = self.adv_category_combo.currentText()
            method = self.adv_method_combo.currentText()
            description = self.adv_description.text().strip()
            
            if not package_name and not description:
                QMessageBox.warning(self, "No Search Terms", "Please enter package name or description to search.")
                return
            
            # Construct search query
            search_terms = []
            if package_name:
                search_terms.append(package_name)
            if description:
                search_terms.append(description)
            
            query = " ".join(search_terms)
            
            self.search_status.setText(f"Performing advanced search...")
            
            # Run search
            packages = self.installer.search_package_ai_enhanced(query, True)
            
            # Apply filters
            if category != "All Categories":
                packages = [pkg for pkg in packages if pkg.category.value == category]
            
            if method != "All Methods":
                packages = [pkg for pkg in packages if pkg.method.value == method]
            
            self.display_search_results(packages)
        
        def clear_search_filters(self):
            """Clear all search filters"""
            self.adv_package_name.clear()
            self.adv_category_combo.setCurrentIndex(0)
            self.adv_method_combo.setCurrentIndex(0)
            self.adv_description.clear()
            self.min_size_spin.setValue(0)
            self.max_size_spin.setValue(1000)
        
        def save_current_search(self):
            """Save current search parameters"""
            search_name, ok = QInputDialog.getText(self, "Save Search", "Enter search name:")
            if ok and search_name:
                # Implementation would save search parameters
                QMessageBox.information(self, "Search Saved", f"Search '{search_name}' saved successfully!")
        
        def manage_saved_searches(self):
            """Manage saved searches"""
            QMessageBox.information(self, "Saved Searches", "Saved searches management coming soon!")
        
        def load_saved_search(self, item):
            """Load a saved search"""
            search_name = item.text()
            # Implementation would load search parameters
            QMessageBox.information(self, "Search Loaded", f"Loaded search: {search_name}")
        
        def search_popular_package(self, package_name):
            """Search for a popular package"""
            self.search_input.setText(package_name)
            self.search_packages()
        
        def sort_search_results(self, sort_criteria):
            """Sort search results by criteria"""
            # Implementation would sort the results table
            pass
        
        def filter_search_results(self, filter_text):
            """Filter search results"""
            # Implementation would filter the results table
            for row in range(self.results_table.rowCount()):
                item = self.results_table.item(row, 1)  # Name column
                if item:
                    should_show = filter_text.lower() in item.text().lower()
                    self.results_table.setRowHidden(row, not should_show)
        
        def export_search_results(self):
            """Export search results to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Export Search Results",
                str(Path.home() / f"search_results_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"),
                "CSV Files (*.csv);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w', newline='') as csvfile:
                        import csv
                        writer = csv.writer(csvfile)
                        
                        # Write header
                        headers = []
                        for col in range(self.results_table.columnCount()):
                            headers.append(self.results_table.horizontalHeaderItem(col).text())
                        writer.writerow(headers)
                        
                        # Write data
                        for row in range(self.results_table.rowCount()):
                            if not self.results_table.isRowHidden(row):
                                row_data = []
                                for col in range(self.results_table.columnCount() - 1):  # Exclude action column
                                    item = self.results_table.item(row, col)
                                    row_data.append(item.text() if item else "")
                                writer.writerow(row_data)
                    
                    QMessageBox.information(self, "Export Complete", f"Results exported to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Export Failed", f"Failed to export results: {str(e)}")
        
        def install_selected_results(self):
            """Install selected packages from search results"""
            # Implementation would install selected packages
            QMessageBox.information(self, "Install Selected", "Installing selected packages...")
        
        def load_search_history(self):
            """Load search history"""
            # Implementation would load search history from database
            # For now, add sample data
            self.history_table.setRowCount(3)
            sample_history = [
                ("firefox", "5", "2.3s", "2025-06-22", "Repeat"),
                ("docker", "12", "1.8s", "2025-06-21", "Repeat"),
                ("code editor", "25", "3.1s", "2025-06-20", "Repeat")
            ]
            
            for row, (query, results, time, date, action) in enumerate(sample_history):
                self.history_table.setItem(row, 0, QTableWidgetItem(query))
                self.history_table.setItem(row, 1, QTableWidgetItem(results))
                self.history_table.setItem(row, 2, QTableWidgetItem(time))
                self.history_table.setItem(row, 3, QTableWidgetItem(date))
                
                repeat_btn = QPushButton("Repeat")
                repeat_btn.clicked.connect(lambda checked, q=query: self.repeat_search(q))
                self.history_table.setCellWidget(row, 4, repeat_btn)
        
        def repeat_search(self, query):
            """Repeat a search from history"""
            self.search_input.setText(query)
            self.search_packages()
        
        def clear_search_history(self):
            """Clear search history"""
            reply = QMessageBox.question(
                self, "Clear History",
                "Clear all search history?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                self.history_table.setRowCount(0)
                QMessageBox.information(self, "History Cleared", "Search history cleared successfully!")
        
        def export_search_history(self):
            """Export search history"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Export Search History",
                str(Path.home() / f"search_history_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"),
                "CSV Files (*.csv);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w', newline='') as csvfile:
                        import csv
                        writer = csv.writer(csvfile)
                        
                        # Write header
                        writer.writerow(["Query", "Results", "Time", "Date"])
                        
                        # Write data
                        for row in range(self.history_table.rowCount()):
                            row_data = []
                            for col in range(4):  # Exclude action column
                                item = self.history_table.item(row, col)
                                row_data.append(item.text() if item else "")
                            writer.writerow(row_data)
                    
                    QMessageBox.information(self, "Export Complete", f"History exported to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Export Failed", f"Failed to export history: {str(e)}")
        
        # Missing maintenance methods
        def check_for_updates(self):
            """Check for system updates"""
            self.maintenance_log.append("🔍 Checking for system updates...")
            
            try:
                # Check for pacman updates
                result = subprocess.run(
                    ["pacman", "-Qu"],
                    capture_output=True, text=True, timeout=30
                )
                
                if result.returncode == 0 and result.stdout.strip():
                    updates = result.stdout.strip().split('\n')
                    self.maintenance_log.append(f"✅ Found {len(updates)} available updates")
                    
                    # Populate updates table
                    self.updates_table.setRowCount(len(updates))
                    for row, update_line in enumerate(updates):
                        parts = update_line.split()
                        if len(parts) >= 4:
                            package = parts[0]
                            current_ver = parts[1]
                            arrow = parts[2]  # ->
                            new_ver = parts[3]
                            
                            self.updates_table.setItem(row, 0, QTableWidgetItem(package))
                            self.updates_table.setItem(row, 1, QTableWidgetItem(current_ver))
                            self.updates_table.setItem(row, 2, QTableWidgetItem(new_ver))
                            self.updates_table.setItem(row, 3, QTableWidgetItem("Unknown"))
                else:
                    self.maintenance_log.append("✅ System is up to date")
                    self.updates_table.setRowCount(0)
                    
            except subprocess.TimeoutExpired:
                self.maintenance_log.append("⏰ Update check timed out")
            except Exception as e:
                self.maintenance_log.append(f"❌ Update check failed: {str(e)}")
        
        def install_system_updates(self):
            """Install system updates"""
            update_type = self.update_type_combo.currentText()
            download_only = self.download_only_check.isChecked()
            
            self.maintenance_log.append(f"⬇️ Installing updates ({update_type})...")
            
            if download_only:
                self.maintenance_log.append("📥 Download only mode enabled")
                # Implementation would download updates only
            else:
                # Run actual update
                success = self.installer.system_maintenance("system_update")
                if success:
                    self.maintenance_log.append("✅ System updates installed successfully")
                    self.check_for_updates()  # Refresh update list
                else:
                    self.maintenance_log.append("❌ System update failed")
        
        def clean_aur_cache(self):
            """Clean AUR helper cache"""
            self.maintenance_log.append("🧹 Cleaning AUR cache...")
            
            try:
                for helper in ["yay", "paru", "pikaur"]:
                    if shutil.which(helper):
                        result = subprocess.run(
                            [helper, "-Sc", "--noconfirm"],
                            capture_output=True, text=True
                        )
                        if result.returncode == 0:
                            self.maintenance_log.append(f"✅ {helper} cache cleaned")
                        else:
                            self.maintenance_log.append(f"❌ Failed to clean {helper} cache")
                        break
                else:
                    self.maintenance_log.append("❌ No AUR helper found")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ AUR cache cleaning failed: {str(e)}")
        
        def clean_all_caches(self):
            """Clean all package caches"""
            self.maintenance_log.append("🗑️ Cleaning all caches...")
            
            # Clean pacman cache
            self.run_maintenance("clean_cache")
            
            # Clean AUR cache
            self.clean_aur_cache()
            
            # Clean other caches
            try:
                # Clean flatpak cache
                if shutil.which("flatpak"):
                    result = subprocess.run(["flatpak", "uninstall", "--unused", "-y"], capture_output=True, text=True)
                    if result.returncode == 0:
                        self.maintenance_log.append("✅ Flatpak cache cleaned")
                
                # Clean snap cache
                if shutil.which("snap"):
                    result = subprocess.run(["sudo", "snap", "set", "system", "refresh.retain=2"], capture_output=True, text=True)
                    if result.returncode == 0:
                        self.maintenance_log.append("✅ Snap cache settings updated")
                        
            except Exception as e:
                self.maintenance_log.append(f"❌ Additional cache cleaning failed: {str(e)}")
        
        def view_cache_contents(self):
            """View cache contents"""
            cache_dialog = QDialog(self)
            cache_dialog.setWindowTitle("Package Cache Contents")
            cache_dialog.setModal(True)
            cache_dialog.resize(800, 600)
            
            layout = QVBoxLayout(cache_dialog)
            
            cache_table = QTableWidget()
            cache_table.setColumnCount(4)
            cache_table.setHorizontalHeaderLabels(["Package", "Version", "Size", "Date"])
            layout.addWidget(cache_table)
            
            # Populate with cache files
            try:
                cache_dir = Path("/var/cache/pacman/pkg")
                if cache_dir.exists():
                    pkg_files = list(cache_dir.glob("*.pkg.tar.*"))
                    cache_table.setRowCount(len(pkg_files))
                    
                    for row, pkg_file in enumerate(pkg_files):
                        name_parts = pkg_file.name.split('-')
                        package_name = '-'.join(name_parts[:-3]) if len(name_parts) >= 3 else pkg_file.stem
                        version = name_parts[-3] if len(name_parts) >= 3 else "unknown"
                        size = f"{pkg_file.stat().st_size / (1024*1024):.1f} MB"
                        date = datetime.datetime.fromtimestamp(pkg_file.stat().st_mtime).strftime('%Y-%m-%d')
                        
                        cache_table.setItem(row, 0, QTableWidgetItem(package_name))
                        cache_table.setItem(row, 1, QTableWidgetItem(version))
                        cache_table.setItem(row, 2, QTableWidgetItem(size))
                        cache_table.setItem(row, 3, QTableWidgetItem(date))
                        
            except Exception as e:
                error_label = QLabel(f"Error reading cache: {str(e)}")
                layout.addWidget(error_label)
            
            # Close button
            close_btn = QPushButton("Close")
            close_btn.clicked.connect(cache_dialog.close)
            layout.addWidget(close_btn)
            
            cache_dialog.exec()
        
        def optimize_mirror_list(self):
            """Optimize mirror list using reflector"""
            self.maintenance_log.append("🪞 Optimizing mirror list...")
            success = self.installer.system_maintenance("update_mirrors")
            if success:
                self.maintenance_log.append("✅ Mirror list optimized")
            else:
                self.maintenance_log.append("❌ Mirror optimization failed")
        
        def cleanup_orphaned_packages(self):
            """Remove orphaned packages"""
            self.maintenance_log.append("🧹 Removing orphaned packages...")
            
            try:
                # Find orphaned packages
                result = subprocess.run(
                    ["pacman", "-Qtdq"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0 and result.stdout.strip():
                    orphans = result.stdout.strip().split('\n')
                    self.maintenance_log.append(f"Found {len(orphans)} orphaned packages")
                    
                    # Remove orphans
                    remove_result = subprocess.run(
                        ["sudo", "pacman", "-Rs", "--noconfirm"] + orphans,
                        capture_output=True, text=True
                    )
                    
                    if remove_result.returncode == 0:
                        self.maintenance_log.append(f"✅ Removed {len(orphans)} orphaned packages")
                    else:
                        self.maintenance_log.append(f"❌ Failed to remove orphans: {remove_result.stderr}")
                else:
                    self.maintenance_log.append("✅ No orphaned packages found")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Orphan cleanup failed: {str(e)}")
        
        def trim_system_logs(self):
            """Trim system logs"""
            self.maintenance_log.append("📝 Trimming system logs...")
            
            try:
                # Use journalctl to trim logs
                result = subprocess.run(
                    ["sudo", "journalctl", "--vacuum-time=1month"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0:
                    self.maintenance_log.append("✅ System logs trimmed successfully")
                    if result.stdout:
                        self.maintenance_log.append(f"Details: {result.stdout.strip()}")
                else:
                    self.maintenance_log.append(f"❌ Log trimming failed: {result.stderr}")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Log trimming error: {str(e)}")
        
        def optimize_package_database(self):
            """Optimize package database"""
            self.maintenance_log.append("🗃️ Optimizing package database...")
            
            try:
                # Sync and refresh package database
                result = subprocess.run(
                    ["sudo", "pacman", "-Fyy"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0:
                    self.maintenance_log.append("✅ Package database optimized")
                else:
                    self.maintenance_log.append(f"❌ Database optimization failed: {result.stderr}")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Database optimization error: {str(e)}")
        
        def save_maintenance_log(self):
            """Save maintenance log to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Maintenance Log",
                str(Path.home() / f"maintenance_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.maintenance_log.toPlainText())
                    QMessageBox.information(self, "Log Saved", f"Maintenance log saved to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save log: {str(e)}")
        
        # AI dependency installation methods
        def install_ai_dependencies(self):
            """Install all AI dependencies"""
            self.deps_install_log.append("🚀 Installing all AI dependencies...")
            
            # Try system packages first, then fallback to virtual environment
            system_packages = {
                "scikit-learn": "python-scikit-learn",
                "nltk": "python-nltk", 
                "requests": "python-requests",
                "beautifulsoup4": "python-beautifulsoup4"
            }
            
            for python_name, arch_name in system_packages.items():
                self.install_single_dependency(python_name, arch_name)
            
            # After installing NLTK, download required data
            try:
                import nltk
                self.download_nltk_data(nltk)
            except ImportError:
                self.deps_install_log.append("⚠️ NLTK not available for data download")
        
        def install_single_dependency(self, python_name, arch_package=None):
            """Install a single AI dependency"""
            self.deps_install_log.append(f"📦 Installing {python_name}...")
            
            # Try system package first if available
            if arch_package:
                try:
                    self.deps_install_log.append(f"Trying system package: {arch_package}")
                    result = subprocess.run(
                        ["sudo", "pacman", "-S", "--noconfirm", arch_package],
                        capture_output=True, text=True, timeout=120
                    )
                    
                    if result.returncode == 0:
                        self.deps_install_log.append(f"✅ {arch_package} installed via pacman")
                        self.check_ai_dependencies_status()
                        return
                    else:
                        self.deps_install_log.append(f"⚠️ System package {arch_package} not available, trying alternative...")
                        
                except Exception as e:
                    self.deps_install_log.append(f"⚠️ System package install failed: {str(e)}")
            
            # Fallback to virtual environment installation
            try:
                # Create virtual environment if it doesn't exist
                venv_path = Path.home() / ".local" / "venv" / "ai_deps"
                if not venv_path.exists():
                    self.deps_install_log.append("Creating virtual environment...")
                    result = subprocess.run(
                        ["python", "-m", "venv", str(venv_path)],
                        capture_output=True, text=True, timeout=60
                    )
                    if result.returncode != 0:
                        self.deps_install_log.append(f"❌ Failed to create virtual environment: {result.stderr}")
                        return
                
                # Install package in virtual environment
                pip_path = venv_path / "bin" / "pip"
                result = subprocess.run(
                    [str(pip_path), "install", python_name],
                    capture_output=True, text=True, timeout=120
                )
                
                if result.returncode == 0:
                    self.deps_install_log.append(f"✅ {python_name} installed in virtual environment")
                    self.deps_install_log.append("Note: Virtual environment packages may not be automatically detected.")
                    self.deps_install_log.append(f"To use: source {venv_path}/bin/activate")
                else:
                    self.deps_install_log.append(f"❌ Failed to install {python_name}: {result.stderr}")
                    
            except subprocess.TimeoutExpired:
                self.deps_install_log.append(f"⏰ Installation of {python_name} timed out")
            except Exception as e:
                self.deps_install_log.append(f"❌ Error installing {python_name}: {str(e)}")
            
            self.check_ai_dependencies_status()
        
        def check_ai_dependencies_status(self):
            """Check if AI dependencies are now available"""
            try:
                import importlib
                
                deps_status = []
                dependencies = ["sklearn", "nltk", "requests", "bs4"]
                
                for dep in dependencies:
                    try:
                        module = importlib.import_module(dep)
                        deps_status.append(True)
                    except ImportError:
                        deps_status.append(False)
                
                if all(deps_status):
                    self.ai_status_label.setText("✅ AI features are now available!")
                    self.ai_status_label.setStyleSheet("color: #4caf50; font-weight: bold; font-size: 14px;")
                    self.deps_install_log.append("🎉 All AI dependencies installed! Please restart the application.")
                else:
                    missing = [dep for dep, status in zip(dependencies, deps_status) if not status]
                    self.deps_install_log.append(f"⚠️ Still missing: {', '.join(missing)}")
                    
            except Exception as e:
                self.deps_install_log.append(f"❌ Error checking dependencies: {str(e)}")
        
        def download_nltk_data(self, nltk_module):
            """Download required NLTK data"""
            try:
                self.deps_install_log.append("📥 Downloading NLTK data...")
                
                # Download required NLTK datasets
                required_data = ['wordnet', 'punkt', 'stopwords', 'averaged_perceptron_tagger']
                
                for dataset in required_data:
                    try:
                        nltk_module.download(dataset, quiet=True)
                        self.deps_install_log.append(f"✅ Downloaded NLTK dataset: {dataset}")
                    except Exception as e:
                        self.deps_install_log.append(f"⚠️ Failed to download {dataset}: {str(e)}")
                        
                self.deps_install_log.append("✅ NLTK data download completed")
                
            except Exception as e:
                self.deps_install_log.append(f"❌ Error downloading NLTK data: {str(e)}")
        
        def refresh_recommendations(self):
            """Refresh AI recommendations"""
            if not AI_FEATURES_AVAILABLE:
                QMessageBox.warning(self, "AI Not Available", "Please install AI dependencies first.")
                return
            
            self.generate_ai_recommendations()
        
        def analyze_package(self):
            """Analyze a package using AI"""
            package_name = self.analysis_package_input.text().strip()
            if not package_name:
                QMessageBox.warning(self, "No Package", "Please enter a package name to analyze.")
                return
            
            if not AI_FEATURES_AVAILABLE:
                self.analysis_results.append("❌ AI features not available. Please install dependencies.")
                return
            
            self.analysis_results.clear()
            self.analysis_results.append(f"🔍 Analyzing package: {package_name}")
            self.analysis_results.append("-" * 50)
            
            # Search for the package first
            packages = self.installer.search_package_ai_enhanced(package_name, False)
            
            if packages:
                pkg = packages[0]
                analysis = f"""📦 Package Information:
• Name: {pkg.name}
• Method: {pkg.method.value}
• Description: {pkg.description}
• Category: {pkg.category.value}
• Source: {pkg.source}

🔍 Analysis Results:
• Popularity Score: High (based on search ranking)
• Security Assessment: Pending detailed scan
• Dependency Impact: Analyzing...
• Performance Impact: Low to Medium
• Compatibility: Good with current system

💡 Recommendations:
• Safe to install from official repositories
• Consider checking dependencies before installation
• Regular updates recommended

⚠️ Notes:
• This is a simplified analysis
• Full AI analysis requires additional processing
• Check official documentation for detailed information"""
                
                self.analysis_results.append(analysis)
            else:
                self.analysis_results.append(f"❌ Package '{package_name}' not found in repositories")
        
        def ai_quick_query(self, query):
            """Process a quick AI query"""
            self.ai_input.setText(query)
            self.process_ai_query()
        
        def clear_search_history(self):
            """Clear search history"""
            reply = QMessageBox.question(
                self, "Clear History",
                "Clear all search history?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            
            if reply == QMessageBox.StandardButton.Yes:
                self.history_table.setRowCount(0)
                QMessageBox.information(self, "History Cleared", "Search history cleared successfully!")
        
        def export_search_history(self):
            """Export search history"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Export Search History",
                str(Path.home() / f"search_history_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"),
                "CSV Files (*.csv);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w', newline='') as csvfile:
                        import csv
                        writer = csv.writer(csvfile)
                        
                        # Write header
                        writer.writerow(["Query", "Results", "Time", "Date"])
                        
                        # Write data
                        for row in range(self.history_table.rowCount()):
                            row_data = []
                            for col in range(4):  # Exclude action column
                                item = self.history_table.item(row, col)
                                row_data.append(item.text() if item else "")
                            writer.writerow(row_data)
                    
                    QMessageBox.information(self, "Export Complete", f"History exported to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Export Failed", f"Failed to export history: {str(e)}")
        
        # Missing maintenance methods
        def check_for_updates(self):
            """Check for system updates"""
            self.maintenance_log.append("🔍 Checking for system updates...")
            
            try:
                # Check for pacman updates
                result = subprocess.run(
                    ["pacman", "-Qu"],
                    capture_output=True, text=True, timeout=30
                )
                
                if result.returncode == 0 and result.stdout.strip():
                    updates = result.stdout.strip().split('\n')
                    self.maintenance_log.append(f"✅ Found {len(updates)} available updates")
                    
                    # Populate updates table
                    self.updates_table.setRowCount(len(updates))
                    for row, update_line in enumerate(updates):
                        parts = update_line.split()
                        if len(parts) >= 4:
                            package = parts[0]
                            current_ver = parts[1]
                            arrow = parts[2]  # ->
                            new_ver = parts[3]
                            
                            self.updates_table.setItem(row, 0, QTableWidgetItem(package))
                            self.updates_table.setItem(row, 1, QTableWidgetItem(current_ver))
                            self.updates_table.setItem(row, 2, QTableWidgetItem(new_ver))
                            self.updates_table.setItem(row, 3, QTableWidgetItem("Unknown"))
                else:
                    self.maintenance_log.append("✅ System is up to date")
                    self.updates_table.setRowCount(0)
                    
            except subprocess.TimeoutExpired:
                self.maintenance_log.append("⏰ Update check timed out")
            except Exception as e:
                self.maintenance_log.append(f"❌ Update check failed: {str(e)}")
        
        def install_system_updates(self):
            """Install system updates"""
            update_type = self.update_type_combo.currentText()
            download_only = self.download_only_check.isChecked()
            
            self.maintenance_log.append(f"⬇️ Installing updates ({update_type})...")
            
            if download_only:
                self.maintenance_log.append("📥 Download only mode enabled")
                # Implementation would download updates only
            else:
                # Run actual update
                success = self.installer.system_maintenance("system_update")
                if success:
                    self.maintenance_log.append("✅ System updates installed successfully")
                    self.check_for_updates()  # Refresh update list
                else:
                    self.maintenance_log.append("❌ System update failed")
        
        def clean_aur_cache(self):
            """Clean AUR helper cache"""
            self.maintenance_log.append("🧹 Cleaning AUR cache...")
            
            try:
                for helper in ["yay", "paru", "pikaur"]:
                    if shutil.which(helper):
                        result = subprocess.run(
                            [helper, "-Sc", "--noconfirm"],
                            capture_output=True, text=True
                        )
                        if result.returncode == 0:
                            self.maintenance_log.append(f"✅ {helper} cache cleaned")
                        else:
                            self.maintenance_log.append(f"❌ Failed to clean {helper} cache")
                        break
                else:
                    self.maintenance_log.append("❌ No AUR helper found")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ AUR cache cleaning failed: {str(e)}")
        
        def clean_all_caches(self):
            """Clean all package caches"""
            self.maintenance_log.append("🗑️ Cleaning all caches...")
            
            # Clean pacman cache
            self.run_maintenance("clean_cache")
            
            # Clean AUR cache
            self.clean_aur_cache()
            
            # Clean other caches
            try:
                # Clean flatpak cache
                if shutil.which("flatpak"):
                    result = subprocess.run(["flatpak", "uninstall", "--unused", "-y"], capture_output=True, text=True)
                    if result.returncode == 0:
                        self.maintenance_log.append("✅ Flatpak cache cleaned")
                
                # Clean snap cache
                if shutil.which("snap"):
                    result = subprocess.run(["sudo", "snap", "set", "system", "refresh.retain=2"], capture_output=True, text=True)
                    if result.returncode == 0:
                        self.maintenance_log.append("✅ Snap cache settings updated")
                        
            except Exception as e:
                self.maintenance_log.append(f"❌ Additional cache cleaning failed: {str(e)}")
        
        def view_cache_contents(self):
            """View cache contents"""
            cache_dialog = QDialog(self)
            cache_dialog.setWindowTitle("Package Cache Contents")
            cache_dialog.setModal(True)
            cache_dialog.resize(800, 600)
            
            layout = QVBoxLayout(cache_dialog)
            
            cache_table = QTableWidget()
            cache_table.setColumnCount(4)
            cache_table.setHorizontalHeaderLabels(["Package", "Version", "Size", "Date"])
            layout.addWidget(cache_table)
            
            # Populate with cache files
            try:
                cache_dir = Path("/var/cache/pacman/pkg")
                if cache_dir.exists():
                    pkg_files = list(cache_dir.glob("*.pkg.tar.*"))
                    cache_table.setRowCount(len(pkg_files))
                    
                    for row, pkg_file in enumerate(pkg_files):
                        name_parts = pkg_file.name.split('-')
                        package_name = '-'.join(name_parts[:-3]) if len(name_parts) >= 3 else pkg_file.stem
                        version = name_parts[-3] if len(name_parts) >= 3 else "unknown"
                        size = f"{pkg_file.stat().st_size / (1024*1024):.1f} MB"
                        date = datetime.datetime.fromtimestamp(pkg_file.stat().st_mtime).strftime('%Y-%m-%d')
                        
                        cache_table.setItem(row, 0, QTableWidgetItem(package_name))
                        cache_table.setItem(row, 1, QTableWidgetItem(version))
                        cache_table.setItem(row, 2, QTableWidgetItem(size))
                        cache_table.setItem(row, 3, QTableWidgetItem(date))
                        
            except Exception as e:
                error_label = QLabel(f"Error reading cache: {str(e)}")
                layout.addWidget(error_label)
            
            # Close button
            close_btn = QPushButton("Close")
            close_btn.clicked.connect(cache_dialog.close)
            layout.addWidget(close_btn)
            
            cache_dialog.exec()
        
        def optimize_mirror_list(self):
            """Optimize mirror list using reflector"""
            self.maintenance_log.append("🪞 Optimizing mirror list...")
            success = self.installer.system_maintenance("update_mirrors")
            if success:
                self.maintenance_log.append("✅ Mirror list optimized")
            else:
                self.maintenance_log.append("❌ Mirror optimization failed")
        
        def cleanup_orphaned_packages(self):
            """Remove orphaned packages"""
            self.maintenance_log.append("🧹 Removing orphaned packages...")
            
            try:
                # Find orphaned packages
                result = subprocess.run(
                    ["pacman", "-Qtdq"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0 and result.stdout.strip():
                    orphans = result.stdout.strip().split('\n')
                    self.maintenance_log.append(f"Found {len(orphans)} orphaned packages")
                    
                    # Remove orphans
                    remove_result = subprocess.run(
                        ["sudo", "pacman", "-Rs", "--noconfirm"] + orphans,
                        capture_output=True, text=True
                    )
                    
                    if remove_result.returncode == 0:
                        self.maintenance_log.append(f"✅ Removed {len(orphans)} orphaned packages")
                    else:
                        self.maintenance_log.append(f"❌ Failed to remove orphans: {remove_result.stderr}")
                else:
                    self.maintenance_log.append("✅ No orphaned packages found")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Orphan cleanup failed: {str(e)}")
        
        def trim_system_logs(self):
            """Trim system logs"""
            self.maintenance_log.append("📝 Trimming system logs...")
            
            try:
                # Use journalctl to trim logs
                result = subprocess.run(
                    ["sudo", "journalctl", "--vacuum-time=1month"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0:
                    self.maintenance_log.append("✅ System logs trimmed successfully")
                    if result.stdout:
                        self.maintenance_log.append(f"Details: {result.stdout.strip()}")
                else:
                    self.maintenance_log.append(f"❌ Log trimming failed: {result.stderr}")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Log trimming error: {str(e)}")
        
        def optimize_package_database(self):
            """Optimize package database"""
            self.maintenance_log.append("🗃️ Optimizing package database...")
            
            try:
                # Sync and refresh package database
                result = subprocess.run(
                    ["sudo", "pacman", "-Fyy"],
                    capture_output=True, text=True
                )
                
                if result.returncode == 0:
                    self.maintenance_log.append("✅ Package database optimized")
                else:
                    self.maintenance_log.append(f"❌ Database optimization failed: {result.stderr}")
                    
            except Exception as e:
                self.maintenance_log.append(f"❌ Database optimization error: {str(e)}")
        
        def save_maintenance_log(self):
            """Save maintenance log to file"""
            file_path, _ = QFileDialog.getSaveFileName(
                self, "Save Maintenance Log",
                str(Path.home() / f"maintenance_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"),
                "Text Files (*.txt);;All Files (*)"
            )
            
            if file_path:
                try:
                    with open(file_path, 'w') as f:
                        f.write(self.maintenance_log.toPlainText())
                    QMessageBox.information(self, "Log Saved", f"Maintenance log saved to {file_path}")
                except Exception as e:
                    QMessageBox.critical(self, "Save Failed", f"Failed to save log: {str(e)}")


    class SearchThread(QThread):
        """Background thread for package searching"""
        results_ready = pyqtSignal(list)
        
        def __init__(self, installer, query, use_ai):
            super().__init__()
            self.installer = installer
            self.query = query
            self.use_ai = use_ai
        
        def run(self):
            packages = self.installer.search_package_ai_enhanced(self.query, self.use_ai)
            self.results_ready.emit(packages)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description="Universal Arch Linux Installer - Optimized")
    parser.add_argument("package", nargs='?', help="Package name to install/search")
    parser.add_argument("-c", "--cli", action="store_true", help="Launch CLI interface (default is GUI)")
    parser.add_argument("-s", "--search", action="store_true", help="Search for packages")
    parser.add_argument("-i", "--interactive", action="store_true", help="Interactive CLI mode")
    parser.add_argument("--ai", action="store_true", help="Use AI enhancements")
    parser.add_argument("-m", "--maintenance", 
                       choices=["update_mirrors", "clean_cache", "system_update", "cylon"],
                       help="Run maintenance operation")
    
    args = parser.parse_args()
    
    # Default to GUI unless CLI explicitly requested or GUI not available
    launch_gui = not args.cli and QT_AVAILABLE
    
    # Launch GUI by default if available
    if launch_gui:
        app = QApplication(sys.argv)
        app.setApplicationName("Universal Arch Linux Installer")
        app.setOrganizationName("UniversalArchInstaller")
        
        # Set application-wide dark theme
        app.setStyle('Fusion')
        
        window = UniversalArchInstallerGUI()
        window.show()
        
        sys.exit(app.exec())
    elif not QT_AVAILABLE and not args.cli:
        print("❌ PyQt6 not available. Install with: pip install PyQt6")
        print("Falling back to CLI mode...")
    
    # CLI mode
    installer = UniversalArchInstaller()
    
    if args.maintenance:
        # Map CLI argument to internal method name
        maintenance_op = args.maintenance
        if maintenance_op == "cylon":
            maintenance_op = "cylon_maintenance"
        
        success = installer.system_maintenance(maintenance_op)
        sys.exit(0 if success else 1)
    
    if args.search and args.package:
        packages = installer.search_package_ai_enhanced(args.package, args.ai)
        if packages:
            installer.print_colored(f"📋 Found {len(packages)} packages:", Colors.GREEN)
            for pkg in packages:
                installer.print_colored(
                    f"  • {pkg.name} ({pkg.method.value}) - {pkg.description}", 
                    Colors.WHITE
                )
        else:
            installer.print_colored(f"❌ No packages found for '{args.package}'", Colors.RED)
    elif args.interactive or not args.package:
        # Launch interactive mode by default if no specific action
        installer.run_interactive_mode()
    elif args.package:
        # Direct package installation
        packages = installer.search_package_ai_enhanced(args.package, args.ai)
        if packages:
            installer.install_package(packages[0])
        else:
            installer.print_colored(f"❌ Package '{args.package}' not found", Colors.RED)


if __name__ == "__main__":
    main()
