use anyhow::Result;

pub struct VmController;

impl VmController {
    pub fn new() -> Self {
        Self
    }

    pub fn start_gaming_mode(&self) -> Result<()> {
        // TODO: Execute beast-control.sh gaming mode
        Ok(())
    }

    pub fn start_self_hosting_mode(&self) -> Result<()> {
        // TODO: Execute beast-control.sh self-hosting mode
        Ok(())
    }

    pub fn start_hybrid_mode(&self) -> Result<()> {
        // TODO: Execute beast-control.sh hybrid mode
        Ok(())
    }

    pub fn stop_all_vms(&self) -> Result<()> {
        // TODO: Execute beast-control.sh stop all
        Ok(())
    }
}
