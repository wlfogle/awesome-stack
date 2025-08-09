use iced::{
    widget::{button, column, container, text},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    SearchPackages,
    InstallPackages,
    ManageWine,
    SystemMaintenance,
    ViewInstalled,
    OpenSettings,
}

#[derive(Default)]
pub struct SoftwareManagementTab;

impl SoftwareManagementTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::SearchPackages => Task::none(),
            Message::InstallPackages => Task::none(),
            Message::ManageWine => Task::none(),
            Message::SystemMaintenance => Task::none(),
            Message::ViewInstalled => Task::none(),
            Message::OpenSettings => Task::none(),
        }
    }

    pub fn view(&self) -> Element<Message> {
        container(
            column![
                text("📦 Software Management").size(24),
                text("Package management and software installation").size(14),
                button("Search Packages").on_press(Message::SearchPackages),
                button("Install Packages").on_press(Message::InstallPackages),
                button("Wine Management").on_press(Message::ManageWine),
                button("System Maintenance").on_press(Message::SystemMaintenance),
                button("View Installed").on_press(Message::ViewInstalled),
                button("Settings").on_press(Message::OpenSettings),
            ]
            .spacing(20)
            .padding(20)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }
}
