const express = require('express');
const axios = require('axios');
const cron = require('node-cron');
const { v4: uuidv4 } = require('uuid');

const app = express();
const port = process.env.PORT || 3000;

app.use(express.json());

// Mock channels data
let channels = [
  {
    id: 'channel-1',
    name: 'Plex Movies',
    description: 'Random movies from Plex library',
    url: `http://plex:32400/video/:/transcode/universal/start?path=%2Flibrary%2Fmetadata%2F1&mediaIndex=0&partIndex=0&protocol=hls`,
    logo: '/images/plex-logo.png',
    category: 'Movies',
    current_program: {
      title: 'Loading...',
      description: 'Program information loading',
      start_time: new Date().toISOString(),
      end_time: new Date(Date.now() + 7200000).toISOString()
    }
  },
  {
    id: 'channel-2', 
    name: 'Jellyfin TV Shows',
    description: 'TV shows from Jellyfin library',
    url: `http://jellyfin:8096/Videos/stream?static=true`,
    logo: '/images/jellyfin-logo.png',
    category: 'TV Shows',
    current_program: {
      title: 'Loading...',
      description: 'Program information loading',
      start_time: new Date().toISOString(),
      end_time: new Date(Date.now() + 3600000).toISOString()
    }
  },
  {
    id: 'channel-3',
    name: 'Random Mix',
    description: 'Mixed content from all libraries',
    url: `http://plex:32400/video/:/transcode/universal/start?path=%2Flibrary%2Fmetadata%2F2&mediaIndex=0&partIndex=0&protocol=hls`,
    logo: '/images/mix-logo.png', 
    category: 'Mixed',
    current_program: {
      title: 'Loading...',
      description: 'Program information loading',
      start_time: new Date().toISOString(),
      end_time: new Date(Date.now() + 5400000).toISOString()
    }
  }
];

// Get all channels
app.get('/api/channels', (req, res) => {
  res.json({
    success: true,
    channels: channels
  });
});

// Get specific channel
app.get('/api/channels/:id', (req, res) => {
  const channel = channels.find(c => c.id === req.params.id);
  if (!channel) {
    return res.status(404).json({
      success: false,
      error: 'Channel not found'
    });
  }
  res.json({
    success: true,
    channel: channel
  });
});

// Get channel stream URL
app.get('/api/channels/:id/stream', (req, res) => {
  const channel = channels.find(c => c.id === req.params.id);
  if (!channel) {
    return res.status(404).json({
      success: false,
      error: 'Channel not found'
    });
  }
  res.json({
    success: true,
    stream_url: channel.url
  });
});

// Create new channel
app.post('/api/channels', (req, res) => {
  const { name, description, url, logo, category } = req.body;
  
  if (!name || !url) {
    return res.status(400).json({
      success: false,
      error: 'Name and URL are required'
    });
  }

  const newChannel = {
    id: `channel-${uuidv4()}`,
    name,
    description: description || '',
    url,
    logo: logo || '/images/default-logo.png',
    category: category || 'General',
    current_program: {
      title: 'New Channel',
      description: 'Recently created channel',
      start_time: new Date().toISOString(),
      end_time: new Date(Date.now() + 3600000).toISOString()
    }
  };

  channels.push(newChannel);
  
  res.status(201).json({
    success: true,
    channel: newChannel
  });
});

// Update channel
app.put('/api/channels/:id', (req, res) => {
  const channelIndex = channels.findIndex(c => c.id === req.params.id);
  if (channelIndex === -1) {
    return res.status(404).json({
      success: false,
      error: 'Channel not found'
    });
  }

  const { name, description, url, logo, category } = req.body;
  
  channels[channelIndex] = {
    ...channels[channelIndex],
    name: name || channels[channelIndex].name,
    description: description || channels[channelIndex].description,
    url: url || channels[channelIndex].url,
    logo: logo || channels[channelIndex].logo,
    category: category || channels[channelIndex].category
  };

  res.json({
    success: true,
    channel: channels[channelIndex]
  });
});

// Delete channel
app.delete('/api/channels/:id', (req, res) => {
  const channelIndex = channels.findIndex(c => c.id === req.params.id);
  if (channelIndex === -1) {
    return res.status(404).json({
      success: false,
      error: 'Channel not found'
    });
  }

  channels.splice(channelIndex, 1);
  
  res.json({
    success: true,
    message: 'Channel deleted successfully'
  });
});

// Get EPG data for all channels
app.get('/api/epg', (req, res) => {
  const epgData = channels.map(channel => ({
    channel_id: channel.id,
    channel_name: channel.name,
    programs: [
      channel.current_program,
      {
        title: 'Next Program',
        description: 'Upcoming content',
        start_time: channel.current_program.end_time,
        end_time: new Date(new Date(channel.current_program.end_time).getTime() + 3600000).toISOString()
      }
    ]
  }));

  res.json({
    success: true,
    epg: epgData
  });
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({
    status: 'healthy',
    timestamp: new Date().toISOString(),
    channels_count: channels.length
  });
});

// Update current programs periodically (every 30 minutes)
cron.schedule('*/30 * * * *', async () => {
  console.log('Updating channel programs...');
  
  // This would typically fetch real program data from Plex/Jellyfin
  channels.forEach(channel => {
    channel.current_program = {
      title: `Program ${Math.floor(Math.random() * 1000)}`,
      description: 'Auto-updated program content',
      start_time: new Date().toISOString(),
      end_time: new Date(Date.now() + (Math.random() * 7200000 + 1800000)).toISOString() // 30min to 2.5hr
    };
  });
  
  console.log('Channel programs updated');
});

app.listen(port, '0.0.0.0', () => {
  console.log(`PseudoTV Server running on port ${port}`);
  console.log(`Available endpoints:`);
  console.log(`  GET  /api/channels - List all channels`);
  console.log(`  GET  /api/channels/:id - Get specific channel`);
  console.log(`  GET  /api/channels/:id/stream - Get channel stream URL`);
  console.log(`  POST /api/channels - Create new channel`);
  console.log(`  PUT  /api/channels/:id - Update channel`);
  console.log(`  DELETE /api/channels/:id - Delete channel`);
  console.log(`  GET  /api/epg - Get EPG data`);
  console.log(`  GET  /health - Health check`);
});
