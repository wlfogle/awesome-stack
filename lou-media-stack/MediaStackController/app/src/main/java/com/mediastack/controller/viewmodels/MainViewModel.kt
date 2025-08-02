package com.mediastack.controller.viewmodels

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.mediastack.controller.models.MediaService
import com.mediastack.controller.models.ServiceCategory
import com.mediastack.controller.models.ServiceStatus
import com.mediastack.controller.network.MediaStackApiService
import kotlinx.coroutines.launch

class MainViewModel : ViewModel() {
    private val apiService = MediaStackApiService()
    
    private val _services = MutableLiveData<List<MediaService>>()
    val services: LiveData<List<MediaService>> = _services
    
    private val _loading = MutableLiveData<Boolean>()
    val loading: LiveData<Boolean> = _loading
    
    private val _error = MutableLiveData<String?>()
    val error: LiveData<String?> = _error
    
    init {
        loadServices()
    }
    
    fun loadServices() {
        _loading.value = true
        _error.value = null
        
        viewModelScope.launch {
            try {
                val serviceList = createDefaultServices()
                _services.value = serviceList
                
                // Check status of each service
                refreshAllServices()
            } catch (e: Exception) {
                _error.value = e.message
            } finally {
                _loading.value = false
            }
        }
    }
    
    fun refreshAllServices() {
        viewModelScope.launch {
            val currentServices = _services.value ?: return@launch
            val updatedServices = currentServices.map { service ->
                try {
                    println("Checking service: ${service.name} at ${service.url}")
                    val status = apiService.checkServiceStatus(service.url)
                    println("Service ${service.name} status: $status")
                    service.copy(status = status, lastChecked = System.currentTimeMillis())
                } catch (e: Exception) {
                    println("Error checking service ${service.name}: ${e.message}")
                    service.copy(status = ServiceStatus.ERROR, lastChecked = System.currentTimeMillis())
                }
            }
            _services.value = updatedServices
        }
    }
    
    fun restartService(serviceId: String) {
        viewModelScope.launch {
            try {
                apiService.restartService(serviceId)
                refreshAllServices()
            } catch (e: Exception) {
                _error.value = "Failed to restart service: ${e.message}"
            }
        }
    }
    
    fun stopService(serviceId: String) {
        viewModelScope.launch {
            try {
                apiService.stopService(serviceId)
                refreshAllServices()
            } catch (e: Exception) {
                _error.value = "Failed to stop service: ${e.message}"
            }
        }
    }
    
    fun startService(serviceId: String) {
        viewModelScope.launch {
            try {
                apiService.startService(serviceId)
                refreshAllServices()
            } catch (e: Exception) {
                _error.value = "Failed to start service: ${e.message}"
            }
        }
    }
    
