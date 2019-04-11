#!/usr/bin/env bash

set -eo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZIRCON_DIR="${DIR}/../../../../.."
SCRIPTS_DIR="${ZIRCON_DIR}/scripts"

"${SCRIPTS_DIR}/package-image.sh" -b eagle \
    -d "$ZIRCON_DIR/kernel/target/arm64/board/eagle/eagle.dtb" -D append \
    -C "bootopt=64S3,32N2,64N2" \
    -g -M 0x40000000 -K 0x000000000 $@
