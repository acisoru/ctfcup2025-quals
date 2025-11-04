FROM rust@sha256:eabb786e74b520e7ea45baca03ea20c3e8c6dc037c392d457badf05d8e5818b5

RUN apt update && apt install -y python3 socat
RUN mkdir /app
WORKDIR /app

RUN useradd -M runner
COPY Cargo.toml .
COPY Cargo.lock .
COPY build.rs build.rs
COPY src src
COPY entrypoint.sh ./
RUN chmod +x entrypoint.sh
COPY main.py ./
RUN chmod +x main.py

CMD ["/app/entrypoint.sh"]
