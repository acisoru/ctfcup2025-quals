# --- build stage ---
FROM golang:1.25-alpine AS builder
WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .
COPY templates ./templates
ENV CGO_ENABLED=0
RUN go build -trimpath -ldflags="-s -w" -o /out/admin .

# --- run stage ---
FROM alpine:3.20
RUN apk add --no-cache ca-certificates tzdata wget
USER 65532:65532
WORKDIR /app

ENV PORT=9090
ENV DB_PATH=/data/app.db

COPY --from=builder /out/admin /app/admin
COPY --from=builder /src/templates ./templates

EXPOSE 9090
ENTRYPOINT ["/app/admin"]