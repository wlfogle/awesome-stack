#!/usr/bin/env python3
"""
AI-Powered Multi-Format Processing System
Reads, extracts, and recomposes documents, code, and scripts
"""

import os
import asyncio
import shutil
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Any
import json
import re
import hashlib

from fastapi import FastAPI, BackgroundTasks, HTTPException
from fastapi.responses import HTMLResponse, JSONResponse
import httpx
from jinja2 import Template

from processors.document_processor import DocumentProcessor
from processors.code_processor import CodeProcessor
from processors.script_processor import ScriptProcessor
from processors.ai_analyzer import AIAnalyzer
from organizers.content_organizer import ContentOrganizer
from recomposers.document_recomposer import DocumentRecomposer
from utils.file_utils import FileAnalyzer
from database import DatabaseManager

app = FastAPI(title="Multi-Format Processing System", version="1.0.0")

# Configuration
WORKSPACE_PATH = os.getenv("WORKSPACE_PATH", "/app/workspace")
OUTPUT_PATH = os.getenv("OUTPUT_PATH", "/app/output")
ARCHIVE_PATH = os.getenv("ARCHIVE_PATH", "/app/archive")
OLLAMA_HOST = os.getenv("OLLAMA_HOST", "192.168.122.172:11434")
OLLAMA_MODEL = os.getenv("OLLAMA_MODEL", "codellama:7b")

# Initialize processors
file_analyzer = FileAnalyzer()
doc_processor = DocumentProcessor()
code_processor = CodeProcessor()
script_processor = ScriptProcessor()
ai_analyzer = AIAnalyzer(OLLAMA_HOST, OLLAMA_MODEL)
content_organizer = ContentOrganizer()
doc_recomposer = DocumentRecomposer()
db_manager = DatabaseManager()

@app.on_event("startup")
async def startup_event():
    """Initialize the application"""
    await db_manager.init_db()
    os.makedirs(OUTPUT_PATH, exist_ok=True)
    os.makedirs(ARCHIVE_PATH, exist_ok=True)
    print(f"🚀 Multi-Format Processing System started")
    print(f"📁 Processing workspace: {WORKSPACE_PATH}")
    print(f"📤 Output directory: {OUTPUT_PATH}")
    print(f"📚 Archive directory: {ARCHIVE_PATH}")

@app.get("/")
async def root():
    """Main dashboard"""
    return HTMLResponse("""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Multi-Format Processing System</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
            .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
            .header { text-align: center; margin-bottom: 40px; }
            .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
            .card { background: #fff; border: 1px solid #ddd; border-radius: 8px; padding: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
            .button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; margin: 5px; }
            .button:hover { background: #0056b3; }
            .button.danger { background: #dc3545; }
            .button.danger:hover { background: #c82333; }
            .button.success { background: #28a745; }
            .button.success:hover { background: #218838; }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>🔧 Multi-Format Processing System</h1>
                <p>Analyze, extract, and recompose documents, code, and scripts</p>
            </div>
            
            <div class="grid">
                <div class="card">
                    <h3>📊 Full Analysis</h3>
                    <p>Scan entire workspace and analyze all content types</p>
                    <a href="/analyze/full" class="button">Start Full Analysis</a>
                </div>
                
                <div class="card">
                    <h3>🧹 Smart Cleanup</h3>
                    <p>Identify and archive outdated content automatically</p>
                    <a href="/cleanup/smart" class="button">Smart Cleanup</a>
                </div>
                
                <div class="card">
                    <h3>📝 Recompose Documents</h3>
                    <p>Merge related documents and eliminate redundancy</p>
                    <a href="/recompose/documents" class="button success">Recompose Docs</a>
                </div>
                
                <div class="card">
                    <h3>💻 Organize Code</h3>
                    <p>Analyze and organize code files and scripts</p>
                    <a href="/organize/code" class="button">Organize Code</a>
                </div>
                
                <div class="card">
                    <h3>🔍 Content Search</h3>
                    <p>Search across all file types with AI understanding</p>
                    <a href="/search" class="button">Search Content</a>
                </div>
                
                <div class="card">
                    <h3>📈 Processing Reports</h3>
                    <p>View analysis and processing reports</p>
                    <a href="/reports" class="button">View Reports</a>
                </div>
            </div>
        </div>
    </body>
    </html>
    """)

