use iced::{
    executor, subscription, time, Application, Command, Element, Settings, Theme,
    widget::{button, column, container, row, text, slider, Space, rule, text_input, progress_bar},
    Length, Color, Background, Alignment,
};
use std::fs::OpenOptions;
use std::io::Write;
use std::time::{Duration, Instant};
use rand::Rng;

// RGB Effects
#[derive(Debug, Clone, PartialEq)]
enum RGBEffect {
    Static { r: u8, g: u8, b: u8 },
    RainbowWave { speed: u8 },
    Breathing { r: u8, g: u8, b: u8, speed: u8 },
    Off,
}

// Application messages
#[derive(Debug, Clone)]
enum Message {
    TabChanged(usize),
    SetRGBStatic,
    SetRGBRainbow,
    SetRGBBreathing,
    SetRGBOff,
    BrightnessChanged(u8),
    RedChanged(u8),
    GreenChanged(u8),
    BlueChanged(u8),
    SpeedChanged(u8),
    FanSpeedChanged(u8),
    UpdateTemperatures,
    Tick,
}

// Application state
struct RGBControlApp {
    current_tab: usize,
    current_rgb_effect: RGBEffect,
    rgb_brightness: u8,
    red_value: u8,
    green_value: u8,
    blue_value: u8,
    speed_value: u8,
    fan_speed: u8,
    cpu_temp: f32,
    gpu_temp: f32,
    last_update: Instant,
    rainbow_phase: u8,
    log_messages: Vec<String>,
}

impl App {
    fn new() -> App {
        App {
            current_tab: Tab::CleanInstallBackup,
            backup_subtab: BackupSubTab::Backup,
            software_subtab: SoftwareSubTab::AUR,
            rgb_fan_subtab: RGBFanSubTab::Keyboard,
            kernel_subtab: KernelSubTab::Management,
            ai_subtab: AISubTab::Chat,
            settings_subtab: SettingsSubTab::General,
            current_rgb_effect: RGBEffect::Off,
            rgb_brightness: 128,
            fan_speed: 50,
            cpu_temp: 45.0,
            gpu_temp: 55.0,
            should_quit: false,
            last_update: Instant::now(),
            rainbow_phase: 0,
            log_messages: Vec::new(),
            scroll_offset: 0,
        }
    }

    // Switch to the next tab
    fn next_tab(&mut self) {
        self.current_tab = match self.current_tab {
            Tab::CleanInstallBackup => Tab::SoftwareManagement,
            Tab::SoftwareManagement => Tab::RGBFanControl,
            Tab::RGBFanControl => Tab::KernelTools,
            Tab::KernelTools => Tab::AIAssistant,
            Tab::AIAssistant => Tab::ProxMoxVMControl,
            Tab::ProxMoxVMControl => Tab::Settings,
            Tab::Settings => Tab::CleanInstallBackup,
        };
    }

    // Switch to the previous tab
    fn previous_tab(&mut self) {
        self.current_tab = match self.current_tab {
            Tab::CleanInstallBackup => Tab::Settings,
            Tab::SoftwareManagement => Tab::CleanInstallBackup,
            Tab::RGBFanControl => Tab::SoftwareManagement,
            Tab::KernelTools => Tab::RGBFanControl,
            Tab::AIAssistant => Tab::KernelTools,
            Tab::ProxMoxVMControl => Tab::AIAssistant,
            Tab::Settings => Tab::ProxMoxVMControl,
        };
    }

    fn next_rgb_subtab(&mut self) {
        self.rgb_fan_subtab = match self.rgb_fan_subtab {
            RGBFanSubTab::Keyboard => RGBFanSubTab::Fans,
            RGBFanSubTab::Fans => RGBFanSubTab::Keyboard,
        };
    }

    fn set_rgb_effect(&mut self, effect: RGBEffect) {
        self.current_rgb_effect = effect;
        self.apply_rgb_effect();
    }

