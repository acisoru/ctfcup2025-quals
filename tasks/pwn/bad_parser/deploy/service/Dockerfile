FROM ubuntu:24.04@sha256:a08e551cb33850e4740772b38217fc1796a66da2506d312abe51acda354ff061

RUN apt update && apt install -y socat

WORKDIR /tmp/chall

COPY ./bad_parser .
COPY ./flag.txt .

RUN chmod +x ./bad_parser
RUN chmod ugo+r ./flag.txt

ENTRYPOINT socat \
    TCP-LISTEN:31337,reuseaddr,fork \
    EXEC:"timeout -s SIGKILL 60 runuser -u nobody -- ./bad_parser",stderr
