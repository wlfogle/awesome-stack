#!/usr/bin/env python3
"""
Calibre Library Scanner and Filename Fixer
==========================================
Scans Calibre library, reads metadata, and fixes filenames according to structure:
author-title-series-series1-series2-series3-series4

Author: AI Assistant
Date: 2025-06-20
License: MIT
"""

import os
import sys
import sqlite3
import shutil
import argparse
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
import json
import logging
import xml.etree.ElementTree as ET
import requests
from urllib.parse import quote
import time
from difflib import SequenceMatcher

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('calibre_fixer.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class BookMetadata:
    """Class to hold book metadata information"""
    
    def __init__(self):
        self.title = ""
        self.authors = []
        self.series = []
        self.isbn = ""
        self.publisher = ""
        self.published_date = ""
        self.description = ""
        self.language = "en"
        self.confidence = 0.0  # Confidence score for parsed data


class MetadataParser:
    """Class for parsing metadata from various sources"""
    
    def __init__(self):
        self.goodreads_api_key = None  # Set this if you have a Goodreads API key
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Calibre-Library-Fixer/1.0 (https://github.com/user/calibre-fixer)'
        })
    
    def parse_filename(self, filename: str) -> BookMetadata:
        """
        Use AI-like parsing to extract metadata from filename
        
        Args:
            filename: The filename to parse
            
        Returns:
            BookMetadata object with parsed information
        """
        metadata = BookMetadata()
        
        # Remove file extension and common prefixes
        clean_name = filename
        if '.' in clean_name:
            clean_name = clean_name.rsplit('.', 1)[0]
        
        # Common patterns for book filenames
        patterns = [
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
            # Just title
            r'^(.+?)$'
        ]
        
        for pattern in patterns:
            match = re.match(pattern, clean_name, re.IGNORECASE)
            if match:
                groups = match.groups()
                
                if len(groups) >= 4:  # Author, Title, Series, Number
                    metadata.authors = [groups[0].strip()]
                    metadata.title = groups[1].strip()
                    metadata.series = [{"name": groups[2].strip(), "index": float(groups[3])}]
                    metadata.confidence = 0.9
                elif len(groups) == 3:  # Might be Author, Title, Series or Title, Author, Series
                    # Heuristic: if first part looks like an author name, treat as author
                    if self._looks_like_author(groups[0]):
                        metadata.authors = [groups[0].strip()]
                        metadata.title = groups[1].strip()
                        metadata.series = [{"name": groups[2].strip(), "index": 1}]
                    else:
                        metadata.title = groups[0].strip()
                        metadata.authors = [groups[1].strip()]
                        metadata.series = [{"name": groups[2].strip(), "index": 1}]
                    metadata.confidence = 0.7
                elif len(groups) == 2:  # Author - Title or Title by Author
                    if ' by ' in clean_name.lower():
                        metadata.title = groups[0].strip()
                        metadata.authors = [groups[1].strip()]
                    else:
                        metadata.authors = [groups[0].strip()]
                        metadata.title = groups[1].strip()
                    metadata.confidence = 0.6
                else:  # Just title
                    metadata.title = groups[0].strip()
                    metadata.confidence = 0.3
                
                break
        
        return metadata
    
    def _looks_like_author(self, text: str) -> bool:
        """
        Heuristic to determine if text looks like an author name
        
        Args:
            text: Text to check
            
        Returns:
            True if text looks like an author name
        """
        # Check for common author name patterns
        author_patterns = [
            r'^[A-Z][a-z]+\s+[A-Z][a-z]+$',  # First Last
            r'^[A-Z]\.[A-Z]\.\s+[A-Z][a-z]+$',  # A.B. Last
            r'^[A-Z][a-z]+\s+[A-Z]\.[A-Z]?\.?\s+[A-Z][a-z]+$',  # First A.B. Last
        ]
        
        for pattern in author_patterns:
            if re.match(pattern, text):
                return True
        
        return False
    
    def read_metadata_opf(self, opf_path: Path) -> BookMetadata:
        """
        Read metadata from Calibre metadata.opf file
        
        Args:
            opf_path: Path to metadata.opf file
            
        Returns:
            BookMetadata object with parsed information
        """
        metadata = BookMetadata()
        
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
            
            # Main series
            series_elem = root.find('.//meta[@name="calibre:series"]')
            if series_elem is not None:
                series_name = series_elem.get('content', '')
                series_index_elem = root.find('.//meta[@name="calibre:series_index"]')
                series_index = 1
                if series_index_elem is not None:
                    try:
                        series_index = float(series_index_elem.get('content', '1'))
                    except ValueError:
                        series_index = 1
                
                metadata.series.append({"name": series_name, "index": series_index})
            
            # Custom series (secondseries, thirdseries, etc.)
            for series_meta in root.findall('.//meta'):
                name = series_meta.get('name', '')
                if 'series' in name and name != 'calibre:series' and name != 'calibre:series_index':
                    content = series_meta.get('content', '')
                    if content and content.startswith('{'):
                        # Parse JSON content for custom series
                        try:
                            series_data = json.loads(content.replace('&quot;', '"'))
                            series_name = series_data.get('#value#', '')
                            series_index = series_data.get('#extra#', 1)
                            if series_name:
                                metadata.series.append({
                                    "name": series_name, 
                                    "index": series_index,
                                    "label": series_data.get('label', '')
                                })
                        except json.JSONDecodeError:
                            pass
            
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
            
            metadata.confidence = 1.0  # High confidence for existing metadata
            
        except ET.ParseError as e:
            logger.warning(f"Could not parse metadata.opf: {e}")
        except Exception as e:
            logger.warning(f"Error reading metadata.opf: {e}")
        
        return metadata
    
    def lookup_goodreads(self, title: str, author: str = "") -> BookMetadata:
        """
        Look up book metadata on Goodreads (via web scraping since API is limited)
        
        Args:
            title: Book title
            author: Author name (optional)
            
        Returns:
            BookMetadata object with found information
        """
        metadata = BookMetadata()
        
        try:
            # Construct search query
            query = title
            if author:
                query += f" {author}"
            
            # Search Goodreads
            search_url = f"https://www.goodreads.com/search?q={quote(query)}"
            
            response = self.session.get(search_url, timeout=10)
            if response.status_code == 200:
                # Basic parsing - this is simplified and would need more robust parsing
                # For production use, consider using a proper web scraping library
                content = response.text
                
                # Try to extract basic information
                title_match = re.search(r'<a class="bookTitle"[^>]*>([^<]+)</a>', content)
                if title_match:
                    metadata.title = title_match.group(1).strip()
                
                author_match = re.search(r'<span itemprop="author"[^>]*>([^<]+)</span>', content)
                if author_match:
                    metadata.authors = [author_match.group(1).strip()]
                
                metadata.confidence = 0.5  # Medium confidence for web scraping
            
            # Rate limiting
            time.sleep(1)
            
        except Exception as e:
            logger.debug(f"Goodreads lookup failed: {e}")
        
        return metadata
    
    def lookup_openlibrary(self, title: str, author: str = "") -> BookMetadata:
        """
        Look up book metadata on Open Library
        
        Args:
            title: Book title
            author: Author name (optional)
            
        Returns:
            BookMetadata object with found information
        """
        metadata = BookMetadata()
        
        try:
            # Construct search query
            query_params = {"title": title}
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
                                        "index": 1
                                    })
                                    break
                    
                    metadata.publisher = book.get('publisher', [''])[0] if book.get('publisher') else ''
                    metadata.published_date = str(book.get('first_publish_year', ''))
                    
                    if book.get('isbn'):
                        metadata.isbn = book['isbn'][0]
                    
                    metadata.confidence = 0.8  # High confidence for API data
            
            # Rate limiting
            time.sleep(0.5)
            
        except Exception as e:
            logger.debug(f"Open Library lookup failed: {e}")
        
        return metadata