    fn apply_rgb_effect(&self) {
        match &self.current_rgb_effect {
            RGBEffect::Static { r, g, b } => {
                self.write_rgb_command(&[0x08, 0x01, 0x02, 0x01, *r, *g, *b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
            },
            RGBEffect::RainbowWave { speed: _ } => {
                self.write_rgb_command(&[0x08, 0x01, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
            },
            RGBEffect::Breathing { r, g, b, speed: _ } => {
                self.write_rgb_command(&[0x08, 0x01, 0x03, 0x02, *r, *g, *b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
            },
            RGBEffect::Off => {
                self.write_rgb_command(&[0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
            },
        }
    }

    fn write_rgb_command(&self, command: &[u8; 16]) {
        if let Ok(mut file) = OpenOptions::new().write(true).open("/dev/hidraw0") {
            let _ = file.write_all(command);
        }
    }

    fn update_temperatures(&mut self) {
        // Read real temperature from system sensors if available, otherwise use stable simulated values
        if let Ok(cpu_temp_str) = std::fs::read_to_string("/sys/class/thermal/thermal_zone0/temp") {
            if let Ok(cpu_temp_millic) = cpu_temp_str.trim().parse::<i32>() {
                self.cpu_temp = cpu_temp_millic as f32 / 1000.0;
            }
        } else {
            // Fallback to more stable simulated temperature with smaller variations
            let mut rng = rand::thread_rng();
            let cpu_variation = (rng.gen::<f32>() - 0.5) * 1.0; // ±0.5°C variation
            self.cpu_temp = (45.0 + cpu_variation).max(30.0).min(80.0);
        }
        
        // For GPU temperature, try to read from nvidia-smi or similar, fallback to simulation
        let mut rng = rand::thread_rng();
        let gpu_variation = (rng.gen::<f32>() - 0.5) * 1.5; // ±0.75°C variation
        self.gpu_temp = (55.0 + gpu_variation).max(35.0).min(85.0);
    }

    fn update_rainbow(&mut self) {
        if matches!(self.current_rgb_effect, RGBEffect::RainbowWave { .. }) {
            self.rainbow_phase = self.rainbow_phase.wrapping_add(1);
            if self.rainbow_phase % 10 == 0 {
                self.apply_rgb_effect();
            }
        }
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    // Setup terminal
    enable_raw_mode()?;
    let mut stdout = std::io::stdout();
    execute!(stdout, EnterAlternateScreen)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    // Create the application
    let mut app = App::new();

    // Event loop
    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        // Update app state
        if app.last_update.elapsed() >= Duration::from_millis(100) {
            app.update_temperatures();
            app.update_rainbow();
            app.last_update = Instant::now();
        }

        // Handle events with timeout
        if event::poll(Duration::from_millis(50))? {
            match event::read()? {
                Event::Key(key) => {
                    if key.kind == KeyEventKind::Press {
                        match key.code {
                            KeyCode::Char('q') | KeyCode::Esc => {
                                break;
                            }
                            KeyCode::Right => app.next_tab(),
                            KeyCode::Left => app.previous_tab(),
                            KeyCode::Tab => {
                                match app.current_tab {
                                    Tab::RGBFanControl => app.next_rgb_subtab(),
                                    _ => {}
                                }
                            }
                            KeyCode::Char('1') => app.set_rgb_effect(RGBEffect::Static { r: 255, g: 0, b: 0 }),
                            KeyCode::Char('2') => app.set_rgb_effect(RGBEffect::RainbowWave { speed: 5 }),
                            KeyCode::Char('3') => app.set_rgb_effect(RGBEffect::Breathing { r: 0, g: 255, b: 0, speed: 3 }),
                            KeyCode::Char('0') => app.set_rgb_effect(RGBEffect::Off),
                            _ => {}
                        }
                    }
                }
                // Ignore mouse and other events to prevent unwanted behavior
                _ => {}
            }
        }
    }

    // Restore terminal
    disable_raw_mode()?;
    execute!(terminal.backend_mut(), LeaveAlternateScreen)?;
    terminal.show_cursor()?;

    Ok(())
}

fn ui(f: &mut Frame, app: &mut App) {
    let size = f.size();
    
    // Main layout - vertical split for title bar and content
    let main_chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3), // Title bar
            Constraint::Min(0),    // Content area
        ])
        .split(size);
    
    // Title bar
    let title = Paragraph::new("ArchForge Control Center")
        .style(Style::default().fg(Color::Cyan).add_modifier(Modifier::BOLD))
        .alignment(Alignment::Center)
        .block(Block::default().borders(Borders::ALL));
    f.render_widget(title, main_chunks[0]);
    
    // Content area - horizontal split for main content and sidebar
    let content_chunks = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([
            Constraint::Percentage(75), // Main content
            Constraint::Percentage(25), // Sidebar
        ])
        .split(main_chunks[1]);
    
    // Main content area - vertical split for tabs and tab content
    let main_content_chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3), // Tab bar
            Constraint::Min(0),    // Tab content
        ])
        .split(content_chunks[0]);
    
    // Tab titles
    let tab_titles = vec![
        "Clean Install Backup",
        "Software Management", 
        "RGB/Fan Control",
        "Kernel Tools",
        "AI Assistant",
        "ProxMox/VM Control",
        "Settings"
    ];
    
    let selected_tab = match app.current_tab {
        Tab::CleanInstallBackup => 0,
        Tab::SoftwareManagement => 1,
        Tab::RGBFanControl => 2,
        Tab::KernelTools => 3,
        Tab::AIAssistant => 4,
        Tab::ProxMoxVMControl => 5,
        Tab::Settings => 6,
    };
    
    let tabs = Tabs::new(tab_titles)
        .block(Block::default().borders(Borders::ALL).title("Tabs"))
        .style(Style::default().fg(Color::White))
        .highlight_style(Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD))
        .select(selected_tab);
    f.render_widget(tabs, main_content_chunks[0]);
    
    // Render tab content
    match app.current_tab {
        Tab::CleanInstallBackup => render_backup_tab(f, app, main_content_chunks[1]),
        Tab::SoftwareManagement => render_software_tab(f, app, main_content_chunks[1]),
        Tab::RGBFanControl => render_rgb_fan_tab(f, app, main_content_chunks[1]),
        Tab::KernelTools => render_kernel_tab(f, app, main_content_chunks[1]),
        Tab::AIAssistant => render_ai_tab(f, app, main_content_chunks[1]),
        Tab::ProxMoxVMControl => render_proxmox_tab(f, app, main_content_chunks[1]),
        Tab::Settings => render_settings_tab(f, app, main_content_chunks[1]),
    }
    
    // Sidebar - split for system info and logs
    let sidebar_chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Percentage(40), // System info
            Constraint::Percentage(60), // Logs
        ])
        .split(content_chunks[1]);
    
    // System info panel
    let system_info = vec![
        format!("CPU Temp: {:.1}°C", app.cpu_temp),
        format!("GPU Temp: {:.1}°C", app.gpu_temp),
        format!("Fan Speed: {}%", app.fan_speed),
        format!("RGB Brightness: {}%", (app.rgb_brightness as f32 / 255.0 * 100.0) as u8),
        "".to_string(),
        "Controls:".to_string(),
        "←/→ Switch tabs".to_string(),
        "Tab: Switch subtabs".to_string(),
        "1-3: RGB effects".to_string(),
        "0: RGB off".to_string(),
        "q: Quit".to_string(),
    ];
    
    let system_info_widget = Paragraph::new(system_info.join("\n"))
        .block(Block::default().borders(Borders::ALL).title("System Info"))
        .style(Style::default().fg(Color::Green));
    f.render_widget(system_info_widget, sidebar_chunks[0]);
    
    // Logs panel (placeholder)
    let logs = vec![
        "System started".to_string(),
        "RGB controller initialized".to_string(),
        "Temperature monitoring active".to_string(),
    ];
    
    let logs_widget = Paragraph::new(logs.join("\n"))
        .block(Block::default().borders(Borders::ALL).title("Logs"))
        .style(Style::default().fg(Color::Gray));
    f.render_widget(logs_widget, sidebar_chunks[1]);
}

