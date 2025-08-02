#!/usr/bin/env python3
"""
Simple HTTP server with CORS support for the weather dashboard
"""
import http.server
import socketserver
import os
from urllib.parse import parse_qs, urlparse
import json
import requests
from datetime import datetime

class WeatherHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/api/weather':
            self.handle_weather_api()
        else:
            super().do_GET()

    def handle_weather_api(self):
        try:
            # Get API key from environment
            api_key = os.getenv('OPENWEATHER_API_KEY')
            if not api_key or api_key == 'your_openweather_api_key':
                self.send_error(500, "OpenWeather API key not configured")
                return

            # Parse query parameters
            query_params = parse_qs(urlparse(self.path).query)
            endpoint = query_params.get('endpoint', ['current'])[0]
            lat = query_params.get('lat', ['38.2856'])[0]  # New Albany, IN
            lon = query_params.get('lon', ['-85.8242'])[0]
            units = query_params.get('units', ['imperial'])[0]

            if endpoint == 'current':
                url = f"https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={api_key}&units={units}"
            elif endpoint == 'forecast':
                url = f"https://api.openweathermap.org/data/2.5/forecast?lat={lat}&lon={lon}&appid={api_key}&units={units}"
            elif endpoint == 'onecall':
                url = f"https://api.openweathermap.org/data/3.0/onecall?lat={lat}&lon={lon}&appid={api_key}&units={units}"
            else:
                self.send_error(400, "Invalid endpoint")
                return

            response = requests.get(url, timeout=10)
            
            if response.status_code == 200:
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(response.content)
            else:
                self.send_error(response.status_code, f"OpenWeather API error: {response.text}")
                
        except requests.exceptions.RequestException as e:
            self.send_error(500, f"Network error: {str(e)}")
        except Exception as e:
            self.send_error(500, f"Server error: {str(e)}")

if __name__ == "__main__":
    PORT = 8081
    
    # Load environment variables
    env_file = "/home/lou/lou-media-stack/.env"
    if os.path.exists(env_file):
        with open(env_file, 'r') as f:
            for line in f:
                if line.strip() and not line.startswith('#'):
                    key, value = line.strip().split('=', 1)
                    os.environ[key] = value.strip('"\'')
    
    with socketserver.TCPServer(("", PORT), WeatherHandler) as httpd:
        print(f"Serving weather dashboard at http://localhost:{PORT}")
        print(f"Weather API proxy available at http://localhost:{PORT}/api/weather")
        print("Press Ctrl+C to stop the server")
        httpd.serve_forever()
