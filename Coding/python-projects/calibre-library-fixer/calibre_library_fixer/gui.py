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
from typing import Optional, Dict
import json
import sqlite3
import re
import shutil

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget,
    QPushButton, QLabel, QLineEdit, QTextEdit, QProgressBar,
    QCheckBox, QGroupBox, QGridLayout, QFileDialog, QMessageBox,
    QSplitter, QTabWidget, QTableWidget, QTableWidgetItem,
    QHeaderView, QComboBox, QSpinBox, QFrame, QScrollArea
)
from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSettings, QSize
)
from PyQt6.QtGui import (
    QFont, QIcon, QPalette, QColor, QPixmap, QPainter, QBrush, QPen
)

# Embedded CalibreLibraryFixer class
class CalibreLibraryFixer:
    """Main class for scanning and fixing Calibre library filenames"""
    
    def __init__(self, library_path: str, dry_run: bool = True):
        self.library_path = Path(library_path)
        self.dry_run = dry_run
        self.db_path = self.library_path / 'metadata.db'
        self.changes_made = []
        self.errors = []
        
        if not self.library_path.exists():
            raise FileNotFoundError(f"Calibre library not found: {library_path}")
        
        if not self.db_path.exists():
            raise FileNotFoundError(f"Calibre database not found: {self.db_path}")
    
    def sanitize_filename(self, text: str) -> str:
        if not text:
            return ""
        
        text = str(text).strip()
        replacements = {
            '/': '-', '\\': '-', ':': '-', '*': '', '?': '', '"': '',
            '<': '', '>': '', '|': '-', '\n': ' ', '\r': ' ', '\t': ' '
        }
        
        for old, new in replacements.items():
            text = text.replace(old, new)
        
        text = re.sub(r'\s+', ' ', text)
        text = re.sub(r'-+', '-', text)
        text = text.strip(' -')
        
        if len(text) > 100:
            text = text[:97] + "..."
        
        return text
    
    def get_book_metadata(self):
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
                
                books.append(book_data)
            
            conn.close()
            return books
            
        except sqlite3.Error as e:
            raise Exception(f"Database error: {e}")
    
    def get_custom_series(self, cursor, book_id):
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
        author = self.sanitize_filename(book['authors'].split(' & ')[0])
        title = self.sanitize_filename(book['title'])
        
        series = ""
        if book['series_name']:
            series_idx = ""
            if book['series_index']:
                idx = float(book['series_index'])
                if idx.is_integer():
                    series_idx = f"{int(idx):02d}"
                else:
                    series_idx = f"{idx:05.1f}".replace('.', '_')
            
            series = self.sanitize_filename(f"{book['series_name']}{series_idx}")
        
        custom_parts = []
        for i in range(1, 5):
            series_key = f"series{i}"
            if series_key in book['custom_series']:
                custom_parts.append(self.sanitize_filename(book['custom_series'][series_key]))
            else:
                custom_parts.append("")
        
        parts = [author, title, series] + custom_parts
        
        while parts and not parts[-1]:
            parts.pop()
        
        filename_parts = []
        for part in parts:
            filename_parts.append(part if part else "")
        
        new_filename = '-'.join(filename_parts)
        new_filename = re.sub(r'-+', '-', new_filename)
        new_filename = new_filename.strip('-')
        
        return new_filename
    
    def scan_and_fix(self, progress_callback=None):
        books = self.get_book_metadata()
        if not books:
            raise Exception("No books found or database error")
        
        changes_made = 0
        errors = 0
        total_books = len(books)
        
        for i, book in enumerate(books):
            if progress_callback:
                progress_callback(int((i / total_books) * 100))
            
            try:
                new_filename = self.generate_new_filename(book)
                current_dir = book['current_path']
                current_dir_name = current_dir.name
                
                if new_filename != current_dir_name:
                    if not self.dry_run:
                        new_dir_path = current_dir.parent / new_filename
                        
                        if new_dir_path.exists():
                            errors += 1
                            continue
                        
                        try:
                            current_dir.rename(new_dir_path)
                            self.update_database_path(book['id'], new_filename)
                            
                            self.changes_made.append({
                                'book_id': book['id'],
                                'old_name': current_dir_name,
                                'new_name': new_filename,
                                'title': book['title'],
                                'author': book['authors']
                            })
                            
                        except OSError:
                            errors += 1
                            continue
                    else:
                        # For dry run, just add to changes list
                        self.changes_made.append({
                            'book_id': book['id'],
                            'old_name': current_dir_name,
                            'new_name': new_filename,
                            'title': book['title'],
                            'author': book['authors']
                        })
                    
                    changes_made += 1
            
            except Exception:
                errors += 1
        
        if progress_callback:
            progress_callback(100)
        
        summary = {
            'books_processed': len(books),
            'changes_needed': changes_made,
            'changes_made': len(self.changes_made) if not self.dry_run else 0,
            'errors': errors,
            'dry_run': self.dry_run
        }
        
        return summary
    
    def update_database_path(self, book_id, new_path):
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
            
        except sqlite3.Error:
            pass

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
        self.init_ui()
        
    def init_ui(self):
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("Library Preview")
        title.setFont(QFont("Arial", 12, QFont.Bold))
        layout.addWidget(title)
        
        # Table for showing changes
        self.changes_table = QTableWidget()
        self.changes_table.setColumnCount(4)
        self.changes_table.setHorizontalHeaderLabels([
            "Book ID", "Current Name", "New Name", "Status"
        ])
        
        # Make table responsive
        header = self.changes_table.horizontalHeader()
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.Stretch)
        
        layout.addWidget(self.changes_table)
        self.setLayout(layout)
    
    def update_preview(self, changes: list):
        """Update the preview table with changes"""
        self.changes_table.setRowCount(len(changes))
        
        for row, change in enumerate(changes):
            self.changes_table.setItem(row, 0, QTableWidgetItem(str(change.get('book_id', ''))))
            self.changes_table.setItem(row, 1, QTableWidgetItem(change.get('old_name', '')))
            self.changes_table.setItem(row, 2, QTableWidgetItem(change.get('new_name', '')))
            self.changes_table.setItem(row, 3, QTableWidgetItem(change.get('status', 'Pending')))


