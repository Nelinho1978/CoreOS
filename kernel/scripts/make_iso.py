#!/usr/bin/env python3
"""
Gera coreos.iso bootavel (El Torito) a partir de coreos.img.
Compativel com VirtualBox (tamanho multiplo de 2048 bytes).
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

SECTOR = 2048
PVD_SECTOR = 16
BOOT_IMG_SECTORS_512 = 64  # MBR + kernel (evita El Torito rejeitar imagem 1.44 MB inteira)


def le32(value: int) -> bytes:
    return struct.pack("<I", value)


def be32(value: int) -> bytes:
    return struct.pack(">I", value)


def dual32(value: int) -> bytes:
    return le32(value) + be32(value)


def pad_sector(data: bytes) -> bytes:
    if len(data) > SECTOR:
        raise ValueError("sector overflow")
    return data + b"\x00" * (SECTOR - len(data))


def dir_record(name: str, extent: int, size: int, is_dir: bool = False) -> bytes:
    flags = 0x02 if is_dir else 0x00
    ident = b"\x00" if is_dir else name.encode("ascii").upper() + b";1"

    rec_len = 37 + len(ident)
    if rec_len % 2:
        rec_len += 1

    rec = bytearray(rec_len)
    rec[0] = rec_len
    rec[1] = 0
    rec[2:10] = dual32(extent)
    rec[10:18] = dual32(size)
    rec[25] = flags
    rec[28:36] = dual32(1)
    rec[36] = len(ident)
    rec[37 : 37 + len(ident)] = ident
    return bytes(rec)


def make_pvd(volume_sectors: int, root_extent: int, root_size: int, path_table_l: int) -> bytes:
    v = bytearray(SECTOR)
    v[0] = 1
    v[1:6] = b"CD001"
    v[6] = 1
    v[8:40] = b"COREOS10".ljust(32)
    v[40:72] = b"COREOS10".ljust(32)
    v[80:88] = dual32(volume_sectors)
    v[120:128] = dual32(1)
    v[124:132] = dual32(1)
    v[128:132] = le32(SECTOR)
    v[132:136] = le32(path_table_l)
    v[140:144] = le32(path_table_l)
    v[156 : 156 + len(dir_record("\x00", root_extent, root_size, True))] = dir_record(
        "\x00", root_extent, root_size, True
    )
    v[813] = 1
    v[814:830] = b"COREOS10".ljust(16)
    v[830:846] = b"COREOS10".ljust(16)
    return bytes(v)


def make_boot_record(catalog_sector: int) -> bytes:
    v = bytearray(SECTOR)
    v[0] = 0
    v[1:6] = b"CD001"
    v[6] = 1
    v[7:39] = b"EL TORITO SPECIFICATION".ljust(32)
    v[71:75] = le32(catalog_sector)
    return bytes(v)


def make_terminator() -> bytes:
    v = bytearray(SECTOR)
    v[0] = 255
    v[1:6] = b"CD001"
    v[6] = 1
    return bytes(v)


def make_path_table_l(root_extent: int) -> bytes:
    entry = bytearray(10)
    entry[0] = 1
    entry[2:4] = struct.pack("<H", root_extent)
    entry[6:8] = struct.pack("<H", 1)
    entry[8] = 0
    return pad_sector(bytes(entry))


def make_boot_catalog(boot_extent: int, boot_sectors_512: int) -> bytes:
    cat = bytearray(SECTOR)
    cat[0] = 0x01
    cat[1] = 0x00
    cat[30] = 0x55
    cat[31] = 0xAA

    cat[32] = 0x90
    cat[33] = 0x00
    cat[34:36] = struct.pack("<H", 1)
    cat[36:64] = b"CoreOS 10".ljust(28)

    cat[64] = 0x88
    cat[65] = 0x00
    cat[66:68] = struct.pack("<H", 0)
    cat[68] = 0x00
    cat[69] = 0x00
    cat[70:72] = struct.pack("<H", boot_sectors_512)
    cat[72:76] = le32(boot_extent)

    words = struct.unpack("<8H", cat[0:16])
    cat[4:6] = struct.pack("<H", (0x10000 - (sum(words) & 0xFFFF)) & 0xFFFF)
    return bytes(cat)


def patch_boot_info_table(
    boot_img: bytearray, pvd_sector: int, boot_extent: int, boot_size: int, catalog_sector: int
) -> None:
    """Grava Boot Info Table nos bytes 8-63 do setor 0 (area reservada no mbr.S)."""
    if len(boot_img) < 64:
        boot_img.extend(b"\x00" * (64 - len(boot_img)))
    struct.pack_into("<I", boot_img, 8, pvd_sector)
    struct.pack_into("<I", boot_img, 12, boot_extent)
    struct.pack_into("<I", boot_img, 16, boot_size)
    struct.pack_into("<I", boot_img, 20, catalog_sector)
    struct.pack_into("<I", boot_img, 24, 1)
    struct.pack_into("<I", boot_img, 28, 0)


def build_iso(img_path: Path, iso_path: Path) -> None:
    full_img = bytearray(img_path.read_bytes())
    boot_size = min(len(full_img), BOOT_IMG_SECTORS_512 * 512)
    boot_data = bytearray(full_img[:boot_size])
    boot_sectors_512 = (boot_size + 511) // 512

    catalog_sector = 21
    boot_extent = 22

    patch_boot_info_table(boot_data, PVD_SECTOR, boot_extent, boot_size, catalog_sector)

    boot_padded = boot_data + b"\x00" * (-len(boot_data) % SECTOR)
    boot_sector_count = len(boot_padded) // SECTOR

    root_record = dir_record("COREOS.IMG", boot_extent, boot_size, is_dir=False)
    root_size = len(root_record)
    root_extent = boot_extent + boot_sector_count
    path_table_l = 19
    volume_sectors = root_extent + 1

    parts = [
        b"\x00" * (SECTOR * 16),
        make_pvd(volume_sectors, root_extent, root_size, path_table_l),
        make_boot_record(catalog_sector),
        make_terminator(),
        make_path_table_l(root_extent),
        pad_sector(b""),
        make_boot_catalog(boot_extent, boot_sectors_512),
        boot_padded,
        pad_sector(root_record),
    ]

    iso = b"".join(parts)
    if len(iso) % SECTOR:
        iso += b"\x00" * (SECTOR - (len(iso) % SECTOR))

    iso_path.write_bytes(iso)
    print(f"ISO gerada: {iso_path} ({len(iso)} bytes, {len(iso) // SECTOR} setores)")


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Uso: {sys.argv[0]} <coreos.img> <coreos.iso>", file=sys.stderr)
        return 1

    img = Path(sys.argv[1])
    iso = Path(sys.argv[2])
    if not img.is_file():
        print(f"ERRO: arquivo nao encontrado: {img}", file=sys.stderr)
        return 1

    build_iso(img, iso)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
