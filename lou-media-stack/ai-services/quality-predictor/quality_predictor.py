#!/usr/bin/env python3
# AI-Powered Quality Prediction Service
# Advanced torrent quality prediction with machine learning

import os
import re
import json
import time
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple

import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report
from flask import Flask, request, jsonify
import joblib

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class TorrentQualityPredictor:
    def __init__(self):
        self.model_path = '/models/torrent_quality_model.joblib'
        self.scaler_path = '/models/torrent_scaler.joblib' 
        self.vectorizer_path = '/models/torrent_vectorizer.joblib'
        
        self.quality_model = None
        self.fake_model = None
        self.scaler = StandardScaler()
        self.vectorizer = TfidfVectorizer(max_features=1000, stop_words='english')
        
        # Quality thresholds (conservative)
        self.quality_thresholds = {
            'excellent': 0.85,
            'good': 0.70,
            'acceptable': 0.55,
            'poor': 0.40
        }
        
        # Known bad patterns
        self.fake_patterns = [
            r'\b(fake|scam|virus|malware)\b',
            r'\b(password|survey|download\.rar)\b',
            r'\b(click here|visit site)\b',
            r'\.(exe|bat|scr|vbs)$',
            r'\b(codecs?\s+required)\b'
        ]
        
        # Good quality indicators
        self.quality_indicators = {
            'release_groups': ['RARBG', 'YTS', 'ETRG', 'x264', 'BluRay', 'WEB-DL', 'BRRip'],
            'video_quality': ['1080p', '720p', '4K', 'UHD', 'HDR'],
            'audio_quality': ['DTS', 'AC3', 'AAC', 'FLAC', 'Atmos'],
            'source_quality': ['BluRay', 'WEB-DL', 'WEBRip', 'DVDRip']
        }
        
        self.load_or_train_models()
    
    def extract_features(self, torrent_data: Dict) -> Dict:
        """Extract features from torrent data for ML prediction"""
        features = {}
        
        title = torrent_data.get('title', '').lower()
        description = torrent_data.get('description', '').lower()
        size = torrent_data.get('size', 0)
        seeders = torrent_data.get('seeders', 0)
        leechers = torrent_data.get('leechers', 0)
        uploader = torrent_data.get('uploader', '').lower()
        
        # Basic numerical features
        features['size_mb'] = size / (1024 * 1024) if size > 0 else 0
        features['seeders'] = seeders
        features['leechers'] = leechers
        features['seed_leech_ratio'] = seeders / max(leechers, 1)
        
        # Title analysis features
        features['title_length'] = len(title)
        features['has_year'] = 1 if re.search(r'\b(19|20)\d{2}\b', title) else 0
        features['has_quality'] = 1 if any(q in title for q in ['1080p', '720p', '4k', 'uhd']) else 0
        features['has_source'] = 1 if any(s in title for s in ['bluray', 'web-dl', 'webrip']) else 0
        
        # Release group analysis
        known_groups = sum(1 for group in self.quality_indicators['release_groups'] 
                          if group.lower() in title)
        features['known_release_groups'] = known_groups
        
        # Fake indicators
        fake_score = sum(1 for pattern in self.fake_patterns 
                        if re.search(pattern, title + ' ' + description, re.IGNORECASE))
        features['fake_indicators'] = fake_score
        
        # Quality indicators
        video_quality_score = sum(1 for q in self.quality_indicators['video_quality'] 
                                 if q.lower() in title)
        audio_quality_score = sum(1 for q in self.quality_indicators['audio_quality'] 
                                 if q.lower() in title)
        source_quality_score = sum(1 for q in self.quality_indicators['source_quality'] 
                                  if q.lower() in title)
        
        features['video_quality_score'] = video_quality_score
        features['audio_quality_score'] = audio_quality_score
        features['source_quality_score'] = source_quality_score
        
        # Uploader reputation (simplified)
        features['uploader_reputation'] = self.get_uploader_reputation(uploader)
        
        return features
    
    def get_uploader_reputation(self, uploader: str) -> float:
        """Get uploader reputation score"""
        # Known good uploaders (would be learned from data in production)
        good_uploaders = ['rarbg', 'yts', 'ettv', 'eztv', 'tgx']
        if any(good in uploader for good in good_uploaders):
            return 0.8
        return 0.5  # Neutral reputation
    
    def predict_quality(self, torrent_data: Dict) -> Dict:
        """Predict torrent quality using ML model"""
        try:
            features = self.extract_features(torrent_data)
            
            # Convert to DataFrame for consistent feature ordering
            feature_df = pd.DataFrame([features])
            
            # Handle missing columns
            expected_columns = ['size_mb', 'seeders', 'leechers', 'seed_leech_ratio',
                              'title_length', 'has_year', 'has_quality', 'has_source',
                              'known_release_groups', 'fake_indicators', 'video_quality_score',
                              'audio_quality_score', 'source_quality_score', 'uploader_reputation']
            
            for col in expected_columns:
                if col not in feature_df.columns:
                    feature_df[col] = 0
            
            feature_df = feature_df[expected_columns]
            
            # Scale features
            if self.scaler:
                features_scaled = self.scaler.transform(feature_df)
            else:
                features_scaled = feature_df.values
            
            # Predict quality
            if self.quality_model:
                quality_prob = self.quality_model.predict_proba(features_scaled)[0]
                quality_score = np.max(quality_prob)
            else:
                # Fallback scoring
                quality_score = self.calculate_fallback_score(features)
            
            # Predict fake probability
            fake_score = features['fake_indicators'] * 0.3
            is_fake = fake_score > 0.5
            
            # Determine quality category
            if quality_score >= self.quality_thresholds['excellent']:
                category = 'excellent'
            elif quality_score >= self.quality_thresholds['good']:
                category = 'good'
            elif quality_score >= self.quality_thresholds['acceptable']:
                category = 'acceptable'
            else:
                category = 'poor'
            
            return {
                'quality_score': float(quality_score),
                'quality_category': category,
                'is_fake': is_fake,
                'fake_probability': float(fake_score),
                'recommendation': self.get_recommendation(quality_score, is_fake),
                'features': features
            }
            
        except Exception as e:
            logger.error(f"Error predicting quality: {e}")
            return {
                'quality_score': 0.5,
                'quality_category': 'unknown',
                'is_fake': False,
                'fake_probability': 0.0,
                'recommendation': 'manual_review',
                'error': str(e)
            }
    
    def calculate_fallback_score(self, features: Dict) -> float:
        """Calculate quality score using rule-based approach as fallback"""
        score = 0.5  # Base score
        
        # Seeder ratio bonus
        if features['seed_leech_ratio'] > 2:
            score += 0.15
        elif features['seed_leech_ratio'] > 1:
            score += 0.1
        
        # Quality indicators
        score += features['video_quality_score'] * 0.1
        score += features['audio_quality_score'] * 0.05
        score += features['source_quality_score'] * 0.1
        score += features['known_release_groups'] * 0.1
        
        # Penalties
        score -= features['fake_indicators'] * 0.2
        
        return min(max(score, 0.0), 1.0)
    
    def get_recommendation(self, quality_score: float, is_fake: bool) -> str:
        """Get download recommendation"""
        if is_fake:
            return 'reject_fake'
        elif quality_score >= self.quality_thresholds['excellent']:
            return 'download_immediately'
        elif quality_score >= self.quality_thresholds['good']:
            return 'download_recommended'
        elif quality_score >= self.quality_thresholds['acceptable']:
            return 'download_if_no_better_option'
        else:
            return 'reject_poor_quality'
    
    def load_or_train_models(self):
        """Load existing models or train new ones"""
        try:
            if os.path.exists(self.model_path):
                self.quality_model = joblib.load(self.model_path)
                logger.info("Loaded existing quality model")
            
            if os.path.exists(self.scaler_path):
                self.scaler = joblib.load(self.scaler_path)
                logger.info("Loaded existing scaler")
            
            if os.path.exists(self.vectorizer_path):
                self.vectorizer = joblib.load(self.vectorizer_path)
                logger.info("Loaded existing vectorizer")
                
        except Exception as e:
            logger.error(f"Error loading models: {e}")
            self.train_initial_model()
    
    def train_initial_model(self):
        """Train initial model with synthetic data"""
        logger.info("Training initial model with synthetic data...")
        
        # Generate synthetic training data
        synthetic_data = self.generate_synthetic_training_data()
        
        if len(synthetic_data) > 0:
            df = pd.DataFrame(synthetic_data)
            
            # Prepare features and target
            feature_columns = ['size_mb', 'seeders', 'leechers', 'seed_leech_ratio',
                             'title_length', 'has_year', 'has_quality', 'has_source',
                             'known_release_groups', 'fake_indicators', 'video_quality_score',
                             'audio_quality_score', 'source_quality_score', 'uploader_reputation']
            
            X = df[feature_columns]
            y = df['quality_label']
            
            # Split data
            X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
            
            # Scale features
            X_train_scaled = self.scaler.fit_transform(X_train)
            X_test_scaled = self.scaler.transform(X_test)
            
            # Train model
            self.quality_model = RandomForestClassifier(n_estimators=100, random_state=42)
            self.quality_model.fit(X_train_scaled, y_train)
            
            # Evaluate
            y_pred = self.quality_model.predict(X_test_scaled)
            accuracy = accuracy_score(y_test, y_pred)
            logger.info(f"Model accuracy: {accuracy:.3f}")
            
            # Save models
            os.makedirs('/models', exist_ok=True)
            joblib.dump(self.quality_model, self.model_path)
            joblib.dump(self.scaler, self.scaler_path)
            
            logger.info("Model training completed and saved")
    
    def generate_synthetic_training_data(self) -> List[Dict]:
        """Generate synthetic training data for initial model"""
        data = []
        
        # High quality examples
        for _ in range(100):
            data.append({
                'size_mb': np.random.uniform(1000, 15000),
                'seeders': np.random.randint(50, 500),
                'leechers': np.random.randint(5, 50),
                'seed_leech_ratio': np.random.uniform(2, 20),
                'title_length': np.random.randint(30, 80),
                'has_year': 1,
                'has_quality': 1,
                'has_source': 1,
                'known_release_groups': np.random.randint(1, 3),
                'fake_indicators': 0,
                'video_quality_score': np.random.randint(1, 2),
                'audio_quality_score': np.random.randint(1, 2),
                'source_quality_score': np.random.randint(1, 2),
                'uploader_reputation': np.random.uniform(0.7, 1.0),
                'quality_label': 'excellent'
            })
        
        # Low quality examples
        for _ in range(100):
            data.append({
                'size_mb': np.random.uniform(100, 1000),
                'seeders': np.random.randint(1, 20),
                'leechers': np.random.randint(0, 10),
                'seed_leech_ratio': np.random.uniform(0.1, 1.5),
                'title_length': np.random.randint(10, 40),
                'has_year': np.random.choice([0, 1]),
                'has_quality': 0,
                'has_source': 0,
                'known_release_groups': 0,
                'fake_indicators': np.random.randint(0, 3),
                'video_quality_score': 0,
                'audio_quality_score': 0,
                'source_quality_score': 0,
                'uploader_reputation': np.random.uniform(0.1, 0.5),
                'quality_label': 'poor'
            })
        
        return data
    
    def retrain_model(self, feedback_data: List[Dict]):
        """Retrain model with user feedback"""
        if len(feedback_data) < 10:
            logger.info("Not enough feedback data for retraining")
            return
        
        logger.info(f"Retraining model with {len(feedback_data)} feedback samples")
        
        try:
            # Process feedback data
            processed_data = []
            for item in feedback_data:
                features = self.extract_features(item['torrent_data'])
                features['quality_label'] = item['user_rating']
                processed_data.append(features)
            
            # Combine with existing synthetic data
            synthetic_data = self.generate_synthetic_training_data()
            all_data = synthetic_data + processed_data
            
            df = pd.DataFrame(all_data)
            
            # Prepare features and target
            feature_columns = ['size_mb', 'seeders', 'leechers', 'seed_leech_ratio',
                             'title_length', 'has_year', 'has_quality', 'has_source',
                             'known_release_groups', 'fake_indicators', 'video_quality_score',
                             'audio_quality_score', 'source_quality_score', 'uploader_reputation']
            
            X = df[feature_columns]
            y = df['quality_label']
            
            # Retrain
            X_scaled = self.scaler.fit_transform(X)
            self.quality_model.fit(X_scaled, y)
            
            # Save updated models
            joblib.dump(self.quality_model, self.model_path)
            joblib.dump(self.scaler, self.scaler_path)
            
            logger.info("Model retraining completed")
            
        except Exception as e:
            logger.error(f"Error during model retraining: {e}")

