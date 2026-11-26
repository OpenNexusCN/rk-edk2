## @file
#  FMX1 Pro common DSC (drivers, libraries, PCDs).
#
#  Copyright (c) 2026, OpenNexusCN contributors
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses.common]
  RockchipBoardLib|Platform/Fmx/Fmx1Pro/Library/Fmx1ProBoardLib/Fmx1ProBoardLib.inf

[PcdsFixedAtBuild.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"rk-edk2 0.0.1"

  # Console: UART2 @ 1500000 8N1
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0xFF1A0000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|1500000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
  # ArmPkg BDS asserts non-zero parity/stop bits. The UART line format is
  # fixed to 8N1 by Dw8250SerialPortLib; these values only feed the console
  # device path.
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1

  # 4GB DRAM, usable up to the 0xF8000000 peripheral window.
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x00000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0xF8000000

  # Firmware volume: loader2 at 8MB. loaderimage --uboot is limited to
  # 0xFF800 bytes, so the FD stays at 0xF0000 (the compressed FV is ~0.5MB).
  gArmTokenSpaceGuid.PcdFdBaseAddress|0x00200000
  gArmTokenSpaceGuid.PcdFdSize|0x000F0000
  gArmTokenSpaceGuid.PcdFvBaseAddress|0x00200000
  gArmTokenSpaceGuid.PcdFvSize|0x000F0000

  # Both ACPI and DT tables are installed; the OS picks.
  gRockchipTokenSpaceGuid.PcdConfigTableMode|2
  gRockchipTokenSpaceGuid.PcdBoardVariant|0

[Components.common]
  # SEC
  ArmPlatformPkg/PeilessSec/PeilessSec.inf

  # DXE core
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  # Architectural protocols
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  Silicon/Rockchip/Drivers/EmuVariableRuntimeDxe/EmuVariableRuntimeDxe.inf
  ArmPkg/Drivers/ArmGicDxe/ArmGicV3Dxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf

  # Console
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  # HII / BDS
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf

  # UEFI Shell
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }
