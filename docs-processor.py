#!/usr/bin/env python3
"""
Document Processor for /home/lou/github
Analyzes, extracts, and organizes documentation, projects, and pages
"""

import os
import re
import json
import shutil
import requests
from pathlib import Path
from datetime import datetime
from typing import List, Dict
import hashlib

class DocumentProcessor:
    def __init__(self, docs_path: str = "/home/lou/github"):
        self.docs_path = Path(docs_path)
        self.output_path = self.docs_path / "_organized"
        self.archive_path = self.docs_path / "_archive"
        self.github_token = os.getenv('GITHUB_TOKEN')  # Get the token from environment variable
        self.analysis_results = {}

        # Create output directories
        self.output_path.mkdir(exist_ok=True)
        self.archive_path.mkdir(exist_ok=True)

    def analyze_all_documents(self) -> Dict:
        """Analyze all documents, projects, and pages in the docs directory"""
        print(f"🔍 Analyzing documents in: {self.docs_path}")

        documents = []
        for file_path in self.docs_path.glob("**/*"):
            if file_path.is_file() and file_path.suffix in ['.md', '.txt', '.sh', '.yml', '.yaml', '.py', '.pdf', '.docx', '.html']:
                if not str(file_path).startswith(str(self.output_path)) and not str(file_path).startswith(str(self.archive_path)):
                    doc_info = self.analyze_document(file_path)
                    documents.append(doc_info)

        print(f"📄 Found {len(documents)} documents to analyze")

        # Analyze GitHub Projects
        project_data = self.analyze_github_projects()

        # Combine results
        self.analysis_results = {
            'documents': documents,
            'projects': project_data,
            'timestamp': datetime.now().isoformat()
        }

        # Save results to a JSON file
        self.save_results()

        return self.analysis_results

    def save_results(self):
        """Save analysis results to a JSON file"""
        output_file = self.output_path / "analysis_results.json"
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(self.analysis_results, f, ensure_ascii=False, indent=2)
        print(f"📁 Analysis results saved to: {output_file}")

    def analyze_github_projects(self) -> List[Dict]:
        """Fetch and analyze GitHub Projects"""
        projects = []
        url = "https://api.github.com/projects/1"  # Replace with your project URL
        headers = {'Authorization': f'token {self.github_token}'}

        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            project_info = response.json()
            project_details = {
                'name': project_info.get('name'),
                'body': project_info.get('body'),
                'url': project_info.get('html_url'),
                'created_at': project_info.get('created_at'),
                'updated_at': project_info.get('updated_at'),
                'columns': self.fetch_project_columns(project_info.get('columns_url'), headers)
            }
            projects.append(project_details)
        else:
            print(f"Error fetching project data: {response.status_code}")

        return projects

    def fetch_project_columns(self, columns_url: str, headers: Dict) -> List[Dict]:
        """Fetch columns for a GitHub Project"""
        columns = []
        response = requests.get(columns_url, headers=headers)
        if response.status_code == 200:
            columns_data = response.json()
            for column in columns_data:
                columns.append({
                    'name': column.get('name'),
                    'cards': self.fetch_column_cards(column.get('cards_url'), headers)
                })
        return columns

    def fetch_column_cards(self, cards_url: str, headers: Dict) -> List[Dict]:
        """Fetch cards for a specific column in a GitHub Project"""
        cards = []
        response = requests.get(cards_url, headers=headers)
        if response.status_code == 200:
            cards_data = response.json()
            for card in cards_data:
                cards.append({
                    'note': card.get('note'),
                    'url': card.get('url'),
                    'created_at': card.get('created_at'),
                    'updated_at': card.get('updated_at')
                })
        return cards

    def analyze_document(self, file_path: Path) -> Dict:
        """Analyze a single document"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecode
