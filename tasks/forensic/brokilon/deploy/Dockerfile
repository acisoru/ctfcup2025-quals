FROM python:3.11-slim

# Install socat for network communication
RUN apt-get update && apt-get install -y socat && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy server script
COPY c2_server.py .

# Create directory for file storage
RUN mkdir -p /app/files

# Make script executable
RUN chmod +x c2_server.py
COPY 60843cbbdcec4da25bfa97e00b164991 /app/files/

# Expose port
EXPOSE 4444

# Use socat to listen on port and pipe to Python script
CMD ["socat", "TCP-LISTEN:4444,fork,reuseaddr", "EXEC:python3 c2_server.py"]
