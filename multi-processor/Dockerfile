FROM python:3.11-slim

WORKDIR /app

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    git \
    curl \
    build-essential \
    libmagic1 \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements first for better caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Install tree-sitter languages
RUN python -c "
import tree_sitter_languages
tree_sitter_languages.get_language('python')
tree_sitter_languages.get_language('javascript')
tree_sitter_languages.get_language('rust')
tree_sitter_languages.get_language('bash')
"

# Copy application code
COPY . .

# Create necessary directories
RUN mkdir -p /app/output /app/archive /app/config

# Expose port
EXPOSE 8000

# Run the application
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000", "--reload"]
