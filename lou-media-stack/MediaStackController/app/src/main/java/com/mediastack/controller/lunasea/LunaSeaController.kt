package com.mediastack.controller.lunasea

import com.mediastack.controller.models.MediaService
import com.mediastack.controller.viewmodels.MainViewModel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class LunaSeaController(private val viewModel: MainViewModel) {

    fun initializeLunaSeaModules() {
        // Integration with LunaSea modules e.g. Sonarr, Radarr
        viewModel.services.observeForever { services ->
            services.forEach { service ->
                initializeServiceModule(service)
            }
        }
    }

    private fun initializeServiceModule(service: MediaService) {
        // Example for initializing specific service modules
        when (service.id) {
            "sonarr" -> integrateSonarr(service)
            "radarr" -> integrateRadarr(service)
            "lidarr" -> integrateLidarr(service)
            // Add other integrations here
        }
    }

    private fun integrateSonarr(service: MediaService) {
        CoroutineScope(Dispatchers.IO).launch {
            // Logic for integrating with the Sonarr API
            println("Integrating with Sonarr at: ${service.url}")
            // Example: Fetch series from Sonarr API
        }
    }

    private fun integrateRadarr(service: MediaService) {
        CoroutineScope(Dispatchers.IO).launch {
            // Logic for integrating with the Radarr API
            println("Integrating with Radarr at: ${service.url}")
            // Example: Fetch movies from Radarr API
        }
    }

    private fun integrateLidarr(service: MediaService) {
        CoroutineScope(Dispatchers.IO).launch {
            // Logic for integrating with the Lidarr API
            println("Integrating with Lidarr at: ${service.url}")
            // Example: Fetch albums from Lidarr API
        }
    }
}

