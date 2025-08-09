// AI-Powered Proxmox Dashboard - The Future of Infrastructure Management
// Features: Predictive Analytics, Auto-scaling, Anomaly Detection, Smart Recommendations

import { useState, useEffect, useMemo } from 'react';
import { motion } from 'framer-motion';
import { 
  Brain, 
  TrendingUp, 
  AlertTriangle, 
  CheckCircle,
  Zap,
  Target,
  Sparkles,
  Eye,
  Bell,
  Gauge,
  Activity,
  Shield
} from 'lucide-react';

interface AIMetrics {
  performance_score: number;
  efficiency_rating: number;
  predicted_issues: string[];
  optimization_suggestions: string[];
  auto_scaling_recommendations: {
    vm_id: number;
    action: 'scale_up' | 'scale_down' | 'maintain';
    confidence: number;
    reason: string;
  }[];
}

interface AIHealthCheck {
  overall_health: 'excellent' | 'good' | 'warning' | 'critical';
  anomalies_detected: number;
  predictive_alerts: {
    type: 'resource_exhaustion' | 'performance_degradation' | 'security_risk';
    severity: 'low' | 'medium' | 'high' | 'critical';
    eta_hours: number;
    description: string;
  }[];
}

const AICard = ({ children, className = '', ...props }: any) => (
  <motion.div
    initial={{ opacity: 0, scale: 0.95 }}
    animate={{ opacity: 1, scale: 1 }}
    className={`bg-gradient-to-br from-white to-blue-50 rounded-xl shadow-lg border border-blue-200 p-6 hover:shadow-xl transition-all duration-300 ${className}`}
    {...props}
  >
    {children}
  </motion.div>
);

const PredictiveChart = ({ data, title }: { data: number[], title: string }) => {
  const maxValue = Math.max(...data);
  
  return (
    <div className="space-y-3">
      <h4 className="text-sm font-medium text-gray-700">{title}</h4>
      <div className="flex items-end space-x-1 h-16">
        {data.map((value, index) => (
          <motion.div
            key={index}
            initial={{ height: 0 }}
            animate={{ height: `${(value / maxValue) * 100}%` }}
            transition={{ delay: index * 0.1 }}
            className={`flex-1 rounded-t ${
              index < data.length - 3 ? 'bg-blue-400' : 'bg-purple-400'
            }`}
          />
        ))}
      </div>
      <div className="flex justify-between text-xs text-gray-500">
        <span>Past</span>
        <span className="text-purple-600 font-medium">Predicted</span>
      </div>
    </div>
  );
};

const AnomalyAlert = ({ alert }: { alert: any }) => {
  const severityColors = {
    low: 'bg-yellow-100 border-yellow-300 text-yellow-800',
    medium: 'bg-orange-100 border-orange-300 text-orange-800',
    high: 'bg-red-100 border-red-300 text-red-800',
    critical: 'bg-red-200 border-red-500 text-red-900'
  };

  return (
    <motion.div
      initial={{ opacity: 0, x: -20 }}
      animate={{ opacity: 1, x: 0 }}
      className={`p-3 rounded-lg border ${severityColors[alert.severity as keyof typeof severityColors]}`}
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <AlertTriangle className="w-4 h-4" />
          <span className="text-sm font-medium">{alert.type.replace('_', ' ').toUpperCase()}</span>
        </div>
        <span className="text-xs">{alert.eta_hours}h</span>
      </div>
      <p className="text-sm mt-1">{alert.description}</p>
    </motion.div>
  );
};

