const express = require('express');
const axios = require('axios');
const path = require('path');

const app = express();
const port = process.env.PORT || 3000;

// Environment variables
const OPENWEATHER_API_KEY = process.env.OPENWEATHER_API_KEY;
const LATITUDE = parseFloat(process.env.LATITUDE) || 40.7128;
const LONGITUDE = parseFloat(process.env.LONGITUDE) || -74.0060;
const LOCATION_NAME = process.env.LOCATION_NAME || 'New York, NY';

// Cache for radar data
let radarCache = {
    data: null,
    lastUpdated: null,
    expiry: 15 * 60 * 1000 // 15 minutes
};

// Set view engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Static files middleware
app.use(express.static(path.join(__dirname, 'public')));

// Middleware for logging
app.use((req, res, next) => {
    console.log(`${new Date().toISOString()} - ${req.method} ${req.path}`);
    next();
});

// Fetch radar data from OpenWeather
async function fetchRadarData() {
    try {
        if (!OPENWEATHER_API_KEY) {
            console.warn('OpenWeather API key not configured');
            return null;
        }

        console.log('Fetching radar data from OpenWeather...');
        
        // OpenWeather doesn't have a direct radar API, so we'll use weather map tiles
        // The actual radar image will be constructed on the client side
        const response = await axios.get(`https://api.openweathermap.org/data/2.5/weather`, {
            params: {
                lat: LATITUDE,
                lon: LONGITUDE,
                appid: OPENWEATHER_API_KEY,
                units: 'imperial'
            },
            timeout: 10000
        });

        const data = {
            current: response.data,
            location: LOCATION_NAME,
            coordinates: {
                lat: LATITUDE,
                lon: LONGITUDE
            },
            timestamp: new Date().toISOString()
        };

        console.log('Radar data fetched successfully');
        return data;

    } catch (error) {
        console.error('Error fetching radar data:', error.message);
        return null;
    }
}

// Update radar cache
async function updateRadarCache() {
    try {
        const data = await fetchRadarData();
        if (data) {
            radarCache.data = data;
            radarCache.lastUpdated = Date.now();
            console.log('Radar cache updated successfully');
        }
    } catch (error) {
        console.error('Error updating radar cache:', error.message);
    }
}

// Get cached radar data or fetch new data
async function getRadarData() {
    const now = Date.now();
    
    // Check if cache is expired or empty
    if (!radarCache.data || 
        !radarCache.lastUpdated || 
        (now - radarCache.lastUpdated) > radarCache.expiry) {
        
        console.log('Cache expired or empty, fetching new radar data...');
        await updateRadarCache();
    }
    
    return radarCache.data;
}

// Routes
app.get('/', async (req, res) => {
    try {
        const radarData = await getRadarData();
        
        // Prepare data for template
        const templateData = {
            location: LOCATION_NAME,
            lat: LATITUDE,
            lon: LONGITUDE,
            radarImage: null, // Will be loaded dynamically on client side
            current: radarData?.current || null,
            timestamp: new Date().toISOString()
        };

        res.render('index', templateData);
        
    } catch (error) {
        console.error('Error rendering radar page:', error.message);
        res.status(500).render('error', { 
            error: 'Unable to load radar data',
            message: 'Please try again later.' 
        });
    }
});

// API endpoint for radar data
app.get('/api/radar', async (req, res) => {
    try {
        const data = await getRadarData();
        res.json(data || { error: 'No radar data available' });
    } catch (error) {
        console.error('Error in radar API:', error.message);
        res.status(500).json({ error: 'Unable to fetch radar data' });
    }
});

// Health check endpoint
app.get('/health', (req, res) => {
    const status = {
        status: 'ok',
        timestamp: new Date().toISOString(),
        uptime: process.uptime(),
        cache: {
            hasData: !!radarCache.data,
            lastUpdated: radarCache.lastUpdated,
            expired: radarCache.lastUpdated ? 
                (Date.now() - radarCache.lastUpdated) > radarCache.expiry : true
        }
    };
    
    res.json(status);
});

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('Unhandled error:', err);
    res.status(500).json({ error: 'Internal server error' });
});

// 404 handler
app.use((req, res) => {
    res.status(404).json({ error: 'Not found' });
});

// Initialize and start server
async function startServer() {
    try {
        // Initial cache update
        console.log('Initializing radar cache...');
        await updateRadarCache();
        
        // Set up periodic cache updates every 15 minutes
        setInterval(updateRadarCache, 15 * 60 * 1000);
        
        app.listen(port, '0.0.0.0', () => {
            console.log(`Weather radar service running on port ${port}`);
            console.log(`Location: ${LOCATION_NAME} (${LATITUDE}, ${LONGITUDE})`);
            console.log(`API Key configured: ${OPENWEATHER_API_KEY ? 'Yes' : 'No'}`);
            console.log('Auto-update interval: 15 minutes');
        });
        
    } catch (error) {
        console.error('Failed to start server:', error.message);
        process.exit(1);
    }
}

// Graceful shutdown
process.on('SIGTERM', () => {
    console.log('Received SIGTERM, shutting down gracefully...');
    process.exit(0);
});

process.on('SIGINT', () => {
    console.log('Received SIGINT, shutting down gracefully...');
    process.exit(0);
});

// Start the server
startServer();
