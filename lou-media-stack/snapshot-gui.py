#!/usr/bin/env python3

import sys
import os
import subprocess
import json
from datetime import datetime
from pathlib import Path

from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                           QHBoxLayout, QLabel, QPushButton, QListWidget, 
                           QTextEdit, QLineEdit, QMessageBox, QProgressBar,
                           QTabWidget, QGroupBox, QSpinBox, QSplitter,
                           QListWidgetItem, QDialog, QDialogButtonBox)
from PyQt6.QtCore import QThread, pyqtSignal, QTimer, Qt
from PyQt6.QtGui import QFont

class SnapshotWorker(QThread):
    """Worker thread for snapshot operations to prevent UI freezing"""
    finished = pyqtSignal(bool, str)
    progress = pyqtSignal(str)
    
    def __init__(self, operation, snapshot_name=None, cleanup_days=None):
        super().__init__()
        self.operation = operation
        self.snapshot_name = snapshot_name
        self.cleanup_days = cleanup_days
        self.script_path = Path(__file__).parent / "snapshot-utility.sh"
    
    def run(self):
        if not self.script_path.exists():
            self.finished.emit(False, 'Snapshot utility script not found')
            return
        try:
            if self.operation == "create":
                cmd = [str(self.script_path), "create"]
                if self.snapshot_name:
                    cmd.append(self.snapshot_name)
                self.progress.emit("Creating snapshot...")
                
            elif self.operation == "restore":
                cmd = [str(self.script_path), "restore", self.snapshot_name]
                self.progress.emit(f"Restoring snapshot: {self.snapshot_name}")
                
            elif self.operation == "cleanup":
                cmd = [str(self.script_path), "cleanup", str(self.cleanup_days)]
                self.progress.emit(f"Cleaning up snapshots older than {self.cleanup_days} days...")
                
            elif self.operation == "list":
                cmd = [str(self.script_path), "list"]
                self.progress.emit("Listing snapshots...")
            
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=Path(__file__).parent)
            
            if result.returncode == 0:
                self.finished.emit(True, result.stdout)
            else:
                self.finished.emit(False, result.stderr or "Operation failed")
                
        except Exception as e:
            self.finished.emit(False, str(e))

class SnapshotDetailsDialog(QDialog):
    """Dialog to show detailed snapshot information"""
    def __init__(self, snapshot_name, parent=None):
        super().__init__(parent)
        self.setWindowTitle(f"Snapshot Details: {snapshot_name}")
        self.setModal(True)
        self.resize(600, 400)
        
        layout = QVBoxLayout()
        
        # Snapshot info
        info_text = QTextEdit()
        info_text.setReadOnly(True)
        
        # Get snapshot details
        snapshot_dir = Path(__file__).parent / "snapshots" / snapshot_name
        if snapshot_dir.exists():
            details = self.get_snapshot_details(snapshot_dir)
            info_text.setPlainText(details)
        else:
            info_text.setPlainText("Snapshot directory not found.")
        
        layout.addWidget(info_text)
        
        # Buttons
        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        buttons.accepted.connect(self.accept)
        layout.addWidget(buttons)
        
        self.setLayout(layout)
    
    def get_snapshot_details(self, snapshot_dir):
        details = []
        
        # Read metadata if available
        metadata_file = snapshot_dir / "metadata.json"
        if metadata_file.exists():
            try:
                with open(metadata_file, 'r') as f:
                    metadata = json.load(f)
                details.append("=== Snapshot Metadata ===")
                for key, value in metadata.items():
                    details.append(f"{key}: {value}")
                details.append("")
            except Exception as e:
                details.append(f"Error reading metadata: {e}")
        
        # List files in snapshot
        details.append("=== Snapshot Contents ===")
        try:
            for item in sorted(snapshot_dir.iterdir()):
                if item.is_file():
                    size = item.stat().st_size
                    details.append(f"FILE: {item.name} ({size} bytes)")
                elif item.is_dir():
                    file_count = len(list(item.rglob("*")))
                    details.append(f"DIR:  {item.name}/ ({file_count} items)")
        except Exception as e:
            details.append(f"Error listing contents: {e}")
        
        return "\n".join(details)

class SnapshotGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Lou Media Stack - Snapshot Manager")
        self.setGeometry(100, 100, 900, 700)
        
        # Initialize workers as None
        self.worker = None
        self.refresh_worker = None
        
        # Check if snapshot utility exists
        self.script_path = Path(__file__).parent / "snapshot-utility.sh"
        if not self.script_path.exists():
            QMessageBox.critical(self, "Error", 
                               "snapshot-utility.sh not found in the current directory!")
            sys.exit(1)
        
        self.init_ui()
        
        # Auto-refresh timer
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self.refresh_snapshots)
        self.refresh_timer.start(30000)  # Refresh every 30 seconds
        
        # Start initial refresh after UI is set up
        QTimer.singleShot(100, self.refresh_snapshots)  # Delay initial refresh
    
    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)
        
        # Title
        title = QLabel("Lou Media Stack Snapshot Manager")
        title_font = QFont()
        title_font.setPointSize(16)
        title_font.setBold(True)
        title.setFont(title_font)
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(title)
        
        # Create tab widget
        tabs = QTabWidget()
        main_layout.addWidget(tabs)
        
        # Snapshots tab
        snapshots_tab = self.create_snapshots_tab()
        tabs.addTab(snapshots_tab, "Snapshots")
        
        # Operations tab
        operations_tab = self.create_operations_tab()
        tabs.addTab(operations_tab, "Operations")
        
        # Status bar
        self.status_bar = self.statusBar()
        self.status_bar.showMessage("Ready")
        
        # Progress bar
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        main_layout.addWidget(self.progress_bar)
    
    def create_snapshots_tab(self):
        tab = QWidget()
        layout = QVBoxLayout()
        tab.setLayout(layout)
        
        # Snapshot list section
        list_group = QGroupBox("Available Snapshots")
        list_layout = QVBoxLayout()
        list_group.setLayout(list_layout)
        
        # Refresh button
        refresh_btn = QPushButton("🔄 Refresh List")
        refresh_btn.clicked.connect(self.refresh_snapshots)
        list_layout.addWidget(refresh_btn)
        
        # Snapshot list
        self.snapshot_list = QListWidget()
        self.snapshot_list.itemDoubleClicked.connect(self.show_snapshot_details)
        list_layout.addWidget(self.snapshot_list)
        
        # Buttons for snapshot operations
        buttons_layout = QHBoxLayout()
        
        self.restore_btn = QPushButton("🔄 Restore Selected")
        self.restore_btn.clicked.connect(self.restore_snapshot)
        self.restore_btn.setEnabled(False)
        buttons_layout.addWidget(self.restore_btn)
        
        self.details_btn = QPushButton("ℹ️ View Details")
        self.details_btn.clicked.connect(self.show_snapshot_details)
        self.details_btn.setEnabled(False)
        buttons_layout.addWidget(self.details_btn)
        
        self.delete_btn = QPushButton("🗑️ Delete Selected")
        self.delete_btn.clicked.connect(self.delete_snapshot)
        self.delete_btn.setEnabled(False)
        buttons_layout.addWidget(self.delete_btn)
        
        list_layout.addLayout(buttons_layout)
        layout.addWidget(list_group)
        
        # Enable/disable buttons based on selection
        self.snapshot_list.itemSelectionChanged.connect(self.on_selection_changed)
        
        return tab
    
    def create_operations_tab(self):
        tab = QWidget()
        layout = QVBoxLayout()
        tab.setLayout(layout)
        
        # Create snapshot section
        create_group = QGroupBox("Create New Snapshot")
        create_layout = QVBoxLayout()
        create_group.setLayout(create_layout)
        
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Snapshot Name (optional):"))
        self.snapshot_name_input = QLineEdit()
        self.snapshot_name_input.setPlaceholderText("Leave empty for auto-generated name")
        name_layout.addWidget(self.snapshot_name_input)
        create_layout.addLayout(name_layout)
        
        self.create_btn = QPushButton("📸 Create Snapshot")
        self.create_btn.clicked.connect(self.create_snapshot)
        create_layout.addWidget(self.create_btn)
        
        layout.addWidget(create_group)
        
        # Cleanup section
        cleanup_group = QGroupBox("Cleanup Old Snapshots")
        cleanup_layout = QVBoxLayout()
        cleanup_group.setLayout(cleanup_layout)
        
        cleanup_info = QLabel("Remove snapshots older than specified number of days:")
        cleanup_layout.addWidget(cleanup_info)
        
        days_layout = QHBoxLayout()
        days_layout.addWidget(QLabel("Days:"))
        self.cleanup_days = QSpinBox()
        self.cleanup_days.setRange(1, 365)
        self.cleanup_days.setValue(30)
        days_layout.addWidget(self.cleanup_days)
        days_layout.addStretch()
        cleanup_layout.addLayout(days_layout)
        
        self.cleanup_btn = QPushButton("🧹 Cleanup Old Snapshots")
        self.cleanup_btn.clicked.connect(self.cleanup_snapshots)
        cleanup_layout.addWidget(self.cleanup_btn)
        
        layout.addWidget(cleanup_group)
        
        # Log section
        log_group = QGroupBox("Operation Log")
        log_layout = QVBoxLayout()
        log_group.setLayout(log_layout)
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(200)
        log_layout.addWidget(self.log_text)
        
        clear_log_btn = QPushButton("Clear Log")
        clear_log_btn.clicked.connect(self.log_text.clear)
        log_layout.addWidget(clear_log_btn)
        
        layout.addWidget(log_group)
        
        layout.addStretch()
        return tab
    
    def on_selection_changed(self):
        has_selection = bool(self.snapshot_list.currentItem())
        self.restore_btn.setEnabled(has_selection)
        self.details_btn.setEnabled(has_selection)
        self.delete_btn.setEnabled(has_selection)
    
    def refresh_snapshots(self):
        # Don't start new refresh if one is already running
        if hasattr(self, 'refresh_worker') and self.refresh_worker and self.refresh_worker.isRunning():
            return
            
        self.snapshot_list.clear()
        self.status_bar.showMessage("Refreshing snapshot list...")
        
        self.refresh_worker = SnapshotWorker("list")
        self.refresh_worker.finished.connect(self.on_list_finished)
        self.refresh_worker.finished.connect(self.refresh_worker.deleteLater)
        self.refresh_worker.start()
    
    def on_list_finished(self, success, output):
        if success:
            lines = output.strip().split('\n')
            for line in lines:
                if line.strip() and not line.startswith('Available snapshots:'):
                    # Extract snapshot name and info
                    if '(' in line and ')' in line:
                        snapshot_info = line.strip()
                        item = QListWidgetItem(snapshot_info)
                        # Store just the snapshot name for operations
                        snapshot_name = line.split()[0]
                        item.setData(Qt.ItemDataRole.UserRole, snapshot_name)
                        self.snapshot_list.addItem(item)
            
            self.status_bar.showMessage(f"Found {self.snapshot_list.count()} snapshots")
        else:
            self.log_message(f"Error listing snapshots: {output}")
            self.status_bar.showMessage("Error refreshing snapshots")
    
    def create_snapshot(self):
        snapshot_name = self.snapshot_name_input.text().strip()
        
        if self.confirm_operation(f"Create snapshot" + (f" '{snapshot_name}'" if snapshot_name else "")):
            self.run_operation("create", snapshot_name if snapshot_name else None)
            self.snapshot_name_input.clear()
    
    def restore_snapshot(self):
        current_item = self.snapshot_list.currentItem()
        if not current_item:
            return
        
        snapshot_name = current_item.data(Qt.ItemDataRole.UserRole)
        
        if self.confirm_operation(f"restore snapshot '{snapshot_name}'?\n\nThis will stop all containers and restore the previous state."):
            self.run_operation("restore", snapshot_name)
    
    def delete_snapshot(self):
        current_item = self.snapshot_list.currentItem()
        if not current_item:
            return
        
        snapshot_name = current_item.data(Qt.ItemDataRole.UserRole)
        
        reply = QMessageBox.question(
            self, "Confirm Deletion",
            f"Are you sure you want to delete snapshot '{snapshot_name}'?\n\nThis action cannot be undone.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            # Delete snapshot directory
            snapshot_dir = Path(__file__).parent / "snapshots" / snapshot_name
            try:
                if snapshot_dir.exists():
                    import shutil
                    shutil.rmtree(snapshot_dir)
                    self.log_message(f"Deleted snapshot: {snapshot_name}")
                    self.refresh_snapshots()
                else:
                    self.log_message(f"Snapshot directory not found: {snapshot_name}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to delete snapshot: {e}")
    
    def cleanup_snapshots(self):
        days = self.cleanup_days.value()
        
        if self.confirm_operation(f"cleanup snapshots older than {days} days"):
            self.run_operation("cleanup", cleanup_days=days)
    
    def show_snapshot_details(self):
        current_item = self.snapshot_list.currentItem()
        if not current_item:
            return
        
        snapshot_name = current_item.data(Qt.ItemDataRole.UserRole)
        dialog = SnapshotDetailsDialog(snapshot_name, self)
        dialog.exec()
    
    def confirm_operation(self, operation):
        reply = QMessageBox.question(
            self, "Confirm Operation",
            f"Are you sure you want to {operation}?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        return reply == QMessageBox.StandardButton.Yes
    
    def run_operation(self, operation, snapshot_name=None, cleanup_days=None):
        # Disable buttons during operation
        self.set_buttons_enabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setRange(0, 0)  # Indeterminate progress
        
        # Start worker thread
        self.worker = SnapshotWorker(operation, snapshot_name, cleanup_days)
        self.worker.finished.connect(self.on_operation_finished)
        self.worker.progress.connect(self.on_operation_progress)
        self.worker.finished.connect(self.worker.deleteLater)  # Ensure thread cleanup
        self.worker.start()
    
    def on_operation_progress(self, message):
        self.status_bar.showMessage(message)
        self.log_message(message)
    
    def on_operation_finished(self, success, output):
        # Re-enable buttons
        self.set_buttons_enabled(True)
        self.progress_bar.setVisible(False)
        
        if success:
            self.log_message("Operation completed successfully")
            self.log_message(output)
            self.status_bar.showMessage("Operation completed")
            self.refresh_snapshots()
        else:
            self.log_message(f"Operation failed: {output}")
            self.status_bar.showMessage("Operation failed")
            QMessageBox.critical(self, "Operation Failed", f"Error: {output}")
    
    def set_buttons_enabled(self, enabled):
        self.create_btn.setEnabled(enabled)
        self.restore_btn.setEnabled(enabled and bool(self.snapshot_list.currentItem()))
        self.cleanup_btn.setEnabled(enabled)
        self.delete_btn.setEnabled(enabled and bool(self.snapshot_list.currentItem()))
    
    def log_message(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
        # Auto-scroll to bottom
        self.log_text.verticalScrollBar().setValue(
            self.log_text.verticalScrollBar().maximum()
        )
    
    def closeEvent(self, event):
        """Handle application close event to clean up threads"""
        # Stop the refresh timer first
        self.refresh_timer.stop()
        
        # Clean up operation worker
        if self.worker and self.worker.isRunning():
            self.worker.quit()
            self.worker.wait(1000)  # Wait up to 1 second for thread to finish
        
        # Clean up refresh worker
        if self.refresh_worker and self.refresh_worker.isRunning():
            self.refresh_worker.quit()
            self.refresh_worker.wait(1000)  # Wait up to 1 second for thread to finish
        
        event.accept()

def main():
    app = QApplication(sys.argv)
    app.setApplicationName("Lou Media Stack Snapshot Manager")
    
    # Check if running in proper environment
    if not os.environ.get('DISPLAY'):
        print("Error: DISPLAY environment variable not set.")
        print("Make sure you're running this from a graphical desktop session.")
        sys.exit(1)
    
    window = SnapshotGUI()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
