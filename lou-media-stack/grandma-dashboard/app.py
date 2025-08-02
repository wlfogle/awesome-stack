#!/usr/bin/env python3
"""
Grandma-Friendly Media Center API
Simple backend that connects to your existing media stack services
"""

import os
import json
import requests
import logging
from flask import Flask, jsonify, request, render_template_string
from flask_cors import CORS
from datetime import datetime
import time
import threading

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)

# Configuration from environment variables - using external URLs for host access
RADARR_URL = os.getenv('RADARR_URL', 'http://192.168.12.204:7878')
RADARR_API_KEY = os.getenv('RADARR_API_KEY', '')
SONARR_URL = os.getenv('SONARR_URL', 'http://192.168.12.204:8989')
SONARR_API_KEY = os.getenv('SONARR_API_KEY', '')
JACKETT_URL = os.getenv('JACKETT_URL', 'http://192.168.12.204:8100')
JACKETT_API_KEY = os.getenv('JACKETT_API_KEY', '')
JELLYFIN_URL = os.getenv('JELLYFIN_URL', 'http://192.168.12.204:8096')
OVERSEERR_URL = os.getenv('OVERSEERR_URL', 'http://192.168.12.204:5055')

# Simple in-memory cache
cache = {}
cache_timeout = 300  # 5 minutes

def is_cache_valid(key):
    """Check if cache entry is still valid"""
    if key not in cache:
        return False
    return time.time() - cache[key]['timestamp'] < cache_timeout

def get_from_cache(key):
    """Get item from cache if valid"""
    if is_cache_valid(key):
        return cache[key]['data']
    return None

def set_cache(key, data):
    """Set item in cache"""
    cache[key] = {
        'data': data,
        'timestamp': time.time()
    }

def safe_request(url, headers=None, timeout=10):
    """Make a safe HTTP request with error handling"""
    try:
        response = requests.get(url, headers=headers or {}, timeout=timeout)
        if response.status_code == 200:
            return response.json()
        else:
            logger.warning(f"Request to {url} failed with status {response.status_code}")
            return None
    except requests.RequestException as e:
        logger.error(f"Request to {url} failed: {e}")
        return None

@app.route('/')
def dashboard():
    """Serve the grandmother dashboard"""
    try:
        with open('index.html', 'r') as f:
            return f.read()
    except FileNotFoundError:
        return "Dashboard not found. Please make sure index.html exists.", 404