class CalibreLibraryFixer:
    """Main class for scanning and fixing Calibre library filenames"""
    
    def __init__(self, library_path: str, dry_run: bool = True, use_ai_parsing: bool = True, use_external_lookup: bool = True):
        """
        Initialize the Calibre library fixer
        
        Args:
            library_path: Path to Calibre library directory
            dry_run: If True, only show what would be changed without making changes
            use_ai_parsing: If True, use AI-like parsing for filenames
            use_external_lookup: If True, look up missing metadata externally
        """
        self.library_path = Path(library_path)
        self.dry_run = dry_run
        self.use_ai_parsing = use_ai_parsing
        self.use_external_lookup = use_external_lookup
        self.db_path = self.library_path / 'metadata.db'
        self.changes_made = []
        self.errors = []
        self.metadata_parser = MetadataParser()
        
        # Verify library exists
        if not self.library_path.exists():
            raise FileNotFoundError(f"Calibre library not found: {library_path}")
        
        if not self.db_path.exists():
            raise FileNotFoundError(f"Calibre database not found: {self.db_path}")
        
        logger.info(f"Initialized Calibre fixer for: {self.library_path}")
        logger.info(f"Mode: {'DRY RUN' if dry_run else 'LIVE MODE'}")
        logger.info(f"AI Parsing: {'Enabled' if use_ai_parsing else 'Disabled'}")
        logger.info(f"External Lookup: {'Enabled' if use_external_lookup else 'Disabled'}")
    
    def sanitize_filename(self, text: str) -> str:
        """
        Sanitize text for use in filenames
        
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
            '\t': ' '
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
    
    def get_book_metadata(self) -> List[Dict]:
        """
        Extract book metadata from Calibre database
        
        Returns:
            List of dictionaries containing book metadata
        """
        books = []
        
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            # Main query to get book information
            query = """
            SELECT 
                b.id,
                b.title,
                b.path,
                b.timestamp,
                GROUP_CONCAT(a.name, ' & ') as authors,
                s.name as series_name,
                b.series_index,
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
                
                # Get custom columns (series1, series2, etc.)
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
                
                books.append(book_data)
            
            conn.close()
            logger.info(f"Found {len(books)} books in library")
            return books
            
        except sqlite3.Error as e:
            logger.error(f"Database error: {e}")
            return []
    
    def get_custom_series(self, cursor: sqlite3.Cursor, book_id: int) -> Dict[str, str]:
        """
        Get custom series columns for a book
        
        Args:
            cursor: Database cursor
            book_id: Book ID
            
        Returns:
            Dictionary of custom series data
        """
        custom_series = {}
        
        try:
            # Get custom column definitions
            cursor.execute("""
                SELECT name, label, display 
                FROM custom_columns 
                WHERE datatype = 'series'
                ORDER BY label
            """)
            
            custom_columns = cursor.fetchall()
            
            for name, label, display in custom_columns:
                # Query the custom column table
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
                    # Table might not exist or other error
                    continue
            
            return custom_series
            
        except sqlite3.Error as e:
            logger.debug(f"Error getting custom series for book {book_id}: {e}")
            return {}
    
    def generate_new_filename(self, book: Dict) -> str:
        """
        Generate new filename based on metadata
        Format: author-title-series-series1-series2-series3-series4
        
        Args:
            book: Book metadata dictionary
            
        Returns:
            New filename (without extension)
        """
        # Clean and prepare components
        author = self.sanitize_filename(book['authors'].split(' & ')[0])  # Use first author
        title = self.sanitize_filename(book['title'])
        
        # Main series
        series = ""
        if book['series_name']:
            series_idx = ""
            if book['series_index']:
                # Format series index (remove .0 if whole number)
                idx = float(book['series_index'])
                if idx.is_integer():
                    series_idx = f"{int(idx):02d}"
                else:
                    series_idx = f"{idx:05.1f}".replace('.', '_')
            
            series = self.sanitize_filename(f"{book['series_name']}{series_idx}")
        
        # Custom series (series1, series2, series3, series4)
        custom_parts = []
        for i in range(1, 5):  # series1 through series4
            series_key = f"series{i}"
            if series_key in book['custom_series']:
                custom_parts.append(self.sanitize_filename(book['custom_series'][series_key]))
            else:
                custom_parts.append("")
        
        # Build filename components
        parts = [author, title, series] + custom_parts
        
        # Remove empty parts from the end
        while parts and not parts[-1]:
            parts.pop()
        
        # Join with dashes, but handle empty middle parts
        filename_parts = []
        for part in parts:
            filename_parts.append(part if part else "")
        
        # Create final filename
        new_filename = '-'.join(filename_parts)
        
        # Clean up multiple dashes
        new_filename = re.sub(r'-+', '-', new_filename)
        new_filename = new_filename.strip('-')
        
        return new_filename
    
    def enhance_book_metadata(self, book: Dict) -> Dict:
        """
        Enhance book metadata using AI parsing and external lookups
        
        Args:
            book: Book metadata dictionary
            
        Returns:
            Enhanced book metadata dictionary
        """
        enhanced_book = book.copy()
        
        # Check if metadata.opf exists and read it
        opf_path = book['current_path'] / 'metadata.opf'
        if opf_path.exists():
            opf_metadata = self.metadata_parser.read_metadata_opf(opf_path)
            
            # Update book with metadata from OPF if better than database
            if opf_metadata.title and (not book['title'] or book['title'] == 'Unknown Title'):
                enhanced_book['title'] = opf_metadata.title
            
            if opf_metadata.authors and (not book['authors'] or book['authors'] == 'Unknown Author'):
                enhanced_book['authors'] = ' & '.join(opf_metadata.authors)
            
            # Handle multiple series from OPF
            if opf_metadata.series:
                # Sort series by index to maintain consistent order
                sorted_series = sorted(opf_metadata.series, key=lambda x: x.get('index', 0))
                
                # Update main series if not set
                if not enhanced_book['series_name'] and sorted_series:
                    main_series = sorted_series[0]
                    enhanced_book['series_name'] = main_series['name']
                    enhanced_book['series_index'] = main_series['index']
                
                # Map additional series to custom series slots
                custom_series = enhanced_book.get('custom_series', {})
                for i, series in enumerate(sorted_series[1:], 1):  # Skip first (main) series
                    if i <= 4:  # Only support up to 4 custom series
                        series_key = f"series{i}"
                        if series_key not in custom_series:
                            series_name = series['name']
                            if series.get('index', 1) != 1:
                                series_name += f" #{series['index']}"
                            custom_series[series_key] = series_name
                
                enhanced_book['custom_series'] = custom_series
        
        # If still missing critical metadata, try AI parsing from filename
        if self.use_ai_parsing and (not enhanced_book['title'] or enhanced_book['title'] == 'Unknown Title' or 
                                    not enhanced_book['authors'] or enhanced_book['authors'] == 'Unknown Author'):
            
            current_dir_name = book['current_path'].name
            parsed_metadata = self.metadata_parser.parse_filename(current_dir_name)
            
            # Use parsed data if confidence is reasonable and we don't have better data
            if parsed_metadata.confidence > 0.5:
                if not enhanced_book['title'] or enhanced_book['title'] == 'Unknown Title':
                    enhanced_book['title'] = parsed_metadata.title
                
                if not enhanced_book['authors'] or enhanced_book['authors'] == 'Unknown Author':
                    enhanced_book['authors'] = ' & '.join(parsed_metadata.authors)
                
                # Add parsed series if not already present
                if parsed_metadata.series and not enhanced_book['series_name']:
                    main_series = parsed_metadata.series[0]
                    enhanced_book['series_name'] = main_series['name']
                    enhanced_book['series_index'] = main_series['index']
        
        # External lookup as last resort for missing metadata
        if (self.use_external_lookup and 
            (not enhanced_book['title'] or enhanced_book['title'] == 'Unknown Title' or
             not enhanced_book['authors'] or enhanced_book['authors'] == 'Unknown Author')):
            
            # Try external lookups
            title_for_lookup = enhanced_book.get('title', '')
            author_for_lookup = enhanced_book.get('authors', '').split(' & ')[0] if enhanced_book.get('authors') else ''
            
            if title_for_lookup and title_for_lookup != 'Unknown Title':
                logger.info(f"  🔍 Looking up metadata for: {title_for_lookup}")
                
                # Try Open Library first (better API)
                external_metadata = self.metadata_parser.lookup_openlibrary(title_for_lookup, author_for_lookup)
                
                # Try Goodreads if Open Library didn't work
                if not external_metadata.title:
                    external_metadata = self.metadata_parser.lookup_goodreads(title_for_lookup, author_for_lookup)
                
                # Use external data if found and confidence is good
                if external_metadata.confidence > 0.6:
                    if not enhanced_book['title'] or enhanced_book['title'] == 'Unknown Title':
                        enhanced_book['title'] = external_metadata.title
                    
                    if not enhanced_book['authors'] or enhanced_book['authors'] == 'Unknown Author':
                        enhanced_book['authors'] = ' & '.join(external_metadata.authors)
                    
                    # Add external series if not already present
                    if external_metadata.series and not enhanced_book['series_name']:
                        main_series = external_metadata.series[0]
                        enhanced_book['series_name'] = main_series['name']
                        enhanced_book['series_index'] = main_series['index']
        
        return enhanced_book
    
    def scan_and_fix(self) -> Dict:
        """
        Main method to scan library and fix filenames
        
        Returns:
            Summary dictionary of changes
        """
        logger.info("Starting Calibre library scan...")
        
        books = self.get_book_metadata()
        if not books:
            logger.error("No books found or database error")
            return {'books_processed': 0, 'changes_made': 0, 'errors': 0}
        
        changes_made = 0
        errors = 0
        metadata_enhanced = 0
        
        for book in books:
            try:
                # Enhance metadata using AI parsing and external lookups
                original_title = book['title']
                original_author = book['authors']
                
                enhanced_book = self.enhance_book_metadata(book)
                
                # Check if metadata was enhanced
                if (enhanced_book['title'] != original_title or 
                    enhanced_book['authors'] != original_author):
                    metadata_enhanced += 1
                    logger.info(f"  📚 Enhanced metadata for: {enhanced_book['title']}")
                
                # Generate new filename
                new_filename = self.generate_new_filename(enhanced_book)
                
                # Get current directory name (should match book path)
                current_dir = book['current_path']
                current_dir_name = current_dir.name
                
                if new_filename != current_dir_name:
                    logger.info(f"Book ID {book['id']}:")
                    logger.info(f"  Current: {current_dir_name}")
                    logger.info(f"  New:     {new_filename}")
                    logger.info(f"  Author:  {enhanced_book['authors']}")
                    logger.info(f"  Title:   {enhanced_book['title']}")
                    if enhanced_book['series_name']:
                        logger.info(f"  Series:  {enhanced_book['series_name']} #{enhanced_book['series_index']}")
                    
                    # Show custom series if any
                    if enhanced_book['custom_series']:
                        for key, value in enhanced_book['custom_series'].items():
                            logger.info(f"  {key}: {value}")
                    
                    if not self.dry_run:
                        # Perform actual rename
                        new_dir_path = current_dir.parent / new_filename
                        
                        if new_dir_path.exists():
                            logger.warning(f"Target directory already exists: {new_filename}")
                            errors += 1
                            continue
                        
                        try:
                            current_dir.rename(new_dir_path)
                            logger.info(f"  ✅ Renamed successfully")
                            
                            # Update database path
                            self.update_database_path(book['id'], new_filename)
                            
                            self.changes_made.append({
                                'book_id': book['id'],
                                'old_name': current_dir_name,
                                'new_name': new_filename,
                                'title': enhanced_book['title'],
                                'author': enhanced_book['authors']
                            })
                            
                        except OSError as e:
                            logger.error(f"  ❌ Failed to rename: {e}")
                            errors += 1
                            continue
                    
                    changes_made += 1
                    logger.info("")  # Empty line for readability
            
            except Exception as e:
                logger.error(f"Error processing book ID {book['id']}: {e}")
                errors += 1
        
        summary = {
            'books_processed': len(books),
            'changes_needed': changes_made,
            'changes_made': len(self.changes_made),
            'metadata_enhanced': metadata_enhanced,
            'errors': errors,
            'dry_run': self.dry_run
        }
        
        return summary
    
    def update_database_path(self, book_id: int, new_path: str):
        """
        Update the book path in Calibre database
        
        Args:
            book_id: Book ID
            new_path: New directory path
        """
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
            logger.error(f"Failed to update database for book {book_id}: {e}")
    
    def generate_report(self, summary: Dict) -> str:
        """
        Generate a detailed report of changes
        
        Args:
            summary: Summary dictionary from scan_and_fix
            
        Returns:
            Report string
        """
        report = []
        report.append("Calibre Library Filename Fix Report")
        report.append("=" * 40)
        report.append(f"Library Path: {self.library_path}")
        report.append(f"Mode: {'DRY RUN' if self.dry_run else 'LIVE MODE'}")
        report.append(f"Books Processed: {summary['books_processed']}")
        report.append(f"Changes Needed: {summary['changes_needed']}")
        if not self.dry_run:
            report.append(f"Changes Made: {summary['changes_made']}")
        report.append(f"Errors: {summary['errors']}")
        report.append("")
        
        if self.changes_made:
            report.append("Changes Made:")
            report.append("-" * 20)
            for change in self.changes_made:
                report.append(f"Book ID {change['book_id']}: {change['author']} - {change['title']}")
                report.append(f"  From: {change['old_name']}")
                report.append(f"  To:   {change['new_name']}")
                report.append("")
        
        return "\n".join(report)


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
        "/home/lou/Documents/Calibre Library"
    ]
    
    for path in common_paths:
        expanded_path = Path(path).expanduser()
        if (expanded_path / 'metadata.db').exists():
            return str(expanded_path)
    
    return None


