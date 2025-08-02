package com.mediastack.controller

import android.content.Intent
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.floatingactionbutton.FloatingActionButton
import com.mediastack.controller.adapters.ServiceAdapter
import com.mediastack.controller.models.MediaService
import com.mediastack.controller.viewmodels.MainViewModel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {
    private lateinit var viewModel: MainViewModel
    private lateinit var serviceAdapter: ServiceAdapter
    private lateinit var recyclerView: RecyclerView
    private lateinit var fabSearch: FloatingActionButton
    private lateinit var fabEPG: FloatingActionButton

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Initialize ViewModel
        viewModel = ViewModelProvider(this)[MainViewModel::class.java]

        // Initialize Views
        initializeViews()
        setupRecyclerView()
        setupFabs()
        observeViewModel()

        // Load services
        viewModel.loadServices()
    }

    private fun initializeViews() {
        recyclerView = findViewById(R.id.recyclerViewServices)
        fabSearch = findViewById(R.id.fabSearch)
        fabEPG = findViewById(R.id.fabEPG)
    }

    private fun setupRecyclerView() {
        serviceAdapter = ServiceAdapter(
            services = mutableListOf(),
            onServiceClick = { service ->
                openServiceDetails(service)
            },
            onServiceLongClick = { service ->
                showServiceOptions(service)
            }
        )
        
        recyclerView.apply {
            adapter = serviceAdapter
            layoutManager = GridLayoutManager(this@MainActivity, 2)
        }
    }

    private fun setupFabs() {
        fabSearch.setOnClickListener {
            startActivity(Intent(this, MediaSearchActivity::class.java))
        }
        
        fabEPG.setOnClickListener {
            startActivity(Intent(this, EPGActivity::class.java))
        }
    }

    private fun observeViewModel() {
        viewModel.services.observe(this) { services ->
            serviceAdapter.updateServices(services)
        }
        
        viewModel.loading.observe(this) { isLoading ->
            // Update loading state
        }
        
        viewModel.error.observe(this) { error ->
            // Handle error
        }
    }

    private fun openServiceDetails(service: MediaService) {
        val intent = Intent(this, ServiceDetailsActivity::class.java).apply {
            putExtra("SERVICE_ID", service.id)
            putExtra("SERVICE_NAME", service.name)
            putExtra("SERVICE_URL", service.url)
        }
        startActivity(intent)
    }

    private fun showServiceOptions(service: MediaService) {
        // Show options dialog for long press
    }

    private fun handleServiceAction(service: MediaService, action: String) {
        CoroutineScope(Dispatchers.IO).launch {
            when (action) {
                "restart" -> viewModel.restartService(service.id)
                "stop" -> viewModel.stopService(service.id)
                "start" -> viewModel.startService(service.id)
                "logs" -> showLogs(service)
            }
        }
    }

    private fun showLogs(service: MediaService) {
        // Implementation for showing logs
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_settings -> {
                startActivity(Intent(this, SettingsActivity::class.java))
                true
            }
            R.id.action_refresh -> {
                viewModel.refreshAllServices()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }
}
