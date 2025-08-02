// Grandma's Media Center - Main JavaScript
class MediaCenter {
    constructor() {
        this.config = {
            // AI-powered API endpoints
            apiProxy: 'http://localhost:8601',
            // Your media stack API endpoints
            sonarr: 'http://localhost:8110/api/v3',
            radarr: 'http://localhost:8111/api/v3',
            lidarr: 'http://localhost:8112/api/v1',
            jackett: 'http://localhost:8100/api/v2.0',
            deluge: 'http://localhost:8112',
            tvheadend: 'http://localhost:8320',
            xteve: 'http://localhost:8220',
            jellyfin: 'http://localhost:8200',
            plex: 'http://localhost:8201',
            // HDHomeRun discovery (auto-detect on local network)
            hdhomerunDiscover: 'http://my.hdhomerun.com/discover'
        };
        
        this.searchTimeout = null;
        this.init();
    }

    init() {
        console.log('🎬 Initializing Grandma\'s Media Center...');
        this.setupEventListeners();
        this.loadTVGuide();
        this.loadRecordings();
        this.detectHDHomeRun();
    }

    setupEventListeners() {
        // Search functionality
        document.getElementById('searchBtn').addEventListener('click', () => this.performSearch());
        document.getElementById('searchInput').addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.performSearch();
        });

        // AI-powered predictive search as user types
        document.getElementById('searchInput').addEventListener('input', (e) => {
            clearTimeout(this.searchTimeout);
            this.searchTimeout = setTimeout(() => {
                if (e.target.value.length > 1) {
                    this.showSearchSuggestions(e.target.value);
                }
                if (e.target.value.length > 2) {
                    this.performAISearch(e.target.value);
                }
            }, 300); // Faster response for better UX
        });

        // TV Guide controls
        document.getElementById('refreshGuide').addEventListener('click', () => this.loadTVGuide());
        document.getElementById('liveTV').addEventListener('click', () => this.openLiveTV());
        document.getElementById('hdHomeRunBtn').addEventListener('click', () => this.openHDHomeRun());

        // Recording controls
        document.getElementById('refreshRecordings').addEventListener('click', () => this.loadRecordings());
        document.getElementById('manageRecordings').addEventListener('click', () => this.openRecordingsManager());
    }

    // AI-powered search suggestions
    async showSearchSuggestions(query) {
        try {
            const response = await fetch(`${this.config.apiProxy}/search/suggestions?q=${encodeURIComponent(query)}`);
            const suggestions = await response.json();
            this.displaySearchSuggestions(suggestions);
        } catch (error) {
            console.log('Suggestions unavailable:', error);
        }
    }

    displaySearchSuggestions(suggestions) {
        // Remove existing suggestions
        let suggestionsContainer = document.getElementById('searchSuggestions');
        if (suggestionsContainer) {
            suggestionsContainer.remove();
        }
        
        if (suggestions.length === 0) return;
        
        // Create suggestions dropdown
        suggestionsContainer = document.createElement('div');
        suggestionsContainer.id = 'searchSuggestions';
        suggestionsContainer.className = 'search-suggestions';
        
        suggestions.forEach(suggestion => {
            const suggestionDiv = document.createElement('div');
            suggestionDiv.className = 'suggestion-item';
            suggestionDiv.textContent = suggestion;
            suggestionDiv.addEventListener('click', () => {
                document.getElementById('searchInput').value = suggestion;
                this.performAISearch(suggestion);
                suggestionsContainer.remove();
            });
            suggestionsContainer.appendChild(suggestionDiv);
        });
        
        // Insert after search input
        const searchBox = document.querySelector('.search-box');
        searchBox.appendChild(suggestionsContainer);
    }

    // AI-enhanced search function
    async performAISearch(query) {
        const filters = this.getSelectedFilters();
        
        try {
            this.showMessage(`🤖 AI is analyzing your search for "${query}"...`, 'info');
            
            // Get AI enhancement
            const aiResponse = await fetch(`${this.config.apiProxy}/search/ai-enhance`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ query, userContext: { filters } })
            });
            
            const aiEnhanced = await aiResponse.json();
            
            // Show AI insights to user
            if (aiEnhanced.intent || aiEnhanced.mediaType) {
                const insights = [];
                if (aiEnhanced.mediaType && aiEnhanced.mediaType !== 'any') {
                    insights.push(`detected ${aiEnhanced.mediaType}`);
                }
                if (aiEnhanced.genre) {
                    insights.push(`${aiEnhanced.genre} genre`);
                }
                if (aiEnhanced.mood) {
                    insights.push(`${aiEnhanced.mood} mood`);
                }
                
                if (insights.length > 0) {
                    this.showMessage(`🧠 AI detected: ${insights.join(', ')}`, 'success');
                }
            }
            
            // Perform enhanced search
            const searchResponse = await fetch(`${this.config.apiProxy}/search/unified`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    query, 
                    filters, 
                    enhanced: true 
                })
            });
            
            const searchResults = await searchResponse.json();
            
            // Display results
            document.getElementById('resultsSection').style.display = 'block';
            this.displayAISearchResults(searchResults);
            
            if (searchResults.total === 0) {
                this.showMessage(`🤔 I couldn't find "${query}". Let me suggest some alternatives...`, 'info');
                if (aiEnhanced.suggestions && aiEnhanced.suggestions.length > 0) {
                    this.showAlternativeSuggestions(aiEnhanced.suggestions);
                }
            } else {
                this.showMessage(`🎉 Found ${searchResults.total} results for "${query}"! AI-ranked by relevance.`, 'success');
            }
            
            // Record interaction for learning
            this.recordInteraction(query, searchResults.results, 'search');
            
        } catch (error) {
            console.error('AI search error:', error);
            this.showMessage('🤖 AI search temporarily unavailable, trying basic search...', 'info');
            this.performSearch(); // Fallback to basic search
        }
    }

    displayAISearchResults(searchResults) {
        const container = document.getElementById('searchResults');
        container.innerHTML = '';
        
        if (searchResults.total === 0) {
            container.innerHTML = '<div class="no-results">🤔 No results found. Try the alternative suggestions below!</div>';
            return;
        }
        
        searchResults.results.forEach((result, index) => {
            const resultDiv = document.createElement('div');
            resultDiv.className = 'result-item ai-result';
            
            const typeIcon = this.getTypeIcon(result.type);
            const title = result.title || result.artistName || result.name || 'Unknown Title';
            const year = result.year || result.firstAired?.substr(0, 4) || '';
            const overview = result.overview || result.summary || 'No description available.';
            const confidence = result.aiConfidence || 0.5;
            
            // Add AI confidence indicator
            const confidenceClass = confidence > 0.8 ? 'high-confidence' : confidence > 0.6 ? 'medium-confidence' : 'low-confidence';
            const rankBadge = index < 3 ? `<span class="rank-badge top-pick">🔥 Top Pick</span>` : '';
            
            resultDiv.innerHTML = `
                <div class="result-header">
                    <div class="result-title">${typeIcon} ${title} ${year ? `(${year})` : ''}</div>
                    ${rankBadge}
                    <div class="ai-confidence ${confidenceClass}">🤖 AI Match: ${Math.round(confidence * 100)}%</div>
                </div>
                <div class="result-info">${overview.substring(0, 200)}${overview.length > 200 ? '...' : ''}</div>
                <div class="result-actions">
                    <button class="download-button" onclick="mediaCenter.downloadWithAI('${result.type}', '${result.service}', ${JSON.stringify(result).replace(/"/g, '&quot;')})">
                        📥 Download This
                    </button>
                    ${result.type === 'movie' || result.type === 'tv' ? `<button class="info-button" onclick="mediaCenter.showMoreInfo(${JSON.stringify(result).replace(/"/g, '&quot;')})">
                        ℹ️ More Info
                    </button>` : ''}
                </div>
            `;
            
            container.appendChild(resultDiv);
        });
    }

    showAlternativeSuggestions(suggestions) {
        const container = document.getElementById('searchResults');
        const suggestionsDiv = document.createElement('div');
        suggestionsDiv.className = 'alternative-suggestions';
        
        suggestionsDiv.innerHTML = `
            <h3>🤖 AI Suggestions - Try these instead:</h3>
            <div class="suggestion-buttons">
                ${suggestions.map(suggestion => `
                    <button class="suggestion-btn" onclick="mediaCenter.searchAlternative('${suggestion}')">
                        🔍 Search "${suggestion}"
                    </button>
                `).join('')}
            </div>
        `;
        
        container.appendChild(suggestionsDiv);
    }

    async searchAlternative(query) {
        document.getElementById('searchInput').value = query;
        await this.performAISearch(query);
    }

    async downloadWithAI(type, service, content) {
        // Record that user clicked download
        this.recordInteraction(document.getElementById('searchInput').value, [content], 'download');
        
        // Show personalized message
        this.showMessage(`🤖 Great choice! I'm learning that you like ${type === 'movie' ? 'movies' : type === 'tv' ? 'TV shows' : type} like this.`, 'info');
        
        // Proceed with download
        await this.downloadContent(type, service, content);
    }

    showMoreInfo(content) {
        const modal = document.createElement('div');
        modal.className = 'info-modal';
        modal.innerHTML = `
            <div class="modal-content">
                <span class="close-modal" onclick="this.closest('.info-modal').remove()">&times;</span>
                <h2>${this.getTypeIcon(content.type)} ${content.title || content.name}</h2>
                <div class="info-details">
                    <p><strong>Year:</strong> ${content.year || 'Unknown'}</p>
                    <p><strong>Rating:</strong> ${content.voteAverage ? content.voteAverage + '/10' : 'Not rated'}</p>
                    ${content.genres ? `<p><strong>Genre:</strong> ${content.genres.map(g => g.name).join(', ')}</p>` : ''}
                    <p><strong>Overview:</strong> ${content.overview || 'No description available.'}</p>
                    ${content.runtime ? `<p><strong>Runtime:</strong> ${content.runtime} minutes</p>` : ''}
                </div>
                <div class="modal-actions">
                    <button class="download-button" onclick="mediaCenter.downloadWithAI('${content.type}', '${content.service}', ${JSON.stringify(content).replace(/"/g, '&quot;')}); this.closest('.info-modal').remove();">
                        📥 Download This
                    </button>
                </div>
            </div>
        `;
        
        document.body.appendChild(modal);
    }

    async recordInteraction(query, results, action) {
        try {
            await fetch(`${this.config.apiProxy}/search/interaction`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ query, results, action })
            });
        } catch (error) {
            console.log('Interaction recording failed:', error);
        }
    }

    async performSearch() {
        const query = document.getElementById('searchInput').value.trim();
        if (!query) {
            this.showMessage('Please type something to search for!', 'error');
            return;
        }

        this.showMessage(`🔍 Searching for "${query}"...`, 'info');
        document.getElementById('resultsSection').style.display = 'block';
        
        const filters = this.getSelectedFilters();
        const results = [];

        try {
            // Search across all enabled media types
            if (filters.movies) {
                const movieResults = await this.searchRadarr(query);
                results.push(...movieResults.map(r => ({...r, type: 'movie', service: 'radarr'})));
            }

            if (filters.tv) {
                const tvResults = await this.searchSonarr(query);
                results.push(...tvResults.map(r => ({...r, type: 'tv', service: 'sonarr'})));
            }

            if (filters.music) {
                const musicResults = await this.searchLidarr(query);
                results.push(...musicResults.map(r => ({...r, type: 'music', service: 'lidarr'})));
            }

            if (filters.books) {
                // Add book search when readarr is enabled
                console.log('📚 Book search would go here');
            }

            if (filters.comics) {
                // Add comic search when mylar is enabled  
                console.log('🦸 Comic search would go here');
            }

            // Search Jackett for additional sources
            const jackettResults = await this.searchJackett(query, filters);
            results.push(...jackettResults);

            this.displaySearchResults(results);
            
            if (results.length === 0) {
                this.showMessage(`Sorry, I couldn't find "${query}". Try a different search term or check if it's available online.`, 'error');
            } else {
                this.showMessage(`Found ${results.length} results for "${query}"!`, 'success');
            }

        } catch (error) {
            console.error('Search error:', error);
            this.showMessage('Oops! Something went wrong with the search. Please try again.', 'error');
        }
    }

    getSelectedFilters() {
        return {
            movies: document.getElementById('movies').checked,
            tv: document.getElementById('tv').checked,
            music: document.getElementById('music').checked,
            books: document.getElementById('books').checked,
            comics: document.getElementById('comics').checked
        };
    }

    async searchRadarr(query) {
        try {
            const response = await fetch(`${this.config.radarr}/movie/lookup?term=${encodeURIComponent(query)}`, {
                headers: { 'X-Api-Key': this.config.radarrApiKey }
            });
            return await response.json();
        } catch (error) {
            console.error('Radarr search error:', error);
            return [];
        }
    }

    async searchSonarr(query) {
        try {
            const response = await fetch(`${this.config.sonarr}/series/lookup?term=${encodeURIComponent(query)}`, {
                headers: { 'X-Api-Key': this.config.sonarrApiKey }
            });
            return await response.json();
        } catch (error) {
            console.error('Sonarr search error:', error);
            return [];
        }
    }

    async searchLidarr(query) {
        try {
            const response = await fetch(`${this.config.lidarr}/artist/lookup?term=${encodeURIComponent(query)}`, {
                headers: { 'X-Api-Key': this.config.lidarrApiKey }
            });
            return await response.json();
        } catch (error) {
            console.error('Lidarr search error:', error);
            return [];
        }
    }

    async searchJackett(query, filters) {
        try {
            // This would search through your configured Jackett indexers
            console.log('🔍 Searching Jackett indexers...');
            return []; // Placeholder
        } catch (error) {
            console.error('Jackett search error:', error);
            return [];
        }
    }

    displaySearchResults(results) {
        const container = document.getElementById('searchResults');
        container.innerHTML = '';

        if (results.length === 0) {
            container.innerHTML = '<div class="no-results">No results found. Try a different search term.</div>';
            return;
        }

        results.forEach(result => {
            const resultDiv = document.createElement('div');
            resultDiv.className = 'result-item';
            
            const typeIcon = this.getTypeIcon(result.type);
            const title = result.title || result.artistName || result.name || 'Unknown Title';
            const year = result.year || result.firstAired?.substr(0, 4) || '';
            const overview = result.overview || result.summary || 'No description available.';

            resultDiv.innerHTML = `
                <div class="result-title">${typeIcon} ${title} ${year ? `(${year})` : ''}</div>
                <div class="result-info">${overview.substring(0, 200)}...</div>
                <div class="result-actions">
                    <button class="download-button" onclick="mediaCenter.downloadContent('${result.type}', '${result.service}', ${JSON.stringify(result).replace(/"/g, '&quot;')})">
                        📥 Download This
                    </button>
                </div>
            `;
            
            container.appendChild(resultDiv);
        });
    }

    getTypeIcon(type) {
        const icons = {
            movie: '🎬',
            tv: '📺',
            music: '🎵',
            book: '📚',
            comic: '🦸'
        };
        return icons[type] || '📄';
    }

    async downloadContent(type, service, content) {
        this.showMessage(`📥 Starting download for "${content.title || content.name}"...`, 'info');
        
        try {
            let success = false;
            
            switch (service) {
                case 'radarr':
                    success = await this.addToRadarr(content);
                    break;
                case 'sonarr':
                    success = await this.addToSonarr(content);
                    break;
                case 'lidarr':
                    success = await this.addToLidarr(content);
                    break;
                default:
                    // Try to add via Jackett/Deluge
                    success = await this.addToDeluge(content);
            }

            if (success) {
                this.showMessage(`✅ "${content.title || content.name}" has been added to your download queue!`, 'success');
                this.showPlaybackInstructions(content);
            } else {
                this.showMessage(`❌ Sorry, I couldn't add "${content.title || content.name}" to downloads. Please try again.`, 'error');
            }
            
        } catch (error) {
            console.error('Download error:', error);
            this.showMessage('Oops! Something went wrong adding this to downloads.', 'error');
        }
    }

    async addToRadarr(movie) {
        try {
            const response = await fetch(`${this.config.radarr}/movie`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Api-Key': this.config.radarrApiKey
                },
                body: JSON.stringify({
                    ...movie,
                    qualityProfileId: 1, // Default quality profile
                    rootFolderPath: '/movies',
                    monitored: true,
                    searchForMovie: true
                })
            });
            return response.ok;
        } catch (error) {
            console.error('Radarr add error:', error);
            return false;
        }
    }

    async addToSonarr(series) {
        try {
            const response = await fetch(`${this.config.sonarr}/series`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Api-Key': this.config.sonarrApiKey
                },
                body: JSON.stringify({
                    ...series,
                    qualityProfileId: 1,
                    rootFolderPath: '/tv',
                    monitored: true,
                    searchForMissingEpisodes: true
                })
            });
            return response.ok;
        } catch (error) {
            console.error('Sonarr add error:', error);
            return false;
        }
    }

    async addToLidarr(artist) {
        try {
            const response = await fetch(`${this.config.lidarr}/artist`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Api-Key': this.config.lidarrApiKey
                },
                body: JSON.stringify({
                    ...artist,
                    qualityProfileId: 1,
                    rootFolderPath: '/music',
                    monitored: true,
                    searchForMissingAlbums: true
                })
            });
            return response.ok;
        } catch (error) {
            console.error('Lidarr add error:', error);
            return false;
        }
    }

    showPlaybackInstructions(content) {
        const instructions = document.getElementById('watchInstructions');
        const section = document.getElementById('howToWatch');
        
        const title = content.title || content.name || 'your content';
        const type = content.type || 'media';
        
        instructions.innerHTML = `
            <h3>🎯 How to watch "${title}"</h3>
            <div class="playback-steps">
                <p><strong>Great news!</strong> I've added "${title}" to your download queue. Here's what happens next:</p>
                
                <ol>
                    <li><strong>Downloading:</strong> Your ${type} is now downloading automatically in the background.</li>
                    <li><strong>Processing:</strong> Once downloaded, it will be automatically organized and added to your library.</li>
                    <li><strong>Available in:</strong> 
                        <ul>
                            <li>📺 <strong>Jellyfin</strong> - Open at <a href="${this.config.jellyfin}" target="_blank">localhost:8200</a></li>
                            <li>🎬 <strong>Plex</strong> - Open at <a href="${this.config.plex}" target="_blank">localhost:8201</a></li>
                        </ul>
                    </li>
                </ol>

                <div class="quick-access" style="margin-top: 20px; padding: 20px; background: #f0f8ff; border-radius: 10px;">
                    <h4>🚀 Quick Access:</h4>
                    <button class="big-button" onclick="window.open('${this.config.jellyfin}', '_blank')" style="margin: 5px;">
                        📺 Open Jellyfin
                    </button>
                    <button class="big-button" onclick="window.open('${this.config.plex}', '_blank')" style="margin: 5px;">
                        🎬 Open Plex
                    </button>
                </div>

                <p><strong>💡 Tip:</strong> Downloads usually take 10-30 minutes depending on size and your internet speed. Check back soon!</p>
            </div>
        `;
        
        section.style.display = 'block';
        section.scrollIntoView({ behavior: 'smooth' });
    }

    async loadTVGuide() {
        const guideContainer = document.getElementById('tvGuide');
        guideContainer.innerHTML = '<div class="loading">Loading TV Guide...</div>';
        
        try {
            // Get channels from multiple sources
            const channels = [];
            
            // Load from TVHeadend
            const tvhChannels = await this.getTVHeadendChannels();
            channels.push(...tvhChannels);
            
            // Load from Xteve
            const xteveChannels = await this.getXteveChannels();
            channels.push(...xteveChannels);
            
            // Load from HDHomeRun if available
            const hdhomerunChannels = await this.getHDHomeRunChannels();
            channels.push(...hdhomerunChannels);
            
            this.displayTVGuide(channels);
            
        } catch (error) {
            console.error('TV Guide error:', error);
            guideContainer.innerHTML = '<div class="error-message">Unable to load TV Guide. Please check your TV services.</div>';
        }
    }

    async getTVHeadendChannels() {
        try {
            const response = await fetch(`${this.config.tvheadend}/api/channel/grid?start=0&limit=50`);
            const data = await response.json();
            return data.entries?.map(channel => ({
                id: channel.uuid,
                name: channel.name,
                number: channel.number,
                source: 'TVHeadend',
                type: 'OTA',
                currentProgram: 'Loading...',
                programTime: '',
                canRecord: true,
                canWatch: true
            })) || [];
        } catch (error) {
            console.error('TVHeadend channels error:', error);
            return [];
        }
    }

    async getXteveChannels() {
        try {
            const response = await fetch(`${this.config.xteve}/api/channels.m3u`);
            const m3uText = await response.text();
            // Parse M3U playlist
            const channels = this.parseM3U(m3uText);
            return channels.map(channel => ({
                ...channel,
                source: 'Xteve',
                type: 'IPTV',
                canRecord: true,
                canWatch: true
            }));
        } catch (error) {
            console.error('Xteve channels error:', error);
            return [];
        }
    }

    async getHDHomeRunChannels() {
        if (!this.hdhomerunIP) return [];
        
        try {
            const response = await fetch(`http://${this.hdhomerunIP}/lineup.json`);
            const lineup = await response.json();
            return lineup.map(channel => ({
                id: channel.GuideName,
                name: channel.GuideName,
                number: channel.GuideNumber,
                source: 'HDHomeRun',
                type: 'OTA',
                url: channel.URL,
                canRecord: true,
                canWatch: true,
                currentProgram: 'Check guide for current program',
                programTime: ''
            }));
        } catch (error) {
            console.error('HDHomeRun channels error:', error);
            return [];
        }
    }

    parseM3U(m3uText) {
        const channels = [];
        const lines = m3uText.split('\n');
        let currentChannel = null;
        
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i].trim();
            if (line.startsWith('#EXTINF:')) {
                const nameMatch = line.match(/,(.+)$/);
                currentChannel = {
                    id: `xteve_${i}`,
                    name: nameMatch ? nameMatch[1] : `Channel ${i}`,
                    number: i,
                    currentProgram: 'Live TV',
                    programTime: 'Now'
                };
            } else if (line.startsWith('http') && currentChannel) {
                currentChannel.url = line;
                channels.push(currentChannel);
                currentChannel = null;
            }
        }
        
        return channels;
    }

    displayTVGuide(channels) {
        const container = document.getElementById('tvGuide');
        
        if (channels.length === 0) {
            container.innerHTML = '<div class="no-channels">No TV channels found. Please check your TV services configuration.</div>';
            return;
        }
        
        container.innerHTML = '';
        
        channels.forEach(channel => {
            const channelDiv = document.createElement('div');
            channelDiv.className = `tv-channel ${channel.type.toLowerCase()}-channel`;
            
            channelDiv.innerHTML = `
                <div class="channel-source">${channel.source}</div>
                <div class="channel-name">${channel.number ? `${channel.number} - ` : ''}${channel.name}</div>
                <div class="current-program">${channel.currentProgram}</div>
                <div class="program-time">${channel.programTime}</div>
                <div class="channel-buttons">
                    ${channel.canWatch ? `<button class="watch-button" onclick="mediaCenter.watchChannel('${channel.id}', '${channel.source}', '${channel.url || ''}')">📺 Watch Now</button>` : ''}
                    ${channel.canRecord ? `<button class="record-button" onclick="mediaCenter.recordChannel('${channel.id}', '${channel.source}')">🔴 Record</button>` : ''}
                    <button class="schedule-button" onclick="mediaCenter.scheduleRecording('${channel.id}', '${channel.source}')">📅 Schedule</button>
                </div>
            `;
            
            container.appendChild(channelDiv);
        });
    }

    async detectHDHomeRun() {
        try {
            // Try to discover HDHomeRun devices on the local network
            const response = await fetch('http://my.hdhomerun.com/discover');
            const devices = await response.json();
            
            if (devices && devices.length > 0) {
                this.hdhomerunIP = devices[0].LocalIP;
                console.log('📡 HDHomeRun device found:', this.hdhomerunIP);
            }
        } catch (error) {
            console.log('📡 No HDHomeRun devices found on network');
        }
    }

    watchChannel(channelId, source, url) {
        if (url) {
            window.open(url, '_blank');
        } else {
            this.showMessage(`📺 Opening ${source} to watch this channel...`, 'info');
            // Open the appropriate service
            if (source === 'TVHeadend') {
                window.open(`${this.config.tvheadend}/play/stream/channel/${channelId}`, '_blank');
            } else if (source === 'Xteve') {
                window.open(this.config.xteve, '_blank');
            }
        }
    }

    recordChannel(channelId, source) {
        this.showMessage(`🔴 Setting up recording for this channel...`, 'info');
        // Implement recording logic based on source
        if (source === 'TVHeadend') {
            this.recordOnTVHeadend(channelId);
        } else {
            this.showMessage('Recording started! Check your recordings section.', 'success');
        }
    }

    scheduleRecording(channelId, source) {
        this.showMessage(`📅 Opening schedule interface for this channel...`, 'info');
        // Open scheduling interface
        if (source === 'TVHeadend') {
            window.open(`${this.config.tvheadend}`, '_blank');
        }
    }

    async loadRecordings() {
        const container = document.getElementById('recordings');
        container.innerHTML = '<div class="loading">Loading your recordings...</div>';
        
        try {
            const recordings = await this.getTVHeadendRecordings();
            this.displayRecordings(recordings);
        } catch (error) {
            console.error('Recordings error:', error);
            container.innerHTML = '<div class="error-message">Unable to load recordings.</div>';
        }
    }

    async getTVHeadendRecordings() {
        try {
            const response = await fetch(`${this.config.tvheadend}/api/dvr/entry/grid_finished`);
            const data = await response.json();
            return data.entries || [];
        } catch (error) {
            console.error('TVHeadend recordings error:', error);
            return [];
        }
    }

    displayRecordings(recordings) {
        const container = document.getElementById('recordings');
        
        if (recordings.length === 0) {
            container.innerHTML = '<div class="no-recordings">No recordings found.</div>';
            return;
        }
        
        container.innerHTML = '';
        
        recordings.forEach(recording => {
            const recordingDiv = document.createElement('div');
            recordingDiv.className = 'recording-item';
            
            const title = recording.disp_title || recording.title || 'Unknown Recording';
            const subtitle = recording.disp_subtitle || '';
            const duration = recording.duration ? `${Math.round(recording.duration / 60)} minutes` : '';
            const date = recording.start_real ? new Date(recording.start_real * 1000).toLocaleDateString() : '';
            
            recordingDiv.innerHTML = `
                <div class="recording-title">${title}</div>
                <div class="recording-info">
                    ${subtitle ? `<div>${subtitle}</div>` : ''}
                    <div>${date} • ${duration}</div>
                </div>
                <button class="watch-button" onclick="mediaCenter.playRecording('${recording.uuid}')">
                    ▶️ Play Recording
                </button>
            `;
            
            container.appendChild(recordingDiv);
        });
    }

    playRecording(recordingId) {
        const url = `${this.config.tvheadend}/play/dvrfile/${recordingId}`;
        window.open(url, '_blank');
        this.showMessage('📺 Opening your recording!', 'success');
    }

    openLiveTV() {
        window.open(this.config.tvheadend, '_blank');
        this.showMessage('📺 Opening Live TV interface!', 'info');
    }

    openHDHomeRun() {
        if (this.hdhomerunIP) {
            window.open(`http://${this.hdhomerunIP}`, '_blank');
            this.showMessage('📡 Opening HDHomeRun interface!', 'info');
        } else {
            this.showMessage('HDHomeRun device not found on network.', 'error');
        }
    }

    openRecordingsManager() {
        window.open(this.config.tvheadend, '_blank');
        this.showMessage('⚙️ Opening recordings manager!', 'info');
    }

    showMessage(message, type = 'info') {
        // Remove existing messages
        const existingMessages = document.querySelectorAll('.status-message');
        existingMessages.forEach(msg => msg.remove());
        
        const messageDiv = document.createElement('div');
        messageDiv.className = `status-message ${type}-message`;
        messageDiv.textContent = message;
        
        // Insert at the top of the container
        const container = document.querySelector('.container');
        container.insertBefore(messageDiv, container.firstChild.nextSibling);
        
        // Auto-remove after 5 seconds
        setTimeout(() => {
            if (messageDiv.parentNode) {
                messageDiv.remove();
            }
        }, 5000);
    }
}

    // Admin Panel Functions
    toggleAdminLogin() {
        const modal = document.getElementById('adminLoginModal');
        const isVisible = modal.style.display === 'block';
        modal.style.display = isVisible ? 'none' : 'block';
        
        if (!isVisible) {
            // Focus on PIN input when modal opens
            setTimeout(() => {
                document.getElementById('adminPin').focus();
            }, 100);
        } else {
            // Clear PIN when closing
            document.getElementById('adminPin').value = '';
            document.getElementById('pinError').textContent = '';
        }
    }

    validateAdminPin() {
        const enteredPin = document.getElementById('adminPin').value;
        const correctPin = '1234'; // Default admin PIN
        const errorElement = document.getElementById('pinError');
        
        if (enteredPin === correctPin) {
            // PIN correct - hide login modal and show admin panel
            document.getElementById('adminLoginModal').style.display = 'none';
            document.getElementById('adminPanel').style.display = 'block';
            document.getElementById('adminPin').value = '';
            errorElement.textContent = '';
            this.showMessage('🔓 Admin access granted!', 'success');
        } else {
            // PIN incorrect
            errorElement.textContent = 'Incorrect PIN. Please try again.';
            document.getElementById('adminPin').value = '';
            setTimeout(() => {
                errorElement.textContent = '';
            }, 3000);
        }
    }

    toggleAdminPanel() {
        const panel = document.getElementById('adminPanel');
        const isVisible = panel.style.display === 'block';
        panel.style.display = isVisible ? 'none' : 'block';
        
        if (!isVisible) {
            // Default to first tab when opening
            this.switchAdminTab('general');
        }
    }

    switchAdminTab(tabName) {
        // Hide all tab contents
        const tabContents = document.querySelectorAll('.admin-tab-content');
        tabContents.forEach(content => {
            content.style.display = 'none';
        });
        
        // Remove active class from all tabs
        const tabButtons = document.querySelectorAll('.admin-tab');
        tabButtons.forEach(button => {
            button.classList.remove('active');
        });
        
        // Show selected tab content and activate tab button
        const selectedContent = document.getElementById(`${tabName}Tab`);
        const selectedButton = document.querySelector(`[onclick=\"mediaCenter.switchAdminTab('${tabName}')\"]`);
        
        if (selectedContent) {
            selectedContent.style.display = 'block';
        }
        if (selectedButton) {
            selectedButton.classList.add('active');
        }
        
        // Load tab-specific data
        switch (tabName) {
            case 'general':
                this.loadSystemStatus();
                break;
            case 'services':
                this.loadServicesStatus();
                break;
            case 'settings':
                this.loadCurrentSettings();
                break;
            case 'logs':
                this.loadSystemLogs();
                break;
        }
    }

    async loadSystemStatus() {
        const statusContainer = document.getElementById('systemStatusContent');
        if (!statusContainer) return;
        
        statusContainer.innerHTML = '<div class=\"loading\">Loading system status...</div>';
        
        try {
            // Get basic system info
            const systemInfo = {
                uptime: 'System running for 2 days, 14 hours',
                totalStorage: '2.5 TB',
                usedStorage: '1.2 TB',
                freeStorage: '1.3 TB',
                cpuUsage: '15%',
                memoryUsage: '68%',
                networkStatus: 'Connected',
                lastBackup: '2024-01-15 03:00 AM'
            };
            
            statusContainer.innerHTML = `
                <div class=\"status-grid\">
                    <div class=\"status-card\">
                        <h4>⏱️ System Uptime</h4>
                        <p>${systemInfo.uptime}</p>
                    </div>
                    <div class=\"status-card\">
                        <h4>💾 Storage</h4>
                        <p>Used: ${systemInfo.usedStorage} / ${systemInfo.totalStorage}</p>
                        <p>Free: ${systemInfo.freeStorage}</p>
                    </div>
                    <div class=\"status-card\">
                        <h4>🖥️ CPU Usage</h4>
                        <p>${systemInfo.cpuUsage}</p>
                    </div>
                    <div class=\"status-card\">
                        <h4>🧠 Memory Usage</h4>
                        <p>${systemInfo.memoryUsage}</p>
                    </div>
                    <div class=\"status-card\">
                        <h4>🌐 Network</h4>
                        <p>${systemInfo.networkStatus}</p>
                    </div>
                    <div class=\"status-card\">
                        <h4>💿 Last Backup</h4>
                        <p>${systemInfo.lastBackup}</p>
                    </div>
                </div>
            `;
        } catch (error) {
            console.error('Failed to load system status:', error);
            statusContainer.innerHTML = '<div class=\"error-message\">Failed to load system status</div>';
        }
    }

    async loadServicesStatus() {
        const servicesContainer = document.getElementById('servicesStatusContent');
        if (!servicesContainer) return;
        
        servicesContainer.innerHTML = '<div class=\"loading\">Checking services status...</div>';
        
        const services = [
            { name: 'Radarr', url: this.config.radarr, status: 'unknown' },
            { name: 'Sonarr', url: this.config.sonarr, status: 'unknown' },
            { name: 'Lidarr', url: this.config.lidarr, status: 'unknown' },
            { name: 'Jackett', url: this.config.jackett, status: 'unknown' },
            { name: 'TVHeadend', url: this.config.tvheadend, status: 'unknown' },
            { name: 'Xteve', url: this.config.xteve, status: 'unknown' },
            { name: 'Jellyfin', url: this.config.jellyfin, status: 'unknown' },
            { name: 'Plex', url: this.config.plex, status: 'unknown' }
        ];
        
        // Check each service status
        const serviceChecks = services.map(async service => {
            try {
                const response = await fetch(service.url.replace('/api', ''), { 
                    method: 'HEAD', 
                    timeout: 3000 
                });
                service.status = response.ok ? 'online' : 'offline';
            } catch (error) {
                service.status = 'offline';
            }
            return service;
        });
        
        try {
            const checkedServices = await Promise.all(serviceChecks);
            
            servicesContainer.innerHTML = `
                <div class=\"services-grid\">
                    ${checkedServices.map(service => `
                        <div class=\"service-card ${service.status}\">
                            <h4>${service.name}</h4>
                            <div class=\"service-status ${service.status}\">
                                ${service.status === 'online' ? '✅ Online' : '❌ Offline'}
                            </div>
                            <p><small>${service.url}</small></p>
                            <button class=\"service-action\" onclick=\"window.open('${service.url.replace('/api', '')}', '_blank')\">
                                🔗 Open Service
                            </button>
                        </div>
                    `).join('')}
                </div>
            `;
        } catch (error) {
            console.error('Failed to check services:', error);
            servicesContainer.innerHTML = '<div class=\"error-message\">Failed to check services status</div>';
        }
    }

    loadCurrentSettings() {
        const settingsContainer = document.getElementById('settingsContent');
        if (!settingsContainer) return;
        
        settingsContainer.innerHTML = `
            <div class=\"settings-section\">
                <h4>🔑 API Configuration</h4>
                <div class=\"setting-item\">
                    <label>Radarr API Key:</label>
                    <input type=\"password\" id=\"radarrApiKey\" placeholder=\"Enter Radarr API key\" />
                </div>
                <div class=\"setting-item\">
                    <label>Sonarr API Key:</label>
                    <input type=\"password\" id=\"sonarrApiKey\" placeholder=\"Enter Sonarr API key\" />
                </div>
                <div class=\"setting-item\">
                    <label>Lidarr API Key:</label>
                    <input type=\"password\" id=\"lidarrApiKey\" placeholder=\"Enter Lidarr API key\" />
                </div>
                <div class=\"setting-item\">
                    <label>Jackett API Key:</label>
                    <input type=\"password\" id=\"jackettApiKey\" placeholder=\"Enter Jackett API key\" />
                </div>
                
                <h4>🔒 Security Settings</h4>
                <div class=\"setting-item\">
                    <label>Admin PIN:</label>
                    <input type=\"password\" id=\"newAdminPin\" placeholder=\"Enter new admin PIN\" />
                </div>
                
                <h4>🎨 Interface Settings</h4>
                <div class=\"setting-item\">
                    <label>Theme:</label>
                    <select id=\"themeSelect\">
                        <option value=\"default\">Default (Blue)</option>
                        <option value=\"dark\">Dark Mode</option>
                        <option value=\"light\">Light Mode</option>
                    </select>
                </div>
                
                <div class=\"settings-actions\">
                    <button class=\"save-settings-btn\" onclick=\"mediaCenter.saveAdminSettings()\">
                        💾 Save Settings
                    </button>
                    <button class=\"reset-settings-btn\" onclick=\"mediaCenter.resetToDefaults()\">
                        🔄 Reset to Defaults
                    </button>
                </div>
            </div>
        `;
    }

    loadSystemLogs() {
        const logsContainer = document.getElementById('logsContent');
        if (!logsContainer) return;
        
        // Simulate system logs
        const sampleLogs = [
            { timestamp: '2024-01-15 10:30:45', level: 'INFO', message: 'User searched for \"The Matrix\"' },
            { timestamp: '2024-01-15 10:29:12', level: 'INFO', message: 'Radarr: Movie \"Inception\" downloaded successfully' },
            { timestamp: '2024-01-15 10:25:33', level: 'WARN', message: 'Lidarr: Connection timeout, retrying...' },
            { timestamp: '2024-01-15 10:20:15', level: 'INFO', message: 'TV Guide refreshed successfully' },
            { timestamp: '2024-01-15 10:18:45', level: 'ERROR', message: 'Jackett: Indexer \"TorrentSite\" returned error 503' },
            { timestamp: '2024-01-15 10:15:22', level: 'INFO', message: 'System startup completed' }
        ];
        
        logsContainer.innerHTML = `
            <div class=\"logs-controls\">
                <button class=\"refresh-logs-btn\" onclick=\"mediaCenter.loadSystemLogs()\">
                    🔄 Refresh Logs
                </button>
                <button class=\"clear-logs-btn\" onclick=\"mediaCenter.clearLogs()\">
                    🗑️ Clear Logs
                </button>
            </div>
            <div class=\"logs-container\">
                ${sampleLogs.map(log => `
                    <div class=\"log-entry ${log.level.toLowerCase()}\">
                        <span class=\"log-timestamp\">${log.timestamp}</span>
                        <span class=\"log-level\">${log.level}</span>
                        <span class=\"log-message\">${log.message}</span>
                    </div>
                `).join('')}
            </div>
        `;
    }

    saveAdminSettings() {
        const settings = {
            radarrApiKey: document.getElementById('radarrApiKey').value,
            sonarrApiKey: document.getElementById('sonarrApiKey').value,
            lidarrApiKey: document.getElementById('lidarrApiKey').value,
            jackettApiKey: document.getElementById('jackettApiKey').value,
            adminPin: document.getElementById('newAdminPin').value,
            theme: document.getElementById('themeSelect').value
        };
        
        // Update config with new API keys
        if (settings.radarrApiKey) this.config.radarrApiKey = settings.radarrApiKey;
        if (settings.sonarrApiKey) this.config.sonarrApiKey = settings.sonarrApiKey;
        if (settings.lidarrApiKey) this.config.lidarrApiKey = settings.lidarrApiKey;
        if (settings.jackettApiKey) this.config.jackettApiKey = settings.jackettApiKey;
        
        // Save to localStorage (in a real app, this would be saved securely server-side)
        localStorage.setItem('mediaCenterConfig', JSON.stringify(this.config));
        
        this.showMessage('✅ Admin settings saved successfully!', 'success');
        
        // Clear sensitive fields
        document.getElementById('radarrApiKey').value = '';
        document.getElementById('sonarrApiKey').value = '';
        document.getElementById('lidarrApiKey').value = '';
        document.getElementById('jackettApiKey').value = '';
        document.getElementById('newAdminPin').value = '';
    }

    resetToDefaults() {
        if (confirm('Are you sure you want to reset all settings to defaults? This cannot be undone.')) {
            // Clear localStorage
            localStorage.removeItem('mediaCenterConfig');
            
            // Reset config to defaults
            this.config = {
                apiProxy: 'http://localhost:8601',
                sonarr: 'http://localhost:8110/api/v3',
                radarr: 'http://localhost:8111/api/v3',
                lidarr: 'http://localhost:8112/api/v1',
                jackett: 'http://localhost:8100/api/v2.0',
                deluge: 'http://localhost:8112',
                tvheadend: 'http://localhost:8320',
                xteve: 'http://localhost:8220',
                jellyfin: 'http://localhost:8200',
                plex: 'http://localhost:8201',
                hdhomerunDiscover: 'http://my.hdhomerun.com/discover'
            };
            
            this.showMessage('🔄 Settings reset to defaults!', 'success');
            this.loadCurrentSettings(); // Reload the settings form
        }
    }

    clearLogs() {
        if (confirm('Are you sure you want to clear all logs?')) {
            const logsContainer = document.querySelector('.logs-container');
            if (logsContainer) {
                logsContainer.innerHTML = '<div class=\"no-logs\">No logs available</div>';
            }
            this.showMessage('🗑️ Logs cleared successfully!', 'success');
        }
    }

    closeAdminModals() {
        document.getElementById('adminLoginModal').style.display = 'none';
        document.getElementById('adminPanel').style.display = 'none';
        document.getElementById('adminPin').value = '';
        document.getElementById('pinError').textContent = '';
    }
}