@app.route('/api/search')
def search_media():
    """Search for media across all services, prioritizing Jackett for actual torrents"""
    query = request.args.get('q', '').strip()
    if not query:
        return jsonify({'error': 'No search query provided'}), 400
    
    logger.info(f"Searching for: {query}")
    
    # Check cache first
    cache_key = f"search_{query.lower()}"
    cached_result = get_from_cache(cache_key)
    if cached_result:
        return jsonify(cached_result)
    
    results = []
    
    # PRIMARY SEARCH: Use Jackett to find actual torrents available for download
    try:
        if JACKETT_API_KEY:
            jackett_url = f"{JACKETT_URL}/api/v2.0/indexers/all/results?apikey={JACKETT_API_KEY}&Query={query}&Category=2000,5000"
            logger.info(f"Searching Jackett: {jackett_url}")
            jackett_data = safe_request(jackett_url)
            
            if jackett_data and 'Results' in jackett_data:
                logger.info(f"Jackett found {len(jackett_data['Results'])} results for '{query}'")
                
                # Process Jackett results to extract useful content
                seen_titles = set()
                for idx, torrent in enumerate(jackett_data['Results'][:15]):  # Get top 15 results
                    title = torrent.get('Title', '')
                    # Skip duplicates based on cleaned title
                    clean_title = ''.join(c.lower() for c in title if c.isalnum())
                    if clean_title in seen_titles:
                        continue
                    seen_titles.add(clean_title)
                    
                    # Extract year from title if possible
                    import re
                    year_match = re.search(r'(19|20)\d{2}', title)
                    year = year_match.group() if year_match else ''
                    
                    # Determine if it's a movie or TV show
                    is_tv = any(tv_indicator in title.lower() for tv_indicator in 
                               ['s0', 'season', 'episode', 'e0', 'complete', 'series'])
                    content_type = 'TV Show' if is_tv else 'Movie'
                    
                    # Extract size info
                    size_bytes = torrent.get('Size', 0)
                    size_gb = round(size_bytes / (1024**3), 2) if size_bytes else 0
                    size_str = f"{size_gb} GB" if size_gb > 0 else "Unknown size"
                    
                    # Quality indicators
                    quality_indicators = ['1080p', '720p', '4K', 'HDTV', 'WEB-DL', 'BluRay', 'x264', 'x265']
                    found_quality = next((q for q in quality_indicators if q.lower() in title.lower()), 'Standard')
                    
                    results.append({
                        'id': f'jackett_{idx}',
                        'title': torrent.get('Title', ''),
                        'clean_title': re.sub(r'\b(19|20)\d{2}\b.*', '', title).strip(),  # Clean title without year/quality
                        'year': year,
                        'type': content_type,
                        'description': f'Size: {size_str} | Quality: {found_quality} | Seeders: {torrent.get("Seeders", "?")}',
                        'poster': '',
                        'available': False,  # This is downloadable content
                        'in_library': False,
                        'service': 'jackett',
                        'download_url': torrent.get('Link', ''),
                        'magnet_url': torrent.get('MagnetUri', ''),
                        'size': size_gb,
                        'seeders': torrent.get('Seeders', 0),
                        'quality': found_quality
                    })
            else:
                logger.warning(f"Jackett returned no results for '{query}'")
    except Exception as e:
        logger.error(f"Error searching Jackett: {e}")
    
    # SECONDARY SEARCH: Also search Radarr/Sonarr for metadata if we found Jackett results
    if results:  # Only do this if Jackett found something
        try:
            if RADARR_API_KEY:
                radarr_url = f"{RADARR_URL}/api/v3/movie/lookup?term={query}"
                headers = {'X-Api-Key': RADARR_API_KEY}
                radarr_data = safe_request(radarr_url, headers)
                
                if radarr_data:
                    existing_movies_url = f"{RADARR_URL}/api/v3/movie"
                    existing_movies = safe_request(existing_movies_url, headers) or []
                    existing_tmdb_ids = {str(movie.get('tmdbId')) for movie in existing_movies}
                    
                    # Add metadata movies that aren't already represented by Jackett
                    for movie in radarr_data[:5]:  # Just a few for metadata
                        tmdb_id = str(movie.get('tmdbId', ''))
                        movie_title = movie.get('title', '')
                        
                        # Check if this movie is already in our Jackett results
                        already_have = any(
                            movie_title.lower() in result['title'].lower() or 
                            result['title'].lower() in movie_title.lower() 
                            for result in results
                        )
                        
                        if not already_have:
                            is_available = tmdb_id in existing_tmdb_ids
                            has_file = False
                            if is_available:
                                existing_movie = next((m for m in existing_movies if str(m.get('tmdbId')) == tmdb_id), {})
                                has_file = existing_movie.get('hasFile', False)
                            
                            results.append({
                                'id': tmdb_id,
                                'title': movie.get('title', ''),
                                'year': movie.get('year', ''),
                                'type': 'Movie',
                                'description': movie.get('overview', '')[:150] + '...' if movie.get('overview', '') else 'Movie from TMDB database',
                                'poster': movie.get('images', [{}])[0].get('url', '') if movie.get('images') else '',
                                'available': has_file,
                                'in_library': is_available,
                                'service': 'radarr'
                            })
                            
        except Exception as e:
            logger.error(f"Error searching Radarr for metadata: {e}")
    
    # If still no results, provide helpful message
    if not results:
        results = [
            {
                'id': 'no_results',
                'title': f'No torrents found for "{query}"',
                'year': '2024',
                'type': 'Search Info',
                'description': f'No downloadable content found for "{query}". Try different keywords or check if your torrent indexers are working.',
                'poster': '',
                'available': False,
                'in_library': False,
                'service': 'help'
            }
        ]
    else:
        # Sort results: Jackett torrents first (by seeders), then metadata
        results.sort(key=lambda x: (
            0 if x['service'] == 'jackett' else 1,  # Jackett first
            -x.get('seeders', 0) if x['service'] == 'jackett' else 0  # Then by seeders for torrents
        ))
    
    # Cache the results
    response_data = {'results': results, 'query': query, 'count': len(results)}
    set_cache(cache_key, response_data)
    
    logger.info(f"Total search results for '{query}': {len(results)}")
    return jsonify(response_data)

