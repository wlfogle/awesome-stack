use eframe::egui;
use std::process::Command;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use chrono::{DateTime, Utc};

#[derive(Debug, Clone, Serialize, Deserialize)]
struct ContainerInfo {
    id: u32,
    name: String,
    status: String,
    uptime: String,
    cpu_usage: f64,
    memory_usage: f64,
    category: String,
    description: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct VmInfo {
    id: u32,
    name: String,
    status: String,
    uptime: String,
    cpu_usage: f64,
    memory_usage: f64,
    description: String,
}

#[derive(Default)]
struct ProxmoxApp {
    containers: Vec<ContainerInfo>,
    vms: Vec<VmInfo>,
    selected_tab: Tab,
    last_refresh: Option<DateTime<Utc>>,
    status_message: String,
    operation_in_progress: HashMap<String, bool>,
}

#[derive(Default, PartialEq)]
enum Tab {
    #[default]
    Containers,
    VirtualMachines,
    SystemHealth,
}

impl ProxmoxApp {
    fn new(_cc: &eframe::CreationContext<'_>) -> Self {
        let mut app = Self::default();
        app.refresh_data();
        app
    }

    fn refresh_data(&mut self) {
        self.status_message = "Refreshing data...".to_string();
        
        // Fetch containers
        match self.get_containers() {
            Ok(containers) => {
                self.containers = containers;
                self.status_message = format!("Last updated: {}", Utc::now().format("%H:%M:%S"));
            }
            Err(e) => {
                self.status_message = format!("Error fetching containers: {}", e);
            }
        }
        
        // Fetch VMs
        match self.get_vms() {
            Ok(vms) => {
                self.vms = vms;
            }
            Err(e) => {
                self.status_message = format!("Error fetching VMs: {}", e);
            }
        }
        
        self.last_refresh = Some(Utc::now());
    }

    fn get_containers(&self) -> Result<Vec<ContainerInfo>, String> {
        let output = Command::new("ssh")
            .args(["proxmox", "pct", "list"])
            .output()
            .map_err(|e| format!("Failed to execute SSH command: {}", e))?;

        if !output.status.success() {
            return Err(format!("Command failed: {}", String::from_utf8_lossy(&output.stderr)));
        }

        let output_str = String::from_utf8_lossy(&output.stdout);
        let mut containers = Vec::new();
        
        for line in output_str.lines().skip(1) { // Skip header line
            if let Some(id_str) = line.split_whitespace().next() {
                if let Ok(id) = id_str.parse::<u32>() {
                    let status = self.get_container_status(id).unwrap_or("Unknown".to_string());
                    containers.push(ContainerInfo {
                        id,
                        name: self.get_container_display_name(id),
                        status,
                        uptime: "Unknown".to_string(),
                        cpu_usage: (id as f64 * 1.2) % 100.0,
                        memory_usage: (id as f64 * 15.0) % 1024.0,
                        category: self.get_container_category(id),
                        description: self.get_container_description(id),
                    });
                }
            }
        }
        
        Ok(containers)
    }

    fn get_vms(&self) -> Result<Vec<VmInfo>, String> {
        let output = Command::new("ssh")
            .args(["proxmox", "qm", "list"])
            .output()
            .map_err(|e| format!("Failed to execute SSH command: {}", e))?;

        if !output.status.success() {
            return Err(format!("Command failed: {}", String::from_utf8_lossy(&output.stderr)));
        }

        let output_str = String::from_utf8_lossy(&output.stdout);
        let mut vms = Vec::new();
        
        for line in output_str.lines().skip(1) { // Skip header line
            if let Some(id_str) = line.split_whitespace().next() {
                if let Ok(id) = id_str.parse::<u32>() {
                    let status = self.get_vm_status(id).unwrap_or("Unknown".to_string());
                    vms.push(VmInfo {
                        id,
                        name: self.get_vm_name(id),
                        status,
                        uptime: "Unknown".to_string(),
                        cpu_usage: (id as f64 * 1.5) % 100.0,
                        memory_usage: (id as f64 * 100.0) % 4096.0,
                        description: self.get_vm_description(id),
                    });
                }
            }
        }
        
        Ok(vms)
    }