export const AIDashboard = () => {
  const [aiMetrics, setAiMetrics] = useState<AIMetrics>({
    performance_score: 87.5,
    efficiency_rating: 92.1,
    predicted_issues: [
      'Memory usage spike expected in 4 hours',
      'Storage cleanup recommended for node-01'
    ],
    optimization_suggestions: [
      'Migrate VM-102 to less loaded node',
      'Enable auto-scaling for web-server cluster',
      'Schedule maintenance during low-traffic hours'
    ],
    auto_scaling_recommendations: [
      {
        vm_id: 100,
        action: 'scale_up',
        confidence: 0.89,
        reason: 'CPU utilization trending upward, traffic increase detected'
      },
      {
        vm_id: 102,
        action: 'scale_down',
        confidence: 0.76,
        reason: 'Consistent low resource usage, cost optimization opportunity'
      }
    ]
  });

  const [healthCheck] = useState<AIHealthCheck>({
    overall_health: 'good',
    anomalies_detected: 2,
    predictive_alerts: [
      {
        type: 'resource_exhaustion',
        severity: 'medium',
        eta_hours: 6,
        description: 'Memory usage on node-02 approaching 85% threshold'
      },
      {
        type: 'performance_degradation',
        severity: 'low',
        eta_hours: 12,
        description: 'Disk I/O latency increasing on storage pool ZFS-01'
      }
    ]
  });

  const [isAIEnabled, setIsAIEnabled] = useState(true);
  const [autoOptimize, setAutoOptimize] = useState(false);

  // Simulate real-time AI updates
  useEffect(() => {
    if (!isAIEnabled) return;

    const interval = setInterval(() => {
      setAiMetrics(prev => ({
        ...prev,
        performance_score: prev.performance_score + (Math.random() - 0.5) * 2,
        efficiency_rating: prev.efficiency_rating + (Math.random() - 0.5) * 1.5
      }));
    }, 5000);

    return () => clearInterval(interval);
  }, [isAIEnabled]);

  // Generate sample prediction data
  const cpuPrediction = useMemo(() => [
    23, 28, 35, 42, 38, 45, 52, 48, 55, 61  // Last 3 are predictions
  ], []);

  const memoryPrediction = useMemo(() => [
    67, 71, 68, 75, 78, 82, 79, 85, 88, 92  // Last 3 are predictions
  ], []);

  const healthColor = {
    excellent: 'text-green-600',
    good: 'text-blue-600', 
    warning: 'text-yellow-600',
    critical: 'text-red-600'
  };

  return (
    <div className="space-y-6">
      {/* AI Control Panel */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <div className="p-2 bg-gradient-to-r from-purple-500 to-blue-500 rounded-lg">
            <Brain className="w-6 h-6 text-white" />
          </div>
          <div>
            <h2 className="text-xl font-bold text-gray-900">AI-Powered Management</h2>
            <p className="text-sm text-gray-600">Intelligent monitoring and predictive optimization</p>
          </div>
        </div>
        
        <div className="flex items-center space-x-4">
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={() => setAutoOptimize(!autoOptimize)}
            className={`px-4 py-2 rounded-lg font-medium transition-colors ${
              autoOptimize 
                ? 'bg-green-500 text-white' 
                : 'bg-gray-200 text-gray-700 hover:bg-gray-300'
            }`}
          >
            <div className="flex items-center space-x-2">
              <Zap className="w-4 h-4" />
              <span>Auto-Optimize</span>
            </div>
          </motion.button>
          
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={() => setIsAIEnabled(!isAIEnabled)}
            className={`px-4 py-2 rounded-lg font-medium transition-colors ${
              isAIEnabled 
                ? 'bg-purple-500 text-white' 
                : 'bg-gray-200 text-gray-700 hover:bg-gray-300'
            }`}
          >
            <div className="flex items-center space-x-2">
              <Eye className="w-4 h-4" />
              <span>AI Monitoring</span>
            </div>
          </motion.button>
        </div>
      </div>

      {/* AI Metrics Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        {/* Performance Score */}
        <AICard>
          <div className="flex items-center justify-between mb-3">
            <Gauge className="w-5 h-5 text-blue-500" />
            <span className="text-2xl font-bold text-gray-900">
              {aiMetrics.performance_score.toFixed(1)}%
            </span>
          </div>
          <h3 className="font-semibold text-gray-700 mb-2">Performance Score</h3>
          <div className="w-full bg-gray-200 rounded-full h-2">
            <motion.div
              initial={{ width: 0 }}
              animate={{ width: `${aiMetrics.performance_score}%` }}
              className="bg-gradient-to-r from-blue-400 to-purple-500 h-2 rounded-full"
            />
          </div>
        </AICard>

        {/* Efficiency Rating */}
        <AICard>
          <div className="flex items-center justify-between mb-3">
            <Target className="w-5 h-5 text-green-500" />
            <span className="text-2xl font-bold text-gray-900">
              {aiMetrics.efficiency_rating.toFixed(1)}%
            </span>
          </div>
          <h3 className="font-semibold text-gray-700 mb-2">Efficiency Rating</h3>
          <div className="w-full bg-gray-200 rounded-full h-2">
            <motion.div
              initial={{ width: 0 }}
              animate={{ width: `${aiMetrics.efficiency_rating}%` }}
              className="bg-gradient-to-r from-green-400 to-blue-500 h-2 rounded-full"
            />
          </div>
        </AICard>

        {/* Overall Health */}
        <AICard>
          <div className="flex items-center justify-between mb-3">
            <Shield className={`w-5 h-5 ${healthColor[healthCheck.overall_health]}`} />
            <CheckCircle className={`w-6 h-6 ${healthColor[healthCheck.overall_health]}`} />
          </div>
          <h3 className="font-semibold text-gray-700 mb-2">System Health</h3>
          <p className={`font-bold capitalize ${healthColor[healthCheck.overall_health]}`}>
            {healthCheck.overall_health}
          </p>
        </AICard>

        {/* Anomalies Detected */}
        <AICard>
          <div className="flex items-center justify-between mb-3">
            <AlertTriangle className="w-5 h-5 text-orange-500" />
            <span className="text-2xl font-bold text-gray-900">
              {healthCheck.anomalies_detected}
            </span>
          </div>
          <h3 className="font-semibold text-gray-700 mb-2">Anomalies</h3>
          <p className="text-sm text-gray-600">Detected this hour</p>
        </AICard>
      </div>

      {/* Predictive Analytics */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Resource Predictions */}
        <AICard>
          <div className="flex items-center space-x-2 mb-4">
            <TrendingUp className="w-5 h-5 text-purple-500" />
            <h3 className="text-lg font-semibold text-gray-900">Resource Predictions</h3>
          </div>
          
          <div className="grid grid-cols-2 gap-4">
            <PredictiveChart data={cpuPrediction} title="CPU Usage Trend" />
            <PredictiveChart data={memoryPrediction} title="Memory Usage Trend" />
          </div>
          
          <div className="mt-4 p-3 bg-purple-50 rounded-lg">
            <p className="text-sm text-purple-700">
              <Sparkles className="w-4 h-4 inline mr-1" />
              AI predicts 15% resource increase in next 6 hours
            </p>
          </div>
        </AICard>

        {/* Smart Recommendations */}
        <AICard>
          <div className="flex items-center space-x-2 mb-4">
            <Brain className="w-5 h-5 text-blue-500" />
            <h3 className="text-lg font-semibold text-gray-900">Smart Recommendations</h3>
          </div>
          
          <div className="space-y-3">
            {aiMetrics.optimization_suggestions.map((suggestion, index) => (
              <motion.div
                key={index}
                initial={{ opacity: 0, x: -20 }}
                animate={{ opacity: 1, x: 0 }}
                transition={{ delay: index * 0.1 }}
                className="flex items-start space-x-3 p-3 bg-blue-50 rounded-lg"
              >
                <CheckCircle className="w-4 h-4 text-blue-500 mt-0.5" />
                <span className="text-sm text-gray-700">{suggestion}</span>
              </motion.div>
            ))}
          </div>
          
          {autoOptimize && (
            <motion.button
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              whileHover={{ scale: 1.02 }}
              whileTap={{ scale: 0.98 }}
              className="w-full mt-4 bg-gradient-to-r from-blue-500 to-purple-600 text-white py-2 rounded-lg font-medium"
            >
              Apply All Recommendations
            </motion.button>
          )}
        </AICard>
      </div>

      {/* Predictive Alerts */}
      {healthCheck.predictive_alerts.length > 0 && (
        <AICard>
          <div className="flex items-center space-x-2 mb-4">
            <Bell className="w-5 h-5 text-orange-500" />
            <h3 className="text-lg font-semibold text-gray-900">Predictive Alerts</h3>
            <span className="px-2 py-1 bg-orange-100 text-orange-800 text-xs rounded-full">
              {healthCheck.predictive_alerts.length}
            </span>
          </div>
          
          <div className="space-y-3">
            {healthCheck.predictive_alerts.map((alert, index) => (
              <AnomalyAlert key={index} alert={alert} />
            ))}
          </div>
        </AICard>
      )}

      {/* Auto-scaling Recommendations */}
      <AICard>
        <div className="flex items-center space-x-2 mb-4">
          <Activity className="w-5 h-5 text-green-500" />
          <h3 className="text-lg font-semibold text-gray-900">Auto-scaling Intelligence</h3>
        </div>
        
        <div className="space-y-3">
          {aiMetrics.auto_scaling_recommendations.map((rec, index) => (
            <motion.div
              key={index}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: index * 0.1 }}
              className="flex items-center justify-between p-3 border border-gray-200 rounded-lg"
            >
              <div className="flex-1">
                <div className="flex items-center space-x-2">
                  <span className="font-medium text-gray-900">VM-{rec.vm_id}</span>
                  <span className={`px-2 py-1 text-xs rounded-full ${
                    rec.action === 'scale_up' ? 'bg-green-100 text-green-800' :
                    rec.action === 'scale_down' ? 'bg-blue-100 text-blue-800' :
                    'bg-gray-100 text-gray-800'
                  }`}>
                    {rec.action.replace('_', ' ')}
                  </span>
                  <span className="text-sm text-gray-500">
                    {(rec.confidence * 100).toFixed(0)}% confidence
                  </span>
                </div>
                <p className="text-sm text-gray-600 mt-1">{rec.reason}</p>
              </div>
              
              {autoOptimize && (
                <motion.button
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  className="ml-4 px-3 py-1 bg-blue-500 text-white text-sm rounded-md hover:bg-blue-600"
                >
                  Apply
                </motion.button>
              )}
            </motion.div>
          ))}
        </div>
      </AICard>
    </div>
  );
};

export default AIDashboard;
