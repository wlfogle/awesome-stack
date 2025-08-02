#!/usr/bin/env python3
"""
LunaSea Linux Controller
This script provides a comprehensive interface to manage and control your media stack
using LunaSea modules and the existing service definitions.
"""

import json
import requests
import subprocess
import sys
import time
from dataclasses import dataclass
from typing import Dict, List, Optional, Any
from enum import Enum

class ServiceStatus(Enum):
    ONLINE = "online"
    OFFLINE = "offline"
    LOADING = "loading"
    ERROR = "error"
    UNKNOWN = "unknown"

class ServiceCategory(Enum):
    MEDIA_SERVER = "media_server"
    DOWNLOAD_CLIENT = "download_client"
    INDEXER = "indexer"
    CONTENT_MANAGEMENT = "content_management"
    MONITORING = "monitoring"
    AUTHENTICATION = "authentication"
    PROXY = "proxy"
    TV_RECORDING = "tv_recording"
    DASHBOARD = "dashboard"
    DATABASE = "database"
    VPN = "vpn"

@dataclass
class MediaService:
    id: str
    name: str
    description: str
    url: str
    port: int
    status: ServiceStatus
    icon: str
    category: ServiceCategory
    type: str
    api_key: Optional[str] = None
    has_web_ui: bool = True
    supports_api: bool = True
    version: Optional[str] = None

