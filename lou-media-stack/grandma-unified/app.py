#!/usr/bin/env python3
"""
Grandmother-Friendly Media Stack Interface
A unified, simple interface for all media stack operations
"""

from flask import Flask, render_template, request, jsonify, send_from_directory
from flask_cors import CORS
import requests
import json
import os
from datetime import datetime, timedelta
import logging
import xml.etree.ElementTree as ET
from urllib.parse import quote, unquote
import re
import psutil
import subprocess
from functools import wraps

app = Flask(__name__)
CORS(app)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Configuration
SERVER_IP = "192.168.12.204"
WEATHER_API_KEY = "1ba37ffe7ff87012d01b72e09a1a8d56"
LOCATION = "New Albany,IN,US"

# Service endpoints
SERVICES = {
    'sonarr': f'http://{SERVER_IP}:8989',
    'radarr': f'http://{SERVER_IP}:7878', 
    'lidarr': f'http://{SERVER_IP}:8686',
    'jackett': f'http://{SERVER_IP}:9117',
    'deluge': f'http://{SERVER_IP}:8112',
    'jellyfin': f'http://{SERVER_IP}:8096',
    'plex': f'http://{SERVER_IP}:32400'
}

# API Keys - these will be loaded from environment or config
API_KEYS = {
    'sonarr': os.getenv('SONARR_API_KEY', ''),
    'radarr': os.getenv('RADARR_API_KEY', ''),
    'lidarr': os.getenv('LIDARR_API_KEY', ''),
    'jackett': os.getenv('JACKETT_API_KEY', 'wmqr8zy5a9wn0k4kw8wkzn9uu9bppnch')
}

def get_service_status():
    """Check if all services are running"""
    status = {}
    for service, url in SERVICES.items():
        try:
            if service in ['sonarr', 'radarr', 'lidarr']:
                api_key = API_KEYS.get(service, '')
                response = requests.get(f"{url}/api/v3/system/status", 
                                      headers={'X-Api-Key': api_key}, 
                                      timeout=5)
                status[service] = response.status_code == 200
            elif service == 'jackett':
                response = requests.get(f"{url}/api/v2.0/indexers", 
                                      params={'apikey': API_KEYS['jackett']}, 
                                      timeout=5)
                status[service] = response.status_code == 200
            else:
                response = requests.get(url, timeout=5)
                status[service] = response.status_code == 200
        except Exception as e:
            logger.error(f"Error checking {service}: {e}")
            status[service] = False
    return status

def search_content(query, content_type='all'):
    """Search for content across all services"""
    results = []
    
    # Search Jackett for torrents
    try:
        jackett_url = f"{SERVICES['jackett']}/api/v2.0/indexers/all/results"
        params = {
            'apikey': API_KEYS['jackett'],
            'Query': query,
            'Category': '5000,2000,8000' if content_type == 'all' else ('5000' if content_type == 'tv' else '2000')
        }
        
        response = requests.get(jackett_url, params=params, timeout=10)
        if response.status_code == 200:
            # Parse Jackett XML response
            root = ET.fromstring(response.content)
            for item in root.findall('.//item'):
                title_elem = item.find('title')
                if title_elem is not None:
                    title = title_elem.text
                    # Extract basic info from title
                    result = {
                        'title': title,
                        'type': 'torrent',
                        'source': 'jackett',
                        'downloadable': True,
                        'link': item.find('link').text if item.find('link') is not None else '',
                        'size': item.find('size').text if item.find('size') is not None else 'Unknown'
                    }
                    results.append(result)
    except Exception as e:
        logger.error(f"Error searching Jackett: {e}")
    
    # Also search existing libraries
    if content_type in ['all', 'movie']:
        try:
            radarr_url = f"{SERVICES['radarr']}/api/v3/movie/lookup"
            params = {'term': query}
            response = requests.get(radarr_url, 
                                  headers={'X-Api-Key': API_KEYS['radarr']}, 
                                  params=params, timeout=10)
            if response.status_code == 200:
                for movie in response.json()[:5]:  # Limit results
                    result = {
                        'title': movie.get('title', 'Unknown'),
                        'year': movie.get('year', ''),
                        'type': 'movie',
                        'source': 'radarr',
                        'downloadable': not movie.get('hasFile', False),
                        'tmdb_id': movie.get('tmdbId'),
                        'overview': movie.get('overview', '')[:100] + '...' if movie.get('overview', '') else ''
                    }
                    results.append(result)
        except Exception as e:
            logger.error(f"Error searching Radarr: {e}")
    
    if content_type in ['all', 'tv']:
        try:
            sonarr_url = f"{SERVICES['sonarr']}/api/v3/series/lookup"
            params = {'term': query}
            response = requests.get(sonarr_url, 
                                  headers={'X-Api-Key': API_KEYS['sonarr']}, 
                                  params=params, timeout=10)
            if response.status_code == 200:
                for show in response.json()[:5]:  # Limit results
                    result = {
                        'title': show.get('title', 'Unknown'),
                        'year': show.get('year', ''),
                        'type': 'tv',
                        'source': 'sonarr',
                        'downloadable': not show.get('monitored', False),
                        'tvdb_id': show.get('tvdbId'),
                        'overview': show.get('overview', '')[:100] + '...' if show.get('overview', '') else ''
                    }
                    results.append(result)
        except Exception as e:
            logger.error(f"Error searching Sonarr: {e}")
    
    return results[:20]  # Return top 20 results