class SettingsWidget(QWidget):
    """Widget for application settings"""
    
    def __init__(self):
        super().__init__()
        self.settings = QSettings('CalibreFixer', 'GUI')
        self.init_ui()
        self.load_settings()
        
    def init_ui(self):
        layout = QVBoxLayout()
        
        # Filename Format Settings
        format_group = QGroupBox("Filename Format Settings")
        format_layout = QGridLayout()
        
        format_layout.addWidget(QLabel("Format:"), 0, 0)
        self.format_label = QLabel("author-title-series-series1-series2-series3-series4")
        self.format_label.setStyleSheet("font-family: monospace; background: #f0f0f0; padding: 5px;")
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
        safety_group = QGroupBox("Safety Settings")
        safety_layout = QVBoxLayout()
        
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
        advanced_group = QGroupBox("Advanced Settings")
        advanced_layout = QGridLayout()
        
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
        self.setLayout(layout)
    
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
        title.setFont(QFont("Arial", 18, QFont.Bold))
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
        """Setup the application style"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f5f5f5;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #cccccc;
                border-radius: 5px;
                margin-top: 1ex;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QTabWidget::pane {
                border: 1px solid #cccccc;
                background-color: white;
            }
            QTabBar::tab {
                background-color: #e0e0e0;
                padding: 8px 12px;
                margin-right: 2px;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }
            QTabBar::tab:selected {
                background-color: white;
                border-bottom: 2px solid #2196F3;
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
        if hasattr(self.worker_thread.fixer, 'changes_made'):
            self.preview_widget.update_preview(self.worker_thread.fixer.changes_made)
        
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
        if not self.current_summary or self.current_summary.get('changes_needed', 0) == 0:
            QMessageBox.information(self, "No Changes", "No changes to apply.")
            return
        
        # Confirm with user
        changes_count = self.current_summary.get('changes_needed', 0)
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
        
        # Disable dry run and start scan again
        self.dry_run_check.setChecked(False)
        self.start_scan()
    
    def add_log(self, message: str):
        """Add message to log display"""
        timestamp = "[" + QTimer().singleShot.__self__.time().toString("hh:mm:ss") + "] "
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
        
        event.accept()


def main():
    """Main application entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("Calibre Library Fixer")
    app.setApplicationVersion("2.0")
    app.setOrganizationName("LibraryTools")
    
    # Set application icon
    # app.setWindowIcon(QIcon("icon.png"))  # Uncomment if you have an icon file
    
    # Create and show main window
    window = CalibreFixerGUI()
    window.show()
    
    # Start event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
