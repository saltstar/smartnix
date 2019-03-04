#!/bin/bash

if [[ $# -lt 1 ]]; then
  echo "Insufficient number of arguments" >&2
  exit 1
fi

readonly RUNTIME_DEPS="$1"
readonly DEFAULT_NAME="$(basename "${RUNTIME_DEPS}")"
readonly DEFAULT_MANIFEST="${DEFAULT_NAME%.*}.manifest"

MANIFEST=()
while read file; do
  path="bin/$(basename $file)"
  MANIFEST+=("${path}=${file}")
done <${RUNTIME_DEPS}

(IFS=$'\n'; echo "${MANIFEST[*]}" >${2:-$DEFAULT_MANIFEST})
