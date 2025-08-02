#!/usr/bin/env python3
"""
MediaStack Real-time Monitoring API
Provides REST endpoints for checking Docker container status and logs
"""

import docker
import json
import subprocess
from datetime import datetime
from flask import Flask, jsonify, request
from flask_cors import CORS

app = Flask(__name__)
CORS(app)  # Enable CORS for web frontend

# Initialize Docker client
try:
    client = docker.from_env()
except Exception as e:
    print(f"Warning: Could not connect to Docker: {e}")
    client = None

def get_container_status():
    """Get status of all MediaStack containers"""
    containers = []
    
    if not client:
        return {"error": "Docker client not available"}
    
    try:
        # Get all containers (running and stopped)
        all_containers = client.containers.list(all=True)
        
        for container in all_containers:
            # Filter for mediastack containers
            if 'mediastack' in container.name.lower():
                status_info = {
                    'name': container.name,
                    'status': container.status,
                    'state': container.attrs['State'],
                    'image': container.image.tags[0] if container.image.tags else 'unknown',
                    'created': container.attrs['Created'],
                    'ports': container.attrs.get('NetworkSettings', {}).get('Ports', {}),
                    'health': container.attrs['State'].get('Health', {}).get('Status', 'none'),
                    'restart_count': container.attrs['RestartCount'],
                    'last_updated': datetime.now().isoformat()
                }
                
                # Add port mappings in readable format
                port_mappings = []
                if status_info['ports']:
                    for internal_port, external in status_info['ports'].items():
                        if external:
                            for mapping in external:
                                port_mappings.append(f"{mapping.get('HostPort', 'N/A')}:{internal_port}")
                        else:
                            port_mappings.append(internal_port)
                
                status_info['port_mappings'] = port_mappings
                containers.append(status_info)
        
        return {
            'timestamp': datetime.now().isoformat(),
            'total_containers': len(containers),
            'containers': sorted(containers, key=lambda x: x['name'])
        }
        
    except Exception as e:
        return {"error": f"Failed to get container status: {str(e)}"}

def get_container_logs(container_name, lines=50):
    """Get recent logs for a specific container"""
    if not client:
        return {"error": "Docker client not available"}
    
    try:
        container = client.containers.get(container_name)
        logs = container.logs(tail=lines, timestamps=True).decode('utf-8', errors='ignore')
        
        return {
            'container': container_name,
            'logs': logs,
            'lines_requested': lines,
            'timestamp': datetime.now().isoformat()
        }
        
    except docker.errors.NotFound:
        return {"error": f"Container '{container_name}' not found"}
    except Exception as e:
        return {"error": f"Failed to get logs: {str(e)}"}

def restart_container(container_name):
    """Restart a specific container"""
    if not client:
        return {"error": "Docker client not available"}
    
    try:
        container = client.containers.get(container_name)
        container.restart()
        
        return {
            'container': container_name,
            'action': 'restarted',
            'timestamp': datetime.now().isoformat(),
            'success': True
        }
        
    except docker.errors.NotFound:
        return {"error": f"Container '{container_name}' not found"}
    except Exception as e:
        return {"error": f"Failed to restart container: {str(e)}"}

@app.route('/api/status')
def api_status():
    """Get all container statuses"""
    return jsonify(get_container_status())

@app.route('/api/status/<container_name>')
def api_container_status(container_name):
    """Get status for a specific container"""
    all_status = get_container_status()
    
    if 'error' in all_status:
        return jsonify(all_status), 500
    
    # Find the specific container
    for container in all_status['containers']:
        if container['name'] == container_name or container['name'].endswith(container_name):
            return jsonify({
                'timestamp': all_status['timestamp'],
                'container': container
            })
    
    return jsonify({"error": f"Container '{container_name}' not found"}), 404

@app.route('/api/logs/<container_name>')
def api_logs(container_name):
    """Get logs for a specific container"""
    lines = request.args.get('lines', 50, type=int)
    return jsonify(get_container_logs(container_name, lines))

@app.route('/api/restart/<container_name>', methods=['POST'])
def api_restart(container_name):
    """Restart a specific container"""
    result = restart_container(container_name)
    if 'error' in result:
        return jsonify(result), 500
    return jsonify(result)

@app.route('/api/health')
def api_health():
    """API health check"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'docker_available': client is not None
    })

@app.route('/api/summary')
def api_summary():
    """Get a summary of service statuses"""
    all_status = get_container_status()
    
    if 'error' in all_status:
        return jsonify(all_status), 500
    
    summary = {
        'timestamp': all_status['timestamp'],
        'total': len(all_status['containers']),
        'running': 0,
        'stopped': 0,
        'restarting': 0,
        'unhealthy': 0,
        'created': 0,
        'problem_services': []
    }
    
    for container in all_status['containers']:
        status = container['status'].lower()
        health = container['health']
        
        if 'running' in status:
            if health == 'unhealthy':
                summary['unhealthy'] += 1
                summary['problem_services'].append({
                    'name': container['name'],
                    'issue': 'unhealthy',
                    'status': container['status']
                })
            else:
                summary['running'] += 1
        elif 'restarting' in status:
            summary['restarting'] += 1
            summary['problem_services'].append({
                'name': container['name'],
                'issue': 'restarting',
                'status': container['status']
            })
        elif 'exited' in status:
            summary['stopped'] += 1
            summary['problem_services'].append({
                'name': container['name'],
                'issue': 'stopped',
                'status': container['status']
            })
        elif 'created' in status:
            summary['created'] += 1
            summary['problem_services'].append({
                'name': container['name'],
                'issue': 'not_started',
                'status': container['status']
            })
    
    return jsonify(summary)

if __name__ == '__main__':
    print("Starting MediaStack Monitoring API...")
    print("Available endpoints:")
    print("  GET  /api/health - API health check")
    print("  GET  /api/status - All container statuses")
    print("  GET  /api/summary - Status summary")
    print("  GET  /api/status/<name> - Specific container status")
    print("  GET  /api/logs/<name> - Container logs")
    print("  POST /api/restart/<name> - Restart container")
    print("\nStarting server on http://localhost:5001")
    
    app.run(host='0.0.0.0', port=5001, debug=False)