class LunaSeaLinuxController:
    def __init__(self):
        self.services = self._load_services()
        self.session = requests.Session()
        self.session.timeout = 30
        
    def _load_services(self) -> List[MediaService]:
        """Load services from the existing configuration"""
        services = [
            MediaService(
                id="jellyfin",
                name="Jellyfin",
                description="Media Server",
                url="http://192.168.12.204:8096",
                port=8096,
                status=ServiceStatus.UNKNOWN,
                icon="jellyfin",
                category=ServiceCategory.MEDIA_SERVER,
                type="Media Server"
            ),
            MediaService(
                id="plex",
                name="Plex",
                description="Alternative Media Server",
                url="http://192.168.12.204:32400",
                port=32400,
                status=ServiceStatus.UNKNOWN,
                icon="plex",
                category=ServiceCategory.MEDIA_SERVER,
                type="Media Server"
            ),
            MediaService(
                id="sonarr",
                name="Sonarr",
                description="TV Series Management",
                url="http://192.168.12.204:8989",
                port=8989,
                status=ServiceStatus.UNKNOWN,
                icon="sonarr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="TV Series",
                api_key="a0a1421101bb471a8db85f4affeb7410"
            ),
            MediaService(
                id="radarr",
                name="Radarr",
                description="Movie Management",
                url="http://192.168.12.204:7878",
                port=7878,
                status=ServiceStatus.UNKNOWN,
                icon="radarr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="Movie",
                api_key="9088fc58d3da47b9b67feac5c83a279b"
            ),
            MediaService(
                id="lidarr",
                name="Lidarr",
                description="Music Management",
                url="http://192.168.12.204:8686",
                port=8686,
                status=ServiceStatus.UNKNOWN,
                icon="lidarr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="Music"
            ),
            MediaService(
                id="bazarr",
                name="Bazarr",
                description="Subtitles Management",
                url="http://192.168.12.204:6767",
                port=6767,
                status=ServiceStatus.UNKNOWN,
                icon="bazarr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="Subtitles",
                api_key="8c93513725bba49fea8fd0d3685e5ff2"
            ),
            MediaService(
                id="jackett",
                name="Jackett",
                description="Indexer Management",
                url="http://192.168.12.204:9117",
                port=9117,
                status=ServiceStatus.UNKNOWN,
                icon="jackett",
                category=ServiceCategory.INDEXER,
                type="Indexer"
            ),
            MediaService(
                id="deluge",
                name="Deluge",
                description="Torrent Client",
                url="http://192.168.12.204:8112",
                port=8112,
                status=ServiceStatus.UNKNOWN,
                icon="deluge",
                category=ServiceCategory.DOWNLOAD_CLIENT,
                type="Torrent"
            ),
            MediaService(
                id="overseerr",
                name="Overseerr",
                description="Request Management for Plex",
                url="http://192.168.12.204:5056",
                port=5056,
                status=ServiceStatus.UNKNOWN,
                icon="overseerr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="Request Management"
            ),
            MediaService(
                id="jellyseerr",
                name="Jellyseerr",
                description="Request Management for Jellyfin",
                url="http://192.168.12.204:5055",
                port=5055,
                status=ServiceStatus.UNKNOWN,
                icon="jellyseerr",
                category=ServiceCategory.CONTENT_MANAGEMENT,
                type="Request Management"
            )
        ]
        return services

    def check_service_status(self, service: MediaService) -> ServiceStatus:
        """Check if a service is online and responsive"""
        try:
            response = self.session.get(f"{service.url}/health", timeout=10)
            if response.status_code == 200:
                return ServiceStatus.ONLINE
            else:
                return ServiceStatus.ERROR
        except requests.exceptions.RequestException:
            try:
                # Try a simple ping to the service
                response = self.session.get(service.url, timeout=10)
                if response.status_code in [200, 401, 403]:  # Service is up but may require auth
                    return ServiceStatus.ONLINE
                else:
                    return ServiceStatus.ERROR
            except:
                return ServiceStatus.OFFLINE

    def refresh_all_services(self):
        """Refresh status of all services"""
        print("Refreshing all services...")
        for service in self.services:
            print(f"Checking {service.name}...")
            service.status = self.check_service_status(service)
            print(f"  Status: {service.status.value}")

    def get_service_by_id(self, service_id: str) -> Optional[MediaService]:
        """Get a service by its ID"""
        return next((s for s in self.services if s.id == service_id), None)

    def sonarr_integration(self, service: MediaService) -> Dict[str, Any]:
        """LunaSea Sonarr module integration"""
        if not service.api_key:
            return {"error": "API key required for Sonarr"}
        
        try:
            headers = {"X-Api-Key": service.api_key}
            
            # Get series list
            series_response = self.session.get(f"{service.url}/api/v3/series", headers=headers)
            if series_response.status_code == 200:
                series_data = series_response.json()
                
                # Get system status
                status_response = self.session.get(f"{service.url}/api/v3/system/status", headers=headers)
                system_status = status_response.json() if status_response.status_code == 200 else {}
                
                return {
                    "series_count": len(series_data),
                    "system_status": system_status,
                    "recent_series": series_data[:5] if series_data else []
                }
            else:
                return {"error": f"Failed to connect to Sonarr: {series_response.status_code}"}
        except Exception as e:
            return {"error": f"Sonarr integration error: {str(e)}"}

    def radarr_integration(self, service: MediaService) -> Dict[str, Any]:
        """LunaSea Radarr module integration"""
        if not service.api_key:
            return {"error": "API key required for Radarr"}
        
        try:
            headers = {"X-Api-Key": service.api_key}
            
            # Get movies list
            movies_response = self.session.get(f"{service.url}/api/v3/movie", headers=headers)
            if movies_response.status_code == 200:
                movies_data = movies_response.json()
                
                # Get system status
                status_response = self.session.get(f"{service.url}/api/v3/system/status", headers=headers)
                system_status = status_response.json() if status_response.status_code == 200 else {}
                
                return {
                    "movies_count": len(movies_data),
                    "system_status": system_status,
                    "recent_movies": movies_data[:5] if movies_data else []
                }
            else:
                return {"error": f"Failed to connect to Radarr: {movies_response.status_code}"}
        except Exception as e:
            return {"error": f"Radarr integration error: {str(e)}"}

    def lidarr_integration(self, service: MediaService) -> Dict[str, Any]:
        """LunaSea Lidarr module integration"""
        if not service.api_key:
            return {"error": "API key required for Lidarr"}
        
        try:
            headers = {"X-Api-Key": service.api_key}
            
            # Get albums list
            albums_response = self.session.get(f"{service.url}/api/v1/album", headers=headers)
            if albums_response.status_code == 200:
                albums_data = albums_response.json()
                
                # Get system status
                status_response = self.session.get(f"{service.url}/api/v1/system/status", headers=headers)
                system_status = status_response.json() if status_response.status_code == 200 else {}
                
                return {
                    "albums_count": len(albums_data),
                    "system_status": system_status,
                    "recent_albums": albums_data[:5] if albums_data else []
                }
            else:
                return {"error": f"Failed to connect to Lidarr: {albums_response.status_code}"}
        except Exception as e:
            return {"error": f"Lidarr integration error: {str(e)}"}

    def launch_lunasea_desktop(self):
        """Launch LunaSea desktop application"""
        try:
            subprocess.run(["lunasea"], check=True)
        except subprocess.CalledProcessError:
            print("Failed to launch LunaSea desktop app")
        except FileNotFoundError:
            print("LunaSea desktop app not found in PATH")

    def get_service_dashboard(self) -> Dict[str, Any]:
        """Get a dashboard view of all services"""
        dashboard = {
            "total_services": len(self.services),
            "online_services": 0,
            "offline_services": 0,
            "error_services": 0,
            "services_by_category": {},
            "recent_activity": []
        }
        
        for service in self.services:
            if service.status == ServiceStatus.ONLINE:
                dashboard["online_services"] += 1
            elif service.status == ServiceStatus.OFFLINE:
                dashboard["offline_services"] += 1
            elif service.status == ServiceStatus.ERROR:
                dashboard["error_services"] += 1
            
            category = service.category.value
            if category not in dashboard["services_by_category"]:
                dashboard["services_by_category"][category] = []
            dashboard["services_by_category"][category].append({
                "name": service.name,
                "status": service.status.value,
                "url": service.url
            })
        
        return dashboard

    def run_interactive_mode(self):
        """Run the controller in interactive mode"""
        print("LunaSea Linux Controller - Interactive Mode")
        print("=" * 50)
        
        while True:
            print("\nOptions:")
            print("1. Refresh all services")
            print("2. View service dashboard")
            print("3. Check specific service")
            print("4. Launch LunaSea desktop")
            print("5. Sonarr integration")
            print("6. Radarr integration")
            print("7. Lidarr integration")
            print("8. Exit")
            
            choice = input("\nEnter your choice (1-8): ").strip()
            
            if choice == "1":
                self.refresh_all_services()
            elif choice == "2":
                dashboard = self.get_service_dashboard()
                print(json.dumps(dashboard, indent=2))
            elif choice == "3":
                service_id = input("Enter service ID: ").strip()
                service = self.get_service_by_id(service_id)
                if service:
                    status = self.check_service_status(service)
                    print(f"{service.name} ({service.url}): {status.value}")
                else:
                    print("Service not found")
            elif choice == "4":
                self.launch_lunasea_desktop()
            elif choice == "5":
                service = self.get_service_by_id("sonarr")
                if service:
                    result = self.sonarr_integration(service)
                    print(json.dumps(result, indent=2))
                else:
                    print("Sonarr service not found")
            elif choice == "6":
                service = self.get_service_by_id("radarr")
                if service:
                    result = self.radarr_integration(service)
                    print(json.dumps(result, indent=2))
                else:
                    print("Radarr service not found")
            elif choice == "7":
                service = self.get_service_by_id("lidarr")
                if service:
                    result = self.lidarr_integration(service)
                    print(json.dumps(result, indent=2))
                else:
                    print("Lidarr service not found")
            elif choice == "8":
                print("Goodbye!")
                break
            else:
                print("Invalid choice. Please try again.")

def main():
    controller = LunaSeaLinuxController()
    
    if len(sys.argv) > 1:
        command = sys.argv[1]
        if command == "refresh":
            controller.refresh_all_services()
        elif command == "dashboard":
            dashboard = controller.get_service_dashboard()
            print(json.dumps(dashboard, indent=2))
        elif command == "launch":
            controller.launch_lunasea_desktop()
        else:
            print(f"Unknown command: {command}")
    else:
        controller.run_interactive_mode()

if __name__ == "__main__":
    main()
