const express = require('express');
const cors = require('cors');
const axios = require('axios');
const app = express();

// Middleware
app.use(cors());
app.use(express.json());

// Configuration from environment variables
const config = {
    sonarr: { url: process.env.SONARR_URL, apiKey: process.env.SONARR_API_KEY },
    radarr: { url: process.env.RADARR_URL, apiKey: process.env.RADARR_API_KEY },
    lidarr: { url: process.env.LIDARR_URL, apiKey: process.env.LIDARR_API_KEY },
    jackett: { url: process.env.JACKETT_URL, apiKey: process.env.JACKETT_API_KEY },
    tvheadend: { url: process.env.TVHEADEND_URL },
    xteve: { url: process.env.XTEVE_URL },
    jellyfin: { url: process.env.JELLYFIN_URL },
    plex: { url: process.env.PLEX_URL },
    openaiApiKey: process.env.OPENAI_API_KEY || process.env.AI_API_KEY
};

// AI-powered search query enhancement
class AISearchEngine {
    constructor() {
        this.searchHistory = new Map();
        this.popularQueries = new Map();
        this.contextPatterns = {
            // Common phrases that indicate specific media types
            movies: ['movie', 'film', 'cinema', 'flick', 'picture'],
            tv: ['show', 'series', 'episode', 'season', 'tv'],
            music: ['song', 'album', 'music', 'artist', 'band', 'track'],
            books: ['book', 'novel', 'author', 'read'],
            comedy: ['funny', 'comedy', 'laugh', 'humor', 'hilarious'],
            action: ['action', 'fight', 'explosion', 'adventure'],
            romance: ['romance', 'love', 'romantic', 'date night'],
            horror: ['scary', 'horror', 'frightening', 'spooky'],
            family: ['family', 'kids', 'children', 'disney']
        };
    }

    // Enhanced query processing with AI-like intelligence
    enhanceQuery(originalQuery, userContext = {}) {
        const query = originalQuery.toLowerCase().trim();
        
        // Extract intent and context
        const intent = this.detectIntent(query);
        const mediaType = this.detectMediaType(query);
        const genre = this.detectGenre(query);
        const mood = this.detectMood(query);
        
        // Generate search suggestions
        const suggestions = this.generateSearchSuggestions(query, intent, mediaType);
        
        // Create enhanced search terms
        const enhancedTerms = this.createEnhancedTerms(query, intent, genre, mood);
        
        return {
            originalQuery: originalQuery,
            enhancedTerms,
            intent,
            mediaType,
            genre,
            mood,
            suggestions,
            confidence: this.calculateConfidence(query, intent, mediaType)
        };
    }

    detectIntent(query) {
        const intents = {
            search: ['find', 'search', 'look for', 'want to watch', 'show me'],
            recent: ['new', 'recent', 'latest', 'just released'],
            popular: ['popular', 'trending', 'top', 'best', 'famous'],
            similar: ['like', 'similar to', 'same as', 'reminds me of'],
            mood: ['feel like', 'in the mood for', 'want something']
        };

        for (const [intent, keywords] of Object.entries(intents)) {
            if (keywords.some(keyword => query.includes(keyword))) {
                return intent;
            }
        }
        return 'search';
    }

    detectMediaType(query) {
        for (const [type, keywords] of Object.entries(this.contextPatterns)) {
            if (keywords.some(keyword => query.includes(keyword))) {
                return type;
            }
        }
        
        // Smart detection based on common patterns
        if (query.match(/season \d+|episode \d+|s\d+e\d+/i)) return 'tv';
        if (query.match(/\d{4}/) && query.length < 50) return 'movies'; // Year suggests movie
        if (query.includes(' by ')) return 'music'; // "Song by Artist"
        
        return 'any';
    }

    detectGenre(query) {
        const genres = {
            action: ['action', 'fight', 'explosion', 'adventure', 'superhero'],
            comedy: ['funny', 'comedy', 'laugh', 'humor', 'hilarious', 'sitcom'],
            drama: ['drama', 'emotional', 'serious', 'deep'],
            horror: ['scary', 'horror', 'frightening', 'spooky', 'thriller'],
            romance: ['romance', 'love', 'romantic', 'date'],
            scifi: ['sci-fi', 'science fiction', 'space', 'future', 'alien'],
            fantasy: ['fantasy', 'magic', 'wizard', 'dragon', 'medieval'],
            documentary: ['documentary', 'real', 'true story', 'factual']
        };

        for (const [genre, keywords] of Object.entries(genres)) {
            if (keywords.some(keyword => query.includes(keyword))) {
                return genre;
            }
        }
        return null;
    }

    detectMood(query) {
        const moods = {
            relaxing: ['chill', 'relax', 'calm', 'peaceful', 'quiet'],
            exciting: ['exciting', 'thrilling', 'intense', 'adrenaline'],
            uplifting: ['happy', 'uplifting', 'feel good', 'positive'],
            nostalgic: ['old', 'classic', 'vintage', 'nostalgic', 'childhood'],
            educational: ['learn', 'educational', 'documentary', 'informative']
        };

        for (const [mood, keywords] of Object.entries(moods)) {
            if (keywords.some(keyword => query.includes(keyword))) {
                return mood;
            }
        }
        return null;
    }

    generateSearchSuggestions(query, intent, mediaType) {
        const suggestions = [];
        
        // Add corrected spellings and alternatives
        suggestions.push(...this.generateSpellingSuggestions(query));
        
        // Add related terms
        suggestions.push(...this.generateRelatedTerms(query, mediaType));
        
        // Add popular variations
        suggestions.push(...this.generatePopularVariations(query));
        
        return suggestions.slice(0, 5); // Limit to top 5 suggestions
    }

    generateSpellingSuggestions(query) {
        // Common misspellings and corrections
        const corrections = {
            'avengers': ['avenger', 'avangers', 'avengres'],
            'batman': ['batmen', 'batman', 'batmann'],
            'star wars': ['starwars', 'star war', 'starwar'],
            'game of thrones': ['game of throne', 'games of thrones'],
            'breaking bad': ['braking bad', 'breaking abd'],
        };
        
        const suggestions = [];
        for (const [correct, variations] of Object.entries(corrections)) {
            if (variations.some(variant => query.includes(variant))) {
                suggestions.push(correct);
            }
        }
        return suggestions;
    }

    generateRelatedTerms(query, mediaType) {
        const related = {
            'superhero': ['marvel', 'dc', 'comic', 'action'],
            'disney': ['pixar', 'animation', 'family', 'kids'],
            'netflix': ['series', 'original', 'streaming'],
            'christmas': ['holiday', 'winter', 'santa', 'family']
        };
        
        const suggestions = [];
        for (const [term, relations] of Object.entries(related)) {
            if (query.includes(term)) {
                suggestions.push(...relations);
            }
        }
        return suggestions;
    }

    generatePopularVariations(query) {
        // Add "the" prefix for common titles
        const variations = [];
        if (!query.startsWith('the ') && query.length > 3) {
            variations.push('the ' + query);
        }
        
        // Add year variations for movies
        const currentYear = new Date().getFullYear();
        for (let i = 0; i < 5; i++) {
            variations.push(`${query} ${currentYear - i}`);
        }
        
        return variations.slice(0, 3);
    }

    createEnhancedTerms(query, intent, genre, mood) {
        let enhanced = [query];
        
        // Add genre-specific terms
        if (genre) {
            enhanced.push(`${query} ${genre}`);
        }
        
        // Add mood-specific terms
        if (mood) {
            enhanced.push(`${query} ${mood}`);
        }
        
        // Add intent-specific modifications
        switch (intent) {
            case 'recent':
                enhanced.push(`${query} 2023`, `${query} 2024`, `${query} new`);
                break;
            case 'popular':
                enhanced.push(`${query} popular`, `${query} best`);
                break;
        }
        
        return enhanced;
    }

    calculateConfidence(query, intent, mediaType) {
        let confidence = 0.5; // Base confidence
        
        // Increase confidence based on specific indicators
        if (mediaType !== 'any') confidence += 0.2;
        if (intent !== 'search') confidence += 0.1;
        if (query.length > 3) confidence += 0.1;
        if (query.includes(' ')) confidence += 0.1; // Multi-word queries are more specific
        
        return Math.min(confidence, 1.0);
    }