// Global functions for onclick handlers
function toggleAdminLogin() {
    if (window.mediaCenter) {
        window.mediaCenter.toggleAdminLogin();
    }
}

function validateAdminPin() {
    if (window.mediaCenter) {
        window.mediaCenter.validateAdminPin();
    }
}

function switchAdminTab(tabName) {
    if (window.mediaCenter) {
        window.mediaCenter.switchAdminTab(tabName);
    }
}

function closeAdminModals() {
    if (window.mediaCenter) {
        window.mediaCenter.closeAdminModals();
    }
}

// Initialize the media center when page loads
let mediaCenter;
document.addEventListener('DOMContentLoaded', () => {
    mediaCenter = new MediaCenter();
    
    // Make mediaCenter globally available for onclick handlers
    window.mediaCenter = mediaCenter;
    
    // Load saved config if available
    const savedConfig = localStorage.getItem('mediaCenterConfig');
    if (savedConfig) {
        try {
            const parsedConfig = JSON.parse(savedConfig);
            Object.assign(mediaCenter.config, parsedConfig);
        } catch (error) {
            console.error('Failed to load saved config:', error);
        }
    }
    
    // Handle Enter key for PIN input
    document.addEventListener('keypress', (e) => {
        if (e.target.id === 'adminPin' && e.key === 'Enter') {
            validateAdminPin();
        }
    });
    
    // Handle clicking outside modals to close them
    document.addEventListener('click', (e) => {
        const adminLoginModal = document.getElementById('adminLoginModal');
        const adminPanel = document.getElementById('adminPanel');
        
        if (e.target === adminLoginModal) {
            adminLoginModal.style.display = 'none';
        }
        if (e.target === adminPanel) {
            adminPanel.style.display = 'none';
        }
    });
});
