package com.mediastack.controller.models

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class MediaService(
    val id: String,
    val name: String,
    val description: String,
    val url: String,
    val port: Int,
    val status: ServiceStatus,
    val icon: String,
    val category: ServiceCategory,
    val type: String,
    val apiKey: String? = null,
    val hasWebUI: Boolean = true,
    val supportsAPI: Boolean = true,
    val version: String? = null,
    val lastChecked: Long = System.currentTimeMillis()
) : Parcelable

enum class ServiceStatus {
    ONLINE, OFFLINE, LOADING, ERROR, UNKNOWN
}

enum class ServiceCategory {
    MEDIA_SERVER,
    DOWNLOAD_CLIENT,
    INDEXER,
    CONTENT_MANAGEMENT,
    MONITORING,
    AUTHENTICATION,
    PROXY,
    TV_RECORDING,
    DASHBOARD,
    DATABASE,
    VPN
}

@Parcelize
data class ServiceAction(
    val id: String,
    val name: String,
    val description: String,
    val icon: String,
    val requiresConfirmation: Boolean = false
) : Parcelable

@Parcelize
data class ServiceStats(
    val cpu: Float,
    val memory: Float,
    val disk: Float,
    val network: Float,
    val uptime: Long
) : Parcelable

@Parcelize
data class MediaItem(
    val id: String,
    val title: String,
    val year: Int?,
    val overview: String?,
    val poster: String?,
    val type: MediaType,
    val service: String,
    val quality: String?,
    val size: Long?,
    val added: Long,
    val monitored: Boolean = false
) : Parcelable

enum class MediaType {
    MOVIE, TV_SHOW, EPISODE, ALBUM, TRACK, BOOK, COMIC
}

@Parcelize
data class DownloadItem(
    val id: String,
    val name: String,
    val status: DownloadStatus,
    val progress: Float,
    val eta: Long?,
    val downloadRate: Float,
    val uploadRate: Float,
    val size: Long,
    val downloaded: Long,
    val seeders: Int,
    val peers: Int,
    val category: String?
) : Parcelable

enum class DownloadStatus {
    DOWNLOADING, SEEDING, PAUSED, QUEUED, ERROR, COMPLETED
}

@Parcelize
data class EPGProgram(
    val id: String,
    val channelId: String,
    val title: String,
    val description: String?,
    val start: Long,
    val end: Long,
    val category: String?,
    val episode: String?,
    val season: String?,
    val rating: String?,
    val isRecording: Boolean = false,
    val isScheduled: Boolean = false
) : Parcelable

@Parcelize
data class TVChannel(
    val id: String,
    val name: String,
    val number: Int,
    val logo: String?,
    val category: String?,
    val isEnabled: Boolean = true,
    val currentProgram: EPGProgram? = null
) : Parcelable

@Parcelize
data class RecordingRule(
    val id: String,
    val name: String,
    val channelId: String?,
    val title: String?,
    val description: String?,
    val isEnabled: Boolean = true,
    val priority: Int = 0,
    val retention: Int = 0,
    val duplicateHandling: DuplicateHandling = DuplicateHandling.RECORD_ONCE
) : Parcelable

enum class DuplicateHandling {
    RECORD_ONCE, RECORD_SERIES, RECORD_ALL
}

@Parcelize
data class SystemStats(
    val totalMovies: Int,
    val totalTVShows: Int,
    val totalEpisodes: Int,
    val totalAlbums: Int,
    val activeDownloads: Int,
    val diskSpace: DiskSpace,
    val systemUptime: Long
) : Parcelable

@Parcelize
data class DiskSpace(
    val total: Long,
    val free: Long,
    val used: Long
) : Parcelable

// API Response models
data class ServiceResponse<T>(
    val success: Boolean,
    val data: T?,
    val message: String?,
    val timestamp: Long = System.currentTimeMillis()
)

data class LogEntry(
    val timestamp: Long,
    val level: String,
    val message: String,
    val source: String
)

data class ServiceConfig(
    val baseUrl: String,
    val apiKey: String?,
    val timeout: Long = 30000,
    val retryCount: Int = 3
)