    fn get_container_status(&self, container_id: u32) -> Result<String, String> {
        let output = Command::new("ssh")
            .args(["proxmox", "pct", "status", &container_id.to_string()])
            .output()
            .map_err(|e| format!("Failed to execute SSH command: {}", e))?;

        let status_str = String::from_utf8_lossy(&output.stdout);
        if status_str.contains("running") {
            Ok("Running".to_string())
        } else if status_str.contains("stopped") {
            Ok("Stopped".to_string())
        } else {
            Ok("Unknown".to_string())
        }
    }

    fn get_vm_status(&self, vm_id: u32) -> Result<String, String> {
        let output = Command::new("ssh")
            .args(["proxmox", "qm", "status", &vm_id.to_string()])
            .output()
            .map_err(|e| format!("Failed to execute SSH command: {}", e))?;

        let status_str = String::from_utf8_lossy(&output.stdout);
        if status_str.contains("running") {
            Ok("Running".to_string())
        } else if status_str.contains("stopped") {
            Ok("Stopped".to_string())
        } else {
            Ok("Unknown".to_string())
        }
    }

    fn start_container(&mut self, container_id: u32) {
        let key = format!("start-{}", container_id);
        self.operation_in_progress.insert(key.clone(), true);
        
        match Command::new("ssh")
            .args(["proxmox", "pct", "start", &container_id.to_string()])
            .output() {
            Ok(output) => {
                if output.status.success() {
                    self.status_message = format!("Container {} started successfully", container_id);
                    self.refresh_data();
                } else {
                    self.status_message = format!("Failed to start container {}: {}", container_id, String::from_utf8_lossy(&output.stderr));
                }
            }
            Err(e) => {
                self.status_message = format!("Error starting container {}: {}", container_id, e);
            }
        }
        
        self.operation_in_progress.remove(&key);
    }

    fn stop_container(&mut self, container_id: u32) {
        let key = format!("stop-{}", container_id);
        self.operation_in_progress.insert(key.clone(), true);
        
        match Command::new("ssh")
            .args(["proxmox", "pct", "stop", &container_id.to_string()])
            .output() {
            Ok(output) => {
                if output.status.success() {
                    self.status_message = format!("Container {} stopped successfully", container_id);
                    self.refresh_data();
                } else {
                    self.status_message = format!("Failed to stop container {}: {}", container_id, String::from_utf8_lossy(&output.stderr));
                }
            }
            Err(e) => {
                self.status_message = format!("Error stopping container {}: {}", container_id, e);
            }
        }
        
        self.operation_in_progress.remove(&key);
    }

    fn restart_container(&mut self, container_id: u32) {
        let key = format!("restart-{}", container_id);
        self.operation_in_progress.insert(key.clone(), true);
        
        match Command::new("ssh")
            .args(["proxmox", "pct", "restart", &container_id.to_string()])
            .output() {
            Ok(output) => {
                if output.status.success() {
                    self.status_message = format!("Container {} restarted successfully", container_id);
                    self.refresh_data();
                } else {
                    self.status_message = format!("Failed to restart container {}: {}", container_id, String::from_utf8_lossy(&output.stderr));
                }
            }
            Err(e) => {
                self.status_message = format!("Error restarting container {}: {}", container_id, e);
            }
        }
        
        self.operation_in_progress.remove(&key);
    }

    fn start_vm(&mut self, vm_id: u32) {
        let key = format!("start-vm-{}", vm_id);
        self.operation_in_progress.insert(key.clone(), true);
        
        match Command::new("ssh")
            .args(["proxmox", "qm", "start", &vm_id.to_string()])
            .output() {
            Ok(output) => {
                if output.status.success() {
                    self.status_message = format!("VM {} started successfully", vm_id);
                    self.refresh_data();
                } else {
                    self.status_message = format!("Failed to start VM {}: {}", vm_id, String::from_utf8_lossy(&output.stderr));
                }
            }
            Err(e) => {
                self.status_message = format!("Error starting VM {}: {}", vm_id, e);
            }
        }
        
        self.operation_in_progress.remove(&key);
    }

    fn stop_vm(&mut self, vm_id: u32) {
        let key = format!("stop-vm-{}", vm_id);
        self.operation_in_progress.insert(key.clone(), true);
        
        match Command::new("ssh")
            .args(["proxmox", "qm", "stop", &vm_id.to_string()])
            .output() {
            Ok(output) => {
                if output.status.success() {
                    self.status_message = format!("VM {} stopped successfully", vm_id);
                    self.refresh_data();
                } else {
                    self.status_message = format!("Failed to stop VM {}: {}", vm_id, String::from_utf8_lossy(&output.stderr));
                }
            }
            Err(e) => {
                self.status_message = format!("Error stopping VM {}: {}", vm_id, e);
            }
        }
        
        self.operation_in_progress.remove(&key);
    }

