#!/usr/bin/env python3
"""
Calibre Library Fixer - GUI Version
===================================
Modern PyQt6 GUI for the Calibre library scanner and filename fixer.

Author: AI Assistant
Date: 2025-06-20
License: MIT
"""

import sys
import os
import threading
from pathlib import Path
from typing import Optional, Dict, List
import json
import sqlite3
import re
import shutil
import logging
import xml.etree.ElementTree as ET
import requests
import time
from datetime import datetime

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget,
    QPushButton, QLabel, QLineEdit, QTextEdit, QProgressBar,
    QCheckBox, QGroupBox, QGridLayout, QFileDialog, QMessageBox,
    QSplitter, QTabWidget, QTableWidget, QTableWidgetItem,
    QHeaderView, QComboBox, QSpinBox, QFrame, QScrollArea,
    QDialog, QDialogButtonBox, QTextBrowser, QButtonGroup, QRadioButton
)
from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSettings, QSize
)
from PyQt6.QtGui import (
    QFont, QIcon, QPalette, QColor, QPixmap, QPainter, QBrush, QPen
)

# Import from the CLI version if available, otherwise use embedded version
try:
    import sys
    from pathlib import Path
    sys.path.append(str(Path(__file__).parent))
    from calibre_library_fixer import CalibreLibraryFixer as CoreCalibreLibraryFixer
    from calibre_library_fixer import find_calibre_library as core_find_calibre_library
except ImportError:
    CoreCalibreLibraryFixer = None
    core_find_calibre_library = None

