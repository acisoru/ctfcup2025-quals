#!/bin/sh
set -e

curdir="$PWD"
pubtemp="$(mktemp -d)"

mkdir "$pubtemp/goose"
cp ./deploy/Dockerfile "$pubtemp/goose"
cp ./deploy/compose.yml "$pubtemp/goose"
cp ./deploy/app.py "$pubtemp/goose"
cp ./deploy/requirements.txt "$pubtemp/goose"
cp -r ./deploy/static "$pubtemp/goose"
cp -r ./deploy/templates "$pubtemp/goose"

cd "$pubtemp"

zip -9 -r goose.zip goose

cd "$curdir"
mv "$pubtemp/goose.zip" public

rm -rf "$pubtemp"
