// Modern Proxmox Manager Frontend - Natural Language Style UI
// This creates an elegant, intuitive interface using modern React patterns

import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { invoke } from '@tauri-apps/api/core';
import * as Tabs from '@radix-ui/react-tabs';
import AIDashboard from './components/AIDashboard';
import ScriptManager from './components/ScriptManager';
import ConnectionManager from './components/ConnectionManager';
import TerminalComponent from './components/Terminal';
import { 
  Server, 
  Monitor, 
  Container, 
  HardDrive, 
  Network, 
  Plus, 
  Play, 
  Square, 
  Settings,
  Activity,
  Cpu,
  MemoryStick,
  Clock,
  Brain,
  RefreshCw,
  Code,
  Terminal
} from 'lucide-react';

// Define our data types in natural language
interface ProxmoxNode {
  name: string;
  status: string;
  cpu_usage: number;
  memory_usage: number;
  uptime: string;
}

interface VirtualMachine {
  id: number;
  name: string;
  status: string;
  cpu_cores: number;
  memory_gb: number;
  disk_gb: number;
  node: string;
}

interface LXCContainer {
  id: number;
  name: string;
  status: string;
  memory_mb: number;
  disk_gb: number;
  template: string;
}

// Beautiful animated card component
const Card = ({ children, className = "", ...props }: any) => (
  <motion.div
    initial={{ opacity: 0, y: 20 }}
    animate={{ opacity: 1, y: 0 }}
    className={`bg-white rounded-xl shadow-lg border border-gray-200 p-6 hover:shadow-xl transition-all duration-300 ${className}`}
    {...props}
  >
    {children}
  </motion.div>
);

// Status badge that changes color based on state
const StatusBadge = ({ status }: { status: string }) => {
  const colors = {
    running: 'bg-green-100 text-green-800 border-green-200',
    stopped: 'bg-red-100 text-red-800 border-red-200',
    online: 'bg-blue-100 text-blue-800 border-blue-200',
    offline: 'bg-gray-100 text-gray-800 border-gray-200',
  };
  
  return (
    <span className={`px-3 py-1 rounded-full text-sm font-medium border ${colors[status as keyof typeof colors] || colors.offline}`}>
      {status}
    </span>
  );
};

