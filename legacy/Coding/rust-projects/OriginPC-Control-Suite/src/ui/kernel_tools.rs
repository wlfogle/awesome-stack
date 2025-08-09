use iced::{
    widget::{button, column, container, text},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    DownloadKernel,
    ConfigureKernel,
    CompileKernel,
    InstallKernel,
}

#[derive(Default)]
pub struct KernelToolsTab;

impl KernelToolsTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::DownloadKernel => Task::none(),
            Message::ConfigureKernel => Task::none(),
            Message::CompileKernel => Task::none(),
            Message::InstallKernel => Task::none(),
        }
    }

    pub fn view(&self) -> Element<Message> {
        container(
            column![
                text("🔧 Kernel Tools").size(24),
                text("Kernel management and compilation tools").size(14),
                button("Download Kernel").on_press(Message::DownloadKernel),
                button("Configure Kernel").on_press(Message::ConfigureKernel),
                button("Compile Kernel").on_press(Message::CompileKernel),
                button("Install Kernel").on_press(Message::InstallKernel),
            ]
            .spacing(20)
            .padding(20)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }
}
