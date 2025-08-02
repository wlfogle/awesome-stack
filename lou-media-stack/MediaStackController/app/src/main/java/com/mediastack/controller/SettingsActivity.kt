package com.mediastack.controller

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.mediastack.controller.ui.theme.MediaStackControllerTheme

class SettingsActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MediaStackControllerTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Column(
                        modifier = Modifier.padding(16.dp)
                    ) {
                        Text("Settings", style = MaterialTheme.typography.headlineSmall)
                        Spacer(modifier = Modifier.height(16.dp))
                        
                        Card(
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Column(
                                modifier = Modifier.padding(16.dp)
                            ) {
                                Text("Service Status", style = MaterialTheme.typography.titleMedium)
                                Spacer(modifier = Modifier.height(8.dp))
                                Text("• Jellyfin: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Plex: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Sonarr: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Radarr: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Lidarr: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Jackett: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Deluge: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Bazarr: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Portainer: ✅ Online", style = MaterialTheme.typography.bodyMedium)
                                Text("• Grafana: ❌ Offline (Port 3800)", style = MaterialTheme.typography.bodyMedium)
                                Text("• Authentik: ❌ Offline (Port 6080)", style = MaterialTheme.typography.bodyMedium)
                                Text("• Homarr: ❌ Offline (Port 3200)", style = MaterialTheme.typography.bodyMedium)
                            }
                        }
                        
                        Spacer(modifier = Modifier.height(16.dp))
                        
                        Card(
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Column(
                                modifier = Modifier.padding(16.dp)
                            ) {
                                Text("App Information", style = MaterialTheme.typography.titleMedium)
                                Spacer(modifier = Modifier.height(8.dp))
                                Text("Version: 1.0.0", style = MaterialTheme.typography.bodyMedium)
                                Text("Server: 192.168.12.204", style = MaterialTheme.typography.bodyMedium)
                                Text("Network: Stella WiFi", style = MaterialTheme.typography.bodyMedium)
                            }
                        }
                    }
                }
            }
        }
    }
}
