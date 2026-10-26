// SPDX-License-Identifier: MIT
// SHA-256 compression/padding adapted from UniXNU NativeAdrenoSha256.cpp.
// Kept local to the loader so firmware crypto policy cannot select a Null hash.
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include "KernelMeasurement.h"

STATIC CONST UINT32 mK[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

STATIC UINT32 Ror (UINT32 Value, UINTN Bits) { return (Value >> Bits) | (Value << (32 - Bits)); }

STATIC VOID Compress (UINT32 State[8], CONST UINT8 Block[64])
{
  UINT32 W[64], A, B, C, D, E, F, G, H, S0, S1, T1, T2;
  UINTN I;
  for (I = 0; I < 16; I++) {
    W[I] = ((UINT32)Block[I*4] << 24) | ((UINT32)Block[I*4+1] << 16) |
           ((UINT32)Block[I*4+2] << 8) | Block[I*4+3];
  }
  for (I = 16; I < 64; I++) {
    S0 = Ror (W[I-15], 7) ^ Ror (W[I-15], 18) ^ (W[I-15] >> 3);
    S1 = Ror (W[I-2], 17) ^ Ror (W[I-2], 19) ^ (W[I-2] >> 10);
    W[I] = W[I-16] + S0 + W[I-7] + S1;
  }
  A=State[0]; B=State[1]; C=State[2]; D=State[3];
  E=State[4]; F=State[5]; G=State[6]; H=State[7];
  for (I = 0; I < 64; I++) {
    S1 = Ror (E, 6) ^ Ror (E, 11) ^ Ror (E, 25);
    T1 = H + S1 + ((E & F) ^ (~E & G)) + mK[I] + W[I];
    S0 = Ror (A, 2) ^ Ror (A, 13) ^ Ror (A, 22);
    T2 = S0 + ((A & B) ^ (A & C) ^ (B & C));
    H=G; G=F; F=E; E=D+T1; D=C; C=B; B=A; A=T1+T2;
  }
  State[0]+=A; State[1]+=B; State[2]+=C; State[3]+=D;
  State[4]+=E; State[5]+=F; State[6]+=G; State[7]+=H;
}

BOOLEAN DarwinKernelSha256 (CONST VOID *Image, UINTN Bytes, UINT8 Digest[32])
{
  UINT32 State[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                     0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  CONST UINT8 *Data = Image;
  UINT8 Tail[64];
  UINTN At, Rest, I;
  UINT64 Bits;
  if (Digest == NULL) { return FALSE; }
  ZeroMem (Digest, 32);
  if ((Data == NULL) || (Bytes == 0) || (Bytes > SIZE_128MB)) { return FALSE; }
  At = 0;
  while (Bytes - At >= 64) { Compress (State, Data + At); At += 64; }
  Rest = Bytes - At;
  ZeroMem (Tail, sizeof (Tail));
  if (Rest != 0) { CopyMem (Tail, Data + At, Rest); }
  Tail[Rest] = 0x80;
  if (Rest >= 56) { Compress (State, Tail); ZeroMem (Tail, sizeof (Tail)); }
  Bits = (UINT64)Bytes * 8;
  for (I = 0; I < 8; I++) { Tail[63-I] = (UINT8)(Bits >> (8*I)); }
  Compress (State, Tail);
  for (I = 0; I < 8; I++) {
    Digest[I*4]=(UINT8)(State[I] >> 24); Digest[I*4+1]=(UINT8)(State[I] >> 16);
    Digest[I*4+2]=(UINT8)(State[I] >> 8); Digest[I*4+3]=(UINT8)State[I];
  }
  return TRUE;
}

EFI_STATUS DarwinMeasureKernel (VOID *Tree, UINTN TreeBytes, UINTN Offset,
                               CONST VOID *Image, UINTN ImageBytes)
{
  UINT8 Digest[32];
  UINT8 *Slot;
  UINTN I;
  if (Offset == MAX_UINTN) { return EFI_SUCCESS; }
  if ((Tree == NULL) || (Offset > TreeBytes) || (32 > TreeBytes - Offset)) { return EFI_COMPROMISED_DATA; }
  Slot = (UINT8 *)Tree + Offset;
  for (I = 0; I < 32; I++) { if (Slot[I] != 0) { return EFI_COMPROMISED_DATA; } }
  if (!DarwinKernelSha256 (Image, ImageBytes, Digest)) { return EFI_BAD_BUFFER_SIZE; }
  CopyMem (Slot, Digest, sizeof (Digest));
  return EFI_SUCCESS;
}
