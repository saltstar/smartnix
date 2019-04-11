#!/usr/bin/env bash

set -eo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZIRCON_DIR="${DIR}/../../../../.."
SCRIPTS_DIR="${ZIRCON_DIR}/scripts"

"${SCRIPTS_DIR}/package-image.sh" -a -b imx8mevk \
    -d "$ZIRCON_DIR/kernel/target/arm64/dtb/dummy-device-tree.dtb" -D mkbootimg \
    -M 0x40000000 $@