    private fun createDefaultServices(): List<MediaService> {
        return listOf(
            MediaService(
                id = "jellyfin",
                name = "Jellyfin",
                description = "Media Server",
                url = "http://192.168.12.204:8096",
                port = 8096,
                status = ServiceStatus.LOADING,
                icon = "jellyfin",
                category = ServiceCategory.MEDIA_SERVER,
                type = "Media Server",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "plex",
                name = "Plex",
                description = "Alternative Media Server",
                url = "http://192.168.12.204:32400",
                port = 32400,
                status = ServiceStatus.LOADING,
                icon = "plex",
                category = ServiceCategory.MEDIA_SERVER,
                type = "Media Server",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "sonarr",
                name = "Sonarr",
                description = "TV Series Management",
                url = "http://192.168.12.204:8989",
                port = 8989,
                status = ServiceStatus.LOADING,
                icon = "sonarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "TV Series",
                apiKey = "a0a1421101bb471a8db85f4affeb7410",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "radarr",
                name = "Radarr",
                description = "Movie Management",
                url = "http://192.168.12.204:7878",
                port = 7878,
                status = ServiceStatus.LOADING,
                icon = "radarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Movie",
                apiKey = "9088fc58d3da47b9b67feac5c83a279b",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "lidarr",
                name = "Lidarr",
                description = "Music Management",
                url = "http://192.168.12.204:8686",
                port = 8686,
                status = ServiceStatus.LOADING,
                icon = "lidarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Music",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "readarr",
                name = "Readarr",
                description = "Books and Audiobooks",
                url = "http://192.168.12.204:8787",
                port = 8787,
                status = ServiceStatus.LOADING,
                icon = "readarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Books",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "jackett",
                name = "Jackett",
                description = "Indexer Management",
                url = "http://192.168.12.204:9117",
                port = 9117,
                status = ServiceStatus.LOADING,
                icon = "jackett",
                category = ServiceCategory.INDEXER,
                type = "Indexer",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "deluge",
                name = "Deluge",
                description = "Torrent Client",
                url = "http://192.168.12.204:8112",
                port = 8112,
                status = ServiceStatus.LOADING,
                icon = "deluge",
                category = ServiceCategory.DOWNLOAD_CLIENT,
                type = "Torrent",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "bazarr",
                name = "Bazarr",
                description = "Subtitles Management",
                url = "http://192.168.12.204:6767",
                port = 6767,
                status = ServiceStatus.LOADING,
                icon = "bazarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Subtitles",
                apiKey = "8c93513725bba49fea8fd0d3685e5ff2",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "portainer",
                name = "Portainer",
                description = "Docker Management",
                url = "http://192.168.12.204:9000",
                port = 9000,
                status = ServiceStatus.LOADING,
                icon = "portainer",
                category = ServiceCategory.MONITORING,
                type = "Docker",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "authentik",
                name = "Authentik",
                description = "Authentication & Authorization",
                url = "http://192.168.12.204:9001",
                port = 9001,
                status = ServiceStatus.LOADING,
                icon = "authentik",
                category = ServiceCategory.AUTHENTICATION,
                type = "Authentication",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "grafana",
                name = "Grafana",
                description = "Analytics & Monitoring",
                url = "http://192.168.12.204:3000",
                port = 3000,
                status = ServiceStatus.LOADING,
                icon = "grafana",
                category = ServiceCategory.MONITORING,
                type = "Monitoring",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "homarr",
                name = "Homarr",
                description = "Modern Dashboard",
                url = "http://192.168.12.204:7575",
                port = 7575,
                status = ServiceStatus.LOADING,
                icon = "homarr",
                category = ServiceCategory.DASHBOARD,
                type = "Dashboard",
                hasWebUI = true,
                supportsAPI = false
            ),
            MediaService(
                id = "heimdall",
                name = "Heimdall",
                description = "Dashboard",
                url = "http://192.168.12.204:2080",
                port = 2080,
                status = ServiceStatus.LOADING,
                icon = "heimdall",
                category = ServiceCategory.DASHBOARD,
                type = "Dashboard",
                hasWebUI = true,
                supportsAPI = false
            ),
            MediaService(
                id = "jellyseerr",
                name = "Jellyseerr",
                description = "Request Management for Jellyfin",
                url = "http://192.168.12.204:5055",
                port = 5055,
                status = ServiceStatus.LOADING,
                icon = "jellyseerr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Request Management",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "overseerr",
                name = "Overseerr",
                description = "Request Management for Plex",
                url = "http://192.168.12.204:5056",
                port = 5056,
                status = ServiceStatus.LOADING,
                icon = "overseerr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Request Management",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "tdarr",
                name = "Tdarr",
                description = "Video Transcoding",
                url = "http://192.168.12.204:8265",
                port = 8265,
                status = ServiceStatus.LOADING,
                icon = "tdarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Transcoding",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "tvheadend",
                name = "TVHeadend",
                description = "TV Recording Server",
                url = "http://192.168.12.204:9981",
                port = 9981,
                status = ServiceStatus.LOADING,
                icon = "tvheadend",
                category = ServiceCategory.TV_RECORDING,
                type = "TV Recording",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "traefik",
                name = "Traefik",
                description = "Reverse Proxy Dashboard",
                url = "http://192.168.12.204:8080",
                port = 8080,
                status = ServiceStatus.LOADING,
                icon = "traefik",
                category = ServiceCategory.PROXY,
                type = "Reverse Proxy",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "mylar",
                name = "Mylar",
                description = "Comic Books Management",
                url = "http://192.168.12.204:8090",
                port = 8090,
                status = ServiceStatus.LOADING,
                icon = "mylar",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Comics",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "whisparr",
                name = "Whisparr",
                description = "Adult Content Management",
                url = "http://192.168.12.204:6969",
                port = 6969,
                status = ServiceStatus.LOADING,
                icon = "whisparr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Adult Content",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "audiobookshelf",
                name = "Audiobookshelf",
                description = "Audiobooks & eBooks",
                url = "http://192.168.12.204:13378",
                port = 13378,
                status = ServiceStatus.LOADING,
                icon = "audiobookshelf",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Audiobooks",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "calibre-web",
                name = "Calibre-Web",
                description = "eBook Management",
                url = "http://192.168.12.204:8083",
                port = 8083,
                status = ServiceStatus.LOADING,
                icon = "calibre",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "eBooks",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "flaresolverr",
                name = "FlareSolverr",
                description = "CloudFlare Bypass",
                url = "http://192.168.12.204:8191",
                port = 8191,
                status = ServiceStatus.LOADING,
                icon = "flaresolverr",
                category = ServiceCategory.PROXY,
                type = "CloudFlare Bypass",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "prometheus",
                name = "Prometheus",
                description = "Monitoring System",
                url = "http://192.168.12.204:9090",
                port = 9090,
                status = ServiceStatus.LOADING,
                icon = "prometheus",
                category = ServiceCategory.MONITORING,
                type = "Metrics",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "filebot",
                name = "FileBot",
                description = "File Renaming",
                url = "http://192.168.12.204:5800",
                port = 5800,
                status = ServiceStatus.LOADING,
                icon = "filebot",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "File Renaming",
                hasWebUI = true,
                supportsAPI = false
            ),
            MediaService(
                id = "channels-dvr",
                name = "Channels DVR",
                description = "Ultimate TV Recording",
                url = "http://192.168.12.204:8089",
                port = 8089,
                status = ServiceStatus.LOADING,
                icon = "channels",
                category = ServiceCategory.TV_RECORDING,
                type = "TV Recording",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "recyclarr",
                name = "Recyclarr",
                description = "Auto-sync TRaSH Guides to Sonarr/Radarr",
                url = "http://192.168.12.204:7878",
                port = 7878,
                status = ServiceStatus.LOADING,
                icon = "recyclarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Quality Management",
                hasWebUI = false,
                supportsAPI = true
            ),
            MediaService(
                id = "maintainerr",
                name = "Maintainerr",
                description = "Plex Ecosystem Maintenance Tool",
                url = "http://192.168.12.204:6246",
                port = 6246,
                status = ServiceStatus.LOADING,
                icon = "maintainerr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Library Maintenance",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "requestrr",
                name = "Requestrr",
                description = "Discord/Telegram Bot for Sonarr/Radarr",
                url = "http://192.168.12.204:4545",
                port = 4545,
                status = ServiceStatus.LOADING,
                icon = "requestrr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Request Bot",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "traktarr",
                name = "Traktarr",
                description = "Add media from Trakt.tv lists",
                url = "http://192.168.12.204:7337",
                port = 7337,
                status = ServiceStatus.LOADING,
                icon = "traktarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "List Automation",
                hasWebUI = false,
                supportsAPI = true
            ),
            MediaService(
                id = "exportarr",
                name = "Exportarr",
                description = "Prometheus Exporter for *arrs",
                url = "http://192.168.12.204:9707",
                port = 9707,
                status = ServiceStatus.LOADING,
                icon = "exportarr",
                category = ServiceCategory.MONITORING,
                type = "Metrics Exporter",
                hasWebUI = false,
                supportsAPI = true
            ),
            MediaService(
                id = "gaps",
                name = "Gaps",
                description = "Find missing movies in collections",
                url = "http://192.168.12.204:8484",
                port = 8484,
                status = ServiceStatus.LOADING,
                icon = "gaps",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Collection Manager",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "janitorr",
                name = "Janitorr",
                description = "Cleans Radarr/Sonarr/Jellyseerr before you run out of space",
                url = "http://192.168.12.204:8978",
                port = 8978,
                status = ServiceStatus.LOADING,
                icon = "janitorr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Storage Cleaner",
                hasWebUI = true,
                supportsAPI = true
            ),
            MediaService(
                id = "doplarr",
                name = "Doplarr",
                description = "Modern Discord bot for *arrs",
                url = "http://192.168.12.204:3939",
                port = 3939,
                status = ServiceStatus.LOADING,
                icon = "doplarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Discord Bot",
                hasWebUI = false,
                supportsAPI = true
            ),
            MediaService(
                id = "kapowarr",
                name = "Kapowarr",
                description = "Comic Book Management (Alternative to Mylar)",
                url = "http://192.168.12.204:5656",
                port = 5656,
                status = ServiceStatus.LOADING,
                icon = "kapowarr",
                category = ServiceCategory.CONTENT_MANAGEMENT,
                type = "Comic Books",
                hasWebUI = true,
                supportsAPI = true
            )
        )
    }
}
