// SPDX-FileCopyrightText: 2004 rmenhal
// SPDX-FileCopyrightText: 2023 Dustin Holden <dustin@makemhz.com>
// SPDX-License-Identifier: GPL-2.0
// Based on attach.c from driveimageutils by rmenhal
#include <stdio.h>
#include <string.h>
#include <hal/xbox.h>
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>
#include "attach.h"
#include "helpers.h"

// Device name
ANSI_STRING VirtualDeviceName = RTL_CONSTANT_STRING("\\Device\\CdRom1");

// Virtual disc image slice data
CHAR VirtualFilePath[8][MAX_PATH] = { 0 };

ATTACH_SLICE_DATA AttachSliceData = {
	.NumberOfSlices = 0,
	.Files = {
		{ .Buffer = VirtualFilePath[0], .MaximumLength = sizeof(VirtualFilePath[0]) },
		{ .Buffer = VirtualFilePath[1], .MaximumLength = sizeof(VirtualFilePath[1]) },
		{ .Buffer = VirtualFilePath[2], .MaximumLength = sizeof(VirtualFilePath[2]) },
		{ .Buffer = VirtualFilePath[3], .MaximumLength = sizeof(VirtualFilePath[3]) },
		{ .Buffer = VirtualFilePath[4], .MaximumLength = sizeof(VirtualFilePath[4]) },
		{ .Buffer = VirtualFilePath[5], .MaximumLength = sizeof(VirtualFilePath[5]) },
		{ .Buffer = VirtualFilePath[6], .MaximumLength = sizeof(VirtualFilePath[6]) },
		{ .Buffer = VirtualFilePath[7], .MaximumLength = sizeof(VirtualFilePath[7]) },
	}
};

