/*
 * Confirm-Shim
 *
 * Copyright (c) 2016 Michael Schierl <schierlm@gmx.de> - Licensed under MIT License
 *
 * Based on:
 * UEFI:SIMPLE - UEFI development made easy
 * Copyright © 2014-2016 Pete Batard <pete@akeo.ie> - Public Domain
 * https://github.com/pbatard/uefi-simple
 */
#include <efi.h>
#include <efilib.h>

#include "security_protocol.h"

static void PrintColor(UINTN color, CHAR16* string)
{
	ST->ConOut->SetAttribute(ST->ConOut, color);
	Print(string);
}

static void PrintBackgroundLine(UINTN col, UINTN row, CHAR16 start, CHAR16 mid, CHAR16 end, BOOLEAN hole)
{
	UINTN i;
	CHAR16 line[75];
	line[0] = start;
	for (i = 1; i < 73; i++)
		line[i] = mid;
	line[73] = end;
	line[74] = 0;
	if (hole)
	{
		line[2] = 0;
		ST->ConOut->SetCursorPosition(ST->ConOut, col + 72, row);
		ST->ConOut->OutputString(ST->ConOut, line + 72);
	}
	ST->ConOut->SetCursorPosition(ST->ConOut, col, row);
	ST->ConOut->OutputString(ST->ConOut, line);
}

static BOOLEAN Confirmed(CHAR16* buffer, UINTN bufsize)
{
	UINTN columns, rows, startCol, startRow, i, j, bufpos = 0;
	CHAR16 onechar[2] = { 0, 0 };
	INT32 oldAttribute = ST->ConOut->Mode->Attribute;
	BOOLEAN oldCursorVisible = ST->ConOut->Mode->CursorVisible;
	BOOLEAN yes = FALSE, decided = FALSE;

	// initialize screen
	if (ST->ConOut->QueryMode(ST->ConOut, ST->ConOut->Mode->Mode, &columns, &rows) != EFI_SUCCESS)
		return FALSE;
	if (columns < 76 || rows < 22)
		return FALSE;
	startCol = (columns - 74) / 2;
	startRow = (rows - 20) / 2;
	ST->ConIn->Reset(ST->ConIn, FALSE);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
	ST->ConOut->EnableCursor(ST->ConOut, FALSE);
	ST->ConOut->ClearScreen(ST->ConOut);

	// first output our (untrusted) text file
	for (i = 0; i < 10; i++) {
		ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 2, startRow + 2 + i);
		for (j = 0; j < 70; j++) {
			if (bufpos >= bufsize || buffer[bufpos] == L'\n') break;
			if (buffer[bufpos] >= L' ' && buffer[bufpos] != 0xFEFF) {
				onechar[0] = buffer[bufpos];
				ST->ConOut->OutputString(ST->ConOut, onechar);
			}
			bufpos++;
		}
		while (bufpos < bufsize && buffer[bufpos] != L'\n') bufpos++;
		if (bufpos < bufsize) bufpos++;
	}

	// then draw the UI around it, to avoid the text file to overwrite the UI
	ST->ConOut->SetAttribute(ST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);
	PrintBackgroundLine(startCol, startRow, BOXDRAW_DOWN_RIGHT, BOXDRAW_HORIZONTAL, BOXDRAW_DOWN_LEFT, FALSE);
	PrintBackgroundLine(startCol, startRow + 1, BOXDRAW_VERTICAL, ' ', BOXDRAW_VERTICAL, FALSE);
	for (i = 2; i < 12; i++) {
		PrintBackgroundLine(startCol, startRow + i, BOXDRAW_VERTICAL, ' ', BOXDRAW_VERTICAL, TRUE);
	}
	for (i = 12; i < 19; i++) {
		PrintBackgroundLine(startCol, startRow + i, BOXDRAW_VERTICAL, ' ', BOXDRAW_VERTICAL, FALSE);
	}
	PrintBackgroundLine(startCol, startRow + 19, BOXDRAW_UP_RIGHT, BOXDRAW_HORIZONTAL, BOXDRAW_UP_LEFT, FALSE);

	// add bottom message
	ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 3, startRow);
	PrintColor(EFI_LIGHTRED | EFI_BACKGROUND_LIGHTGRAY, L" Are you sure you want to boot from the device claiming to contain ");
	ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 2, startRow + 13);
	PrintColor(EFI_LIGHTRED | EFI_BACKGROUND_BLUE, L"The device will have full access to all resources of your computer and");
	ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 2, startRow + 14);
	PrintColor(EFI_LIGHTRED | EFI_BACKGROUND_BLUE, L"may install or delete anything on your hard drive, including malware.");
	ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 2, startRow + 15);
	PrintColor(EFI_LIGHTRED | EFI_BACKGROUND_BLUE, L"Do not boot from this device if you do not trust its origin.");
	ST->ConOut->EnableCursor(ST->ConOut, FALSE);

	while (!decided) {
		EFI_INPUT_KEY key;

		// draw buttons
		ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 24, startRow + 17);
		if (yes) {
			PrintColor(EFI_YELLOW | EFI_BACKGROUND_CYAN, L">  Yes   <");
			ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 40, startRow + 17);
			PrintColor(EFI_DARKGRAY | EFI_BACKGROUND_CYAN, L"    No    ");
			ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 27, startRow + 17);
		}
		else {
			PrintColor(EFI_DARKGRAY | EFI_BACKGROUND_CYAN, L"   Yes    ");
			ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 40, startRow + 17);
			PrintColor(EFI_YELLOW | EFI_BACKGROUND_CYAN, L">   No   <");
			ST->ConOut->SetCursorPosition(ST->ConOut, startCol + 44, startRow + 17);
		}
		if (WaitForSingleEvent(ST->ConIn->WaitForKey, 0) != EFI_SUCCESS || ST->ConIn->ReadKeyStroke(ST->ConIn, &key) != EFI_SUCCESS)
			return FALSE;
		if (key.UnicodeChar == '\r') {
			decided = TRUE;
		}
		else if (key.UnicodeChar == '\t') {
			yes = !yes;
		}
		else {
			switch (key.ScanCode) {
			case SCAN_ESC:
				yes = FALSE;
				decided = TRUE;
				break;
			case SCAN_LEFT:
				yes = TRUE;
				break;
			case SCAN_RIGHT:
				yes = FALSE;
				break;
			}
		}
	}

	// clean up
	ST->ConOut->SetAttribute(ST->ConOut, oldAttribute);
	ST->ConOut->ClearScreen(ST->ConOut);
	ST->ConOut->EnableCursor(ST->ConOut, oldCursorVisible);
	return yes;
}