    // Learn from user interactions
    recordSearchInteraction(query, results, userAction) {
        const key = query.toLowerCase();
        if (!this.searchHistory.has(key)) {
            this.searchHistory.set(key, { count: 0, results: [], actions: [] });
        }
        
        const history = this.searchHistory.get(key);
        history.count++;
        history.results.push(results);
        history.actions.push(userAction);
        
        // Update popularity
        if (userAction === 'download' || userAction === 'watch') {
            this.popularQueries.set(key, (this.popularQueries.get(key) || 0) + 1);
        }
    }

    // Get personalized recommendations
    getPersonalizedSuggestions(partialQuery) {
        const suggestions = [];
        
        // Get from search history
        for (const [query, data] of this.searchHistory.entries()) {
            if (query.startsWith(partialQuery.toLowerCase()) && data.count > 0) {
                suggestions.push({ query, popularity: data.count });
            }
        }
        
        // Sort by popularity
        suggestions.sort((a, b) => b.popularity - a.popularity);
        
        return suggestions.slice(0, 5).map(s => s.query);
    }
}

// Initialize AI Search Engine
const aiSearch = new AISearchEngine();

// API Routes

// AI-Enhanced Search Endpoint
app.post('/search/ai-enhance', async (req, res) => {
    try {
        const { query, userContext = {} } = req.body;
        
        if (!query) {
            return res.status(400).json({ error: 'Query is required' });
        }
        
        const enhanced = aiSearch.enhanceQuery(query, userContext);
        
        // If we have OpenAI API key, use it for even better enhancement
        if (config.openaiApiKey) {
            try {
                const aiEnhanced = await enhanceWithOpenAI(query, enhanced);
                enhanced.aiSuggestions = aiEnhanced;
            } catch (error) {
                console.log('OpenAI enhancement failed, using local AI:', error.message);
            }
        }
        
        res.json(enhanced);
        
    } catch (error) {
        console.error('AI search enhancement error:', error);
        res.status(500).json({ error: 'AI search enhancement failed' });
    }
});

// Predictive search suggestions
app.get('/search/suggestions', async (req, res) => {
    try {
        const { q } = req.query;
        
        if (!q || q.length < 2) {
            return res.json([]);
        }
        
        const suggestions = aiSearch.getPersonalizedSuggestions(q);
        res.json(suggestions);
        
    } catch (error) {
        console.error('Search suggestions error:', error);
        res.status(500).json({ error: 'Failed to get suggestions' });
    }
});

// Enhanced search across all services
app.post('/search/unified', async (req, res) => {
    try {
        const { query, filters = {}, enhanced = false } = req.body;
        
        let searchQueries = [query];
        
        // Use AI enhancement if requested
        if (enhanced) {
            const aiEnhanced = aiSearch.enhanceQuery(query);
            searchQueries = [...aiEnhanced.enhancedTerms, ...aiEnhanced.suggestions];
        }
        
        const results = [];
        
        // Search all enabled services concurrently
        const searchPromises = [];
        
        if (filters.movies !== false) {
            searchPromises.push(searchRadarr(searchQueries));
        }
        
        if (filters.tv !== false) {
            searchPromises.push(searchSonarr(searchQueries));
        }
        
        if (filters.music !== false) {
            searchPromises.push(searchLidarr(searchQueries));
        }
        
        // Wait for all searches to complete
        const searchResults = await Promise.allSettled(searchPromises);
        
        // Combine and rank results
        searchResults.forEach(result => {
            if (result.status === 'fulfilled' && result.value) {
                results.push(...result.value);
            }
        });
        
        // AI-powered result ranking
        const rankedResults = rankResults(results, query);
        
        res.json({
            query,
            total: rankedResults.length,
            results: rankedResults,
            aiEnhanced: enhanced
        });
        
    } catch (error) {
        console.error('Unified search error:', error);
        res.status(500).json({ error: 'Search failed' });
    }
});

// OpenAI integration for advanced AI features
async function enhanceWithOpenAI(query, localEnhanced) {
    if (!config.openaiApiKey) return null;
    
    try {
        const prompt = `
        User wants to search for: "${query}"
        
        Based on this query, provide:
        1. 3 alternative search terms that might find what they want
        2. The most likely media type (movie, tv show, music, book)
        3. Suggested genre if apparent
        4. A user-friendly explanation of what you think they're looking for
        
        Respond in JSON format only.
        `;
        
        const response = await axios.post('https://api.openai.com/v1/chat/completions', {
            model: 'gpt-3.5-turbo',
            messages: [{ role: 'user', content: prompt }],
            max_tokens: 200,
            temperature: 0.3
        }, {
            headers: {
                'Authorization': `Bearer ${config.openaiApiKey}`,
                'Content-Type': 'application/json'
            }
        });
        
        return JSON.parse(response.data.choices[0].message.content);
        
    } catch (error) {
        console.error('OpenAI API error:', error);
        return null;
    }
}

