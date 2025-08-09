#!/usr/bin/env python3
"""
Enhanced Calibre Library Scanner and Filename Fixer
===================================================
Scans Calibre library, reads metadata from .opf files, uses AI parsing and external 
lookups for missing metadata, and fixes filenames according to structure:
author-title-series-series1-series2-series3-series4

Features:
- Reads metadata from Calibre metadata.opf files  
- Handles multiple series (main + custom series)
- AI-powered filename parsing for missing metadata
- External database lookups (Goodreads, Open Library, Google Books)
- Comprehensive logging with named log files
- Dry-run and live modes
- Handles books with missing metadata gracefully

Author: AI Assistant (Enhanced)
Date: 2025-06-20
License: MIT
"""

import os
import sys
import sqlite3
import shutil
import argparse
import re
import json
import logging
import xml.etree.ElementTree as ET
import requests
import time
import hashlib
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from urllib.parse import quote
from difflib import SequenceMatcher
from datetime import datetime

# Create timestamped log filename
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
log_filename = f"calibre_fixer_{timestamp}.log"

# Configure comprehensive logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(funcName)s:%(lineno)d - %(message)s',
    handlers=[
        logging.FileHandler(log_filename),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class BookMetadata:
    """Enhanced class to hold comprehensive book metadata information"""
    
    def __init__(self):
        self.title = ""
        self.authors = []
        self.series = []  # List of dicts: [{"name": str, "index": float, "label": str}]
        self.isbn = ""
        self.publisher = ""
        self.published_date = ""
        self.description = ""
        self.language = "en"
        self.confidence = 0.0  # Confidence score for parsed data
        self.source = "unknown"  # Source of metadata (opf, filename, external)
        self.tags = []
        self.calibre_id = ""
        self.identifiers = {}  # External identifiers

    def to_dict(self):
        """Convert metadata to dictionary for logging"""
        return {
            'title': self.title,
            'authors': self.authors,
            'series': self.series,
            'confidence': self.confidence,
            'source': self.source
        }


class EnhancedMetadataParser:
    """Enhanced class for parsing metadata from various sources"""
    
    def __init__(self, log_operations=True):
        self.goodreads_api_key = None  # Set this if you have a Goodreads API key
        self.google_books_api_key = None  # Set this if you have a Google Books API key
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Calibre-Library-Fixer-Enhanced/2.0 (Linux; Educational)'
        })
        self.log_operations = log_operations
        self.operations_log = []
        
    def log_operation(self, operation: str, details: dict):
        """Log operations for debugging and tracking"""
        if self.log_operations:
            log_entry = {
                'timestamp': datetime.now().isoformat(),
                'operation': operation,
                'details': details
            }
            self.operations_log.append(log_entry)
            logger.debug(f"Operation: {operation} - {details}")
    
    def parse_filename_ai(self, filename: str) -> BookMetadata:
        """
        Enhanced AI-like parsing to extract metadata from filename using multiple patterns
        
        Args:
            filename: The filename to parse
            
        Returns:
            BookMetadata object with parsed information
        """
        metadata = BookMetadata()
        metadata.source = "filename_ai"
        
        self.log_operation("parse_filename_ai", {"filename": filename})
        
        # Remove file extension and common prefixes/suffixes
        clean_name = filename
        if '.' in clean_name:
            clean_name = clean_name.rsplit('.', 1)[0]
        
        # Remove common book ID patterns at the end (like Calibre IDs)
        clean_name = re.sub(r'\s*\(\d+\)$', '', clean_name)
        
        # Advanced patterns for book filenames
        patterns = [
            # Author - Title - Series #N (Subseries #M)
            r'^(.+?)\s*-\s*(.+?)\s*-\s*(.+?)\s*#?(\d+(?:\.\d+)?)\s*\((.+?)\s*#?(\d+(?:\.\d+)?)\)$',
            # Author - Title - Series #N [Year]
            r'^(.+?)\s*-\s*(.+?)\s*-\s*(.+?)\s*#?(\d+(?:\.\d+)?)\s*\[(\d{4})\]$',
            # Author - Title - Series #N
            r'^(.+?)\s*-\s*(.+?)\s*-\s*(.+?)\s*#?(\d+(?:\.\d+)?)$',
            # Author - Title (Series #N)
            r'^(.+?)\s*-\s*(.+?)\s*\((.+?)\s*#?(\d+(?:\.\d+)?)\)$',
            # Author - Title [Series #N]
            r'^(.+?)\s*-\s*(.+?)\s*\[(.+?)\s*#?(\d+(?:\.\d+)?)\]$',
            # Title - Author - Series #N
            r'^(.+?)\s*-\s*(.+?)\s*-\s*(.+?)\s*#?(\d+(?:\.\d+)?)$',
            # Author - Title
            r'^(.+?)\s*-\s*(.+?)$',
            # Title by Author
            r'^(.+?)\s+by\s+(.+?)$',
            # Author: Title
            r'^(.+?):\s*(.+?)$',
            # Just title
            r'^(.+?)$'
        ]
        
        for i, pattern in enumerate(patterns):
            match = re.match(pattern, clean_name, re.IGNORECASE)
            if match:
                groups = match.groups()
                
                if i == 0:  # Complex pattern with subseries
                    metadata.authors = [groups[0].strip()]
                    metadata.title = groups[1].strip()
                    metadata.series = [
                        {"name": groups[2].strip(), "index": float(groups[3]), "label": "main"},
                        {"name": groups[4].strip(), "index": float(groups[5]), "label": "subseries"}
                    ]
                    metadata.confidence = 0.95
                elif i == 1:  # With year
                    metadata.authors = [groups[0].strip()]
                    metadata.title = groups[1].strip()
                    metadata.series = [{"name": groups[2].strip(), "index": float(groups[3]), "label": "main"}]
                    metadata.published_date = groups[4]
                    metadata.confidence = 0.9
                elif i in [2, 3, 4]:  # Series patterns
                    if i == 4 and not self._looks_like_author(groups[0]):
                        # Title - Author - Series pattern
                        metadata.title = groups[0].strip()
                        metadata.authors = [groups[1].strip()]
                        metadata.series = [{"name": groups[2].strip(), "index": float(groups[3]), "label": "main"}]
                    else:
                        metadata.authors = [groups[0].strip()]
                        metadata.title = groups[1].strip()
                        metadata.series = [{"name": groups[2].strip(), "index": float(groups[3]), "label": "main"}]
                    metadata.confidence = 0.85
                elif i in [6, 7]:  # Author - Title or Title by Author
                    if ' by ' in clean_name.lower():
                        metadata.title = groups[0].strip()
                        metadata.authors = [groups[1].strip()]
                    elif ':' in clean_name:
                        metadata.authors = [groups[0].strip()]
                        metadata.title = groups[1].strip()
                    else:
                        # Use heuristic to determine which is author
                        if self._looks_like_author(groups[0]):
                            metadata.authors = [groups[0].strip()]
                            metadata.title = groups[1].strip()
                        else:
                            metadata.title = groups[0].strip()
                            metadata.authors = [groups[1].strip()]
                    metadata.confidence = 0.7
                else:  # Just title
                    metadata.title = groups[0].strip()
                    metadata.confidence = 0.4
                
                self.log_operation("filename_pattern_matched", {
                    "pattern_index": i,
                    "confidence": metadata.confidence,
                    "parsed_data": metadata.to_dict()
                })
                break
        
        # Post-processing: clean up author and title
        metadata.authors = [self._clean_author_name(author) for author in metadata.authors]
        metadata.title = self._clean_title(metadata.title)
        
        return metadata
    
    def _looks_like_author(self, text: str) -> bool:
        """Enhanced heuristic to determine if text looks like an author name"""
        # Enhanced patterns for author names
        author_patterns = [
            r'^[A-Z][a-z]+\s+[A-Z][a-z]+$',  # First Last
            r'^[A-Z]\.\s*[A-Z]\.\s+[A-Z][a-z]+$',  # A. B. Last
            r'^[A-Z][a-z]+\s+[A-Z]\.\s*[A-Z]?\.\s*[A-Z][a-z]+$',  # First A. B. Last
            r'^[A-Z][a-z]+\s+[A-Z][a-z]+\s+[A-Z][a-z]+$',  # First Middle Last
            r'^[A-Z][a-z]+,\s+[A-Z][a-z]+$',  # Last, First
        ]
        
        for pattern in author_patterns:
            if re.match(pattern, text.strip()):
                return True
        
        # Check for common author indicators
        author_indicators = ['jr.', 'sr.', 'dr.', 'prof.', 'phd']
        text_lower = text.lower()
        if any(indicator in text_lower for indicator in author_indicators):
            return True
            
        return False
    
    def _clean_author_name(self, author: str) -> str:
        """Clean and standardize author name"""
        author = author.strip()
        
        # Handle "Last, First" format
        if ',' in author:
            parts = [part.strip() for part in author.split(',')]
            if len(parts) == 2:
                author = f"{parts[1]} {parts[0]}"
        
        # Remove extra whitespace
        author = re.sub(r'\s+', ' ', author)
        
        return author
    
    def _clean_title(self, title: str) -> str:
        """Clean and standardize title"""
        title = title.strip()
        
        # Remove excessive punctuation
        title = re.sub(r'[.]{2,}', '', title)
        title = re.sub(r'\s+', ' ', title)
        
        return title
    
    def read_metadata_opf_enhanced(self, opf_path: Path) -> BookMetadata:
        """
        Enhanced metadata reading from Calibre metadata.opf file with better series handling
        
        Args:
            opf_path: Path to metadata.opf file
            
        Returns:
            BookMetadata object with parsed information
        """
        metadata = BookMetadata()
        metadata.source = "opf_file"
        
        self.log_operation("read_metadata_opf", {"file": str(opf_path)})
        
        try:
            tree = ET.parse(opf_path)
            root = tree.getroot()
            
            # Define namespaces
            namespaces = {
                'dc': 'http://purl.org/dc/elements/1.1/',
                'opf': 'http://www.idpf.org/2007/opf'
            }
            
            # Extract basic metadata
            title_elem = root.find('.//dc:title', namespaces)
            if title_elem is not None:
                metadata.title = title_elem.text or ""
            
            # Authors
            author_elems = root.findall('.//dc:creator[@opf:role="aut"]', namespaces)
            if not author_elems:
                author_elems = root.findall('.//dc:creator', namespaces)
            
            metadata.authors = [elem.text for elem in author_elems if elem.text]
            
            # Calibre ID
            calibre_id_elem = root.find('.//dc:identifier[@opf:scheme="calibre"]', namespaces)
            if calibre_id_elem is not None:
                metadata.calibre_id = calibre_id_elem.text or ""
            
            # Main series
            series_elem = root.find('.//meta[@name="calibre:series"]')
            if series_elem is not None:
                series_name = series_elem.get('content', '')
                series_index_elem = root.find('.//meta[@name="calibre:series_index"]')
                series_index = 1.0
                if series_index_elem is not None:
                    try:
                        series_index = float(series_index_elem.get('content', '1'))
                    except ValueError:
                        series_index = 1.0
                
                if series_name:
                    metadata.series.append({
                        "name": series_name, 
                        "index": series_index, 
                        "label": "main"
                    })
            
            # Enhanced custom series handling
            custom_series_patterns = [
                'secondseries', 'thirdseries', 'fourthseries', 'fifthseries',
                'series1', 'series2', 'series3', 'series4', 'series5'
            ]
            
            for meta_elem in root.findall('.//meta'):
                name = meta_elem.get('name', '')
                content = meta_elem.get('content', '')
                
                # Handle calibre user metadata for custom series
                if 'user_metadata' in name and 'series' in name:
                    try:
                        # Parse the JSON content
                        json_content = content.replace('&quot;', '"')
                        user_meta = json.loads(json_content)
                        
                        series_name = user_meta.get('#value#', '')
                        series_index = user_meta.get('#extra#', 1.0)
                        series_label = user_meta.get('label', '')
                        
                        if series_name and series_label:
                            metadata.series.append({
                                "name": series_name,
                                "index": float(series_index) if series_index else 1.0,
                                "label": series_label
                            })
                            
                    except (json.JSONDecodeError, KeyError, ValueError) as e:
                        logger.debug(f"Could not parse custom series metadata: {e}")
                        continue
            
            # Other metadata
            publisher_elem = root.find('.//dc:publisher', namespaces)
            if publisher_elem is not None:
                metadata.publisher = publisher_elem.text or ""
            
            date_elem = root.find('.//dc:date', namespaces)
            if date_elem is not None:
                metadata.published_date = date_elem.text or ""
            
            # ISBN
            isbn_elem = root.find('.//dc:identifier[@opf:scheme="ISBN"]', namespaces)
            if isbn_elem is not None:
                metadata.isbn = isbn_elem.text or ""
            
            # Collect all identifiers
            identifier_elems = root.findall('.//dc:identifier', namespaces)
            for elem in identifier_elems:
                scheme = elem.get(f'{{{namespaces["opf"]}}}scheme', '')
                if scheme and elem.text:
                    metadata.identifiers[scheme.lower()] = elem.text
            
            # Tags/subjects
            tag_elems = root.findall('.//dc:subject', namespaces)
            metadata.tags = [elem.text for elem in tag_elems if elem.text]
            
            # Description
            desc_elem = root.find('.//dc:description', namespaces)
            if desc_elem is not None:
                metadata.description = desc_elem.text or ""
            
            metadata.confidence = 1.0  # High confidence for existing metadata
            
            self.log_operation("opf_metadata_extracted", {
                "title": metadata.title,
                "authors": metadata.authors,
                "series_count": len(metadata.series),
                "has_custom_series": len([s for s in metadata.series if s["label"] != "main"]) > 0
            })
            
        except ET.ParseError as e:
            logger.warning(f"Could not parse metadata.opf: {e}")
            self.log_operation("opf_parse_error", {"error": str(e), "file": str(opf_path)})
        except Exception as e:
            logger.warning(f"Error reading metadata.opf: {e}")
            self.log_operation("opf_read_error", {"error": str(e), "file": str(opf_path)})
        
        return metadata
    
    def lookup_google_books(self, title: str, author: str = "") -> BookMetadata:
        """
        Look up book metadata on Google Books API
        
        Args:
            title: Book title
            author: Author name (optional)
            
        Returns:
            BookMetadata object with found information
        """
        metadata = BookMetadata()
        metadata.source = "google_books"
        
        try:
            # Construct search query
            query = f'intitle:"{title}"'
            if author:
                query += f' inauthor:"{author}"'
            
            params = {
                'q': query,
                'maxResults': 5,
                'printType': 'books'
            }
            
            if self.google_books_api_key:
                params['key'] = self.google_books_api_key
            
            # Search Google Books
            search_url = "https://www.googleapis.com/books/v1/volumes"
            
            response = self.session.get(search_url, params=params, timeout=10)
            if response.status_code == 200:
                data = response.json()
                
                if data.get('items'):
                    book = data['items'][0]['volumeInfo']  # Take first result
                    
                    metadata.title = book.get('title', '')
                    metadata.authors = book.get('authors', [])
                    metadata.publisher = book.get('publisher', '')
                    metadata.published_date = book.get('publishedDate', '')
                    metadata.description = book.get('description', '')
                    
                    # ISBN
                    if 'industryIdentifiers' in book:
                        for identifier in book['industryIdentifiers']:
                            if identifier['type'] in ['ISBN_13', 'ISBN_10']:
                                metadata.isbn = identifier['identifier']
                                break
                    
                    # Try to extract series from subtitle or description
                    subtitle = book.get('subtitle', '')
                    if subtitle:
                        series_match = re.search(r'(.+?)\s+(?:book|volume|#)\s*(\d+)', subtitle, re.IGNORECASE)
                        if series_match:
                            metadata.series.append({
                                "name": series_match.group(1).strip(),
                                "index": float(series_match.group(2)),
                                "label": "main"
                            })
                    
                    metadata.confidence = 0.8  # High confidence for API data
                    
                    self.log_operation("google_books_lookup_success", {
                        "query": query,
                        "found_title": metadata.title,
                        "found_authors": metadata.authors
                    })
            
            # Rate limiting
            time.sleep(0.5)
            
        except Exception as e:
            logger.debug(f"Google Books lookup failed: {e}")
            self.log_operation("google_books_lookup_failed", {"error": str(e), "query": f"{title} by {author}"})
        
        return metadata
    
    def lookup_openlibrary_enhanced(self, title: str, author: str = "") -> BookMetadata:
        """
        Enhanced Open Library lookup with better series detection
        
        Args:
            title: Book title
            author: Author name (optional)
            
        Returns:
            BookMetadata object with found information
        """
        metadata = BookMetadata()
        metadata.source = "openlibrary"
        
        try:
            # Construct search query
            query_params = {"title": title, "limit": 5}
            if author:
                query_params["author"] = author
            
            # Search Open Library
            search_url = "https://openlibrary.org/search.json"
            
            response = self.session.get(search_url, params=query_params, timeout=10)
            if response.status_code == 200:
                data = response.json()
                
                if data.get('docs'):
                    book = data['docs'][0]  # Take first result
                    
                    metadata.title = book.get('title', '')
                    metadata.authors = book.get('author_name', [])
                    
                    # Extract series information if available
                    if 'subject' in book:
                        for subject in book['subject']:
                            if 'series' in subject.lower():
                                # Try to extract series name
                                series_match = re.search(r'(.+?)\s+series', subject, re.IGNORECASE)
                                if series_match:
                                    metadata.series.append({
                                        "name": series_match.group(1),
                                        "index": 1.0,
                                        "label": "main"
                                    })
                                    break
                    
                    # Check if title contains series information
                    title_series_match = re.search(r'(.+?)\s*:\s*(.+?)\s+(?:book|#)\s*(\d+)', metadata.title, re.IGNORECASE)
                    if title_series_match:
                        metadata.title = title_series_match.group(2)
                        metadata.series.append({
                            "name": title_series_match.group(1),
                            "index": float(title_series_match.group(3)),
                            "label": "main"
                        })
                    
                    metadata.publisher = book.get('publisher', [''])[0] if book.get('publisher') else ''
                    metadata.published_date = str(book.get('first_publish_year', ''))
                    
                    if book.get('isbn'):
                        metadata.isbn = book['isbn'][0]
                    
                    metadata.confidence = 0.8  # High confidence for API data
                    
                    self.log_operation("openlibrary_lookup_success", {
                        "query": query_params,
                        "found_title": metadata.title,
                        "found_authors": metadata.authors
                    })
            
            # Rate limiting
            time.sleep(0.5)
            
        except Exception as e:
            logger.debug(f"Open Library lookup failed: {e}")
            self.log_operation("openlibrary_lookup_failed", {"error": str(e), "query": f"{title} by {author}"})
        
        return metadata
    
    def save_operations_log(self, filename: str):
        """Save operations log to file"""
        try:
            with open(filename, 'w') as f:
                json.dump(self.operations_log, f, indent=2)
            logger.info(f"Operations log saved to: {filename}")
        except Exception as e:
            logger.error(f"Failed to save operations log: {e}")


