// Hardware control module for OriginPC Eon17-X
// Handles RGB, fan control, and HID device communication

pub mod rgb;
pub mod fan;
pub mod hid;

use anyhow::Result;

#[derive(Debug, Clone)]
pub struct HardwareManager {
    pub rgb_controller: rgb::RgbController,
    pub fan_controller: fan::FanController,
}

impl HardwareManager {
    pub fn new() -> Result<Self> {
        Ok(Self {
            rgb_controller: rgb::RgbController::new()?,
            fan_controller: fan::FanController::new()?,
        })
    }

    pub fn initialize(&mut self) -> Result<()> {
        self.rgb_controller.initialize()?;
        self.fan_controller.initialize()?;
        Ok(())
    }
}