@app.get("/analyze/full")
async def analyze_full(background_tasks: BackgroundTasks):
    """Start comprehensive analysis of all content"""
    analysis_id = f"full_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_full_analysis, analysis_id)
    return {"message": "Full analysis started", "analysis_id": analysis_id, "status": "running"}

@app.get("/cleanup/smart")
async def smart_cleanup(background_tasks: BackgroundTasks):
    """Start smart cleanup process"""
    cleanup_id = f"cleanup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_smart_cleanup, cleanup_id)
    return {"message": "Smart cleanup started", "cleanup_id": cleanup_id, "status": "running"}

@app.get("/recompose/documents")
async def recompose_documents(background_tasks: BackgroundTasks):
    """Start document recomposition process"""
    recompose_id = f"recompose_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_document_recomposition, recompose_id)
    return {"message": "Document recomposition started", "recompose_id": recompose_id, "status": "running"}

@app.get("/organize/code")
async def organize_code(background_tasks: BackgroundTasks):
    """Start code organization process"""
    organize_id = f"organize_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_code_organization, organize_id)
    return {"message": "Code organization started", "organize_id": organize_id, "status": "running"}

async def run_full_analysis(analysis_id: str):
    """Comprehensive analysis of all content types"""
    try:
        print(f"🔄 Starting full analysis: {analysis_id}")
        
        # 1. Scan all files
        all_files = await file_analyzer.scan_workspace(WORKSPACE_PATH)
        print(f"📄 Found {len(all_files)} files")
        
        # 2. Categorize files by type
        categorized = await file_analyzer.categorize_files(all_files)
        print(f"📊 Categorized into {len(categorized)} categories")
        
        # 3. Analyze documents
        doc_analysis = await doc_processor.analyze_documents(categorized.get('documents', []))
        print(f"📝 Analyzed {len(doc_analysis)} documents")
        
        # 4. Analyze code files
        code_analysis = await code_processor.analyze_code(categorized.get('code', []))
        print(f"💻 Analyzed {len(code_analysis)} code files")
        
        # 5. Analyze scripts
        script_analysis = await script_processor.analyze_scripts(categorized.get('scripts', []))
        print(f"⚙️ Analyzed {len(script_analysis)} scripts")
        
        # 6. AI-powered content analysis
        ai_insights = await ai_analyzer.analyze_all_content({
            'documents': doc_analysis,
            'code': code_analysis,
            'scripts': script_analysis
        })
        print("🧠 Completed AI analysis")
        
        # 7. Generate comprehensive report
        report = await generate_analysis_report({
            'analysis_id': analysis_id,
            'timestamp': datetime.now().isoformat(),
            'files': categorized,
            'documents': doc_analysis,
            'code': code_analysis,
            'scripts': script_analysis,
            'ai_insights': ai_insights
        })
        
        # 8. Save report
        report_path = Path(OUTPUT_PATH) / f"analysis_{analysis_id}.html"
        with open(report_path, "w") as f:
            f.write(report)
        
        print(f"✅ Full analysis completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Analysis failed: {str(e)}")

