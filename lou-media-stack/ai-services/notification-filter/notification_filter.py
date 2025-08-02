# notification_filter.py

import time
from datetime import datetime, timedelta

class SmartNotificationFilter:
    def __init__(self):
        self.user_preferences = {}
        self.notification_history = []
        self.spam_patterns = [
            'test notification',
            'connection timeout',
            'temporary error'
        ]
    
    def should_send_notification(self, notification):
        """Determine if notification should be sent based on AI filtering"""
        # Check for spam patterns
        if self._is_spam(notification):
            return False
        
        # Check frequency limits
        if self._is_too_frequent(notification):
            return False
        
        # Check severity
        if self._is_low_priority(notification):
            return False
        
        return True
    
    def _is_spam(self, notification):
        """Check if notification matches spam patterns"""
        message = notification.get('message', '').lower()
        return any(pattern in message for pattern in self.spam_patterns)
    
    def _is_too_frequent(self, notification):
        """Check if similar notifications were sent recently"""
        current_time = time.time()
        recent_notifications = [
            n for n in self.notification_history 
            if n.get('timestamp', 0) > current_time - 300  # Last 5 minutes
        ]
        
        # Don't send more than 3 similar notifications in 5 minutes
        similar_count = sum(1 for n in recent_notifications 
                          if n.get('type') == notification.get('type'))
        return similar_count >= 3
    
    def _is_low_priority(self, notification):
        """Check if notification is low priority"""
        low_priority_types = ['info', 'debug', 'routine_update']
        return notification.get('priority', 'medium') in low_priority_types
    
    def enhance_notification(self, notification):
        """Add AI-generated context or suggestions"""
        enhanced = notification.copy()
        
        # Add context based on notification type
        if notification.get('type') == 'download_failed':
            enhanced['suggestion'] = 'Check indexer health and retry'
        elif notification.get('type') == 'service_down':
            enhanced['suggestion'] = 'Service may need restart or maintenance'
        elif notification.get('type') == 'disk_space_low':
            enhanced['suggestion'] = 'Run cleanup scripts or add storage'
        
        # Add estimated resolution time
        enhanced['estimated_resolution'] = self._estimate_resolution_time(notification)
        
        return enhanced
    
    def _estimate_resolution_time(self, notification):
        """Estimate how long issue might take to resolve"""
        type_to_time = {
            'download_failed': '5-10 minutes',
            'service_down': '1-5 minutes',
            'disk_space_low': '10-30 minutes',
            'connection_error': '2-5 minutes'
        }
        return type_to_time.get(notification.get('type'), 'Unknown')

# API endpoint for notification filtering
from flask import Flask, jsonify, request

app = Flask(__name__)
notification_filter = SmartNotificationFilter()

@app.route('/filter-notification', methods=['POST'])
def filter_notification():
    data = request.get_json() or {}
    notification = data.get('notification', {})
    
    should_send = notification_filter.should_send_notification(notification)
    enhanced = notification_filter.enhance_notification(notification) if should_send else None
    
    return jsonify(
        should_send=should_send,
        enhanced_notification=enhanced,
        reason='Filtered by AI' if not should_send else 'Approved by AI'
    )

@app.route('/health', methods=['GET'])
def health_check():
    return jsonify(status='healthy')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8007)
