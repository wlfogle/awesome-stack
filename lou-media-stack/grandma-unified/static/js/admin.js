// Admin dashboard JavaScript

class AdminDashboard {
    constructor() {
        this.init();
        this.refreshInterval = 30000; // 30 seconds
    }

    init() {
        this.setupEventListeners();
        this.loadAllData();
        
        // Auto-refresh data
        setInterval(() => {
            this.loadAllData();
        }, this.refreshInterval);
    }

    setupEventListeners() {
        // Refresh buttons
        document.querySelectorAll('[data-refresh]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const target = e.target.dataset.refresh;
                this.refreshSection(target);
            });
        });

        // Service control buttons
        document.addEventListener('click', (e) => {
            if (e.target.matches('[data-service-action]')) {
                const service = e.target.dataset.service;
                const action = e.target.dataset.serviceAction;
                this.handleServiceAction(service, action);
            }
        });

        // Download queue actions
        document.addEventListener('click', (e) => {
            if (e.target.matches('[data-queue-action]')) {
                const action = e.target.dataset.queueAction;
                this.handleQueueAction(action);
            }
        });

        // Configuration toggles
        document.querySelectorAll('[data-config-toggle]').forEach(toggle => {
            toggle.addEventListener('change', (e) => {
                const setting = e.target.dataset.configToggle;
                this.updateConfig(setting, e.target.checked);
            });
        });

        // Configuration inputs
        document.querySelectorAll('[data-config-input]').forEach(input => {
            input.addEventListener('blur', (e) => {
                const setting = e.target.dataset.configInput;
                this.updateConfig(setting, e.target.value);
            });
        });

        // Log actions
        document.addEventListener('click', (e) => {
            if (e.target.matches('[data-log-action]')) {
                const action = e.target.dataset.logAction;
                const logType = e.target.dataset.logType || 'system';
                this.handleLogAction(action, logType);
            }
        });
    }

    loadAllData() {
        this.loadServiceStatus();
        this.loadSystemResources();
        this.loadDownloadQueue();
        this.loadRecentActivity();
        this.loadConfiguration();
    }

    async loadServiceStatus() {
        try {
            const response = await fetch('/api/admin/services');
            const data = await response.json();
            
            const servicesGrid = document.getElementById('services-grid');
            if (servicesGrid && data.services) {
                servicesGrid.innerHTML = '';
                
                Object.entries(data.services).forEach(([name, service]) => {
                    const serviceCard = this.createServiceCard(name, service);
                    servicesGrid.appendChild(serviceCard);
                });
            }
        } catch (error) {
            console.error('Failed to load service status:', error);
            this.showError('Failed to load service status');
        }
    }

    createServiceCard(name, service) {
        const card = document.createElement('div');
        card.className = `service-card ${service.status === 'running' ? 'online' : 'offline'}`;
        
        const formatName = name.split('_').map(word => 
            word.charAt(0).toUpperCase() + word.slice(1)
        ).join(' ');

        card.innerHTML = `
            <div class="service-header">
                <h3>${formatName}</h3>
                <span class="status-badge ${service.status}">${service.status}</span>
            </div>
            <div class="service-info">
                <p>CPU: ${service.cpu || 'N/A'}%</p>
                <p>Memory: ${service.memory || 'N/A'} MB</p>
                <p>Uptime: ${service.uptime || 'N/A'}</p>
            </div>
            <div class="service-actions">
                <button data-service="${name}" data-service-action="restart" 
                        class="btn btn-warning">Restart</button>
                <button data-service="${name}" data-service-action="stop" 
                        class="btn btn-danger">Stop</button>
                <button data-service="${name}" data-service-action="start" 
                        class="btn btn-success">Start</button>
                <a href="${service.url || '#'}" target="_blank" class="btn btn-primary">
                    Open
                </a>
            </div>
        `;

        return card;
    }

    async loadSystemResources() {
        try {
            const response = await fetch('/api/admin/system');
            const data = await response.json();
            
            if (data.success) {
                this.updateResourceDisplay('cpu', data.system.cpu);
                this.updateResourceDisplay('memory', data.system.memory);
                this.updateResourceDisplay('disk', data.system.disk);
                this.updateResourceDisplay('network', data.system.network);
            }
        } catch (error) {
            console.error('Failed to load system resources:', error);
        }
    }

    updateResourceDisplay(resource, data) {
        const container = document.querySelector(`[data-resource="${resource}"]`);
        if (!container || !data) return;

        const percentage = data.percentage || 0;
        const progressBar = container.querySelector('.progress-bar');
        const valueText = container.querySelector('.resource-value');
        const detailText = container.querySelector('.resource-detail');

        if (progressBar) {
            progressBar.style.width = `${percentage}%`;
            progressBar.className = `progress-bar ${this.getResourceClass(percentage)}`;
        }

        if (valueText) {
            valueText.textContent = `${percentage}%`;
        }

        if (detailText) {
            switch (resource) {
                case 'cpu':
                    detailText.textContent = `Load: ${data.load || 'N/A'}`;
                    break;
                case 'memory':
                    detailText.textContent = `${data.used || 'N/A'} / ${data.total || 'N/A'} GB`;
                    break;
                case 'disk':
                    detailText.textContent = `${data.free || 'N/A'} GB free`;
                    break;
                case 'network':
                    detailText.textContent = `↓${data.down || 'N/A'} ↑${data.up || 'N/A'} Mbps`;
                    break;
            }
        }
    }

    getResourceClass(percentage) {
        if (percentage < 60) return 'low';
        if (percentage < 80) return 'medium';
        return 'high';
    }

    async loadDownloadQueue() {
        try {
            const response = await fetch('/api/admin/queue');
            const data = await response.json();
            
            const queueList = document.getElementById('queue-list');
            if (queueList && data.queue) {
                queueList.innerHTML = '';
                
                if (data.queue.length === 0) {
                    queueList.innerHTML = '<p class="empty-state">No downloads in queue</p>';
                } else {
                    data.queue.forEach(item => {
                        const queueItem = this.createQueueItem(item);
                        queueList.appendChild(queueItem);
                    });
                }
            }
        } catch (error) {
            console.error('Failed to load download queue:', error);
        }
    }

    createQueueItem(item) {
        const div = document.createElement('div');
        div.className = `queue-item ${item.status || 'queued'}`;
        
        div.innerHTML = `
            <div class="queue-info">
                <h4>${item.name}</h4>
                <p>${item.size || 'Unknown size'} • ${item.eta || 'Unknown ETA'}</p>
                <div class="progress">
                    <div class="progress-bar" style="width: ${item.progress || 0}%"></div>
                </div>
            </div>
            <div class="queue-actions">
                <button onclick="adminDashboard.removeFromQueue('${item.id}')" 
                        class="btn btn-sm btn-danger">Remove</button>
            </div>
        `;

        return div;
    }

    async loadRecentActivity() {
        try {
            const response = await fetch('/api/admin/activity');
            const data = await response.json();
            
            const activityList = document.getElementById('activity-list');
            if (activityList && data.activities) {
                activityList.innerHTML = '';
                
                data.activities.slice(0, 10).forEach(activity => {
                    const activityItem = this.createActivityItem(activity);
                    activityList.appendChild(activityItem);
                });
            }
        } catch (error) {
            console.error('Failed to load recent activity:', error);
        }
    }

    createActivityItem(activity) {
        const div = document.createElement('div');
        div.className = `activity-item ${activity.type || 'info'}`;
        
        div.innerHTML = `
            <div class="activity-icon">${this.getActivityIcon(activity.type)}</div>
            <div class="activity-content">
                <p>${activity.message}</p>
                <small>${activity.timestamp}</small>
            </div>
        `;

        return div;
    }

    getActivityIcon(type) {
        const icons = {
            'download': '⬇️',
            'search': '🔍',
            'error': '❌',
            'warning': '⚠️',
            'success': '✅',
            'info': 'ℹ️'
        };
        return icons[type] || 'ℹ️';
    }

    async loadConfiguration() {
        try {
            const response = await fetch('/api/admin/config');
            const data = await response.json();
            
            if (data.config) {
                Object.entries(data.config).forEach(([key, value]) => {
                    const toggle = document.querySelector(`[data-config-toggle="${key}"]`);
                    const input = document.querySelector(`[data-config-input="${key}"]`);
                    
                    if (toggle && typeof value === 'boolean') {
                        toggle.checked = value;
                    } else if (input) {
                        input.value = value;
                    }
                });
            }
        } catch (error) {
            console.error('Failed to load configuration:', error);
        }
    }

    async handleServiceAction(service, action) {
        try {
            this.showMessage(`${action}ing ${service}...`, 'info');
            
            const response = await fetch('/api/admin/services/action', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ service, action })
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.showMessage(`${service} ${action}ed successfully`, 'success');
                setTimeout(() => this.loadServiceStatus(), 2000);
            } else {
                this.showMessage(`Failed to ${action} ${service}: ${data.error}`, 'error');
            }
        } catch (error) {
            console.error(`Service action failed:`, error);
            this.showMessage(`Failed to ${action} ${service}`, 'error');
        }
    }

    async handleQueueAction(action) {
        try {
            const response = await fetch('/api/admin/queue/action', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ action })
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.showMessage(`Queue ${action}ed successfully`, 'success');
                this.loadDownloadQueue();
            } else {
                this.showMessage(`Failed to ${action} queue: ${data.error}`, 'error');
            }
        } catch (error) {
            console.error('Queue action failed:', error);
            this.showMessage(`Failed to ${action} queue`, 'error');
        }
    }

    async removeFromQueue(itemId) {
        try {
            const response = await fetch('/api/admin/queue/remove', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ id: itemId })
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.showMessage('Item removed from queue', 'success');
                this.loadDownloadQueue();
            } else {
                this.showMessage('Failed to remove item from queue', 'error');
            }
        } catch (error) {
            console.error('Remove from queue failed:', error);
            this.showMessage('Failed to remove item from queue', 'error');
        }
    }

    async updateConfig(setting, value) {
        try {
            const response = await fetch('/api/admin/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ setting, value })
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.showMessage(`${setting} updated`, 'success');
            } else {
                this.showMessage(`Failed to update ${setting}`, 'error');
            }
        } catch (error) {
            console.error('Config update failed:', error);
            this.showMessage(`Failed to update ${setting}`, 'error');
        }
    }

    async handleLogAction(action, logType) {
        try {
            switch (action) {
                case 'refresh':
                    await this.loadLogs(logType);
                    break;
                case 'download':
                    await this.downloadLogs(logType);
                    break;
                case 'clear':
                    await this.clearLogs(logType);
                    break;
            }
        } catch (error) {
            console.error(`Log action ${action} failed:`, error);
            this.showMessage(`Failed to ${action} logs`, 'error');
        }
    }

    async loadLogs(logType) {
        try {
            const response = await fetch(`/api/admin/logs/${logType}`);
            const data = await response.json();
            
            const logContainer = document.getElementById(`${logType}-logs`);
            if (logContainer && data.logs) {
                logContainer.innerHTML = '';
                
                data.logs.forEach(log => {
                    const logEntry = document.createElement('div');
                    logEntry.className = `log-entry ${log.level || 'info'}`;
                    logEntry.innerHTML = `
                        <span class="log-timestamp">${log.timestamp}</span>
                        <span class="log-level">${log.level || 'INFO'}</span>
                        <span class="log-message">${log.message}</span>
                    `;
                    logContainer.appendChild(logEntry);
                });
            }
        } catch (error) {
            console.error('Failed to load logs:', error);
        }
    }

    async downloadLogs(logType) {
        try {
            const response = await fetch(`/api/admin/logs/${logType}/download`);
            const blob = await response.blob();
            
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `${logType}-logs-${new Date().toISOString().split('T')[0]}.log`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);
            
            this.showMessage('Logs downloaded successfully', 'success');
        } catch (error) {
            console.error('Failed to download logs:', error);
            this.showMessage('Failed to download logs', 'error');
        }
    }

    async clearLogs(logType) {
        if (!confirm(`Are you sure you want to clear all ${logType} logs?`)) {
            return;
        }
        
        try {
            const response = await fetch(`/api/admin/logs/${logType}/clear`, {
                method: 'DELETE'
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.showMessage('Logs cleared successfully', 'success');
                await this.loadLogs(logType);
            } else {
                this.showMessage('Failed to clear logs', 'error');
            }
        } catch (error) {
            console.error('Failed to clear logs:', error);
            this.showMessage('Failed to clear logs', 'error');
        }
    }

    refreshSection(section) {
        switch (section) {
            case 'services':
                this.loadServiceStatus();
                break;
            case 'system':
                this.loadSystemResources();
                break;
            case 'queue':
                this.loadDownloadQueue();
                break;
            case 'activity':
                this.loadRecentActivity();
                break;
            case 'config':
                this.loadConfiguration();
                break;
            default:
                this.loadAllData();
        }
        
        this.showMessage(`${section} refreshed`, 'success');
    }

    showMessage(message, type) {
        // Remove existing messages
        const existingMessages = document.querySelectorAll('.admin-message');
        existingMessages.forEach(msg => msg.remove());
        
        const messageDiv = document.createElement('div');
        messageDiv.className = `admin-message ${type}`;
        messageDiv.textContent = message;
        
        messageDiv.style.cssText = `
            position: fixed; top: 20px; right: 20px; z-index: 1000;
            background: ${this.getMessageColor(type)}; color: white;
            padding: 15px 20px; border-radius: 5px; font-weight: bold;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        `;
        
        document.body.appendChild(messageDiv);
        
        setTimeout(() => {
            if (messageDiv.parentNode) {
                messageDiv.parentNode.removeChild(messageDiv);
            }
        }, 5000);
    }

    getMessageColor(type) {
        const colors = {
            'success': '#28a745',
            'error': '#dc3545',
            'warning': '#ffc107',
            'info': '#17a2b8'
        };
        return colors[type] || colors.info;
    }

    showError(message) {
        this.showMessage(message, 'error');
    }

    showSuccess(message) {
        this.showMessage(message, 'success');
    }
}

// Initialize admin dashboard when page loads
let adminDashboard;
document.addEventListener('DOMContentLoaded', () => {
    adminDashboard = new AdminDashboard();
});

// Make admin dashboard globally accessible
window.adminDashboard = adminDashboard;