// Main application component
function App() {
  // Application state using natural language variable names
  const [isConnectedToProxmox, setIsConnectedToProxmox] = useState(false);
  const [proxmoxNodes, setProxmoxNodes] = useState<ProxmoxNode[]>([]);
  const [virtualMachines, setVirtualMachines] = useState<VirtualMachine[]>([]);
  const [containers, setContainers] = useState<LXCContainer[]>([]);
  const [selectedTab, setSelectedTab] = useState('overview');
  const [, setIsCreatingNewVM] = useState(false);
  const [loadingMessage, setLoadingMessage] = useState('');
  const [currentConnection, setCurrentConnection] = useState<any>(null);
  const [connectionError, setConnectionError] = useState<string>('');

  // Connect to Proxmox with beautiful loading states
  const connectToProxmox = async (connection: any) => {
    setLoadingMessage('Connecting to your Proxmox cluster...');
    setConnectionError(''); // Clear any previous error
    try {
      console.log('Attempting SSH connection to:', connection.host, 'with username:', connection.username);
      await invoke('connect_to_proxmox', {
        host: connection.host,
        username: connection.username,
        password: connection.password
      });
      console.log('SSH connection successful');
      setCurrentConnection(connection);
      setIsConnectedToProxmox(true);
      await loadAllData();
      setLoadingMessage(''); // Clear loading message on success
    } catch (error) {
      console.error('Failed to connect:', error);
      const errorMessage = error instanceof Error ? error.message : 'Unknown connection error';
      setConnectionError(`Connection failed: ${errorMessage}`);
      setLoadingMessage('');
      setIsConnectedToProxmox(false);
      setCurrentConnection(null);
    }
  };

  // Load all infrastructure data
  const loadAllData = async () => {
    try {
      const [nodes, vms, ctrs] = await Promise.all([
        invoke('get_cluster_overview'),
        invoke('list_virtual_machines'),
        invoke('list_containers')
      ]);
      
      setProxmoxNodes(nodes as ProxmoxNode[]);
      setVirtualMachines(vms as VirtualMachine[]);
      setContainers(ctrs as LXCContainer[]);
    } catch (error) {
      console.error('Failed to load data:', error);
    }
  };

  // Start a virtual machine with smooth animations
  const startVirtualMachine = async (vmId: number) => {
    setLoadingMessage(`Starting virtual machine ${vmId}...`);
    try {
      await invoke('start_virtual_machine', { vmId });
      await loadAllData(); // Refresh the data
    } catch (error) {
      console.error('Failed to start VM:', error);
    }
    setLoadingMessage('');
  };

  // Stop a virtual machine
  const stopVirtualMachine = async (vmId: number) => {
    setLoadingMessage(`Stopping virtual machine ${vmId}...`);
    try {
      await invoke('stop_virtual_machine', { vmId });
      await loadAllData(); // Refresh the data
    } catch (error) {
      console.error('Failed to stop VM:', error);
    }
    setLoadingMessage('');
  };

  // Connection screen with beautiful design
  if (!isConnectedToProxmox) {
    return (
      <ConnectionManager 
        onConnect={connectToProxmox}
        isConnecting={!!loadingMessage}
      />
    );
  }

  // Main dashboard interface
  return (
    <div className="min-h-screen bg-gray-50">
      {/* Top navigation bar */}
      <div className="bg-white border-b border-gray-200 px-6 py-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-3">
            <div className="w-8 h-8 bg-blue-500 rounded-lg flex items-center justify-center">
              <Server className="w-5 h-5 text-white" />
            </div>
            <h1 className="text-xl font-semibold text-gray-900">Proxmox Manager</h1>
          </div>
          <div className="flex items-center space-x-4">
            <div className="flex items-center space-x-2">
              <div className="w-3 h-3 bg-green-500 rounded-full animate-pulse"></div>
              <span className="text-sm text-gray-600">Connected to {currentConnection?.name || 'Proxmox'}</span>
            </div>
            <motion.button
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              onClick={() => setIsConnectedToProxmox(false)}
              className="text-sm text-gray-500 hover:text-gray-700 px-3 py-1 border border-gray-300 rounded-lg hover:bg-gray-50 transition-colors"
            >
              Switch Connection
            </motion.button>
          </div>
        </div>
      </div>

      {/* Main content area */}
      <div className="p-6">
        <Tabs.Root value={selectedTab} onValueChange={setSelectedTab}>
          {/* Tab navigation */}
          <Tabs.List className="flex space-x-1 bg-gray-100 p-1 rounded-lg mb-6">
            {[
              { id: 'overview', label: 'Cluster Overview', icon: Activity },
              { id: 'ai-dashboard', label: 'AI Intelligence', icon: Brain },
              { id: 'script-manager', label: 'Pimox Scripts', icon: Code },
              { id: 'terminal', label: 'Terminal', icon: Terminal },
              { id: 'vms', label: 'Virtual Machines', icon: Monitor },
              { id: 'containers', label: 'Containers', icon: Container },
              { id: 'storage', label: 'Storage', icon: HardDrive },
              { id: 'network', label: 'Network', icon: Network },
            ].map(({ id, label, icon: Icon }) => (
              <Tabs.Trigger
                key={id}
                value={id}
                className="flex items-center space-x-2 px-4 py-2 rounded-md text-sm font-medium transition-all data-[state=active]:bg-white data-[state=active]:shadow-sm"
              >
                <Icon className="w-4 h-4" />
                <span>{label}</span>
              </Tabs.Trigger>
            ))}
          </Tabs.List>

          {/* Cluster Overview Tab */}
          <Tabs.Content value="overview">
            {/* Enhanced overview with real-time metrics */}
            <div className="mb-6">
              <div className="flex items-center justify-between">
                <h2 className="text-2xl font-bold text-gray-900">Infrastructure Overview</h2>
                <div className="flex items-center space-x-4">
                  <motion.button
                    whileHover={{ scale: 1.05 }}
                    whileTap={{ scale: 0.95 }}
                    onClick={loadAllData}
                    className="flex items-center space-x-2 bg-blue-500 text-white px-4 py-2 rounded-lg hover:bg-blue-600 transition-colors"
                  >
                    <RefreshCw className="w-4 h-4" />
                    <span>Refresh</span>
                  </motion.button>
                  <div className="flex items-center space-x-2 text-sm text-gray-600">
                    <Clock className="w-4 h-4" />
                    <span>Last updated: {new Date().toLocaleTimeString()}</span>
                  </div>
                </div>
              </div>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
              {proxmoxNodes.map((node, index) => (
                <Card key={index}>
                  <div className="flex items-center justify-between mb-4">
                    <h3 className="text-lg font-semibold text-gray-900">{node.name}</h3>
                    <StatusBadge status={node.status} />
                  </div>
                  
                  <div className="space-y-3">
                    <div className="flex items-center justify-between">
                      <div className="flex items-center space-x-2">
                        <Cpu className="w-4 h-4 text-gray-500" />
                        <span className="text-sm text-gray-600">CPU Usage</span>
                      </div>
                      <span className="text-sm font-medium">{node.cpu_usage}%</span>
                    </div>
                    
                    <div className="flex items-center justify-between">
                      <div className="flex items-center space-x-2">
                        <MemoryStick className="w-4 h-4 text-gray-500" />
                        <span className="text-sm text-gray-600">Memory Usage</span>
                      </div>
                      <span className="text-sm font-medium">{node.memory_usage}%</span>
                    </div>
                    
                    <div className="flex items-center justify-between">
                      <div className="flex items-center space-x-2">
                        <Activity className="w-4 h-4 text-gray-500" />
                        <span className="text-sm text-gray-600">Uptime</span>
                      </div>
                      <span className="text-sm font-medium">{node.uptime}</span>
                    </div>
                  </div>
                </Card>
              ))}
            </div>
          </Tabs.Content>

          {/* AI Dashboard Tab */}
          <Tabs.Content value="ai-dashboard">
            <AIDashboard />
          </Tabs.Content>

          {/* Script Manager Tab */}
          <Tabs.Content value="script-manager">
            <ScriptManager />
          </Tabs.Content>

          {/* Terminal Tab */}
          <Tabs.Content value="terminal">
            {currentConnection ? (
              <TerminalComponent connection={currentConnection} />
            ) : (
              <Card>
                <div className="text-center py-12">
                  <Terminal className="w-12 h-12 text-gray-400 mx-auto mb-4" />
                  <h3 className="text-lg font-semibold text-gray-900 mb-2">SSH Terminal</h3>
                  <p className="text-gray-600">Please select a connection to access the terminal</p>
                </div>
              </Card>
            )}
          </Tabs.Content>

          {/* Virtual Machines Tab */}
          <Tabs.Content value="vms">
            <div className="mb-6">
              <motion.button
                whileHover={{ scale: 1.02 }}
                whileTap={{ scale: 0.98 }}
                onClick={() => setIsCreatingNewVM(true)}
                className="flex items-center space-x-2 bg-blue-500 text-white px-4 py-2 rounded-lg font-medium hover:bg-blue-600 transition-colors"
              >
                <Plus className="w-4 h-4" />
                <span>Create New VM</span>
              </motion.button>
            </div>

            <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
              {virtualMachines.map((vm) => (
                <Card key={vm.id}>
                  <div className="flex items-center justify-between mb-4">
                    <h3 className="text-lg font-semibold text-gray-900">{vm.name}</h3>
                    <StatusBadge status={vm.status} />
                  </div>
                  
                  <div className="space-y-2 mb-4">
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">ID:</span> {vm.id}
                    </div>
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">Node:</span> {vm.node}
                    </div>
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">Resources:</span> {vm.cpu_cores} CPU, {vm.memory_gb}GB RAM, {vm.disk_gb}GB Disk
                    </div>
                  </div>
                  
                  <div className="flex space-x-2">
                    {vm.status === 'stopped' ? (
                      <motion.button
                        whileHover={{ scale: 1.05 }}
                        whileTap={{ scale: 0.95 }}
                        onClick={() => startVirtualMachine(vm.id)}
                        className="flex items-center space-x-1 bg-green-500 text-white px-3 py-1 rounded text-sm font-medium hover:bg-green-600 transition-colors"
                      >
                        <Play className="w-3 h-3" />
                        <span>Start</span>
                      </motion.button>
                    ) : (
                      <motion.button
                        whileHover={{ scale: 1.05 }}
                        whileTap={{ scale: 0.95 }}
                        onClick={() => stopVirtualMachine(vm.id)}
                        className="flex items-center space-x-1 bg-red-500 text-white px-3 py-1 rounded text-sm font-medium hover:bg-red-600 transition-colors"
                      >
                        <Square className="w-3 h-3" />
                        <span>Stop</span>
                      </motion.button>
                    )}
                    
                    <motion.button
                      whileHover={{ scale: 1.05 }}
                      whileTap={{ scale: 0.95 }}
                      className="flex items-center space-x-1 bg-gray-500 text-white px-3 py-1 rounded text-sm font-medium hover:bg-gray-600 transition-colors"
                    >
                      <Settings className="w-3 h-3" />
                      <span>Configure</span>
                    </motion.button>
                  </div>
                </Card>
              ))}
            </div>
          </Tabs.Content>

          {/* Containers Tab */}
          <Tabs.Content value="containers">
            <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
              {containers.map((container) => (
                <Card key={container.id}>
                  <div className="flex items-center justify-between mb-4">
                    <h3 className="text-lg font-semibold text-gray-900">{container.name}</h3>
                    <StatusBadge status={container.status} />
                  </div>
                  
                  <div className="space-y-2 mb-4">
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">ID:</span> {container.id}
                    </div>
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">Template:</span> {container.template}
                    </div>
                    <div className="text-sm text-gray-600">
                      <span className="font-medium">Resources:</span> {container.memory_mb}MB RAM, {container.disk_gb}GB Disk
                    </div>
                  </div>
                  
                  <div className="flex space-x-2">
                    <motion.button
                      whileHover={{ scale: 1.05 }}
                      whileTap={{ scale: 0.95 }}
                      className="flex items-center space-x-1 bg-blue-500 text-white px-3 py-1 rounded text-sm font-medium hover:bg-blue-600 transition-colors"
                    >
                      <Settings className="w-3 h-3" />
                      <span>Manage</span>
                    </motion.button>
                  </div>
                </Card>
              ))}
            </div>
          </Tabs.Content>

          {/* Storage Tab */}
          <Tabs.Content value="storage">
            <Card>
              <div className="text-center py-12">
                <HardDrive className="w-12 h-12 text-gray-400 mx-auto mb-4" />
                <h3 className="text-lg font-semibold text-gray-900 mb-2">Storage Management</h3>
                <p className="text-gray-600">Storage pools, backups, and disk management coming soon</p>
              </div>
            </Card>
          </Tabs.Content>

          {/* Network Tab */}
          <Tabs.Content value="network">
            <Card>
              <div className="text-center py-12">
                <Network className="w-12 h-12 text-gray-400 mx-auto mb-4" />
                <h3 className="text-lg font-semibold text-gray-900 mb-2">Network Configuration</h3>
                <p className="text-gray-600">Virtual networks, VLANs, and firewall rules coming soon</p>
              </div>
            </Card>
          </Tabs.Content>
        </Tabs.Root>
      </div>

      {/* Loading overlay */}
      <AnimatePresence>
        {loadingMessage && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50"
          >
            <div className="bg-white rounded-lg p-6 flex items-center space-x-3">
              <div className="animate-spin rounded-full h-6 w-6 border-b-2 border-blue-500"></div>
              <span className="text-gray-900">{loadingMessage}</span>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

export default App;
