package com.mediastack.controller.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.compose.ui.platform.ComposeView
import com.mediastack.controller.models.MediaService
import com.mediastack.controller.ui.components.ServiceCard

class ServiceAdapter(
    var services: List<MediaService>,
    private val onServiceClick: (MediaService) -> Unit,
    private val onServiceLongClick: (MediaService) -> Unit
) : RecyclerView.Adapter<ServiceAdapter.ServiceViewHolder>() {

    class ServiceViewHolder(private val composeView: ComposeView) : RecyclerView.ViewHolder(composeView) {
        fun bind(service: MediaService, onServiceClick: (MediaService) -> Unit, onServiceLongClick: (MediaService) -> Unit) {
            composeView.setContent {
                ServiceCard(
                    service = service,
                    onClick = { onServiceClick(service) },
                    onLongClick = { onServiceLongClick(service) }
                )
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ServiceViewHolder {
        val composeView = ComposeView(parent.context)
        return ServiceViewHolder(composeView)
    }

    override fun onBindViewHolder(holder: ServiceViewHolder, position: Int) {
        holder.bind(services[position], onServiceClick, onServiceLongClick)
    }

    override fun getItemCount(): Int = services.size

    fun updateServices(newServices: List<MediaService>) {
        services = newServices
        notifyDataSetChanged()
    }
}
