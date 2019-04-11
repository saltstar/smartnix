// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/bootsvc-protocol/processargs.h>
#include <zircon/processargs.h>

// Special startup handles that devmgr understands

// Optional handle to a channel that when read from will produce the root resource
#define DEVMGR_LAUNCHER_ROOT_RESOURCE_CHANNEL_HND BOOTSVC_ROOT_RESOURCE_CHANNEL_HND

// Optional handle to a channel that devmgr will connect to the devfs root after starting
#define DEVMGR_LAUNCHER_DEVFS_ROOT_HND PA_HND(PA_USER1, 0)
