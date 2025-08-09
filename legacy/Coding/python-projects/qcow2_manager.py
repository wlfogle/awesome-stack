#!/usr/bin/env python3
"""
QCOW2 Manager - A FileZilla-like GUI for mounting and browsing qcow2 files
Dependencies: python3-tkinter, qemu-utils, python3-psutil
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import os
import subprocess
import threading
import time
import shutil
from pathlib import Path
import json
import sys

class QCow2Manager:
    def __init__(self, root):
        self.root = root
        self.root.title("QCOW2 Manager - FileZilla-like Interface")
        self.root.geometry("1200x800")
        
        # State variables
        self.mounted_images = {}  # {mount_point: {nbd_device, qcow2_file, partition}}
        self.current_local_path = str(Path.home())
        self.current_remote_path = ""
        self.selected_mount = None
        
        # Check dependencies on startup
        self.check_dependencies()
        
        # Create GUI
        self.create_gui()
        
        # Start monitoring thread
        self.monitor_thread = threading.Thread(target=self.monitor_mounts, daemon=True)
        self.monitor_thread.start()
        
        # Refresh initial view
        self.refresh_local_view()
        
    def check_dependencies(self):
        """Check if all required dependencies are installed"""
        missing_deps = []
        
        # Check for qemu-nbd
        if not shutil.which('qemu-nbd'):
            missing_deps.append('qemu-utils')
            
        # Check for modprobe
        if not shutil.which('modprobe'):
            missing_deps.append('kmod')
            
        # Check for mount/umount
        if not shutil.which('mount') or not shutil.which('umount'):
            missing_deps.append('mount')
            
        if missing_deps:
            msg = f"Missing dependencies: {', '.join(missing_deps)}\n\n"
            msg += "Install them using your package manager:\n"
            msg += f"• Arch Linux: sudo pacman -S {' '.join(missing_deps)}\n"
            msg += f"• Ubuntu/Debian: sudo apt install {' '.join(missing_deps)}\n"
            msg += f"• Fedora: sudo dnf install {' '.join(missing_deps)}\n"
            msg += f"• openSUSE: sudo zypper install {' '.join(missing_deps)}"
            
            messagebox.showerror("Missing Dependencies", msg)
            sys.exit(1)
            
    def create_gui(self):
        """Create the main GUI interface"""
        # Create main frame
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create toolbar
        self.create_toolbar(main_frame)
        
        # Create main paned window (like FileZilla)
        self.paned_window = ttk.PanedWindow(main_frame, orient=tk.HORIZONTAL)
        self.paned_window.pack(fill=tk.BOTH, expand=True, pady=(5, 0))
        
        # Create left pane (Local files)
        self.create_local_pane()
        
        # Create right pane (Mounted qcow2 files)
        self.create_remote_pane()
        
        # Create bottom status bar
        self.create_status_bar(main_frame)
        
        # Create context menus
        self.create_context_menus()
        
    def create_toolbar(self, parent):
        """Create the toolbar with buttons"""
        toolbar = ttk.Frame(parent)
        toolbar.pack(fill=tk.X, pady=(0, 5))
        
        # Mount button
        ttk.Button(toolbar, text="Mount QCOW2", command=self.mount_qcow2).pack(side=tk.LEFT, padx=(0, 5))
        
        # Unmount button
        ttk.Button(toolbar, text="Unmount Selected", command=self.unmount_selected).pack(side=tk.LEFT, padx=(0, 5))
        
        # Refresh button
        ttk.Button(toolbar, text="Refresh", command=self.refresh_all).pack(side=tk.LEFT, padx=(0, 5))
        
        # Separator
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)
        
        # Transfer buttons
        ttk.Button(toolbar, text="→ Copy to Remote", command=self.copy_to_remote).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="← Copy to Local", command=self.copy_to_local).pack(side=tk.LEFT, padx=(0, 5))
        
        # Separator
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)
        
        # Settings button
        ttk.Button(toolbar, text="Settings", command=self.show_settings).pack(side=tk.LEFT, padx=(0, 5))
        
    def create_local_pane(self):
        """Create the local file browser pane"""
        # Local pane frame
        local_frame = ttk.Frame(self.paned_window)
        self.paned_window.add(local_frame, weight=1)
        
        # Local pane header
        local_header = ttk.Frame(local_frame)
        local_header.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(local_header, text="Local Files", font=("Arial", 12, "bold")).pack(side=tk.LEFT)
        
        # Local path entry
        path_frame = ttk.Frame(local_frame)
        path_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(path_frame, text="Path:").pack(side=tk.LEFT)
        self.local_path_var = tk.StringVar(value=self.current_local_path)
        self.local_path_entry = ttk.Entry(path_frame, textvariable=self.local_path_var)
        self.local_path_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.local_path_entry.bind('<Return>', lambda e: self.navigate_local())
        
        # Local file tree
        self.local_tree = ttk.Treeview(local_frame, columns=('Size', 'Modified'), show='tree headings')
        self.local_tree.heading('#0', text='Name')
        self.local_tree.heading('Size', text='Size')
        self.local_tree.heading('Modified', text='Modified')
        self.local_tree.column('#0', width=300)
        self.local_tree.column('Size', width=100)
        self.local_tree.column('Modified', width=150)
        
        # Local tree scrollbar
        local_scrollbar = ttk.Scrollbar(local_frame, orient=tk.VERTICAL, command=self.local_tree.yview)
        self.local_tree.configure(yscrollcommand=local_scrollbar.set)
        
        self.local_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        local_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Bind events
        self.local_tree.bind('<Double-1>', self.on_local_double_click)
        self.local_tree.bind('<Button-3>', self.show_local_context_menu)
        
    def create_remote_pane(self):
        """Create the remote/mounted file browser pane"""
        # Remote pane frame
        remote_frame = ttk.Frame(self.paned_window)
        self.paned_window.add(remote_frame, weight=1)
        
        # Remote pane header
        remote_header = ttk.Frame(remote_frame)
        remote_header.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(remote_header, text="Mounted QCOW2 Files", font=("Arial", 12, "bold")).pack(side=tk.LEFT)
        
        # Mount selection
        mount_frame = ttk.Frame(remote_frame)
        mount_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(mount_frame, text="Mount:").pack(side=tk.LEFT)
        self.mount_var = tk.StringVar()
        self.mount_combo = ttk.Combobox(mount_frame, textvariable=self.mount_var, state='readonly')
        self.mount_combo.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.mount_combo.bind('<<ComboboxSelected>>', self.on_mount_selected)
        
        # Remote path entry
        remote_path_frame = ttk.Frame(remote_frame)
        remote_path_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(remote_path_frame, text="Path:").pack(side=tk.LEFT)
        self.remote_path_var = tk.StringVar()
        self.remote_path_entry = ttk.Entry(remote_path_frame, textvariable=self.remote_path_var)
        self.remote_path_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.remote_path_entry.bind('<Return>', lambda e: self.navigate_remote())
        
        # Remote file tree
        self.remote_tree = ttk.Treeview(remote_frame, columns=('Size', 'Modified'), show='tree headings')
        self.remote_tree.heading('#0', text='Name')
        self.remote_tree.heading('Size', text='Size')
        self.remote_tree.heading('Modified', text='Modified')
        self.remote_tree.column('#0', width=300)
        self.remote_tree.column('Size', width=100)
        self.remote_tree.column('Modified', width=150)
        
        # Remote tree scrollbar
        remote_scrollbar = ttk.Scrollbar(remote_frame, orient=tk.VERTICAL, command=self.remote_tree.yview)
        self.remote_tree.configure(yscrollcommand=remote_scrollbar.set)
        
        self.remote_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        remote_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Bind events
        self.remote_tree.bind('<Double-1>', self.on_remote_double_click)
        self.remote_tree.bind('<Button-3>', self.show_remote_context_menu)
        
    def create_status_bar(self, parent):
        """Create the status bar"""
        self.status_bar = ttk.Frame(parent)
        self.status_bar.pack(fill=tk.X, pady=(5, 0))
        
        self.status_label = ttk.Label(self.status_bar, text="Ready")
        self.status_label.pack(side=tk.LEFT)
        
        # Progress bar
        self.progress_bar = ttk.Progressbar(self.status_bar, mode='indeterminate')
        self.progress_bar.pack(side=tk.RIGHT, padx=(5, 0))
        
    def create_context_menus(self):
        """Create context menus for file operations"""
        # Local context menu
        self.local_context_menu = tk.Menu(self.root, tearoff=0)
        self.local_context_menu.add_command(label="Copy to Remote", command=self.copy_to_remote)
        self.local_context_menu.add_command(label="Delete", command=self.delete_local_file)
        self.local_context_menu.add_separator()
        self.local_context_menu.add_command(label="Properties", command=self.show_local_properties)
        
        # Remote context menu
        self.remote_context_menu = tk.Menu(self.root, tearoff=0)
        self.remote_context_menu.add_command(label="Copy to Local", command=self.copy_to_local)
        self.remote_context_menu.add_command(label="Delete", command=self.delete_remote_file)
        self.remote_context_menu.add_separator()
        self.remote_context_menu.add_command(label="Properties", command=self.show_remote_properties)
        
    def mount_qcow2(self):
        """Mount a qcow2 file"""
        qcow2_file = filedialog.askopenfilename(
            title="Select QCOW2 file to mount",
            filetypes=[("QCOW2 files", "*.qcow2"), ("All files", "*")]
        )
        
        if not qcow2_file:
            return
            
        # Run mounting in separate thread
        thread = threading.Thread(target=self._mount_qcow2_thread, args=(qcow2_file,))
        thread.daemon = True
        thread.start()
        
    def _mount_qcow2_thread(self, qcow2_file):
        """Mount qcow2 file in separate thread"""
        try:
            self.update_status("Mounting QCOW2 file...")
            self.progress_bar.start()
            
            # Load NBD module
            subprocess.run(['sudo', 'modprobe', 'nbd', 'max_part=8'], check=True)
            
            # Find available NBD device
            nbd_device = None
            for i in range(16):
                device = f"/dev/nbd{i}"
                try:
                    # Try to connect
                    result = subprocess.run(['sudo', 'qemu-nbd', '--connect', device, qcow2_file], 
                                          capture_output=True, text=True)
                    if result.returncode == 0:
                        nbd_device = device
                        break
                except:
                    continue
                    
            if not nbd_device:
                messagebox.showerror("Error", "No available NBD device found")
                return
                
            # Wait for device to be ready
            time.sleep(2)
            
            # Check for partitions
            partitions = []
            try:
                subprocess.run(['sudo', 'partprobe', nbd_device], check=False)
                time.sleep(1)
                
                # List partitions
                for i in range(1, 9):
                    part = f"{nbd_device}p{i}"
                    if os.path.exists(part):
                        partitions.append(part)
                        
                if not partitions:
                    partitions = [nbd_device]  # Use device directly if no partitions
                    
            except:
                partitions = [nbd_device]
                
            # Select partition if multiple
            selected_partition = partitions[0]
            if len(partitions) > 1:
                # Show selection dialog
                self.root.after(0, lambda: self._select_partition(partitions, nbd_device, qcow2_file))
                return
                
            # Mount the partition
            self._mount_partition(selected_partition, nbd_device, qcow2_file)
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to mount QCOW2 file: {str(e)}")
        finally:
            self.progress_bar.stop()
            self.update_status("Ready")
            
    def _select_partition(self, partitions, nbd_device, qcow2_file):
        """Show partition selection dialog"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Select Partition")
        dialog.geometry("400x300")
        dialog.transient(self.root)
        dialog.grab_set()
        
        ttk.Label(dialog, text="Multiple partitions found. Select one to mount:").pack(pady=10)
        
        # Partition listbox
        listbox = tk.Listbox(dialog)
        listbox.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        for partition in partitions:
            listbox.insert(tk.END, partition)
            
        listbox.selection_set(0)
        
        # Buttons
        button_frame = ttk.Frame(dialog)
        button_frame.pack(fill=tk.X, padx=10, pady=10)
        
        def on_mount():
            selection = listbox.curselection()
            if selection:
                selected_partition = partitions[selection[0]]
                dialog.destroy()
                threading.Thread(target=self._mount_partition, 
                               args=(selected_partition, nbd_device, qcow2_file)).start()
                
        def on_cancel():
            # Cleanup NBD device
            subprocess.run(['sudo', 'qemu-nbd', '--disconnect', nbd_device], check=False)
            dialog.destroy()
            
        ttk.Button(button_frame, text="Mount", command=on_mount).pack(side=tk.RIGHT, padx=(5, 0))
        ttk.Button(button_frame, text="Cancel", command=on_cancel).pack(side=tk.RIGHT)
        
    def _mount_partition(self, partition, nbd_device, qcow2_file):
        """Mount a specific partition"""
        try:
            # Create mount point
            mount_point = f"/tmp/qcow2_mount_{int(time.time())}"
            os.makedirs(mount_point, exist_ok=True)
            
            # Mount with read/write access
            subprocess.run(['sudo', 'mount', '-o', 'rw', partition, mount_point], check=True)
            
            # Store mount info
            self.mounted_images[mount_point] = {
                'nbd_device': nbd_device,
                'qcow2_file': qcow2_file,
                'partition': partition
            }
            
            # Update UI
            self.root.after(0, self.update_mount_list)
            self.update_status(f"Mounted {os.path.basename(qcow2_file)} at {mount_point}")
            
        except Exception as e:
            # Cleanup on error
            subprocess.run(['sudo', 'qemu-nbd', '--disconnect', nbd_device], check=False)
            messagebox.showerror("Error", f"Failed to mount partition: {str(e)}")
            
    def unmount_selected(self):
        """Unmount the selected mount point"""
        if not self.selected_mount:
            messagebox.showwarning("Warning", "No mount selected")
            return
            
        thread = threading.Thread(target=self._unmount_thread, args=(self.selected_mount,))
        thread.daemon = True
        thread.start()
        
    def _unmount_thread(self, mount_point):
        """Unmount in separate thread"""
        try:
            self.update_status("Unmounting...")
            self.progress_bar.start()
            
            if mount_point in self.mounted_images:
                mount_info = self.mounted_images[mount_point]
                
                # Unmount
                subprocess.run(['sudo', 'umount', mount_point], check=True)
                
                # Disconnect NBD device
                subprocess.run(['sudo', 'qemu-nbd', '--disconnect', mount_info['nbd_device']], check=True)
                
                # Remove mount point
                os.rmdir(mount_point)
                
                # Remove from our tracking
                del self.mounted_images[mount_point]
                
                # Update UI
                self.root.after(0, self.update_mount_list)
                self.update_status(f"Unmounted {os.path.basename(mount_info['qcow2_file'])}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Failed to unmount: {str(e)}")
        finally:
            self.progress_bar.stop()
            self.update_status("Ready")
            
    def refresh_all(self):
        """Refresh both local and remote views"""
        self.refresh_local_view()
        self.refresh_remote_view()
        
    def refresh_local_view(self):
        """Refresh the local file view"""
        self.local_tree.delete(*self.local_tree.get_children())
        
        try:
            path = Path(self.current_local_path)
            
            # Add parent directory if not root
            if path.parent != path:
                self.local_tree.insert('', 'end', text='..', values=('', ''))
                
            # Add directories first
            for item in sorted(path.iterdir()):
                if item.is_dir():
                    try:
                        stat = item.stat()
                        modified = time.strftime('%Y-%m-%d %H:%M', time.localtime(stat.st_mtime))
                        self.local_tree.insert('', 'end', text=f"📁 {item.name}", values=('', modified))
                    except:
                        self.local_tree.insert('', 'end', text=f"📁 {item.name}", values=('', ''))
                        
            # Add files
            for item in sorted(path.iterdir()):
                if item.is_file():
                    try:
                        stat = item.stat()
                        size = self.format_size(stat.st_size)
                        modified = time.strftime('%Y-%m-%d %H:%M', time.localtime(stat.st_mtime))
                        icon = "💾" if item.suffix == ".qcow2" else "📄"
                        self.local_tree.insert('', 'end', text=f"{icon} {item.name}", values=(size, modified))
                    except:
                        self.local_tree.insert('', 'end', text=f"📄 {item.name}", values=('', ''))
                        
        except Exception as e:
            messagebox.showerror("Error", f"Failed to refresh local view: {str(e)}")
            
    def refresh_remote_view(self):
        """Refresh the remote file view"""
        self.remote_tree.delete(*self.remote_tree.get_children())
        
        if not self.selected_mount or not os.path.exists(self.selected_mount):
            return
            
        try:
            current_path = os.path.join(self.selected_mount, self.current_remote_path.lstrip('/'))
            path = Path(current_path)
            
            # Add parent directory if not root
            if path != Path(self.selected_mount):
                self.remote_tree.insert('', 'end', text='..', values=('', ''))
                
            # Add directories first
            for item in sorted(path.iterdir()):
                if item.is_dir():
                    try:
                        stat = item.stat()
                        modified = time.strftime('%Y-%m-%d %H:%M', time.localtime(stat.st_mtime))
                        self.remote_tree.insert('', 'end', text=f"📁 {item.name}", values=('', modified))
                    except:
                        self.remote_tree.insert('', 'end', text=f"📁 {item.name}", values=('', ''))
                        
            # Add files
            for item in sorted(path.iterdir()):
                if item.is_file():
                    try:
                        stat = item.stat()
                        size = self.format_size(stat.st_size)
                        modified = time.strftime('%Y-%m-%d %H:%M', time.localtime(stat.st_mtime))
                        self.remote_tree.insert('', 'end', text=f"📄 {item.name}", values=(size, modified))
                    except:
                        self.remote_tree.insert('', 'end', text=f"📄 {item.name}", values=('', ''))
                        
        except Exception as e:
            messagebox.showerror("Error", f"Failed to refresh remote view: {str(e)}")
            
    def format_size(self, size):
        """Format file size in human readable format"""
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if size < 1024.0:
                return f"{size:.1f} {unit}"
            size /= 1024.0
        return f"{size:.1f} PB"
        
    def update_mount_list(self):
        """Update the mount point combo box"""
        mount_points = list(self.mounted_images.keys())
        self.mount_combo['values'] = mount_points
        
        if mount_points and not self.selected_mount:
            self.mount_combo.set(mount_points[0])
            self.selected_mount = mount_points[0]
            self.current_remote_path = ""
            self.remote_path_var.set("")
            self.refresh_remote_view()
            
    def update_status(self, message):
        """Update the status bar"""
        self.status_label.config(text=message)
        
    def navigate_local(self):
        """Navigate to the path in local entry"""
        new_path = self.local_path_var.get()
        if os.path.isdir(new_path):
            self.current_local_path = new_path
            self.refresh_local_view()
        else:
            messagebox.showerror("Error", f"Path does not exist: {new_path}")
            self.local_path_var.set(self.current_local_path)
            
    def navigate_remote(self):
        """Navigate to the path in remote entry"""
        if not self.selected_mount:
            return
            
        new_path = self.remote_path_var.get()
        full_path = os.path.join(self.selected_mount, new_path.lstrip('/'))
        
        if os.path.isdir(full_path):
            self.current_remote_path = new_path
            self.refresh_remote_view()
        else:
            messagebox.showerror("Error", f"Path does not exist: {new_path}")
            self.remote_path_var.set(self.current_remote_path)
            
    def on_local_double_click(self, event):
        """Handle double-click on local file tree"""
        selection = self.local_tree.selection()
        if not selection:
            return
            
        item = self.local_tree.item(selection[0])
        name = item['text']
        
        if name.startswith('📁') or name == '..':
            # Navigate to directory
            dir_name = name[2:] if name.startswith('📁') else name
            if dir_name == '..':
                self.current_local_path = str(Path(self.current_local_path).parent)
            else:
                self.current_local_path = os.path.join(self.current_local_path, dir_name)
            self.local_path_var.set(self.current_local_path)
            self.refresh_local_view()
            
    def on_remote_double_click(self, event):
        """Handle double-click on remote file tree"""
        selection = self.remote_tree.selection()
        if not selection or not self.selected_mount:
            return
            
        item = self.remote_tree.item(selection[0])
        name = item['text']
        
        if name.startswith('📁') or name == '..':
            # Navigate to directory
            dir_name = name[2:] if name.startswith('📁') else name
            if dir_name == '..':
                self.current_remote_path = '/'.join(self.current_remote_path.split('/')[:-1])
            else:
                self.current_remote_path = os.path.join(self.current_remote_path, dir_name).replace('\\', '/')
            self.remote_path_var.set(self.current_remote_path)
            self.refresh_remote_view()
            
    def on_mount_selected(self, event):
        """Handle mount selection change"""
        self.selected_mount = self.mount_var.get()
        self.current_remote_path = ""
        self.remote_path_var.set("")
        self.refresh_remote_view()
        
    def copy_to_remote(self):
        """Copy selected local file to remote"""
        if not self.selected_mount:
            messagebox.showwarning("Warning", "No mount selected")
            return
            
        # Implementation for copying files
        messagebox.showinfo("Info", "Copy to remote functionality - to be implemented")
        
    def copy_to_local(self):
        """Copy selected remote file to local"""
        if not self.selected_mount:
            messagebox.showwarning("Warning", "No mount selected")
            return
            
        # Implementation for copying files
        messagebox.showinfo("Info", "Copy to local functionality - to be implemented")
        
    def delete_local_file(self):
        """Delete selected local file"""
        messagebox.showinfo("Info", "Delete local file functionality - to be implemented")
        
    def delete_remote_file(self):
        """Delete selected remote file"""
        messagebox.showinfo("Info", "Delete remote file functionality - to be implemented")
        
    def show_local_properties(self):
        """Show properties of selected local file"""
        messagebox.showinfo("Info", "Local file properties - to be implemented")
        
    def show_remote_properties(self):
        """Show properties of selected remote file"""
        messagebox.showinfo("Info", "Remote file properties - to be implemented")
        
    def show_local_context_menu(self, event):
        """Show context menu for local files"""
        self.local_context_menu.post(event.x_root, event.y_root)
        
    def show_remote_context_menu(self, event):
        """Show context menu for remote files"""
        self.remote_context_menu.post(event.x_root, event.y_root)
        
    def show_settings(self):
        """Show settings dialog"""
        messagebox.showinfo("Info", "Settings dialog - to be implemented")
        
    def monitor_mounts(self):
        """Monitor mounted filesystems"""
        while True:
            try:
                # Check if mounted filesystems are still valid
                to_remove = []
                for mount_point in self.mounted_images:
                    if not os.path.ismount(mount_point):
                        to_remove.append(mount_point)
                        
                for mount_point in to_remove:
                    del self.mounted_images[mount_point]
                    
                if to_remove:
                    self.root.after(0, self.update_mount_list)
                    
                time.sleep(5)  # Check every 5 seconds
            except:
                break
                
    def on_closing(self):
        """Handle application closing"""
        # Cleanup mounted filesystems
        for mount_point in list(self.mounted_images.keys()):
            try:
                mount_info = self.mounted_images[mount_point]
                subprocess.run(['sudo', 'umount', mount_point], check=False)
                subprocess.run(['sudo', 'qemu-nbd', '--disconnect', mount_info['nbd_device']], check=False)
                os.rmdir(mount_point)
            except:
                pass
                
        self.root.destroy()

def main():
    """Main entry point"""
    root = tk.Tk()
    app = QCow2Manager(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
