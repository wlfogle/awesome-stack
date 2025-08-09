use iced::widget::{button, container, text};
use iced::{application, Background, Border, Color, Theme};

#[derive(Debug, Clone, Copy, Default)]
pub struct DarkTheme;

// Define our custom color palette
pub mod colors {
    use iced::Color;
    
    pub const BACKGROUND: Color = Color::from_rgb(0.08, 0.09, 0.12);
    pub const SURFACE: Color = Color::from_rgb(0.12, 0.14, 0.18);
    pub const SURFACE_VARIANT: Color = Color::from_rgb(0.18, 0.20, 0.25);
    pub const PRIMARY: Color = Color::from_rgb(0.27, 0.51, 0.96);
    pub const PRIMARY_VARIANT: Color = Color::from_rgb(0.15, 0.35, 0.85);
    pub const SECONDARY: Color = Color::from_rgb(0.31, 0.78, 0.47);
    pub const ERROR: Color = Color::from_rgb(0.96, 0.31, 0.31);
    pub const ON_BACKGROUND: Color = Color::from_rgb(0.95, 0.95, 0.95);
    pub const ON_SURFACE: Color = Color::from_rgb(0.88, 0.88, 0.88);
    pub const ON_PRIMARY: Color = Color::WHITE;
}

impl application::StyleSheet for DarkTheme {
    type Style = ();

    fn appearance(&self, _style: &Self::Style) -> application::Appearance {
        application::Appearance {
            background_color: colors::BACKGROUND,
            text_color: colors::ON_BACKGROUND,
        }
    }
}

// Button styles
pub fn primary_button_style(_theme: &Theme, _status: button::Status) -> button::Appearance {
    button::Appearance {
        background: Some(Background::Color(colors::PRIMARY)),
        text_color: colors::ON_PRIMARY,
        border: Border {
            color: colors::PRIMARY_VARIANT,
            width: 1.0,
            radius: 8.0.into(),
        },
        shadow_offset: iced::Vector::new(0.0, 2.0),
    }
}

pub fn secondary_button_style(_theme: &Theme, status: button::Status) -> button::Appearance {
    let (background, border_color) = match status {
        button::Status::Active => (colors::SURFACE_VARIANT, colors::PRIMARY),
        button::Status::Hovered => (colors::PRIMARY.scale_alpha(0.1), colors::PRIMARY),
        button::Status::Pressed => (colors::PRIMARY.scale_alpha(0.2), colors::PRIMARY_VARIANT),
        button::Status::Disabled => (colors::SURFACE, Color::from_rgb(0.5, 0.5, 0.5)),
    };

    button::Appearance {
        background: Some(Background::Color(background)),
        text_color: colors::ON_SURFACE,
        border: Border {
            color: border_color,
            width: 1.0,
            radius: 8.0.into(),
        },
        shadow_offset: iced::Vector::new(0.0, 1.0),
    }
}

pub fn tab_button_style(active: bool) -> impl Fn(&Theme, button::Status) -> button::Appearance {
    move |_theme: &Theme, status: button::Status| {
        if active {
            button::Appearance {
                background: Some(Background::Color(colors::PRIMARY)),
                text_color: colors::ON_PRIMARY,
                border: Border {
                    color: colors::PRIMARY_VARIANT,
                    width: 2.0,
                    radius: 12.0.into(),
                },
                shadow_offset: iced::Vector::new(0.0, 2.0),
            }
        } else {
            let (background, text_color) = match status {
                button::Status::Active => (colors::SURFACE, colors::ON_SURFACE),
                button::Status::Hovered => (colors::SURFACE_VARIANT, colors::ON_SURFACE),
                button::Status::Pressed => (colors::PRIMARY.scale_alpha(0.1), colors::ON_SURFACE),
                button::Status::Disabled => (colors::SURFACE, Color::from_rgb(0.5, 0.5, 0.5)),
            };

            button::Appearance {
                background: Some(Background::Color(background)),
                text_color,
                border: Border {
                    color: colors::SURFACE_VARIANT,
                    width: 1.0,
                    radius: 8.0.into(),
                },
                shadow_offset: iced::Vector::new(0.0, 1.0),
            }
        }
    }
}

// Container styles
pub fn card_container_style(_theme: &Theme) -> container::Appearance {
    container::Appearance {
        background: Some(Background::Color(colors::SURFACE)),
        text_color: Some(colors::ON_SURFACE),
        border: Border {
            color: colors::SURFACE_VARIANT,
            width: 1.0,
            radius: 12.0.into(),
        },
        shadow: iced::Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.1),
            offset: iced::Vector::new(0.0, 4.0),
            blur_radius: 8.0,
        },
    }
}

pub fn header_container_style(_theme: &Theme) -> container::Appearance {
    container::Appearance {
        background: Some(Background::Color(colors::SURFACE)),
        text_color: Some(colors::ON_SURFACE),
        border: Border {
            color: colors::SURFACE_VARIANT,
            width: 0.0,
            radius: 0.0.into(),
        },
        shadow: iced::Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.2),
            offset: iced::Vector::new(0.0, 2.0),
            blur_radius: 4.0,
        },
    }
}

pub fn main_container_style(_theme: &Theme) -> container::Appearance {
    container::Appearance {
        background: Some(Background::Color(colors::BACKGROUND)),
        text_color: Some(colors::ON_BACKGROUND),
        border: Border::default(),
        shadow: iced::Shadow::default(),
    }
}
