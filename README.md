# DarwinBoot

UEFI bootloader for UniXNU. Loads the payload, kernel, device tree and trust cache, then starts XNU.

Current state: boots from the firmware volume and supports the packed RAM disk handoff.

Built as part of the firmware volume.

Driver entry can run before BDS connects the system consoles. It uses the firmware
debug channel and only prints to ConOut when available. `Test/EntryValidation.c`
loads the real driver with ConOut absent, then with both ConOut and StdErr absent.

DarwinBoot optionally consumes `UNIXNU_STORAGE_QUIESCE_PROTOCOL` from UniXNUPkg.
After closing the boot volume and before its final memory map, it invokes all
providers. Each provider must close submissions, complete outstanding work,
synchronize writes, stop hardware and revoke firmware DMA. Only exact
`EFI_SUCCESS` permits kernel entry; warnings and failures refuse it. With no
provider, the normal RAM-only boot path is preserved. EBS retries refresh the map without
reopening storage or repeating callbacks. Platform providers must be idempotent
and the kernel must allocate its own queues and mappings.

The standalone `DarwinBoot.dsc` also builds `Test/StorageValidation.inf` for the
explicit QEMU `--storage-handoff` test. That fixture refuses one valid boot with
a timeout, then seals a test provider and forces one EBS retry before the real
firmware exit and synthetic Mach-O entry. Dali's DSC and DXE FDF list include
only `DarwinBootPkg/Driver/DarwinBoot.inf`; the fixture is not included there.
See UniXNU's `docs/storage-handoff.md` for commands and the qualification boundary.

An optional `/chosen/darwinboot-kernel-sha256` property is a measurement slot,
not a host-provided digest. The complete ADT validator requires a unique
32-byte all-zero value directly under `/chosen`; prefilled, duplicate,
misplaced and malformed values refuse boot. Before allocation or relocation,
DarwinBoot hashes the complete original kernel file (including bytes outside
loaded segments) and fills the slot. The owned handoff ADT receives that value.
Missing slots preserve legacy RAM boots; consumers that require a measurement
must reject absence. SHA-256 is linked directly from `KernelMeasurement.c`,
adapted from UniXNU's MIT NativeAdreno SHA implementation, without a selectable
Null library. This is a loader observation, not a signature or source-build
attestation. A platform authorizing writes must also bind its actual loader
binary and source pins, and independently validate device/layout/range policy.

The synthetic QEMU kernel compares the post-EBS slot with an independent test
digest in its ADT. Its original file includes an unaligned trailer outside all
loaded segments. Validation covers seven bad measurement slots in addition to
the existing 33 rejected requests; the storage fixture also forces an EBS retry.
