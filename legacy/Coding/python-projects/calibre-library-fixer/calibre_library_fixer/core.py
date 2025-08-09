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
from typing import Dict, List, Optional, Tuple
import json
import logging

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

class CalibreLibraryFixer:
    """Main class for scanning and fixing Calibre library filenames"""
    
    def __init__(self, library_path: str, dry_run: bool = True):
        """
        Initialize the Calibre library fixer
        
        Args:
            library_path: Path to Calibre library directory
            dry_run: If True, only show what would be changed without making changes
        """
        self.library_path = Path(library_path)
        self.dry_run = dry_run
        self.db_path = self.library_path / 'metadata.db'
        self.changes_made = []
        self.errors = []
        
        # Verify library exists
        if not self.library_path.exists():
            raise FileNotFoundError(f"Calibre library not found: {library_path}")
        
        if not self.db_path.exists():
            raise FileNotFoundError(f"Calibre database not found: {self.db_path}")
        
        logger.info(f"Initialized Calibre fixer for: {self.library_path}")
        logger.info(f"Mode: {'DRY RUN' if dry_run else 'LIVE MODE'}")
    
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
        
        for book in books:
            try:
                # Generate new filename
                new_filename = self.generate_new_filename(book)
                
                # Get current directory name (should match book path)
                current_dir = book['current_path']
                current_dir_name = current_dir.name
                
                if new_filename != current_dir_name:
                    logger.info(f"Book ID {book['id']}:")
                    logger.info(f"  Current: {current_dir_name}")
                    logger.info(f"  New:     {new_filename}")
                    logger.info(f"  Author:  {book['authors']}")
                    logger.info(f"  Title:   {book['title']}")
                    if book['series_name']:
                        logger.info(f"  Series:  {book['series_name']} #{book['series_index']}")
                    
                    # Show custom series if any
                    if book['custom_series']:
                        for key, value in book['custom_series'].items():
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
                                'title': book['title'],
                                'author': book['authors']
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
        fixer = CalibreLibraryFixer(library_path, dry_run=dry_run)
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
