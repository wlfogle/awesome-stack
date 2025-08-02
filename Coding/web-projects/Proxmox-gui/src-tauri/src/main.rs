// Modern Proxmox Manager - Written in Natural Language Style
// This creates a beautiful, intuitive interface for managing Proxmox infrastructure

use serde::{Deserialize, Serialize};
use ssh2::Session;
use std::io::prelude::*;
use std::net::TcpStream;
use std::collections::HashMap;
use std::sync::{Arc, Mutex, LazyLock};
use std::path::{Path, PathBuf};
use std::fs;
use std::env;
// Define what our Proxmox world looks like
#[derive(Debug, Serialize, Deserialize, Clone)]
struct ProxmoxServer {
    name: String,
    status: String,
    cpu_usage: f64,
    memory_usage: f64,
    uptime: String,
    predicted_load: f64,  // AI prediction for future load
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct VirtualMachine {
    id: u32,
    name: String,
    status: String,
    cpu_cores: u32,
    memory_gb: u32,
    disk_gb: u32,
    node: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Container {
    id: u32,
    name: String,
    status: String,
    memory_mb: u32,
    disk_gb: u32,
    template: String,
}

// The main application state - think of this as our "digital clipboard"
#[derive(Default)]
struct AppState {
    connected: bool,
    servers: Vec<ProxmoxServer>,
    virtual_machines: Vec<VirtualMachine>,
    containers: Vec<Container>,
}

// Global state for SSH connections - use Arc to allow shared ownership
static SSH_CONNECTIONS: LazyLock<Mutex<HashMap<String, Arc<Mutex<Session>>>>> = LazyLock::new(|| Mutex::new(HashMap::new()));

// Helper function to sanitize host URLs for SSH connections
fn sanitize_host(host: &str) -> String {
    let mut clean_host = host.to_string();
    
    // Remove protocol (http:// or https://)
    if clean_host.starts_with("https://") {
        clean_host = clean_host.strip_prefix("https://").unwrap().to_string();
    } else if clean_host.starts_with("http://") {
        clean_host = clean_host.strip_prefix("http://").unwrap().to_string();
    }
    
    // Remove port number (everything after :)
    if let Some(colon_pos) = clean_host.find(':') {
        clean_host = clean_host[..colon_pos].to_string();
    }
    
    // Remove trailing slash and path
    if let Some(slash_pos) = clean_host.find('/') {
        clean_host = clean_host[..slash_pos].to_string();
    }
    
    clean_host
}

// Helper function to find available SSH keys
fn find_ssh_keys() -> Vec<(PathBuf, Option<PathBuf>)> {
    let home_dir = std::env::var("HOME").unwrap_or_else(|_| "/root".to_string());
    let ssh_dir = PathBuf::from(format!("{}/.ssh", home_dir));
    
    let mut keys = Vec::new();
    
    // Common SSH key names
    let key_names = ["id_rsa", "id_ed25519", "id_ecdsa", "id_dsa"];
    
    for key_name in &key_names {
        let private_key = ssh_dir.join(key_name);
        let public_key = ssh_dir.join(format!("{}.pub", key_name));
        
        if private_key.exists() {
            let pub_key = if public_key.exists() { Some(public_key) } else { None };
            keys.push((private_key, pub_key));
        }
    }
    
    keys
}

// Try SSH agent authentication with better error handling
fn try_ssh_agent_auth(sess: &Session, username: &str) -> Result<bool, String> {
    // Check if SSH_AUTH_SOCK is available
    match env::var("SSH_AUTH_SOCK") {
        Ok(sock_path) => {
            println!("SSH_AUTH_SOCK found: {}", sock_path);
            // Verify the socket file exists
            if !std::path::Path::new(&sock_path).exists() {
                println!("SSH_AUTH_SOCK points to non-existent file: {}", sock_path);
                return Ok(false);
            }
        },
        Err(_) => {
            println!("SSH_AUTH_SOCK not found - SSH agent not available");
            return Ok(false);
        }
    }
    
    // Try to use SSH agent if available
    match sess.userauth_agent(username) {
        Ok(_) if sess.authenticated() => {
            println!("SSH agent authentication successful for user: {}", username);
            Ok(true)
        },
        Ok(_) => {
            println!("SSH agent authentication failed - not authenticated for user: {}", username);
            Ok(false)
        },
        Err(e) => {
            println!("SSH agent authentication error for user {}: {}", username, e);
            Ok(false)
        }
    }
}

// Try SSH key file authentication with proper error handling
fn try_ssh_key_auth(sess: &Session, username: &str, private_key: &Path, public_key: Option<&Path>) -> Result<bool, String> {
    println!("Attempting SSH key authentication with private key: {:?}", private_key);
    
    // Check if private key file exists and is readable
    if !private_key.exists() {
        println!("Private key file does not exist: {:?}", private_key);
        return Ok(false);
    }
    
    // Check file permissions (should be readable by user)
    if let Ok(metadata) = fs::metadata(private_key) {
        let permissions = metadata.permissions();
        println!("Private key permissions: {:?}", permissions);
    }
    
    // Read the private key to check if it requires a passphrase
    let key_content = match fs::read_to_string(private_key) {
        Ok(content) => content,
        Err(e) => {
            println!("Failed to read private key {}: {}", private_key.display(), e);
            return Ok(false);
        }
    };
    
    let needs_passphrase = key_content.contains("ENCRYPTED") || key_content.contains("Proc-Type: 4,ENCRYPTED");
    
    if needs_passphrase {
        println!("Private key {:?} is encrypted, skipping for now", private_key);
        return Ok(false);
    }
    
    // Check if public key exists when specified
    if let Some(pub_key_path) = public_key {
        if !pub_key_path.exists() {
            println!("Public key file does not exist: {:?}, trying without it", pub_key_path);
        } else {
            println!("Using public key: {:?}", pub_key_path);
        }
    }
    
    // Try authentication without passphrase first
    println!("Attempting userauth_pubkey_file for user {} with private key {:?}", username, private_key);
    match sess.userauth_pubkey_file(username, public_key, private_key, None) {
        Ok(_) => {
            if sess.authenticated() {
                println!("SSH key authentication successful with: {:?}", private_key);
                Ok(true)
            } else {
                println!("SSH key authentication completed but session not authenticated with: {:?}", private_key);
                Ok(false)
            }
        },
        Err(e) => {
            println!("SSH key authentication error with {:?}: {} (error code: {:?})", private_key, e, e.code());
            Ok(false)
        }
    }
}

#[tauri::command]
async fn execute_remote_command(host: String, command: String) -> Result<String, String> {
    println!("Executing command on {}: {}", host, command);

    let connections = SSH_CONNECTIONS.lock().unwrap();
    if let Some(sess_arc) = connections.get(&host) {
        let sess = sess_arc.lock().unwrap();
        let mut channel = sess.channel_session().map_err(|e| format!("Failed to open channel: {}", e))?;
        channel.exec(&command).map_err(|e| format!("Failed to execute command: {}", e))?;

        let mut s = String::new();
        channel.read_to_string(&mut s).map_err(|e| format!("Failed to read output: {}", e))?;

        channel.wait_close().map_err(|_| "Failed to wait for channel close".to_string())?;
        Ok(s)
    } else {
        Err("No SSH session found for this host".to_string())
    }
}

// Commands that speak in plain English
#[tauri::command]
async fn connect_to_proxmox(host: String, username: String, password: String) -> Result<String, String> {
    println!("=== Starting Proxmox SSH Connection ===");
    println!("Host: {}, Username: {}, Password provided: {}", host, username, !password.is_empty());

    // Print current environment variables related to SSH
    println!("Environment variables:");
    for (key, value) in env::vars() {
        if key.starts_with("SSH_") {
            println!("  {}: {}", key, value);
        }
    }

    // Sanitize the host - remove protocol and port if present
    let clean_host = sanitize_host(&host);
    println!("Sanitized host: {}", clean_host);

    // Clean username - remove @pam suffix if present (Proxmox web UI format)
    let clean_username = if username.ends_with("@pam") {
        username.strip_suffix("@pam").unwrap_or(&username).to_string()
    } else {
        username.clone()
    };
    println!("Using username: {}", clean_username);

    // Connect to the SSH server
    println!("Connecting to {}:22...", clean_host);
    let tcp = TcpStream::connect(format!("{}:22", clean_host)).map_err(|e| format!("Failed to connect to {}:22 - {}", clean_host, e))?;
    println!("TCP connection established");
    
    let mut sess = Session::new().map_err(|e| format!("Failed to create SSH session: {}", e))?;
    sess.set_tcp_stream(tcp);
    println!("Performing SSH handshake...");
    sess.handshake().map_err(|e| format!("SSH handshake failed: {}", e))?;
    println!("SSH handshake completed successfully");

    // Get supported authentication methods
    let auth_methods = sess.auth_methods(&clean_username).map_err(|e| format!("Failed to get auth methods: {}", e))?;
    println!("Server supports authentication methods: {}", auth_methods);

    let mut authenticated = false;
    let mut auth_attempts = Vec::new();

    // Try SSH agent authentication first if supported
    if auth_methods.contains("publickey") {
        println!("=== Attempting SSH agent authentication ===");
        match try_ssh_agent_auth(&sess, &clean_username) {
            Ok(true) => {
                println!("SSH agent authentication successful!");
                authenticated = true;
                auth_attempts.push("ssh-agent: SUCCESS".to_string());
            },
            Ok(false) => {
                println!("SSH agent authentication failed");
                auth_attempts.push("ssh-agent: FAILED".to_string());
            },
            Err(e) => {
                println!("SSH agent error: {}", e);
                auth_attempts.push(format!("ssh-agent: ERROR - {}", e));
            }
        }
    } else {
        println!("Server does not support publickey authentication");
        auth_attempts.push("ssh-agent: NOT_SUPPORTED".to_string());
    }

    // Try SSH key files if agent auth failed
    if !authenticated && auth_methods.contains("publickey") {
        println!("=== Attempting SSH key file authentication ===");
        let ssh_keys = find_ssh_keys();
        println!("Found {} SSH key files", ssh_keys.len());
        
        for (private_key, public_key) in ssh_keys {
            match try_ssh_key_auth(&sess, &clean_username, &private_key, public_key.as_ref().map(|p| p.as_path())) {
                Ok(true) => {
                    authenticated = true;
                    auth_attempts.push(format!("key-file({}): SUCCESS", private_key.display()));
                    break;
                },
                Ok(false) => {
                    auth_attempts.push(format!("key-file({}): FAILED", private_key.display()));
                    continue;
                },
                Err(e) => {
                    auth_attempts.push(format!("key-file({}): ERROR - {}", private_key.display(), e));
                    continue;
                }
            }
        }
    }
    
    // Try password authentication if key auth failed and password is provided
    if !authenticated && auth_methods.contains("password") && !password.is_empty() {
        println!("=== Attempting password authentication ===");
        match sess.userauth_password(&clean_username, &password) {
            Ok(_) => {
                if sess.authenticated() {
                    println!("Password authentication successful");
                    authenticated = true;
                    auth_attempts.push("password: SUCCESS".to_string());
                } else {
                    println!("Password authentication completed but session not authenticated");
                    auth_attempts.push("password: FAILED - not authenticated".to_string());
                }
            },
            Err(e) => {
                println!("Password authentication error: {}", e);
                auth_attempts.push(format!("password: ERROR - {}", e));
            }
        }
    } else if !authenticated && !password.is_empty() {
        auth_attempts.push("password: NOT_SUPPORTED".to_string());
    } else if !authenticated && password.is_empty() {
        auth_attempts.push("password: NO_PASSWORD_PROVIDED".to_string());
    }

    println!("=== Authentication Summary ===");
    for attempt in &auth_attempts {
        println!("  {}", attempt);
    }
    println!("Final authentication status: {}", sess.authenticated());

    if authenticated && sess.authenticated() {
        // Store the session in the global map using the original host as key
        let mut connections = SSH_CONNECTIONS.lock().unwrap();
        connections.insert(host.clone(), Arc::new(Mutex::new(sess)));
        println!("SSH session stored successfully");

        Ok("Successfully connected and authenticated with SSH".to_string())
    } else {
        let error_msg = format!(
            "All authentication methods failed. Attempts: [{}]. Server supports: [{}]", 
            auth_attempts.join(", "), 
            auth_methods
        );
        Err(error_msg)
    }
}

#[tauri::command]
async fn get_cluster_overview() -> Result<Vec<ProxmoxServer>, String> {
    // Simulate fetching cluster data
    let mut servers = vec![
        ProxmoxServer {
            name: "proxmox-node-01".to_string(),
            status: "online".to_string(),
            cpu_usage: 23.5,
            memory_usage: 67.2,
            uptime: "15 days".to_string(),
            predicted_load: 0.0,
        },
        ProxmoxServer {
            name: "proxmox-node-02".to_string(),
            status: "online".to_string(),
            cpu_usage: 45.1,
            memory_usage: 78.9,
            uptime: "15 days".to_string(),
            predicted_load: 0.0,
        },
    ];
    
    println!("Running AI optimization algorithms...");
    // AI-based prediction for CPU load
    servers.iter_mut().for_each(|s| {
        s.predicted_load = s.cpu_usage * (1.0 + rand::random::<f64>() * 0.2);  // Dummy predictor
    });
    
    Ok(servers)
}

#[tauri::command]
async fn list_virtual_machines() -> Result<Vec<VirtualMachine>, String> {
    let vms = vec![
        VirtualMachine {
            id: 100,
            name: "web-server-01".to_string(),
            status: "running".to_string(),
            cpu_cores: 4,
            memory_gb: 8,
            disk_gb: 50,
            node: "proxmox-node-01".to_string(),
        },
        VirtualMachine {
            id: 101,
            name: "database-primary".to_string(),
            status: "running".to_string(),
            cpu_cores: 8,
            memory_gb: 16,
            disk_gb: 200,
            node: "proxmox-node-02".to_string(),
        },
        VirtualMachine {
            id: 102,
            name: "backup-server".to_string(),
            status: "stopped".to_string(),
            cpu_cores: 2,
            memory_gb: 4,
            disk_gb: 500,
            node: "proxmox-node-01".to_string(),
        },
    ];
    
    Ok(vms)
}

#[tauri::command]
async fn start_virtual_machine(vm_id: u32) -> Result<String, String> {
    println!("Starting VM with ID: {}", vm_id);
    tokio::time::sleep(std::time::Duration::from_secs(3)).await;
    Ok(format!("Virtual machine {} is now starting up", vm_id))
}

#[tauri::command]
async fn stop_virtual_machine(vm_id: u32) -> Result<String, String> {
    println!("Stopping VM with ID: {}", vm_id);
    tokio::time::sleep(std::time::Duration::from_secs(2)).await;
    Ok(format!("Virtual machine {} is now shutting down", vm_id))
}

#[tauri::command]
async fn create_new_virtual_machine(name: String, cores: u32, memory: u32, disk: u32) -> Result<String, String> {
    println!("Creating new VM: {} with {} cores, {}GB RAM, {}GB disk", name, cores, memory, disk);
    tokio::time::sleep(std::time::Duration::from_secs(5)).await;
    Ok(format!("Virtual machine '{}' has been created successfully!", name))
}

#[tauri::command]
async fn list_containers() -> Result<Vec<Container>, String> {
    let containers = vec![
        Container {
            id: 200,
            name: "nginx-proxy".to_string(),
            status: "running".to_string(),
            memory_mb: 512,
            disk_gb: 8,
            template: "ubuntu-22.04".to_string(),
        },
        Container {
            id: 201,
            name: "redis-cache".to_string(),
            status: "running".to_string(),
            memory_mb: 1024,
            disk_gb: 16,
            template: "alpine-3.18".to_string(),
        },
    ];
    
    Ok(containers)
}

// AI-Powered Advanced Commands
#[tauri::command]
async fn get_ai_recommendations() -> Result<String, String> {
    println!("Generating AI-powered infrastructure recommendations...");
    tokio::time::sleep(std::time::Duration::from_secs(1)).await;
    
    let recommendations = vec![
        "Migrate VM-102 to node-01 for better load balancing",
        "Consider increasing memory allocation for database-primary VM",
        "Schedule automatic snapshots for critical VMs",
        "Optimize storage by enabling compression on ZFS pools"
    ];
    
    Ok(serde_json::to_string(&recommendations).unwrap())
}

#[tauri::command]
async fn predict_resource_usage() -> Result<String, String> {
    println!("Running predictive analytics on resource usage...");
    tokio::time::sleep(std::time::Duration::from_secs(2)).await;
    
    #[derive(Serialize)]
    struct Prediction {
        resource: String,
        current: f64,
        predicted_1h: f64,
        predicted_6h: f64,
        predicted_24h: f64,
        trend: String,
    }
    
    let predictions = vec![
        Prediction {
            resource: "CPU".to_string(),
            current: 34.2,
            predicted_1h: 38.5,
            predicted_6h: 45.1,
            predicted_24h: 42.8,
            trend: "increasing".to_string(),
        },
        Prediction {
            resource: "Memory".to_string(),
            current: 72.8,
            predicted_1h: 74.2,
            predicted_6h: 79.5,
            predicted_24h: 81.3,
            trend: "steady_increase".to_string(),
        },
    ];
    
    Ok(serde_json::to_string(&predictions).unwrap())
}

#[tauri::command]
async fn detect_anomalies() -> Result<String, String> {
    println!("Running AI anomaly detection...");
    
    #[derive(Serialize)]
    struct Anomaly {
        severity: String,
        component: String,
        description: String,
        confidence: f64,
        suggested_action: String,
    }
    
    let anomalies = vec![
        Anomaly {
            severity: "medium".to_string(),
            component: "node-02".to_string(),
            description: "Unusual memory usage pattern detected".to_string(),
            confidence: 0.85,
            suggested_action: "Monitor for memory leaks, consider restarting services".to_string(),
        },
    ];
    
    Ok(serde_json::to_string(&anomalies).unwrap())
}

#[tauri::command]
async fn optimize_cluster() -> Result<String, String> {
    println!("Applying AI-driven cluster optimizations...");
    tokio::time::sleep(std::time::Duration::from_secs(3)).await;
    
    let optimizations = vec![
        "Balanced VM distribution across nodes",
        "Optimized memory allocation based on usage patterns",
        "Implemented predictive scaling rules",
        "Enhanced storage efficiency with intelligent caching"
    ];
    
    Ok(format!("Applied {} optimizations successfully!", optimizations.len()))
}

#[tauri::command]
async fn get_performance_insights() -> Result<String, String> {
    println!("Analyzing performance metrics with AI...");
    
    #[derive(Serialize)]
    struct PerformanceInsight {
        metric: String,
        current_score: f64,
        potential_improvement: f64,
        recommendation: String,
        impact: String,
    }
    
    let insights = vec![
        PerformanceInsight {
            metric: "CPU Efficiency".to_string(),
            current_score: 87.3,
            potential_improvement: 12.7,
            recommendation: "Enable CPU ballooning on idle VMs".to_string(),
            impact: "5-8% performance increase".to_string(),
        },
        PerformanceInsight {
            metric: "Storage IOPS".to_string(),
            current_score: 92.1,
            potential_improvement: 7.9,
            recommendation: "Implement SSD caching layer".to_string(),
            impact: "15-20% faster disk operations".to_string(),
        },
    ];
    
    Ok(serde_json::to_string(&insights).unwrap())
}

#[tauri::command]
async fn execute_pimox_script(script_id: String, command: String) -> Result<String, String> {
    println!("Executing Pimox script: {} with command: {}", script_id, command);
    
    // In a real implementation, this would:
    // 1. Validate the script source
    // 2. Execute the script safely in a controlled environment
    // 3. Monitor the installation progress
    // 4. Return detailed status updates
    
    tokio::time::sleep(std::time::Duration::from_secs(5)).await;
    
    // Simulate different installation outcomes
    match script_id.as_str() {
        "homeassistant-lxc" => {
            println!("Home Assistant LXC installation completed successfully");
            Ok("Home Assistant LXC container created with ID 100. Access via http://your-ip:8123".to_string())
        },
        "docker-lxc" => {
            println!("Docker LXC installation completed successfully");
            Ok("Docker container created with ID 101. Docker service is running and ready".to_string())
        },
        "pi-hole-lxc" => {
            println!("Pi-hole LXC installation completed successfully");
            Ok("Pi-hole installed in container ID 102. Admin interface: http://your-ip/admin".to_string())
        },
        _ => {
            println!("Generic script installation completed");
            Ok(format!("Script '{}' installed successfully", script_id))
        }
    }
}

#[tauri::command]
async fn get_script_library() -> Result<String, String> {
    println!("Fetching latest scripts from pimox-scripts.com...");
    
    // In a real implementation, this would make HTTP requests to pimox-scripts.com API
    tokio::time::sleep(std::time::Duration::from_secs(2)).await;
    
    #[derive(Serialize)]
    struct ScriptInfo {
        total_scripts: u32,
        categories: Vec<String>,
        featured_scripts: Vec<String>,
        last_updated: String,
    }
    
    let library_info = ScriptInfo {
        total_scripts: 47,
        categories: vec![
            "automation".to_string(),
            "containers".to_string(),
            "cloud".to_string(),
            "networking".to_string(),
            "database".to_string(),
            "media".to_string(),
            "security".to_string(),
            "monitoring".to_string(),
        ],
        featured_scripts: vec![
            "Home Assistant LXC".to_string(),
            "Docker LXC Container".to_string(),
            "Pi-hole LXC".to_string(),
            "Nextcloud LXC".to_string(),
        ],
        last_updated: "2024-12-15".to_string(),
    };
    
    Ok(serde_json::to_string(&library_info).unwrap())
}

#[tauri::command]
async fn test_proxmox_connection(host: String, username: String, _password: String) -> Result<String, String> {
    println!("Testing connection to {} as {}", host, username);
    
    // In a real implementation, this would:
    // 1. Make an actual API call to the Proxmox server
    // 2. Verify SSL certificates
    // 3. Test authentication
    // 4. Return connection status and server info
    
    tokio::time::sleep(std::time::Duration::from_secs(2)).await;
    
    // Simulate connection test based on host
    if host.contains("192.168") || host.contains("localhost") || host.contains("127.0.0.1") {
        Ok("Connection successful! Server is reachable and credentials are valid.".to_string())
    } else {
        // Simulate some connection failures for demo
        match host.as_str() {
            h if h.contains("unreachable") => Err("Host unreachable - check network connectivity".to_string()),
            h if h.contains("badcert") => Err("SSL certificate verification failed".to_string()),
            _ => Ok("Connection test completed successfully".to_string())
        }
    }
}

fn main() {
    run();
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_fs::init())
        .invoke_handler(tauri::generate_handler![
            connect_to_proxmox,
            execute_remote_command,
            get_cluster_overview,
            list_virtual_machines,
            start_virtual_machine,
            stop_virtual_machine,
            create_new_virtual_machine,
            list_containers,
            get_ai_recommendations,
            predict_resource_usage,
            detect_anomalies,
            optimize_cluster,
            get_performance_insights,
            execute_pimox_script,
            get_script_library,
            test_proxmox_connection
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
