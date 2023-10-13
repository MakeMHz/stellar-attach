// SPDX-FileCopyrightText: 2004 rmenhal
// SPDX-FileCopyrightText: 2023 Dustin Holden <dustin@makemhz.com>
// SPDX-License-Identifier: GPL-2.0
// Based on attach.c from driveimageutils by rmenhal
#include <xboxkrnl/xboxkrnl.h>
#include <stdio.h>
#include <string.h>
#include "attach.h"

#define MAX_PATHNAME 256

int has_iso_extension(unsigned int len, char *str) {
	ANSI_STRING tail;
	char extension[] = ".iso";
	ANSI_STRING ext_str = { sizeof(extension) - 1, sizeof(extension), extension };

	if(len < sizeof(extension) - 1)
		return FALSE;

	tail.Length = sizeof(extension) - 1;
	tail.MaximumLength = tail.Length;
	tail.Buffer = str + len - sizeof(extension) + 1;

	return (RtlCompareString(&tail, &ext_str, TRUE) == 0);
}

int has_cso_extension(unsigned int len, char *str) {
	ANSI_STRING tail;
	char extension[] = ".cso";
	ANSI_STRING ext_str = { sizeof(extension) - 1, sizeof(extension), extension };

	if(len < sizeof(extension) - 1)
		return FALSE;

	tail.Length = sizeof(extension) - 1;
	tail.MaximumLength = tail.Length;
	tail.Buffer = str + len - sizeof(extension) + 1;

	return (RtlCompareString(&tail, &ext_str, TRUE) == 0);
}

int compare_string_tails(PANSI_STRING str1, PANSI_STRING str2, WORD skip) {
	ANSI_STRING n1, n2;

	if(str1->Length <= skip)
		return(str2->Length <= skip) ? 0 : -1;
	else if(str2->Length <= skip)
		return 1;

	n1.Length = str1->Length - skip;
	n1.MaximumLength = n1.Length;
	n1.Buffer = str1->Buffer + skip;

	n2.Length = str2->Length - skip;
	n2.MaximumLength = n2.Length;
	n2.Buffer = str2->Buffer + skip;

	return RtlCompareString(&n1, &n2, TRUE);
}

int main(void) {
	char info_buf[sizeof(FILE_DIRECTORY_INFORMATION) + MAX_PATHNAME];

	char path[MAX_PATHNAME] = { 0 };
	strncpy(path, XeImageFileName->Buffer,
		XeImageFileName->Length < (MAX_PATHNAME - 1) ? XeImageFileName->Length : (MAX_PATHNAME - 1));

	int pathlen = strrchr(path, '\\') - path + 1;
	path[pathlen] = '\0';

	ANSI_STRING dir_name;
	RtlInitAnsiString(&dir_name, path);

	OBJECT_ATTRIBUTES obj_attr = {
		.RootDirectory = NULL,
		.ObjectName    = &dir_name,
		.Attributes    = OBJ_CASE_INSENSITIVE,
	};

	IO_STATUS_BLOCK io_status;
	HANDLE h;
	NTSTATUS status = NtOpenFile(&h, GENERIC_READ | SYNCHRONIZE, &obj_attr, &io_status,
			FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(status))
		HalReturnToFirmware(HalQuickRebootRoutine);

	void *membuf = NULL;
	unsigned long membuf_size = 1024 * 1024;

	status = NtAllocateVirtualMemory(&membuf, 0, &membuf_size, MEM_COMMIT | MEM_NOZERO, PAGE_READWRITE);

	if(!NT_SUCCESS(status))
		HalReturnToFirmware(HalQuickRebootRoutine);

	ATTACH_SLICE_DATA *asd = (ATTACH_SLICE_DATA *)membuf;
	asd->num_slices = 0;

	char *strbuf = (char *)membuf + sizeof(ATTACH_SLICE_DATA);

	PFILE_DIRECTORY_INFORMATION dir_info;
	BOOLEAN first = TRUE;
	for(;;) {
		if(first || dir_info->NextEntryOffset == 0) {
			status = NtQueryDirectoryFile(h, NULL, NULL, NULL, &io_status,
						info_buf, sizeof(info_buf), FileDirectoryInformation, NULL, first);

			if(status == STATUS_NO_MORE_FILES)
				break;

			first = FALSE;
			dir_info = (PFILE_DIRECTORY_INFORMATION)info_buf;
		} else
			dir_info = (PFILE_DIRECTORY_INFORMATION)((char *)dir_info + dir_info->NextEntryOffset);

		if(!has_iso_extension(dir_info->FileNameLength, dir_info->FileName) &&
		   !has_cso_extension(dir_info->FileNameLength, dir_info->FileName))
			continue;

		ANSI_STRING new_file = {
			.Length        = pathlen + dir_info->FileNameLength,
			.MaximumLength = new_file.Length,
			.Buffer        = strbuf
		};

		memcpy(strbuf, path, pathlen);
		memcpy(strbuf + pathlen, dir_info->FileName, dir_info->FileNameLength);
		strbuf += new_file.Length;

		int i;
		for(i = 0; i < asd->num_slices; i++) {
			if(compare_string_tails(&new_file, &asd->slice_files[i], pathlen) < 0)
				break;
		}

		RtlMoveMemory(&asd->slice_files[i] + 1, &asd->slice_files[i], (asd->num_slices - i) * sizeof(ANSI_STRING));

		asd->slice_files[i] = new_file;

		if(++asd->num_slices >= MAX_IMAGE_SLICES)
			break;
	}

	NtClose(h);

	ANSI_STRING dev_name;
	RtlInitAnsiString(&dev_name, "\\Device\\CdRom1");

	obj_attr.RootDirectory = NULL;
	obj_attr.ObjectName = &dev_name;
	obj_attr.Attributes = OBJ_CASE_INSENSITIVE;

	status = NtOpenFile(&h, GENERIC_READ | SYNCHRONIZE, &obj_attr, &io_status,
			FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(status))
		HalReturnToFirmware(HalQuickRebootRoutine);

	status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &io_status, IOCTL_VIRTUAL_CDROM_DETACH, NULL, 0, NULL, 0);

	/* Note that opening the handle will also mount a file system. All access
	* via the handle will go through the file system driver. Ideally we should
	* first dismount any possibly mounted file system and then detach the virtual
	* disc. It would be nice if we could open a direct access to the device driver,
	* but this doesn't seem to be possible with the Xbox kernel API (opening with
	* limited access rights doesn't yield the result.)
	*
	* We will just close the handle and dismount the automatically mounted file
	* system.
	*/
	NtClose(h);
	status = IoDismountVolumeByName(&dev_name);

	status = NtOpenFile(&h, GENERIC_READ | SYNCHRONIZE, &obj_attr, &io_status,
			FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT);

	if(!NT_SUCCESS(status))
		HalReturnToFirmware(HalQuickRebootRoutine);

	status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &io_status,
				IOCTL_VIRTUAL_CDROM_ATTACH,
				asd, sizeof(ATTACH_SLICE_DATA), NULL, 0);

	NtClose(h);

	/* Also note that access via our second file handle goes through the raw
	* file system (there's no other choice since the virtual disc was detached).
	* It's still mounted at this point. If we now want to access the attached
	* image, we will first need to dismount that file system.
	*
	* It doesn't matter for this attach.xbe, but do it anyway so we won't forget
	* about it in any future application.
	*/
	status = IoDismountVolumeByName(&dev_name);

	HalReturnToFirmware(HalQuickRebootRoutine);

	return 0;
}
