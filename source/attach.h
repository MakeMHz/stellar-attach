// SPDX-FileCopyrightText: 2004 rmenhal
// SPDX-FileCopyrightText: 2023 Dustin Holden <dustin@makemhz.com>
// SPDX-License-Identifier: GPL-2.0
// Based on virtualcdrom.h from driveimageutils by rmenhal
#pragma once

#include <xboxkrnl/xboxkrnl.h>
#include <stdint.h>

#define IOCTL_VIRTUAL_CDROM_ID      0x1EE7CD00
#define IOCTL_VIRTUAL_CDROM_ATTACH  0x1EE7CD01
#define IOCTL_VIRTUAL_CDROM_DETACH  0x1EE7CD02

#define MAX_IMAGE_SLICES            8

typedef struct _ATTACH_SLICE_DATA {
	uint32_t    num_slices;
	ANSI_STRING slice_files[MAX_IMAGE_SLICES];
} ATTACH_SLICE_DATA;
