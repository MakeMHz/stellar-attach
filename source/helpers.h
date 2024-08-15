// SPDX-FileCopyrightText: 2004 rmenhal
// SPDX-FileCopyrightText: 2023 Dustin Holden <dustin@makemhz.com>
// SPDX-License-Identifier: GPL-2.0
// Based on virtualcdrom.h from driveimageutils by rmenhal
#pragma once
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

// Functions
BOOLEAN IsFileDiscImage(PANSI_STRING FileName);
PCHAR   StrRChr(PSTRING String, CHAR Char);
LONG    CompareStringTails(PANSI_STRING String1, PANSI_STRING String2, WORD Skip);
