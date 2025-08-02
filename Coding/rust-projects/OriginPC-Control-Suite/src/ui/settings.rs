use iced::{
    widget::{button, column, container, text},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    Help,
    About,
}

#[derive(Default)]
pub struct SettingsTab;

impl SettingsTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Help => Task::none(),
            Message::About => Task::none(),
        }
    }

    pub fn view(&self) -> Element<Message> {
        container(
            column![
                text("⚙️ Settings").size(24),
                text("Application settings and information").size(14),
                button("Help").on_press(Message::Help),
                button("About").on_press(Message::About),
            ]
            .spacing(20)
            .padding(20)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }
}