class EnhancedCalibreLibraryFixer:
    """Enhanced main class for scanning and fixing Calibre library filenames"""
    
    def __init__(self, library_path: str, dry_run: bool = True, use_ai_parsing: bool = True, 
                 use_external_lookup: bool = True, backup_changes: bool = True):
        """
        Initialize the enhanced Calibre library fixer
        
        Args:
            library_path: Path to Calibre library directory
            dry_run: If True, only show what would be changed without making changes
            use_ai_parsing: If True, use AI-like parsing for filenames
            use_external_lookup: If True, look up missing metadata externally
            backup_changes: If True, create backups before making changes
        """
        self.library_path = Path(library_path)
        self.dry_run = dry_run
        self.use_ai_parsing = use_ai_parsing
        self.use_external_lookup = use_external_lookup
        self.backup_changes = backup_changes
        self.changes_made = []
        self.errors = []
        self.metadata_parser = EnhancedMetadataParser()
        self.processed_books = 0
        self.enhanced_metadata_count = 0
        
        # Verify library exists
        if not self.library_path.exists():
            raise FileNotFoundError(f"Calibre library not found: {library_path}")
        
        logger.info(f"Enhanced Calibre fixer initialized for: {self.library_path}")
        logger.info(f"Mode: {'DRY RUN' if dry_run else 'LIVE MODE'}")
        logger.info(f"AI Parsing: {'Enabled' if use_ai_parsing else 'Disabled'}")
        logger.info(f"External Lookup: {'Enabled' if use_external_lookup else 'Disabled'}")
        logger.info(f"Backup Changes: {'Enabled' if backup_changes else 'Disabled'}")
    
    def sanitize_filename(self, text: str) -> str:
        """
        Enhanced filename sanitization
        
        Args:
            text: Raw text to sanitize
            
        Returns:
            Sanitized filename-safe text
        """
        if not text:
            return ""
        
        # Remove/replace problematic characters
        text = str(text).strip()
        
        # Replace common problematic characters
        replacements = {
            '/': '-',
            '\\': '-',
            ':': '-',
            '*': '',
            '?': '',
            '"': '',
            '<': '',
            '>': '',
            '|': '-',
            '\n': ' ',
            '\r': ' ',
            '\t': ' ',
            '&': 'and',
            '#': 'No',
            '@': 'at',
            '%': 'percent'
        }
        
        for old, new in replacements.items():
            text = text.replace(old, new)
        
        # Remove multiple spaces and dashes
        text = re.sub(r'\s+', ' ', text)
        text = re.sub(r'-+', '-', text)
        
        # Remove leading/trailing spaces and dashes
        text = text.strip(' -')
        
        # Limit length to reasonable filename length
        if len(text) > 100:
            text = text[:97] + "..."
        
        return text
    
    def discover_books(self) -> List[Dict]:
        """
        Discover all books in the library by scanning for metadata.opf files
        
        Returns:
            List of dictionaries containing book information
        """
        books = []
        
        logger.info("Discovering books in library...")
        
        # Find all metadata.opf files
        opf_files = list(self.library_path.rglob("metadata.opf"))
        
        logger.info(f"Found {len(opf_files)} metadata.opf files")
        
        for opf_path in opf_files:
            try:
                book_dir = opf_path.parent
                author_dir = book_dir.parent
                
                # Extract basic info from directory structure
                book_folder_name = book_dir.name
                author_folder_name = author_dir.name
                
                # Try to extract Calibre ID from folder name
                calibre_id_match = re.search(r'\((\d+)\)$', book_folder_name)
                calibre_id = calibre_id_match.group(1) if calibre_id_match else ""
                
                book_data = {
                    'opf_path': opf_path,
                    'book_dir': book_dir,
                    'author_dir': author_dir,
                    'book_folder_name': book_folder_name,
                    'author_folder_name': author_folder_name,
                    'calibre_id': calibre_id,
                    'relative_path': book_dir.relative_to(self.library_path)
                }
                
                books.append(book_data)
                
            except Exception as e:
                logger.error(f"Error processing {opf_path}: {e}")
                self.errors.append(f"Error processing {opf_path}: {e}")
        
        logger.info(f"Discovered {len(books)} books")
        return books
    
    def enhance_book_metadata(self, book: Dict) -> BookMetadata:
        """
        Enhanced metadata enhancement using multiple sources
        
        Args:
            book: Book information dictionary
            
        Returns:
            Enhanced BookMetadata object
        """
        enhanced_metadata = BookMetadata()
        
        # Step 1: Read from OPF file
        opf_metadata = self.metadata_parser.read_metadata_opf_enhanced(book['opf_path'])
        if opf_metadata.title:
            enhanced_metadata = opf_metadata
            logger.debug(f"Using OPF metadata for: {enhanced_metadata.title}")
        
        # Step 2: If metadata is incomplete, try AI parsing from folder name
        if (self.use_ai_parsing and 
            (not enhanced_metadata.title or 
             enhanced_metadata.title == "Unknown Title" or
             not enhanced_metadata.authors)):
            
            logger.info(f"  🤖 AI parsing folder name: {book['book_folder_name']}")
            parsed_metadata = self.metadata_parser.parse_filename_ai(book['book_folder_name'])
            
            if parsed_metadata.confidence > 0.5:
                # Merge parsed data with existing data
                if not enhanced_metadata.title or enhanced_metadata.title == "Unknown Title":
                    enhanced_metadata.title = parsed_metadata.title
                    enhanced_metadata.source = "ai_parsed"
                
                if not enhanced_metadata.authors:
                    enhanced_metadata.authors = parsed_metadata.authors
                
                # Add parsed series if no series exists
                if not enhanced_metadata.series and parsed_metadata.series:
                    enhanced_metadata.series = parsed_metadata.series
                
                self.enhanced_metadata_count += 1
                logger.info(f"  ✨ Enhanced metadata with AI parsing")
        
        # Step 3: External lookup as last resort
        if (self.use_external_lookup and 
            enhanced_metadata.title and 
            enhanced_metadata.title != "Unknown Title" and
            (not enhanced_metadata.authors or enhanced_metadata.confidence < 0.7)):
            
            title_for_lookup = enhanced_metadata.title
            author_for_lookup = enhanced_metadata.authors[0] if enhanced_metadata.authors else ""
            
            logger.info(f"  🔍 External lookup for: {title_for_lookup}")
            
            # Try Google Books first
            external_metadata = self.metadata_parser.lookup_google_books(title_for_lookup, author_for_lookup)
            
            # Try Open Library if Google Books didn't work
            if not external_metadata.title or external_metadata.confidence < 0.7:
                external_metadata = self.metadata_parser.lookup_openlibrary_enhanced(title_for_lookup, author_for_lookup)
            
            # Use external data if found and confidence is good
            if external_metadata.confidence > 0.6:
                if not enhanced_metadata.authors:
                    enhanced_metadata.authors = external_metadata.authors
                
                # Add external series if not already present
                if not enhanced_metadata.series and external_metadata.series:
                    enhanced_metadata.series = external_metadata.series
                
                # Update other missing fields
                if not enhanced_metadata.publisher:
                    enhanced_metadata.publisher = external_metadata.publisher
                
                if not enhanced_metadata.published_date:
                    enhanced_metadata.published_date = external_metadata.published_date
                
                enhanced_metadata.source = f"{enhanced_metadata.source}+external"
                self.enhanced_metadata_count += 1
                logger.info(f"  🌐 Enhanced metadata with external lookup")
        
        return enhanced_metadata
    
    def generate_new_filename(self, metadata: BookMetadata) -> str:
        """
        Generate new filename based on enhanced metadata
        Format: author-title-series-series1-series2-series3-series4
        
        Args:
            metadata: Enhanced BookMetadata object
            
        Returns:
            New filename (without extension)
        """
        # Clean and prepare components
        author = self.sanitize_filename(metadata.authors[0] if metadata.authors else "Unknown Author")
        title = self.sanitize_filename(metadata.title if metadata.title else "Unknown Title")
        
        # Sort series by label priority (main first, then custom series)
        sorted_series = sorted(metadata.series, key=lambda x: (
            0 if x["label"] == "main" else 1,  # Main series first
            x["label"]  # Then alphabetically by label
        ))
        
        # Build series components
        series_parts = []
        
        # Main series (first)
        if sorted_series:
            main_series = sorted_series[0]
            series_name = main_series["name"]
            series_index = main_series["index"]
            
            # Format series index
            if series_index and series_index != 1.0:
                if series_index.is_integer():
                    series_idx = f"{int(series_index):02d}"
                else:
                    series_idx = f"{series_index:05.1f}".replace('.', '_')
                series_name = f"{series_name}{series_idx}"
            
            series_parts.append(self.sanitize_filename(series_name))
        
        # Additional series (up to 4 more)
        for series in sorted_series[1:5]:  # Skip first (main), take up to 4 more
            series_name = series["name"]
            series_index = series["index"]
            
            if series_index and series_index != 1.0:
                if series_index.is_integer():
                    series_idx = f"{int(series_index):02d}"
                else:
                    series_idx = f"{series_index:05.1f}".replace('.', '_')
                series_name = f"{series_name}{series_idx}"
            
            series_parts.append(self.sanitize_filename(series_name))
        
        # Pad with empty strings up to 4 series slots
        while len(series_parts) < 4:
            series_parts.append("")
        
        # Build filename components
        parts = [author, title] + series_parts
        
        # Remove empty parts from the end
        while parts and not parts[-1]:
            parts.pop()
        
        # Create final filename
        new_filename = '-'.join(parts)
        
        # Clean up multiple dashes
        new_filename = re.sub(r'-+', '-', new_filename)
        new_filename = new_filename.strip('-')
        
        return new_filename
    
    def create_backup(self, source_path: Path) -> Optional[Path]:
        """
        Create backup of directory before renaming
        
        Args:
            source_path: Path to backup
            
        Returns:
            Path to backup or None if failed
        """
        if not self.backup_changes:
            return None
        
        try:
            backup_dir = self.library_path / ".calibre_fixer_backups"
            backup_dir.mkdir(exist_ok=True)
            
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            backup_name = f"{source_path.name}_{timestamp}"
            backup_path = backup_dir / backup_name
            
            shutil.copytree(source_path, backup_path)
            logger.debug(f"Created backup: {backup_path}")
            return backup_path
            
        except Exception as e:
            logger.error(f"Failed to create backup: {e}")
            return None
    
    def scan_and_fix(self) -> Dict:
        """
        Enhanced main method to scan library and fix filenames
        
        Returns:
            Summary dictionary of changes
        """
        logger.info("🚀 Starting enhanced Calibre library scan...")
        
        books = self.discover_books()
        if not books:
            logger.error("No books found in library")
            return {'books_processed': 0, 'changes_made': 0, 'errors': len(self.errors)}
        
        changes_needed = 0
        
        for i, book in enumerate(books, 1):
            try:
                self.processed_books += 1
                
                logger.info(f"📖 Processing book {i}/{len(books)}: {book['book_folder_name']}")
                
                # Enhance metadata
                enhanced_metadata = self.enhance_book_metadata(book)
                
                # Generate new filename
                new_filename = self.generate_new_filename(enhanced_metadata)
                current_dir_name = book['book_folder_name']
                
                if new_filename != current_dir_name:
                    changes_needed += 1
                    
                    logger.info(f"  📋 Change needed:")
                    logger.info(f"    Current: {current_dir_name}")
                    logger.info(f"    New:     {new_filename}")
                    logger.info(f"    Author:  {', '.join(enhanced_metadata.authors) if enhanced_metadata.authors else 'Unknown'}")
                    logger.info(f"    Title:   {enhanced_metadata.title or 'Unknown'}")
                    
                    if enhanced_metadata.series:
                        for j, series in enumerate(enhanced_metadata.series):
                            series_label = series.get('label', f'series{j+1}')
                            logger.info(f"    {series_label.title()}: {series['name']} #{series['index']}")
                    
                    logger.info(f"    Source:  {enhanced_metadata.source}")
                    logger.info(f"    Confidence: {enhanced_metadata.confidence:.2f}")
                    
                    if not self.dry_run:
                        # Create backup if enabled
                        backup_path = self.create_backup(book['book_dir'])
                        
                        # Perform actual rename
                        new_dir_path = book['book_dir'].parent / new_filename
                        
                        if new_dir_path.exists():
                            logger.warning(f"  ⚠️  Target directory already exists: {new_filename}")
                            self.errors.append(f"Target exists: {new_filename}")
                            continue
                        
                        try:
                            book['book_dir'].rename(new_dir_path)
                            logger.info(f"  ✅ Renamed successfully")
                            
                            self.changes_made.append({
                                'book_id': book.get('calibre_id', ''),
                                'old_name': current_dir_name,
                                'new_name': new_filename,
                                'title': enhanced_metadata.title,
                                'authors': enhanced_metadata.authors,
                                'backup_path': str(backup_path) if backup_path else None,
                                'metadata_source': enhanced_metadata.source,
                                'confidence': enhanced_metadata.confidence
                            })
                            
                        except OSError as e:
                            logger.error(f"  ❌ Failed to rename: {e}")
                            self.errors.append(f"Rename failed for {current_dir_name}: {e}")
                            continue
                    
                    logger.info("")  # Empty line for readability
                
                # Progress indicator
                if i % 10 == 0:
                    logger.info(f"📊 Progress: {i}/{len(books)} books processed")
            
            except Exception as e:
                logger.error(f"❌ Error processing book {book['book_folder_name']}: {e}")
                self.errors.append(f"Error processing {book['book_folder_name']}: {e}")
        
        summary = {
            'books_processed': len(books),
            'changes_needed': changes_needed,
            'changes_made': len(self.changes_made),
            'metadata_enhanced': self.enhanced_metadata_count,
            'errors': len(self.errors),
            'dry_run': self.dry_run
        }
        
        return summary
    
    def generate_detailed_report(self, summary: Dict) -> str:
        """
        Generate a comprehensive report of changes and operations
        
        Args:
            summary: Summary dictionary from scan_and_fix
            
        Returns:
            Detailed report string
        """
        report = []
        report.append("=" * 60)
        report.append("ENHANCED CALIBRE LIBRARY FILENAME FIX REPORT")
        report.append("=" * 60)
        report.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"Library Path: {self.library_path}")
        report.append(f"Mode: {'DRY RUN' if self.dry_run else 'LIVE MODE'}")
        report.append(f"AI Parsing: {'Enabled' if self.use_ai_parsing else 'Disabled'}")
        report.append(f"External Lookup: {'Enabled' if self.use_external_lookup else 'Disabled'}")
        report.append(f"Backup Changes: {'Enabled' if self.backup_changes else 'Disabled'}")
        report.append("")
        
        # Summary statistics
        report.append("SUMMARY STATISTICS")
        report.append("-" * 40)
        report.append(f"Books Processed: {summary['books_processed']}")
        report.append(f"Changes Needed: {summary['changes_needed']}")
        if not self.dry_run:
            report.append(f"Changes Made: {summary['changes_made']}")
        report.append(f"Metadata Enhanced: {summary['metadata_enhanced']}")
        report.append(f"Errors: {summary['errors']}")
        report.append("")
        
        # Changes made
        if self.changes_made:
            report.append("CHANGES MADE")
            report.append("-" * 40)
            for change in self.changes_made:
                report.append(f"Book: {', '.join(change['authors'])} - {change['title']}")
                report.append(f"  From: {change['old_name']}")
                report.append(f"  To:   {change['new_name']}")
                report.append(f"  Source: {change['metadata_source']}")
                report.append(f"  Confidence: {change['confidence']:.2f}")
                if change.get('backup_path'):
                    report.append(f"  Backup: {change['backup_path']}")
                report.append("")
        
        # Errors
        if self.errors:
            report.append("ERRORS ENCOUNTERED")
            report.append("-" * 40)
            for error in self.errors:
                report.append(f"• {error}")
            report.append("")
        
        # Performance statistics
        if hasattr(self.metadata_parser, 'operations_log'):
            operations = self.metadata_parser.operations_log
            report.append("OPERATION STATISTICS")
            report.append("-" * 40)
            
            op_counts = {}
            for op in operations:
                op_type = op['operation']
                op_counts[op_type] = op_counts.get(op_type, 0) + 1
            
            for op_type, count in op_counts.items():
                report.append(f"{op_type}: {count}")
            report.append("")
        
        return "\n".join(report)
    
    def save_change_log(self, filename: str):
        """Save detailed change log as JSON"""
        try:
            log_data = {
                'timestamp': datetime.now().isoformat(),
                'library_path': str(self.library_path),
                'settings': {
                    'dry_run': self.dry_run,
                    'use_ai_parsing': self.use_ai_parsing,
                    'use_external_lookup': self.use_external_lookup,
                    'backup_changes': self.backup_changes
                },
                'changes': self.changes_made,
                'errors': self.errors
            }
            
            with open(filename, 'w') as f:
                json.dump(log_data, f, indent=2)
            
            logger.info(f"Change log saved to: {filename}")
            
        except Exception as e:
            logger.error(f"Failed to save change log: {e}")


