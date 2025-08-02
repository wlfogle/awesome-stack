const express = require('express');
const axios = require('axios');
const moment = require('moment');
const cron = require('node-cron');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// Set EJS as template engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// OpenWeather API configuration
const API_KEY = process.env.OPENWEATHER_API_KEY;
const DEFAULT_CITY = process.env.DEFAULT_CITY || 'London';
const BASE_URL = 'https://api.openweathermap.org/data/2.5';

// In-memory cache for weather data
let weatherCache = {
  current: null,
  forecast: null,
  lastUpdate: null
};

// Fetch current weather data
async function fetchCurrentWeather(city = DEFAULT_CITY) {
  try {
    const response = await axios.get(`${BASE_URL}/weather`, {
      params: {
        q: city,
        appid: API_KEY,
        units: 'metric'
      }
    });
    return response.data;
  } catch (error) {
    console.error('Error fetching current weather:', error.message);
    throw error;
  }
}

// Fetch 5-day forecast
async function fetchForecast(city = DEFAULT_CITY) {
  try {
    const response = await axios.get(`${BASE_URL}/forecast`, {
      params: {
        q: city,
        appid: API_KEY,
        units: 'metric'
      }
    });
    return response.data;
  } catch (error) {
    console.error('Error fetching forecast:', error.message);
    throw error;
  }
}

// Process forecast data to get daily summaries
function processForecast(forecastData) {
  const dailyForecasts = {};
  
  forecastData.list.forEach(item => {
    const date = moment(item.dt * 1000).format('YYYY-MM-DD');
    
    if (!dailyForecasts[date]) {
      dailyForecasts[date] = {
        date: date,
        temps: [],
        conditions: [],
        humidity: [],
        pressure: [],
        wind: [],
        weather: item.weather[0]
      };
    }
    
    dailyForecasts[date].temps.push(item.main.temp);
    dailyForecasts[date].conditions.push(item.weather[0].main);
    dailyForecasts[date].humidity.push(item.main.humidity);
    dailyForecasts[date].pressure.push(item.main.pressure);
    dailyForecasts[date].wind.push(item.wind.speed);
  });
  
  // Calculate daily averages and extremes
  return Object.values(dailyForecasts).map(day => ({
    date: day.date,
    dayName: moment(day.date).format('dddd'),
    temp_min: Math.round(Math.min(...day.temps)),
    temp_max: Math.round(Math.max(...day.temps)),
    temp_avg: Math.round(day.temps.reduce((a, b) => a + b, 0) / day.temps.length),
    humidity: Math.round(day.humidity.reduce((a, b) => a + b, 0) / day.humidity.length),
    pressure: Math.round(day.pressure.reduce((a, b) => a + b, 0) / day.pressure.length),
    wind_speed: Math.round(day.wind.reduce((a, b) => a + b, 0) / day.wind.length),
    weather: day.weather,
    conditions: day.conditions[0]
  }));
}

// Update weather cache
async function updateWeatherCache(city = DEFAULT_CITY) {
  try {
    console.log(`Updating weather cache for ${city}...`);
    
    const [currentData, forecastData] = await Promise.all([
      fetchCurrentWeather(city),
      fetchForecast(city)
    ]);
    
    weatherCache.current = currentData;
    weatherCache.forecast = processForecast(forecastData);
    weatherCache.lastUpdate = new Date();
    
    console.log('Weather cache updated successfully');
  } catch (error) {
    console.error('Error updating weather cache:', error.message);
  }
}

// Weather icon mapping
function getWeatherIcon(condition) {
  const iconMap = {
    'Clear': '☀️',
    'Clouds': '☁️',
    'Rain': '🌧️',
    'Drizzle': '🌦️',
    'Thunderstorm': '⛈️',
    'Snow': '❄️',
    'Mist': '🌫️',
    'Fog': '🌫️',
    'Haze': '🌫️'
  };
  return iconMap[condition] || '🌤️';
}

// Routes
app.get('/', async (req, res) => {
  try {
    const city = req.query.city || DEFAULT_CITY;
    
    // Update cache if it's empty or older than 10 minutes
    if (!weatherCache.current || !weatherCache.lastUpdate || 
        (new Date() - weatherCache.lastUpdate) > 10 * 60 * 1000) {
      await updateWeatherCache(city);
    }
    
    if (!weatherCache.current) {
      throw new Error('No weather data available');
    }
    
    res.render('dashboard', {
      current: weatherCache.current,
      forecast: weatherCache.forecast,
      lastUpdate: weatherCache.lastUpdate,
      getWeatherIcon: getWeatherIcon,
      moment: moment
    });
  } catch (error) {
    console.error('Error rendering dashboard:', error.message);
    res.status(500).render('error', { 
      error: error.message,
      apiConfigured: !!API_KEY 
    });
  }
});

app.get('/api/weather/current', async (req, res) => {
  try {
    const city = req.query.city || DEFAULT_CITY;
    const data = await fetchCurrentWeather(city);
    res.json(data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/weather/forecast', async (req, res) => {
  try {
    const city = req.query.city || DEFAULT_CITY;
    const data = await fetchForecast(city);
    res.json(data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    apiConfigured: !!API_KEY,
    cacheStatus: {
      hasCurrent: !!weatherCache.current,
      hasForecast: !!weatherCache.forecast,
      lastUpdate: weatherCache.lastUpdate
    }
  });
});

// Schedule weather updates every 10 minutes
cron.schedule('*/10 * * * *', () => {
  updateWeatherCache();
});

// Initial cache update on startup
if (API_KEY) {
  updateWeatherCache().catch(console.error);
} else {
  console.warn('Warning: OPENWEATHER_API_KEY not configured');
}

app.listen(PORT, () => {
  console.log(`Weather dashboard running on port ${PORT}`);
  console.log(`API Key configured: ${!!API_KEY}`);
});
