use iced::widget::{button, column, text};
use iced::{Element, Task};

pub fn main() -> iced::Result {
    iced::run("Test", update, view)
}

#[derive(Debug, Clone)]
pub enum Message {
    ButtonPressed,
}

pub fn update(message: Message) -> Task<Message> {
    match message {
        Message::ButtonPressed => Task::none(),
    }
}

pub fn view() -> Element<'static, Message> {
    column![
        text("Hello OriginPC!"),
        button("Test Button").on_press(Message::ButtonPressed),
    ]
    .into()
}
