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
import androidx.lifecycle.viewmodel.compose.viewModel
import com.mediastack.controller.ui.theme.MediaStackControllerTheme
import com.mediastack.controller.viewmodels.MainViewModel

class MediaSearchActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MediaStackControllerTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    MediaSearchScreen()
                }
            }
        }
    }
}

@Composable
fun MediaSearchScreen() {
    var searchQuery by remember { mutableStateOf("") }
    var searchResults by remember { mutableStateOf(emptyList<String>()) }
    var isSearching by remember { mutableStateOf(false) }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Media Search",
            style = MaterialTheme.typography.headlineSmall,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        OutlinedTextField(
            value = searchQuery,
            onValueChange = { searchQuery = it },
            label = { Text("Search Movies, TV Shows, Music...") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        Button(
            onClick = {
                if (searchQuery.isNotEmpty()) {
                    isSearching = true
                    // Simulate search results
                    searchResults = listOf(
                        "Movie: $searchQuery (2023)",
                        "TV Show: $searchQuery Series",
                        "Album: $searchQuery - Greatest Hits",
                        "Episode: $searchQuery S01E01"
                    )
                    isSearching = false
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            if (isSearching) {
                CircularProgressIndicator(modifier = Modifier.size(16.dp))
            } else {
                Text("Search")
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        if (searchResults.isNotEmpty()) {
            Text(
                text = "Results:",
                style = MaterialTheme.typography.titleMedium,
                modifier = Modifier.padding(bottom = 8.dp)
            )
            
            LazyColumn {
                items(searchResults) { result ->
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp)
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(16.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text(
                                text = result,
                                modifier = Modifier.weight(1f)
                            )
                            Row {
                                Button(
                                    onClick = {
                                        // Add to download queue
                                        searchResults = searchResults.map {
                                            if (it == result) "✓ Added: $it" else it
                                        }
                                    },
                                    modifier = Modifier.padding(start = 8.dp)
                                ) {
                                    Text("Add")
                                }
                                Button(
                                    onClick = {
                                        // Open details
                                        searchResults = searchResults.map {
                                            if (it == result) "👁 Viewing: $it" else it
                                        }
                                    },
                                    modifier = Modifier.padding(start = 4.dp)
                                ) {
                                    Text("View")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
