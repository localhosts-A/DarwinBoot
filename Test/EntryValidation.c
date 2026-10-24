// SPDX-License-Identifier: BSD-2-Clause-Patent
#include <Uefi.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UniXNULoader.h>

STATIC
EFI_STATUS
StartWithoutConsole (EFI_HANDLE Image, EFI_HANDLE Volume, BOOLEAN ClearStdErr, EFI_HANDLE *Driver)
{
  EFI_DEVICE_PATH_PROTOCOL         *Path;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SavedOut;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SavedErr;
  EFI_HANDLE                       SavedOutHandle;
  EFI_HANDLE                       SavedErrHandle;
  EFI_STATUS                       Status;
  UINT32                           SavedCrc;

  Path = FileDevicePath (Volume, L"\\EFI\\UniXNU\\Payloads\\DarwinBoot.efi");
  if (Path == NULL) { return EFI_OUT_OF_RESOURCES; }
  Status = gBS->LoadImage (FALSE, Image, Path, NULL, 0, Driver);
  FreePool (Path);
  if (EFI_ERROR (Status)) { return Status; }

  SavedOut = gST->ConOut;
  SavedErr = gST->StdErr;
  SavedOutHandle = gST->ConsoleOutHandle;
  SavedErrHandle = gST->StandardErrorHandle;
  SavedCrc = gST->Hdr.CRC32;
  gST->ConOut = NULL;
  gST->ConsoleOutHandle = NULL;
  if (ClearStdErr) {
    gST->StdErr = NULL;
    gST->StandardErrorHandle = NULL;
  }
  gST->Hdr.CRC32 = 0;
  Status = gBS->CalculateCrc32 (gST, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
  if (!EFI_ERROR (Status)) { Status = gBS->StartImage (*Driver, NULL, NULL); }
  gST->ConOut = SavedOut;
  gST->StdErr = SavedErr;
  gST->ConsoleOutHandle = SavedOutHandle;
  gST->StandardErrorHandle = SavedErrHandle;
  gST->Hdr.CRC32 = SavedCrc;
  if (EFI_ERROR (Status)) { gBS->UnloadImage (*Driver); }
  return Status;
}

EFI_STATUS
EFIAPI
EntryValidationEntry (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_LOADED_IMAGE_PROTOCOL  *Loaded;
  UNIXNU_LOADER_PROTOCOL     *Loader;
  EFI_HANDLE                Driver;
  EFI_STATUS                Status;

  (VOID)SystemTable;
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&Loaded);
  if (EFI_ERROR (Status)) { return Status; }
  for (UINTN Index = 0; Index < 2; Index++) {
    Status = StartWithoutConsole (ImageHandle, Loaded->DeviceHandle, Index == 1, &Driver);
    if (EFI_ERROR (Status)) { goto Failed; }
    Status = gBS->LocateProtocol (&gUniXNULoaderProtocolGuid, NULL, (VOID **)&Loader);
    if (EFI_ERROR (Status) || Loader == NULL) { goto Failed; }
    Print (L"DARWINBOOT-ENTRY: no ConOut%s PASS\n", Index == 1 ? L" or StdErr" : L"");
    if (Index == 0) {
      Status = gBS->UnloadImage (Driver);
      if (EFI_ERROR (Status)) { goto Failed; }
    }
  }
  return EFI_SUCCESS;
Failed:
  Print (L"DARWINBOOT-ENTRY: FAIL %r\n", Status);
  return EFI_DEVICE_ERROR;
}
