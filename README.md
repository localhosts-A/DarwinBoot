# DarwinBoot

UEFI bootloader for UniXNU. Loads the payload, kernel, device tree and trust cache, then starts XNU.

Current state: boots from the firmware volume and supports the packed RAM disk handoff.

Built as part of the firmware volume.

Driver entry can run before BDS connects the system consoles. It uses the firmware
debug channel and only prints to ConOut when available. `Test/EntryValidation.c`
loads the real driver with ConOut absent, then with both ConOut and StdErr absent.