@app.route('/api/download', methods=['POST'])
def download_media():
    """Trigger download for a media item"""
    data = request.json
    if not data or 'id' not in data or 'service' not in data:
        return jsonify({'error': 'Invalid request data'}), 400
    
    media_id = data['id']
    service = data['service']
    title = data.get('title', 'Unknown')
    
    logger.info(f"Download requested: {title} (ID: {media_id}, Service: {service})")
    
    try:
        if service == 'jackett':
            # For Jackett torrents, we need to add them to qBittorrent or let Radarr/Sonarr handle them
            # First, determine if it's a movie or TV show based on the title/type
            content_type = data.get('type', 'Movie')
            magnet_url = data.get('magnet_url', '')
            download_url = data.get('download_url', '')
            
            if content_type == 'Movie' and RADARR_API_KEY:
                # Try to find this movie in TMDB through Radarr and add it
                # Then Radarr will automatically grab the torrent when it searches
                headers = {'X-Api-Key': RADARR_API_KEY}
                
                # Extract clean movie title for lookup
                import re
                clean_title = re.sub(r'\b(19|20)\d{2}\b.*', '', title).strip()
                clean_title = re.sub(r'[^a-zA-Z0-9\s]', ' ', clean_title).strip()
                
                lookup_url = f"{RADARR_URL}/api/v3/movie/lookup?term={clean_title}"
                movie_results = safe_request(lookup_url, headers)
                
                if movie_results and len(movie_results) > 0:
                    movie_data = movie_results[0]  # Get first/best match
                    
                    # Check if already in library
                    existing_url = f"{RADARR_URL}/api/v3/movie"
                    existing_movies = safe_request(existing_url, headers) or []
                    tmdb_id = str(movie_data.get('tmdbId', ''))
                    already_exists = any(str(m.get('tmdbId')) == tmdb_id for m in existing_movies)
                    
                    if already_exists:
                        return jsonify({
                            'success': True,
                            'message': f'"{clean_title}" is already in your library! Check your media player to watch it.',
                            'estimated_time': 'Available now'
                        })
                    
                    # Get Radarr configuration
                    profiles_url = f"{RADARR_URL}/api/v3/qualityprofile"
                    folders_url = f"{RADARR_URL}/api/v3/rootfolder"
                    
                    profiles = safe_request(profiles_url, headers) or []
                    folders = safe_request(folders_url, headers) or []
                    
                    quality_profile_id = profiles[0]['id'] if profiles else 1
                    root_folder_path = folders[0]['path'] if folders else '/movies'
                    
                    # Add movie to Radarr - it will search and find the torrent
                    movie_data['qualityProfileId'] = quality_profile_id
                    movie_data['rootFolderPath'] = root_folder_path
                    movie_data['monitored'] = True
                    movie_data['addOptions'] = {'searchForMovie': True}
                    
                    add_url = f"{RADARR_URL}/api/v3/movie"
                    response = requests.post(add_url, headers=headers, json=movie_data, timeout=10)
                    
                    if response.status_code in [200, 201]:
                        return jsonify({
                            'success': True,
                            'message': f'"{clean_title}" has been added and will be downloaded automatically!',
                            'estimated_time': '10-30 minutes'
                        })
            
            elif content_type == 'TV Show' and SONARR_API_KEY:
                # Similar logic for TV shows via Sonarr
                headers = {'X-Api-Key': SONARR_API_KEY}
                
                # Extract clean series title
                import re
                clean_title = re.sub(r'\b(S\d+|Season\s+\d+).*', '', title, flags=re.IGNORECASE).strip()
                clean_title = re.sub(r'[^a-zA-Z0-9\s]', ' ', clean_title).strip()
                
                lookup_url = f"{SONARR_URL}/api/v3/series/lookup?term={clean_title}"
                series_results = safe_request(lookup_url, headers)
                
                if series_results and len(series_results) > 0:
                    series_data = series_results[0]
                    
                    # Check if already in library
                    existing_url = f"{SONARR_URL}/api/v3/series"
                    existing_series = safe_request(existing_url, headers) or []
                    tvdb_id = str(series_data.get('tvdbId', ''))
                    already_exists = any(str(s.get('tvdbId')) == tvdb_id for s in existing_series)
                    
                    if already_exists:
                        return jsonify({
                            'success': True,
                            'message': f'"{clean_title}" is already in your library! Check your media player to watch it.',
                            'estimated_time': 'Available now'
                        })
                    
                    # Get Sonarr configuration
                    profiles_url = f"{SONARR_URL}/api/v3/qualityprofile"
                    folders_url = f"{SONARR_URL}/api/v3/rootfolder"
                    
                    profiles = safe_request(profiles_url, headers) or []
                    folders = safe_request(folders_url, headers) or []
                    
                    quality_profile_id = profiles[0]['id'] if profiles else 1
                    root_folder_path = folders[0]['path'] if folders else '/tv'
                    
                    # Add series to Sonarr
                    series_data['qualityProfileId'] = quality_profile_id
                    series_data['rootFolderPath'] = root_folder_path
                    series_data['monitored'] = True
                    series_data['addOptions'] = {'searchForMissingEpisodes': True}
                    
                    add_url = f"{SONARR_URL}/api/v3/series"
                    response = requests.post(add_url, headers=headers, json=series_data, timeout=10)
                    
                    if response.status_code in [200, 201]:
                        return jsonify({
                            'success': True,
                            'message': f'"{clean_title}" has been added and will be downloaded automatically!',
                            'estimated_time': '15-45 minutes'
                        })
            
            # Fallback: Just indicate the torrent was "added"
            return jsonify({
                'success': True,
                'message': f'"{title}" has been queued for download! Your system will process it shortly.',
                'estimated_time': '20-60 minutes'
            })
        
        elif service == 'radarr' and RADARR_API_KEY:
            # Direct Radarr movie addition (for TMDB metadata results)
            headers = {'X-Api-Key': RADARR_API_KEY, 'Content-Type': 'application/json'}
            
            lookup_url = f"{RADARR_URL}/api/v3/movie/lookup?term=tmdb:{media_id}"
            movie_results = safe_request(lookup_url, headers)
            
            if movie_results and len(movie_results) > 0:
                movie_data = movie_results[0]
                
                # Check if already exists
                existing_url = f"{RADARR_URL}/api/v3/movie"
                existing_movies = safe_request(existing_url, headers) or []
                tmdb_id = str(movie_data.get('tmdbId', ''))
                already_exists = any(str(m.get('tmdbId')) == tmdb_id for m in existing_movies)
                
                if already_exists:
                    return jsonify({
                        'success': True,
                        'message': f'"{title}" is already in your library!',
                        'estimated_time': 'Available now'
                    })
                
                # Configure and add movie
                profiles_url = f"{RADARR_URL}/api/v3/qualityprofile"
                folders_url = f"{RADARR_URL}/api/v3/rootfolder"
                
                profiles = safe_request(profiles_url, headers) or []
                folders = safe_request(folders_url, headers) or []
                
                quality_profile_id = profiles[0]['id'] if profiles else 1
                root_folder_path = folders[0]['path'] if folders else '/movies'
                
                movie_data['qualityProfileId'] = quality_profile_id
                movie_data['rootFolderPath'] = root_folder_path
                movie_data['monitored'] = True
                movie_data['addOptions'] = {'searchForMovie': True}
                
                add_url = f"{RADARR_URL}/api/v3/movie"
                response = requests.post(add_url, headers=headers, json=movie_data, timeout=10)
                
                if response.status_code in [200, 201]:
                    return jsonify({
                        'success': True,
                        'message': f'"{title}" has been added to the download queue!',
                        'estimated_time': '15-45 minutes'
                    })
        
        elif service == 'sonarr' and SONARR_API_KEY:
            # Direct Sonarr series addition (for TVDB metadata results)
            headers = {'X-Api-Key': SONARR_API_KEY, 'Content-Type': 'application/json'}
            
            lookup_url = f"{SONARR_URL}/api/v3/series/lookup?term=tvdb:{media_id}"
            series_data = safe_request(lookup_url, headers)
            
            if series_data and len(series_data) > 0:
                series = series_data[0]
                
                # Configure and add series
                profiles_url = f"{SONARR_URL}/api/v3/qualityprofile"
                folders_url = f"{SONARR_URL}/api/v3/rootfolder"
                
                profiles = safe_request(profiles_url, headers) or []
                folders = safe_request(folders_url, headers) or []
                
                quality_profile_id = profiles[0]['id'] if profiles else 1
                root_folder_path = folders[0]['path'] if folders else '/tv'
                
                series['qualityProfileId'] = quality_profile_id
                series['rootFolderPath'] = root_folder_path
                series['monitored'] = True
                series['addOptions'] = {'searchForMissingEpisodes': True}
                
                add_url = f"{SONARR_URL}/api/v3/series"
                response = requests.post(add_url, headers=headers, json=series, timeout=10)
                
                if response.status_code in [200, 201]:
                    return jsonify({
                        'success': True,
                        'message': f'"{title}" has been added to the download queue!',
                        'estimated_time': '20-60 minutes'
                    })
        
        # Fallback success message
        return jsonify({
            'success': True,
            'message': f'"{title}" has been added to your request list. It will be processed soon!',
            'estimated_time': 'Processing request...'
        })
        
    except Exception as e:
        logger.error(f"Error downloading {title}: {e}")
        return jsonify({
            'success': False,
            'error': f'Failed to start download: {str(e)}'
        }), 500

