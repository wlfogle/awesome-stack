# Developer Documentation

This document provides detailed information about the Open Interpreter GUI architecture, development setup, and contribution guidelines.

## Architecture Overview

### Core Components

#### MainWindow Class (`main.py`)
The central application controller that orchestrates all functionality:

```python
class MainWindow:
    def __init__(self)
    def setup_ui()           # UI initialization and layout
    def browse_directory()   # Directory selection and project detection
    def analyze_code()       # Main analysis orchestration
    def choose_model()       # AI model selection logic
    def create_prompt()      # Dynamic prompt construction
    def stream_analysis()    # Real-time progress streaming
```

#### Key Architectural Patterns

1. **MVC Pattern**: Clear separation between UI (View), business logic (Controller), and data (Model)
2. **Observer Pattern**: Progress updates and status notifications
3. **Strategy Pattern**: Different AI models for different project types
4. **Command Pattern**: Action buttons implementing specific operations

### Data Flow

```
User Input → MainWindow → Model Selection → Prompt Construction → Ollama API → Stream Processing → UI Updates
```

## Development Environment Setup

### Prerequisites
```bash
# System requirements
python --version  # Python 3.8+
ollama --version  # Ollama latest

# Development tools
pip install -r requirements-dev.txt
```

### Development Dependencies
```txt
# requirements-dev.txt
tkinter>=8.6
requests>=2.25.0
pytest>=6.0.0
black>=21.0.0
flake8>=3.8.0
mypy>=0.812
```

### Environment Setup
```bash
# Clone and setup
git clone <repository-url>
cd open_interpreter_gui

# Create virtual environment
python -m venv venv
source venv/bin/activate  # Linux/Mac
# or
venv\Scripts\activate     # Windows

# Install dependencies
pip install -r requirements.txt
pip install -r requirements-dev.txt

# Setup Ollama models
ollama pull codellama
ollama pull magicoder
ollama pull qwen2.5-coder
```

## Code Architecture Details

### UI Framework (Tkinter)
```python
# Main window structure
root = tk.Tk()
├── Header Frame
│   ├── Directory Selection
│   └── Browse Button
├── Button Panel (3x3 Grid)
│   ├── Analyze, Find Bugs, Fix Issues
│   ├── Test, Build, Run
│   └── Document, Optimize, Warp AI
├── Progress Frame
│   ├── Progress Bar
│   ├── Status Label
│   └── Cancel Button
└── Output Text Area
```

### AI Model Selection Logic
```python
def choose_model(self, project_path, file_count):
    """
    Intelligent model selection based on:
    - Project language detection
    - Codebase complexity
    - File count and size
    """
    # Language detection from file extensions
    languages = self.detect_languages(project_path)
    
    # Complexity assessment
    complexity = "complex" if file_count > 50 else "simple"
    
    # Model mapping
    model_map = {
        'cpp': {'complex': 'magicoder', 'simple': 'codellama'},
        'python': {'complex': 'qwen2.5-coder', 'simple': 'codellama'},
        'javascript': {'complex': 'starcoder2', 'simple': 'codegemma'},
        # ... additional mappings
    }
```

### Prompt Engineering System
```python
def create_prompt(self, action, code_content):
    """
    Dynamic prompt construction with:
    - Action-specific instructions
    - Code context integration
    - Output format requirements
    """
    base_prompts = {
        'analyze': "AGGRESSIVE ANALYSIS REQUIRED...",
        'find_bugs': "BUG HUNTING MODE...",
        'fix_issues': "CONCRETE FIXES DEMANDED...",
        # ... specialized prompts
    }
```

### Streaming & Progress Management
```python
def stream_analysis(self, model, prompt):
    """
    Real-time streaming with:
    - Progress bar updates
    - Cancellation support
    - Error handling
    - Timeout management
    """
    try:
        response = requests.post(
            f"{self.ollama_url}/api/generate",
            json={"model": model, "prompt": prompt, "stream": True},
            stream=True,
            timeout=300
        )
        
        for line in response.iter_lines():
            if self.cancel_requested:
                break
            # Process streaming data
            
    except requests.exceptions.Timeout:
        # Handle timeout gracefully
```

## API Integration

### Ollama API Endpoints
```python
# Primary endpoints used
BASE_URL = "http://localhost:11434"

# Generate completion
POST /api/generate
{
    "model": "codellama",
    "prompt": "...",
    "stream": true
}

# List available models
GET /api/tags

# Check model status
POST /api/show
{
    "name": "model-name"
}
```

### Warp AI Integration
```python
def generate_warp_commands(self, project_info):
    """
    Generate intelligent terminal commands:
    - Build commands for detected languages
    - Test execution commands
    - Deployment scripts
    - Debug helpers
    """
    commands = {
        'build': self.get_build_command(project_info),
        'test': self.get_test_command(project_info),
        'run': self.get_run_command(project_info),
        'debug': self.get_debug_command(project_info)
    }
```

## Error Handling Strategy

### Exception Hierarchy
```python
class OllamaError(Exception):
    """Base exception for Ollama-related errors"""

class ModelNotFoundError(OllamaError):
    """Raised when requested model is not available"""

class ServerError(OllamaError):
    """Raised for Ollama server errors (500, etc.)"""

class TimeoutError(OllamaError):
    """Raised when operations exceed timeout"""
```

