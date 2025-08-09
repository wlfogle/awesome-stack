#!/usr/bin/env python3
"""
Test script to demonstrate enhanced metadata parsing for Richard A. Knaak books
"""

import os
import sys
import json
from pathlib import Path

# Add the current directory to path to import our enhanced classes
sys.path.insert(0, '/home/lou')

# Import directly from the script file
import importlib.util
spec = importlib.util.spec_from_file_location("calibre_library_fixer", "/home/lou/calibre-library-fixer.py")
calibre_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(calibre_module)
MetadataParser = calibre_module.MetadataParser

def test_knaak_books():
    """Test the enhanced metadata parsing on Richard A. Knaak's books"""
    
    knaak_dir = Path("/run/media/lou/Data/Lou Fogle/Lou's Library/Richard A. Knaak")
    if not knaak_dir.exists():
        print("Knaak directory not found")
        return
    
    parser = MetadataParser()
    
    print("🧪 Testing Enhanced Metadata Parsing for Richard A. Knaak Books")
    print("=" * 70)
    print()
    
    # Test each book directory
    for book_dir in sorted(knaak_dir.iterdir()):
        if not book_dir.is_dir():
            continue
            
        print(f"📖 Book: {book_dir.name}")
        print("-" * 50)
        
        # Test filename parsing
        parsed_metadata = parser.parse_filename(book_dir.name)
        print(f"🤖 AI Filename Parsing (confidence: {parsed_metadata.confidence:.1f}):")
        print(f"   Title: {parsed_metadata.title}")
        print(f"   Authors: {parsed_metadata.authors}")
        print(f"   Series: {parsed_metadata.series}")
        
        # Test metadata.opf reading if it exists
        opf_path = book_dir / "metadata.opf"
        if opf_path.exists():
            opf_metadata = parser.read_metadata_opf(opf_path)
            print(f"📄 OPF Metadata (confidence: {opf_metadata.confidence:.1f}):")
            print(f"   Title: {opf_metadata.title}")
            print(f"   Authors: {opf_metadata.authors}")
            print(f"   Series: {opf_metadata.series}")
            
            # Simulate what the enhanced filename would be
            if opf_metadata.series:
                # Sort series by index to show the enhanced naming
                sorted_series = sorted(opf_metadata.series, key=lambda x: x.get('index', 0))
                
                # Build enhanced filename components
                author = opf_metadata.authors[0] if opf_metadata.authors else "Unknown"
                title = opf_metadata.title
                
                # Main series
                main_series = ""
                if sorted_series:
                    main = sorted_series[0]
                    series_idx = f"{int(main['index']):03d}" if main.get('index') else "001"
                    main_series = f"{main['name']}{series_idx}"
                
                # Additional series
                additional_series = []
                for series in sorted_series[1:]:
                    series_name = series['name']
                    if series.get('index', 1) != 1:
                        series_name += f" #{int(series['index'])}"
                    additional_series.append(series_name)
                
                # Build proposed filename
                filename_parts = [author, title, main_series] + additional_series[:4]  # Max 4 additional
                proposed_filename = '-'.join(part for part in filename_parts if part)
                
                print(f"✨ Enhanced Filename Would Be:")
                print(f"   {proposed_filename}")
        else:
            print("📄 No metadata.opf found")
        
        print()

def test_filename_patterns():
    """Test the AI parsing on various filename patterns"""
    
    parser = MetadataParser()
    
    print("🧪 Testing AI Filename Pattern Recognition")
    print("=" * 50)
    
    test_filenames = [
        "Richard A. Knaak - Land of the Minotaurs - Lost Histories #4",
        "Empire of Blood (5713)",
        "Kaz the Minotaur (5599)", 
        "The Legend of Huma - Richard A. Knaak",
        "Night of Blood by Richard A. Knaak",
        "Dragonlance - The Fire Rose - Book 2",
        "Tides of Blood"
    ]
    
    for filename in test_filenames:
        print(f"📁 Testing: '{filename}'")
        metadata = parser.parse_filename(filename)
        print(f"   Title: {metadata.title}")
        print(f"   Authors: {metadata.authors}")
        print(f"   Series: {metadata.series}")
        print(f"   Confidence: {metadata.confidence:.1f}")
        print()

if __name__ == "__main__":
    test_knaak_books()
    print("\n" + "="*70 + "\n")
    test_filename_patterns()
