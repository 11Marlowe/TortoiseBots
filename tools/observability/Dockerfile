# Stage 1: Build static binary
FROM golang:1.22-alpine AS builder

WORKDIR /src
COPY go.mod go.sum* ./
RUN go mod download || true

COPY . .
RUN go mod tidy
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o /bin/tortoise-observability ./cmd/server

# Stage 2: Minimal runtime image (<20MB)
FROM alpine:3.20

RUN apk --no-cache add ca-certificates tzdata

WORKDIR /app
COPY --from=builder /bin/tortoise-observability /app/tortoise-observability

# The daemon binds 127.0.0.1 by default (safe for a host install); a
# container must listen on all interfaces for its port to be publishable.
ENV HTTP_HOST=0.0.0.0

EXPOSE 8095
EXPOSE 9195/udp

ENTRYPOINT ["/app/tortoise-observability"]