# Enhanced CalibreLibraryFixer class with full metadata support
class CalibreLibraryFixer:
    """Advanced Calibre library scanner with enhanced metadata detection and external lookups"""
    
    def __init__(self, library_path: str, dry_run: bool = True, log_file: str = "/home/lou/calibre_fixer.log"):
        self.library_path = Path(library_path)
        self.dry_run = dry_run
        self.db_path = self.library_path / 'metadata.db'
        self.changes_made = []
        self.errors = []
        self.log_file = log_file
        
        # Setup enhanced logging
        self.setup_logging()
        
        # Initialize metadata cache
        self.metadata_cache = {}
        self.opf_metadata = {}
        
        if not self.library_path.exists():
            raise FileNotFoundError(f"Calibre library not found: {library_path}")
        
        # Check if we should use database or OPF files
        self.use_database = self.db_path.exists() and self.db_path.stat().st_size > 0
        
        if not self.use_database:
            self.log(f"Database not available or empty, using OPF files for metadata")
        else:
            self.log(f"Using Calibre database: {self.db_path}")
    
    def setup_logging(self):
        """Setup enhanced logging to file and console"""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler(self.log_file, encoding='utf-8'),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger('CalibreLibraryFixer')
        
    def log(self, message: str, level='info'):
        """Enhanced logging with file output"""
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        log_entry = f"[{timestamp}] {message}"
        
        # Write to log file
        try:
            with open(self.log_file, 'a', encoding='utf-8') as f:
                f.write(log_entry + '\n')
        except Exception:
            pass
        
        # Also use logger
        if level == 'error':
            self.logger.error(message)
        elif level == 'warning':
            self.logger.warning(message)
        else:
            self.logger.info(message)
    
    # Add filename sanitization cache for performance
    _filename_cache = {}
    
    def sanitize_filename(self, text: str) -> str:
        """Enhanced filename sanitization with caching"""
        if not text:
            return ""
        
        # Check cache first
        if text in self._filename_cache:
            return self._filename_cache[text]
        
        original_text = text
        text = str(text).strip()
        
        # Use regex for more efficient replacements
        text = re.sub(r'[/\\:*?"<>|\n\r\t]', '-', text)
        text = re.sub(r'\s+', ' ', text)  # Multiple spaces to single
        text = re.sub(r'-+', '-', text)   # Multiple dashes to single
        text = text.strip(' -')
        
        # Limit length intelligently
        if len(text) > 100:
            text = text[:97] + "..."
        
        # Cache the result
        self._filename_cache[original_text] = text
        return text
    
    def discover_opf_files(self):
        """Discover all metadata.opf files in the library"""
        self.log("Discovering OPF metadata files...")
        opf_files = []
        
        try:
            for opf_file in self.library_path.rglob('metadata.opf'):
                opf_files.append(opf_file)
            
            self.log(f"Found {len(opf_files)} OPF metadata files")
            return opf_files
            
        except Exception as e:
            self.log(f"Error discovering OPF files: {e}", 'error')
            return []
    
    def parse_opf_metadata(self, opf_file: Path) -> dict:
        """Parse metadata from OPF file with enhanced series detection"""
        try:
            with open(opf_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            root = ET.fromstring(content)
            
            # Define namespaces
            namespaces = {
                'dc': 'http://purl.org/dc/elements/1.1/',
                'opf': 'http://www.idpf.org/2007/opf',
                'calibre': 'http://calibre.kovidgoyal.net/2009/metadata'
            }
            
            metadata = {}
            
            # Extract basic metadata
            title_elem = root.find('.//dc:title', namespaces)
            if title_elem is not None and title_elem.text:
                metadata['title'] = title_elem.text.strip()
            
            # Extract authors
            authors = []
            for author_elem in root.findall('.//dc:creator', namespaces):
                if author_elem.text:
                    authors.append(author_elem.text.strip())
            metadata['authors'] = ' & '.join(authors) if authors else 'Unknown Author'
            
            # Extract main series from calibre:series meta
            series_metas = root.findall('.//opf:meta[@name="calibre:series"]', namespaces)
            for meta in series_metas:
                series_name = meta.get('content')
                if series_name:
                    metadata['series_name'] = series_name
                    break
            
            # Extract series index
            index_metas = root.findall('.//opf:meta[@name="calibre:series_index"]', namespaces)
            for meta in index_metas:
                series_index = meta.get('content')
                if series_index:
                    try:
                        metadata['series_index'] = float(series_index)
                    except ValueError:
                        pass
                    break
            
            # Extract custom series (series1, series2, etc.)
            custom_series = {}
            for i in range(1, 5):
                series_key = f'calibre:series{i}'
                custom_metas = root.findall(f'.//opf:meta[@name="{series_key}"]', namespaces)
                for meta in custom_metas:
                    custom_value = meta.get('content')
                    if custom_value:
                        custom_series[f'series{i}'] = custom_value
                        break
            
            metadata['custom_series'] = custom_series
            
            # Extract path info from parent directory
            book_dir = opf_file.parent
            relative_path = book_dir.relative_to(self.library_path)
            metadata['path'] = str(relative_path)
            metadata['current_path'] = book_dir
            
            # Generate a pseudo book ID based on path
            metadata['id'] = str(relative_path).replace('/', '_').replace('\\', '_')
            
            return metadata
            
        except Exception as e:
            self.log(f"Error parsing OPF file {opf_file}: {e}", 'error')
            return {}
    
    def get_book_metadata(self):
        """Get book metadata from database or OPF files"""
        if self.use_database:
            return self.get_database_metadata()
        else:
            return self.get_opf_metadata()
    
    def get_database_metadata(self):
        """Get metadata from Calibre database"""
        books = []
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            query = """
            SELECT 
                b.id, b.title, b.path, b.timestamp,
                GROUP_CONCAT(a.name, ' & ') as authors,
                s.name as series_name, b.series_index,
                GROUP_CONCAT(DISTINCT d.format) as formats
            FROM books b
            LEFT JOIN books_authors_link bal ON b.id = bal.book
            LEFT JOIN authors a ON bal.author = a.id
            LEFT JOIN books_series_link bsl ON b.id = bsl.book
            LEFT JOIN series s ON bsl.series = s.id
            LEFT JOIN data d ON b.id = d.book
            GROUP BY b.id
            ORDER BY a.name, b.title
            """
            
            cursor.execute(query)
            rows = cursor.fetchall()
            
            for row in rows:
                book_id, title, path, timestamp, authors, series_name, series_index, formats = row
                custom_series = self.get_custom_series(cursor, book_id)
                
                book_data = {
                    'id': book_id,
                    'title': title or 'Unknown Title',
                    'path': path,
                    'timestamp': timestamp,
                    'authors': authors or 'Unknown Author',
                    'series_name': series_name,
                    'series_index': series_index,
                    'formats': formats.split(',') if formats else [],
                    'custom_series': custom_series,
                    'current_path': self.library_path / path
                }
                
                # Try to enhance with OPF data if metadata looks poor
                if self.needs_metadata_enhancement(book_data):
                    enhanced_data = self.enhance_from_opf(book_data)
                    if enhanced_data:
                        book_data.update(enhanced_data)
                
                books.append(book_data)
            
            conn.close()
            self.log(f"Loaded {len(books)} books from database")
            return books
            
        except sqlite3.Error as e:
            self.log(f"Database error: {e}", 'error')
            # Fall back to OPF files
            return self.get_opf_metadata()
    
    def get_opf_metadata(self):
        """Get metadata from OPF files"""
        books = []
        opf_files = self.discover_opf_files()
        
        for opf_file in opf_files:
            metadata = self.parse_opf_metadata(opf_file)
            if metadata:
                # Enhance with AI parsing if needed
                if self.needs_metadata_enhancement(metadata):
                    enhanced_data = self.ai_parse_from_filename(metadata)
                    if enhanced_data:
                        metadata.update(enhanced_data)
                
                books.append(metadata)
        
        self.log(f"Loaded {len(books)} books from OPF files")
        return books
    
    def enhance_from_opf(self, book_data):
        """Enhance database metadata with OPF file data"""
        try:
            book_dir = book_data['current_path']
            opf_file = book_dir / 'metadata.opf'
            
            if opf_file.exists():
                opf_metadata = self.parse_opf_metadata(opf_file)
                return opf_metadata
            
        except Exception as e:
            self.log(f"Error enhancing from OPF: {e}", 'error')
        
        return {}
    
    def ai_parse_from_filename(self, book_data):
        """AI-enhanced parsing from directory/file names"""
        try:
            path = book_data.get('path', '')
            current_path = book_data.get('current_path')
            
            if not path and current_path:
                path = current_path.name
            
            enhanced_data = {}
            
            # Enhanced regex patterns for parsing
            patterns = [
                # Author - Title - Series Name Book Number
                r'^([^-]+)\s*-\s*([^-]+)\s*-\s*(.+?)\s+(?:Book|Vol|Volume|#)\s*(\d+)',
                # Author - Title - Series Name - Number
                r'^([^-]+)\s*-\s*([^-]+)\s*-\s*([^-]+)\s*-\s*(\d+)',
                # Author - Series Name Number - Title
                r'^([^-]+)\s*-\s*(.+?)\s*(\d+)\s*-\s*([^-]+)',
                # Author - Title (Series Name #Number)
                r'^([^-]+)\s*-\s*([^\(]+)\s*\(([^#]+)#(\d+)\)',
                # Simple Author - Title
                r'^([^-]+)\s*-\s*(.+)',
            ]
            
            for pattern in patterns:
                match = re.match(pattern, path, re.IGNORECASE)
                if match:
                    groups = match.groups()
                    
                    if len(groups) >= 2:
                        enhanced_data['authors'] = groups[0].strip()
                        enhanced_data['title'] = groups[1].strip()
                    
                    if len(groups) >= 4:
                        enhanced_data['series_name'] = groups[2].strip()
                        try:
                            enhanced_data['series_index'] = float(groups[3])
                        except ValueError:
                            pass
                    
                    break
            
            # Try to extract additional series from remaining path parts
            if enhanced_data:
                remaining_parts = path.split('-')[3:]  # Skip author, title, first series
                custom_series = {}
                
                for i, part in enumerate(remaining_parts[:4], 1):
                    part = part.strip()
                    if part and not part.isdigit():
                        custom_series[f'series{i}'] = part
                
                if custom_series:
                    enhanced_data['custom_series'] = custom_series
            
            return enhanced_data
            
        except Exception as e:
            self.log(f"Error in AI parsing: {e}", 'error')
            return {}
    
    def get_custom_series(self, cursor, book_id):
        """Get custom series from database"""
        custom_series = {}
        try:
            cursor.execute("""
                SELECT name, label, display 
                FROM custom_columns 
                WHERE datatype = 'series'
                ORDER BY label
            """)
            
            custom_columns = cursor.fetchall()
            
            for name, label, display in custom_columns:
                table_name = f"custom_column_{name.split('_')[-1]}"
                
                try:
                    cursor.execute(f"""
                        SELECT value 
                        FROM {table_name} 
                        WHERE book = ?
                    """, (book_id,))
                    
                    result = cursor.fetchone()
                    if result and result[0]:
                        custom_series[label] = str(result[0])
                        
                except sqlite3.Error:
                    continue
            
            return custom_series
            
        except sqlite3.Error:
            return {}
    
    def generate_new_filename(self, book):
        """Generate new filename with enhanced format"""
        author = self.sanitize_filename(book['authors'].split(' & ')[0])
        title = self.sanitize_filename(book['title'])
        
        # Build parts list: author-title-series-seriesnumber-series2-series2number-series3-series3number-series4-series4number
        parts = [author, title]
        
        # Main series with space between name and number
        if book.get('series_name') and book['series_name'].strip():
            series_name = self.sanitize_filename(book['series_name'])
            
            # Add series number with space (not as separate part)
            if book.get('series_index'):
                idx = float(book['series_index'])
                if idx.is_integer():
                    series_number = f"{int(idx):02d}"
                else:
                    series_number = f"{idx:05.1f}".replace('.', '_')
                # Combine series name and number with space
                series_with_number = f"{series_name} {series_number}"
                parts.append(series_with_number)
            else:
                parts.append(series_name)
        
        # Custom series with separate names and numbers
        custom_series = book.get('custom_series', {})
        for i in range(1, 5):
            series_key = f"series{i}"
            if (series_key in custom_series and 
                custom_series[series_key] and 
                custom_series[series_key].strip()):
                
                custom_series_value = custom_series[series_key]
                
                # Try to extract series name and number from custom series
                # Check for "Series Name [number]" format
                bracket_match = re.match(r'^(.+?)\s*\[(\d+(?:\.\d+)?)\]\s*$', custom_series_value)
                if bracket_match:
                    series_name = self.sanitize_filename(bracket_match.group(1).strip())
                    series_number = bracket_match.group(2)
                    if '.' in series_number:
                        series_number = f"{float(series_number):05.1f}".replace('.', '_')
                    else:
                        series_number = f"{int(series_number):02d}"
                    parts.extend([series_name, series_number])
                    continue
                
                # Check for "Series Name number" format (number at end)
                number_match = re.match(r'^(.+?)\s+(\d+(?:\.\d+)?)\s*$', custom_series_value)
                if number_match:
                    series_name = self.sanitize_filename(number_match.group(1).strip())
                    series_number = number_match.group(2)
                    if '.' in series_number:
                        series_number = f"{float(series_number):05.1f}".replace('.', '_')
                    else:
                        series_number = f"{int(series_number):02d}"
                    parts.extend([series_name, series_number])
                    continue
                
                # If no number pattern found, just add the series name
                series_name = self.sanitize_filename(custom_series_value)
                parts.append(series_name)
        
        # Filter out empty parts
        filename_parts = [part for part in parts if part and part.strip()]
        
        new_filename = '-'.join(filename_parts)
        new_filename = re.sub(r'-+', '-', new_filename)
        new_filename = new_filename.strip('-')
        
        return new_filename
    
    def needs_metadata_enhancement(self, book):
        """Check if book metadata needs enhancement"""
        poor_metadata_indicators = [
            not book.get('series_name'),  # No main series
            book.get('title', '') in ['Unknown Title', '', None],  # Poor title
            book.get('authors', '') in ['Unknown Author', '', None],  # Poor author
            len(book.get('custom_series', {})) == 0,  # No custom series
            # Check if title looks like a filename rather than a proper title
            book.get('title', '') and ('_' in book['title'] or book['title'].lower().endswith('.epub')),
            # Check if author looks like it was parsed from filename
            book.get('authors', '') and ('-' in book['authors'] and len(book['authors'].split()) == 1)
        ]
        
        return any(poor_metadata_indicators)
    
    def ai_detect_duplicates(self, books):
        """AI-powered duplicate detection using fuzzy matching"""
        duplicates = []
        processed = set()
        
        for i, book1 in enumerate(books):
            if i in processed:
                continue
                
            book1_sig = self.create_book_signature(book1)
            duplicate_group = [book1]
            
            for j, book2 in enumerate(books[i+1:], i+1):
                if j in processed:
                    continue
                    
                book2_sig = self.create_book_signature(book2)
                similarity = self.calculate_similarity(book1_sig, book2_sig)
                
                if similarity > 0.85:  # 85% similarity threshold
                    duplicate_group.append(book2)
                    processed.add(j)
            
            if len(duplicate_group) > 1:
                duplicates.append(duplicate_group)
                processed.add(i)
        
        return duplicates
    
    def create_book_signature(self, book):
        """Create a normalized signature for duplicate detection"""
        title = re.sub(r'[^a-zA-Z0-9]', '', book.get('title', '').lower())
        author = re.sub(r'[^a-zA-Z0-9]', '', book.get('authors', '').split('&')[0].lower())
        series = re.sub(r'[^a-zA-Z0-9]', '', book.get('series_name', '').lower())
        
        return {
            'title': title,
            'author': author,
            'series': series,
            'title_words': set(title.split()) if title else set(),
            'author_words': set(author.split()) if author else set()
        }
    
    def calculate_similarity(self, sig1, sig2):
        """Calculate similarity between two book signatures"""
        # Title similarity (weighted heavily)
        title_sim = self.jaccard_similarity(sig1['title_words'], sig2['title_words'])
        
        # Author similarity
        author_sim = self.jaccard_similarity(sig1['author_words'], sig2['author_words'])
        
        # String similarity for exact matches
        title_str_sim = self.string_similarity(sig1['title'], sig2['title'])
        author_str_sim = self.string_similarity(sig1['author'], sig2['author'])
        
        # Weighted average
        similarity = (title_sim * 0.4 + author_sim * 0.3 + title_str_sim * 0.2 + author_str_sim * 0.1)
        
        return similarity
    
    def jaccard_similarity(self, set1, set2):
        """Calculate Jaccard similarity between two sets"""
        if not set1 and not set2:
            return 1.0
        if not set1 or not set2:
            return 0.0
        
        intersection = len(set1.intersection(set2))
        union = len(set1.union(set2))
        
        return intersection / union if union > 0 else 0.0
    
    def string_similarity(self, str1, str2):
        """Calculate string similarity using Levenshtein distance"""
        if not str1 and not str2:
            return 1.0
        if not str1 or not str2:
            return 0.0
        
        # Simple implementation of normalized Levenshtein distance
        def levenshtein(s1, s2):
            if len(s1) < len(s2):
                return levenshtein(s2, s1)
            
            if len(s2) == 0:
                return len(s1)
            
            previous_row = list(range(len(s2) + 1))
            for i, c1 in enumerate(s1):
                current_row = [i + 1]
                for j, c2 in enumerate(s2):
                    insertions = previous_row[j + 1] + 1
                    deletions = current_row[j] + 1
                    substitutions = previous_row[j] + (c1 != c2)
                    current_row.append(min(insertions, deletions, substitutions))
                previous_row = current_row
            
            return previous_row[-1]
        
        max_len = max(len(str1), len(str2))
        distance = levenshtein(str1, str2)
        
        return 1 - (distance / max_len) if max_len > 0 else 1.0
    
    def ai_extract_series_from_title(self, title):
        """AI-powered series extraction from book titles"""
        series_patterns = [
            # Series Name: Book Title
            r'^([^:]+):\s*(.+)$',
            # Book Title (Series Name #Number)
            r'^(.+?)\s*\(([^#)]+)\s*#\s*(\d+(?:\.\d+)?)\)\s*$',
            # Book Title (Series Name Book Number)
            r'^(.+?)\s*\(([^)]+)\s+(?:Book|Vol|Volume)\s+(\d+(?:\.\d+)?)\)\s*$',
            # Series Name Book Number: Book Title
            r'^([^:]+)\s+(?:Book|Vol|Volume)\s+(\d+(?:\.\d+)?):?\s*(.+)$',
            # Series Name #Number: Book Title
            r'^([^:]+)\s*#(\d+(?:\.\d+)?):?\s*(.+)$',
        ]
        
        for pattern in series_patterns:
            match = re.match(pattern, title.strip(), re.IGNORECASE)
            if match:
                groups = match.groups()
                
                if len(groups) == 2:  # Series: Title format
                    return {
                        'series_name': groups[0].strip(),
                        'title': groups[1].strip(),
                        'series_index': None
                    }
                elif len(groups) == 3:  # Title (Series #Number) or Series Number: Title
                    if ':' in title:  # Series Number: Title format
                        return {
                            'series_name': groups[0].strip(),
                            'title': groups[2].strip(),
                            'series_index': float(groups[1]) if groups[1] else None
                        }
                    else:  # Title (Series #Number) format
                        return {
                            'series_name': groups[1].strip(),
                            'title': groups[0].strip(),
                            'series_index': float(groups[2]) if groups[2] else None
                        }
        
        return {'title': title, 'series_name': None, 'series_index': None}
    
    def ai_smart_title_cleanup(self, title):
        """AI-powered title cleanup and normalization"""
        if not title:
            return title
        
        # Remove common file extensions
        title = re.sub(r'\.(epub|pdf|mobi|azw3?|txt|doc|docx)$', '', title, flags=re.IGNORECASE)
        
        # Remove version numbers and brackets
        title = re.sub(r'\s*\[.*?\]\s*', ' ', title)
        title = re.sub(r'\s*\((?:v\d+|version\s*\d+|final|complete|unabridged)\)\s*', ' ', title, flags=re.IGNORECASE)
        
        # Remove publishing markers
        title = re.sub(r'\s*-\s*(?:retail|published|final|complete|unabridged)\s*$', '', title, flags=re.IGNORECASE)
        
        # Clean up spacing
        title = re.sub(r'\s+', ' ', title).strip()
        
        # Fix common title case issues
        articles = ['the', 'a', 'an', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by']
        words = title.split()
        
        if words:
            # Always capitalize first word
            words[0] = words[0].capitalize()
            
            for i in range(1, len(words)):
                if words[i].lower() not in articles or len(words[i]) > 3:
                    words[i] = words[i].capitalize()
                else:
                    words[i] = words[i].lower()
        
        return ' '.join(words)
    
    def ai_author_name_cleanup(self, author):
        """AI-powered author name cleanup and standardization"""
        if not author:
            return author
        
        # Handle multiple authors
        authors = re.split(r'[&,;]|\band\b', author)
        cleaned_authors = []
        
        for auth in authors:
            auth = auth.strip()
            if not auth:
                continue
            
            # Remove titles and suffixes
            auth = re.sub(r'\b(?:Dr|Mr|Ms|Mrs|Prof|Professor)\.?\s+', '', auth, flags=re.IGNORECASE)
            auth = re.sub(r'\s+(?:Jr|Sr|III?|IV|Ph\.?D|M\.?D)\.?$', '', auth, flags=re.IGNORECASE)
            
            # Fix name order if needed (Last, First -> First Last)
            if ',' in auth and len(auth.split(',')) == 2:
                last, first = auth.split(',', 1)
                auth = f"{first.strip()} {last.strip()}"
            
            # Capitalize properly
            words = auth.split()
            capitalized = []
            
            for word in words:
                if len(word) > 1:
                    # Handle special cases like O'Connor, MacDonald
                    if "'" in word:
                        parts = word.split("'")
                        word = "'".join([p.capitalize() for p in parts])
                    elif word.lower().startswith('mac') and len(word) > 3:
                        word = 'Mac' + word[3:].capitalize()
                    elif word.lower().startswith('mc') and len(word) > 2:
                        word = 'Mc' + word[2:].capitalize()
                    else:
                        word = word.capitalize()
                
                capitalized.append(word)
            
            auth = ' '.join(capitalized)
            cleaned_authors.append(auth)
        
        return ' & '.join(cleaned_authors)
    
    def ai_quality_score(self, book):
        """AI-powered quality scoring for book metadata (0-100)"""
        score = 0
        max_score = 100
        
        # Title quality (25 points)
        title = book.get('title', '')
        if title and title not in ['Unknown Title', '']:
            score += 15
            # Bonus for well-formatted title
            if not re.search(r'[_\d]{3,}', title):  # No long digit/underscore sequences
                score += 5
            if len(title.split()) >= 2:  # Multi-word title
                score += 3
            if title[0].isupper():  # Proper capitalization
                score += 2
        
        # Author quality (20 points)
        authors = book.get('authors', '')
        if authors and authors not in ['Unknown Author', '']:
            score += 12
            # Bonus for proper author format
            if ' & ' in authors or ',' in authors:  # Multiple authors properly formatted
                score += 3
            if not re.search(r'[_\d-]', authors):  # No weird characters
                score += 3
            if len(authors.split()) >= 2:  # First and last name
                score += 2
        
        # Series quality (25 points)
        series_name = book.get('series_name', '')
        if series_name:
            score += 15
            series_index = book.get('series_index')
            if series_index is not None:
                score += 5
            if not re.search(r'[_\d]{3,}', series_name):  # Well-formatted series name
                score += 3
            if len(series_name.split()) >= 2:  # Multi-word series
                score += 2
        
        # Custom series bonus (15 points)
        custom_series = book.get('custom_series', {})
        if custom_series:
            score += min(len(custom_series) * 3, 10)  # Up to 10 points for multiple series
            # Bonus for well-formatted custom series
            well_formatted = sum(1 for v in custom_series.values() 
                               if v and not re.search(r'[_\d]{3,}', str(v)))
            score += min(well_formatted * 2, 5)
        
        # File format bonus (10 points)
        formats = book.get('formats', [])
        if formats:
            score += min(len(formats) * 2, 8)  # Multiple formats
            if any(fmt.upper() in ['EPUB', 'PDF', 'MOBI'] for fmt in formats):
                score += 2  # Common formats
        
        # Metadata completeness bonus (5 points)
        if book.get('timestamp'):  # Has timestamp
            score += 2
        if book.get('path') and not re.search(r'\d{3,}', book['path']):  # Clean path
            score += 3
        
        return min(score, max_score)
    
    def ai_detect_genre(self, book):
        """AI-powered genre detection based on title, series, and metadata"""
        title = book.get('title', '').lower()
        series = book.get('series_name', '').lower()
        author = book.get('authors', '').lower()
        
        # Combine all text for analysis
        text = f"{title} {series} {author}"
        
        # Genre detection patterns
        genre_patterns = {
            'Science Fiction': [
                r'\b(space|galaxy|alien|robot|cyber|tech|future|star|ship|planet|mars|lunar|android|ai|artificial intelligence)\b',
                r'\b(trek|wars|fiction|scifi|sf|enterprise|federation|empire|republic)\b'
            ],
            'Fantasy': [
                r'\b(dragon|magic|wizard|witch|elf|dwarf|orc|sword|quest|kingdom|realm|throne|crown)\b',
                r'\b(fantasy|epic|legend|myth|chronicles|saga|tales|enchanted|magical)\b'
            ],
            'Romance': [
                r'\b(love|heart|passion|desire|romance|bride|wedding|kiss|affair|dating)\b',
                r'\b(romantic|lover|beloved|sweetheart|courtship|marriage|husband|wife)\b'
            ],
            'Mystery': [
                r'\b(murder|detective|crime|police|investigation|clue|suspect|mystery|thriller)\b',
                r'\b(noir|procedural|whodunit|sleuth|inspector|sergeant|case|evidence)\b'
            ],
            'Horror': [
                r'\b(vampire|zombie|ghost|demon|evil|death|blood|dark|nightmare|terror)\b',
                r'\b(horror|gothic|supernatural|paranormal|occult|haunted|cursed)\b'
            ],
            'Historical': [
                r'\b(war|civil|world|ancient|medieval|victorian|regency|empire|colonial)\b',
                r'\b(historical|period|era|century|revolution|battle|king|queen|dynasty)\b'
            ],
            'Biography': [
                r'\b(life|biography|memoir|autobiography|story of|journey|legacy)\b',
                r'\b(biography|biographical|memoirs|personal|real|true)\b'
            ],
            'Young Adult': [
                r'\b(teen|young|school|college|coming|age|youth|adolescent)\b',
                r'\b(ya|young adult|teenage|high school|academy|student)\b'
            ]
        }
        
        detected_genres = []
        confidence_scores = {}
        
        for genre, patterns in genre_patterns.items():
            score = 0
            for pattern in patterns:
                matches = len(re.findall(pattern, text, re.IGNORECASE))
                score += matches
            
            if score > 0:
                confidence_scores[genre] = score
                detected_genres.append((genre, score))
        
        # Sort by confidence score
        detected_genres.sort(key=lambda x: x[1], reverse=True)
        
        # Return top 3 genres with confidence
        result = []
        for genre, score in detected_genres[:3]:
            confidence = min(score * 20, 100)  # Convert to percentage
            result.append({'genre': genre, 'confidence': confidence})
        
        return result
    
    def ai_suggest_improvements(self, book):
        """AI-powered suggestions for improving book metadata"""
        suggestions = []
        quality_score = self.ai_quality_score(book)
        
        title = book.get('title', '')
        authors = book.get('authors', '')
        series_name = book.get('series_name', '')
        
        # Title improvements
        if not title or title in ['Unknown Title', '']:
            suggestions.append({
                'type': 'critical',
                'category': 'Title',
                'issue': 'Missing or invalid title',
                'suggestion': 'Extract title from filename or search online databases'
            })
        elif re.search(r'[_\d]{3,}', title):
            suggestions.append({
                'type': 'warning',
                'category': 'Title',
                'issue': 'Title contains filename artifacts',
                'suggestion': 'Clean up title by removing underscores and number sequences'
            })
        
        # Author improvements
        if not authors or authors in ['Unknown Author', '']:
            suggestions.append({
                'type': 'critical',
                'category': 'Author',
                'issue': 'Missing or invalid author',
                'suggestion': 'Extract author from filename or search online databases'
            })
        elif re.search(r'[_-]', authors) and len(authors.split()) == 1:
            suggestions.append({
                'type': 'warning',
                'category': 'Author',
                'issue': 'Author name appears to be from filename',
                'suggestion': 'Format author name properly (First Last)'
            })
        
        # Series improvements
        if not series_name:
            # Try to detect if this might be part of a series
            if re.search(r'\b(book|vol|volume|#)\s*\d+\b', title, re.IGNORECASE):
                suggestions.append({
                    'type': 'info',
                    'category': 'Series',
                    'issue': 'Possible series book without series metadata',
                    'suggestion': 'Extract series information from title or search online'
                })
        
        # Custom series suggestions
        custom_series = book.get('custom_series', {})
        if len(custom_series) == 0 and series_name:
            suggestions.append({
                'type': 'info',
                'category': 'Organization',
                'issue': 'No custom series for better organization',
                'suggestion': 'Consider adding genre or sub-series classifications'
            })
        
        # Format suggestions
        formats = book.get('formats', [])
        if not formats:
            suggestions.append({
                'type': 'warning',
                'category': 'Files',
                'issue': 'No book formats detected',
                'suggestion': 'Check if book files exist in the directory'
            })
        elif len(formats) == 1 and formats[0].upper() not in ['EPUB', 'PDF']:
            suggestions.append({
                'type': 'info',
                'category': 'Files',
                'issue': 'Uncommon or single format',
                'suggestion': 'Consider converting to EPUB or PDF for better compatibility'
            })
        
        # Overall quality assessment
        if quality_score < 50:
            suggestions.append({
                'type': 'critical',
                'category': 'Overall',
                'issue': f'Low metadata quality score ({quality_score}/100)',
                'suggestion': 'Consider using external metadata lookup to improve information'
            })
        elif quality_score < 75:
            suggestions.append({
                'type': 'warning',
                'category': 'Overall',
                'issue': f'Moderate metadata quality score ({quality_score}/100)',
                'suggestion': 'Some metadata fields could be improved'
            })
        
        return suggestions
    
    def ai_smart_series_grouping(self, books):
        """AI-powered series grouping and organization suggestions"""
        series_groups = {}
        author_series = {}
        
        # Group books by series and author
        for book in books:
            author = book.get('authors', 'Unknown Author').split(' & ')[0]
            series = book.get('series_name', '')
            
            if series:
                key = f"{author}::{series}"
                if key not in series_groups:
                    series_groups[key] = []
                series_groups[key].append(book)
            
            # Track author's series
            if author not in author_series:
                author_series[author] = set()
            if series:
                author_series[author].add(series)
        
        suggestions = []
        
        # Analyze series completeness
        for key, books_in_series in series_groups.items():
            author, series = key.split('::', 1)
            indices = []
            
            for book in books_in_series:
                idx = book.get('series_index')
                if idx is not None:
                    indices.append(float(idx))
            
            if indices:
                indices.sort()
                missing_indices = []
                
                # Check for gaps in series
                for i in range(1, int(max(indices)) + 1):
                    if i not in indices:
                        missing_indices.append(i)
                
                if missing_indices:
                    suggestions.append({
                        'type': 'info',
                        'category': 'Series Completeness',
                        'series': series,
                        'author': author,
                        'issue': f'Missing books in series: {missing_indices}',
                        'suggestion': 'Search for missing books in this series'
                    })
        
        # Suggest potential series connections
        for author, series_set in author_series.items():
            if len(series_set) > 1:
                # Check for similar series names that might be the same
                series_list = list(series_set)
                for i, series1 in enumerate(series_list):
                    for series2 in series_list[i+1:]:
                        similarity = self.string_similarity(series1.lower(), series2.lower())
                        if similarity > 0.7:  # 70% similar
                            suggestions.append({
                                'type': 'warning',
                                'category': 'Series Duplication',
                                'author': author,
                                'issue': f'Similar series names: "{series1}" and "{series2}"',
                                'suggestion': 'Check if these are the same series with different naming'
                            })
        
        return suggestions
    
    def ai_content_analysis(self, book):
        """AI-powered content analysis from book files"""
        analysis_results = {
            'reading_level': None,
            'word_count': None,
            'language': None,
            'themes': [],
            'keywords': [],
            'content_warnings': []
        }
        
        try:
            # Try to read book content for analysis
            current_path = book.get('current_path')
            if not current_path:
                return analysis_results
            
            # Look for text files or try to extract from EPUB
            content_text = self.extract_book_content(current_path)
            if content_text:
                analysis_results.update(self.analyze_text_content(content_text))
            
            return analysis_results
            
        except Exception as e:
            self.log(f"Content analysis error: {e}", 'error')
            return analysis_results
    
    def extract_book_content(self, book_path):
        """Extract readable content from book files"""
        try:
            # Look for EPUB files first
            epub_files = list(book_path.glob('*.epub'))
            if epub_files:
                return self.extract_epub_content(epub_files[0])
            
            # Look for text files
            txt_files = list(book_path.glob('*.txt'))
            if txt_files:
                with open(txt_files[0], 'r', encoding='utf-8', errors='ignore') as f:
                    return f.read()[:10000]  # First 10k characters
            
            return None
            
        except Exception:
            return None
    
    def extract_epub_content(self, epub_path):
        """Extract text content from EPUB file"""
        try:
            import zipfile
            
            with zipfile.ZipFile(epub_path, 'r') as epub:
                # Look for HTML/XHTML files
                content_files = [f for f in epub.namelist() 
                               if f.endswith(('.html', '.xhtml', '.htm'))]
                
                text_content = ""
                for content_file in content_files[:3]:  # First 3 content files
                    try:
                        with epub.open(content_file) as f:
                            html_content = f.read().decode('utf-8', errors='ignore')
                            # Basic HTML tag removal
                            text = re.sub(r'<[^>]+>', ' ', html_content)
                            text = re.sub(r'\s+', ' ', text).strip()
                            text_content += text + " "
                            
                            if len(text_content) > 10000:  # Limit content
                                break
                    except:
                        continue
                
                return text_content[:10000] if text_content else None
                
        except Exception:
            return None
    
    def analyze_text_content(self, text):
        """Analyze extracted text content"""
        analysis = {
            'reading_level': self.estimate_reading_level(text),
            'word_count': len(text.split()),
            'language': self.detect_language(text),
            'themes': self.extract_themes(text),
            'keywords': self.extract_keywords(text),
            'content_warnings': self.detect_content_warnings(text)
        }
        
        return analysis
    
    def estimate_reading_level(self, text):
        """Estimate reading level using simple metrics"""
        words = text.split()
        sentences = text.count('.') + text.count('!') + text.count('?')
        
        if not words or not sentences:
            return "Unknown"
        
        # Simple reading level estimation
        avg_words_per_sentence = len(words) / sentences
        avg_syllables_per_word = sum(self.count_syllables(word) for word in words[:100]) / min(100, len(words))
        
        # Flesch Reading Ease approximation
        score = 206.835 - (1.015 * avg_words_per_sentence) - (84.6 * avg_syllables_per_word)
        
        if score >= 90:
            return "Elementary"
        elif score >= 80:
            return "Middle School"
        elif score >= 70:
            return "High School"
        elif score >= 60:
            return "College"
        else:
            return "Graduate"
    
    def count_syllables(self, word):
        """Count syllables in a word (simplified)"""
        word = word.lower()
        vowels = 'aeiouy'
        syllable_count = 0
        prev_was_vowel = False
        
        for char in word:
            is_vowel = char in vowels
            if is_vowel and not prev_was_vowel:
                syllable_count += 1
            prev_was_vowel = is_vowel
        
        # Handle silent 'e'
        if word.endswith('e') and syllable_count > 1:
            syllable_count -= 1
        
        return max(1, syllable_count)
    
    def detect_language(self, text):
        """Simple language detection"""
        # Common words in different languages
        language_indicators = {
            'English': ['the', 'and', 'that', 'have', 'for', 'not', 'with', 'you', 'this', 'but'],
            'Spanish': ['que', 'una', 'con', 'para', 'por', 'como', 'más', 'sus', 'año', 'son'],
            'French': ['que', 'les', 'une', 'sur', 'avec', 'son', 'ses', 'aux', 'par', 'ont'],
            'German': ['der', 'und', 'die', 'von', 'den', 'das', 'mit', 'sich', 'auf', 'für'],
            'Italian': ['che', 'con', 'una', 'per', 'più', 'sua', 'suo', 'sono', 'dal', 'come']
        }
        
        text_words = set(word.lower() for word in text.split()[:500])  # First 500 words
        
        best_match = "Unknown"
        best_score = 0
        
        for language, indicators in language_indicators.items():
            score = len(set(indicators) & text_words)
            if score > best_score:
                best_score = score
                best_match = language
        
        return best_match if best_score > 2 else "Unknown"
    
    def extract_themes(self, text):
        """Extract main themes from text"""
        theme_keywords = {
            'Romance': ['love', 'heart', 'kiss', 'romance', 'relationship', 'wedding', 'marriage'],
            'Adventure': ['journey', 'quest', 'adventure', 'explore', 'travel', 'discover'],
            'Mystery': ['mystery', 'secret', 'investigate', 'clue', 'detective', 'solve'],
            'Fantasy': ['magic', 'wizard', 'dragon', 'kingdom', 'spell', 'enchanted'],
            'Science Fiction': ['space', 'future', 'technology', 'alien', 'robot', 'galaxy'],
            'Horror': ['fear', 'dark', 'nightmare', 'terror', 'ghost', 'death'],
            'War': ['war', 'battle', 'soldier', 'fight', 'army', 'conflict'],
            'Family': ['family', 'mother', 'father', 'children', 'home', 'parent'],
            'Coming of Age': ['young', 'grow', 'learn', 'school', 'teenager', 'adult']
        }
        
        text_lower = text.lower()
        detected_themes = []
        
        for theme, keywords in theme_keywords.items():
            score = sum(text_lower.count(keyword) for keyword in keywords)
            if score > 2:  # Threshold for theme detection
                detected_themes.append({'theme': theme, 'confidence': min(score * 10, 100)})
        
        return sorted(detected_themes, key=lambda x: x['confidence'], reverse=True)[:3]
    
    def extract_keywords(self, text):
        """Extract important keywords from text"""
        # Common stop words to ignore
        stop_words = set([
            'the', 'and', 'that', 'have', 'for', 'not', 'with', 'you', 'this', 'but',
            'his', 'from', 'they', 'she', 'her', 'been', 'than', 'its', 'were', 'said',
            'each', 'which', 'their', 'time', 'will', 'about', 'would', 'there', 'could',
            'other', 'after', 'first', 'well', 'water', 'very', 'what', 'know', 'while',
            'new', 'take', 'came', 'them', 'way', 'made', 'may', 'part', 'over', 'such'
        ])
        
        # Extract words and count frequency
        words = re.findall(r'\b[a-zA-Z]{4,}\b', text.lower())  # Words 4+ characters
        word_freq = {}
        
        for word in words:
            if word not in stop_words:
                word_freq[word] = word_freq.get(word, 0) + 1
        
        # Get top keywords
        top_keywords = sorted(word_freq.items(), key=lambda x: x[1], reverse=True)[:10]
        return [{'keyword': word, 'frequency': freq} for word, freq in top_keywords]
    
    def detect_content_warnings(self, text):
        """Detect potential content warnings"""
        warning_keywords = {
            'Violence': ['violence', 'violent', 'blood', 'kill', 'murder', 'death', 'weapon', 'fight'],
            'Adult Content': ['sexual', 'explicit', 'adult', 'mature', 'intimate'],
            'Language': ['profanity', 'curse', 'swear', 'explicit language'],
            'Substance Use': ['drug', 'alcohol', 'drinking', 'addiction', 'substance'],
            'Mental Health': ['depression', 'suicide', 'self-harm', 'anxiety', 'trauma']
        }
        
        text_lower = text.lower()
        warnings = []
        
        for warning_type, keywords in warning_keywords.items():
            if any(keyword in text_lower for keyword in keywords):
                warnings.append(warning_type)
        
        return warnings
    
    def scan_and_fix(self, progress_callback=None):
        """Main scanning function with enhanced processing and batch optimization"""
        try:
            self.log("Starting enhanced library scan...")
            
            books = self.get_book_metadata()
            if not books:
                raise Exception("No books found or database error")
            
            changes_made = 0
            errors = 0
            total_books = len(books)
            self.changes_made = []  # Reset changes list
            batch_size = 50  # Process in batches for better performance
            
            self.log(f"Processing {total_books} books in batches of {batch_size}...")
            if progress_callback:
                progress_callback(f"Processing {total_books} books...")
            
            # Process books in batches for better memory management
            for batch_start in range(0, total_books, batch_size):
                batch_end = min(batch_start + batch_size, total_books)
                batch_books = books[batch_start:batch_end]
                
                if progress_callback:
                    progress_callback(f"Processing batch {batch_start//batch_size + 1} ({batch_start + 1}-{batch_end}/{total_books})")
                
                # Process this batch
                batch_changes, batch_errors = self.process_book_batch(batch_books, batch_start)
                changes_made += batch_changes
                errors += batch_errors
                
                # Clear filename cache periodically to prevent memory buildup
                if len(self._filename_cache) > 1000:
                    self._filename_cache.clear()
            
            for i, book in enumerate(books):
                if progress_callback and i % 25 == 0:  # Update every 25 books for smoother UI
                    progress_callback(f"Processing book {i+1}/{total_books}")
                
                try:
                    # Get current directory info
                    current_dir = book.get('current_path')
                    if isinstance(current_dir, str):
                        current_dir = Path(current_dir)
                    
                    current_dir_name = current_dir.name if current_dir and current_dir.exists() else book.get('path', 'Unknown')
                    
                    # Generate new filename
                    new_filename = self.generate_new_filename(book)
                    
                    # Check if filename needs to change
                    needs_change = new_filename != current_dir_name
                    needs_enhancement = self.needs_metadata_enhancement(book)
                    
                    # Log detailed info for first few books
                    if i < 5:
                        self.log(f"Book {i+1}: '{current_dir_name}' -> '{new_filename}' | Change: {needs_change} | Enhancement: {needs_enhancement}")
                    
                    # Include books that need changes or enhancement
                    if needs_change or needs_enhancement:
                        change_data = {
                            'book_id': book.get('id', 'Unknown'),
                            'old_name': current_dir_name,
                            'new_name': new_filename,
                            'title': book.get('title', 'Unknown Title'),
                            'author': book.get('authors', 'Unknown Author').split(' & ')[0],
                            'authors': book.get('authors', 'Unknown Author'),
                            'series_name': book.get('series_name', ''),
                            'series_index': book.get('series_index', ''),
                            'custom_series': book.get('custom_series', {}),
                            'formats': book.get('formats', []),
                            'current_path': str(current_dir) if current_dir else '',
                            'needs_enhancement': needs_enhancement,
                            'status': 'Needs Enhancement' if needs_enhancement and not needs_change else 'Ready',
                            'change_type': 'filename_change' if needs_change else 'metadata_enhancement'
                        }
                        
                        if not self.dry_run and needs_change:
                            # Actually apply the change
                            if current_dir and current_dir.exists():
                                new_dir_path = current_dir.parent / new_filename
                                
                                if new_dir_path.exists():
                                    change_data['status'] = 'Error: Target exists'
                                    errors += 1
                                    self.log(f"Error: Target directory already exists: {new_filename}", 'error')
                                else:
                                    try:
                                        current_dir.rename(new_dir_path)
                                        if self.use_database:
                                            self.update_database_path(book['id'], new_filename)
                                        change_data['status'] = 'Applied'
                                        self.log(f"Renamed: {current_dir_name} -> {new_filename}")
                                    except OSError as e:
                                        change_data['status'] = f'Error: {e}'
                                        errors += 1
                                        self.log(f"Error renaming {current_dir_name}: {e}", 'error')
                        
                        self.changes_made.append(change_data)
                        changes_made += 1
                
                except Exception as e:
                    # Add error entry for debugging
                    error_data = {
                        'book_id': book.get('id', 'Unknown'),
                        'old_name': book.get('path', 'Unknown'),
                        'new_name': 'Error processing',
                        'title': book.get('title', 'Unknown'),
                        'author': 'Error',
                        'status': f'Processing Error: {str(e)}',
                        'needs_enhancement': False,
                        'change_type': 'error'
                    }
                    self.changes_made.append(error_data)
                    errors += 1
                    self.log(f"Error processing book {book.get('title', 'Unknown')}: {str(e)}", 'error')
            
            # Final progress update
            self.log(f"Scan complete: {changes_made} changes found, {errors} errors")
            if progress_callback:
                progress_callback(f"Scan complete: {changes_made} changes found, {errors} errors")
            
            summary = {
                'books_processed': len(books),
                'changes_needed': changes_made,
                'changes_made': len([c for c in self.changes_made if c.get('status') == 'Applied']) if not self.dry_run else 0,
                'errors': errors,
                'dry_run': self.dry_run,
                'changes_list': self.changes_made
            }
            
            return summary
            
        except Exception as e:
            self.log(f"Scan failed: {str(e)}", 'error')
            raise Exception(f"Scan failed: {str(e)}")
    
    def lookup_external_metadata(self, author, title):
        """Lookup metadata from external sources (Google Books, Open Library, Barnes & Noble, etc.)"""
        enhanced_data = {}
        
        try:
            # Google Books API lookup
            google_data = self.lookup_google_books(author, title)
            if google_data:
                enhanced_data.update(google_data)
            
            # Open Library lookup
            if not enhanced_data.get('title') or not enhanced_data.get('authors'):
                openlibrary_data = self.lookup_open_library(author, title)
                if openlibrary_data:
                    enhanced_data.update(openlibrary_data)
            
            # Barnes & Noble lookup
            if not enhanced_data.get('title') or not enhanced_data.get('authors'):
                bn_data = self.lookup_barnes_noble(author, title)
                if bn_data:
                    enhanced_data.update(bn_data)
            
            # Goodreads lookup
            if not enhanced_data.get('title') or not enhanced_data.get('authors'):
                goodreads_data = self.lookup_goodreads(author, title)
                if goodreads_data:
                    enhanced_data.update(goodreads_data)
            
            # Baen Books lookup
            if not enhanced_data.get('title') or not enhanced_data.get('authors'):
                baen_data = self.lookup_baen(author, title)
                if baen_data:
                    enhanced_data.update(baen_data)
            
            # Fantastic Fiction lookup
            if not enhanced_data.get('title') or not enhanced_data.get('authors'):
                ff_data = self.lookup_fantastic_fiction(author, title)
                if ff_data:
                    enhanced_data.update(ff_data)
            
            return enhanced_data
            
        except Exception as e:
            self.log(f"Error in external lookup: {e}", 'error')
            return {}
    
    def lookup_google_books(self, author, title):
        """Lookup book metadata from Google Books API"""
        try:
            query = f"inauthor:{author} intitle:{title}"
            url = f"https://www.googleapis.com/books/v1/volumes?q={query}&maxResults=1"
            
            response = requests.get(url, timeout=10)
            if response.status_code == 200:
                data = response.json()
                if 'items' in data and len(data['items']) > 0:
                    book_info = data['items'][0]['volumeInfo']
                    
                    enhanced_data = {}
                    if 'title' in book_info:
                        enhanced_data['title'] = book_info['title']
                    if 'authors' in book_info:
                        enhanced_data['authors'] = ' & '.join(book_info['authors'])
                    
                    return enhanced_data
        
        except Exception as e:
            self.log(f"Google Books lookup error: {e}", 'error')
        
        return {}
    
    def lookup_open_library(self, author, title):
        """Lookup book metadata from Open Library API"""
        try:
            # Open Library search
            query = f"author:{author} title:{title}"
            url = f"https://openlibrary.org/search.json?q={query}&limit=1"
            
            response = requests.get(url, timeout=10)
            if response.status_code == 200:
                data = response.json()
                if 'docs' in data and len(data['docs']) > 0:
                    book_info = data['docs'][0]
                    
                    enhanced_data = {}
                    if 'title' in book_info:
                        enhanced_data['title'] = book_info['title']
                    if 'author_name' in book_info:
                        enhanced_data['authors'] = ' & '.join(book_info['author_name'])
                    
                    return enhanced_data
        
        except Exception as e:
            self.log(f"Open Library lookup error: {e}", 'error')
        
        return {}
    
    def lookup_barnes_noble(self, author, title):
        """Lookup book metadata from Barnes & Noble (web scraping)"""
        try:
            import urllib.parse
            
            query = f"{author} {title}"
            encoded_query = urllib.parse.quote(query)
            url = f"https://www.barnesandnoble.com/s/{encoded_query}"
            
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
            }
            
            response = requests.get(url, headers=headers, timeout=10)
            if response.status_code == 200:
                # Basic parsing - would need BeautifulSoup for full implementation
                content = response.text
                if title.lower() in content.lower() and author.lower() in content.lower():
                    # Found matching content - return basic confirmation
                    return {'title': title, 'authors': author}
        
        except Exception as e:
            self.log(f"Barnes & Noble lookup error: {e}", 'error')
        
        return {}
    
    def lookup_goodreads(self, author, title):
        """Lookup book metadata from Goodreads (web scraping)"""
        try:
            import urllib.parse
            
            query = f"{title} {author}"
            encoded_query = urllib.parse.quote(query)
            url = f"https://www.goodreads.com/search?q={encoded_query}"
            
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
            }
            
            response = requests.get(url, headers=headers, timeout=10)
            if response.status_code == 200:
                # Basic parsing - would need BeautifulSoup for full implementation
                content = response.text
                if title.lower() in content.lower() and author.lower() in content.lower():
                    # Found matching content - return basic confirmation
                    return {'title': title, 'authors': author}
        
        except Exception as e:
            self.log(f"Goodreads lookup error: {e}", 'error')
        
        return {}
    
    def lookup_baen(self, author, title):
        """Lookup book metadata from Baen Books"""
        try:
            import urllib.parse
            
            query = f"{title} {author}"
            encoded_query = urllib.parse.quote(query)
            url = f"https://www.baen.com/allbooks?searched={encoded_query}"
            
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
            }
            
            response = requests.get(url, headers=headers, timeout=10)
            if response.status_code == 200:
                # Basic parsing - would need BeautifulSoup for full implementation
                content = response.text
                if title.lower() in content.lower() and author.lower() in content.lower():
                    # Found matching content - return basic confirmation
                    return {'title': title, 'authors': author}
        
        except Exception as e:
            self.log(f"Baen lookup error: {e}", 'error')
        
        return {}
    
    def lookup_fantastic_fiction(self, author, title):
        """Lookup book metadata from Fantastic Fiction"""
        try:
            import urllib.parse
            
            # Fantastic Fiction uses author-based searches
            author_search = urllib.parse.quote(author.replace(' ', '+'))
            url = f"https://www.fantasticfiction.com/search/?searchfor=author&keywords={author_search}"
            
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
            }
            
            response = requests.get(url, headers=headers, timeout=10)
            if response.status_code == 200:
                # Basic parsing - would need BeautifulSoup for full implementation
                content = response.text
                if title.lower() in content.lower() and author.lower() in content.lower():
                    # Found matching content - return basic confirmation
                    return {'title': title, 'authors': author}
        
        except Exception as e:
            self.log(f"Fantastic Fiction lookup error: {e}", 'error')
        
        return {}
    
    def update_database_path(self, book_id, new_path):
        """Update database path for renamed book"""
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            cursor.execute("""
                UPDATE books 
                SET path = ? 
                WHERE id = ?
            """, (new_path, book_id))
            
            conn.commit()
            conn.close()
            
        except sqlite3.Error as e:
            self.log(f"Database update error: {e}", 'error')