// Smart result ranking algorithm
function rankResults(results, originalQuery) {
    const query = originalQuery.toLowerCase();
    
    return results.map(result => {
        let score = 0;
        const title = (result.title || result.name || '').toLowerCase();
        
        // Exact match bonus
        if (title === query) score += 100;
        
        // Title contains query bonus
        if (title.includes(query)) score += 50;
        
        // Year match bonus (if query contains a year)
        const yearMatch = query.match(/\b(19|20)\d{2}\b/);
        if (yearMatch && (result.year == yearMatch[0] || (result.firstAired && result.firstAired.includes(yearMatch[0])))) {
            score += 30;
        }
        
        // Popularity bonus (based on vote average, ratings, etc.)
        if (result.voteAverage) score += result.voteAverage;
        if (result.ratings && result.ratings.imdb) score += result.ratings.imdb.value / 10;
        
        // Recent release bonus
        const currentYear = new Date().getFullYear();
        const itemYear = result.year || (result.firstAired ? parseInt(result.firstAired.substr(0, 4)) : 0);
        if (itemYear >= currentYear - 2) score += 20;
        
        return { ...result, searchScore: score };
    })
    .sort((a, b) => b.searchScore - a.searchScore)
    .map(({ searchScore, ...item }) => item); // Remove score from final output
}

// Service-specific search functions
async function searchRadarr(queries) {
    const results = [];
    for (const query of queries.slice(0, 3)) { // Limit to prevent too many requests
        try {
            const response = await axios.get(`${config.radarr.url}/api/v3/movie/lookup`, {
                params: { term: query },
                headers: { 'X-Api-Key': config.radarr.apiKey }
            });
            
            results.push(...response.data.map(movie => ({
                ...movie,
                service: 'radarr',
                type: 'movie'
            })));
        } catch (error) {
            console.error(`Radarr search error for "${query}":`, error.message);
        }
    }
    return results;
}

async function searchSonarr(queries) {
    const results = [];
    for (const query of queries.slice(0, 3)) {
        try {
            const response = await axios.get(`${config.sonarr.url}/api/v3/series/lookup`, {
                params: { term: query },
                headers: { 'X-Api-Key': config.sonarr.apiKey }
            });
            
            results.push(...response.data.map(series => ({
                ...series,
                service: 'sonarr',
                type: 'tv'
            })));
        } catch (error) {
            console.error(`Sonarr search error for "${query}":`, error.message);
        }
    }
    return results;
}

async function searchLidarr(queries) {
    const results = [];
    for (const query of queries.slice(0, 3)) {
        try {
            const response = await axios.get(`${config.lidarr.url}/api/v1/artist/lookup`, {
                params: { term: query },
                headers: { 'X-Api-Key': config.lidarr.apiKey }
            });
            
            results.push(...response.data.map(artist => ({
                ...artist,
                service: 'lidarr',
                type: 'music'
            })));
        } catch (error) {
            console.error(`Lidarr search error for "${query}":`, error.message);
        }
    }
    return results;
}

// Record user interaction for learning
app.post('/search/interaction', (req, res) => {
    try {
        const { query, results, action } = req.body;
        aiSearch.recordSearchInteraction(query, results, action);
        res.json({ success: true });
    } catch (error) {
        console.error('Interaction recording error:', error);
        res.status(500).json({ error: 'Failed to record interaction' });
    }
});

// Health check
app.get('/health', (req, res) => {
    res.json({ 
        status: 'healthy', 
        timestamp: new Date().toISOString(),
        aiEnabled: !!config.openaiApiKey
    });
});

// Start server
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`🚀 Unified Media Dashboard API running on port ${PORT}`);
    console.log(`🤖 AI Search Engine initialized`);
    console.log(`🔗 OpenAI integration: ${config.openaiApiKey ? 'ENABLED' : 'DISABLED'}`);
});
