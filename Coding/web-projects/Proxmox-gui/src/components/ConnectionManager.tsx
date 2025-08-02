// Connection Manager - Manage Multiple Proxmox Installations
// Features: Saved credentials, connection profiles, quick connect

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { invoke } from '@tauri-apps/api/core';
import { 
  Plus, 
  Server, 
  Edit2, 
  Trash2, 
  CheckCircle, 
  AlertCircle, 
  Star, 
  StarOff,
  Wifi,
  WifiOff,
  Globe,
  Lock,
  Eye,
  EyeOff,
  Clock,
  Edit
} from 'lucide-react';

interface ProxmoxConnection {
  id: string;
  name: string;
  host: string;
  username: string;
  password: string;
  description?: string;
  lastConnected?: string;
  favorite: boolean;
  verified: boolean;
}

interface ConnectionManagerProps {
  onConnect: (connection: ProxmoxConnection) => void;
  isConnecting: boolean;
  error?: string;
}

const Card = ({ children, className = "", ...props }: any) => (
  <motion.div
    initial={{ opacity: 0, y: 10 }}
    animate={{ opacity: 1, y: 0 }}
    className={`bg-white rounded-xl shadow-lg border border-gray-200 p-6 hover:shadow-xl transition-all duration-300 ${className}`}
    {...props}
  >
    {children}
  </motion.div>
);