def main():
    """Main function with command line interface"""
    parser = argparse.ArgumentParser(
        description="Calibre Library Scanner and Filename Fixer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Dry run (preview changes)
  python calibre-library-fixer.py --library ~/Calibre Library --dry-run
  
  # Actually make changes
  python calibre-library-fixer.py --library ~/Calibre Library --fix
  
  # Auto-find library and preview
  python calibre-library-fixer.py --auto --dry-run

Filename Format:
  author-title-series-series1-series2-series3-series4
  
  Where:
  - author: First author name
  - title: Book title
  - series: Main series name + number
  - series1-4: Custom series columns from Calibre
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
                       help='Save report to file')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Enable verbose logging')
    parser.add_argument('--no-ai-parsing', action='store_true',
                       help='Disable AI-like filename parsing for missing metadata')
    parser.add_argument('--no-external-lookup', action='store_true',
                       help='Disable external database lookups (Goodreads, Open Library)')
    
    args = parser.parse_args()
    
    # Configure logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Determine library path
    library_path = None
    
    if args.auto:
        library_path = find_calibre_library()
        if library_path:
            print(f"Found Calibre library: {library_path}")
        else:
            print("Could not auto-detect Calibre library")
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
        response = input("Are you sure you want to continue? (yes/no): ")
        if response.lower() != 'yes':
            print("Cancelled.")
            sys.exit(0)
        print()
    
    try:
        # Initialize and run fixer
        fixer = CalibreLibraryFixer(
            library_path, 
            dry_run=dry_run,
            use_ai_parsing=not args.no_ai_parsing,
            use_external_lookup=not args.no_external_lookup
        )
        summary = fixer.scan_and_fix()
        
        # Generate and display report
        report = fixer.generate_report(summary)
        print("\n" + report)
        
        # Save report if requested
        if args.report:
            with open(args.report, 'w') as f:
                f.write(report)
            print(f"\nReport saved to: {args.report}")
        
        # Summary
        if dry_run and summary['changes_needed'] > 0:
            print(f"\n💡 Found {summary['changes_needed']} files that need renaming.")
            print("   Use --fix to make the changes.")
        elif not dry_run:
            print(f"\n✅ Successfully processed {summary['changes_made']} files.")
            if summary['errors'] > 0:
                print(f"⚠️  {summary['errors']} errors occurred.")
        
    except Exception as e:
        logger.error(f"Fatal error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
