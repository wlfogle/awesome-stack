#!/usr/bin/env python3
"""
Enhanced Grandmother-Friendly Media Stack Dashboard
Complete solution with Weather, AI Search, PseudoTV, EPG, and DVR
"""

import os
import sys
import json
import requests
import time
import asyncio
import logging
from datetime import datetime, timedelta
from flask import Flask, render_template, request, jsonify, redirect, url_for, send_from_directory
from flask_cors import CORS
from cachetools import TTLCache
import xml.etree.ElementTree as ET
from fuzzywuzzy import fuzz
import sqlite3
import threading

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)

# Cache for API responses
cache = TTLCache(maxsize=100, ttl=300)  # 5 minute cache

# Configuration
RADARR_URL = os.getenv('RADARR_URL', 'http://mediastack-radarr:7878')
RADARR_API_KEY = os.getenv('RADARR_API_KEY', '')
SONARR_URL = os.getenv('SONARR_URL', 'http://mediastack-sonarr:8989')
SONARR_API_KEY = os.getenv('SONARR_API_KEY', '')
JACKETT_URL = os.getenv('JACKETT_URL', 'http://mediastack-jackett:9117')
JACKETT_API_KEY = os.getenv('JACKETT_API_KEY', '')
JELLYFIN_URL = os.getenv('JELLYFIN_URL', 'http://mediastack-jellyfin:8096')
OVERSEERR_URL = os.getenv('OVERSEERR_URL', 'http://mediastack-overseerr:5055')
PLEX_URL = os.getenv('PLEX_URL', 'http://mediastack-plex:32400')
PLEX_TOKEN = os.getenv('PLEX_TOKEN', '')
TVHEADEND_URL = os.getenv('TVHEADEND_URL', 'http://mediastack-tvheadend:9981')
IPTV_PROXY_URL = os.getenv('IPTV_PROXY_URL', 'http://mediastack-iptv-proxy:8080')
WEATHER_API_KEY = os.getenv('WEATHER_API_KEY', '')
WEATHER_LOCATION = os.getenv('WEATHER_LOCATION', 'New Albany, IN')
OPENAI_API_KEY = os.getenv('OPENAI_API_KEY', '')

# PseudoTV Configuration
PSEUDOTV_URL = 'http://localhost:8890'
EPG_URL = 'http://localhost:8888'
UNIFIED_EPG_URL = 'http://localhost:8888/epg.xml'
UNIFIED_PLAYLIST_URL = 'http://localhost:8888/playlist.m3u'

class WeatherService:
    """Weather service for dashboard"""
    
    def __init__(self):
        self.api_key = WEATHER_API_KEY
        self.location = WEATHER_LOCATION
        self.base_url = "https://api.openweathermap.org/data/2.5"
    
    def get_current_weather(self):
        """Get current weather conditions"""
        if not self.api_key:
            return self._get_demo_weather()
        
        try:
            url = f"{self.base_url}/weather?q={self.location}&appid={self.api_key}&units=imperial"
            response = requests.get(url, timeout=10)
            if response.status_code == 200:
                data = response.json()
                return {
                    'temperature': round(data['main']['temp']),
                    'feels_like': round(data['main']['feels_like']),
                    'description': data['weather'][0]['description'].title(),
                    'icon': data['weather'][0]['icon'],
                    'humidity': data['main']['humidity'],
                    'pressure': data['main']['pressure'],
                    'wind_speed': round(data['wind']['speed']),
                    'wind_direction': data['wind'].get('deg', 0),
                    'visibility': data.get('visibility', 0) / 1000,  # Convert to km
                    'location': data['name']
                }
        except Exception as e:
            logger.error(f"Weather API error: {e}")
        
        return self._get_demo_weather()
    
    def get_forecast(self):
        """Get 5-day weather forecast"""
        if not self.api_key:
            return self._get_demo_forecast()
        
        try:
            url = f"{self.base_url}/forecast?q={self.location}&appid={self.api_key}&units=imperial"
            response = requests.get(url, timeout=10)
            if response.status_code == 200:
                data = response.json()
                forecast = []
                for item in data['list'][:40]:  # 5 days, 8 per day
                    forecast.append({
                        'datetime': item['dt_txt'],
                        'temperature': round(item['main']['temp']),
                        'description': item['weather'][0]['description'].title(),
                        'icon': item['weather'][0]['icon'],
                        'precipitation': item.get('rain', {}).get('3h', 0)
                    })
                return forecast
        except Exception as e:
            logger.error(f"Forecast API error: {e}")
        
        return self._get_demo_forecast()
    
    def get_alerts(self):
        """Get weather alerts"""
        # For demo purposes, return no alerts
        return []
    
    def _get_demo_weather(self):
        """Demo weather data"""
        return {
            'temperature': 72,
            'feels_like': 75,
            'description': 'Partly Cloudy',
            'icon': '02d',
            'humidity': 65,
            'pressure': 1013,
            'wind_speed': 8,
            'wind_direction': 180,
            'visibility': 16,
            'location': 'New Albany, IN'
        }
    
    def _get_demo_forecast(self):
        """Demo forecast data"""
        base_temp = 72
        forecast = []
        for i in range(40):
            forecast.append({
                'datetime': (datetime.now() + timedelta(hours=i*3)).strftime('%Y-%m-%d %H:%M:%S'),
                'temperature': base_temp + (i % 10) - 5,
                'description': 'Partly Cloudy',
                'icon': '02d' if i % 2 == 0 else '01d',
                'precipitation': 0
            })
        return forecast