static CHAR16* GetFileDevicePath(EFI_HANDLE ImageHandle, CHAR16* FileNameSuffix, EFI_HANDLE* OutDeviceHandle)
{
	EFI_GUID loadedImageProtocol = LOADED_IMAGE_PROTOCOL;
	EFI_LOADED_IMAGE *li;
	CHAR16* myname;
	CHAR16* pathname;

	BS->HandleProtocol(ImageHandle, &loadedImageProtocol, (void **)&li);
	myname = DevicePathToStr(li->FilePath);
	pathname = AllocateZeroPool((StrLen(myname) + StrLen(FileNameSuffix) + 1) * sizeof(CHAR16));
	if (pathname != NULL) {
		StrCat(pathname, myname);
		StrCat(pathname, FileNameSuffix);
	}
	FreePool(myname);
	*OutDeviceHandle = li->DeviceHandle;
	return pathname;
}

static EFI_STATUS ReadFile(EFI_HANDLE ImageHandle, CHAR16* FileNameSuffix, CHAR16* buffer, UINTN* bufsize)
{
	static EFI_GUID simpleFSProtocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_HANDLE device;
	CHAR16 *name;
	EFI_FILE* file;
	EFI_FILE_IO_INTERFACE *drive;
	EFI_FILE *root;
	EFI_STATUS result;

	name = GetFileDevicePath(ImageHandle, FileNameSuffix, &device);

	if (name == NULL)
		return EFI_OUT_OF_RESOURCES;

	BS->HandleProtocol(device, &simpleFSProtocol, (VOID**)&drive);
	result = drive->OpenVolume(drive, &root);
	if (result == EFI_SUCCESS)
		result = root->Open(root, &file, name, EFI_FILE_MODE_READ, 0);
	FreePool(name);

	if (result == EFI_SUCCESS)
		result = file->SetPosition(file, 0);
	if (result == EFI_SUCCESS)
		result = file->Read(file, bufsize, buffer);
	if (result == EFI_SUCCESS)
		result = file->Close(file);

	*bufsize /= sizeof(CHAR16);

	return result;
}

