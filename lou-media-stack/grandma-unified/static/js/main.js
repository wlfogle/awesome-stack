// Grandmother-friendly media dashboard JavaScript

class MediaDashboard {
    constructor() {
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.loadWeather();
        this.loadSystemStatus();
        this.loadRecentContent();
        
        // Refresh data every 5 minutes
        setInterval(() => {
            this.loadSystemStatus();
            this.loadRecentContent();
        }, 300000);
        
        // Refresh weather every hour
        setInterval(() => {
            this.loadWeather();
        }, 3600000);
    }

    setupEventListeners() {
        // Search functionality
        const searchBtn = document.getElementById('search-btn');
        const searchInput = document.getElementById('search-input');
        
        if (searchBtn) {
            searchBtn.addEventListener('click', () => this.performSearch());
        }
        
        if (searchInput) {
            searchInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    this.performSearch();
                }
            });
        }

        // Quick action buttons
        const quickActions = document.querySelectorAll('.action-btn');
        quickActions.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const action = e.target.dataset.action;
                this.handleQuickAction(action);
            });
        });
    }

    async loadWeather() {
        try {
            const response = await fetch('/api/weather');
            const data = await response.json();
            
            const weatherWidget = document.querySelector('.weather-widget');
            if (weatherWidget && data.success) {
                weatherWidget.innerHTML = `
                    <i class="weather-icon">${this.getWeatherIcon(data.weather.icon)}</i>
                    ${data.weather.location} - ${Math.round(data.weather.temperature)}°F
                    <br><small>${data.weather.description}</small>
                `;
            } else if (weatherWidget) {
                weatherWidget.innerHTML = 'Weather unavailable';
            }
        } catch (error) {
            console.error('Weather loading failed:', error);
            const weatherWidget = document.querySelector('.weather-widget');
            if (weatherWidget) {
                weatherWidget.innerHTML = 'Weather unavailable';
            }
        }
    }

    getWeatherIcon(iconCode) {
        const iconMap = {
            '01d': '☀️', '01n': '🌙', '02d': '⛅', '02n': '☁️',
            '03d': '☁️', '03n': '☁️', '04d': '☁️', '04n': '☁️',
            '09d': '🌧️', '09n': '🌧️', '10d': '🌦️', '10n': '🌧️',
            '11d': '⛈️', '11n': '⛈️', '13d': '❄️', '13n': '❄️',
            '50d': '🌫️', '50n': '🌫️'
        };
        return iconMap[iconCode] || '🌤️';
    }

    async loadSystemStatus() {
        try {
            const response = await fetch('/api/status');
            const data = await response.json();
            
            const statusGrid = document.querySelector('.status-grid');
            if (statusGrid && data.services) {
                statusGrid.innerHTML = '';
                
                Object.entries(data.services).forEach(([name, status]) => {
                    const statusItem = document.createElement('div');
                    statusItem.className = `status-item ${status.online ? '' : 'offline'}`;
                    statusItem.innerHTML = `
                        <h4>${this.formatServiceName(name)}</h4>
                        <p>${status.online ? 'Online' : 'Offline'}</p>
                    `;
                    statusGrid.appendChild(statusItem);
                });
            }
        } catch (error) {
            console.error('Status loading failed:', error);
            this.showError('Unable to load system status');
        }
    }

    async loadRecentContent() {
        try {
            const response = await fetch('/api/recent');
            const data = await response.json();
            
            const recentGrid = document.querySelector('.recent-grid');
            if (recentGrid && data.items) {
                recentGrid.innerHTML = '';
                
                data.items.slice(0, 6).forEach(item => {
                    const recentItem = document.createElement('div');
                    recentItem.className = 'recent-item';
                    recentItem.innerHTML = `
                        <img src="${item.poster || '/static/images/placeholder.jpg'}" 
                             alt="${item.title}" 
                             onerror="this.src='/static/images/placeholder.jpg'">
                        <div class="recent-item-content">
                            <h4>${item.title}</h4>
                            <p>${item.type}</p>
                        </div>
                    `;
                    recentItem.addEventListener('click', () => this.playContent(item));
                    recentGrid.appendChild(recentItem);
                });
            }
        } catch (error) {
            console.error('Recent content loading failed:', error);
        }
    }

    async performSearch() {
        const searchInput = document.getElementById('search-input');
        const searchResults = document.getElementById('search-results');
        const query = searchInput.value.trim();

        if (!query) {
            this.showError('Please enter something to search for');
            return;
        }

        // Show loading state
        searchResults.innerHTML = '<div class="loading">🔍 Searching for "' + query + '"...</div>';

        try {
            const response = await fetch('/api/search', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ query: query })
            });

            const data = await response.json();

            if (data.success && data.results && data.results.length > 0) {
                this.displaySearchResults(data.results);
            } else {
                searchResults.innerHTML = `
                    <div class="error">
                        No results found for "${query}". 
                        <br><small>Try searching for movies, TV shows, or music</small>
                    </div>
                `;
            }
        } catch (error) {
            console.error('Search failed:', error);
            searchResults.innerHTML = `
                <div class="error">
                    Search failed. Please try again.
                    <br><small>Error: ${error.message}</small>
                </div>
            `;
        }
    }

    displaySearchResults(results) {
        const searchResults = document.getElementById('search-results');
        const resultsGrid = document.createElement('div');
        resultsGrid.className = 'results-grid';

        results.forEach(result => {
            const resultCard = document.createElement('div');
            resultCard.className = 'result-card';

            const isAvailable = result.available;
            const buttonText = isAvailable ? 'Play Now' : 'Download & Play';
            const buttonClass = isAvailable ? 'play-btn' : 'download-btn';

            resultCard.innerHTML = `
                <img src="${result.poster || '/static/images/placeholder.jpg'}" 
                     alt="${result.title}"
                     onerror="this.src='/static/images/placeholder.jpg'">
                <div class="result-card-content">
                    <h3>${result.title}</h3>
                    <p>${result.year || ''} • ${result.type || 'Media'}</p>
                    <p><small>${result.overview || 'No description available'}</small></p>
                    <button class="${buttonClass}" 
                            onclick="dashboard.handleContentAction('${result.id}', '${result.type}', ${isAvailable})">
                        ${buttonText}
                    </button>
                </div>
            `;

            resultsGrid.appendChild(resultCard);
        });

        searchResults.innerHTML = '';
        searchResults.appendChild(resultsGrid);
    }

    async handleContentAction(id, type, isAvailable) {
        if (isAvailable) {
            this.playContent({ id, type });
        } else {
            await this.downloadContent(id, type);
        }
    }

    async downloadContent(id, type) {
        try {
            this.showSuccess('Starting download...');
            
            const response = await fetch('/api/download', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ id: id, type: type })
            });

            const data = await response.json();

            if (data.success) {
                this.showSuccess('Download started! Content will be available soon.');
                // Refresh recent content after a short delay
                setTimeout(() => this.loadRecentContent(), 5000);
            } else {
                this.showError('Download failed: ' + (data.error || 'Unknown error'));
            }
        } catch (error) {
            console.error('Download failed:', error);
            this.showError('Download failed: ' + error.message);
        }
    }

    playContent(item) {
        // For now, show instructions on how to play
        const instructions = this.getPlayInstructions(item.type || 'media');
        
        const modal = document.createElement('div');
        modal.className = 'play-modal';
        modal.innerHTML = `
            <div class="play-modal-content">
                <h3>How to watch "${item.title}"</h3>
                <div class="play-instructions">
                    ${instructions}
                </div>
                <button onclick="this.parentElement.parentElement.remove()">Got it!</button>
            </div>
        `;
        modal.style.cssText = `
            position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(0,0,0,0.8); display: flex; align-items: center;
            justify-content: center; z-index: 1000;
        `;
        modal.querySelector('.play-modal-content').style.cssText = `
            background: white; padding: 30px; border-radius: 15px;
            max-width: 500px; text-align: center;
        `;
        modal.querySelector('button').style.cssText = `
            background: #667eea; color: white; border: none;
            padding: 15px 30px; border-radius: 10px; font-size: 1.1rem;
            cursor: pointer; margin-top: 20px;
        `;

        document.body.appendChild(modal);
    }

    getPlayInstructions(type) {
        const baseInstructions = `
            <p><strong>On your TV:</strong></p>
            <ol>
                <li>Open the Jellyfin app on your Fire TV</li>
                <li>Look for this content in your library</li>
                <li>Click to start watching!</li>
            </ol>
        `;

        if (type === 'tv') {
            return baseInstructions + `
                <p><strong>For Live TV:</strong></p>
                <ul>
                    <li>Use the TV Guide in Jellyfin</li>
                    <li>Or ask: "Alexa, tune to [channel name]"</li>
                </ul>
            `;
        }

        return baseInstructions;
    }

    handleQuickAction(action) {
        switch (action) {
            case 'tv':
                this.showTVGuide();
                break;
            case 'movies':
                this.searchByType('movie');
                break;
            case 'shows':
                this.searchByType('tv');
                break;
            case 'music':
                this.searchByType('music');
                break;
            default:
                console.log('Unknown action:', action);
        }
    }

    showTVGuide() {
        // For now, just show instructions
        this.showSuccess('Open Jellyfin on your Fire TV to see the TV Guide!');
    }

    searchByType(type) {
        const searchInput = document.getElementById('search-input');
        const placeholder = {
            'movie': 'Search for movies...',
            'tv': 'Search for TV shows...',
            'music': 'Search for music...'
        };
        
        searchInput.placeholder = placeholder[type] || 'Search...';
        searchInput.focus();
    }

    formatServiceName(name) {
        return name.split('_').map(word => 
            word.charAt(0).toUpperCase() + word.slice(1)
        ).join(' ');
    }

    showError(message) {
        this.showMessage(message, 'error');
    }

    showSuccess(message) {
        this.showMessage(message, 'success');
    }

    showMessage(message, type) {
        const messageDiv = document.createElement('div');
        messageDiv.className = type;
        messageDiv.textContent = message;
        
        // Find a good place to show the message
        const container = document.querySelector('.container');
        if (container) {
            container.insertBefore(messageDiv, container.firstChild);
            
            // Auto-remove after 5 seconds
            setTimeout(() => {
                if (messageDiv.parentNode) {
                    messageDiv.parentNode.removeChild(messageDiv);
                }
            }, 5000);
        }
    }
}

// Initialize dashboard when page loads
let dashboard;
document.addEventListener('DOMContentLoaded', () => {
    dashboard = new MediaDashboard();
});

// Make dashboard globally accessible
window.dashboard = dashboard;
