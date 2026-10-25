## SPDX-License-Identifier: BSD-2-Clause-Patent
[Defines]
  PLATFORM_NAME = DarwinBoot
  PLATFORM_GUID = D94E8E62-812F-44AC-98B2-1BE8E1B2EF86
  PLATFORM_VERSION = 0.1
  DSC_SPECIFICATION = 0x0001001C
  OUTPUT_DIRECTORY = build/DarwinBoot
  SUPPORTED_ARCHITECTURES = AARCH64
  BUILD_TARGETS = DEBUG|RELEASE
  SKUID_IDENTIFIER = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  # Standalone validation runs on QEMU virt. Firmware integrations supply
  # their own SerialPortLib; Dali binds this class to its ramoops writer.
  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf
  PL011UartClockLib|ArmPlatformPkg/Library/PL011UartClockLib/PL011UartClockLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x09000000
  gArmPlatformTokenSpaceGuid.PL011UartClkInHz|24000000

[Components]
  DarwinBootPkg/Driver/DarwinBoot.inf
  Test/Validation.inf
  Test/EntryValidation.inf
  Test/StorageValidation.inf
