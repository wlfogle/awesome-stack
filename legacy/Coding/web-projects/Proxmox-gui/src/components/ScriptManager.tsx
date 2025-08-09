// Pimox Scripts Integration - Community Script Manager
// This component allows browsing and installing scripts from pimox-scripts.com

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { invoke } from '@tauri-apps/api/core';
import { 
  Download, 
  Code, 
  Play, 
  Search, 
  Filter, 
  Star, 
  ExternalLink,
  CheckCircle,
  AlertTriangle,
  Info,
  Terminal,
  Package,
  Settings,
  Shield,
  Database,
  Network,
  Monitor,
  HardDrive,
  Zap,
  Cloud,
  RefreshCw,
  Eye,
  BookOpen
} from 'lucide-react';

interface PimoxScript {
  id: string;
  name: string;
  description: string;
  category: string;
  tags: string[];
  install_command: string;
  difficulty: 'beginner' | 'intermediate' | 'advanced';
  estimated_time: string;
  requirements: string[];
  author: string;
  last_updated: string;
  popularity: number;
  verified: boolean;
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

const CategoryIcon = ({ category }: { category: string }) => {
  const icons = {
    'containers': Package,
    'networking': Network,
    'storage': HardDrive,
    'monitoring': Monitor,
    'security': Shield,
    'database': Database,
    'automation': Zap,
    'cloud': Cloud,
    'system': Settings,
    'default': Code
  };
  
  const Icon = icons[category.toLowerCase() as keyof typeof icons] || icons.default;
  return <Icon className="w-5 h-5" />;
};

const DifficultyBadge = ({ level }: { level: string }) => {
  const colors = {
    beginner: 'bg-green-100 text-green-800 border-green-200',
    intermediate: 'bg-yellow-100 text-yellow-800 border-yellow-200',
    advanced: 'bg-red-100 text-red-800 border-red-200'
  };
  
  return (
    <span className={`px-2 py-1 rounded-full text-xs font-medium border ${colors[level as keyof typeof colors]}`}>
      {level}
    </span>
  );
};

export const ScriptManager = () => {
  const [scripts, setScripts] = useState<PimoxScript[]>([]);
  const [filteredScripts, setFilteredScripts] = useState<PimoxScript[]>([]);
  const [searchTerm, setSearchTerm] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('all');
  const [selectedScript, setSelectedScript] = useState<PimoxScript | null>(null);
  const [isInstalling, setIsInstalling] = useState<string | null>(null);
  const [installedScripts, setInstalledScripts] = useState<Set<string>>(new Set());
  const [loading, setLoading] = useState(true);

  // Sample Pimox Scripts data (in real app, this would come from the API)
  const sampleScripts: PimoxScript[] = [
    {
      id: 'homeassistant-lxc',
      name: 'Home Assistant LXC',
      description: 'Install Home Assistant in a Linux Container with automatic updates and SSL support',
      category: 'automation',
      tags: ['home-automation', 'iot', 'smart-home'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/homeassistant.sh)"',
      difficulty: 'beginner',
      estimated_time: '5-10 minutes',
      requirements: ['Proxmox VE 6.2+', '2GB RAM', '8GB Storage'],
      author: 'tteck',
      last_updated: '2024-12-15',
      popularity: 9.5,
      verified: true
    },
    {
      id: 'docker-lxc',
      name: 'Docker LXC Container',
      description: 'Create a privileged Docker container with all necessary configurations',
      category: 'containers',
      tags: ['docker', 'containerization', 'development'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/docker.sh)"',
      difficulty: 'intermediate',
      estimated_time: '3-5 minutes',
      requirements: ['Proxmox VE 7.0+', '1GB RAM', '4GB Storage'],
      author: 'tteck',
      last_updated: '2024-12-10',
      popularity: 9.8,
      verified: true
    },
    {
      id: 'nextcloud-lxc',
      name: 'Nextcloud LXC',
      description: 'Deploy Nextcloud file sharing and collaboration platform',
      category: 'cloud',
      tags: ['file-sharing', 'collaboration', 'self-hosted'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/nextcloud.sh)"',
      difficulty: 'intermediate',
      estimated_time: '10-15 minutes',
      requirements: ['Proxmox VE 6.2+', '4GB RAM', '20GB Storage'],
      author: 'tteck',
      last_updated: '2024-12-08',
      popularity: 9.2,
      verified: true
    },
    {
      id: 'pi-hole-lxc',
      name: 'Pi-hole LXC',
      description: 'Network-wide ad blocker and DNS sinkhole',
      category: 'networking',
      tags: ['dns', 'ad-blocking', 'network-security'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/pihole.sh)"',
      difficulty: 'beginner',
      estimated_time: '5 minutes',
      requirements: ['Proxmox VE 6.2+', '512MB RAM', '2GB Storage'],
      author: 'tteck',
      last_updated: '2024-12-12',
      popularity: 9.7,
      verified: true
    },
    {
      id: 'plex-lxc',
      name: 'Plex Media Server LXC',
      description: 'Stream your personal media collection anywhere',
      category: 'media',
      tags: ['media-server', 'streaming', 'entertainment'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/plex.sh)"',
      difficulty: 'intermediate',
      estimated_time: '8-12 minutes',
      requirements: ['Proxmox VE 7.0+', '2GB RAM', '10GB Storage'],
      author: 'tteck',
      last_updated: '2024-12-05',
      popularity: 8.9,
      verified: true
    },
    {
      id: 'mongodb-lxc',
      name: 'MongoDB LXC',
      description: 'NoSQL database for modern applications',
      category: 'database',
      tags: ['database', 'nosql', 'development'],
      install_command: 'bash -c "$(wget -qLO - https://github.com/tteck/Proxmox/raw/main/ct/mongodb.sh)"',
      difficulty: 'advanced',
      estimated_time: '10-15 minutes',
      requirements: ['Proxmox VE 7.0+', '4GB RAM', '20GB Storage'],
      author: 'tteck',
      last_updated: '2024-11-28',
      popularity: 8.1,
      verified: true
    }
  ];

  const categories = [
    'all',
    'automation',
    'containers',
    'cloud',
    'networking',
    'database',
    'media',
    'security',
    'monitoring'
  ];

  useEffect(() => {
    // Simulate loading scripts from Pimox API
    const loadScripts = async () => {
      setLoading(true);
      await new Promise(resolve => setTimeout(resolve, 1000)); // Simulate API call
      setScripts(sampleScripts);
      setFilteredScripts(sampleScripts);
      setLoading(false);
    };

    loadScripts();
  }, []);

  useEffect(() => {
    let filtered = scripts;

    // Filter by category
    if (selectedCategory !== 'all') {
      filtered = filtered.filter(script => script.category === selectedCategory);
    }

    // Filter by search term
    if (searchTerm) {
      filtered = filtered.filter(script =>
        script.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
        script.description.toLowerCase().includes(searchTerm.toLowerCase()) ||
        script.tags.some(tag => tag.toLowerCase().includes(searchTerm.toLowerCase()))
      );
    }

    setFilteredScripts(filtered);
  }, [scripts, selectedCategory, searchTerm]);

  const installScript = async (script: PimoxScript) => {
    setIsInstalling(script.id);
    try {
      await invoke('execute_pimox_script', {
        scriptId: script.id,
        command: script.install_command
      });
      
      setInstalledScripts(prev => new Set(prev).add(script.id));
      console.log(`Successfully installed ${script.name}`);
    } catch (error) {
      console.error(`Failed to install ${script.name}:`, error);
    } finally {
      setIsInstalling(null);
    }
  };

  const refreshScripts = async () => {
    setLoading(true);
    try {
      // In real app, this would fetch from pimox-scripts.com API
      await new Promise(resolve => setTimeout(resolve, 1000));
      console.log('Scripts refreshed from pimox-scripts.com');
    } catch (error) {
      console.error('Failed to refresh scripts:', error);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <div className="p-2 bg-gradient-to-r from-green-500 to-blue-500 rounded-lg">
            <Code className="w-6 h-6 text-white" />
          </div>
          <div>
            <h2 className="text-xl font-bold text-gray-900">Pimox Scripts</h2>
            <p className="text-sm text-gray-600">Community helper scripts for Proxmox VE</p>
          </div>
        </div>
        
        <div className="flex items-center space-x-4">
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={refreshScripts}
            disabled={loading}
            className="flex items-center space-x-2 bg-blue-500 text-white px-4 py-2 rounded-lg hover:bg-blue-600 transition-colors disabled:opacity-50"
          >
            <RefreshCw className={`w-4 h-4 ${loading ? 'animate-spin' : ''}`} />
            <span>Refresh</span>
          </motion.button>
          
          <motion.a
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            href="https://pimox-scripts.com/scripts"
            target="_blank"
            rel="noopener noreferrer"
            className="flex items-center space-x-2 bg-gray-500 text-white px-4 py-2 rounded-lg hover:bg-gray-600 transition-colors"
          >
            <ExternalLink className="w-4 h-4" />
            <span>Visit Website</span>
          </motion.a>
        </div>
      </div>

      {/* Search and Filters */}
      <div className="flex flex-col sm:flex-row gap-4">
        <div className="flex-1 relative">
          <Search className="w-5 h-5 absolute left-3 top-1/2 transform -translate-y-1/2 text-gray-400" />
          <input
            type="text"
            placeholder="Search scripts..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full pl-10 pr-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          />
        </div>
        
        <div className="flex items-center space-x-2">
          <Filter className="w-5 h-5 text-gray-500" />
          <select
            value={selectedCategory}
            onChange={(e) => setSelectedCategory(e.target.value)}
            className="px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          >
            {categories.map(category => (
              <option key={category} value={category}>
                {category === 'all' ? 'All Categories' : category.charAt(0).toUpperCase() + category.slice(1)}
              </option>
            ))}
          </select>
        </div>
      </div>

      {/* Scripts Grid */}
      {loading ? (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {[...Array(6)].map((_, i) => (
            <Card key={i} className="animate-pulse">
              <div className="h-4 bg-gray-200 rounded w-3/4 mb-3"></div>
              <div className="h-3 bg-gray-200 rounded w-full mb-2"></div>
              <div className="h-3 bg-gray-200 rounded w-2/3 mb-4"></div>
              <div className="h-8 bg-gray-200 rounded w-1/3"></div>
            </Card>
          ))}
        </div>
      ) : (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {filteredScripts.map((script) => (
            <Card key={script.id} className="cursor-pointer" onClick={() => setSelectedScript(script)}>
              <div className="flex items-start justify-between mb-3">
                <div className="flex items-center space-x-2">
                  <CategoryIcon category={script.category} />
                  <h3 className="font-semibold text-gray-900 text-lg">{script.name}</h3>
                </div>
                {script.verified && (
                  <CheckCircle className="w-5 h-5 text-green-500" />
                )}
              </div>
              
              <p className="text-gray-600 text-sm mb-4 line-clamp-2">{script.description}</p>
              
              <div className="flex items-center justify-between mb-4">
                <DifficultyBadge level={script.difficulty} />
                <div className="flex items-center space-x-1">
                  <Star className="w-4 h-4 text-yellow-500 fill-current" />
                  <span className="text-sm text-gray-600">{script.popularity}</span>
                </div>
              </div>
              
              <div className="flex flex-wrap gap-1 mb-4">
                {script.tags.slice(0, 3).map((tag) => (
                  <span
                    key={tag}
                    className="px-2 py-1 bg-gray-100 text-gray-600 text-xs rounded-full"
                  >
                    {tag}
                  </span>
                ))}
              </div>
              
              <div className="flex items-center justify-between">
                <span className="text-xs text-gray-500">
                  Est. {script.estimated_time}
                </span>
                
                {installedScripts.has(script.id) ? (
                  <div className="flex items-center space-x-1 text-green-600">
                    <CheckCircle className="w-4 h-4" />
                    <span className="text-sm font-medium">Installed</span>
                  </div>
                ) : (
                  <motion.button
                    whileHover={{ scale: 1.05 }}
                    whileTap={{ scale: 0.95 }}
                    onClick={(e) => {
                      e.stopPropagation();
                      installScript(script);
                    }}
                    disabled={isInstalling === script.id}
                    className="flex items-center space-x-1 bg-green-500 text-white px-3 py-1 rounded-md text-sm font-medium hover:bg-green-600 transition-colors disabled:opacity-50"
                  >
                    {isInstalling === script.id ? (
                      <>
                        <div className="animate-spin rounded-full h-3 w-3 border-b-2 border-white"></div>
                        <span>Installing...</span>
                      </>
                    ) : (
                      <>
                        <Download className="w-3 h-3" />
                        <span>Install</span>
                      </>
                    )}
                  </motion.button>
                )}
              </div>
            </Card>
          ))}
        </div>
      )}

      {/* Script Detail Modal */}
      <AnimatePresence>
        {selectedScript && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50 p-4"
            onClick={() => setSelectedScript(null)}
          >
            <motion.div
              initial={{ scale: 0.9, opacity: 0 }}
              animate={{ scale: 1, opacity: 1 }}
              exit={{ scale: 0.9, opacity: 0 }}
              className="bg-white rounded-xl max-w-2xl w-full max-h-[80vh] overflow-y-auto"
              onClick={(e) => e.stopPropagation()}
            >
              <div className="p-6">
                <div className="flex items-start justify-between mb-4">
                  <div className="flex items-center space-x-3">
                    <CategoryIcon category={selectedScript.category} />
                    <div>
                      <h3 className="text-xl font-bold text-gray-900">{selectedScript.name}</h3>
                      <p className="text-sm text-gray-500">by {selectedScript.author}</p>
                    </div>
                  </div>
                  <button
                    onClick={() => setSelectedScript(null)}
                    className="text-gray-400 hover:text-gray-600"
                  >
                    ×
                  </button>
                </div>
                
                <p className="text-gray-700 mb-6">{selectedScript.description}</p>
                
                <div className="grid grid-cols-2 gap-4 mb-6">
                  <div>
                    <h4 className="font-semibold text-gray-900 mb-2">Details</h4>
                    <div className="space-y-2 text-sm">
                      <div className="flex justify-between">
                        <span className="text-gray-600">Difficulty:</span>
                        <DifficultyBadge level={selectedScript.difficulty} />
                      </div>
                      <div className="flex justify-between">
                        <span className="text-gray-600">Est. Time:</span>
                        <span>{selectedScript.estimated_time}</span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-gray-600">Rating:</span>
                        <div className="flex items-center space-x-1">
                          <Star className="w-4 h-4 text-yellow-500 fill-current" />
                          <span>{selectedScript.popularity}</span>
                        </div>
                      </div>
                    </div>
                  </div>
                  
                  <div>
                    <h4 className="font-semibold text-gray-900 mb-2">Requirements</h4>
                    <ul className="space-y-1 text-sm text-gray-600">
                      {selectedScript.requirements.map((req, index) => (
                        <li key={index} className="flex items-center space-x-2">
                          <CheckCircle className="w-3 h-3 text-green-500" />
                          <span>{req}</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                </div>
                
                <div className="mb-6">
                  <h4 className="font-semibold text-gray-900 mb-2">Tags</h4>
                  <div className="flex flex-wrap gap-2">
                    {selectedScript.tags.map((tag) => (
                      <span
                        key={tag}
                        className="px-3 py-1 bg-blue-100 text-blue-800 text-sm rounded-full"
                      >
                        {tag}
                      </span>
                    ))}
                  </div>
                </div>
                
                <div className="mb-6">
                  <h4 className="font-semibold text-gray-900 mb-2">Install Command</h4>
                  <div className="bg-gray-900 text-green-400 p-4 rounded-lg font-mono text-sm overflow-x-auto">
                    {selectedScript.install_command}
                  </div>
                </div>
                
                <div className="flex justify-end space-x-3">
                  <button
                    onClick={() => setSelectedScript(null)}
                    className="px-4 py-2 text-gray-600 border border-gray-300 rounded-lg hover:bg-gray-50"
                  >
                    Close
                  </button>
                  
                  {!installedScripts.has(selectedScript.id) ? (
                    <motion.button
                      whileHover={{ scale: 1.05 }}
                      whileTap={{ scale: 0.95 }}
                      onClick={() => installScript(selectedScript)}
                      disabled={isInstalling === selectedScript.id}
                      className="flex items-center space-x-2 bg-green-500 text-white px-4 py-2 rounded-lg hover:bg-green-600 transition-colors disabled:opacity-50"
                    >
                      {isInstalling === selectedScript.id ? (
                        <>
                          <div className="animate-spin rounded-full h-4 w-4 border-b-2 border-white"></div>
                          <span>Installing...</span>
                        </>
                      ) : (
                        <>
                          <Download className="w-4 h-4" />
                          <span>Install Script</span>
                        </>
                      )}
                    </motion.button>
                  ) : (
                    <div className="flex items-center space-x-2 text-green-600 px-4 py-2">
                      <CheckCircle className="w-4 h-4" />
                      <span className="font-medium">Already Installed</span>
                    </div>
                  )}
                </div>
              </div>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};

export default ScriptManager;