def download_content(title, content_type, source='auto'):
    """Download content using appropriate service"""
    try:
        if source == 'jackett' or source == 'auto':
            # Search Jackett and add to download client
            jackett_url = f"{SERVICES['jackett']}/api/v2.0/indexers/all/results"
            params = {
                'apikey': API_KEYS['jackett'],
                'Query': title
            }
            
            response = requests.get(jackett_url, params=params, timeout=10)
            if response.status_code == 200:
                root = ET.fromstring(response.content)
                items = root.findall('.//item')
                if items:
                    # Get the first (best) result
                    first_item = items[0]
                    magnet_link = None
                    
                    # Look for magnet link
                    for link in first_item.findall('.//link'):
                        if link.text and link.text.startswith('magnet:'):
                            magnet_link = link.text
                            break
                    
                    if magnet_link:
                        # Add to Deluge
                        deluge_url = f"{SERVICES['deluge']}/json"
                        deluge_data = {
                            "method": "web.add_torrents",
                            "params": [[{
                                "path": magnet_link,
                                "options": {}
                            }]],
                            "id": 1
                        }
                        
                        deluge_response = requests.post(deluge_url, 
                                                      json=deluge_data, 
                                                      timeout=10)
                        
                        if deluge_response.status_code == 200:
                            return {"success": True, "message": f"Started downloading '{title}'"}
        
        # Fallback to *arr services
        if content_type == 'movie':
            # Add to Radarr
            radarr_url = f"{SERVICES['radarr']}/api/v3/movie/lookup"
            params = {'term': title}
            response = requests.get(radarr_url, 
                                  headers={'X-Api-Key': API_KEYS['radarr']}, 
                                  params=params, timeout=10)
            if response.status_code == 200 and response.json():
                movie = response.json()[0]
                add_url = f"{SERVICES['radarr']}/api/v3/movie"
                movie_data = {
                    "title": movie['title'],
                    "tmdbId": movie['tmdbId'],
                    "qualityProfileId": 1,
                    "rootFolderPath": "/data/media/movies",
                    "monitored": True,
                    "addOptions": {"searchForMovie": True}
                }
                add_response = requests.post(add_url, 
                                           headers={'X-Api-Key': API_KEYS['radarr']}, 
                                           json=movie_data, timeout=10)
                if add_response.status_code in [200, 201]:
                    return {"success": True, "message": f"Added '{title}' to movie downloads"}
        
        elif content_type == 'tv':
            # Add to Sonarr
            sonarr_url = f"{SERVICES['sonarr']}/api/v3/series/lookup"
            params = {'term': title}
            response = requests.get(sonarr_url, 
                                  headers={'X-Api-Key': API_KEYS['sonarr']}, 
                                  params=params, timeout=10)
            if response.status_code == 200 and response.json():
                show = response.json()[0]
                add_url = f"{SERVICES['sonarr']}/api/v3/series"
                show_data = {
                    "title": show['title'],
                    "tvdbId": show['tvdbId'],
                    "qualityProfileId": 1,
                    "languageProfileId": 1,
                    "rootFolderPath": "/data/media/tv",
                    "monitored": True,
                    "addOptions": {"searchForMissingEpisodes": True}
                }
                add_response = requests.post(add_url, 
                                           headers={'X-Api-Key': API_KEYS['sonarr']}, 
                                           json=show_data, timeout=10)
                if add_response.status_code in [200, 201]:
                    return {"success": True, "message": f"Added '{title}' to TV downloads"}
        
        return {"success": False, "message": f"Could not download '{title}'"}
        
    except Exception as e:
        logger.error(f"Error downloading {title}: {e}")
        return {"success": False, "message": f"Error downloading '{title}': {str(e)}"}

