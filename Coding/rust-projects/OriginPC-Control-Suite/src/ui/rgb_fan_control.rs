use iced::{
    widget::{button, column, container, row, text, slider},
    Element, Length, Task,
};

#[derive(Debug, Clone)]
pub enum Message {
    KeyboardTabSelected,
    FansTabSelected,
    RgbModeChanged(RgbMode),
    ColorChanged([f32; 3]),
    BrightnessChanged(f32),
    FanSpeedChanged(u8),
    ApplySettings,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SubTabId {
    Keyboard,
    Fans,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RgbMode {
    Static,
    Rainbow,
    Breathing,
    Wave,
    Custom,
}

impl SubTabId {
    const ALL: [SubTabId; 2] = [SubTabId::Keyboard, SubTabId::Fans];
}

impl SubTabId {
    fn title(&self) -> &'static str {
        match self {
            SubTabId::Keyboard => "⌨️ Keyboard",
            SubTabId::Fans => "🌀 Fans",
        }
    }
}

#[derive(Default)]
pub struct RgbFanControlTab {
    active_subtab: SubTabId,
    rgb_mode: RgbMode,
    rgb_color: [f32; 3],
    brightness: f32,
    fan_speed: u8,
}

impl Default for SubTabId {
    fn default() -> Self {
        SubTabId::Keyboard
    }
}

impl Default for RgbMode {
    fn default() -> Self {
        RgbMode::Rainbow
    }
}

impl RgbFanControlTab {
    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::KeyboardTabSelected => {
                self.active_subtab = SubTabId::Keyboard;
                Task::none()
            }
            Message::FansTabSelected => {
                self.active_subtab = SubTabId::Fans;
                Task::none()
            }
            Message::RgbModeChanged(mode) => {
                self.rgb_mode = mode;
                Task::none()
            }
            Message::ColorChanged(color) => {
                self.rgb_color = color;
                Task::none()
            }
            Message::BrightnessChanged(brightness) => {
                self.brightness = brightness;
                Task::none()
            }
            Message::FanSpeedChanged(speed) => {
                self.fan_speed = speed;
                Task::none()
            }
            Message::ApplySettings => {
                // TODO: Apply RGB/Fan settings to hardware
                Task::none()
            }
        }
    }

    pub fn view(&self) -> Element<Message> {
        let keyboard_content = container(
            column![
                text("🌈 RGB Keyboard Control").size(20),
                text("RGB Mode: Rainbow Wave (Active)").size(14),
                text("Current: Blue static (after OS boot)").size(12),
            row![
                button("Static").on_press(Message::RgbModeChanged(RgbMode::Static)),
                button("Rainbow").on_press(Message::RgbModeChanged(RgbMode::Rainbow)),
                button("Breathing").on_press(Message::RgbModeChanged(RgbMode::Breathing)),
                button("Wave").on_press(Message::RgbModeChanged(RgbMode::Wave)),
            ].spacing(10),
            slider(0.0..=100.0, self.brightness, Message::BrightnessChanged)
                .step(1.0),
            text(format!("Brightness: {}%", self.brightness as u8)),
            button("Apply RGB Settings").on_press(Message::ApplySettings),
            ]
            .spacing(20)
        )
        .padding(20)
;

        let fans_content = container(
            column![
                text("🌀 Fan Control").size(20),
                text("Clevo Fan Management").size(14),
            slider(0.0..=100.0, self.fan_speed as f32, |v| Message::FanSpeedChanged(v as u8))
                .step(1.0),
            text(format!("Fan Speed: {}%", self.fan_speed)),
            button("Apply Fan Settings").on_press(Message::ApplySettings),
            ]
            .spacing(20)
        )
        .padding(20)
;

        let subtab_buttons = row![
            button(SubTabId::Keyboard.title())
                .on_press(Message::KeyboardTabSelected),
            button(SubTabId::Fans.title())
                .on_press(Message::FansTabSelected),
        ].spacing(10);

        let content = match self.active_subtab {
            SubTabId::Keyboard => keyboard_content,
            SubTabId::Fans => fans_content,
        };

        container(
            column![
                subtab_buttons,
                content
            ].spacing(10)
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .padding(10)
        .into()
    }
}