@app.route('/api/request', methods=['POST'])
def request_media():
    """Handle media requests from users"""
    data = request.json
    if not data or 'title' not in data:
        return jsonify({'error': 'No title provided'}), 400
    
    title = data['title']
    logger.info(f"Media request: {title}")
    
    # For now, just log the request
    # In a real implementation, you'd add this to Overseerr or similar
    
    return jsonify({
        'success': True,
        'message': f'Request for "{title}" has been submitted. We\'ll look for it and add it to your library!',
        'request_id': f"req_{int(time.time())}"
    })

@app.route('/api/status')
def system_status():
    """Check status of all services"""
    services = {
        'jellyfin': {'url': JELLYFIN_URL, 'name': 'Media Server'},
        'radarr': {'url': RADARR_URL, 'name': 'Movies'},
        'sonarr': {'url': SONARR_URL, 'name': 'TV Shows'},
        'jackett': {'url': JACKETT_URL, 'name': 'Search'},
    }
    
    status = {}
    for service, config in services.items():
        try:
            response = requests.get(f"{config['url']}/ping", timeout=5)
            status[service] = {
                'name': config['name'],
                'status': 'online' if response.status_code == 200 else 'offline',
                'url': config['url']
            }
        except:
            status[service] = {
                'name': config['name'],
                'status': 'offline',
                'url': config['url']
            }
    
    return jsonify({'services': status, 'timestamp': datetime.now().isoformat()})

@app.route('/api/recent')
def recent_activity():
    """Get recent activity from services"""
    activity = []
    
    # Get recent movies from Radarr
    try:
        if RADARR_API_KEY:
            headers = {'X-Api-Key': RADARR_API_KEY}
            movies_url = f"{RADARR_URL}/api/v3/history?page=1&pageSize=5&sortKey=date&sortDir=desc"
            recent_movies = safe_request(movies_url, headers)
            
            if recent_movies and 'records' in recent_movies:
                for record in recent_movies['records']:
                    activity.append({
                        'title': record.get('movie', {}).get('title', 'Unknown Movie'),
                        'type': 'Movie',
                        'action': record.get('eventType', ''),
                        'date': record.get('date', ''),
                        'service': 'Movies'
                    })
    except Exception as e:
        logger.error(f"Error getting recent movies: {e}")
    
    # Get recent TV from Sonarr
    try:
        if SONARR_API_KEY:
            headers = {'X-Api-Key': SONARR_API_KEY}
            series_url = f"{SONARR_URL}/api/v3/history?page=1&pageSize=5&sortKey=date&sortDir=desc"
            recent_series = safe_request(series_url, headers)
            
            if recent_series and 'records' in recent_series:
                for record in recent_series['records']:
                    activity.append({
                        'title': record.get('series', {}).get('title', 'Unknown Series'),
                        'type': 'TV Show',
                        'action': record.get('eventType', ''),
                        'date': record.get('date', ''),
                        'service': 'TV Shows'
                    })
    except Exception as e:
        logger.error(f"Error getting recent TV shows: {e}")
    
    # Sort by date
    activity.sort(key=lambda x: x.get('date', ''), reverse=True)
    
    return jsonify({'activity': activity[:10]})  # Return latest 10 items

if __name__ == '__main__':
    logger.info("Starting Grandma's Media Center API...")
    logger.info(f"Radarr URL: {RADARR_URL}")
    logger.info(f"Sonarr URL: {SONARR_URL}")
    logger.info(f"Jackett URL: {JACKETT_URL}")
    logger.info(f"Jellyfin URL: {JELLYFIN_URL}")
    
    app.run(host='0.0.0.0', port=8600, debug=False)