def find_calibre_library():
    common_paths = [
        os.path.expanduser("~/Calibre Library"),
        os.path.expanduser("~/Documents/Calibre Library"),
        os.path.expanduser("~/Books"),
        os.path.expanduser("~/Documents/Books"),
    ]
    
    for path in common_paths:
        if os.path.exists(os.path.join(path, 'metadata.db')):
            return path
    
    return None


class WorkerThread(QThread):
    """Worker thread for library scanning to keep GUI responsive"""
    
    progress_update = pyqtSignal(str)
    status_update = pyqtSignal(str)
    finished_scan = pyqtSignal(dict)
    error_occurred = pyqtSignal(str)
    
    def __init__(self, library_path: str, dry_run: bool = True):
        super().__init__()
        self.library_path = library_path
        self.dry_run = dry_run
        self.fixer = None
        
    def run(self):
        """Run the library scanning in background thread"""
        try:
            self.status_update.emit("Initializing Calibre library scanner...")
            self.fixer = CalibreLibraryFixer(self.library_path, self.dry_run)
            
            self.status_update.emit("Scanning library and analyzing filenames...")
            self.progress_update.emit("Starting scan...")
            
            # Run the actual scan
            summary = self.fixer.scan_and_fix()
            
            self.status_update.emit("Scan completed")
            self.finished_scan.emit(summary)
            
        except Exception as e:
            self.error_occurred.emit(str(e))