### Error Recovery Patterns
```python
def robust_api_call(self, model, prompt, max_retries=3):
    """
    Implements exponential backoff and retry logic
    """
    for attempt in range(max_retries):
        try:
            return self.call_ollama(model, prompt)
        except ServerError:
            if attempt < max_retries - 1:
                time.sleep(2 ** attempt)
                continue
            raise
```

## Testing Strategy

### Unit Tests
```python
# test_main.py
class TestMainWindow:
    def test_model_selection(self):
        """Test AI model selection logic"""
        
    def test_prompt_construction(self):
        """Test prompt building for different actions"""
        
    def test_directory_analysis(self):
        """Test project structure analysis"""
```

### Integration Tests
```python
# test_integration.py
class TestOllamaIntegration:
    def test_api_connectivity(self):
        """Test Ollama API connection"""
        
    def test_streaming_response(self):
        """Test streaming response handling"""
```

### Running Tests
```bash
# Run all tests
pytest

# Run with coverage
pytest --cov=main

# Run specific test category
pytest -k "test_model"
```

## Performance Optimization

### Memory Management
- **Prompt Size Limiting**: Maximum 8KB prompts to prevent server errors
- **Stream Processing**: Process responses in chunks to avoid memory buildup
- **Resource Cleanup**: Proper cleanup of threads and network connections

### Network Optimization
- **Connection Pooling**: Reuse HTTP connections for multiple requests
- **Timeout Management**: Intelligent timeouts based on operation complexity
- **Retry Logic**: Exponential backoff for failed requests

### UI Responsiveness
- **Threading**: All API calls on separate threads
- **Progress Updates**: Regular UI updates during long operations
- **Cancellation**: User can cancel long-running operations

## Deployment

### Packaging for Distribution

#### Python Package
```bash
# Setup.py configuration
python setup.py sdist bdist_wheel
twine upload dist/*
```

#### Arch Linux Package
```bash
# PKGBUILD development
makepkg --printsrcinfo > .SRCINFO
makepkg -si  # Build and install
```

#### Cross-platform Executable
```bash
# PyInstaller configuration
pyinstaller --onefile --windowed main.py
```

## Contributing Guidelines

### Code Style
- **PEP 8**: Follow Python style guidelines
- **Black**: Use black for code formatting
- **Type Hints**: Add type hints for function parameters and returns

### Git Workflow
1. **Fork**: Create a fork of the repository
2. **Branch**: Create feature branch from main
3. **Commit**: Use conventional commit messages
4. **Test**: Ensure all tests pass
5. **PR**: Submit pull request with detailed description

### Code Review Process
- **Automated Checks**: CI/CD runs tests and linting
- **Manual Review**: Code review by maintainers
- **Documentation**: Update docs for new features
- **Backwards Compatibility**: Maintain API compatibility

## Debugging Guide

### Common Development Issues

#### Ollama Connection Problems
```bash
# Check Ollama status
ollama list
systemctl status ollama

# Test API directly
curl http://localhost:11434/api/tags
```

#### UI Threading Issues
```python
# Debug thread safety
import threading
print(f"Current thread: {threading.current_thread().name}")

# Use thread-safe UI updates
self.root.after(0, self.update_ui, data)
```

#### Memory Leaks
```python
# Profile memory usage
import tracemalloc
tracemalloc.start()

# ... run operation ...

current, peak = tracemalloc.get_traced_memory()
print(f"Current: {current / 1024 / 1024:.1f} MB")
print(f"Peak: {peak / 1024 / 1024:.1f} MB")
```

### Debug Mode
```python
# Enable debug logging
import logging
logging.basicConfig(level=logging.DEBUG)

# Add debug flags
DEBUG = os.environ.get('DEBUG', 'false').lower() == 'true'
```

## Performance Monitoring

### Metrics Collection
- **Response Time**: Track API call latencies
- **Memory Usage**: Monitor memory consumption
- **Error Rate**: Track error frequency and types
- **User Actions**: Monitor button usage patterns

### Logging Strategy
```python
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('app.log'),
        logging.StreamHandler()
    ]
)
```

## Future Enhancements

### Planned Features
- **Plugin System**: Support for custom analysis plugins
- **Configuration UI**: GUI for settings management
- **Project Templates**: Quick setup for common project types
- **Cloud Integration**: Support for cloud-based AI models
- **Collaborative Features**: Team-based code analysis

### Technical Debt
- **Refactor UI**: Move to more modern framework (PyQt6/PySide6)
- **API Abstraction**: Create abstraction layer for different AI providers
- **Configuration Management**: Implement proper config file system
- **Async Operations**: Move to async/await for better performance

---

## Quick Reference

### Key Files
- `main.py`: Main application entry point
- `README.md`: User documentation
- `DEVELOPER.md`: This developer guide
- `requirements.txt`: Python dependencies
- `PKGBUILD`: Arch Linux package definition

### Important Classes
- `MainWindow`: Primary application controller
- `AnalysisThread`: Background analysis processing
- `ProgressManager`: Progress tracking and UI updates

### Configuration
- Ollama URL: `http://localhost:11434`
- Default timeout: 300 seconds
- Max prompt size: 8KB
- Supported models: codellama, magicoder, qwen2.5-coder, codegemma, starcoder2

For additional questions or clarifications, please refer to the main documentation or create an issue in the repository.