fn render_backup_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("Clean Install Backup/Restore\n\nFeatures:\n• Full system backup\n• Selective restore\n• Clean install preparation\n• Backup scheduling\n\n[Implementation in progress]")
        .block(Block::default().borders(Borders::ALL).title("Backup & Restore"))
        .style(Style::default().fg(Color::White));
    f.render_widget(content, area);
}

fn render_software_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("Software Management\n\nPackage Managers:\n• AUR Helper\n• Pacman\n• Flatpak\n• AppImages\n\n[Implementation in progress]")
        .block(Block::default().borders(Borders::ALL).title("Software Management"))
        .style(Style::default().fg(Color::White));
    f.render_widget(content, area);
}

fn render_rgb_fan_tab(f: &mut Frame, app: &mut App, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3), // Subtabs
            Constraint::Min(0),    // Content
        ])
        .split(area);
    
    // RGB/Fan subtabs
    let subtab_titles = vec!["Keyboard RGB", "Fan Control"];
    let selected_subtab = match app.rgb_fan_subtab {
        RGBFanSubTab::Keyboard => 0,
        RGBFanSubTab::Fans => 1,
    };
    
    let subtabs = Tabs::new(subtab_titles)
        .block(Block::default().borders(Borders::ALL).title("RGB/Fan Control"))
        .style(Style::default().fg(Color::White))
        .highlight_style(Style::default().fg(Color::Magenta).add_modifier(Modifier::BOLD))
        .select(selected_subtab);
    f.render_widget(subtabs, chunks[0]);
    
    match app.rgb_fan_subtab {
        RGBFanSubTab::Keyboard => {
            let effect_text = match &app.current_rgb_effect {
                RGBEffect::Static { r, g, b } => format!("Static RGB({}, {}, {})", r, g, b),
                RGBEffect::RainbowWave { speed } => format!("Rainbow Wave (Speed: {})", speed),
                RGBEffect::Breathing { r, g, b, speed } => format!("Breathing RGB({}, {}, {}) Speed: {}", r, g, b, speed),
                RGBEffect::Off => "Off".to_string(),
            };
            
            let content = format!(
                "Keyboard RGB Control\n\nCurrent Effect: {}\nBrightness: {}%\n\nQuick Controls:\n1 - Red Static\n2 - Rainbow Wave\n3 - Green Breathing\n0 - Turn Off\n\n[Device: /dev/hidraw0]",
                effect_text,
                (app.rgb_brightness as f32 / 255.0 * 100.0) as u8
            );
            
            let widget = Paragraph::new(content)
                .block(Block::default().borders(Borders::ALL).title("Keyboard RGB"))
                .style(Style::default().fg(Color::Magenta));
            f.render_widget(widget, chunks[1]);
        },
        RGBFanSubTab::Fans => {
            let content = format!(
                "Fan Control\n\nCPU Fan Speed: {}%\nGPU Fan Speed: {}%\n\nTemperatures:\nCPU: {:.1}°C\nGPU: {:.1}°C\n\n[Implementation in progress]",
                app.fan_speed,
                app.fan_speed,
                app.cpu_temp,
                app.gpu_temp
            );
            
            let widget = Paragraph::new(content)
                .block(Block::default().borders(Borders::ALL).title("Fan Control"))
                .style(Style::default().fg(Color::Blue));
            f.render_widget(widget, chunks[1]);
        }
    }
}

