FROM python:3.11-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    git \
    ffmpeg \
    build-essential \
    libmagic1 \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy requirements first for better caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application files
COPY . .

# Create necessary directories
RUN mkdir -p /app/static/css /app/static/js /app/templates /app/weather /app/epg /app/pseudotv /app/cache

# Create non-root user
RUN groupadd -r grandma && useradd -r -g grandma grandma
RUN chown -R grandma:grandma /app
USER grandma

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8600/health || exit 1

# Expose port
EXPOSE 8600

# Run the application
CMD ["python", "grandma_app.py"]
