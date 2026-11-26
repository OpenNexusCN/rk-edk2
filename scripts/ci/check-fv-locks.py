#!/usr/bin/env python3
"""Fail if a firmware volume is locked or not writable.

FV attribute bits are not CPU fuses, but this project keeps the firmware volume
writable on purpose: users must always be able to update or repair the board.
"""

import glob
import struct
import sys

EFI_FVB2_READ_DISABLED_CAP = 0x00000001
EFI_FVB2_WRITE_DISABLED_CAP = 0x00000008
EFI_FVB2_WRITE_ENABLED_CAP = 0x00000010
EFI_FVB2_WRITE_STATUS = 0x00000020
EFI_FVB2_LOCK_CAP = 0x00000040
EFI_FVB2_LOCK_STATUS = 0x00000080
EFI_FVB2_READ_LOCK_CAP = 0x00001000
EFI_FVB2_READ_LOCK_STATUS = 0x00002000
EFI_FVB2_WRITE_LOCK_CAP = 0x00004000
EFI_FVB2_WRITE_LOCK_STATUS = 0x00008000

LOCK_MASK = (
    EFI_FVB2_LOCK_CAP
    | EFI_FVB2_LOCK_STATUS
    | EFI_FVB2_READ_LOCK_CAP
    | EFI_FVB2_READ_LOCK_STATUS
    | EFI_FVB2_WRITE_LOCK_CAP
    | EFI_FVB2_WRITE_LOCK_STATUS
)

ATTR_OFFSET = 0x2C  # EFI_FIRMWARE_VOLUME_HEADER.Attributes


def check(path: str) -> bool:
    with open(path, "rb") as f:
        header = f.read(ATTR_OFFSET + 4)
    if len(header) < ATTR_OFFSET + 4:
        print(f"FAIL {path}: truncated FV header")
        return False

    attributes = struct.unpack_from("<I", header, ATTR_OFFSET)[0]
    problems = []

    if attributes & LOCK_MASK:
        problems.append(f"lock bits set: 0x{attributes & LOCK_MASK:08x}")
    if not (attributes & EFI_FVB2_WRITE_ENABLED_CAP):
        problems.append("WRITE_ENABLED_CAP is clear")
    if not (attributes & EFI_FVB2_WRITE_STATUS):
        problems.append("WRITE_STATUS is clear")
    if attributes & EFI_FVB2_WRITE_DISABLED_CAP:
        problems.append("WRITE_DISABLED_CAP is set")
    if attributes & EFI_FVB2_READ_DISABLED_CAP:
        problems.append("READ_DISABLED_CAP is set")

    status = "OK" if not problems else "FAIL"
    print(f"{status} {path}: attributes=0x{attributes:08x}")
    for problem in problems:
        print(f"     - {problem}")
    return not problems


def main(argv: list[str]) -> int:
    paths: list[str] = []
    for arg in argv[1:]:
        paths.extend(sorted(glob.glob(arg)) or [arg])

    if not paths:
        print("usage: check-fv-locks.py <FV file> [...]", file=sys.stderr)
        return 2

    ok = True
    for path in paths:
        try:
            ok = check(path) and ok
        except OSError as exc:
            print(f"FAIL {path}: {exc}")
            ok = False

    if not ok:
        print("Firmware volume must stay writable and non-lockable.")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
