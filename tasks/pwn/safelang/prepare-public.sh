#!/bin/sh
set -e

curdir="$PWD"
pubtemp="$(mktemp -d)"

mkdir "$pubtemp/safelang"
cp ./deploy/Cargo.lock "$pubtemp/safelang/"
cp ./deploy/Cargo.toml "$pubtemp/safelang/"
cp -r ./deploy/src "$pubtemp/safelang/"
cp ./deploy/build.rs "$pubtemp/safelang/"
cp ./deploy/Dockerfile "$pubtemp/safelang/"
cp ./deploy/compose.yml "$pubtemp/safelang/"
cp ./deploy/entrypoint.sh "$pubtemp/safelang/"
cp ./deploy/main.py "$pubtemp/safelang/"

cd "$pubtemp"

zip -9 -r safelang.zip safelang

cd "$curdir"
mv "$pubtemp/safelang.zip" public

rm -rf "$pubtemp"
