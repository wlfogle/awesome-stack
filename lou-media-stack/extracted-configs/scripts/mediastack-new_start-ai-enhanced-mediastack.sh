#!/bin/bash
# Enhanced MediaStack Start Script with AI Services
# This script starts the enhanced mediastack with AI capabilities

set -e

echo "🚀 Starting AI-Enhanced MediaStack..."

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Check if required environment variables are set
if [ ! -f "config/.env" ]; then
    echo "❌ Environment file not found. Please create config/.env"
    exit 1
fi

# Source environment variables
source config/.env

# Create necessary directories
echo "📁 Creating directory structure..."
mkdir -p ai-models/{quality-predictor,recommendation-engine,storage-optimizer,dashboard-optimizer}
mkdir -p data/{training-data,user-data,user-analytics,health-metrics,notification-history}
mkdir -p ai-dashboards

# Start Phase 1: Core Infrastructure with AI
echo "🏗️  Phase 1: Starting Core Infrastructure..."

# Start VPN
echo "🔒 Starting VPN (Gluetun)..."
docker-compose -f config/docker-compose.yml up -d gluetun

# Wait for VPN to be ready
echo "⏳ Waiting for VPN to be ready..."
sleep 30

# Start databases
echo "🗄️  Starting databases..."
docker-compose -f config/docker-compose.yml up -d postgres valkey

# Start authentication
echo "🔐 Starting Authentik..."
docker-compose -f config/docker-compose.yml up -d authentik

# Start reverse proxy
echo "🌐 Starting Traefik..."
docker-compose -f config/docker-compose.yml up -d traefik

# Phase 2: AI Services
echo "🤖 Phase 2: Starting AI Services..."

# Start AI microservices
echo "🧠 Starting AI Quality Predictor..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-quality-predictor

echo "🎯 Starting AI Recommendation Engine..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-recommendation-engine

echo "💾 Starting AI Storage Optimizer..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-storage-optimizer

echo "📊 Starting AI Dashboard Optimizer..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-dashboard-optimizer

echo "🚨 Starting AI Anomaly Detector..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-anomaly-detector

echo "🏥 Starting AI Health Predictor..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-health-predictor

echo "📢 Starting AI Notification Manager..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-notification-manager

echo "📚 Starting AI Training Data Collector..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ai-training-collector

# Wait for AI services to be ready
echo "⏳ Waiting for AI services to initialize..."
sleep 60

# Phase 3: Enhanced Media Services
echo "🎬 Phase 3: Starting Enhanced Media Services..."

# Start enhanced indexing
echo "🔍 Starting Autobrr (AI-Enhanced)..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d autobrr

# Keep Jackett as backup
echo "🔍 Starting Jackett (Backup)..."
docker-compose -f config/docker-compose.yml up -d jackett

# Start download client
echo "📥 Starting Deluge..."
docker-compose -f config/docker-compose.yml up -d deluge

# Start enhanced *arr services
echo "🎵 Starting Lidarr-on-Steroids..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d lidarr-steroids

echo "🎬 Starting Radarr Extended..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d radarr-extended

echo "📺 Starting Sonarr Extended..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d sonarr-extended

# Phase 4: Request Management and Enhancement Services
echo "🎯 Phase 4: Starting Enhancement Services..."

echo "📋 Starting Ombi (AI-Enhanced)..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d ombi

echo "📊 Starting Traktarr..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d traktarr

echo "🧹 Starting Janitorr..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d janitorr

echo "🕳️  Starting Gaps..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d gaps

# Phase 5: Media Servers
echo "🎬 Phase 5: Starting Media Servers..."

echo "🎵 Starting Jellyfin..."
docker-compose -f config/docker-compose.yml up -d jellyfin

echo "🎬 Starting Plex..."
docker-compose -f config/docker-compose.yml up -d plex

echo "📝 Starting Bazarr..."
docker-compose -f config/docker-compose.yml up -d bazarr

