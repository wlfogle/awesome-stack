// OriginPC Control Suite - Interactive TypeScript
const { invoke } = window.__TAURI__.core;

// State management
interface AppState {
  activeTab: string;
  activeSubTab: string;
  rgbMode: string;
  brightness: number;
  fanSpeed: number;
}

let appState: AppState = {
  activeTab: 'rgb-fan-control',
  activeSubTab: 'keyboard',
  rgbMode: 'rainbow',
  brightness: 75,
  fanSpeed: 50
};

// Tab management
function switchTab(tabId: string) {
  // Remove active class from all tabs and content
  document.querySelectorAll('.tab-button').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
  
  // Add active class to selected tab and content
  const tabButton = document.querySelector(`[data-tab="${tabId}"]`);
  const tabContent = document.getElementById(tabId);
  
  if (tabButton && tabContent) {
    tabButton.classList.add('active');
    tabContent.classList.add('active');
    appState.activeTab = tabId;
  }
}

function switchSubTab(subTabId: string) {
  // Remove active class from all sub-tabs and panels
  document.querySelectorAll('.sub-tab-button').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.subtab-panel').forEach(panel => panel.classList.remove('active'));
  
  // Add active class to selected sub-tab and panel
  const subTabButton = document.querySelector(`[data-subtab="${subTabId}"]`);
  const subTabPanel = document.getElementById(subTabId);
  
  if (subTabButton && subTabPanel) {
    subTabButton.classList.add('active');
    subTabPanel.classList.add('active');
    appState.activeSubTab = subTabId;
  }
}

// RGB Control functions
function setRgbMode(mode: string) {
  // Remove active class from all RGB mode buttons
  document.querySelectorAll('.control-btn').forEach(btn => btn.classList.remove('active'));
  
  // Find the button with matching text content
  document.querySelectorAll('.control-btn').forEach(btn => {
    if (btn.textContent?.toLowerCase().includes(mode.toLowerCase())) {
      btn.classList.add('active');
    }
  });
  
  appState.rgbMode = mode;
  console.log(`RGB Mode set to: ${mode}`);
}

function updateBrightness(value: number) {
  appState.brightness = value;
  const brightnessValue = document.getElementById('brightness-value');
  if (brightnessValue) {
    brightnessValue.textContent = `${value}%`;
  }
}

function updateFanSpeed(value: number) {
  appState.fanSpeed = value;
  const fanSpeedValue = document.getElementById('fan-speed-value');
  if (fanSpeedValue) {
    fanSpeedValue.textContent = `${value}%`;
  }
}

// Hardware control functions (Tauri commands)
async function applyRgbSettings() {
  try {
    await invoke('set_rgb_mode', { 
      mode: appState.rgbMode, 
      brightness: appState.brightness 
    });
    console.log('RGB settings applied successfully');
    
    // Show success feedback
    showNotification('RGB settings applied successfully!', 'success');
  } catch (error) {
    console.error('Failed to apply RGB settings:', error);
    showNotification('Failed to apply RGB settings', 'error');
  }
}

async function applyFanSettings() {
  try {
    await invoke('set_fan_speed', { speed: appState.fanSpeed });
    console.log('Fan settings applied successfully');
    
    showNotification('Fan settings applied successfully!', 'success');
  } catch (error) {
    console.error('Failed to apply fan settings:', error);
    showNotification('Failed to apply fan settings', 'error');
  }
}

// VM Control functions
async function controlVM(mode: string) {
  try {
    await invoke('control_vm', { mode });
    console.log(`VM ${mode} mode activated`);
    
    showNotification(`${mode.charAt(0).toUpperCase() + mode.slice(1)} mode activated!`, 'success');
  } catch (error) {
    console.error(`Failed to activate ${mode} mode:`, error);
    showNotification(`Failed to activate ${mode} mode`, 'error');
  }
}

// Notification system
function showNotification(message: string, type: 'success' | 'error' | 'info' = 'info') {
  // Create notification element
  const notification = document.createElement('div');
  notification.className = `notification notification-${type}`;
  notification.textContent = message;
  
  // Style the notification
  Object.assign(notification.style, {
    position: 'fixed',
    top: '20px',
    right: '20px',
    padding: '12px 24px',
    borderRadius: '8px',
    color: 'white',
    fontWeight: '500',
    zIndex: '1000',
    transform: 'translateX(100%)',
    transition: 'transform 0.3s ease',
    backgroundColor: type === 'success' ? '#4fc678' : type === 'error' ? '#f54f4f' : '#4584f4'
  });
  
  document.body.appendChild(notification);
  
  // Animate in
  setTimeout(() => {
    notification.style.transform = 'translateX(0)';
  }, 100);
  
  // Remove after 3 seconds
  setTimeout(() => {
    notification.style.transform = 'translateX(100%)';
    setTimeout(() => {
      document.body.removeChild(notification);
    }, 300);
  }, 3000);
}

// Event listeners
function setupEventListeners() {
  // Tab navigation
  document.querySelectorAll('.tab-button').forEach(button => {
    button.addEventListener('click', (e) => {
      const target = e.target as HTMLElement;
      const tabId = target.getAttribute('data-tab');
      if (tabId) switchTab(tabId);
    });
  });
  
  // Sub-tab navigation
  document.querySelectorAll('.sub-tab-button').forEach(button => {
    button.addEventListener('click', (e) => {
      const target = e.target as HTMLElement;
      const subTabId = target.getAttribute('data-subtab');
      if (subTabId) switchSubTab(subTabId);
    });
  });
  
  // RGB mode buttons
  document.querySelectorAll('.control-btn').forEach(button => {
    button.addEventListener('click', (e) => {
      const target = e.target as HTMLElement;
      const mode = target.textContent?.toLowerCase() || '';
      setRgbMode(mode);
    });
  });
  
  // Brightness slider
  const brightnessSlider = document.getElementById('brightness') as HTMLInputElement;
  if (brightnessSlider) {
    brightnessSlider.addEventListener('input', (e) => {
      const target = e.target as HTMLInputElement;
      updateBrightness(parseInt(target.value));
    });
  }
  
  // Fan speed slider
  const fanSpeedSlider = document.getElementById('fan-speed') as HTMLInputElement;
  if (fanSpeedSlider) {
    fanSpeedSlider.addEventListener('input', (e) => {
      const target = e.target as HTMLInputElement;
      updateFanSpeed(parseInt(target.value));
    });
  }
  
  // Apply buttons
  document.querySelectorAll('.apply-btn').forEach(button => {
    button.addEventListener('click', (e) => {
      const target = e.target as HTMLElement;
      if (target.textContent?.includes('RGB')) {
        applyRgbSettings();
      } else if (target.textContent?.includes('Fan')) {
        applyFanSettings();
      }
    });
  });
  
  // VM control buttons
  document.querySelectorAll('.vm-btn').forEach(button => {
    button.addEventListener('click', (e) => {
      const target = e.target as HTMLElement;
      const text = target.textContent || '';
      
      if (text.includes('Gaming')) {
        controlVM('gaming');
      } else if (text.includes('Self-Hosting')) {
        controlVM('selfhosting');
      } else if (text.includes('Hybrid')) {
        controlVM('hybrid');
      } else if (text.includes('Stop')) {
        controlVM('stop');
      }
    });
  });
}

// Initialize the application
window.addEventListener('DOMContentLoaded', () => {
  setupEventListeners();
  console.log('🔥 OriginPC Control Suite initialized - Beast Mode Ready!');
  
  // Show welcome notification
  setTimeout(() => {
    showNotification('OriginPC Control Suite loaded successfully!', 'success');
  }, 1000);
});