class AISearchService:
    """AI-powered search service"""
    
    def __init__(self):
        self.openai_key = OPENAI_API_KEY
    
    def search_content(self, query):
        """Search for content using AI recognition"""
        # Get content from all services
        movies = self._get_all_movies()
        shows = self._get_all_shows()
        
        # Simple fuzzy search (can be enhanced with AI)
        results = []
        
        # Search movies
        for movie in movies:
            title = movie.get('title', '')
            if fuzz.partial_ratio(query.lower(), title.lower()) > 60:
                results.append({
                    'type': 'movie',
                    'title': title,
                    'year': movie.get('year', ''),
                    'overview': movie.get('overview', ''),
                    'score': fuzz.partial_ratio(query.lower(), title.lower())
                })
        
        # Search TV shows
        for show in shows:
            title = show.get('title', '')
            if fuzz.partial_ratio(query.lower(), title.lower()) > 60:
                results.append({
                    'type': 'tv',
                    'title': title,
                    'year': show.get('year', ''),
                    'overview': show.get('overview', ''),
                    'score': fuzz.partial_ratio(query.lower(), title.lower())
                })
        
        # Sort by score
        results.sort(key=lambda x: x['score'], reverse=True)
        return results[:20]  # Return top 20 matches
    
    def _get_all_movies(self):
        """Get all movies from Radarr"""
        try:
            headers = {'X-Api-Key': RADARR_API_KEY}
            return make_request(f"{RADARR_URL}/api/v3/movie", headers) or []
        except:
            return []
    
    def _get_all_shows(self):
        """Get all TV shows from Sonarr"""
        try:
            headers = {'X-Api-Key': SONARR_API_KEY}
            return make_request(f"{SONARR_URL}/api/v3/series", headers) or []
        except:
            return []

class EPGService:
    """EPG and DVR management service"""
    
    def __init__(self):
        self.epg_url = UNIFIED_EPG_URL
        self.playlist_url = UNIFIED_PLAYLIST_URL
    
    def get_channel_list(self):
        """Get list of all channels"""
        try:
            response = requests.get(self.playlist_url, timeout=10)
            if response.status_code == 200:
                lines = response.text.split('\n')
                channels = []
                current_channel = {}
                
                for line in lines:
                    if line.startswith('#EXTINF:'):
                        # Parse channel info
                        parts = line.split(',', 1)
                        if len(parts) > 1:
                            current_channel['name'] = parts[1].strip()
                    elif line.startswith('http'):
                        current_channel['url'] = line.strip()
                        channels.append(current_channel.copy())
                        current_channel = {}
                
                return channels[:50]  # Return first 50 channels
        except Exception as e:
            logger.error(f"Channel list error: {e}")
        
        return self._get_demo_channels()
    
    def get_epg_data(self):
        """Get EPG data for channels"""
        try:
            response = requests.get(self.epg_url, timeout=10)
            if response.status_code == 200:
                # Parse XML EPG data
                root = ET.fromstring(response.text)
                programs = []
                
                for programme in root.findall('.//programme'):
                    programs.append({
                        'channel': programme.get('channel', ''),
                        'start': programme.get('start', ''),
                        'stop': programme.get('stop', ''),
                        'title': programme.find('title').text if programme.find('title') is not None else 'Unknown',
                        'desc': programme.find('desc').text if programme.find('desc') is not None else ''
                    })
                
                return programs[:100]  # Return first 100 programs
        except Exception as e:
            logger.error(f"EPG data error: {e}")
        
        return self._get_demo_epg()
    
    def schedule_recording(self, channel, start_time, duration, title):
        """Schedule a recording via TVHeadend"""
        try:
            # TVHeadend API call to schedule recording
            data = {
                'channel': channel,
                'start': start_time,
                'stop': start_time + duration,
                'title': title
            }
            response = requests.post(f"{TVHEADEND_URL}/api/dvr/entry/create", json=data, timeout=10)
            return response.status_code == 200
        except Exception as e:
            logger.error(f"Recording schedule error: {e}")
            return False
    
    def _get_demo_channels(self):
        """Demo channel data"""
        return [
            {'name': 'Action Movies 24/7', 'url': 'http://localhost:8890/stream/100'},
            {'name': 'Comedy Central Movies', 'url': 'http://localhost:8890/stream/101'},
            {'name': 'Horror Theatre', 'url': 'http://localhost:8890/stream/102'},
            {'name': 'Sitcom Central', 'url': 'http://localhost:8890/stream/200'},
            {'name': 'Drama Network', 'url': 'http://localhost:8890/stream/201'}
        ]
    
    def _get_demo_epg(self):
        """Demo EPG data"""
        now = datetime.now()
        return [
            {
                'channel': 'Action Movies 24/7',
                'start': now.strftime('%Y%m%d%H%M%S'),
                'stop': (now + timedelta(hours=2)).strftime('%Y%m%d%H%M%S'),
                'title': 'Die Hard',
                'desc': 'Action movie starring Bruce Willis'
            },
            {
                'channel': 'Comedy Central Movies',
                'start': now.strftime('%Y%m%d%H%M%S'),
                'stop': (now + timedelta(hours=1, minutes=30)).strftime('%Y%m%d%H%M%S'),
                'title': 'Groundhog Day',
                'desc': 'Comedy starring Bill Murray'
            }
        ]

