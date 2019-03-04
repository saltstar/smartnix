#!/bin/sh

# restore config.mk from the cached configs directory
# if it exists
cp -f ./prebuilt/configs/config.mk ./prebuilt/config.mk

echo "Downloading Toolchain"
./scripts/download-prebuilt

# save config.mk to the configs directory so it will be
# cached along with the downloaded toolchain
cp -f ./prebuilt/config.mk ./prebuilt/configs

echo "Starting build '$PROJECT'"
make $PROJECT -j16
