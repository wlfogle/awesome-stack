package com.mediastack.controller

import android.content.Intent
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.mediastack.controller.adapters.FireTVServiceAdapter
import com.mediastack.controller.models.MediaService
import com.mediastack.controller.viewmodels.MainViewModel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class FireTVActivity : AppCompatActivity() {
    private lateinit var viewModel: MainViewModel
    private lateinit var serviceAdapter: FireTVServiceAdapter
    private lateinit var recyclerView: RecyclerView
    private var selectedPosition = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_fire_tv)

        // Initialize ViewModel
        viewModel = ViewModelProvider(this)[MainViewModel::class.java]

        // Initialize Views
        initializeViews()
        setupGridView()
        observeViewModel()

        // Load services
        viewModel.loadServices()
    }

    private fun initializeViews() {
        recyclerView = findViewById(R.id.recyclerViewServices)
    }

    private fun setupGridView() {
        serviceAdapter = FireTVServiceAdapter(
            services = mutableListOf(),
            onServiceClick = { service ->
                openServiceWebView(service)
            },
            onServiceLongClick = { service ->
                showServiceOptions(service)
            }
        )
        
        recyclerView.apply {
            adapter = serviceAdapter
            layoutManager = GridLayoutManager(this@FireTVActivity, 3)
        }
    }

    private fun observeViewModel() {
        viewModel.services.observe(this) { services ->
            serviceAdapter.updateServices(services)
            if (services.isNotEmpty()) {
                recyclerView.requestFocus()
            }
        }
        
        viewModel.loading.observe(this) { isLoading ->
            // Update loading state in Fire TV UI
        }
        
        viewModel.error.observe(this) { error ->
            // Handle error in Fire TV UI
        }
    }

    private fun showServiceOptions(service: MediaService) {
        // Show service options for Fire TV
        showQuickActions()
    }

    private fun openServiceWebView(service: MediaService) {
        val intent = Intent(this, ServiceWebViewActivity::class.java).apply {
            putExtra("SERVICE_NAME", service.name)
            putExtra("SERVICE_URL", service.url)
            putExtra("SERVICE_ID", service.id)
        }
        startActivity(intent)
    }

    private fun handleServiceAction(service: MediaService, action: String) {
        CoroutineScope(Dispatchers.IO).launch {
            when (action) {
                "restart" -> viewModel.restartService(service.id)
                "stop" -> viewModel.stopService(service.id)
                "start" -> viewModel.startService(service.id)
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        return when (keyCode) {
            KeyEvent.KEYCODE_DPAD_UP -> {
                navigateUp()
                true
            }
            KeyEvent.KEYCODE_DPAD_DOWN -> {
                navigateDown()
                true
            }
            KeyEvent.KEYCODE_DPAD_LEFT -> {
                navigateLeft()
                true
            }
            KeyEvent.KEYCODE_DPAD_RIGHT -> {
                navigateRight()
                true
            }
            KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_ENTER -> {
                selectCurrentItem()
                true
            }
            KeyEvent.KEYCODE_MENU -> {
                showQuickActions()
                true
            }
            else -> super.onKeyDown(keyCode, event)
        }
    }

    private fun navigateUp() {
        val columns = 3
        val newPosition = selectedPosition - columns
        if (newPosition >= 0) {
            selectedPosition = newPosition
            updateSelection()
        }
    }

    private fun navigateDown() {
        val columns = 3
        val newPosition = selectedPosition + columns
        if (newPosition < serviceAdapter.services.size) {
            selectedPosition = newPosition
            updateSelection()
        }
    }

    private fun navigateLeft() {
        if (selectedPosition > 0) {
            selectedPosition--
            updateSelection()
        }
    }

    private fun navigateRight() {
        if (selectedPosition < serviceAdapter.services.size - 1) {
            selectedPosition++
            updateSelection()
        }
    }

    private fun updateSelection() {
        recyclerView.scrollToPosition(selectedPosition)
        // Update visual selection if needed
    }

    private fun selectCurrentItem() {
        if (selectedPosition < serviceAdapter.services.size) {
            val service = serviceAdapter.services[selectedPosition]
            openServiceWebView(service)
        }
    }

    private fun showQuickActions() {
        // Show Fire TV context menu with quick actions
        if (selectedPosition >= serviceAdapter.services.size) return
        val service = serviceAdapter.services[selectedPosition]
        val actions = arrayOf("Open", "Restart", "Stop", "View Logs")
        
        // Create and show Fire TV compatible dialog
        val dialog = android.app.AlertDialog.Builder(this)
            .setTitle("${service.name} Actions")
            .setItems(actions) { _, which ->
                when (which) {
                    0 -> openServiceWebView(service)
                    1 -> handleServiceAction(service, "restart")
                    2 -> handleServiceAction(service, "stop")
                    3 -> showServiceLogs(service)
                }
            }
            .create()
        
        dialog.show()
    }

    private fun showServiceLogs(service: MediaService) {
        val intent = Intent(this, LogsActivity::class.java).apply {
            putExtra("SERVICE_ID", service.id)
            putExtra("SERVICE_NAME", service.name)
        }
        startActivity(intent)
    }

    override fun onResume() {
        super.onResume()
        viewModel.refreshAllServices()
    }
}