int main(void) __attribute__((optnone)) {
	// Make a copy of the XBE launch path.
	ANSI_STRING SearchPath = *XeImageFileName;

	// Determine the last backslash in the search path.
	PCHAR LastBackslash = StrRChr(&SearchPath, '\\');

	// Check if the last backslash was found.
	if(LastBackslash == NULL)
		goto CleanupAndExit;

	// Update the search path to only include the full device path.
	SearchPath.Length = (USHORT)(LastBackslash - SearchPath.Buffer + 1);

	// Sanity check the length of the search path.
	if(SearchPath.Length < sizeof("\\Device\\") || SearchPath.Length > (MAX_PATH - 4))
		goto CleanupAndExit;

	// Open the directory.
	OBJECT_ATTRIBUTES ObjectAttributes = {
		.ObjectName    = &SearchPath,
		.Attributes    = OBJ_CASE_INSENSITIVE,
	};

	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE          Handle;

	NTSTATUS Status = NtOpenFile(&Handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
		FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(Status))
		goto CleanupAndExit;

	// Query the directory for the first file.
	// TODO: Can we use the FileName mask here to filter out only the files we're interested in?
	FileInfo FileInformation = { 0 };
	Status = NtQueryDirectoryFile(Handle, NULL, NULL, NULL, &IoStatusBlock, &FileInformation, sizeof(FileInformation),
		FileDirectoryInformation, NULL, TRUE);

	// Loop through all files in the directory.
	while(TRUE) {
		// Check if we've reached the end of the directory.
		if(Status == STATUS_NO_MORE_FILES)
			break;

		// Check if an error occurred.
		if(!NT_SUCCESS(Status)) {
			NtClose(Handle);
			goto CleanupAndExit;
		}

		// Skip directories.
		if(FileInformation.DirectoryInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			goto QueryNextFile;

		// Null-terminate the file name.
		ANSI_STRING FileName = {
			.Length        = FileInformation.DirectoryInfo.FileNameLength,
			.MaximumLength = FileInformation.DirectoryInfo.FileNameLength,
			.Buffer        = FileInformation.DirectoryInfo.FileName,
		};

		// Check if the file is a supported virtual disc image.
		if(!IsFileDiscImage(&FileName))
			goto QueryNextFile;

		// Build the slice path string by concatenating the search path and the file name.
		ANSI_STRING SlicePath = {
			.Length        = 0,
			.MaximumLength = sizeof(VirtualFilePath[0]),
			.Buffer        = VirtualFilePath[AttachSliceData.NumberOfSlices]
		};

		// NOTE: We've already checked that the search path string is < MAX_PATH, so no need to check it here.
		RtlCopyString(&SlicePath, &SearchPath);

		Status = RtlAppendStringToString(&SlicePath, &FileName);
		if(!NT_SUCCESS(Status)) {
			NtClose(Handle);
			goto CleanupAndExit;
		}

		// Determine the slice index so the slices are stored in alphabetical order.
		DWORD Index;
		for(Index = 0; Index < AttachSliceData.NumberOfSlices; Index++) {
			if(CompareStringTails(&SlicePath, &AttachSliceData.Files[Index], SearchPath.Length) < 0)
				break;
		}

		// Move the slice data so the new slice can be inserted at the correct position.
		RtlMoveMemory(&AttachSliceData.Files[Index] + 1, &AttachSliceData.Files[Index],
			(AttachSliceData.NumberOfSlices - Index) * sizeof(ANSI_STRING));

		// Store the slice data.
		AttachSliceData.Files[Index] = SlicePath;

		// Increment the number of slices.
		AttachSliceData.NumberOfSlices++;

		// Check if we've reached the maximum number of slices.
		if(AttachSliceData.NumberOfSlices >= MAX_IMAGE_SLICES)
			break;

QueryNextFile:
		RtlZeroMemory(&FileInformation, sizeof(FileInformation));
		Status = NtQueryDirectoryFile(Handle, NULL, NULL, NULL, &IoStatusBlock, &FileInformation,
			sizeof(FileInformation), FileDirectoryInformation, NULL, FALSE);
	}

	// Close the directory handle.
	NtClose(Handle);

	// Open the virtual device.
	OBJECT_ATTRIBUTES DeviceObjectAttributes = {
		.ObjectName    = &VirtualDeviceName,
		.Attributes    = OBJ_CASE_INSENSITIVE,
	};

	Status = NtOpenFile(&Handle, GENERIC_READ | SYNCHRONIZE, &DeviceObjectAttributes, &IoStatusBlock, FILE_SHARE_READ,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(Status))
		goto CleanupAndExit;

	Status = NtDeviceIoControlFile(Handle, NULL, NULL, NULL, &IoStatusBlock, IOCTL_VIRTUAL_CDROM_DETACH, NULL, 0, NULL, 0);

	// Note that opening the handle will also mount a file system. All access via the handle will go through the file
	// system driver. Ideally we should first dismount any possibly mounted file system and then detach the virtual
	// disc. It would be nice if we could open a direct access to the device driver, but this doesn't seem to be possible
	// with the Xbox kernel API (opening with limited access rights doesn't yield the result.)
	//
	// We will just close the handle and dismount the automatically mounted file system.
	NtClose(Handle);
	Status = IoDismountVolumeByName(&VirtualDeviceName);

	Status = NtOpenFile(&Handle, GENERIC_READ | SYNCHRONIZE, &DeviceObjectAttributes, &IoStatusBlock, FILE_SHARE_READ,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(Status)) {
		NtClose(Handle);
		goto CleanupAndExit;
	}

	Status = NtDeviceIoControlFile(Handle, NULL, NULL, NULL, &IoStatusBlock, IOCTL_VIRTUAL_CDROM_ATTACH,
		(PVOID)&AttachSliceData, sizeof(AttachSliceData), NULL, 0);
	NtClose(Handle);

	// Also note that access via our second file handle goes through the raw file system (there's no other choice since
	// the virtual disc was detached).
	// It's still mounted at this point. If we now want to access the attached image, we will first need to dismount
	// that file system.
	// It doesn't matter for this attach.xbe, but do it anyway so we won't forget about it in any future application.
	Status = IoDismountVolumeByName(&VirtualDeviceName);

CleanupAndExit:
	return 0;
}
