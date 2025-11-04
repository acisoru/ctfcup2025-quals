#!/bin/sh
set -e

curdir="$PWD"
pubtemp="$(mktemp -d)"

mkdir "$pubtemp/cursed"
cp ./dev/task.sage "$pubtemp/cursed"
cp ./dev/output.sobj "$pubtemp/cursed"

cd "$pubtemp"

zip -9 -r cursed.zip cursed

cd "$curdir"
mv "$pubtemp/cursed.zip" public

rm -rf "$pubtemp"
