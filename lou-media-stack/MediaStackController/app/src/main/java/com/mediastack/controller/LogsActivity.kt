package com.mediastack.controller

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.mediastack.controller.ui.theme.MediaStackControllerTheme

class LogsActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val serviceId = intent.getStringExtra("SERVICE_ID") ?: "unknown"
        val serviceName = intent.getStringExtra("SERVICE_NAME") ?: "Service"
        
        setContent {
            MediaStackControllerTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    LogsScreen(serviceName, serviceId)
                }
            }
        }
    }
}

@Composable
fun LogsScreen(serviceName: String, serviceId: String) {
    val logs = remember {
        listOf(
            "[2025-07-15 17:13:00] INFO: $serviceName service started",
            "[2025-07-15 17:13:01] DEBUG: Configuration loaded successfully",
            "[2025-07-15 17:13:02] INFO: Database connection established",
            "[2025-07-15 17:13:03] INFO: API endpoints initialized",
            "[2025-07-15 17:13:04] INFO: $serviceName is ready to accept requests",
            "[2025-07-15 17:13:05] DEBUG: Health check passed",
            "[2025-07-15 17:13:06] INFO: Processing request from 192.168.12.x",
            "[2025-07-15 17:13:07] DEBUG: Cache updated",
            "[2025-07-15 17:13:08] INFO: Scheduled task completed",
            "[2025-07-15 17:13:09] INFO: System status: healthy"
        )
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "$serviceName Logs",
            style = MaterialTheme.typography.headlineSmall,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Card(
            modifier = Modifier.fillMaxSize()
        ) {
            LazyColumn(
                modifier = Modifier.padding(16.dp)
            ) {
                items(logs) { log ->
                    Text(
                        text = log,
                        style = MaterialTheme.typography.bodySmall,
                        modifier = Modifier.padding(vertical = 2.dp)
                    )
                }
            }
        }
    }
}