def get_weather():
    """Get current weather"""
    try:
        url = f"http://api.openweathermap.org/data/2.5/weather"
        params = {
            'q': LOCATION,
            'appid': WEATHER_API_KEY,
            'units': 'imperial'
        }
        
        response = requests.get(url, params=params, timeout=10)
        if response.status_code == 200:
            data = response.json()
            return {
                'location': data['name'],
                'temperature': round(data['main']['temp']),
                'description': data['weather'][0]['description'].title(),
                'humidity': data['main']['humidity'],
                'wind_speed': round(data['wind']['speed'])
            }
    except Exception as e:
        logger.error(f"Error getting weather: {e}")
    
    return {
        'location': 'New Albany',
        'temperature': 72,
        'description': 'Partly Cloudy',
        'humidity': 65,
        'wind_speed': 5
    }

# Routes
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status')
def api_status():
    return jsonify(get_service_status())

@app.route('/api/search')
def api_search():
    query = request.args.get('q', '')
    content_type = request.args.get('type', 'all')
    
    if not query:
        return jsonify({'error': 'No search query provided'})
    
    results = search_content(query, content_type)
    return jsonify({'results': results})

@app.route('/api/download', methods=['POST'])
def api_download():
    data = request.get_json()
    title = data.get('title', '')
    content_type = data.get('type', 'auto')
    source = data.get('source', 'auto')
    
    if not title:
        return jsonify({'error': 'No title provided'})
    
    result = download_content(title, content_type, source)
    return jsonify(result)

@app.route('/api/weather')
def api_weather():
    return jsonify(get_weather())

@app.route('/api/library')
def api_library():
    """Get recent additions to library"""
    recent = []
    
    # Get recent movies from Radarr
    try:
        response = requests.get(f"{SERVICES['radarr']}/api/v3/movie", 
                              headers={'X-Api-Key': API_KEYS['radarr']}, 
                              timeout=10)
        if response.status_code == 200:
            movies = response.json()
            for movie in movies:
                if movie.get('hasFile', False):
                    recent.append({
                        'title': movie.get('title', ''),
                        'year': movie.get('year', ''),
                        'type': 'movie',
                        'added': movie.get('added', '')
                    })
    except Exception as e:
        logger.error(f"Error getting movies: {e}")
    
    # Get recent shows from Sonarr  
    try:
        response = requests.get(f"{SERVICES['sonarr']}/api/v3/series", 
                              headers={'X-Api-Key': API_KEYS['sonarr']}, 
                              timeout=10)
        if response.status_code == 200:
            shows = response.json()
            for show in shows[:10]:  # Limit results
                if show.get('episodeFileCount', 0) > 0:
                    recent.append({
                        'title': show.get('title', ''),
                        'year': show.get('year', ''),
                        'type': 'tv',
                        'added': show.get('added', ''),
                        'episodes': show.get('episodeFileCount', 0)
                    })
    except Exception as e:
        logger.error(f"Error getting shows: {e}")
    
    # Sort by added date and return most recent
    recent.sort(key=lambda x: x.get('added', ''), reverse=True)
    return jsonify({'recent': recent[:10]})

@app.route('/static/<path:filename>')
def serve_static(filename):
    return send_from_directory('static', filename)

if __name__ == '__main__':
    print("🎬 Starting Grandmother-Friendly Media Interface...")
    print(f"🌐 Server will be available at: http://{SERVER_IP}:5000")
    print(f"📡 Services configured: {', '.join(SERVICES.keys())}")
    app.run(host='0.0.0.0', port=5000, debug=True)