# Phase 6: Dashboard and Monitoring
echo "📊 Phase 6: Starting Dashboard and Monitoring..."

echo "🌐 Starting Organizr (AI-Enhanced)..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d organizr

echo "📈 Starting Exportarr..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d exportarr

echo "🏥 Starting Checkrr..."
docker-compose -f config/docker-compose-ai-enhanced.yml up -d checkrr

echo "🐳 Starting Portainer..."
docker-compose -f config/docker-compose.yml up -d portainer

# Phase 7: Initialize AI Models
echo "🧠 Phase 7: Initializing AI Models..."

# Wait for services to be fully ready
echo "⏳ Waiting for all services to be ready..."
sleep 120

# Test AI services
echo "🧪 Testing AI Services..."

# Test Quality Predictor
echo "Testing Quality Predictor..."
curl -X POST http://localhost:5001/health || echo "Quality Predictor not ready yet"

# Test Recommendation Engine
echo "Testing Recommendation Engine..."
curl -X POST http://localhost:5002/health || echo "Recommendation Engine not ready yet"

# Initialize training data collection
echo "📚 Initializing training data collection..."
curl -X POST http://localhost:5008/start-collection || echo "Training collector not ready yet"

# Display service status
echo "📊 Service Status:"
docker-compose -f config/docker-compose.yml -f config/docker-compose-ai-enhanced.yml ps

echo "✅ AI-Enhanced MediaStack started successfully!"
echo ""
echo "🌐 Access your services:"
echo "  • Dashboard (Organizr): http://localhost:8080"
echo "  • Autobrr: http://localhost:7474"
echo "  • Radarr: http://localhost:7878"
echo "  • Sonarr: http://localhost:8989"
echo "  • Lidarr: http://localhost:8686"
echo "  • Ombi: http://localhost:3579"
echo "  • Janitorr: http://localhost:8998"
echo "  • Gaps: http://localhost:8484"
echo "  • Jellyfin: http://localhost:8096"
echo "  • Plex: http://localhost:32400"
echo ""
echo "🤖 AI Services:"
echo "  • Quality Predictor: http://localhost:5001"
echo "  • Recommendation Engine: http://localhost:5002"
echo "  • Storage Optimizer: http://localhost:5003"
echo "  • Dashboard Optimizer: http://localhost:5004"
echo "  • Anomaly Detector: http://localhost:5005"
echo "  • Health Predictor: http://localhost:5006"
echo "  • Notification Manager: http://localhost:5007"
echo ""
echo "📚 Next Steps:"
echo "1. Configure VPN settings in Gluetun"
echo "2. Set up indexers in Autobrr and Jackett"
echo "3. Configure quality profiles in enhanced *arr services"
echo "4. Set up Ombi for user requests"
echo "5. Configure AI model training schedules"
echo "6. Monitor AI service performance in dashboard"
echo ""
echo "🎯 The AI services will learn from your usage patterns and continuously improve!"

# Create a health check script
cat > check-ai-health.sh << 'EOF'
#!/bin/bash
echo "🏥 AI MediaStack Health Check"
echo "=================================="

services=(
  "ai-quality-predictor:5001"
  "ai-recommendation-engine:5002"
  "ai-storage-optimizer:5003"
  "ai-dashboard-optimizer:5004"
  "ai-anomaly-detector:5005"
  "ai-health-predictor:5006"
  "ai-notification-manager:5007"
)

for service in "${services[@]}"; do
  name=$(echo $service | cut -d':' -f1)
  port=$(echo $service | cut -d':' -f2)
  
  if curl -s -o /dev/null -w "%{http_code}" http://localhost:$port/health | grep -q "200"; then
    echo "✅ $name: Healthy"
  else
    echo "❌ $name: Unhealthy"
  fi
done
EOF

chmod +x check-ai-health.sh

echo "✅ Health check script created: ./check-ai-health.sh"