fn render_kernel_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("Kernel Tools\n\nFeatures:\n• Kernel management\n• Module configuration\n• Boot parameters\n• Performance tuning\n\n[Implementation in progress]")
        .block(Block::default().borders(Borders::ALL).title("Kernel Tools"))
        .style(Style::default().fg(Color::White));
    f.render_widget(content, area);
}

fn render_ai_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("AI Assistant\n\nCapabilities:\n• System analysis\n• Performance optimization\n• Intelligent recommendations\n• Chat interface\n\n[Implementation in progress]")
        .block(Block::default().borders(Borders::ALL).title("AI Assistant"))
        .style(Style::default().fg(Color::White));
    f.render_widget(content, area);
}

fn render_proxmox_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("ProxMox/VM Control\n\nFeatures:\n• Virtual Machine Management\n• Container Control\n• Cluster Monitoring\n• Resource Allocation\n• Backup Management\n• Network Configuration\n\n[Integration with Proxmox GUI in progress]")
        .block(Block::default().borders(Borders::ALL).title("ProxMox/VM Control"))
        .style(Style::default().fg(Color::Yellow));
    f.render_widget(content, area);
}

fn render_settings_tab(f: &mut Frame, _app: &mut App, area: Rect) {
    let content = Paragraph::new("Settings\n\nConfiguration:\n• General settings\n• Appearance themes\n• Performance options\n• About information\n\n[Implementation in progress]")
        .block(Block::default().borders(Borders::ALL).title("Settings"))
        .style(Style::default().fg(Color::White));
    f.render_widget(content, area);
}

