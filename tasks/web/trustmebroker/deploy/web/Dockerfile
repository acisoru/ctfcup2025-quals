
FROM golang:1.25-alpine AS builder
WORKDIR /src
COPY go.mod go.sum ./
RUN go mod download
COPY . .
ENV CGO_ENABLED=0
RUN go build -trimpath -ldflags="-s -w" -o /out/web .
FROM alpine:3.20
RUN apk add --no-cache ca-certificates tzdata wget
WORKDIR /app
ENV PORT=8080 \
    DB_PATH=/data/app.db \
    PDF_SERVICE_URL=http://pdf:3000 \
    DATA_DIR=/app/data
COPY --from=builder /out/web /app/web
COPY ./flag/agreement_a916728b5c3ceec5d94e2c9011a419f8.pdf /app/data/agreements/agreement_a916728b5c3ceec5d94e2c9011a419f8.pdf
COPY ./templates ./templates
COPY ./assets ./assets
USER 0:0
EXPOSE 8080
ENTRYPOINT ["/app/web"]