# Initialize services
weather_service = WeatherService()
ai_search_service = AISearchService()
epg_service = EPGService()

def make_request(url, headers=None):
    """Make HTTP request with error handling and caching"""
    cache_key = f"{url}_{str(headers)}"
    if cache_key in cache:
        return cache[cache_key]
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            data = response.json()
            cache[cache_key] = data
            return data
        else:
            logger.error(f"HTTP {response.status_code} for {url}")
            return None
    except Exception as e:
        logger.error(f"Request failed for {url}: {e}")
        return None

@app.route('/')
def index():
    """Main grandmother dashboard"""
    # Get system status
    system_status = {
        'radarr': check_service(RADARR_URL, {'X-Api-Key': RADARR_API_KEY}),
        'sonarr': check_service(SONARR_URL, {'X-Api-Key': SONARR_API_KEY}),
        'jackett': check_service(JACKETT_URL),
        'jellyfin': check_service(JELLYFIN_URL),
        'overseerr': check_service(OVERSEERR_URL),
        'plex': check_service(PLEX_URL),
        'tvheadend': check_service(TVHEADEND_URL),
        'iptv_proxy': check_service(IPTV_PROXY_URL),
        'pseudotv': check_service(PSEUDOTV_URL),
        'epg': check_service(EPG_URL)
    }
    
    # Get weather data
    weather_data = {
        'current': weather_service.get_current_weather(),
        'forecast': weather_service.get_forecast()[:12],  # 12 hours
        'daily': weather_service.get_forecast()[::8][:7],  # 7 days
        'alerts': weather_service.get_alerts()
    }
    
    # Get recent activity
    recent_movies = get_recent_movies()
    recent_shows = get_recent_shows()
    
    # Get channel data
    channels = epg_service.get_channel_list()[:10]  # First 10 channels
    epg_data = epg_service.get_epg_data()[:10]  # First 10 programs
    
    return render_template('grandma_dashboard.html', 
                         system_status=system_status,
                         weather=weather_data,
                         recent_movies=recent_movies,
                         recent_shows=recent_shows,
                         channels=channels,
                         epg_data=epg_data)

@app.route('/weather')
def weather():
    """Weather dashboard page"""
    weather_data = {
        'current': weather_service.get_current_weather(),
        'hourly': weather_service.get_forecast()[:12],
        'daily': weather_service.get_forecast()[::8][:7],
        'alerts': weather_service.get_alerts()
    }
    return render_template('weather.html', weather=weather_data)

@app.route('/search')
def search():
    """Search page"""
    query = request.args.get('q', '')
    results = []
    
    if query:
        results = ai_search_service.search_content(query)
    
    return render_template('search.html', query=query, results=results)

@app.route('/tv')
def tv_guide():
    """TV Guide and EPG page"""
    channels = epg_service.get_channel_list()
    epg_data = epg_service.get_epg_data()
    
    return render_template('tv_guide.html', channels=channels, epg_data=epg_data)