async def run_smart_cleanup(cleanup_id: str):
    """Smart cleanup of outdated and redundant content"""
    try:
        print(f"🧹 Starting smart cleanup: {cleanup_id}")
        
        # 1. Identify outdated content
        outdated_files = await ai_analyzer.identify_outdated_content(WORKSPACE_PATH)
        print(f"📅 Found {len(outdated_files)} potentially outdated files")
        
        # 2. Find duplicate content
        duplicates = await content_organizer.find_duplicates(WORKSPACE_PATH)
        print(f"📄 Found {len(duplicates)} duplicate groups")
        
        # 3. Identify redundant documentation
        redundant_docs = await doc_processor.find_redundant_documentation(WORKSPACE_PATH)
        print(f"📚 Found {len(redundant_docs)} redundant documents")
        
        # 4. Create backup before cleanup
        backup_path = Path(ARCHIVE_PATH) / f"backup_{cleanup_id}"
        await create_backup(outdated_files + duplicates + redundant_docs, backup_path)
        print(f"💾 Created backup: {backup_path}")
        
        # 5. Archive outdated content
        for file_path in outdated_files:
            await archive_file(file_path, backup_path)
        
        # 6. Handle duplicates (keep best version)
        for duplicate_group in duplicates:
            await handle_duplicate_group(duplicate_group, backup_path)
        
        # 7. Consolidate redundant documents
        for redundant_group in redundant_docs:
            await consolidate_documents(redundant_group, backup_path)
        
        # 8. Generate cleanup report
        cleanup_report = await generate_cleanup_report({
            'cleanup_id': cleanup_id,
            'timestamp': datetime.now().isoformat(),
            'outdated_files': outdated_files,
            'duplicates': duplicates,
            'redundant_docs': redundant_docs,
            'backup_path': str(backup_path)
        })
        
        report_path = Path(OUTPUT_PATH) / f"cleanup_{cleanup_id}.html"
        with open(report_path, "w") as f:
            f.write(cleanup_report)
        
        print(f"✅ Smart cleanup completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Cleanup failed: {str(e)}")

async def run_document_recomposition(recompose_id: str):
    """Recompose documents by merging related content"""
    try:
        print(f"📝 Starting document recomposition: {recompose_id}")
        
        # 1. Find related documents
        related_groups = await doc_processor.find_related_documents(WORKSPACE_PATH)
        print(f"🔗 Found {len(related_groups)} related document groups")
        
        # 2. Analyze document relationships
        relationships = await ai_analyzer.analyze_document_relationships(related_groups)
        print("🧠 Analyzed document relationships")
        
        # 3. Create recomposed documents
        recomposed_docs = []
        for group in related_groups:
            if len(group) > 1:  # Only recompose if multiple related docs
                recomposed = await doc_recomposer.merge_documents(group, relationships)
                recomposed_docs.append(recomposed)
        
        print(f"📄 Created {len(recomposed_docs)} recomposed documents")
        
        # 4. Save recomposed documents
        output_dir = Path(OUTPUT_PATH) / f"recomposed_{recompose_id}"
        output_dir.mkdir(exist_ok=True)
        
        for doc in recomposed_docs:
            doc_path = output_dir / doc['filename']
            with open(doc_path, "w") as f:
                f.write(doc['content'])
        
        # 5. Generate recomposition report
        recompose_report = await generate_recompose_report({
            'recompose_id': recompose_id,
            'timestamp': datetime.now().isoformat(),
            'related_groups': related_groups,
            'recomposed_docs': recomposed_docs,
            'output_dir': str(output_dir)
        })
        
        report_path = Path(OUTPUT_PATH) / f"recompose_{recompose_id}.html"
        with open(report_path, "w") as f:
            f.write(recompose_report)
        
        print(f"✅ Document recomposition completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Recomposition failed: {str(e)}")

async def run_code_organization(organize_id: str):
    """Organize code files and scripts"""
    try:
        print(f"💻 Starting code organization: {organize_id}")
        
        # 1. Analyze code structure
        code_structure = await code_processor.analyze_code_structure(WORKSPACE_PATH)
        print("🏗️ Analyzed code structure")
        
        # 2. Identify code relationships
        code_relationships = await code_processor.find_code_relationships(WORKSPACE_PATH)
        print("🔗 Found code relationships")
        
        # 3. Organize by functionality
        organized_structure = await content_organizer.organize_code_by_function(
            code_structure, code_relationships
        )
        print("📁 Created organized structure")
        
        # 4. Create organized directory structure
        org_dir = Path(OUTPUT_PATH) / f"organized_code_{organize_id}"
        await content_organizer.create_organized_structure(organized_structure, org_dir)
        
        # 5. Generate organization report
        org_report = await generate_organization_report({
            'organize_id': organize_id,
            'timestamp': datetime.now().isoformat(),
            'code_structure': code_structure,
            'organized_structure': organized_structure,
            'output_dir': str(org_dir)
        })
        
        report_path = Path(OUTPUT_PATH) / f"organize_{organize_id}.html"
        with open(report_path, "w") as f:
            f.write(org_report)
        
        print(f"✅ Code organization completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Organization failed: {str(e)}")