def find_calibre_library() -> Optional[str]:
    """
    Try to find Calibre library in common locations
    
    Returns:
        Path to library if found, None otherwise
    """
    common_paths = [
        "~/Calibre Library",
        "~/Documents/Calibre Library",
        "~/Books",
        "~/Documents/Books",
        "/home/lou/Calibre Library",
        "/home/lou/Documents/Calibre Library",
        "/run/media/lou/Data/Lou Fogle/Lou's Library"
    ]
    
    for path in common_paths:
        expanded_path = Path(path).expanduser()
        # Check for OPF files instead of metadata.db since the db might be empty
        if expanded_path.exists() and list(expanded_path.rglob("metadata.opf")):
            return str(expanded_path)
    
    return None


def main():
    """Enhanced main function with comprehensive command line interface"""
    parser = argparse.ArgumentParser(
        description="Enhanced Calibre Library Scanner and Filename Fixer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Dry run with AI parsing and external lookup
  python calibre-library-fixer-enhanced.py --library ~/Library --dry-run
  
  # Actually make changes with all features
  python calibre-library-fixer-enhanced.py --library ~/Library --fix
  
  # Auto-find library and preview changes
  python calibre-library-fixer-enhanced.py --auto --dry-run
  
  # Disable external lookups for faster processing
  python calibre-library-fixer-enhanced.py --auto --fix --no-external-lookup

Enhanced Features:
  ✨ AI-powered filename parsing for missing metadata
  🌐 External database lookups (Google Books, Open Library)
  📚 Multiple series support (main + 4 custom series)
  🔄 Automatic backups before changes
  📊 Comprehensive logging and reporting
  🎯 High confidence metadata enhancement

Filename Format:
  author-title-series-series1-series2-series3-series4
  
  Where:
  - author: Primary author name
  - title: Book title
  - series: Main series name + index
  - series1-4: Additional series from custom Calibre columns
        """
    )
    
    parser.add_argument('--library', '-l', 
                       help='Path to Calibre library directory')
    parser.add_argument('--auto', '-a', action='store_true',
                       help='Auto-detect Calibre library location')
    parser.add_argument('--dry-run', action='store_true', default=True,
                       help='Preview changes without making them (default)')
    parser.add_argument('--fix', action='store_true',
                       help='Actually make the changes (disables dry-run)')
    parser.add_argument('--report', '-r',
                       help='Save detailed report to file')
    parser.add_argument('--change-log',
                       help='Save detailed change log as JSON')
    parser.add_argument('--operations-log',
                       help='Save operations log as JSON')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Enable verbose logging')
    parser.add_argument('--no-ai-parsing', action='store_true',
                       help='Disable AI-like filename parsing for missing metadata')
    parser.add_argument('--no-external-lookup', action='store_true',
                       help='Disable external database lookups (faster processing)')
    parser.add_argument('--no-backup', action='store_true',
                       help='Disable automatic backups before changes')
    
    args = parser.parse_args()
    
    # Configure logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Determine library path
    library_path = None
    
    if args.auto:
        library_path = find_calibre_library()
        if library_path:
            print(f"🔍 Found Calibre library: {library_path}")
        else:
            print("❌ Could not auto-detect Calibre library")
            print("   Looked for directories containing metadata.opf files in common locations")
            sys.exit(1)
    elif args.library:
        library_path = args.library
    else:
        print("Please specify --library path or use --auto to detect")
        parser.print_help()
        sys.exit(1)
    
    # Determine mode
    dry_run = not args.fix
    
    if dry_run:
        print("\n🔍 DRY RUN MODE - No changes will be made")
        print("Use --fix to actually make changes\n")
    else:
        print("\n⚠️  LIVE MODE - Changes will be made to your library")
        print(f"📁 Library: {library_path}")
        if not args.no_backup:
            print("💾 Backups will be created before changes")
        response = input("Are you sure you want to continue? (yes/no): ")
        if response.lower() != 'yes':
            print("Cancelled.")
            sys.exit(0)
        print()
    
    try:
        # Initialize and run enhanced fixer
        fixer = EnhancedCalibreLibraryFixer(
            library_path, 
            dry_run=dry_run,
            use_ai_parsing=not args.no_ai_parsing,
            use_external_lookup=not args.no_external_lookup,
            backup_changes=not args.no_backup
        )
        
        # Run the scan and fix process
        summary = fixer.scan_and_fix()
        
        # Generate and display report
        report = fixer.generate_detailed_report(summary)
        print(f"\n{report}")
        
        # Save reports if requested
        if args.report:
            with open(args.report, 'w') as f:
                f.write(report)
            print(f"📄 Report saved to: {args.report}")
        
        if args.change_log:
            fixer.save_change_log(args.change_log)
        
        if args.operations_log:
            fixer.metadata_parser.save_operations_log(args.operations_log)
        
        # Save automatic logs
        auto_report_file = f"calibre_fixer_report_{timestamp}.txt"
        with open(auto_report_file, 'w') as f:
            f.write(report)
        
        auto_change_log = f"calibre_fixer_changes_{timestamp}.json"
        fixer.save_change_log(auto_change_log)
        
        print(f"\n📋 Automatic logs saved:")
        print(f"   Report: {auto_report_file}")
        print(f"   Changes: {auto_change_log}")
        print(f"   Debug: {log_filename}")
        
        # Summary
        if dry_run and summary['changes_needed'] > 0:
            print(f"\n💡 Found {summary['changes_needed']} files that need renaming.")
            if summary['metadata_enhanced'] > 0:
                print(f"   Enhanced metadata for {summary['metadata_enhanced']} books.")
            print("   Use --fix to make the changes.")
        elif not dry_run:
            print(f"\n✅ Successfully processed {summary['changes_made']} files.")
            if summary['metadata_enhanced'] > 0:
                print(f"🚀 Enhanced metadata for {summary['metadata_enhanced']} books.")
            if summary['errors'] > 0:
                print(f"⚠️  {summary['errors']} errors occurred (see log for details).")
        
    except Exception as e:
        logger.error(f"Fatal error: {e}")
        print(f"\n❌ Fatal error: {e}")
        print(f"Check the log file for details: {log_filename}")
        sys.exit(1)


if __name__ == "__main__":
    main()