# Flask API
app = Flask(__name__)
predictor = TorrentQualityPredictor()

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'service': 'AI Torrent Quality Predictor',
        'version': '1.0.0',
        'model_loaded': predictor.quality_model is not None,
        'timestamp': datetime.utcnow().isoformat()
    })

@app.route('/predict', methods=['POST'])
def predict_quality():
    """Predict torrent quality"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No data provided'}), 400
        
        torrent_data = data.get('torrent', {})
        if not torrent_data:
            return jsonify({'error': 'No torrent data provided'}), 400
        
        prediction = predictor.predict_quality(torrent_data)
        
        return jsonify({
            'success': True,
            'prediction': prediction,
            'timestamp': datetime.utcnow().isoformat()
        })
        
    except Exception as e:
        logger.error(f"Error in prediction endpoint: {e}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/feedback', methods=['POST'])
def submit_feedback():
    """Submit feedback for model improvement"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No feedback data provided'}), 400
        
        # Store feedback for later retraining
        # In production, this would be stored in a database
        feedback_file = '/data/feedback.json'
        
        if os.path.exists(feedback_file):
            with open(feedback_file, 'r') as f:
                feedback_data = json.load(f)
        else:
            feedback_data = []
        
        feedback_data.append({
            'torrent_data': data.get('torrent', {}),
            'user_rating': data.get('rating'),
            'timestamp': datetime.utcnow().isoformat()
        })
        
        os.makedirs('/data', exist_ok=True)
        with open(feedback_file, 'w') as f:
            json.dump(feedback_data, f, indent=2)
        
        # Retrain if we have enough feedback
        if len(feedback_data) % 50 == 0:  # Retrain every 50 feedback items
            predictor.retrain_model(feedback_data)
        
        return jsonify({
            'success': True,
            'message': 'Feedback received',
            'feedback_count': len(feedback_data)
        })
        
    except Exception as e:
        logger.error(f"Error in feedback endpoint: {e}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/stats', methods=['GET'])
def get_stats():
    """Get prediction statistics"""
    try:
        feedback_file = '/data/feedback.json'
        stats = {
            'model_loaded': predictor.quality_model is not None,
            'feedback_count': 0,
            'quality_thresholds': predictor.quality_thresholds,
            'service_uptime': time.time()
        }
        
        if os.path.exists(feedback_file):
            with open(feedback_file, 'r') as f:
                feedback_data = json.load(f)
                stats['feedback_count'] = len(feedback_data)
        
        return jsonify(stats)
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8777, debug=False)
