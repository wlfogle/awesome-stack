#!/usr/bin/env python3
import sys
import os
import json
import time
import threading
from datetime import datetime
from pathlib import Path

# Default paths - will be updated by get_log_directory()
LOG_FILE = None
STATE_FILE = None
MONITOR_FILE = None
CONFIG_FILE = os.path.expanduser("~/.conversation_logger_config.json")

def load_config():
    """Load configuration from file"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, 'r') as f:
                return json.load(f)
        except:
            pass
    return {}

def save_config(config):
    """Save configuration to file"""
    with open(CONFIG_FILE, 'w') as f:
        json.dump(config, f, indent=2)

def get_log_directory():
    """Get or set the log directory"""
    config = load_config()
    
    if 'log_directory' in config and os.path.exists(config['log_directory']):
        return config['log_directory']
    
    print("\n=== Conversation Logger Setup ===")
    print("Please choose where to store conversation logs:")
    print("1. Current directory")
    print("2. Home directory")
    print("3. Custom directory")
    print("4. Use previous location (if exists)")
    
    while True:
        try:
            choice = input("\nEnter choice (1-4): ").strip()
            
            if choice == '1':
                log_dir = os.getcwd()
                break
            elif choice == '2':
                log_dir = os.path.expanduser("~")
                break
            elif choice == '3':
                log_dir = input("Enter full path: ").strip()
                if not log_dir:
                    print("Invalid path. Please try again.")
                    continue
                log_dir = os.path.expanduser(log_dir)
                break
            elif choice == '4':
                if 'log_directory' in config:
                    log_dir = config['log_directory']
                    break
                else:
                    print("No previous location found.")
                    continue
            else:
                print("Please enter 1, 2, 3, or 4.")
                continue
        except KeyboardInterrupt:
            print("\nSetup cancelled.")
            sys.exit(1)
    
    # Create directory if it doesn't exist
    try:
        os.makedirs(log_dir, exist_ok=True)
        
        # Test write permissions
        test_file = os.path.join(log_dir, '.test_write')
        with open(test_file, 'w') as f:
            f.write('test')
        os.remove(test_file)
        
        print(f"\nLog directory set to: {log_dir}")
        
        # Save configuration
        config['log_directory'] = log_dir
        save_config(config)
        
        return log_dir
        
    except Exception as e:
        print(f"Error: Cannot write to {log_dir}: {e}")
        print("Please choose a different directory.")
        return get_log_directory()

def setup_paths():
    """Setup all file paths based on log directory"""
    global LOG_FILE, STATE_FILE, MONITOR_FILE
    
    log_dir = get_log_directory()
    
    LOG_FILE = os.path.join(log_dir, "SESSION_LOG.md")
    STATE_FILE = os.path.join(log_dir, ".logger_state.json")
    MONITOR_FILE = os.path.join(log_dir, ".conversation_buffer")
    
    return log_dir

def log_entry(role, content):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
    
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        if role == "USER":
            f.write(f"\n## User Input - {timestamp}\n{content}\n")
        elif role == "ASSISTANT":
            f.write(f"\n## Assistant Response - {timestamp}\n{content}\n")
        elif role == "SYSTEM":
            f.write(f"\n## System Event - {timestamp}\n{content}\n")
        f.write("\n---\n")
    
    # Update state
    save_state({
        'last_update': timestamp,
        'last_role': role,
        'session_active': True
    })

def save_state(state):
    os.makedirs(os.path.dirname(STATE_FILE), exist_ok=True)
    with open(STATE_FILE, 'w') as f:
        json.dump(state, f)

def load_state():
    if os.path.exists(STATE_FILE):
        try:
            with open(STATE_FILE, 'r') as f:
                return json.load(f)
        except:
            pass
    return {}

def start_monitor():
    """Start real-time monitoring for conversation updates"""
    log_entry("SYSTEM", "Real-time conversation monitoring started")
    
    def monitor_loop():
        while True:
            try:
                if os.path.exists(MONITOR_FILE):
                    with open(MONITOR_FILE, 'r') as f:
                        lines = f.readlines()
                    
                    if lines:
                        # Process new conversation data
                        for line in lines:
                            if line.strip():
                                try:
                                    data = json.loads(line.strip())
                                    log_entry(data.get('role', 'UNKNOWN'), data.get('content', ''))
                                except:
                                    pass
                        
                        # Clear the buffer after processing
                        open(MONITOR_FILE, 'w').close()
                
                time.sleep(1)  # Check every second
            except KeyboardInterrupt:
                break
            except Exception as e:
                time.sleep(5)  # Wait longer on error
    
    # Run monitor in background
    monitor_thread = threading.Thread(target=monitor_loop, daemon=True)
    monitor_thread.start()
    
    return monitor_thread

def add_to_buffer(role, content):
    """Add conversation data to buffer for real-time processing"""
    os.makedirs(os.path.dirname(MONITOR_FILE), exist_ok=True)
    data = {'role': role, 'content': content, 'timestamp': datetime.now().isoformat()}
    
    with open(MONITOR_FILE, 'a') as f:
        f.write(json.dumps(data) + '\n')

def status():
    """Show current logger status"""
    state = load_state()
    print(f"Logger Status:")
    print(f"  Log File: {LOG_FILE}")
    print(f"  Last Update: {state.get('last_update', 'Never')}")
    print(f"  Session Active: {state.get('session_active', False)}")
    print(f"  Monitor File: {MONITOR_FILE}")
    
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: conversation_logger.py [USER|ASSISTANT|SYSTEM|MONITOR|STATUS|SETUP] [content]")
        sys.exit(1)
    
    command = sys.argv[1].upper()
    
    # Setup paths unless it's a setup command
    if command != "SETUP":
        setup_paths()
    
    if command == "MONITOR":
        print("Starting real-time conversation monitor...")
        thread = start_monitor()
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            log_entry("SYSTEM", "Real-time monitoring stopped")
            print("\nMonitoring stopped.")
    
    elif command == "STATUS":
        status()
    
    elif command == "SETUP":
        print("Reconfiguring log directory...")
        # Clear existing config to force new selection
        config = load_config()
        if 'log_directory' in config:
            del config['log_directory']
        save_config(config)
        log_dir = setup_paths()
        print(f"Log directory updated to: {log_dir}")
    
    elif command in ["USER", "ASSISTANT", "SYSTEM"]:
        if len(sys.argv) < 3:
            print("Content required for logging")
            sys.exit(1)
        content = " ".join(sys.argv[2:])
        log_entry(command, content)
        add_to_buffer(command, content)
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)
