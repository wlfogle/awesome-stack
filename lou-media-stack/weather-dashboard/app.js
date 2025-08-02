const express = require('express');
const axios = require('axios');
const path = require('path');

const app = express();
const port = process.env.PORT || 3000;

// Set up EJS template engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// OpenWeather API configuration
const API_KEY = process.env.OPENWEATHER_API_KEY || 'your-api-key-here';
const DEFAULT_CITY = process.env.DEFAULT_CITY || 'New York';
const DEFAULT_STATE = process.env.DEFAULT_STATE || '';
const DEFAULT_COUNTRY = process.env.DEFAULT_COUNTRY || 'US';

// In-memory cache
const cache = new Map();
const CACHE_DURATION = 10 * 60 * 1000; // 10 minutes

// Helper function to get cached data or fetch new data
async function getCachedData(key, fetchFunction) {
    const cached = cache.get(key);
    if (cached && Date.now() - cached.timestamp < CACHE_DURATION) {
        return cached.data;
    }
    
    try {
        const data = await fetchFunction();
        cache.set(key, { data, timestamp: Date.now() });
        return data;
    } catch (error) {
        console.error(`Error fetching ${key}:`, error.message);
        // Return cached data if available, even if expired
        if (cached) {
            return cached.data;
        }
        return null;
    }
}

// Get location string for API calls
function getLocationString() {
    let location = DEFAULT_CITY;
    if (DEFAULT_STATE) location += `,${DEFAULT_STATE}`;
    if (DEFAULT_COUNTRY) location += `,${DEFAULT_COUNTRY}`;
    return location;
}

// Fetch current weather
async function fetchCurrentWeather() {
    const location = getLocationString();
    const response = await axios.get(
        `https://api.openweathermap.org/data/2.5/weather?q=${location}&appid=${API_KEY}&units=imperial`
    );
    return response.data;
}

// Fetch 5-day forecast
async function fetchForecast() {
    const location = getLocationString();
    const response = await axios.get(
        `https://api.openweathermap.org/data/2.5/forecast?q=${location}&appid=${API_KEY}&units=imperial`
    );
    return response.data;
}

// Fetch weather alerts (using One Call API 3.0)
async function fetchWeatherAlerts(lat, lon) {
    try {
        const response = await axios.get(
            `https://api.openweathermap.org/data/3.0/onecall?lat=${lat}&lon=${lon}&appid=${API_KEY}&exclude=minutely,hourly,daily`
        );
        return response.data.alerts || [];
    } catch (error) {
        console.error('Error fetching alerts:', error.message);
        return [];
    }
}

// Fetch UV Index
async function fetchUVIndex(lat, lon) {
    try {
        const response = await axios.get(
            `https://api.openweathermap.org/data/2.5/uvi?lat=${lat}&lon=${lon}&appid=${API_KEY}`
        );
        return response.data;
    } catch (error) {
        console.error('Error fetching UV index:', error.message);
        return { value: 'N/A' };
    }
}

// Fetch air quality data
async function fetchAirQuality(lat, lon) {
    try {
        const response = await axios.get(
            `https://api.openweathermap.org/data/2.5/air_pollution?lat=${lat}&lon=${lon}&appid=${API_KEY}`
        );
        return response.data;
    } catch (error) {
        console.error('Error fetching air quality:', error.message);
        return null;
    }
}

// Main route
app.get('/', async (req, res) => {
    try {
        // Check if API key is configured
        if (!API_KEY || API_KEY === 'your-api-key-here') {
            return res.render('index', {
                weather: {
                    current: null,
                    forecast: null,
                    alerts: [],
                    uv: null,
                    airQuality: null
                },
                error: 'OpenWeather API key not configured. Please set OPENWEATHER_API_KEY environment variable.'
            });
        }

        // Fetch current weather data
        const current = await getCachedData('current', fetchCurrentWeather);
        
        if (!current) {
            throw new Error('Unable to fetch current weather data');
        }

        // Extract coordinates for additional API calls
        const lat = current.coord.lat;
        const lon = current.coord.lon;

        // Fetch all weather data in parallel
        const [forecast, alerts, uv, airQuality] = await Promise.all([
            getCachedData('forecast', fetchForecast),
            getCachedData('alerts', () => fetchWeatherAlerts(lat, lon)),
            getCachedData('uv', () => fetchUVIndex(lat, lon)),
            getCachedData('airQuality', () => fetchAirQuality(lat, lon))
        ]);

        // Prepare weather data object
        const weatherData = {
            current,
            forecast,
            alerts,
            uv,
            airQuality
        };

        res.render('index', { weather: weatherData, error: null });

    } catch (error) {
        console.error('Error in main route:', error.message);
        res.render('index', {
            weather: {
                current: null,
                forecast: null,
                alerts: [],
                uv: null,
                airQuality: null
            },
            error: `Error loading weather data: ${error.message}`
        });
    }
});

// API endpoints for JSON data
app.get('/api/current', async (req, res) => {
    try {
        const current = await getCachedData('current', fetchCurrentWeather);
        res.json(current);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/forecast', async (req, res) => {
    try {
        const forecast = await getCachedData('forecast', fetchForecast);
        res.json(forecast);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/alerts', async (req, res) => {
    try {
        // First get current weather to get coordinates
        const current = await getCachedData('current', fetchCurrentWeather);
        if (!current) {
            throw new Error('Unable to get location coordinates');
        }
        
        const alerts = await getCachedData('alerts', () => fetchWeatherAlerts(current.coord.lat, current.coord.lon));
        res.json(alerts);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/uv', async (req, res) => {
    try {
        const current = await getCachedData('current', fetchCurrentWeather);
        if (!current) {
            throw new Error('Unable to get location coordinates');
        }
        
        const uv = await getCachedData('uv', () => fetchUVIndex(current.coord.lat, current.coord.lon));
        res.json(uv);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/air-quality', async (req, res) => {
    try {
        const current = await getCachedData('current', fetchCurrentWeather);
        if (!current) {
            throw new Error('Unable to get location coordinates');
        }
        
        const airQuality = await getCachedData('airQuality', () => fetchAirQuality(current.coord.lat, current.coord.lon));
        res.json(airQuality);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ 
        status: 'healthy', 
        timestamp: new Date().toISOString(),
        cacheSize: cache.size
    });
});

// Clear cache endpoint (for debugging)
app.get('/api/clear-cache', (req, res) => {
    cache.clear();
    res.json({ message: 'Cache cleared successfully' });
});

// Error handling middleware
app.use((error, req, res, next) => {
    console.error('Unhandled error:', error);
    res.status(500).json({ error: 'Internal server error' });
});

// 404 handler
app.use((req, res) => {
    res.status(404).json({ error: 'Not found' });
});

// Start server
app.listen(port, '0.0.0.0', () => {
    console.log(`Weather Dashboard server running on port ${port}`);
    console.log(`Location: ${getLocationString()}`);
    console.log(`API Key configured: ${API_KEY !== 'your-api-key-here' ? 'Yes' : 'No'}`);
});

// Graceful shutdown
process.on('SIGTERM', () => {
    console.log('Received SIGTERM, shutting down gracefully');
    process.exit(0);
});

process.on('SIGINT', () => {
    console.log('Received SIGINT, shutting down gracefully');
    process.exit(0);
});
