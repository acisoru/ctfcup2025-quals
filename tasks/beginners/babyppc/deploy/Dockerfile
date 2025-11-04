FROM python:3.12-slim

RUN apt update && apt install -y socat
RUN mkdir /app
WORKDIR /app

COPY main.py ./
RUN chmod +x main.py

CMD ["socat", "TCP-LISTEN:8118,reuseaddr,fork", "EXEC:runuser -u nobody timeout 10 -- /app/main.py"]
