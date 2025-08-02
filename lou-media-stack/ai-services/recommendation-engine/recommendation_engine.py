#!/usr/bin/env python3
# AI-Powered Content Recommendation Service
# Offers user-centric content recommendations based on machine learning

import os
import logging
from datetime import datetime
from typing import List, Dict

import numpy as np
import pandas as pd
from flask import Flask, request, jsonify
from sklearn.neighbors import NearestNeighbors
from sklearn.metrics.pairwise import cosine_similarity
import psycopg2
import redis
import joblib

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class ContentRecommendationEngine:
    def __init__(self):
        self.model_path = '/models/recommendation_model.joblib'       # Placeholder for serialized model
        self.user_feature_path = '/data/user_features.npy'            # Placeholder for user features
        self.content_feature_path = '/data/content_features.npy'      # Placeholder for content features
        
        self.user_features = np.array([])
        self.content_features = np.array([])
        
        # Redis and Postgres configuration
        self.redis_host = os.getenv('REDIS_HOST', 'localhost')
        self.redis_port = int(os.getenv('REDIS_PORT', 6379))
        self.postgres_config = {
            'dbname': os.getenv('POSTGRES_DB', 'mydb'),
            'user': os.getenv('POSTGRES_USER', 'myuser'),
            'password': os.getenv('POSTGRES_PASSWORD', 'mypassword'),
            'host': os.getenv('POSTGRES_HOST', 'localhost'),
            'port': os.getenv('POSTGRES_PORT', 5432)
        }
        
        self.load_models()
        self.redis = redis.StrictRedis(host=self.redis_host, port=self.redis_port)
        self.db_connection = psycopg2.connect(**self.postgres_config)

    def load_models(self) -> None:
        """Load user and content features for recommendations"""
        try:
            if os.path.isfile(self.user_feature_path):
                self.user_features = np.load(self.user_feature_path)
                logger.info("Loaded user features")
            
            if os.path.isfile(self.content_feature_path):
                self.content_features = np.load(self.content_feature_path)
                logger.info("Loaded content features")
            
        except Exception as e:
            logger.error(f"Error loading models: {e}")

    def recommend_for_user(self, user_id: int, num_recommendations: int = 5) -> List[Dict]:
        """Recommend content for a given user based on similarity"""
        try:
            # Basic checks
            if self.user_features.size == 0 or self.content_features.size == 0:
                raise ValueError("User or content features are not loaded")
            
            if user_id >= len(self.user_features):
                return [{'error': 'User not found'}]
            
            user_vector = self.user_features[user_id]
            
            # Compute similarity scores
            scores = cosine_similarity([user_vector], self.content_features)[0]
            
            # Get top content recommendations
            recommended_indices = np.argsort(scores)[-num_recommendations:][::-1]
            recommended_content = [{'content_id': i, 'score': scores[i]} for i in recommended_indices]
            
            logger.info(f"Generated {num_recommendations} recommendations for user {user_id}")
            return recommended_content
        except Exception as e:
            logger.error(f"Error in recommendation: {e}")
            return [{'error': str(e)}]
    
    def update_user_preferences(self, user_id: int, preferences: Dict) -> None:
        """Persist updated user preferences to Redis"""
        try:
            self.redis.hmset(f'user:{user_id}:prefs', preferences)
            logger.info(f"Updated preferences for user {user_id}")
        except Exception as e:
            logger.error(f"Error updating preferences for user {user_id}: {e}")

# Flask API
app = Flask(__name__)
recommendation_engine = ContentRecommendationEngine()

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'service': 'AI Recommendation Engine',
        'version': '1.0.0',
        'timestamp': datetime.utcnow().isoformat()
    })

@app.route('/recommendations', methods=['GET'])
def get_recommendations():
    """Get content recommendations for a user"""
    try:
        user_id = int(request.args.get('user_id'))
        num_recommendations = int(request.args.get('num_recommendations', 5))
        recommendations = recommendation_engine.recommend_for_user(user_id, num_recommendations)
        return jsonify(recommendations)
    except Exception as e:
        logger.error(f"Error in get_recommendations: {e}")
        return jsonify({ 'error': str(e) }), 500

@app.route('/preferences', methods=['POST'])
def update_preferences():
    """Update user preferences"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({ 'error': 'Invalid body' }), 400
        
        user_id = int(data.get('user_id'))
        preferences = data.get('preferences', {})
        recommendation_engine.update_user_preferences(user_id, preferences)
        return jsonify({ 'success': True })
    except Exception as e:
        logger.error(f"Error in update_preferences: {e}")
        return jsonify({ 'error': str(e) }), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8778, debug=False)
