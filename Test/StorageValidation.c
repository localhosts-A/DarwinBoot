// SPDX-License-Identifier: BSD-2-Clause-Patent
// QEMU-only ownership provider and deliberate first-EBS refusal. Never included
// in a device firmware DSC or normal UniXNU payload.
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UniXNULoader.h>
#include <Protocol/UniXNUStorageQuiesce.h>

STATIC EFI_EXIT_BOOT_SERVICES mRealExit;
STATIC UINT32 mOriginalCrc;
STATIC UINTN mRejectCalls, mQuiesceCalls, mExitCalls;
STATIC BOOLEAN mSealed;

STATIC EFI_STATUS UpdateServicesCrc (VOID) {
  gBS->Hdr.CRC32 = 0;
  return gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
}

STATIC EFI_STATUS EFIAPI RetryExit (EFI_HANDLE Image, UINTN Key) {
  STATIC CONST CHAR8 Pass[] = "DARWINBOOT-STORAGE-VALIDATION: forced EBS retry while sealed PASS\n";
  STATIC CONST CHAR8 Fail[] = "DARWINBOOT-STORAGE-VALIDATION: FAIL storage reopened or repeated during EBS\n";
  if (!mSealed || mQuiesceCalls != 1) {
    SerialPortWrite ((UINT8 *)Fail, sizeof Fail - 1);
    return EFI_DEVICE_ERROR;
  }
  if (++mExitCalls == 1) {
    SerialPortWrite ((UINT8 *)Pass, sizeof Pass - 1);
    return EFI_INVALID_PARAMETER;
  }
  gBS->ExitBootServices = mRealExit;
  // Restore the exact original table; no CRC service call after the final map.
  gBS->Hdr.CRC32 = mOriginalCrc;
  return mRealExit (Image, Key);
}

STATIC EFI_STATUS EFIAPI Reject (UNIXNU_STORAGE_QUIESCE_PROTOCOL *This) {
  (VOID)This;
  ++mRejectCalls;
  return EFI_TIMEOUT;
}

STATIC EFI_STATUS EFIAPI Seal (UNIXNU_STORAGE_QUIESCE_PROTOCOL *This) {
  (VOID)This;
  ++mQuiesceCalls;
  if (mSealed) { return EFI_SUCCESS; }
  mSealed = TRUE;
  mRealExit = gBS->ExitBootServices;
  mOriginalCrc = gBS->Hdr.CRC32;
  gBS->ExitBootServices = RetryExit;
  EFI_STATUS Status = UpdateServicesCrc ();
  if (Status != EFI_SUCCESS) {
    gBS->ExitBootServices = mRealExit;
    gBS->Hdr.CRC32 = mOriginalCrc;
    return EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
  }
  Print (L"DARWINBOOT-STORAGE-VALIDATION: provider sealed before final map PASS\n");
  return EFI_SUCCESS;
}

STATIC UNIXNU_STORAGE_QUIESCE_PROTOCOL mReject = { UNIXNU_STORAGE_QUIESCE_REVISION, Reject };
STATIC UNIXNU_STORAGE_QUIESCE_PROTOCOL mAccept = { UNIXNU_STORAGE_QUIESCE_REVISION, Seal };

EFI_STATUS EFIAPI StorageValidationEntry (EFI_HANDLE Image, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_LOADED_IMAGE_PROTOCOL *Loaded;
  UNIXNU_LOADER_PROTOCOL *Loader;
  UNIXNU_LOADER_BOOT_REQUEST Request;
  EFI_HANDLE Provider = NULL;
  UINT8 RamdiskByte = 0x5A;
  (VOID)SystemTable;
  EFI_STATUS Status = gBS->HandleProtocol (Image, &gEfiLoadedImageProtocolGuid, (VOID **)&Loaded);
  if (EFI_ERROR (Status)) { return Status; }
  Status = gBS->LocateProtocol (&gUniXNULoaderProtocolGuid, NULL, (VOID **)&Loader);
  if (EFI_ERROR (Status)) { return Status; }
  Status = gBS->InstallMultipleProtocolInterfaces (&Provider, &gUniXNUStorageQuiesceProtocolGuid, &mReject, NULL);
  if (EFI_ERROR (Status)) { return Status; }
  ZeroMem (&Request, sizeof Request);
  Request.Volume = Loaded->DeviceHandle;
  Request.KernelPath = L"\\EFI\\UniXNU\\Payloads\\kernel.macho";
  Request.Ramdisk = &RamdiskByte;
  Request.RamdiskSize = 1;
  Status = Loader->Boot (Loader, &Request);
  EFI_STATUS Removed = gBS->UninstallMultipleProtocolInterfaces (Provider, &gUniXNUStorageQuiesceProtocolGuid, &mReject, NULL);
  if (Status != EFI_TIMEOUT || mRejectCalls != 1 || EFI_ERROR (Removed)) {
    Print (L"DARWINBOOT-STORAGE-VALIDATION: FAIL reject status=%r calls=%lu remove=%r\n", Status, (UINT64)mRejectCalls, Removed);
    return EFI_DEVICE_ERROR;
  }
  Print (L"DARWINBOOT-STORAGE-VALIDATION: failed quiesce refused kernel entry PASS\n");
  Provider = NULL;
  return gBS->InstallMultipleProtocolInterfaces (&Provider, &gUniXNUStorageQuiesceProtocolGuid, &mAccept, NULL);
}
