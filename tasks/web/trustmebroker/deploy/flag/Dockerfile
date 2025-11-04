FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

WORKDIR /app

RUN pip install --no-cache-dir requests

COPY flag.py /app/flag.py

CMD ["python", "/app/flag.py"]
