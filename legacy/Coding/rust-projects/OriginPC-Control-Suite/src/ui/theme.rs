use iced::widget::{button, container};
use iced::{Background, Border, Color, Radius, Shadow, Vector};

// Color palette inspired by modern dark themes
pub mod colors {
    use iced::Color;
    
    // Primary colors
    pub const BACKGROUND_PRIMARY: Color = Color::from_rgb(0.08, 0.09, 0.12);        // #141617
    pub const BACKGROUND_SECONDARY: Color = Color::from_rgb(0.12, 0.14, 0.18);      // #1F242D
    pub const BACKGROUND_TERTIARY: Color = Color::from_rgb(0.16, 0.18, 0.22);       // #292E38
    
    // Surface colors
    pub const SURFACE: Color = Color::from_rgb(0.18, 0.20, 0.25);                   // #2E333D
    pub const SURFACE_ELEVATED: Color = Color::from_rgb(0.22, 0.24, 0.29);          // #383D4A
    
    // Accent colors
    pub const ACCENT_PRIMARY: Color = Color::from_rgb(0.27, 0.51, 0.96);            // #4584F4 (blue)
    pub const ACCENT_SECONDARY: Color = Color::from_rgb(0.96, 0.31, 0.31);          // #F54F4F (red)
    pub const ACCENT_SUCCESS: Color = Color::from_rgb(0.31, 0.78, 0.47);            // #4FC678 (green)
    pub const ACCENT_WARNING: Color = Color::from_rgb(0.96, 0.67, 0.31);            // #F5AB4F (orange)
    
    // Text colors
    pub const TEXT_PRIMARY: Color = Color::from_rgb(0.95, 0.95, 0.95);              // #F2F2F2
    pub const TEXT_SECONDARY: Color = Color::from_rgb(0.75, 0.75, 0.75);            // #BFBFBF
    pub const TEXT_MUTED: Color = Color::from_rgb(0.55, 0.55, 0.55);                // #8C8C8C
    
    // Border colors
    pub const BORDER_PRIMARY: Color = Color::from_rgb(0.25, 0.27, 0.32);            // #404551
    pub const BORDER_ELEVATED: Color = Color::from_rgb(0.35, 0.37, 0.42);           // #595E6B
    
    // RGB themed colors
    pub const RGB_RED: Color = Color::from_rgb(1.0, 0.2, 0.3);
    pub const RGB_GREEN: Color = Color::from_rgb(0.2, 1.0, 0.3);
    pub const RGB_BLUE: Color = Color::from_rgb(0.2, 0.3, 1.0);
    pub const RGB_PURPLE: Color = Color::from_rgb(0.8, 0.2, 1.0);
    pub const RGB_CYAN: Color = Color::from_rgb(0.2, 0.8, 1.0);
}

// Modern button styles
pub fn primary_button() -> button::Style {
    button::Style {
        background: Some(Background::Color(colors::ACCENT_PRIMARY)),
        text_color: colors::TEXT_PRIMARY,
        border: Border {
            color: colors::BORDER_ELEVATED,
            width: 1.0,
            radius: Radius::from(8.0),
        },
        shadow: Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.2),
            offset: Vector::new(0.0, 2.0),
            blur_radius: 4.0,
        },
    }
}

pub fn secondary_button() -> button::Style {
    button::Style {
        background: Some(Background::Color(colors::SURFACE_ELEVATED)),
        text_color: colors::TEXT_PRIMARY,
        border: Border {
            color: colors::BORDER_PRIMARY,
            width: 1.0,
            radius: Radius::from(8.0),
        },
        shadow: Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.15),
            offset: Vector::new(0.0, 1.0),
            blur_radius: 2.0,
        },
    }
}

pub fn tab_button(active: bool) -> button::Style {
    if active {
        button::Style {
            background: Some(Background::Color(colors::ACCENT_PRIMARY)),
            text_color: colors::TEXT_PRIMARY,
            border: Border {
                color: colors::ACCENT_PRIMARY,
                width: 2.0,
                radius: Radius::from(12.0),
            },
            shadow: Shadow {
                color: Color::from_rgba(0.27, 0.51, 0.96, 0.3),
                offset: Vector::new(0.0, 2.0),
                blur_radius: 8.0,
            },
        }
    } else {
        button::Style {
            background: Some(Background::Color(colors::SURFACE)),
            text_color: colors::TEXT_SECONDARY,
            border: Border {
                color: colors::BORDER_PRIMARY,
                width: 1.0,
                radius: Radius::from(8.0),
            },
            shadow: Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.1),
                offset: Vector::new(0.0, 1.0),
                blur_radius: 2.0,
            },
        }
    }
}

pub fn rgb_button() -> button::Style {
    button::Style {
        background: Some(Background::Color(colors::RGB_PURPLE)),
        text_color: colors::TEXT_PRIMARY,
        border: Border {
            color: colors::RGB_CYAN,
            width: 2.0,
            radius: Radius::from(10.0),
        },
        shadow: Shadow {
            color: Color::from_rgba(0.8, 0.2, 1.0, 0.4),
            offset: Vector::new(0.0, 4.0),
            blur_radius: 12.0,
        },
    }
}

// Container styles
pub fn main_container() -> container::Style {
    container::Style {
        background: Some(Background::Color(colors::BACKGROUND_PRIMARY)),
        border: Border::default(),
        shadow: Shadow::default(),
    }
}

pub fn card_container() -> container::Style {
    container::Style {
        background: Some(Background::Color(colors::SURFACE)),
        border: Border {
            color: colors::BORDER_PRIMARY,
            width: 1.0,
            radius: Radius::from(12.0),
        },
        shadow: Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.15),
            offset: Vector::new(0.0, 4.0),
            blur_radius: 8.0,
        },
    }
}

pub fn header_container() -> container::Style {
    container::Style {
        background: Some(Background::Color(colors::BACKGROUND_SECONDARY)),
        border: Border {
            color: colors::BORDER_PRIMARY,
            width: 0.0,
            radius: Radius::from(0.0),
        },
        shadow: Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.2),
            offset: Vector::new(0.0, 2.0),
            blur_radius: 4.0,
        },
    }
}