@app.route('/pseudotv')
def pseudotv():
    """PseudoTV channels page"""
    try:
        # Get PseudoTV channel info
        response = requests.get(f"{PSEUDOTV_URL}/channels", timeout=10)
        if response.status_code == 200:
            channels = response.json()
        else:
            channels = []
    except:
        channels = []
    
    return render_template('pseudotv.html', channels=channels)

# API Endpoints
@app.route('/api/status')
def api_status():
    """API endpoint for service status"""
    return jsonify({
        'radarr': check_service(RADARR_URL, {'X-Api-Key': RADARR_API_KEY}),
        'sonarr': check_service(SONARR_URL, {'X-Api-Key': SONARR_API_KEY}),
        'jackett': check_service(JACKETT_URL),
        'jellyfin': check_service(JELLYFIN_URL),
        'overseerr': check_service(OVERSEERR_URL),
        'plex': check_service(PLEX_URL),
        'tvheadend': check_service(TVHEADEND_URL),
        'pseudotv': check_service(PSEUDOTV_URL),
        'timestamp': datetime.now().isoformat()
    })

@app.route('/api/weather')
def api_weather():
    """API endpoint for weather data"""
    return jsonify({
        'current': weather_service.get_current_weather(),
        'forecast': weather_service.get_forecast(),
        'alerts': weather_service.get_alerts()
    })

@app.route('/api/search')
def api_search():
    """API endpoint for content search"""
    query = request.args.get('q', '')
    if query:
        results = ai_search_service.search_content(query)
        return jsonify({'results': results})
    return jsonify({'results': []})

@app.route('/api/record', methods=['POST'])
def api_record():
    """API endpoint for scheduling recordings"""
    data = request.json
    success = epg_service.schedule_recording(
        data.get('channel'),
        data.get('start_time'),
        data.get('duration'),
        data.get('title')
    )
    return jsonify({'success': success})

@app.route('/health')
def health():
    """Health check endpoint"""
    return jsonify({'status': 'healthy', 'timestamp': datetime.now().isoformat()})

# Helper functions
def check_service(url, headers=None):
    """Check if a service is running"""
    try:
        if 'radarr' in url or 'sonarr' in url:
            test_url = f"{url}/api/v3/system/status"
        elif 'jellyfin' in url:
            test_url = f"{url}/System/Info/Public"
        elif 'plex' in url:
            test_url = f"{url}/identity"
        elif 'tvheadend' in url:
            test_url = f"{url}/api/serverinfo"
        else:
            test_url = url
            
        response = requests.get(test_url, headers=headers, timeout=5)
        return response.status_code == 200
    except:
        return False

def get_recent_movies():
    """Get recently added movies"""
    try:
        headers = {'X-Api-Key': RADARR_API_KEY}
        movies = make_request(f"{RADARR_URL}/api/v3/movie", headers)
        if movies:
            recent = sorted(movies, key=lambda x: x.get('added', ''), reverse=True)[:5]
            return [{'title': m.get('title', 'Unknown'), 'year': m.get('year', '')} for m in recent]
    except Exception as e:
        logger.error(f"Error getting recent movies: {e}")
    return []

def get_recent_shows():
    """Get recently added TV shows"""
    try:
        headers = {'X-Api-Key': SONARR_API_KEY}
        shows = make_request(f"{SONARR_URL}/api/v3/series", headers)
        if shows:
            recent = sorted(shows, key=lambda x: x.get('added', ''), reverse=True)[:5]
            return [{'title': s.get('title', 'Unknown'), 'year': s.get('year', '')} for s in recent]
    except Exception as e:
        logger.error(f"Error getting recent shows: {e}")
    return []

if __name__ == '__main__':
    print("🏠 Starting Enhanced Grandmother Dashboard...")
    print(f"🎬 Radarr: {RADARR_URL}")
    print(f"📺 Sonarr: {SONARR_URL}")
    print(f"🔍 Jackett: {JACKETT_URL}")
    print(f"🎥 Jellyfin: {JELLYFIN_URL}")
    print(f"📋 Overseerr: {OVERSEERR_URL}")
    print(f"🎭 Plex: {PLEX_URL}")
    print(f"📡 TVHeadend: {TVHEADEND_URL}")
    print(f"📺 PseudoTV: {PSEUDOTV_URL}")
    print(f"🌤️ Weather: {WEATHER_LOCATION}")
    print("🚀 Enhanced Dashboard ready at http://0.0.0.0:8600")
    
    app.run(host='0.0.0.0', port=8600, debug=False)
