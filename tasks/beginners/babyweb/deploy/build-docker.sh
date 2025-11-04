#!/bin/bash
docker rm -f ctfmarket_app
docker rm -f ctfmarket_db
docker compose build --no-cache
docker compose up --force-recreate -d