class LibraryPreviewWidget(QWidget):
    """Widget to preview library changes"""
    
    def __init__(self):
        super().__init__()
        self.changes_data = []
        self.init_ui()
        
    def init_ui(self):
        layout = QVBoxLayout()
        
        # Header with title and controls
        header_layout = QHBoxLayout()
        
        title = QLabel("📋 Library Preview")
        title.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        header_layout.addWidget(title)
        
        header_layout.addStretch()
        
        # Buttons for bulk operations
        self.select_all_button = QPushButton("✅ Select All")
        self.select_all_button.clicked.connect(self.select_all_changes)
        header_layout.addWidget(self.select_all_button)
        
        self.deselect_all_button = QPushButton("❌ Deselect All")
        self.deselect_all_button.clicked.connect(self.deselect_all_changes)
        header_layout.addWidget(self.deselect_all_button)
        
        self.reset_button = QPushButton("🔄 Reset Names")
        self.reset_button.clicked.connect(self.reset_filename_changes)
        header_layout.addWidget(self.reset_button)
        
        layout.addLayout(header_layout)
        
        # Info label
        self.info_label = QLabel("Double-click on 'New Name' cells to edit filenames")
        self.info_label.setStyleSheet("color: #666; font-style: italic; margin: 5px;")
        layout.addWidget(self.info_label)
        
        # Table for showing changes
        self.changes_table = QTableWidget()
        self.changes_table.setColumnCount(6)
        self.changes_table.setHorizontalHeaderLabels([
            "Apply", "Book Title", "Author", "Current Name", "New Name", "Status"
        ])
        
        # Make table responsive with manual resizing enabled
        header = self.changes_table.horizontalHeader()
        header.setSectionResizeMode(QHeaderView.ResizeMode.Interactive)  # Allow manual resizing
        header.setStretchLastSection(True)  # Stretch the last column
        
        # Set initial column widths
        self.changes_table.setColumnWidth(0, 60)   # Apply checkbox
        self.changes_table.setColumnWidth(1, 200)  # Title
        self.changes_table.setColumnWidth(2, 150)  # Author
        self.changes_table.setColumnWidth(3, 250)  # Current Name
        self.changes_table.setColumnWidth(4, 250)  # New Name
        # Status column will stretch
        
        # Enable editing for the "New Name" column
        self.changes_table.setEditTriggers(QTableWidget.EditTrigger.DoubleClicked)
        self.changes_table.itemChanged.connect(self.on_item_changed)
        
        # Style the table with better contrast
        self.changes_table.setStyleSheet("""
            QTableWidget {
                gridline-color: #333;
                background-color: #2b2b2b;
                alternate-background-color: #3b3b3b;
                color: #ffffff;
                border: 1px solid #555;
            }
            QTableWidget::item {
                padding: 8px;
                color: #ffffff;
                border-bottom: 1px solid #444;
            }
            QTableWidget::item:selected {
                background-color: #0d7377;
                color: #ffffff;
            }
            QHeaderView::section {
                background-color: #1e1e1e;
                padding: 8px;
                border: 1px solid #555;
                font-weight: bold;
                color: #ffffff;
            }
        """)
        
        self.changes_table.setAlternatingRowColors(True)
        layout.addWidget(self.changes_table)
        
        # Summary section
        summary_layout = QHBoxLayout()
        self.summary_label = QLabel("No changes to preview")
        self.summary_label.setStyleSheet("font-weight: bold; color: #fff; margin: 10px;")
        summary_layout.addWidget(self.summary_label)
        
        summary_layout.addStretch()
        
        # AI enhancement button
        self.ai_enhance_button = QPushButton("🧠 AI Enhance Names")
        self.ai_enhance_button.setStyleSheet("""
            QPushButton {
                background-color: #9C27B0;
                color: white;
                border: none;
                border-radius: 5px;
                font-size: 12px;
                font-weight: bold;
                padding: 8px;
            }
            QPushButton:hover {
                background-color: #7B1FA2;
            }
            QPushButton:disabled {
                background-color: #cccccc;
            }
        """)
        self.ai_enhance_button.clicked.connect(self.ai_enhance_filenames)
        summary_layout.addWidget(self.ai_enhance_button)
        
        # Export button
        self.export_button = QPushButton("📄 Export Preview")
        self.export_button.clicked.connect(self.export_preview)
        summary_layout.addWidget(self.export_button)
        
        # COMMIT CHANGES BUTTON (This is the actual apply button)
        self.commit_button = QPushButton("💾 COMMIT CHANGES")
        self.commit_button.setStyleSheet("""
            QPushButton {
                background-color: #FF5722;
                color: white;
                border: none;
                border-radius: 5px;
                font-size: 14px;
                font-weight: bold;
                padding: 10px;
            }
            QPushButton:hover {
                background-color: #E64A19;
            }
            QPushButton:disabled {
                background-color: #cccccc;
            }
        """)
        self.commit_button.clicked.connect(self.commit_selected_changes)
        summary_layout.addWidget(self.commit_button)
        
        layout.addLayout(summary_layout)
        self.setLayout(layout)
    
    def update_preview(self, changes: list):
        """Update the preview table with changes"""
        self.changes_data = changes.copy() if changes else []
        self.changes_table.setRowCount(len(self.changes_data))
        
        for row, change in enumerate(self.changes_data):
            # Apply checkbox
            apply_checkbox = QCheckBox()
            apply_checkbox.setChecked(True)
            apply_checkbox.stateChanged.connect(self.update_summary)
            self.changes_table.setCellWidget(row, 0, apply_checkbox)
            
            # Book info (read-only)
            title_item = QTableWidgetItem(change.get('title', 'Unknown Title'))
            title_item.setFlags(title_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.changes_table.setItem(row, 1, title_item)
            
            author_item = QTableWidgetItem(change.get('author', 'Unknown Author'))
            author_item.setFlags(author_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.changes_table.setItem(row, 2, author_item)
            
            # Current name (read-only)
            current_item = QTableWidgetItem(change.get('old_name', ''))
            current_item.setFlags(current_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            current_item.setBackground(QColor(255, 248, 248))  # Light red background
            self.changes_table.setItem(row, 3, current_item)
            
            # New name (editable)
            new_item = QTableWidgetItem(change.get('new_name', ''))
            new_item.setBackground(QColor(248, 255, 248))  # Light green background
            new_item.setToolTip("Double-click to edit this filename")
            self.changes_table.setItem(row, 4, new_item)
            
            # Status
            status_item = QTableWidgetItem(change.get('status', 'Ready'))
            status_item.setFlags(status_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.changes_table.setItem(row, 5, status_item)
        
        self.update_summary()
    
    def on_item_changed(self, item):
        """Handle when a table item is changed"""
        if item.column() == 4:  # New Name column
            row = item.row()
            new_filename = item.text().strip()
            
            # Validate filename
            if self.validate_filename(new_filename):
                # Update our data
                if row < len(self.changes_data):
                    self.changes_data[row]['new_name'] = new_filename
                    self.changes_data[row]['status'] = 'Modified'
                    
                    # Update status column
                    status_item = self.changes_table.item(row, 5)
                    if status_item:
                        status_item.setText('Modified')
                        status_item.setBackground(QColor(255, 248, 192))  # Light yellow
                
                item.setBackground(QColor(248, 255, 248))  # Light green
            else:
                # Invalid filename - revert or show error
                item.setBackground(QColor(255, 220, 220))  # Light red
                item.setToolTip("Invalid filename! Contains illegal characters.")
        
        self.update_summary()
    
    def validate_filename(self, filename: str) -> bool:
        """Validate if filename is acceptable"""
        if not filename or not filename.strip():
            return False
        
        # Check for illegal characters
        illegal_chars = ['/', '\\', ':', '*', '?', '"', '<', '>', '|']
        return not any(char in filename for char in illegal_chars)
    
    def select_all_changes(self):
        """Select all changes for application"""
        for row in range(self.changes_table.rowCount()):
            checkbox = self.changes_table.cellWidget(row, 0)
            if checkbox:
                checkbox.setChecked(True)
    
    def deselect_all_changes(self):
        """Deselect all changes"""
        for row in range(self.changes_table.rowCount()):
            checkbox = self.changes_table.cellWidget(row, 0)
            if checkbox:
                checkbox.setChecked(False)
    
    def reset_filename_changes(self):
        """Reset all filename changes to original generated names"""
        reply = QMessageBox.question(
            self,
            "Reset Filenames",
            "Are you sure you want to reset all filename changes to the original generated names?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            # Re-update with original data (this will reset any manual changes)
            self.update_preview(self.changes_data)
    
    def update_summary(self):
        """Update the summary label"""
        total = self.changes_table.rowCount()
        selected = 0
        
        for row in range(total):
            checkbox = self.changes_table.cellWidget(row, 0)
            if checkbox and checkbox.isChecked():
                selected += 1
        
        if total == 0:
            self.summary_label.setText("No changes to preview")
        else:
            self.summary_label.setText(f"📊 {selected} of {total} changes selected for application")
    
    def export_preview(self):
        """Export preview to CSV file"""
        if not self.changes_data:
            QMessageBox.information(self, "No Data", "No preview data to export.")
            return
        
        filename, _ = QFileDialog.getSaveFileName(
            self,
            "Export Preview",
            "calibre_preview.csv",
            "CSV Files (*.csv);;All Files (*)"
        )
        
        if filename:
            try:
                import csv
                with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerow(['Book ID', 'Title', 'Author', 'Current Name', 'New Name', 'Status', 'Apply'])
                    
                    for row in range(self.changes_table.rowCount()):
                        checkbox = self.changes_table.cellWidget(row, 0)
                        apply_status = 'Yes' if checkbox and checkbox.isChecked() else 'No'
                        
                        row_data = [
                            self.changes_data[row].get('book_id', ''),
                            self.changes_table.item(row, 1).text() if self.changes_table.item(row, 1) else '',
                            self.changes_table.item(row, 2).text() if self.changes_table.item(row, 2) else '',
                            self.changes_table.item(row, 3).text() if self.changes_table.item(row, 3) else '',
                            self.changes_table.item(row, 4).text() if self.changes_table.item(row, 4) else '',
                            self.changes_table.item(row, 5).text() if self.changes_table.item(row, 5) else '',
                            apply_status
                        ]
                        writer.writerow(row_data)
                
                QMessageBox.information(self, "Export Complete", f"Preview exported to:\n{filename}")
            except Exception as e:
                QMessageBox.critical(self, "Export Error", f"Could not export preview:\n{e}")
    
    def get_selected_changes(self):
        """Get list of changes that are selected for application"""
        selected_changes = []
        
        for row in range(self.changes_table.rowCount()):
            checkbox = self.changes_table.cellWidget(row, 0)
            if checkbox and checkbox.isChecked():
                # Get the current filename from the table (may have been edited)
                new_name_item = self.changes_table.item(row, 4)
                if new_name_item and row < len(self.changes_data):
                    change_data = self.changes_data[row].copy()
                    change_data['new_name'] = new_name_item.text()
                    selected_changes.append(change_data)
        
        return selected_changes
    
    def ai_enhance_filenames(self):
        """Use AI to enhance filename suggestions"""
        if not self.changes_data:
            QMessageBox.information(self, "No Data", "No filename data to enhance.")
            return
        
        # Show AI enhancement dialog
        dialog = AIEnhancementDialog(self.changes_data, self)
        if dialog.exec() == dialog.DialogCode.Accepted:
            enhanced_changes = dialog.get_enhanced_changes()
            if enhanced_changes:
                self.update_preview(enhanced_changes)
                QMessageBox.information(self, "Enhancement Complete", f"Enhanced {len(enhanced_changes)} filenames using AI suggestions.")
    
    def commit_selected_changes(self):
        """Commit the selected changes (this is the main apply button)"""
        selected_changes = self.get_selected_changes()
        
        if not selected_changes:
            QMessageBox.information(self, "No Changes Selected", "Please select at least one change to commit.")
            return
        
        # Get parent window (main GUI) and trigger apply changes
        parent_window = self.parent()
        while parent_window and not hasattr(parent_window, 'apply_selected_changes'):
            parent_window = parent_window.parent()
        
        if parent_window:
            parent_window.apply_selected_changes(selected_changes)


class SettingsWidget(QWidget):
    """Widget for application settings"""
    
    def __init__(self):
        super().__init__()
        self.settings = QSettings('CalibreFixer', 'GUI')
        self.init_ui()
        self.load_settings()
        
    def init_ui(self):
        # Create scroll area for better layout
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        
        scroll_widget = QWidget()
        scroll_widget.setStyleSheet("""
            QWidget {
                background-color: #fafafa;
            }
            QGroupBox {
                background-color: white;
                border: 1px solid #ddd;
                border-radius: 8px;
                margin-top: 1ex;
                padding-top: 15px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 8px 0 8px;
                background-color: white;
                color: #333;
            }
            QLabel {
                color: #555;
                font-weight: normal;
            }
            QLineEdit, QSpinBox, QComboBox {
                padding: 8px;
                border: 1px solid #ccc;
                border-radius: 4px;
                background-color: white;
            }
            QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
                border-color: #2196F3;
                outline: none;
            }
            QCheckBox {
                color: #555;
                font-weight: normal;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
            }
            QCheckBox::indicator:unchecked {
                border: 2px solid #ccc;
                border-radius: 3px;
                background-color: white;
            }
            QCheckBox::indicator:checked {
                border: 2px solid #2196F3;
                border-radius: 3px;
                background-color: #2196F3;
                image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTQiIGhlaWdodD0iMTQiIHZpZXdCb3g9IjAgMCAxNCAxNCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTMgN0w2IDEwTDExIDQiIHN0cm9rZT0id2hpdGUiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIi8+Cjwvc3ZnPgo=);
            }
        """)
        
        layout = QVBoxLayout(scroll_widget)
        layout.setSpacing(20)
        layout.setContentsMargins(20, 20, 20, 20)
        
        # Filename Format Settings
        format_group = QGroupBox("📝 Filename Format Settings")
        format_layout = QGridLayout()
        format_layout.setSpacing(12)
        
        format_layout.addWidget(QLabel("Format:"), 0, 0)
        self.format_label = QLabel("author-title-series-seriesnumber-series2-series2number-series3-series3number-series4-series4number")
        self.format_label.setStyleSheet("""
            font-family: 'Courier New', monospace; 
            background: #f8f8f8; 
            padding: 10px; 
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            color: #333;
        """)
        format_layout.addWidget(self.format_label, 0, 1)
        
        format_layout.addWidget(QLabel("Max filename length:"), 1, 0)
        self.max_length_spin = QSpinBox()
        self.max_length_spin.setRange(50, 255)
        self.max_length_spin.setValue(100)
        format_layout.addWidget(self.max_length_spin, 1, 1)
        
        format_layout.addWidget(QLabel("Series number format:"), 2, 0)
        self.series_format = QComboBox()
        self.series_format.addItems(["01, 02, 03...", "1, 2, 3...", "001, 002, 003..."])
        format_layout.addWidget(self.series_format, 2, 1)
        
        format_group.setLayout(format_layout)
        layout.addWidget(format_group)
        
        # Safety Settings
        safety_group = QGroupBox("🛡️ Safety Settings")
        safety_layout = QVBoxLayout()
        safety_layout.setSpacing(12)
        
        self.backup_check = QCheckBox("Create backup before making changes")
        self.backup_check.setChecked(True)
        safety_layout.addWidget(self.backup_check)
        
        self.confirm_check = QCheckBox("Confirm each change")
        safety_layout.addWidget(self.confirm_check)
        
        self.log_check = QCheckBox("Enable detailed logging")
        self.log_check.setChecked(True)
        safety_layout.addWidget(self.log_check)
        
        safety_group.setLayout(safety_layout)
        layout.addWidget(safety_group)
        
        # Advanced Settings
        advanced_group = QGroupBox("⚙️ Advanced Settings")
        advanced_layout = QGridLayout()
        advanced_layout.setSpacing(12)
        
        advanced_layout.addWidget(QLabel("Custom series columns:"), 0, 0)
        self.custom_series_edit = QLineEdit("series1,series2,series3,series4")
        advanced_layout.addWidget(self.custom_series_edit, 0, 1)
        
        advanced_layout.addWidget(QLabel("Database timeout (s):"), 1, 0)
        self.timeout_spin = QSpinBox()
        self.timeout_spin.setRange(5, 60)
        self.timeout_spin.setValue(30)
        advanced_layout.addWidget(self.timeout_spin, 1, 1)
        
        advanced_group.setLayout(advanced_layout)
        layout.addWidget(advanced_group)
        
        layout.addStretch()
        scroll.setWidget(scroll_widget)
        
        main_layout = QVBoxLayout()
        main_layout.addWidget(scroll)
        self.setLayout(main_layout)
    
    def load_settings(self):
        """Load settings from QSettings"""
        self.max_length_spin.setValue(self.settings.value('max_length', 100, type=int))
        self.series_format.setCurrentText(self.settings.value('series_format', '01, 02, 03...'))
        self.backup_check.setChecked(self.settings.value('backup', True, type=bool))
        self.confirm_check.setChecked(self.settings.value('confirm', False, type=bool))
        self.log_check.setChecked(self.settings.value('logging', True, type=bool))
        self.custom_series_edit.setText(self.settings.value('custom_series', 'series1,series2,series3,series4'))
        self.timeout_spin.setValue(self.settings.value('timeout', 30, type=int))
    
    def save_settings(self):
        """Save settings to QSettings"""
        self.settings.setValue('max_length', self.max_length_spin.value())
        self.settings.setValue('series_format', self.series_format.currentText())
        self.settings.setValue('backup', self.backup_check.isChecked())
        self.settings.setValue('confirm', self.confirm_check.isChecked())
        self.settings.setValue('logging', self.log_check.isChecked())
        self.settings.setValue('custom_series', self.custom_series_edit.text())
        self.settings.setValue('timeout', self.timeout_spin.value())


class CalibreFixerGUI(QMainWindow):
    """Main GUI window for Calibre Library Fixer"""
    
    def __init__(self):
        super().__init__()
        self.settings = QSettings('CalibreFixer', 'GUI')
        self.worker_thread = None
        self.current_summary = None
        
        self.init_ui()
        self.setup_style()
        self.load_window_state()
        
        # Auto-detect library on startup
        QTimer.singleShot(100, self.auto_detect_library)
    
    def init_ui(self):
        """Initialize the user interface"""
        self.setWindowTitle("Calibre Library Fixer v2.0")
        self.setMinimumSize(1000, 700)
        
        # Central widget with tabs
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        
        # Header section
        header_layout = self.create_header()
        main_layout.addLayout(header_layout)
        
        # Tab widget
        self.tab_widget = QTabWidget()
        
        # Main tab
        main_tab = self.create_main_tab()
        self.tab_widget.addTab(main_tab, "📚 Library Scanner")
        
        # Preview tab
        self.preview_widget = LibraryPreviewWidget()
        self.tab_widget.addTab(self.preview_widget, "👁️ Preview Changes")
        
        # Settings tab
        self.settings_widget = SettingsWidget()
        self.tab_widget.addTab(self.settings_widget, "⚙️ Settings")
        
        # Log tab
        log_tab = self.create_log_tab()
        self.tab_widget.addTab(log_tab, "📝 Logs")
        
        main_layout.addWidget(self.tab_widget)
        
        # Status bar
        self.statusBar().showMessage("Ready - Select a Calibre library to begin")
    
    def create_header(self):
        """Create the header section with logo and title"""
        header_layout = QHBoxLayout()
        
        # Create a simple icon
        icon_label = QLabel()
        icon_pixmap = self.create_app_icon()
        icon_label.setPixmap(icon_pixmap)
        header_layout.addWidget(icon_label)
        
        # Title and description
        title_layout = QVBoxLayout()
        
        title = QLabel("Calibre Library Fixer")
        title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title_layout.addWidget(title)
        
        subtitle = QLabel("Organize your ebook library with standardized filenames")
        subtitle.setFont(QFont("Arial", 10))
        subtitle.setStyleSheet("color: #666;")
        title_layout.addWidget(subtitle)
        
        header_layout.addLayout(title_layout)
        header_layout.addStretch()
        
        return header_layout
    
    def create_app_icon(self) -> QPixmap:
        """Create a simple application icon"""
        pixmap = QPixmap(64, 64)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Draw book icon
        painter.setBrush(QBrush(QColor(70, 130, 180)))
        painter.setPen(QPen(QColor(50, 110, 160), 2))
        painter.drawRoundedRect(10, 15, 35, 45, 3, 3)
        
        # Draw pages
        painter.setBrush(QBrush(QColor(255, 255, 255)))
        painter.drawRoundedRect(12, 17, 31, 41, 2, 2)
        
        # Draw lines
        painter.setPen(QPen(QColor(200, 200, 200), 1))
        for y in range(25, 50, 4):
            painter.drawLine(16, y, 39, y)
        
        # Draw gear (settings icon)
        painter.setBrush(QBrush(QColor(255, 165, 0)))
        painter.setPen(QPen(QColor(235, 145, 0), 1))
        painter.drawEllipse(40, 35, 20, 20)
        
        painter.end()
        return pixmap
    
    def create_main_tab(self):
        """Create the main scanner tab"""
        main_widget = QWidget()
        layout = QVBoxLayout(main_widget)
        
        # Library selection section
        library_group = QGroupBox("📁 Calibre Library Selection")
        library_layout = QVBoxLayout()
        
        # Library path
        path_layout = QHBoxLayout()
        path_layout.addWidget(QLabel("Library Path:"))
        
        self.library_path_edit = QLineEdit()
        self.library_path_edit.setPlaceholderText("Select your Calibre library directory...")
        path_layout.addWidget(self.library_path_edit)
        
        self.browse_button = QPushButton("📂 Browse")
        self.browse_button.clicked.connect(self.browse_library)
        path_layout.addWidget(self.browse_button)
        
        self.auto_detect_button = QPushButton("🔍 Auto-Detect")
        self.auto_detect_button.clicked.connect(self.auto_detect_library)
        path_layout.addWidget(self.auto_detect_button)
        
        library_layout.addLayout(path_layout)
        
        # Library info
        self.library_info_label = QLabel("No library selected")
        self.library_info_label.setStyleSheet("color: #666; font-style: italic;")
        library_layout.addWidget(self.library_info_label)
        
        library_group.setLayout(library_layout)
        layout.addWidget(library_group)
        
        # Scan options
        options_group = QGroupBox("🔧 Scan Options")
        options_layout = QHBoxLayout()
        
        self.dry_run_check = QCheckBox("Dry Run (Preview Only)")
        self.dry_run_check.setChecked(True)
        self.dry_run_check.setToolTip("Preview changes without making them")
        options_layout.addWidget(self.dry_run_check)
        
        self.backup_check = QCheckBox("Create Backup")
        self.backup_check.setChecked(True)
        self.backup_check.setToolTip("Create backup before making changes")
        options_layout.addWidget(self.backup_check)
        
        options_layout.addStretch()
        
        options_group.setLayout(options_layout)
        layout.addWidget(options_group)
        
        # Action buttons
        button_layout = QHBoxLayout()
        
        self.scan_button = QPushButton("🔍 Scan Library")
        self.scan_button.setMinimumHeight(40)
        self.scan_button.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 5px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #cccccc;
            }
        """)
        self.scan_button.clicked.connect(self.start_scan)
        button_layout.addWidget(self.scan_button)
        
        self.apply_button = QPushButton("✅ Apply Changes")
        self.apply_button.setMinimumHeight(40)
        self.apply_button.setStyleSheet("""
            QPushButton {
                background-color: #2196F3;
                color: white;
                border: none;
                border-radius: 5px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #1976D2;
            }
            QPushButton:disabled {
                background-color: #cccccc;
            }
        """)
        self.apply_button.clicked.connect(self.apply_changes)
        self.apply_button.setEnabled(False)
        button_layout.addWidget(self.apply_button)
        
        layout.addLayout(button_layout)
        
        # Progress section
        progress_group = QGroupBox("📊 Progress")
        progress_layout = QVBoxLayout()
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        progress_layout.addWidget(self.progress_bar)
        
        self.status_label = QLabel("Ready to scan")
        progress_layout.addWidget(self.status_label)
        
        # Results summary
        self.results_label = QLabel("")
        self.results_label.setStyleSheet("font-weight: bold; color: #333;")
        progress_layout.addWidget(self.results_label)
        
        progress_group.setLayout(progress_layout)
        layout.addWidget(progress_group)
        
        layout.addStretch()
        return main_widget
    
    def create_log_tab(self):
        """Create the log viewing tab"""
        log_widget = QWidget()
        layout = QVBoxLayout(log_widget)
        
        # Log controls
        controls_layout = QHBoxLayout()
        
        clear_button = QPushButton("🗑️ Clear")
        clear_button.clicked.connect(self.clear_logs)
        controls_layout.addWidget(clear_button)
        
        save_button = QPushButton("💾 Save Log")
        save_button.clicked.connect(self.save_log)
        controls_layout.addWidget(save_button)
        
        controls_layout.addStretch()
        
        layout.addLayout(controls_layout)
        
        # Log display
        self.log_display = QTextEdit()
        self.log_display.setReadOnly(True)
        self.log_display.setFont(QFont("Consolas", 9))
        self.log_display.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e1e;
                color: #d4d4d4;
                border: 1px solid #444;
            }
        """)
        layout.addWidget(self.log_display)
        
        return log_widget
    
    def setup_style(self):
        """Setup the application style with better contrast"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QWidget {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #555555;
                border-radius: 8px;
                margin-top: 1ex;
                padding-top: 15px;
                background-color: #3b3b3b;
                color: #ffffff;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 8px 0 8px;
                background-color: #3b3b3b;
                color: #ffffff;
            }
            QLabel {
                color: #ffffff;
                background-color: transparent;
            }
            QLineEdit {
                background-color: #4b4b4b;
                border: 2px solid #666666;
                border-radius: 4px;
                padding: 8px;
                color: #ffffff;
                font-size: 12px;
            }
            QLineEdit:focus {
                border-color: #2196F3;
            }
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 10px;
                font-weight: bold;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #666666;
                color: #999999;
            }
            QCheckBox {
                color: #ffffff;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
            }
            QCheckBox::indicator:unchecked {
                border: 2px solid #666666;
                border-radius: 3px;
                background-color: #4b4b4b;
            }
            QCheckBox::indicator:checked {
                border: 2px solid #2196F3;
                border-radius: 3px;
                background-color: #2196F3;
            }
            QProgressBar {
                border: 2px solid #666666;
                border-radius: 5px;
                background-color: #4b4b4b;
                text-align: center;
                color: #ffffff;
            }
            QProgressBar::chunk {
                background-color: #2196F3;
                border-radius: 3px;
            }
            QTabWidget::pane {
                border: 2px solid #555555;
                background-color: #3b3b3b;
            }
            QTabBar::tab {
                background-color: #4b4b4b;
                color: #ffffff;
                padding: 12px 16px;
                margin-right: 2px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                border: 1px solid #666666;
            }
            QTabBar::tab:selected {
                background-color: #3b3b3b;
                border-bottom: 3px solid #2196F3;
                color: #ffffff;
            }
            QTabBar::tab:hover {
                background-color: #5b5b5b;
            }
            QStatusBar {
                background-color: #1e1e1e;
                color: #ffffff;
                border-top: 1px solid #555555;
            }
        """)
    
    def browse_library(self):
        """Browse for Calibre library directory"""
        directory = QFileDialog.getExistingDirectory(
            self, 
            "Select Calibre Library Directory",
            os.path.expanduser("~")
        )
        
        if directory:
            self.set_library_path(directory)
    
    def auto_detect_library(self):
        """Auto-detect Calibre library"""
        library_path = find_calibre_library()
        if library_path:
            self.set_library_path(library_path)
            self.add_log(f"✅ Auto-detected library: {library_path}")
        else:
            self.add_log("❌ Could not auto-detect Calibre library")
            QMessageBox.information(
                self, 
                "Auto-Detection", 
                "Could not automatically find a Calibre library.\nPlease select one manually."
            )
    
    def set_library_path(self, path: str):
        """Set the library path and validate it"""
        self.library_path_edit.setText(path)
        
        # Validate library
        if self.validate_library(path):
            self.library_info_label.setText(f"✅ Valid Calibre library found")
            self.library_info_label.setStyleSheet("color: green;")
            self.scan_button.setEnabled(True)
            
            # Try to get book count
            try:
                db_path = Path(path) / 'metadata.db'
                conn = sqlite3.connect(str(db_path))
                cursor = conn.cursor()
                cursor.execute("SELECT COUNT(*) FROM books")
                book_count = cursor.fetchone()[0]
                conn.close()
                
                self.library_info_label.setText(f"✅ Valid library with {book_count} books")
            except:
                pass
        else:
            self.library_info_label.setText("❌ Invalid Calibre library")
            self.library_info_label.setStyleSheet("color: red;")
            self.scan_button.setEnabled(False)
    
    def validate_library(self, path: str) -> bool:
        """Validate if path contains a valid Calibre library"""
        library_path = Path(path)
        return (library_path.exists() and 
                (library_path / 'metadata.db').exists())
    
    def start_scan(self):
        """Start the library scan"""
        library_path = self.library_path_edit.text().strip()
        if not library_path or not self.validate_library(library_path):
            QMessageBox.warning(self, "Invalid Library", "Please select a valid Calibre library first.")
            return
        
        # Save current settings
        self.settings_widget.save_settings()
        
        # Setup UI for scanning
        self.scan_button.setEnabled(False)
        self.apply_button.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setRange(0, 0)  # Indeterminate progress
        self.status_label.setText("Scanning library...")
        
        # Start worker thread
        self.worker_thread = WorkerThread(library_path, self.dry_run_check.isChecked())
        self.worker_thread.progress_update.connect(self.add_log)
        self.worker_thread.status_update.connect(self.status_label.setText)
        self.worker_thread.finished_scan.connect(self.scan_finished)
        self.worker_thread.error_occurred.connect(self.scan_error)
        self.worker_thread.start()
        
        self.add_log(f"🔍 Starting scan of library: {library_path}")
        self.add_log(f"Mode: {'DRY RUN' if self.dry_run_check.isChecked() else 'LIVE MODE'}")
    
    def scan_finished(self, summary: Dict):
        """Handle scan completion"""
        self.current_summary = summary
        
        # Update UI
        self.progress_bar.setVisible(False)
        self.scan_button.setEnabled(True)
        
        # Update results
        books_processed = summary.get('books_processed', 0)
        changes_needed = summary.get('changes_needed', 0)
        errors = summary.get('errors', 0)
        
        if changes_needed > 0:
            self.results_label.setText(
                f"📊 Scan Complete: {books_processed} books processed, "
                f"{changes_needed} need renaming, {errors} errors"
            )
            
            if self.dry_run_check.isChecked():
                self.apply_button.setEnabled(True)
                self.status_label.setText(f"Ready to apply {changes_needed} changes")
            else:
                changes_made = summary.get('changes_made', 0)
                self.status_label.setText(f"Applied {changes_made} changes successfully")
        else:
            self.results_label.setText(f"📊 Scan Complete: All {books_processed} books already have correct filenames")
            self.status_label.setText("No changes needed")
        
        # Update preview if we have changes
        changes_list = summary.get('changes_list', [])
        if changes_list:
            self.preview_widget.update_preview(changes_list)
            self.add_log(f"📋 Preview updated with {len(changes_list)} changes")
        else:
            self.add_log("🔍 No changes detected in scan results")
            # Debug: Log some book info to see what's being processed
            if hasattr(self.worker_thread, 'fixer') and self.worker_thread.fixer:
                try:
                    books = self.worker_thread.fixer.get_book_metadata()
                    if books:
                        sample_book = books[0] if books else None
                        if sample_book:
                            current_name = sample_book['path']
                            new_name = self.worker_thread.fixer.generate_new_filename(sample_book)
                            self.add_log(f"🔍 Sample comparison: '{current_name}' vs '{new_name}'")
                            self.add_log(f"🔍 Sample book data: {sample_book['title']} by {sample_book['authors']}")
                except Exception as e:
                    self.add_log(f"🔍 Debug error: {e}")
        
        self.add_log(f"✅ Scan completed successfully")
        
        # Switch to preview tab if there are changes
        if changes_needed > 0:
            self.tab_widget.setCurrentIndex(1)
    
    def scan_error(self, error_message: str):
        """Handle scan error"""
        self.progress_bar.setVisible(False)
        self.scan_button.setEnabled(True)
        self.status_label.setText("Scan failed")
        
        self.add_log(f"❌ Scan error: {error_message}")
        QMessageBox.critical(self, "Scan Error", f"An error occurred during scanning:\n\n{error_message}")
    
    def apply_changes(self):
        """Apply the changes from dry run"""
        # Get selected changes from preview widget
        selected_changes = self.preview_widget.get_selected_changes()
        
        if not selected_changes:
            QMessageBox.information(self, "No Changes Selected", "Please select at least one change to apply.")
            return
        
        # Confirm with user
        changes_count = len(selected_changes)
        reply = QMessageBox.question(
            self,
            "Confirm Changes",
            f"Are you sure you want to apply {changes_count} filename changes?\n\n"
            "This action cannot be undone easily. Make sure you have a backup!",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        
        if reply != QMessageBox.StandardButton.Yes:
            return
        
        # Apply selected changes
        self.apply_selected_changes(selected_changes)
    
    def apply_selected_changes(self, changes: list):
        """Apply only the selected changes"""
        library_path = self.library_path_edit.text().strip()
        if not library_path:
            return
        
        # Setup UI for applying changes
        self.apply_button.setEnabled(False)
        self.scan_button.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setRange(0, len(changes))
        self.status_label.setText("Applying changes...")
        
        applied_count = 0
        error_count = 0
        
        try:
            fixer = CalibreLibraryFixer(library_path, dry_run=False)
            
            for i, change in enumerate(changes):
                self.progress_bar.setValue(i)
                self.status_label.setText(f"Applying change {i+1} of {len(changes)}...")
                
                try:
                    # Apply the individual change
                    book_id = change.get('book_id')
                    new_name = change.get('new_name')
                    old_name = change.get('old_name')
                    
                    if book_id and new_name and old_name:
                        current_dir = Path(library_path) / old_name
                        new_dir = Path(library_path) / new_name
                        
                        if current_dir.exists() and not new_dir.exists():
                            # Rename directory
                            current_dir.rename(new_dir)
                            
                            # Update database
                            fixer.update_database_path(book_id, new_name)
                            
                            applied_count += 1
                            self.add_log(f"✅ Renamed: {old_name} → {new_name}")
                        else:
                            error_count += 1
                            self.add_log(f"❌ Could not rename: {old_name} (target exists or source missing)")
                    else:
                        error_count += 1
                        self.add_log(f"❌ Invalid change data for book ID {book_id}")
                        
                except Exception as e:
                    error_count += 1
                    self.add_log(f"❌ Error applying change: {e}")
                
                # Process events to keep UI responsive
                QApplication.processEvents()
            
            # Update UI
            self.progress_bar.setVisible(False)
            self.scan_button.setEnabled(True)
            
            if applied_count > 0:
                self.status_label.setText(f"Applied {applied_count} changes successfully")
                self.results_label.setText(f"📊 Changes Applied: {applied_count} successful, {error_count} errors")
                self.add_log(f"🎉 Successfully applied {applied_count} changes")
                
                # Refresh the library scan to update the preview
                QTimer.singleShot(1000, self.refresh_after_apply)
            else:
                self.status_label.setText("No changes were applied")
                self.apply_button.setEnabled(True)
                
        except Exception as e:
            self.progress_bar.setVisible(False)
            self.scan_button.setEnabled(True)
            self.apply_button.setEnabled(True)
            self.add_log(f"❌ Error during apply operation: {e}")
            QMessageBox.critical(self, "Apply Error", f"An error occurred while applying changes:\n\n{e}")
    
    def refresh_after_apply(self):
        """Refresh the library scan after applying changes"""
        self.dry_run_check.setChecked(True)
        self.start_scan()
    
    def add_log(self, message: str):
        """Add message to log display"""
        from datetime import datetime
        timestamp = f"[{datetime.now().strftime('%H:%M:%S')}] "
        self.log_display.append(timestamp + message)
        
        # Auto-scroll to bottom
        scrollbar = self.log_display.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
    
    def clear_logs(self):
        """Clear the log display"""
        self.log_display.clear()
    
    def save_log(self):
        """Save log to file"""
        filename, _ = QFileDialog.getSaveFileName(
            self, 
            "Save Log File",
            "calibre_fixer_log.txt",
            "Text Files (*.txt);;All Files (*)"
        )
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.log_display.toPlainText())
                self.add_log(f"💾 Log saved to: {filename}")
            except Exception as e:
                QMessageBox.critical(self, "Save Error", f"Could not save log file:\n{e}")
    
    def load_window_state(self):
        """Load window geometry and state"""
        geometry = self.settings.value('geometry')
        if geometry:
            self.restoreGeometry(geometry)
        
        library_path = self.settings.value('last_library_path', '')
        if library_path and self.validate_library(library_path):
            self.set_library_path(library_path)
    
    def closeEvent(self, event):
        """Handle window close event"""
        # Save window state
        self.settings.setValue('geometry', self.saveGeometry())
        self.settings.setValue('last_library_path', self.library_path_edit.text())
        
        # Save settings
        self.settings_widget.save_settings()
        
        # Stop worker thread if running
        if self.worker_thread and self.worker_thread.isRunning():
            self.worker_thread.quit()
            self.worker_thread.wait(3000)  # Wait up to 3 seconds
        
        # Properly close the application
        QApplication.instance().quit()
        event.accept()


class AIEnhancementDialog(QDialog):
    """Dialog for AI-enhanced filename suggestions"""
    
    def __init__(self, changes_data, parent=None):
        super().__init__(parent)
        self.changes_data = changes_data
        self.enhanced_changes = []
        self.init_ui()
        self.enhance_filenames()
    
    def init_ui(self):
        self.setWindowTitle("🧠 AI Filename Enhancement")
        self.setModal(True)
        self.resize(900, 700)
        
        # Set better styling for the dialog
        self.setStyleSheet("""
            QDialog {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QLabel {
                color: #ffffff;
                background-color: transparent;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #555555;
                border-radius: 8px;
                margin-top: 1ex;
                padding-top: 15px;
                background-color: #3b3b3b;
                color: #ffffff;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 8px 0 8px;
                background-color: #3b3b3b;
                color: #ffffff;
            }
            QCheckBox {
                color: #ffffff;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
            }
            QCheckBox::indicator:unchecked {
                border: 2px solid #666666;
                border-radius: 3px;
                background-color: #4b4b4b;
            }
            QCheckBox::indicator:checked {
                border: 2px solid #2196F3;
                border-radius: 3px;
                background-color: #2196F3;
            }
            QTableWidget {
                gridline-color: #555;
                background-color: #2b2b2b;
                alternate-background-color: #3b3b3b;
                color: #ffffff;
                border: 1px solid #555;
            }
            QTableWidget::item {
                padding: 8px;
                color: #ffffff;
                border-bottom: 1px solid #444;
            }
            QTableWidget::item:selected {
                background-color: #0d7377;
                color: #ffffff;
            }
            QHeaderView::section {
                background-color: #1e1e1e;
                padding: 8px;
                border: 1px solid #555;
                font-weight: bold;
                color: #ffffff;
            }
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 10px;
                font-weight: bold;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #666666;
                color: #999999;
            }
        """)
        
        layout = QVBoxLayout()
        
        # Header
        header = QLabel("🧠 AI Filename Enhancement")
        header.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        header.setStyleSheet("color: #ffffff; margin: 10px; font-size: 18px;")
        layout.addWidget(header)
        
        description = QLabel(
            "The AI has analyzed your filenames and suggests improvements based on:\n"
            "• Consistent formatting patterns\n"
            "• Better abbreviations and spacing\n"
            "• Removal of redundant information\n"
            "• Improved readability"
        )
        description.setStyleSheet("""
            color: #cccccc; 
            margin: 10px; 
            background: #3b3b3b; 
            padding: 15px; 
            border-radius: 8px;
            border: 1px solid #555;
            font-size: 12px;
        """)
        layout.addWidget(description)
        
        # Enhancement options
        options_group = QGroupBox("Enhancement Options")
        options_layout = QVBoxLayout()
        
        self.remove_redundant = QCheckBox("Remove redundant words (e.g., 'Book', 'Novel')")
        self.remove_redundant.setChecked(True)
        options_layout.addWidget(self.remove_redundant)
        
        self.improve_spacing = QCheckBox("Improve spacing and capitalization")
        self.improve_spacing.setChecked(True)
        options_layout.addWidget(self.improve_spacing)
        
        self.smart_abbreviations = QCheckBox("Use smart abbreviations for common words")
        self.smart_abbreviations.setChecked(True)
        options_layout.addWidget(self.smart_abbreviations)
        
        self.limit_length = QCheckBox("Intelligently limit filename length")
        self.limit_length.setChecked(True)
        options_layout.addWidget(self.limit_length)
        
        options_group.setLayout(options_layout)
        layout.addWidget(options_group)
        
        # Preview table
        self.preview_table = QTableWidget()
        self.preview_table.setColumnCount(3)
        self.preview_table.setHorizontalHeaderLabels(["Original", "Current", "AI Enhanced"])
        
        # Make table responsive with manual resizing enabled
        header = self.preview_table.horizontalHeader()
        header.setSectionResizeMode(QHeaderView.ResizeMode.Interactive)  # Allow manual resizing
        header.setStretchLastSection(True)  # Stretch the last column
        
        # Set initial column widths
        self.preview_table.setColumnWidth(0, 300)  # Original
        self.preview_table.setColumnWidth(1, 300)  # Current
        # AI Enhanced column will stretch
        
        # Table styling is already set by the main dialog stylesheet
        self.preview_table.setAlternatingRowColors(True)
        
        layout.addWidget(self.preview_table)
        
        # Buttons
        button_box = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        
        # Add refresh button
        self.refresh_button = QPushButton("🔄 Refresh Suggestions")
        self.refresh_button.clicked.connect(self.enhance_filenames)
        button_box.addButton(self.refresh_button, QDialogButtonBox.ButtonRole.ActionRole)
        
        layout.addWidget(button_box)
        
        self.setLayout(layout)
    
    def enhance_filenames(self):
        """Generate AI-enhanced filename suggestions"""
        self.enhanced_changes = []
        
        for change in self.changes_data:
            original_name = change.get('old_name', '')
            current_name = change.get('new_name', '')
            title = change.get('title', '')
            author = change.get('author', '')
            
            # AI enhancement logic
            enhanced_name = self.apply_ai_enhancements(current_name, title, author)
            
            enhanced_change = change.copy()
            enhanced_change['new_name'] = enhanced_name
            enhanced_change['ai_enhanced'] = True
            self.enhanced_changes.append(enhanced_change)
        
        self.update_preview_table()
    
    def apply_ai_enhancements(self, filename, title, author):
        """Apply AI-based enhancements to filename"""
        enhanced = filename
        
        if self.remove_redundant.isChecked():
            enhanced = self.remove_redundant_words(enhanced)
        
        if self.improve_spacing.isChecked():
            enhanced = self.improve_spacing_and_caps(enhanced)
        
        if self.smart_abbreviations.isChecked():
            enhanced = self.apply_smart_abbreviations(enhanced)
        
        if self.limit_length.isChecked():
            enhanced = self.intelligent_length_limit(enhanced)
        
        return enhanced
    
    def remove_redundant_words(self, filename):
        """Remove redundant words from filename"""
        redundant_words = [
            'book', 'novel', 'story', 'tale', 'series', 'volume', 'vol',
            'edition', 'ed', 'the', 'a', 'an', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by'
        ]
        
        parts = filename.split('-')
        filtered_parts = []
        
        for part in parts:
            words = part.lower().split()
            filtered_words = [word for word in words if word not in redundant_words or len(words) <= 2]
            if filtered_words:
                filtered_parts.append(' '.join(filtered_words))
        
        return '-'.join(filtered_parts)
    
    def improve_spacing_and_caps(self, filename):
        """Improve spacing and capitalization"""
        parts = filename.split('-')
        improved_parts = []
        
        for part in parts:
            # Improve capitalization
            words = part.split()
            capitalized_words = []
            
            for word in words:
                if len(word) > 3:
                    capitalized_words.append(word.title())
                else:
                    capitalized_words.append(word.upper() if word.upper() in ['II', 'III', 'IV', 'USA', 'UK'] else word.title())
            
            improved_parts.append(' '.join(capitalized_words))
        
        return '-'.join(improved_parts)
    
    def apply_smart_abbreviations(self, filename):
        """Apply smart abbreviations for common words"""
        abbreviations = {
            'Chronicles': 'Chron',
            'Adventures': 'Adv',
            'Mysteries': 'Myst',
            'Detective': 'Det',
            'Investigation': 'Invest',
            'Collection': 'Coll',
            'Anthology': 'Anth',
            'Complete': 'Comp',
            'Ultimate': 'Ult',
            'Special': 'Spec',
            'Extended': 'Ext',
            'Director': 'Dir',
            'International': 'Intl',
            'Corporation': 'Corp',
            'Company': 'Co',
            'University': 'Univ',
            'Professor': 'Prof',
            'Doctor': 'Dr',
            'Mister': 'Mr',
            'Misses': 'Mrs'
        }
        
        enhanced = filename
        for full_word, abbrev in abbreviations.items():
            enhanced = enhanced.replace(full_word, abbrev)
            enhanced = enhanced.replace(full_word.lower(), abbrev)
        
        return enhanced
    
    def intelligent_length_limit(self, filename, max_length=80):
        """Intelligently limit filename length while preserving important information"""
        if len(filename) <= max_length:
            return filename
        
        parts = filename.split('-')
        if len(parts) < 2:
            return filename[:max_length]
        
        # Prioritize author and title, then series info
        author_part = parts[0] if parts else ''
        title_part = parts[1] if len(parts) > 1 else ''
        series_parts = parts[2:] if len(parts) > 2 else []
        
        # Start with author and title
        essential = f"{author_part}-{title_part}"
        
        # Add series parts if there's room
        for series_part in series_parts:
            test_length = len(essential) + len(series_part) + 1  # +1 for dash
            if test_length <= max_length:
                essential += f"-{series_part}"
            else:
                break
        
        return essential
    
    def update_preview_table(self):
        """Update the preview table with enhancements"""
        self.preview_table.setRowCount(len(self.enhanced_changes))
        
        for row, (original, enhanced) in enumerate(zip(self.changes_data, self.enhanced_changes)):
            # Original name
            original_item = QTableWidgetItem(original.get('old_name', ''))
            original_item.setFlags(original_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            original_item.setBackground(QColor(80, 60, 60))  # Dark red background
            original_item.setForeground(QColor(255, 255, 255))  # White text
            self.preview_table.setItem(row, 0, original_item)
            
            # Current name
            current_item = QTableWidgetItem(original.get('new_name', ''))
            current_item.setFlags(current_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            current_item.setBackground(QColor(60, 70, 80))  # Dark blue background
            current_item.setForeground(QColor(255, 255, 255))  # White text
            self.preview_table.setItem(row, 1, current_item)
            
            # Enhanced name
            enhanced_item = QTableWidgetItem(enhanced.get('new_name', ''))
            enhanced_item.setFlags(enhanced_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            enhanced_item.setBackground(QColor(60, 80, 60))  # Dark green background
            enhanced_item.setForeground(QColor(255, 255, 255))  # White text
            self.preview_table.setItem(row, 2, enhanced_item)
    
    def get_enhanced_changes(self):
        """Get the enhanced changes list"""
        return self.enhanced_changes


def main():
    """Main application entry point"""
    import signal
    
    app = QApplication(sys.argv)
    app.setApplicationName("Calibre Library Fixer")
    app.setApplicationVersion("2.0")
    app.setOrganizationName("LibraryTools")
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        print("\nReceived interrupt signal, closing application...")
        app.quit()
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Make sure Python processes Ctrl+C
    timer = QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)
    
    # Set application icon
    # app.setWindowIcon(QIcon("icon.png"))  # Uncomment if you have an icon file
    
    # Create and show main window
    window = CalibreFixerGUI()
    window.show()
    
    # Start event loop
    try:
        sys.exit(app.exec())
    except KeyboardInterrupt:
        print("\nKeyboard interrupt received, exiting...")
        app.quit()
        sys.exit(0)


if __name__ == "__main__":
    main()
