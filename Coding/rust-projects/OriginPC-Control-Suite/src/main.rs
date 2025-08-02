use iced::Theme;
use tracing::{info, Level};
use tracing_subscriber;

mod ui;
mod hardware;
mod system;
mod config;

use ui::OriginPCControlSuite;

fn main() -> iced::Result {
    // Initialize logging
    tracing_subscriber::fmt()
        .with_max_level(Level::INFO)
        .init();

    info!("Starting OriginPC Control Suite...");

    iced::run(
        "OriginPC Control Suite - Eon17-X", 
        OriginPCControlSuite::update,
        OriginPCControlSuite::view
    )
    .theme(Theme::Dark)
}
