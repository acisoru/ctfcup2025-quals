#!/bin/sh

FLAG_FILE=/flag-"$(tr -dc 'a-f0-9' </dev/urandom | head -c32)".txt
echo "$FLAG" >"$FLAG_FILE"

unset FLAG
chmod 444 "$FLAG_FILE"


socat 'TCP-LISTEN:2112,reuseaddr,fork' 'SYSTEM:/app/main.py 2>&1'
