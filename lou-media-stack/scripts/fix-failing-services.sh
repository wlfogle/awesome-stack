#!/bin/bash

echo "🔧 Fixing MediaStack failing services..."

# Get user/group IDs
PUID=${PUID:-1000}
PGID=${PGID:-1000}

# Fix DDNS Updater permissions
echo "📝 Fixing DDNS Updater permissions..."
mkdir -p ./config/ddns-updater/data
chown -R $PUID:$PGID ./config/ddns-updater/
chmod 755 ./config/ddns-updater/data

# Fix EPGStation logs
echo "📺 Fixing EPGStation logs..."
mkdir -p ./config/epgstation/logs
touch ./config/epgstation/logs/system.log
chown -R $PUID:$PGID ./config/epgstation/

# Fix AI services requirements
echo "🤖 Fixing AI services..."
mkdir -p ./config/ai-recommendation-engine ./config/ai-storage-optimizer

cat > ./config/ai-recommendation-engine/requirements.txt << EOF
flask==2.3.0
pandas==2.0.0
numpy==1.24.0
scikit-learn==1.3.0
requests==2.31.0
EOF

cat > ./config/ai-storage-optimizer/requirements.txt << EOF
flask==2.3.0
pandas==2.0.0
numpy==1.24.0
scikit-learn==1.3.0
psutil==5.9.0
EOF

chown -R $PUID:$PGID ./config/ai-recommendation-engine/
chown -R $PUID:$PGID ./config/ai-storage-optimizer/

# Fix FileBot permissions
echo "📁 Fixing FileBot permissions..."
mkdir -p ./config/filebot
chown -R $PUID:$PGID ./config/filebot/

# Fix Janitorr permissions
echo "🧹 Fixing Janitorr permissions..."
mkdir -p ./config/janitorr
chown -R $PUID:$PGID ./config/janitorr/

# Fix HeadPlane dependencies
echo "✈️  Fixing HeadPlane..."
mkdir -p ./config/headplane
chown -R $PUID:$PGID ./config/headplane/

echo "🔄 Restarting fixed services..."
docker restart mediastack_ddns_updater
docker restart mediastack_epgstation  
docker restart mediastack_ai_recommendation_engine
docker restart mediastack_ai_storage_optimizer
docker restart mediastack-filebot
docker restart mediastack_janitorr
docker restart mediastack_headplane

echo "✅ Fixes applied! Check service status in 30 seconds..."
sleep 5
echo "🔍 Current status:"
docker ps --filter "status=restarting" --format "table {{.Names}}\t{{.Status}}"

echo ""
echo "🎉 MediaStack service fixes complete!"
echo "💡 For HeadScale, you may need to update the docker-compose command to 'headscale serve'"
