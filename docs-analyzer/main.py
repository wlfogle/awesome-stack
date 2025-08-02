#!/usr/bin/env python3
"""
AI-Powered Document Analysis System
Analyzes all documents in /home/lou/awesome_stack/docs and generates detailed reports
"""

import os
import json
import asyncio
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional
import hashlib
import re

from fastapi import FastAPI, BackgroundTasks, HTTPException, Depends
from fastapi.responses import HTMLResponse, FileResponse
from fastapi.staticfiles import StaticFiles
import httpx
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import plotly.graph_objects as go
import plotly.express as px
from jinja2 import Template

from document_processor import DocumentProcessor
from report_generator import ReportGenerator
from database import DatabaseManager
from models import AnalysisRequest, AnalysisResult, ReportSummary

app = FastAPI(title="Document Analysis System", version="1.0.0")

# Configuration
DOCS_PATH = os.getenv("DOCS_PATH", "/app/documents")
REPORTS_PATH = os.getenv("REPORTS_PATH", "/app/reports")
OLLAMA_HOST = os.getenv("OLLAMA_HOST", "192.168.122.172:11434")
OLLAMA_MODEL = os.getenv("OLLAMA_MODEL", "codellama:7b")

# Initialize components
doc_processor = DocumentProcessor(DOCS_PATH, OLLAMA_HOST, OLLAMA_MODEL)
report_generator = ReportGenerator(REPORTS_PATH)
db_manager = DatabaseManager()

@app.on_event("startup")
async def startup_event():
    """Initialize the application"""
    await db_manager.init_db()
    os.makedirs(REPORTS_PATH, exist_ok=True)
    print(f"🚀 Document Analysis System started")
    print(f"📁 Monitoring documents in: {DOCS_PATH}")
    print(f"📊 Reports saved to: {REPORTS_PATH}")

@app.get("/")
async def root():
    """Main dashboard"""
    return HTMLResponse("""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Document Analysis System</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
            .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
            .header { text-align: center; margin-bottom: 40px; }
            .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
            .card { background: #fff; border: 1px solid #ddd; border-radius: 8px; padding: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
            .button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; }
            .button:hover { background: #0056b3; }
            .status { padding: 5px 10px; border-radius: 3px; font-size: 0.9em; }
            .status.running { background: #d4edda; color: #155724; }
            .status.completed { background: #cce7ff; color: #004085; }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>🧠 AI Document Analysis System</h1>
                <p>Analyze and generate reports from your awesome_stack documentation</p>
            </div>
            
            <div class="grid">
                <div class="card">
                    <h3>📊 Full Analysis</h3>
                    <p>Analyze all documents and generate comprehensive reports</p>
                    <a href="/analyze/full" class="button">Start Full Analysis</a>
                </div>
                
                <div class="card">
                    <h3>🎯 Project Status</h3>
                    <p>Analyze project progress and completion status</p>
                    <a href="/analyze/status" class="button">Analyze Status</a>
                </div>
                
                <div class="card">
                    <h3>🔍 Smart Search</h3>
                    <p>Search across all documents with AI understanding</p>
                    <a href="/search" class="button">Smart Search</a>
                </div>
                
                <div class="card">
                    <h3>📈 Reports</h3>
                    <p>View generated analysis reports</p>
                    <a href="/reports" class="button">View Reports</a>
                </div>
                
                <div class="card">
                    <h3>⚡ Quick Insights</h3>
                    <p>Get instant insights about your documentation</p>
                    <a href="/insights" class="button">Get Insights</a>
                </div>
                
                <div class="card">
                    <h3>🔄 Auto Analysis</h3>
                    <p>Schedule automatic analysis runs</p>
                    <a href="/schedule" class="button">Setup Schedule</a>
                </div>
            </div>
        </div>
    </body>
    </html>
    """)

