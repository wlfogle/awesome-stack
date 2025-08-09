use iced::{
    widget::{button, column, container, text},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    OpenChat,
    GetRecommendations,
    RunAnalysis,
    OpenSettings,
}

#[derive(Default)]
pub struct AiAssistantTab;

impl AiAssistantTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::OpenChat => Task::none(),
            Message::GetRecommendations => Task::none(),
            Message::RunAnalysis => Task::none(),
            Message::OpenSettings => Task::none(),
        }
    }

    pub fn view(&self) -> Element<Message> {
        container(
            column![
                text("🤖 AI Assistant").size(24),
                text("AI-powered system assistance and recommendations").size(14),
                button("Open Chat").on_press(Message::OpenChat),
                button("Get Recommendations").on_press(Message::GetRecommendations),
                button("Run Analysis").on_press(Message::RunAnalysis),
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
