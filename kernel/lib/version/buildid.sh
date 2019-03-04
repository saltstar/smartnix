#!/usr/bin/env bash

GIT_REV="git-$(git rev-parse HEAD 2>/dev/null)"
# Is there a .git or revision?
if [ $? -eq 0 ]; then
    if [ -n "$(git status --porcelain 2>/dev/null)" ]; then
        GIT_REV+="-dirty"
    fi
else
    GIT_REV="unknown"
fi

if [ $# -eq 1 ]; then
  cat > "$1" <<END
#ifndef __BUILDID_H
#define __BUILDID_H
#define ${GIT_REV}
#endif
END
else
    echo "${GIT_REV}"
fi