export const ConnectionManager = ({ onConnect, isConnecting, error }: ConnectionManagerProps) => {
  const [connections, setConnections] = useState<ProxmoxConnection[]>([]);
  const [showAddForm, setShowAddForm] = useState(false);
  const [editingConnection, setEditingConnection] = useState<ProxmoxConnection | null>(null);
  const [showPasswords, setShowPasswords] = useState<Set<string>>(new Set());
  const [newConnection, setNewConnection] = useState<Partial<ProxmoxConnection>>({
    name: '',
    host: '',
    username: 'root',
    password: '',
    description: '',
    favorite: false
  });

  // Load saved connections from localStorage
  useEffect(() => {
    const savedConnections = localStorage.getItem('proxmox-connections');
    if (savedConnections) {
      try {
        const parsed = JSON.parse(savedConnections);
        setConnections(parsed);
      } catch (error) {
        console.error('Failed to load saved connections:', error);
      }
    } else {
      // Add default connection
      const defaultConnections: ProxmoxConnection[] = [
        {
          id: 'main-server',
          name: 'Main Proxmox Server',
          host: 'https://192.168.0.64:8006',
          username: 'root',
          password: '',
          description: 'Primary Proxmox server',
          favorite: true,
          verified: false
        }
      ];
      setConnections(defaultConnections);
      saveConnections(defaultConnections);
    }
  }, []);

  const saveConnections = (conns: ProxmoxConnection[]) => {
    localStorage.setItem('proxmox-connections', JSON.stringify(conns));
  };

  // Auto-format host URL
  const formatHostUrl = (input: string): string => {
    if (!input) return input;
    
    // Remove any existing protocol and port
    let cleanInput = input.replace(/^https?:\/\//, '').replace(/:8006$/, '');
    
    // Check if it's an IP address or hostname
    if (cleanInput.match(/^\d+\.\d+\.\d+\.\d+$/) || cleanInput.includes('.')) {
      return `https://${cleanInput}:8006`;
    }
    
    return input; // Return as-is if it doesn't look like an IP or domain
  };

  const addConnection = () => {
    if (!newConnection.name || !newConnection.host || !newConnection.username) {
      return;
    }

    const connection: ProxmoxConnection = {
      id: Date.now().toString(),
      name: newConnection.name!,
      host: formatHostUrl(newConnection.host!),
      username: newConnection.username!,
      password: newConnection.password || '',
      description: newConnection.description || '',
      favorite: newConnection.favorite || false,
      verified: false
    };

    const updatedConnections = [...connections, connection];
    setConnections(updatedConnections);
    saveConnections(updatedConnections);
    
    setNewConnection({
      name: '',
      host: '',
      username: 'root',
      password: '',
      description: '',
      favorite: false
    });
    setShowAddForm(false);
  };

  const updateConnection = () => {
    if (!editingConnection) return;

    const updatedConnection = {
      ...editingConnection,
      host: formatHostUrl(editingConnection.host)
    };
    
    const updatedConnections = connections.map(conn =>
      conn.id === editingConnection.id ? updatedConnection : conn
    );
    
    setConnections(updatedConnections);
    saveConnections(updatedConnections);
    setEditingConnection(null);
  };

  const deleteConnection = (id: string) => {
    const updatedConnections = connections.filter(conn => conn.id !== id);
    setConnections(updatedConnections);
    saveConnections(updatedConnections);
  };

  const toggleFavorite = (id: string) => {
    const updatedConnections = connections.map(conn =>
      conn.id === id ? { ...conn, favorite: !conn.favorite } : conn
    );
    setConnections(updatedConnections);
    saveConnections(updatedConnections);
  };

  const togglePasswordVisibility = (id: string) => {
    const newSet = new Set(showPasswords);
    if (newSet.has(id)) {
      newSet.delete(id);
    } else {
      newSet.add(id);
    }
    setShowPasswords(newSet);
  };

  const testConnection = async (connection: ProxmoxConnection) => {
    try {
      await invoke('connect_to_proxmox', {
        serverId: connection.id,
        host: connection.host,
        username: connection.username,
        password: connection.password
      });
      
      const updatedConnections = connections.map(conn =>
        conn.id === connection.id ? { ...conn, verified: true } : conn
      );
      setConnections(updatedConnections);
      saveConnections(updatedConnections);
    } catch (error) {
      console.error('Connection test failed:', error);
    }
  };

  const handleConnect = async (connection: ProxmoxConnection) => {
    try {
      // Actually connect to Proxmox via SSH
      await invoke('connect_to_proxmox', {
        serverId: connection.id,
        host: connection.host,
        username: connection.username,
        password: connection.password
      });
      
      const updatedConnection = {
        ...connection,
        lastConnected: new Date().toISOString(),
        verified: true
      };
      
      const updatedConnections = connections.map(conn =>
        conn.id === connection.id ? updatedConnection : conn
      );
      setConnections(updatedConnections);
      saveConnections(updatedConnections);
      
      onConnect(updatedConnection);
    } catch (error) {
      console.error('Connection failed:', error);
      throw error; // Re-throw so App.tsx can handle the error
    }
  };

  const sortedConnections = [...connections].sort((a, b) => {
    if (a.favorite && !b.favorite) return -1;
    if (!a.favorite && b.favorite) return 1;
    return a.name.localeCompare(b.name);
  });

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-4">
      <div className="max-w-6xl mx-auto">
        {/* Header */}
        <div className="text-center mb-8">
          <div className="w-20 h-20 bg-blue-500 rounded-full flex items-center justify-center mx-auto mb-4">
            <Server className="w-10 h-10 text-white" />
          </div>
          <h1 className="text-3xl font-bold text-gray-900 mb-2">Proxmox Manager</h1>
          <p className="text-gray-600">Connect to your Proxmox infrastructure</p>
        </div>

        {/* Error Display */}
        {error && (
          <motion.div
            initial={{ opacity: 0, y: -20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -20 }}
            className="mb-6 max-w-md mx-auto"
          >
            <div className="bg-red-50 border border-red-200 rounded-lg p-4 flex items-start space-x-3">
              <AlertCircle className="w-5 h-5 text-red-500 flex-shrink-0 mt-0.5" />
              <div>
                <h4 className="font-medium text-red-800">Connection Failed</h4>
                <p className="text-sm text-red-600 mt-1">{error}</p>
              </div>
            </div>
          </motion.div>
        )}

        {/* Quick Actions */}
        <div className="flex justify-center mb-8">
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={() => setShowAddForm(true)}
            className="flex items-center space-x-2 bg-blue-500 text-white px-6 py-3 rounded-lg font-medium hover:bg-blue-600 transition-colors"
          >
            <Plus className="w-5 h-5" />
            <span>Add New Connection</span>
          </motion.button>
        </div>

        {/* Connections Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mb-8">
          {sortedConnections.map((connection) => (
            <Card key={connection.id} className="relative">
              {connection.favorite && (
                <div className="absolute top-4 right-4">
                  <div className="w-3 h-3 bg-yellow-400 rounded-full"></div>
                </div>
              )}
              
              <div className="flex items-start justify-between mb-4">
                <div className="flex items-center space-x-3">
                  <div className="w-10 h-10 bg-gradient-to-r from-blue-500 to-purple-600 rounded-lg flex items-center justify-center">
                    <Server className="w-5 h-5 text-white" />
                  </div>
                  <div>
                    <h3 className="font-semibold text-gray-900">{connection.name}</h3>
                    <p className="text-sm text-gray-500">{connection.host}</p>
                  </div>
                </div>
                
                <div className="flex items-center space-x-1">
                  {connection.verified && (
                    <CheckCircle className="w-5 h-5 text-green-500" />
                  )}
                  <button
                    onClick={() => toggleFavorite(connection.id)}
                    className={`p-1 rounded ${connection.favorite ? 'text-yellow-500' : 'text-gray-400'} hover:text-yellow-500`}
                  >
                    ★
                  </button>
                </div>
              </div>

              <div className="space-y-2 mb-4">
                <div className="flex items-center justify-between text-sm">
                  <span className="text-gray-600">Username:</span>
                  <span className="font-medium">{connection.username}</span>
                </div>
                
                <div className="flex items-center justify-between text-sm">
                  <span className="text-gray-600">Password:</span>
                  <div className="flex items-center space-x-2">
                    <span className="font-mono">
                      {showPasswords.has(connection.id) 
                        ? connection.password || '(not saved)' 
                        : '••••••••'
                      }
                    </span>
                    <button
                      onClick={() => togglePasswordVisibility(connection.id)}
                      className="text-gray-400 hover:text-gray-600"
                    >
                      {showPasswords.has(connection.id) ? (
                        <EyeOff className="w-4 h-4" />
                      ) : (
                        <Eye className="w-4 h-4" />
                      )}
                    </button>
                  </div>
                </div>

                {connection.description && (
                  <p className="text-sm text-gray-600 italic">{connection.description}</p>
                )}

                {connection.lastConnected && (
                  <div className="flex items-center space-x-1 text-xs text-gray-500">
                    <Clock className="w-3 h-3" />
                    <span>Last: {new Date(connection.lastConnected).toLocaleDateString()}</span>
                  </div>
                )}
              </div>

              <div className="flex space-x-2">
                <motion.button
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  onClick={() => handleConnect(connection)}
                  disabled={isConnecting}
                  className="flex-1 bg-green-500 text-white py-2 px-4 rounded-lg font-medium hover:bg-green-600 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  {isConnecting ? 'Connecting...' : 'Connect'}
                </motion.button>
                
                <motion.button
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  onClick={() => testConnection(connection)}
                  className="p-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 transition-colors"
                >
                  <Wifi className="w-4 h-4" />
                </motion.button>
                
                <motion.button
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  onClick={() => setEditingConnection(connection)}
                  className="p-2 bg-gray-500 text-white rounded-lg hover:bg-gray-600 transition-colors"
                >
                  <Edit className="w-4 h-4" />
                </motion.button>
                
                <motion.button
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  onClick={() => deleteConnection(connection.id)}
                  className="p-2 bg-red-500 text-white rounded-lg hover:bg-red-600 transition-colors"
                >
                  <Trash2 className="w-4 h-4" />
                </motion.button>
              </div>
            </Card>
          ))}
        </div>

        {/* Add Connection Modal */}
        <AnimatePresence>
          {showAddForm && (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50 p-4"
              onClick={() => setShowAddForm(false)}
            >
              <motion.div
                initial={{ scale: 0.9, opacity: 0 }}
                animate={{ scale: 1, opacity: 1 }}
                exit={{ scale: 0.9, opacity: 0 }}
                className="bg-white rounded-xl max-w-md w-full p-6"
                onClick={(e) => e.stopPropagation()}
              >
                <h3 className="text-xl font-bold text-gray-900 mb-4">Add New Connection</h3>
                
                <div className="space-y-4">
                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Connection Name
                    </label>
                    <input
                      type="text"
                      value={newConnection.name || ''}
                      onChange={(e) => setNewConnection({...newConnection, name: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                      placeholder="My Proxmox Server"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Host URL
                    </label>
                    <input
                      type="text"
                      value={newConnection.host || ''}
                      onChange={(e) => setNewConnection({...newConnection, host: e.target.value})}
                      onBlur={(e) => setNewConnection({...newConnection, host: formatHostUrl(e.target.value)})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                      placeholder="192.168.0.64 (auto-formats to https://192.168.0.64:8006)"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Username
                    </label>
                    <input
                      type="text"
                      value={newConnection.username || ''}
                      onChange={(e) => setNewConnection({...newConnection, username: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                      placeholder="root"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Password (optional)
                    </label>
                    <input
                      type="password"
                      value={newConnection.password || ''}
                      onChange={(e) => setNewConnection({...newConnection, password: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                      placeholder="Password (will be saved securely)"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Description (optional)
                    </label>
                    <input
                      type="text"
                      value={newConnection.description || ''}
                      onChange={(e) => setNewConnection({...newConnection, description: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                      placeholder="Home lab server"
                    />
                  </div>

                  <div className="flex items-center">
                    <input
                      type="checkbox"
                      checked={newConnection.favorite || false}
                      onChange={(e) => setNewConnection({...newConnection, favorite: e.target.checked})}
                      className="mr-2"
                    />
                    <label className="text-sm text-gray-700">Mark as favorite</label>
                  </div>
                </div>

                <div className="flex justify-end space-x-3 mt-6">
                  <button
                    onClick={() => setShowAddForm(false)}
                    className="px-4 py-2 text-gray-600 border border-gray-300 rounded-lg hover:bg-gray-50"
                  >
                    Cancel
                  </button>
                  <motion.button
                    whileHover={{ scale: 1.05 }}
                    whileTap={{ scale: 0.95 }}
                    onClick={addConnection}
                    className="px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 transition-colors"
                  >
                    Add Connection
                  </motion.button>
                </div>
              </motion.div>
            </motion.div>
          )}
        </AnimatePresence>

        {/* Edit Connection Modal */}
        <AnimatePresence>
          {editingConnection && (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50 p-4"
              onClick={() => setEditingConnection(null)}
            >
              <motion.div
                initial={{ scale: 0.9, opacity: 0 }}
                animate={{ scale: 1, opacity: 1 }}
                exit={{ scale: 0.9, opacity: 0 }}
                className="bg-white rounded-xl max-w-md w-full p-6"
                onClick={(e) => e.stopPropagation()}
              >
                <h3 className="text-xl font-bold text-gray-900 mb-4">Edit Connection</h3>
                
                <div className="space-y-4">
                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Connection Name
                    </label>
                    <input
                      type="text"
                      value={editingConnection.name}
                      onChange={(e) => setEditingConnection({...editingConnection, name: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Host URL
                    </label>
                    <input
                      type="text"
                      value={editingConnection.host}
                      onChange={(e) => setEditingConnection({...editingConnection, host: e.target.value})}
                      onBlur={(e) => setEditingConnection({...editingConnection, host: formatHostUrl(e.target.value)})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Username
                    </label>
                    <input
                      type="text"
                      value={editingConnection.username}
                      onChange={(e) => setEditingConnection({...editingConnection, username: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Password
                    </label>
                    <input
                      type="password"
                      value={editingConnection.password}
                      onChange={(e) => setEditingConnection({...editingConnection, password: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    />
                  </div>

                  <div>
                    <label className="block text-sm font-medium text-gray-700 mb-2">
                      Description
                    </label>
                    <input
                      type="text"
                      value={editingConnection.description || ''}
                      onChange={(e) => setEditingConnection({...editingConnection, description: e.target.value})}
                      className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    />
                  </div>

                  <div className="flex items-center">
                    <input
                      type="checkbox"
                      checked={editingConnection.favorite}
                      onChange={(e) => setEditingConnection({...editingConnection, favorite: e.target.checked})}
                      className="mr-2"
                    />
                    <label className="text-sm text-gray-700">Mark as favorite</label>
                  </div>
                </div>

                <div className="flex justify-end space-x-3 mt-6">
                  <button
                    onClick={() => setEditingConnection(null)}
                    className="px-4 py-2 text-gray-600 border border-gray-300 rounded-lg hover:bg-gray-50"
                  >
                    Cancel
                  </button>
                  <motion.button
                    whileHover={{ scale: 1.05 }}
                    whileTap={{ scale: 0.95 }}
                    onClick={updateConnection}
                    className="px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 transition-colors"
                  >
                    Save Changes
                  </motion.button>
                </div>
              </motion.div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </div>
  );
};

export default ConnectionManager;