@app.get("/analyze/full")
async def analyze_full(background_tasks: BackgroundTasks):
    """Start full document analysis"""
    analysis_id = f"full_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_full_analysis, analysis_id)
    return {"message": "Full analysis started", "analysis_id": analysis_id, "status": "running"}

@app.get("/analyze/status")
async def analyze_status(background_tasks: BackgroundTasks):
    """Analyze project status and progress"""
    analysis_id = f"status_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    background_tasks.add_task(run_status_analysis, analysis_id)
    return {"message": "Status analysis started", "analysis_id": analysis_id, "status": "running"}

@app.get("/insights")
async def get_quick_insights():
    """Get quick insights about the documentation"""
    try:
        insights = await doc_processor.get_quick_insights()
        return {"insights": insights, "timestamp": datetime.now().isoformat()}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/reports")
async def list_reports():
    """List all generated reports"""
    reports = []
    reports_dir = Path(REPORTS_PATH)
    
    if reports_dir.exists():
        for report_file in reports_dir.glob("*.html"):
            stat = report_file.stat()
            reports.append({
                "name": report_file.stem,
                "file": report_file.name,
                "size": stat.st_size,
                "created": datetime.fromtimestamp(stat.st_ctime).isoformat(),
                "modified": datetime.fromtimestamp(stat.st_mtime).isoformat()
            })
    
    return {"reports": sorted(reports, key=lambda x: x["modified"], reverse=True)}

@app.get("/reports/{report_name}")
async def get_report(report_name: str):
    """Get a specific report"""
    report_path = Path(REPORTS_PATH) / f"{report_name}.html"
    if not report_path.exists():
        raise HTTPException(status_code=404, detail="Report not found")
    return FileResponse(report_path)

@app.get("/search")
async def search_documents(q: str, limit: int = 10):
    """Smart search across all documents"""
    try:
        results = await doc_processor.smart_search(q, limit)
        return {"query": q, "results": results, "count": len(results)}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

async def run_full_analysis(analysis_id: str):
    """Run comprehensive analysis of all documents"""
    try:
        print(f"🔄 Starting full analysis: {analysis_id}")
        
        # 1. Process all documents
        documents = await doc_processor.process_all_documents()
        print(f"📄 Processed {len(documents)} documents")
        
        # 2. Generate embeddings and store in vector DB
        await doc_processor.generate_embeddings(documents)
        print("🧠 Generated embeddings")
        
        # 3. Analyze document relationships
        relationships = await doc_processor.analyze_relationships(documents)
        print(f"🔗 Found {len(relationships)} relationships")
        
        # 4. Analyze project progress
        progress = await doc_processor.analyze_project_progress(documents)
        print("📊 Analyzed project progress")
        
        # 5. Generate comprehensive report
        report = await report_generator.generate_full_report({
            "documents": documents,
            "relationships": relationships,
            "progress": progress,
            "analysis_id": analysis_id,
            "timestamp": datetime.now().isoformat()
        })
        
        # 6. Save report
        report_path = Path(REPORTS_PATH) / f"full_analysis_{analysis_id}.html"
        with open(report_path, "w") as f:
            f.write(report)
        
        print(f"✅ Full analysis completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Analysis failed: {str(e)}")

async def run_status_analysis(analysis_id: str):
    """Run project status analysis"""
    try:
        print(f"🔄 Starting status analysis: {analysis_id}")
        
        # Focus on project management documents
        project_docs = await doc_processor.get_project_documents()
        
        # Analyze current status
        status = await doc_processor.analyze_current_status(project_docs)
        
        # Generate status report
        report = await report_generator.generate_status_report({
            "status": status,
            "analysis_id": analysis_id,
            "timestamp": datetime.now().isoformat()
        })
        
        # Save report
        report_path = Path(REPORTS_PATH) / f"status_analysis_{analysis_id}.html"
        with open(report_path, "w") as f:
            f.write(report)
        
        print(f"✅ Status analysis completed: {report_path}")
        
    except Exception as e:
        print(f"❌ Status analysis failed: {str(e)}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
