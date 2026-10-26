// SPDX-License-Identifier: MIT
#ifndef DARWIN_KERNEL_MEASUREMENT_H
#define DARWIN_KERNEL_MEASUREMENT_H

#include <Uefi.h>

#define DARWIN_KERNEL_SHA256_PROPERTY "darwinboot-kernel-sha256"
#define DARWIN_KERNEL_SHA256_BYTES 32

// Hash the original complete file, including bytes outside loaded segments.
// This implementation is linked directly; no platform-selectable Null library.
BOOLEAN DarwinKernelSha256 (CONST VOID *Image, UINTN Bytes, UINT8 Digest[32]);

// Offset is obtained only by the full ADT validator. MAX_UINTN means absent
// (legacy RAM boot). Present slots must still contain exactly 32 zero bytes.
EFI_STATUS DarwinMeasureKernel (VOID *Tree, UINTN TreeBytes, UINTN Offset,
                               CONST VOID *Image, UINTN ImageBytes);

#endif
