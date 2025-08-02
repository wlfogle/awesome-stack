package com.mediastack.controller.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.compose.ui.platform.ComposeView
import com.mediastack.controller.models.MediaService
import com.mediastack.controller.ui.components.ServiceCard

class FireTVServiceAdapter(
    var services: List<MediaService>,
    private val onServiceClick: (MediaService) -> Unit,
    private val onServiceLongClick: (MediaService) -> Unit
) : RecyclerView.Adapter<FireTVServiceAdapter.FireTVServiceViewHolder>() {

    class FireTVServiceViewHolder(private val composeView: ComposeView) : RecyclerView.ViewHolder(composeView) {
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

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): FireTVServiceViewHolder {
        val composeView = ComposeView(parent.context)
        return FireTVServiceViewHolder(composeView)
    }

    override fun onBindViewHolder(holder: FireTVServiceViewHolder, position: Int) {
        holder.bind(services[position], onServiceClick, onServiceLongClick)
    }

    override fun getItemCount(): Int = services.size

    fun updateServices(newServices: List<MediaService>) {
        services = newServices
        notifyDataSetChanged()
    }
}
