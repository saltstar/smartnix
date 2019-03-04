#!/usr/bin/env bash

header() {
  echo "// This file is generated by $0.  DO NOT EDIT."
  echo
  echo '#include <stdint.h>'
  echo '#include "vdso-code.h"'
  echo
  echo "\
struct VDso::ValidSyscallPC {
"
}

footer() {
  echo '};'
}

scan() {
  local define symbol rest syscall caller
  local syscalls=''

  while read define symbol rest; do
    case "$symbol" in
    VDSO_CODE_SYSRET_*) ;;
    *) continue ;;
    esac

    syscall="${symbol#VDSO_CODE_SYSRET_zx_}"
    caller="${syscall#*_VIA_}"
    syscall="${syscall%_VIA_*}"
    if eval "test -z \"\$syscall_callers_${syscall}\""; then
      syscalls+=" $syscall"
      eval "local syscall_callers_${syscall}=\$caller"
    else
      eval "syscall_callers_${syscall}+=\" \$caller\""
    fi
  done

  for syscall in $syscalls; do
    echo "\
    static bool ${syscall}(uintptr_t offset) {
        switch (offset) {\
"
    eval "local callers=\$syscall_callers_$syscall"
    for caller in $callers; do
      echo "\
        case VDSO_CODE_SYSRET_zx_${syscall}_VIA_${caller} - VDSO_CODE_START:
            return true;\
"
    done
    echo "\
        }
        return false;
    }
"
  done
}

set -e
header
scan < "$1"
footer