use iced::{
    widget::{button, column, container, text},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    StartGamingMode,
    StartSelfHostingMode,
    StartHybridMode,
    StopAllVMs,
    ViewClusterStatus,
}

#[derive(Default)]
pub struct ProxmoxVmControlTab;

impl ProxmoxVmControlTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::StartGamingMode => Task::none(),
            Message::StartSelfHostingMode => Task::none(),
            Message::StartHybridMode => Task::none(),
            Message::StopAllVMs => Task::none(),
            Message::ViewClusterStatus => Task::none(),
        }
    }

    pub fn view(&self) -> Element<Message> {
        container(
            column![
                text("🖥️ ProxMox/VM Control").size(24),
                text("Beast Mode VM Management").size(14),
                button("🎮 Gaming Mode").on_press(Message::StartGamingMode),
                button("🏠 Self-Hosting Mode").on_press(Message::StartSelfHostingMode),
                button("🔥 Hybrid Mode").on_press(Message::StartHybridMode),
                button("🛑 Stop All VMs").on_press(Message::StopAllVMs),
                button("📊 Cluster Status").on_press(Message::ViewClusterStatus),
            ]
            .spacing(20)
            .padding(20)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }
}