# Helper functions
async def create_backup(files: List[Path], backup_dir: Path):
    """Create backup of files before modification"""
    backup_dir.mkdir(parents=True, exist_ok=True)
    for file_path in files:
        if file_path.exists():
            rel_path = file_path.relative_to(WORKSPACE_PATH)
            backup_path = backup_dir / rel_path
            backup_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file_path, backup_path)

async def archive_file(file_path: Path, archive_dir: Path):
    """Archive a file to the archive directory"""
    if file_path.exists():
        rel_path = file_path.relative_to(WORKSPACE_PATH)
        archive_path = archive_dir / "archived" / rel_path
        archive_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(file_path), str(archive_path))

async def handle_duplicate_group(duplicate_group: List[Path], backup_dir: Path):
    """Handle a group of duplicate files by keeping the best version"""
    if len(duplicate_group) > 1:
        # Find the best version (most recent, largest, or most complete)
        best_file = await ai_analyzer.select_best_duplicate(duplicate_group)
        
        # Archive the rest
        for file_path in duplicate_group:
            if file_path != best_file:
                await archive_file(file_path, backup_dir)

async def consolidate_documents(redundant_group: List[Path], backup_dir: Path):
    """Consolidate redundant documents into a single comprehensive document"""
    if len(redundant_group) > 1:
        # Create consolidated document
        consolidated = await doc_recomposer.consolidate_redundant_docs(redundant_group)
        
        # Save consolidated document
        primary_doc = redundant_group[0]
        with open(primary_doc, "w") as f:
            f.write(consolidated['content'])
        
        # Archive the rest
        for file_path in redundant_group[1:]:
            await archive_file(file_path, backup_dir)

# Report generation functions
async def generate_analysis_report(data: Dict[str, Any]) -> str:
    """Generate comprehensive analysis report"""
    template = Template("""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Full Analysis Report</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; }
            .summary { background: #f8f9fa; padding: 20px; border-radius: 5px; margin-bottom: 20px; }
            .section { margin-bottom: 30px; }
            .metric { display: inline-block; margin: 10px; padding: 10px; background: #e9ecef; border-radius: 3px; }
        </style>
    </head>
    <body>
        <h1>🔍 Full Analysis Report</h1>
        <div class="summary">
            <h2>Executive Summary</h2>
            <p><strong>Analysis ID:</strong> {{ data.analysis_id }}</p>
            <p><strong>Timestamp:</strong> {{ data.timestamp }}</p>
            <div class="metric">Documents: {{ data.documents|length }}</div>
            <div class="metric">Code Files: {{ data.code|length }}</div>
            <div class="metric">Scripts: {{ data.scripts|length }}</div>
        </div>
        
        <div class="section">
            <h2>📄 Document Analysis</h2>
            <!-- Document analysis details -->
        </div>
        
        <div class="section">
            <h2>💻 Code Analysis</h2>
            <!-- Code analysis details -->
        </div>
        
        <div class="section">
            <h2>⚙️ Script Analysis</h2>
            <!-- Script analysis details -->
        </div>
        
        <div class="section">
            <h2>🧠 AI Insights</h2>
            <!-- AI insights -->
        </div>
    </body>
    </html>
    """)
    return template.render(data=data)

async def generate_cleanup_report(data: Dict[str, Any]) -> str:
    """Generate cleanup report"""
    # Implementation for cleanup report template
    pass

async def generate_recompose_report(data: Dict[str, Any]) -> str:
    """Generate document recomposition report"""
    # Implementation for recomposition report template
    pass

async def generate_organization_report(data: Dict[str, Any]) -> str:
    """Generate code organization report"""
    # Implementation for organization report template
    pass

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
