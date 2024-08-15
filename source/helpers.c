// SPDX-FileCopyrightText: 2004 rmenhal
// SPDX-FileCopyrightText: 2023 Dustin Holden <dustin@makemhz.com>
// SPDX-License-Identifier: GPL-2.0
// Based on attach.c from driveimageutils by rmenhal
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>
#include "helpers.h"

// Helper strings
ANSI_STRING AllowedExtensions[] = {
	RTL_CONSTANT_STRING(".iso"),
	RTL_CONSTANT_STRING(".cso"),
};

// Helper function to check if a file has a specific extension.
BOOLEAN IsFileDiscImage(PANSI_STRING FileName) {
	if(FileName->Length < 4)
		return FALSE;

	for(DWORD Index = 0; Index < ARRAYSIZE(AllowedExtensions); Index++) {
		PANSI_STRING Extension = &AllowedExtensions[Index];
		ANSI_STRING  FileExtension = {
			.Length        = Extension->Length,
			.MaximumLength = Extension->Length,
			.Buffer        = FileName->Buffer + FileName->Length - Extension->Length,
		};

		if(RtlEqualString(&FileExtension, Extension, TRUE))
			return TRUE;
	}

	return FALSE;
}

// Helper function to find the last occurrence of a character in a string.
// NOTE: This probably doesn't match the Win32 API implementation.
PCHAR StrRChr(PSTRING String, CHAR Char) {
	for(PCHAR Ptr = String->Buffer + String->Length - 1; Ptr >= String->Buffer; Ptr--) {
		if(*Ptr == Char)
			return Ptr;
	}

	return NULL;
}

// Helper function to compare the tails of two strings.
LONG CompareStringTails(PANSI_STRING String1, PANSI_STRING String2, WORD Skip) {
	ANSI_STRING n1, n2;

	if(String1->Length <= Skip)
		return(String2->Length <= Skip) ? 0 : -1;
	else if(String2->Length <= Skip)
		return 1;

	n1.Length = String1->Length - Skip;
	n1.MaximumLength = n1.Length;
	n1.Buffer = String1->Buffer + Skip;

	n2.Length = String2->Length - Skip;
	n2.MaximumLength = n2.Length;
	n2.Buffer = String2->Buffer + Skip;

	return RtlCompareString(&n1, &n2, TRUE);
}
