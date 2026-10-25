// SPDX-License-Identifier: BSD-2-Clause-Patent
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UniXNUStorageQuiesce.h>
#include "StorageHandoff.h"

EFI_STATUS DarwinQuiesceStorage (UINTN *Completed) {
  EFI_HANDLE *Handles = NULL;
  UINTN Count = 0;
  EFI_STATUS Status, FirstError = EFI_SUCCESS;
  if (Completed == NULL) { return EFI_INVALID_PARAMETER; }
  *Completed = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gUniXNUStorageQuiesceProtocolGuid, NULL, &Count, &Handles);
  if (Status == EFI_NOT_FOUND) { Status = EFI_SUCCESS; goto Done; }
  if (Status != EFI_SUCCESS) { goto Done; }
  // Bound firmware enumeration; an absent optional interface is represented by
  // EFI_NOT_FOUND, never a malformed successful handle list.
  if (Count == 0 || Handles == NULL) { Status = EFI_COMPROMISED_DATA; goto Done; }
  if (Count > 64) { Status = EFI_BAD_BUFFER_SIZE; goto Done; }
  for (UINTN Index = 0; Index < Count; Index++) {
    UNIXNU_STORAGE_QUIESCE_PROTOCOL *Provider = NULL;
    Status = Handles[Index] == NULL ? EFI_COMPROMISED_DATA :
      gBS->HandleProtocol (Handles[Index], &gUniXNUStorageQuiesceProtocolGuid, (VOID **)&Provider);
    if (Status == EFI_SUCCESS) {
      if (Provider == NULL || Provider->Quiesce == NULL) { Status = EFI_COMPROMISED_DATA; }
      else if (Provider->Revision != UNIXNU_STORAGE_QUIESCE_REVISION) { Status = EFI_UNSUPPORTED; }
      else { Status = Provider->Quiesce (Provider); }
    }
    if (Status != EFI_SUCCESS) {
      // The ownership contract guarantees shutdown only for exact SUCCESS.
      // Warnings must not be promoted into permission to enter the kernel.
      if (!EFI_ERROR (Status)) { Status = EFI_DEVICE_ERROR; }
      if (!EFI_ERROR (FirstError)) { FirstError = Status; }
    } else { ++*Completed; }
    // Attempt the remaining providers even on failure, retaining the first
    // error. There is no rollback/reopen and the caller must refuse XNU entry.
  }
  Status = FirstError;
Done:
  if (Handles != NULL) { FreePool (Handles); }
  if (Status != EFI_SUCCESS && !EFI_ERROR (Status)) { Status = EFI_DEVICE_ERROR; }
  return Status;
}
