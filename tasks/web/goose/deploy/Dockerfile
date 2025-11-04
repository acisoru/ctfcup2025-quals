FROM python:3.13-slim

WORKDIR /app
COPY requirements.txt requirements.txt
RUN pip install -r requirements.txt
COPY static static
COPY templates templates
COPY app.py .
RUN chmod +x app.py

CMD ["/app/app.py"]
