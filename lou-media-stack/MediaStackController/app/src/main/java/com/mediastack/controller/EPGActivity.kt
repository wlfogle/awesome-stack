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

class EPGActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MediaStackControllerTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    EPGScreen()
                }
            }
        }
    }
}

@Composable
fun EPGScreen() {
    var programs by remember {
        mutableStateOf(listOf(
            "9:00 AM - BBC News",
            "9:30 AM - Morning Show",
            "10:00 AM - Documentary: Nature",
            "11:00 AM - Comedy Series",
            "12:00 PM - Lunch Break News",
            "1:00 PM - Movies: Action Film",
            "3:00 PM - Sports: Football",
            "5:00 PM - Evening News",
            "6:00 PM - Drama Series",
            "7:00 PM - Talk Show",
            "8:00 PM - Prime Time Movie",
            "10:00 PM - Late Night News"
        ))
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "TV Guide (EPG)",
            style = MaterialTheme.typography.headlineSmall,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Card(
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Today's Schedule",
                    style = MaterialTheme.typography.titleMedium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                LazyColumn {
                    items(programs) { program ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 8.dp),
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text(
                                text = program,
                                style = MaterialTheme.typography.bodyMedium,
                                modifier = Modifier.weight(1f)
                            )
                            Button(
                                onClick = {
                                    // Schedule recording
                                    programs = programs.map {
                                        if (it == program) "📺 Recording: $it" else it
                                    }
                                },
                                modifier = Modifier.size(80.dp, 32.dp)
                            ) {
                                Text("Record", style = MaterialTheme.typography.bodySmall)
                            }
                        }
                        Divider()
                    }
                }
            }
        }
    }
}