    fn get_container_display_name(&self, container_id: u32) -> String {
        match container_id {
            100 => "WireGuard".to_string(),
            101 => "Gluetun".to_string(),
            102 => "Flaresolverr".to_string(),
            103 => "Traefik".to_string(),
            104 => "Vaultwarden".to_string(),
            105 => "Valkey".to_string(),
            106 => "PostgreSQL".to_string(),
            107 => "Authentik".to_string(),
            210 => "Prowlarr".to_string(),
            211 => "Jackett".to_string(),
            212 => "QBittorrent".to_string(),
            214 => "Sonarr".to_string(),
            215 => "Radarr".to_string(),
            230 => "Plex".to_string(),
            231 => "Jellyfin".to_string(),
            _ => format!("CT-{}", container_id),
        }
    }

    fn get_container_category(&self, container_id: u32) -> String {
        match container_id {
            100..=199 => "Core Infrastructure".to_string(),
            210..=229 => "Essential Media Services".to_string(),
            230..=239 => "Media Servers".to_string(),
            240..=250 => "Enhancement Services".to_string(),
            260..=269 => "Monitoring & Analytics".to_string(),
            270..=279 => "Management & Utilities".to_string(),
            _ => "Other".to_string(),
        }
    }

    fn get_container_description(&self, container_id: u32) -> String {
        match container_id {
            100 => "VPN access and secure tunneling".to_string(),
            101 => "VPN client container for other services".to_string(),
            102 => "Cloudflare solver proxy".to_string(),
            103 => "Reverse proxy and load balancer".to_string(),
            210 => "Indexer manager and proxy".to_string(),
            214 => "TV series management".to_string(),
            215 => "Movie management".to_string(),
            230 => "Media server and streaming platform".to_string(),
            231 => "Open-source media server".to_string(),
            _ => "Service container".to_string(),
        }
    }

    fn get_vm_name(&self, vm_id: u32) -> String {
        match vm_id {
            500 => "Home Assistant".to_string(),
            611 => "Ziggy".to_string(),
            612 => "Bliss OS Android".to_string(),
            900 => "AI System".to_string(),
            _ => format!("VM-{}", vm_id),
        }
    }

    fn get_vm_description(&self, vm_id: u32) -> String {
        match vm_id {
            500 => "Home automation platform".to_string(),
            611 => "Media bridging and streaming VM".to_string(),
            612 => "Android emulation and testing environment".to_string(),
            900 => "Artificial intelligence services".to_string(),
            _ => "Virtual machine".to_string(),
        }
    }
}

impl eframe::App for ProxmoxApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::TopBottomPanel::top("top_panel").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading("🏠 Proxmox Infrastructure Manager");
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui.button("🔄 Refresh").clicked() {
                        self.refresh_data();
                    }
                });
            });
        });

        egui::TopBottomPanel::bottom("bottom_panel").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.label(&self.status_message);
                if let Some(last_refresh) = self.last_refresh {
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        ui.label(format!("Last refresh: {}", last_refresh.format("%H:%M:%S")));
                    });
                }
            });
        });

        egui::SidePanel::left("tabs_panel").resizable(false).exact_width(200.0).show(ctx, |ui| {
            ui.vertical(|ui| {
                ui.selectable_value(&mut self.selected_tab, Tab::Containers, "📦 Containers");
                ui.selectable_value(&mut self.selected_tab, Tab::VirtualMachines, "💻 Virtual Machines");
                ui.selectable_value(&mut self.selected_tab, Tab::SystemHealth, "📊 System Health");
            });
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            match self.selected_tab {
                Tab::Containers => self.show_containers_tab(ui),
                Tab::VirtualMachines => self.show_vms_tab(ui),
                Tab::SystemHealth => self.show_system_health_tab(ui),
            }
        });
    }
}

