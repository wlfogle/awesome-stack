use iced::{
    widget::{button, column, container, row, text},
    Element, Length, Task, Theme,
};
use iced::application::Application;

pub mod backup_restore;
pub mod software_management;
pub mod rgb_fan_control;
pub mod kernel_tools;
pub mod ai_assistant;
pub mod proxmox_vm_control;
pub mod settings;

use backup_restore::BackupRestoreTab;
use software_management::SoftwareManagementTab;
use rgb_fan_control::RgbFanControlTab;
use kernel_tools::KernelToolsTab;
use ai_assistant::AiAssistantTab;
use proxmox_vm_control::ProxmoxVmControlTab;
use settings::SettingsTab;

#[derive(Debug, Clone)]
pub enum Message {
    TabSelected(TabId),
    BackupRestore(backup_restore::Message),
    SoftwareManagement(software_management::Message),
    RgbFanControl(rgb_fan_control::Message),
    KernelTools(kernel_tools::Message),
    AiAssistant(ai_assistant::Message),
    ProxmoxVmControl(proxmox_vm_control::Message),
    Settings(settings::Message),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TabId {
    BackupRestore,
    SoftwareManagement,
    RgbFanControl,
    KernelTools,
    AiAssistant,
    ProxmoxVmControl,
    Settings,
}

impl TabId {
    const ALL: [TabId; 7] = [
        TabId::BackupRestore,
        TabId::SoftwareManagement,
        TabId::RgbFanControl,
        TabId::KernelTools,
        TabId::AiAssistant,
        TabId::ProxmoxVmControl,
        TabId::Settings,
    ];
}

impl TabId {
    fn title(&self) -> &'static str {
        match self {
            TabId::BackupRestore => "🔄 Backup/Restore",
            TabId::SoftwareManagement => "📦 Software Management",
            TabId::RgbFanControl => "🌈 RGB/Fan Control",
            TabId::KernelTools => "🔧 Kernel Tools",
            TabId::AiAssistant => "🤖 AI Assistant",
            TabId::ProxmoxVmControl => "🖥️ ProxMox/VM Control",
            TabId::Settings => "⚙️ Settings",
        }
    }
}

pub struct OriginPCControlSuite {
    active_tab: TabId,
    backup_restore_tab: BackupRestoreTab,
    software_management_tab: SoftwareManagementTab,
    rgb_fan_control_tab: RgbFanControlTab,
    kernel_tools_tab: KernelToolsTab,
    ai_assistant_tab: AiAssistantTab,
    proxmox_vm_control_tab: ProxmoxVmControlTab,
    settings_tab: SettingsTab,
}

impl Default for OriginPCControlSuite {
    fn default() -> Self {
        Self {
            active_tab: TabId::RgbFanControl, // Start with RGB control as main feature
            backup_restore_tab: BackupRestoreTab::default(),
            software_management_tab: SoftwareManagementTab::default(),
            rgb_fan_control_tab: RgbFanControlTab::default(),
            kernel_tools_tab: KernelToolsTab::default(),
            ai_assistant_tab: AiAssistantTab::default(),
            proxmox_vm_control_tab: ProxmoxVmControlTab::default(),
            settings_tab: SettingsTab::default(),
        }
    }
}

impl Application for OriginPCControlSuite {
    type Message = Message;
    type Theme = Theme;
    type Executor = iced::executor::Default;
    type Flags = ();

    fn new(_flags: Self::Flags) -> (Self, Task<Self::Message>) {
        (Self::default(), Task::none())
    }

    fn title(&self) -> String {
        "OriginPC Control Suite - Eon17-X".to_string()
    }

    fn update(&mut self, message: Self::Message) -> Task<Self::Message> {
        match message {
            Message::TabSelected(tab_id) => {
                self.active_tab = tab_id;
                Task::none()
            }
            Message::BackupRestore(msg) => self
                .backup_restore_tab
                .update(msg)
                .map(Message::BackupRestore),
            Message::SoftwareManagement(msg) => self
                .software_management_tab
                .update(msg)
                .map(Message::SoftwareManagement),
            Message::RgbFanControl(msg) => self
                .rgb_fan_control_tab
                .update(msg)
                .map(Message::RgbFanControl),
            Message::KernelTools(msg) => self
                .kernel_tools_tab
                .update(msg)
                .map(Message::KernelTools),
            Message::AiAssistant(msg) => self
                .ai_assistant_tab
                .update(msg)
                .map(Message::AiAssistant),
            Message::ProxmoxVmControl(msg) => self
                .proxmox_vm_control_tab
                .update(msg)
                .map(Message::ProxmoxVmControl),
            Message::Settings(msg) => self
                .settings_tab
                .update(msg)
                .map(Message::Settings),
        }
    }

    fn view(&self) -> Element<Self::Message> {
        let header = container(
            row![
                text("🔥 OriginPC Control Suite").size(24),
                text("Eon17-X Beast Mode").size(16)
            ]
            .spacing(20)
            .align_y(iced::alignment::Vertical::Center)
        )
        .padding(20)
;

        // Tab buttons
        let tab_buttons = row![
            button(TabId::BackupRestore.title())
                .on_press(Message::TabSelected(TabId::BackupRestore)),
            button(TabId::SoftwareManagement.title())
                .on_press(Message::TabSelected(TabId::SoftwareManagement)),
            button(TabId::RgbFanControl.title())
                .on_press(Message::TabSelected(TabId::RgbFanControl)),
            button(TabId::KernelTools.title())
                .on_press(Message::TabSelected(TabId::KernelTools)),
            button(TabId::AiAssistant.title())
                .on_press(Message::TabSelected(TabId::AiAssistant)),
            button(TabId::ProxmoxVmControl.title())
                .on_press(Message::TabSelected(TabId::ProxmoxVmControl)),
            button(TabId::Settings.title())
                .on_press(Message::TabSelected(TabId::Settings)),
        ]
        .spacing(10)
        .padding(10);

        // Content based on active tab
        let content = match self.active_tab {
            TabId::BackupRestore => self.backup_restore_tab.view().map(Message::BackupRestore),
            TabId::SoftwareManagement => self.software_management_tab.view().map(Message::SoftwareManagement),
            TabId::RgbFanControl => self.rgb_fan_control_tab.view().map(Message::RgbFanControl),
            TabId::KernelTools => self.kernel_tools_tab.view().map(Message::KernelTools),
            TabId::AiAssistant => self.ai_assistant_tab.view().map(Message::AiAssistant),
            TabId::ProxmoxVmControl => self.proxmox_vm_control_tab.view().map(Message::ProxmoxVmControl),
            TabId::Settings => self.settings_tab.view().map(Message::Settings),
        };

        container(
            column![
                header,
                tab_buttons,
                content
            ]
            .spacing(0)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }

    fn theme(&self) -> Self::Theme {
        Theme::Dark
    }
}
