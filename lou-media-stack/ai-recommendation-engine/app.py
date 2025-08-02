from flask import Flask, jsonify, request
import os
import requests
import openai
import numpy as np
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity

app = Flask(__name__)

# Initialize OpenAI
openai.api_key = os.getenv('OPENAI_API_KEY')

class RecommendationEngine:
    def __init__(self):
        self.radarr_url = os.getenv('RADARR_URL', 'http://radarr:7878')
        self.sonarr_url = os.getenv('SONARR_URL', 'http://sonarr:8989')
        self.radarr_api_key = os.getenv('RADARR_API_KEY')
        self.sonarr_api_key = os.getenv('SONARR_API_KEY')
        self.vectorizer = TfidfVectorizer(stop_words='english', max_features=1000)
        
    def get_radarr_movies(self):
        try:
            response = requests.get(f"{self.radarr_url}/api/v3/movie", 
                                  headers={'X-Api-Key': self.radarr_api_key})
            if response.status_code == 200:
                return response.json()
        except Exception as e:
            print(f"Error fetching Radarr movies: {e}")
        return []
    
    def get_sonarr_series(self):
        try:
            response = requests.get(f"{self.sonarr_url}/api/v3/series",
                                  headers={'X-Api-Key': self.sonarr_api_key})
            if response.status_code == 200:
                return response.json()
        except Exception as e:
            print(f"Error fetching Sonarr series: {e}")
        return []
    
    def generate_ai_recommendations(self, user_query, content_type='all'):
        try:
            # Get existing content
            movies = self.get_radarr_movies() if content_type in ['all', 'movies'] else []
            series = self.get_sonarr_series() if content_type in ['all', 'tv'] else []
            
            # Create content descriptions
            content_descriptions = []
            if movies:
                for movie in movies[:20]:  # Limit for API efficiency
                    desc = f"Movie: {movie.get('title', '')} - {movie.get('overview', '')}"
                    content_descriptions.append(desc)
            
            if series:
                for show in series[:20]:  # Limit for API efficiency
                    desc = f"TV Show: {show.get('title', '')} - {show.get('overview', '')}"
                    content_descriptions.append(desc)
            
            # Generate AI recommendation
            prompt = f"""
            Based on the user's request: "{user_query}"
            
            And their existing media library containing:
            {chr(10).join(content_descriptions[:10])}  # Show first 10 items
            
            Recommend 5 specific movies or TV shows that would match their preferences.
            Format your response as a JSON array with objects containing 'title', 'type' (movie/tv), and 'reason'.
            """
            
            response = openai.Completion.create(
                engine="text-davinci-003",
                prompt=prompt,
                max_tokens=500,
                temperature=0.7
            )
            
            return response.choices[0].text.strip()
        
        except Exception as e:
            print(f"Error generating AI recommendations: {e}")
            return "Error generating recommendations"
    
    def content_based_recommendations(self, title, content_type='movies'):
        try:
            if content_type == 'movies':
                content = self.get_radarr_movies()
            else:
                content = self.get_sonarr_series()
            
            if not content:
                return []
            
            # Find the target item
            target_item = None
            for item in content:
                if item.get('title', '').lower() == title.lower():
                    target_item = item
                    break
            
            if not target_item:
                return []
            
            # Create feature vectors from overviews
            overviews = [item.get('overview', '') for item in content]
            if not any(overviews):
                return []
            
            # Calculate similarity
            tfidf_matrix = self.vectorizer.fit_transform(overviews)
            target_idx = content.index(target_item)
            cosine_similarities = cosine_similarity(tfidf_matrix[target_idx:target_idx+1], tfidf_matrix).flatten()
            
            # Get top 5 similar items (excluding the target)
            similar_indices = cosine_similarities.argsort()[-6:-1][::-1]
            similar_indices = [idx for idx in similar_indices if idx != target_idx][:5]
            
            recommendations = []
            for idx in similar_indices:
                item = content[idx]
                recommendations.append({
                    'title': item.get('title', ''),
                    'overview': item.get('overview', ''),
                    'similarity_score': float(cosine_similarities[idx])
                })
            
            return recommendations
        
        except Exception as e:
            print(f"Error in content-based recommendations: {e}")
            return []

recommendation_engine = RecommendationEngine()

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'})

@app.route('/recommend/ai', methods=['POST'])
def ai_recommend():
    data = request.get_json()
    query = data.get('query', '')
    content_type = data.get('type', 'all')
    
    if not query:
        return jsonify({'error': 'Query required'}), 400
    
    recommendations = recommendation_engine.generate_ai_recommendations(query, content_type)
    return jsonify({'recommendations': recommendations})

@app.route('/recommend/similar/<content_type>/<title>')
def similar_content(content_type, title):
    recommendations = recommendation_engine.content_based_recommendations(title, content_type)
    return jsonify({'recommendations': recommendations})

@app.route('/library/summary')
def library_summary():
    movies = recommendation_engine.get_radarr_movies()
    series = recommendation_engine.get_sonarr_series()
    
    return jsonify({
        'movies_count': len(movies),
        'series_count': len(series),
        'total_content': len(movies) + len(series)
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)