impl ProxmoxApp {
    fn show_containers_tab(&mut self, ui: &mut egui::Ui) {
        ui.heading("Container Management");
        
        egui::ScrollArea::vertical().show(ui, |ui| {
            for container in &self.containers.clone() {
                ui.group(|ui| {
                    ui.horizontal(|ui| {
                        ui.vertical(|ui| {
                            ui.strong(&container.name);
                            ui.label(format!("ID: {}", container.id));
                            ui.label(format!("Category: {}", container.category));
                            ui.label(&container.description);
                        });
                        
                        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                            let status_color = match container.status.as_str() {
                                "Running" => egui::Color32::GREEN,
                                "Stopped" => egui::Color32::RED,
                                _ => egui::Color32::GRAY,
                            };
                            
                            ui.colored_label(status_color, &container.status);
                            
                            ui.vertical(|ui| {
                                let start_enabled = container.status != "Running" && 
                                    !self.operation_in_progress.contains_key(&format!("start-{}", container.id));
                                let stop_enabled = container.status == "Running" && 
                                    !self.operation_in_progress.contains_key(&format!("stop-{}", container.id));
                                let restart_enabled = !self.operation_in_progress.contains_key(&format!("restart-{}", container.id));
                                
                                ui.horizontal(|ui| {
                                    if ui.add_enabled(start_enabled, egui::Button::new("▶ Start")).clicked() {
                                        let container_id = container.id;
                                        self.start_container(container_id);
                                    }
                                    if ui.add_enabled(stop_enabled, egui::Button::new("⏹ Stop")).clicked() {
                                        let container_id = container.id;
                                        self.stop_container(container_id);
                                    }
                                    if ui.add_enabled(restart_enabled, egui::Button::new("🔄 Restart")).clicked() {
                                        let container_id = container.id;
                                        self.restart_container(container_id);
                                    }
                                });
                            });
                        });
                    });
                });
                ui.add_space(10.0);
            }
        });
    }

    fn show_vms_tab(&mut self, ui: &mut egui::Ui) {
        ui.heading("Virtual Machine Management");
        
        egui::ScrollArea::vertical().show(ui, |ui| {
            for vm in &self.vms.clone() {
                ui.group(|ui| {
                    ui.horizontal(|ui| {
                        ui.vertical(|ui| {
                            ui.strong(&vm.name);
                            ui.label(format!("ID: {}", vm.id));
                            ui.label(&vm.description);
                        });
                        
                        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                            let status_color = match vm.status.as_str() {
                                "Running" => egui::Color32::GREEN,
                                "Stopped" => egui::Color32::RED,
                                _ => egui::Color32::GRAY,
                            };
                            
                            ui.colored_label(status_color, &vm.status);
                            
                            ui.vertical(|ui| {
                                let start_enabled = vm.status != "Running" && 
                                    !self.operation_in_progress.contains_key(&format!("start-vm-{}", vm.id));
                                let stop_enabled = vm.status == "Running" && 
                                    !self.operation_in_progress.contains_key(&format!("stop-vm-{}", vm.id));
                                
                                ui.horizontal(|ui| {
                                    if ui.add_enabled(start_enabled, egui::Button::new("▶ Start")).clicked() {
                                        let vm_id = vm.id;
                                        self.start_vm(vm_id);
                                    }
                                    if ui.add_enabled(stop_enabled, egui::Button::new("⏹ Stop")).clicked() {
                                        let vm_id = vm.id;
                                        self.stop_vm(vm_id);
                                    }
                                });
                            });
                        });
                    });
                });
                ui.add_space(10.0);
            }
        });
    }

    fn show_system_health_tab(&mut self, ui: &mut egui::Ui) {
        ui.heading("System Health Overview");
        
        ui.separator();
        
        ui.horizontal(|ui| {
            ui.vertical(|ui| {
                ui.strong("Containers");
                ui.label(format!("Total: {}", self.containers.len()));
                let running_containers = self.containers.iter().filter(|c| c.status == "Running").count();
                ui.label(format!("Running: {}", running_containers));
                ui.label(format!("Stopped: {}", self.containers.len() - running_containers));
            });
            
            ui.separator();
            
            ui.vertical(|ui| {
                ui.strong("Virtual Machines");
                ui.label(format!("Total: {}", self.vms.len()));
                let running_vms = self.vms.iter().filter(|v| v.status == "Running").count();
                ui.label(format!("Running: {}", running_vms));
                ui.label(format!("Stopped: {}", self.vms.len() - running_vms));
            });
        });
    }
}

fn main() -> Result<(), eframe::Error> {
    tracing_subscriber::fmt::init();
    
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1200.0, 800.0])
            .with_min_inner_size([800.0, 600.0]),
        ..Default::default()
    };
    
    eframe::run_native(
        "Proxmox Infrastructure Manager",
        options,
        Box::new(|cc| Box::new(ProxmoxApp::new(cc))),
    )
}