static void RunImage(EFI_HANDLE ImageHandle, CHAR16* FileNameSuffix)
{
	EFI_HANDLE newImage;
	EFI_DEVICE_PATH* imagePath;
	EFI_HANDLE devHandle;
	CHAR16* pathname;

	pathname = GetFileDevicePath(ImageHandle, FileNameSuffix, &devHandle);
	if (pathname == NULL)
		return;
	imagePath = FileDevicePath(devHandle, pathname);
	if (BS->LoadImage(FALSE, ImageHandle, imagePath, NULL, 0, &newImage) == EFI_SUCCESS) {
		BS->StartImage(newImage, NULL, NULL);
	}
	FreePool(pathname);
	FreePool(imagePath);
}

static EFI_SECURITY_FILE_AUTHENTICATION_STATE esfas = NULL;
static EFI_SECURITY2_FILE_AUTHENTICATION es2fa = NULL;

static EFI_STATUS Security2PolicyAuthentication(const EFI_SECURITY2_PROTOCOL *This, const EFI_DEVICE_PATH_PROTOCOL *DevicePath, VOID *FileBuffer, UINTN FileSize, BOOLEAN BootPolicy)
{
	return EFI_SUCCESS;
}

static EFI_STATUS SecurityPolicyAuthentication(const EFI_SECURITY_PROTOCOL *This, UINT32 AuthenticationStatus, const EFI_DEVICE_PATH_PROTOCOL *DevicePathConst)
{
	return EFI_SUCCESS;
}

static EFI_STATUS InstallSecurityPolicy() {
	EFI_SECURITY_PROTOCOL *proto;
	EFI_SECURITY2_PROTOCOL *proto2 = NULL;
	EFI_STATUS status;

	status = LibLocateProtocol(&SECURITY_PROTOCOL_GUID, &proto);
	if (status != EFI_SUCCESS)
		return status;
	LibLocateProtocol(&SECURITY2_PROTOCOL_GUID, &proto2);

	if (proto2) {
		es2fa = proto2->FileAuthentication;
		proto2->FileAuthentication = Security2PolicyAuthentication;
	}

	esfas = proto->FileAuthenticationState;
	proto->FileAuthenticationState = SecurityPolicyAuthentication;

	/* check for security policy in write protected memory */
	if (proto->FileAuthenticationState != SecurityPolicyAuthentication)
		return EFI_ACCESS_DENIED;
	if (proto2 && proto2->FileAuthentication != Security2PolicyAuthentication)
		return EFI_ACCESS_DENIED;

	return EFI_SUCCESS;
}

static EFI_STATUS UninstallSecurityPolicy()
{
	EFI_STATUS status;
	EFI_SECURITY_PROTOCOL *proto;
	status = LibLocateProtocol(&SECURITY_PROTOCOL_GUID, &proto);
	if (status == EFI_SUCCESS)
		proto->FileAuthenticationState = esfas;

	if (es2fa && status == EFI_SUCCESS) {
		EFI_SECURITY2_PROTOCOL *proto2;
		status = LibLocateProtocol(&SECURITY2_PROTOCOL_GUID, &proto2);
		if (status == EFI_SUCCESS)
			proto2->FileAuthentication = es2fa;
	}

	return status;
}


static BOOLEAN SecureBootEnabled()
{
	EFI_GUID gvGUID = EFI_GLOBAL_VARIABLE;
	UINT8 secureBoot = 0, setupMode = 0;
	UINTN dataSize = sizeof(UINT8);
	EFI_STATUS status;

	status = RT->GetVariable(L"SecureBoot", &gvGUID, NULL, &dataSize, &secureBoot);
	if (status != EFI_SUCCESS)
		secureBoot = 0;
	status = RT->GetVariable(L"SetupMode", &gvGUID, NULL, &dataSize, &setupMode);
	if (status != EFI_SUCCESS)
		setupMode = 0;

	return secureBoot == 1 && setupMode != 1;
}

// Application entry point
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	UINTN bufsize = 10 * 75 * sizeof(CHAR16);
	CHAR16 buffer[10 * 75];

	// <filename>-confirmed.efi
	// <filename>-description.txt

	InitializeLib(ImageHandle, SystemTable);

#ifndef UI_PROTOTYPE
	if (!SecureBootEnabled()) {
		RunImage(ImageHandle, L"-confirmed.efi");
		return EFI_SUCCESS;
	}
#endif

	if (ReadFile(ImageHandle, L"-description.txt", buffer, &bufsize) == EFI_SUCCESS) {
		if (Confirmed(buffer, bufsize)) {
			if (InstallSecurityPolicy() != EFI_SUCCESS) {
				Print(L"Failed to install override security policy.");
				return EFI_SUCCESS;
			}
			RunImage(ImageHandle, L"-confirmed.efi");
			UninstallSecurityPolicy();
		}
	}
	return EFI_SUCCESS;
}
