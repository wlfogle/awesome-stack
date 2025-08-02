#!/usr/bin/env python3
"""
Document Processor for /home/lou/awesome_stack/docs
Analyzes, extracts, and recomposes documentation
"""

import os
import re
import json
import shutil
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Set, Tuple
from collections import defaultdict
import hashlib

class DocumentProcessor:
    def __init__(self, docs_path: str):
        self.docs_path = Path(docs_path)
        self.output_path = self.docs_path / "_organized"
        self.archive_path = self.docs_path / "_archive"
        self.analysis_results = {}
        
        # Create output directories
        self.output_path.mkdir(exist_ok=True)
        self.archive_path.mkdir(exist_ok=True)
        
    def analyze_all_documents(self) -> Dict:
        """Analyze all documents in the docs directory"""
        print(f"🔍 Analyzing documents in: {self.docs_path}")
        
        documents = []
        for file_path in self.docs_path.glob("**/*"):
            if file_path.is_file() and file_path.suffix in ['.md', '.txt', '.sh', '.yml', '.yaml']:
                if not str(file_path).startswith(str(self.output_path)) and not str(file_path).startswith(str(self.archive_path)):
                    doc_info = self.analyze_document(file_path)
                    documents.append(doc_info)
        
        print(f"📄 Found {len(documents)} documents to analyze")
        
        # Categorize documents
        categories = self.categorize_documents(documents)
        
        # Find related documents
        related_groups = self.find_related_documents(documents)
        
        # Identify outdated/redundant content
        outdated_docs = self.identify_outdated_documents(documents)
        duplicate_groups = self.find_duplicate_content(documents)
        
        self.analysis_results = {
            'documents': documents,
            'categories': categories,
            'related_groups': related_groups,
            'outdated_docs': outdated_docs,
            'duplicate_groups': duplicate_groups,
            'timestamp': datetime.now().isoformat()
        }
        
        return self.analysis_results
    
    def analyze_document(self, file_path: Path) -> Dict:
        """Analyze a single document"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(file_path, 'r', encoding='latin-1') as f:
                content = f.read()
        
        # Extract metadata
        lines = content.split('\n')
        word_count = len(content.split())
        
        # Analyze content structure
        headers = self.extract_headers(content)
        keywords = self.extract_keywords(content)
        dates = self.extract_dates(content)
        status_indicators = self.extract_status_indicators(content)
        
        # Calculate content hash for duplicate detection
        content_hash = hashlib.md5(content.encode()).hexdigest()
        
        return {
            'path': file_path,
            'filename': file_path.name,
            'size': file_path.stat().st_size,
            'modified': datetime.fromtimestamp(file_path.stat().st_mtime),
            'created': datetime.fromtimestamp(file_path.stat().st_ctime),
            'content': content,
            'word_count': word_count,
            'line_count': len(lines),
            'headers': headers,
            'keywords': keywords,
            'dates': dates,
            'status_indicators': status_indicators,
            'content_hash': content_hash,
            'category': self.determine_category(file_path.name, content, keywords)
        }
    
    def extract_headers(self, content: str) -> List[str]:
        """Extract markdown headers"""
        headers = []
        for line in content.split('\n'):
            if line.startswith('#'):
                headers.append(line.strip())
        return headers
    
    def extract_keywords(self, content: str) -> List[str]:
        """Extract important keywords"""
        # Common technical keywords to look for
        tech_keywords = [
            'docker', 'compose', 'kubernetes', 'proxmox', 'ollama', 'ai', 'assistant',
            'tauri', 'rust', 'python', 'javascript', 'vue', 'react', 'api', 'server',
            'database', 'postgres', 'redis', 'nginx', 'traefik', 'ssl', 'https',
            'alexa', 'voice', 'smart', 'home', 'automation', 'media', 'plex', 'jellyfin',
            'radarr', 'sonarr', 'lidarr', 'prowlarr', 'optimization', 'performance',
            'setup', 'installation', 'configuration', 'guide', 'tutorial', 'documentation'
        ]
        
        found_keywords = []
        content_lower = content.lower()
        for keyword in tech_keywords:
            if keyword in content_lower:
                found_keywords.append(keyword)
        
        return found_keywords
    
    def extract_dates(self, content: str) -> List[str]:
        """Extract dates from content"""
        date_patterns = [
            r'\d{4}-\d{2}-\d{2}',  # YYYY-MM-DD
            r'\d{2}/\d{2}/\d{4}',  # MM/DD/YYYY
            r'\d{1,2} \w+ \d{4}',  # DD Month YYYY
        ]
        
        dates = []
        for pattern in date_patterns:
            matches = re.findall(pattern, content)
            dates.extend(matches)
        
        return dates
    
    def extract_status_indicators(self, content: str) -> List[str]:
        """Extract status indicators like checkmarks, todos, etc."""
        status_patterns = [
            r'✅', r'❌', r'⚠️', r'🔄', r'📊', r'🎯',
            r'- \[x\]', r'- \[ \]',  # Markdown checkboxes
            r'TODO:', r'FIXME:', r'NOTE:', r'WARNING:',
            r'COMPLETED', r'IN PROGRESS', r'PENDING'
        ]
        
        statuses = []
        for pattern in status_patterns:
            if re.search(pattern, content, re.IGNORECASE):
                statuses.append(pattern)
        
        return statuses
    
    def determine_category(self, filename: str, content: str, keywords: List[str]) -> str:
        """Determine document category based on filename and content"""
        filename_lower = filename.lower()
        content_lower = content.lower()
        
        # Category mapping based on filename patterns
        if 'alexa' in filename_lower:
            return 'smart_home'
        elif any(word in filename_lower for word in ['ai', 'assistant', 'tauri']):
            return 'ai_systems'
        elif any(word in filename_lower for word in ['proxmox', 'vm', 'container']):
            return 'infrastructure'
        elif any(word in filename_lower for word in ['media', 'grandma', 'stack']):
            return 'media_stack'
        elif any(word in filename_lower for word in ['optimization', 'performance', 'hardware']):
            return 'optimization'
        elif any(word in filename_lower for word in ['project', 'plan', 'summary']):
            return 'project_management'
        elif filename_lower.endswith('.sh'):
            return 'scripts'
        elif any(word in filename_lower for word in ['config', 'settings', 'api-keys']):
            return 'configuration'
        else:
            return 'general'
    
    def categorize_documents(self, documents: List[Dict]) -> Dict[str, List[Dict]]:
        """Group documents by category"""
        categories = defaultdict(list)
        for doc in documents:
            categories[doc['category']].append(doc)
        return dict(categories)
    
    def find_related_documents(self, documents: List[Dict]) -> List[List[Dict]]:
        """Find groups of related documents"""
        related_groups = []
        processed = set()
        
        for i, doc1 in enumerate(documents):
            if i in processed:
                continue
                
            group = [doc1]
            processed.add(i)
            
            for j, doc2 in enumerate(documents[i+1:], i+1):
                if j in processed:
                    continue
                
                # Check for relationship based on various factors
                if self.are_documents_related(doc1, doc2):
                    group.append(doc2)
                    processed.add(j)
            
            if len(group) > 1:
                related_groups.append(group)
        
        return related_groups
    
    def are_documents_related(self, doc1: Dict, doc2: Dict) -> bool:
        """Determine if two documents are related"""
        # Same category
        if doc1['category'] == doc2['category']:
            # Check for similar keywords
            common_keywords = set(doc1['keywords']) & set(doc2['keywords'])
            if len(common_keywords) >= 3:
                return True
            
            # Check for similar filename patterns
            name1_parts = set(doc1['filename'].lower().replace('-', ' ').replace('_', ' ').split())
            name2_parts = set(doc2['filename'].lower().replace('-', ' ').replace('_', ' ').split())
            common_name_parts = name1_parts & name2_parts
            if len(common_name_parts) >= 2:
                return True
        
        return False
    
    def identify_outdated_documents(self, documents: List[Dict]) -> List[Dict]:
        """Identify potentially outdated documents"""
        outdated = []
        cutoff_date = datetime.now().replace(year=datetime.now().year - 1)  # 1 year old
        
        for doc in documents:
            # Check modification date
            if doc['modified'] < cutoff_date:
                # Look for indicators of outdated content
                content_lower = doc['content'].lower()
                outdated_indicators = [
                    'old', 'deprecated', 'legacy', 'obsolete', 'archived',
                    'no longer', 'discontinued', 'replaced by'
                ]
                
                if any(indicator in content_lower for indicator in outdated_indicators):
                    outdated.append(doc)
                    continue
                
                # Check for version numbers that might be old
                version_patterns = [r'v\d+\.\d+', r'version \d+\.\d+']
                for pattern in version_patterns:
                    matches = re.findall(pattern, content_lower)
                    if matches:
                        # Simple heuristic: if version is < 2.0, might be outdated
                        for match in matches:
                            version_num = re.search(r'\d+\.\d+', match)
                            if version_num and float(version_num.group()) < 2.0:
                                outdated.append(doc)
                                break
        
        return outdated
    
    def find_duplicate_content(self, documents: List[Dict]) -> List[List[Dict]]:
        """Find documents with duplicate or very similar content"""
        duplicates = []
        content_groups = defaultdict(list)
        
        # Group by content hash
        for doc in documents:
            content_groups[doc['content_hash']].append(doc)
        
        # Find groups with multiple documents
        for content_hash, docs in content_groups.items():
            if len(docs) > 1:
                duplicates.append(docs)
        
        # Also check for similar content (not exact duplicates)
        similar_groups = self.find_similar_content(documents)
        duplicates.extend(similar_groups)
        
        return duplicates
    
    def find_similar_content(self, documents: List[Dict]) -> List[List[Dict]]:
        """Find documents with similar content using basic text similarity"""
        similar_groups = []
        processed = set()
        
        for i, doc1 in enumerate(documents):
            if i in processed:
                continue
            
            group = [doc1]
            for j, doc2 in enumerate(documents[i+1:], i+1):
                if j in processed:
                    continue
                
                # Simple similarity check based on shared lines
                lines1 = set(doc1['content'].split('\n'))
                lines2 = set(doc2['content'].split('\n'))
                
                if len(lines1) > 0 and len(lines2) > 0:
                    similarity = len(lines1 & lines2) / len(lines1 | lines2)
                    if similarity > 0.7:  # 70% similar
                        group.append(doc2)
                        processed.add(j)
            
            if len(group) > 1:
                similar_groups.append(group)
                for doc in group:
                    processed.add(documents.index(doc))
        
        return similar_groups
    
    def organize_documents(self):
        """Organize documents based on analysis results"""
        if not self.analysis_results:
            print("❌ No analysis results found. Run analyze_all_documents() first.")
            return
        
        print("📁 Organizing documents...")
        
        # Create category directories
        for category in self.analysis_results['categories']:
            category_dir = self.output_path / category
            category_dir.mkdir(exist_ok=True)
        
        # Archive outdated documents
        outdated_dir = self.archive_path / "outdated"
        outdated_dir.mkdir(exist_ok=True)
        
        for doc in self.analysis_results['outdated_docs']:
            dest_path = outdated_dir / doc['filename']
            print(f"📦 Archiving outdated: {doc['filename']}")
            shutil.move(str(doc['path']), str(dest_path))
        
        # Handle duplicates
        duplicates_dir = self.archive_path / "duplicates"
        duplicates_dir.mkdir(exist_ok=True)
        
        for duplicate_group in self.analysis_results['duplicate_groups']:
            if len(duplicate_group) > 1:
                # Keep the most recent version
                latest_doc = max(duplicate_group, key=lambda x: x['modified'])
                
                for doc in duplicate_group:
                    if doc != latest_doc:
                        dest_path = duplicates_dir / doc['filename']
                        print(f"📦 Archiving duplicate: {doc['filename']}")
                        if doc['path'].exists():
                            shutil.move(str(doc['path']), str(dest_path))
        
        # Organize remaining documents by category
        for category, docs in self.analysis_results['categories'].items():
            category_dir = self.output_path / category
            
            for doc in docs:
                if doc['path'].exists():  # Check if not already moved
                    dest_path = category_dir / doc['filename']
                    print(f"📁 Moving to {category}: {doc['filename']}")
                    shutil.copy2(str(doc['path']), str(dest_path))
    
    def recompose_related_documents(self):
        """Merge related documents into consolidated versions"""
        if not self.analysis_results:
            print("❌ No analysis results found. Run analyze_all_documents() first.")
            return
        
        print("📝 Recomposing related documents...")
        
        recomposed_dir = self.output_path / "recomposed"
        recomposed_dir.mkdir(exist_ok=True)
        
        for i, related_group in enumerate(self.analysis_results['related_groups']):
            if len(related_group) > 1:
                # Create a merged document
                merged_content = self.merge_documents(related_group)
                
                # Generate filename based on common themes
                common_name = self.generate_merged_filename(related_group)
                merged_file = recomposed_dir / f"{common_name}.md"
                
                with open(merged_file, 'w', encoding='utf-8') as f:
                    f.write(merged_content)
                
                print(f"📝 Created merged document: {merged_file.name}")
                
                # Archive original files
                for doc in related_group:
                    if doc['path'].exists():
                        archive_path = self.archive_path / "merged_originals" / doc['filename']
                        archive_path.parent.mkdir(exist_ok=True)
                        shutil.move(str(doc['path']), str(archive_path))
    
    def merge_documents(self, documents: List[Dict]) -> str:
        """Merge multiple documents into a single comprehensive document"""
        merged_content = []
        
        # Add header
        doc_names = [doc['filename'] for doc in documents]
        merged_content.append(f"# Merged Documentation")
        merged_content.append(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        merged_content.append(f"**Source Documents**: {', '.join(doc_names)}")
        merged_content.append("")
        
        # Add table of contents
        merged_content.append("## Table of Contents")
        for i, doc in enumerate(documents, 1):
            merged_content.append(f"{i}. [{doc['filename']}](#{doc['filename'].lower().replace(' ', '-').replace('.', '')})")
        merged_content.append("")
        
        # Add content from each document
        for doc in documents:
            merged_content.append(f"## {doc['filename']}")
            merged_content.append(f"**Last Modified**: {doc['modified'].strftime('%Y-%m-%d')}")
            merged_content.append("")
            merged_content.append(doc['content'])
            merged_content.append("")
            merged_content.append("---")
            merged_content.append("")
        
        return '\n'.join(merged_content)
    
    def generate_merged_filename(self, documents: List[Dict]) -> str:
        """Generate a filename for merged documents"""
        # Find common words in filenames
        all_words = []
        for doc in documents:
            words = doc['filename'].lower().replace('-', ' ').replace('_', ' ').replace('.md', '').split()
            all_words.extend(words)
        
        # Count word frequency
        word_count = defaultdict(int)
        for word in all_words:
            word_count[word] += 1
        
        # Find most common words (appearing in multiple documents)
        common_words = [word for word, count in word_count.items() if count > 1]
        
        if common_words:
            return '-'.join(common_words[:3])  # Use top 3 common words
        else:
            # Fallback to category-based naming
            categories = [doc['category'] for doc in documents]
            most_common_category = max(set(categories), key=categories.count)
            return f"{most_common_category}-consolidated"
    
    def generate_report(self) -> str:
        """Generate a comprehensive analysis report"""
        if not self.analysis_results:
            return "No analysis results available."
        
        report = []
        report.append("# Document Analysis Report")
        report.append(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"**Analyzed Path**: {self.docs_path}")
        report.append("")
        
        # Summary statistics
        total_docs = len(self.analysis_results['documents'])
        total_categories = len(self.analysis_results['categories'])
        total_outdated = len(self.analysis_results['outdated_docs'])
        total_duplicates = sum(len(group) for group in self.analysis_results['duplicate_groups'])
        
        report.append("## Summary")
        report.append(f"- **Total Documents**: {total_docs}")
        report.append(f"- **Categories**: {total_categories}")
        report.append(f"- **Outdated Documents**: {total_outdated}")
        report.append(f"- **Duplicate Documents**: {total_duplicates}")
        report.append(f"- **Related Groups**: {len(self.analysis_results['related_groups'])}")
        report.append("")
        
        # Category breakdown
        report.append("## Category Breakdown")
        for category, docs in self.analysis_results['categories'].items():
            report.append(f"- **{category.replace('_', ' ').title()}**: {len(docs)} documents")
        report.append("")
        
        # Outdated documents
        if self.analysis_results['outdated_docs']:
            report.append("## Outdated Documents")
            for doc in self.analysis_results['outdated_docs']:
                report.append(f"- {doc['filename']} (last modified: {doc['modified'].strftime('%Y-%m-%d')})")
            report.append("")
        
        # Duplicate groups
        if self.analysis_results['duplicate_groups']:
            report.append("## Duplicate/Similar Content Groups")
            for i, group in enumerate(self.analysis_results['duplicate_groups'], 1):
                report.append(f"### Group {i}")
                for doc in group:
                    report.append(f"- {doc['filename']}")
                report.append("")
        
        # Related document groups
        if self.analysis_results['related_groups']:
            report.append("## Related Document Groups")
            for i, group in enumerate(self.analysis_results['related_groups'], 1):
                report.append(f"### Group {i}")
                for doc in group:
                    report.append(f"- {doc['filename']} ({doc['category']})")
                report.append("")
        
        return '\n'.join(report)

def main():
    """Main function to run the document processor"""
    docs_path = "/home/lou/awesome_stack/docs"
    
    if not os.path.exists(docs_path):
        print(f"❌ Directory not found: {docs_path}")
        return
    
    processor = DocumentProcessor(docs_path)
    
    print("🚀 Starting document analysis and organization...")
    
    # Step 1: Analyze all documents
    print("\n📊 Step 1: Analyzing documents...")
    processor.analyze_all_documents()
    
    # Step 2: Generate and save report
    print("\n📝 Step 2: Generating analysis report...")
    report = processor.generate_report()
    report_path = processor.output_path / "analysis_report.md"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write(report)
    print(f"📋 Report saved to: {report_path}")
    
    # Step 3: Organize documents
    print("\n📁 Step 3: Organizing documents...")
    processor.organize_documents()
    
    # Step 4: Recompose related documents
    print("\n🔧 Step 4: Recomposing related documents...")
    processor.recompose_related_documents()
    
    print(f"\n✅ Processing complete!")
    print(f"📂 Organized files: {processor.output_path}")
    print(f"📦 Archived files: {processor.archive_path}")
    print(f"📊 Analysis report: {report_path}")

if __name__ == "__main__":
    main()
