"""
Calibre Library Fixer
=====================

A comprehensive tool for organizing Calibre library filenames with standardized naming conventions.

This package provides both GUI and CLI interfaces for scanning and fixing Calibre library
filenames according to the pattern: author-title-series-series1-series2-series3-series4

Author: Lou
License: MIT
Version: 2.0.0
"""

__version__ = "2.0.0"
__author__ = "Lou"
__email__ = "lou@example.com"
__license__ = "MIT"

from .core import CalibreLibraryFixer, find_calibre_library

__all__ = ["CalibreLibraryFixer", "find_calibre_library"]
