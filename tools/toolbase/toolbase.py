#!/usr/bin/env python3
"""Small native TOOLBASE replacement for simple GodLib asset scripts.

Implemented commands:
  MASKMAKE <src.pi1> <dst.pi1>
  BSBMAKE  <script.scr>
  PALMAKE  <src.pi1> <dst.pal>
  MAKELINK <src_dir> <dst.lnk>
  GODPACK  <src> [dst]

Implemented BSBMAKE chunks:
  [ HEADER ], [ SPRITE ], [ FONT ]

The binary output is written in GodLib's delocated big-endian layouts.
Rotation and GUIBUILD are intentionally not implemented here.
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


DEGAS_HEADER_SIZE = 34
DEGAS_SCREEN_SIZE = 32000
SPRITE_STRUCT_SIZE = 16
SPRITE_BLOCK_HEADER_SIZE = 8
FONT_STRUCT_SIZE = 32
SPRITE_REGION_SIZE = 8
LINKFILE_STRUCT_SIZE = 28
LINKFILE_FOLDER_SIZE = 16
LINKFILE_FILE_SIZE = 40
ASSET_ITEM_SIZE = 20


def u16be(value: int) -> bytes:
    return struct.pack(">H", value & 0xFFFF)


def u32be(value: int) -> bytes:
    return struct.pack(">I", value & 0xFFFFFFFF)


def norm_path(path: str) -> str:
    return path.strip().strip('"').replace("\\", os.sep)


def resolve(base: Path, path: str) -> Path:
    p = Path(norm_path(path))
    if p.is_absolute():
        return p
    return base / p


def read_pi1(path: Path) -> tuple[bytes, bytes]:
    data = path.read_bytes()
    if len(data) < DEGAS_HEADER_SIZE + DEGAS_SCREEN_SIZE:
        raise ValueError(f"{path}: file too small for PI1")
    return data[:DEGAS_HEADER_SIZE], data[DEGAS_HEADER_SIZE:DEGAS_HEADER_SIZE + DEGAS_SCREEN_SIZE]


def maskmake(src: Path, dst: Path) -> None:
    header, pixels = read_pi1(src)
    out = bytearray(DEGAS_SCREEN_SIZE)
    for y in range(200):
        row = y * 160
        for xw in range(20):
            off = row + xw * 8
            words = struct.unpack_from(">4H", pixels, off)
            mask = (~(words[0] | words[1] | words[2] | words[3])) & 0xFFFF
            struct.pack_into(">4H", out, off, mask, mask, mask, mask)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(header + out + bytes(32))


def palmake(src: Path, dst: Path) -> None:
    data = src.read_bytes()
    if len(data) < DEGAS_HEADER_SIZE:
        raise ValueError(f"{src}: file too small for PI1 palette")
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data[2:34])


def first_word(line: str) -> str:
    return line.strip().split(None, 1)[0] if line.strip() else ""


def assigned(line: str) -> str:
    if "=" not in line:
        return ""
    return line.split("=", 1)[1].strip()


def clean_lines(path: Path) -> list[str]:
    lines: list[str] = []
    for raw in path.read_text(errors="replace").splitlines():
        stripped = raw.strip()
        if not stripped or stripped.startswith("#"):
            continue
        lines.append(stripped)
    return lines


@dataclass
class SpriteDef:
    src: str = ""
    msk: str = ""
    dst: str = ""
    x: int = 0
    y: int = 0
    stepx: int = 0
    stepy: int = 0
    width: int = 0
    height: int = 0
    planes: int = 4
    canvaswidth: int = 320
    framecount: int = 1
    opaque: int = 0
    font: bool = False
    charfirst: int = 0
    charlast: int = 0
    spacewidth: int = 4
    charmap: list[int] = field(default_factory=list)


@dataclass
class HeaderDef:
    gfxpath: str = ""
    mskpath: str = ""
    dstpath: str = ""


@dataclass
class SpriteBytes:
    width: int
    height: int
    gfx_planes: int
    mask_planes: int
    mask: bytes
    gfx: bytes

    @property
    def size(self) -> int:
        return SPRITE_STRUCT_SIZE + len(self.mask) + len(self.gfx)


def parse_scr(path: Path) -> tuple[HeaderDef, list[SpriteDef]]:
    header = HeaderDef()
    sprites: list[SpriteDef] = []
    lines = clean_lines(path)
    i = 0
    while i < len(lines):
        line = lines[i]
        if line not in ("[ HEADER ]", "[ SPRITE ]", "[ FONT ]"):
            i += 1
            continue
        kind = line
        i += 1
        if i >= len(lines) or lines[i] != "{":
            raise ValueError(f"{path}: expected '{{' after {kind}")
        i += 1
        block: list[str] = []
        while i < len(lines) and lines[i] != "}":
            block.append(lines[i])
            i += 1
        if i >= len(lines):
            raise ValueError(f"{path}: missing '}}' for {kind}")
        i += 1

        if kind == "[ HEADER ]":
            for item in block:
                key = first_word(item).lower()
                val = assigned(item)
                if key == "gfxpath":
                    header.gfxpath = val
                elif key == "mskpath":
                    header.mskpath = val
                elif key == "dstpath":
                    header.dstpath = val
            continue

        spr = SpriteDef(font=(kind == "[ FONT ]"))
        for item in block:
            key = first_word(item).lower()
            val = assigned(item)
            if key in ("src", "msk", "dst"):
                setattr(spr, key, val)
            elif key == "charfirst":
                spr.charfirst = ord(val.strip()[0])
                spr.charmap = build_charmap(spr.charfirst, spr.charlast)
            elif key == "charlast":
                spr.charlast = ord(val.strip()[0])
                spr.charmap = build_charmap(spr.charfirst, spr.charlast)
            elif key == "charfirstvalue":
                spr.charfirst = int(val, 0) & 0xFF
                spr.charmap = build_charmap(spr.charfirst, spr.charlast)
            elif key == "charlastvalue":
                spr.charlast = int(val, 0) & 0xFF
                spr.charmap = build_charmap(spr.charfirst, spr.charlast)
            elif key == "charmap":
                spr.charmap = parse_charmap(val)
            elif key == "spacewidth":
                spr.spacewidth = int(val, 0)
            elif key in {
                "x", "y", "stepx", "stepy", "width", "height", "planes",
                "canvaswidth", "framecount", "opaque",
            }:
                setattr(spr, key, int(val, 0))
            elif key.startswith("rot"):
                raise NotImplementedError("rotated sprites are not implemented")
        sprites.append(spr)
    return header, sprites


def build_charmap(first: int, last: int) -> list[int]:
    cmap = [0xFF] * 256
    if last >= first:
        for idx, ch in enumerate(range(first, last + 1)):
            cmap[ch] = idx & 0xFF
    return cmap


def parse_charmap(value: str) -> list[int]:
    cmap = [0xFF] * 256
    text = value.strip().strip('"')
    for idx, ch in enumerate(text[:256]):
        cmap[ord(ch) & 0xFF] = idx & 0xFF
    return cmap


def make_sprite(gfx_pixels: bytes, mask_pixels: bytes, spr: SpriteDef, frame: int) -> SpriteBytes:
    x = spr.x + frame * spr.stepx
    y = spr.y
    if spr.canvaswidth:
        while x + spr.width > spr.x + spr.canvaswidth:
            x -= spr.canvaswidth
            y += spr.stepy
    xwords = (spr.width + 15) >> 4
    src_xword = x >> 4
    mask_out = bytearray()
    gfx_out = bytearray()

    for row in range(spr.height):
        base = (y + row) * 160 + src_xword * 8
        for col in range(xwords):
            off = base + col * 8
            if spr.opaque:
                mask = 0
            else:
                words = struct.unpack_from(">4H", mask_pixels, off)
                mask = (words[0] | words[1] | words[2] | words[3]) & 0xFFFF
            mask_out += u16be(mask)

    for row in range(spr.height):
        base = (y + row) * 160 + src_xword * 8
        for col in range(xwords):
            off = base + col * 8
            words = struct.unpack_from(">4H", gfx_pixels, off)
            for plane in range(spr.planes):
                gfx_out += u16be(words[plane])

    return SpriteBytes(spr.width, spr.height, spr.planes, 4 if spr.msk else 0, bytes(mask_out), bytes(gfx_out))


def serialize_sprite(sprite: SpriteBytes, sprite_offset: int) -> bytes:
    mask_offset = SPRITE_STRUCT_SIZE
    gfx_offset = SPRITE_STRUCT_SIZE + len(sprite.mask)
    return b"".join(
        [
            u32be(gfx_offset),
            u32be(mask_offset),
            u16be(sprite.width),
            u16be(sprite.height),
            u16be(sprite.gfx_planes),
            u16be(sprite.mask_planes),
            sprite.mask,
            sprite.gfx,
        ]
    )


def serialize_bsb(sprites: list[SpriteBytes]) -> bytes:
    count = len(sprites)
    header_size = SPRITE_BLOCK_HEADER_SIZE + count * 4
    offsets: list[int] = []
    pos = header_size
    parts: list[bytes] = []
    for sprite in sprites:
        offsets.append(pos)
        data = serialize_sprite(sprite, pos)
        parts.append(data)
        pos += len(data)
    out = bytearray()
    out += b"BSBK"
    out += u16be(0)
    out += u16be(count)
    for off in offsets:
        out += u32be(off)
    for part in parts:
        out += part
    return bytes(out)


def sprite_region(sprite: SpriteBytes, fixed_width: int) -> tuple[int, int, int, int]:
    if fixed_width:
        return 0, 0, fixed_width - 1, sprite.height - 1

    xwords = (sprite.width + 15) >> 4
    x0 = sprite.width
    x1 = 0
    y0 = sprite.height
    y1 = 0
    found = False
    for y in range(sprite.height):
        for x in range(sprite.width):
            word_idx = y * xwords + (x >> 4)
            word = struct.unpack_from(">H", sprite.mask, word_idx * 2)[0]
            if (word & (0x8000 >> (x & 15))) == 0:
                found = True
                x0 = min(x0, x)
                x1 = max(x1, x)
                y0 = min(y0, y)
                y1 = max(y1, y)
    if not found:
        return 0, 0, 0, 0
    return x0, y0, x1, y1


def serialize_bfb(block_sprites: list[SpriteBytes], spr: SpriteDef) -> bytes:
    cmap = spr.charmap or build_charmap(spr.charfirst, spr.charlast)
    used_flags = [False] * 256
    char_map = [0xFF] * 256
    for ch, sprite_idx in enumerate(cmap):
        if sprite_idx < len(block_sprites):
            char_map[ch] = sprite_idx
            used_flags[sprite_idx] = True

    used_indices = [idx for idx, used in enumerate(used_flags[:len(block_sprites)]) if used]
    index_remap = {old: new for new, old in enumerate(used_indices)}
    font_sprites = [block_sprites[idx] for idx in used_indices]
    char_first = 0xFF
    char_last = 0
    for ch, old_idx in enumerate(char_map):
        if old_idx in index_remap:
            char_first = min(char_first, ch)
            char_last = max(char_last, ch)
    if char_first == 0xFF:
        char_first = 0
        char_last = 0

    remapped = bytearray()
    for ch in range(char_first, char_last + 1):
        remapped.append(index_remap.get(char_map[ch], 0xFF))

    sprite_table_off = FONT_STRUCT_SIZE
    regions_off = sprite_table_off + len(font_sprites) * SPRITE_STRUCT_SIZE
    data_off = regions_off + len(font_sprites) * SPRITE_REGION_SIZE
    pos = data_off
    sprite_parts: list[bytes] = []
    for sprite in font_sprites:
        sprite_off = sprite_table_off + len(sprite_parts) * SPRITE_STRUCT_SIZE
        mask_off = pos
        gfx_off = mask_off + len(sprite.mask)
        sprite_parts.append(sprite.mask + sprite.gfx)
        pos += len(sprite.mask) + len(sprite.gfx)
    charmap_off = pos

    regions = [sprite_region(sprite, 0) for sprite in font_sprites]
    width_max = max((r[2] - r[0] for r in regions), default=0)
    height_max = max((r[3] - r[1] for r in regions), default=0)

    out = bytearray()
    out += b"TNOF"
    out += u32be(0)
    out += bytes([char_first, char_last, 2, spr.spacewidth & 0xFF])
    out += u16be(width_max)
    out += u16be(height_max)
    out += u16be(len(font_sprites))
    out += u16be(0)
    out += u32be(charmap_off)
    out += u32be(sprite_table_off)
    out += u32be(regions_off)

    for idx, sprite in enumerate(font_sprites):
        sprite_off = sprite_table_off + idx * SPRITE_STRUCT_SIZE
        sprite_off = sprite_table_off + idx * SPRITE_STRUCT_SIZE
        mask_abs = data_off + sum(len(s.mask) + len(s.gfx) for s in font_sprites[:idx])
        gfx_abs = mask_abs + len(sprite.mask)
        gfx_off = gfx_abs - sprite_off
        mask_off = mask_abs - sprite_off
        out += u32be(gfx_off)
        out += u32be(mask_off)
        out += u16be(sprite.width)
        out += u16be(sprite.height)
        out += u16be(sprite.gfx_planes)
        out += u16be(sprite.mask_planes)

    for x0, y0, x1, y1 in regions:
        out += u16be(x0)
        out += u16be(y0)
        out += u16be(x1)
        out += u16be(y1)

    for part in sprite_parts:
        out += part
    out += remapped
    return bytes(out)


def asset_hash(text: str, max_len: int) -> int:
    h = 0
    for ch in text[:max_len]:
        if ch == "\0":
            break
        h = ((h << 4) + ord(ch)) & 0xFFFFFFFF
        temp = h & 0xF0000000
        if temp:
            h ^= temp >> 24
        h &= (~temp) & 0xFFFFFFFF
    return h & 0xFFFFFFFF


def packed_info(data: bytes) -> tuple[int, int]:
    if len(data) >= 18 and data[:3] == b"RNC" and data[3] in (1, 2):
        return 1, int.from_bytes(data[4:8], "big")
    if len(data) >= 20 and data[:4] == b"GDPK":
        return 1, int.from_bytes(data[12:16], "big")
    return 0, len(data)


@dataclass
class LinkFileEntry:
    name: str
    path: Path
    size: int = 0
    unpacked_size: int = 0
    packed_flag: int = 0
    offset: int = 0
    name_off: int = 0


def align4(value: int) -> int:
    return (value + 3) & ~3


def linkfile_sort_key(name: str) -> str:
    return name.lower().replace("_", "{")


def makelink(src_dir: Path, dst_file: Path) -> None:
    if not src_dir.is_dir():
        raise ValueError(f"{src_dir}: not a directory")

    entries: list[LinkFileEntry] = []
    for item in os.scandir(src_dir):
        if item.name.startswith(".") or not item.is_file():
            continue
        size = item.stat().st_size
        if size == 0:
            continue
        entries.append(LinkFileEntry(item.name, Path(item.path)))
    entries.sort(key=lambda entry: linkfile_sort_key(entry.name))

    folder_count = 1
    file_count = len(entries)
    string_size = 1 + sum(len(e.name.encode("latin-1")) + 1 for e in entries)
    fat_size = align4(LINKFILE_STRUCT_SIZE + folder_count * LINKFILE_FOLDER_SIZE + file_count * LINKFILE_FILE_SIZE + string_size)
    files_off = LINKFILE_STRUCT_SIZE + LINKFILE_FOLDER_SIZE
    strings_off = files_off + file_count * LINKFILE_FILE_SIZE

    cursor = strings_off
    root_name_off = cursor
    cursor += 1
    for entry in entries:
        entry.name_off = cursor
        cursor += len(entry.name.encode("latin-1")) + 1

    data_offset = fat_size
    for entry in entries:
        data = entry.path.read_bytes()
        entry.size = len(data)
        entry.packed_flag, entry.unpacked_size = packed_info(data)
        entry.offset = data_offset
        data_offset += align4(entry.size)

    out = bytearray(fat_size)
    out[0:4] = u32be(0x12345678)
    out[4:8] = u32be(0x0A)
    out[8:12] = u32be(fat_size)
    out[12:16] = u32be(file_count)
    out[16:18] = u16be(0)
    out[18:20] = u16be(folder_count)
    out[20:24] = u32be(0)
    out[24:28] = u32be(LINKFILE_STRUCT_SIZE)

    folder_off = LINKFILE_STRUCT_SIZE
    out[folder_off:folder_off + 2] = u16be(file_count)
    out[folder_off + 2:folder_off + 4] = u16be(0)
    out[folder_off + 4:folder_off + 8] = u32be(root_name_off)
    out[folder_off + 8:folder_off + 12] = u32be(files_off)
    out[folder_off + 12:folder_off + 16] = u32be(files_off)

    for idx, entry in enumerate(entries):
        off = files_off + idx * LINKFILE_FILE_SIZE
        ext = ""
        if "." in entry.name:
            ext = entry.name.split(".", 1)[1]
        out[off:off + 4] = u32be(0)  # mAsset.mpData
        out[off + 4:off + 8] = u32be(entry.size)
        out[off + 8:off + 12] = u32be(asset_hash(entry.name, len(entry.name)))
        out[off + 12:off + 16] = u32be(asset_hash(ext, 4))
        out[off + 16:off + 20] = u32be(0)  # status
        out[off + 20:off + 24] = u32be(entry.unpacked_size)
        out[off + 24:off + 28] = u32be(entry.offset)
        out[off + 28:off + 30] = u16be(entry.packed_flag)
        out[off + 30:off + 32] = u16be(0)
        out[off + 32:off + 36] = u32be(0)
        out[off + 36:off + 40] = u32be(entry.name_off)

    out[root_name_off] = 0
    for entry in entries:
        raw = entry.name.encode("latin-1")
        out[entry.name_off:entry.name_off + len(raw)] = raw
        out[entry.name_off + len(raw)] = 0

    dst_file.parent.mkdir(parents=True, exist_ok=True)
    with dst_file.open("wb") as fh:
        fh.write(out)
        for entry in entries:
            data = entry.path.read_bytes()
            fh.write(data)
            pad = align4(len(data)) - len(data)
            if pad:
                fh.write(bytes(pad))


def find_godpack_tool() -> Path:
    env_tool = os.environ.get("GODPACK")
    if env_tool:
        return Path(env_tool)

    godlib_dir = Path(__file__).resolve().parents[2]
    system = platform.system()
    machine = platform.machine()
    for candidate in (
        godlib_dir / "bin" / system / machine / "godpack",
        godlib_dir / "bin" / system / "x86_64" / "godpack",
    ):
        if candidate.exists():
            return candidate

    found = shutil.which("godpack")
    if found:
        return Path(found)

    raise FileNotFoundError("GODPACK tool not found; build godlib/tools/godpack first")


def godpack(src: Path, dst: Path) -> None:
    tool = find_godpack_tool()
    dst.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(tool), str(src), str(dst)], check=True)


def bsbmake(script: Path) -> None:
    header, defs = parse_scr(script)
    base = script.parent
    gfx_base = resolve(base, header.gfxpath) if header.gfxpath else base
    msk_base = resolve(base, header.mskpath) if header.mskpath else base
    dst_base = resolve(base, header.dstpath) if header.dstpath else base

    for spr in defs:
        gfx_path = resolve(gfx_base, spr.src)
        msk_path = resolve(msk_base, spr.msk) if spr.msk else gfx_path
        dst_path = resolve(dst_base, spr.dst)
        _, gfx_pixels = read_pi1(gfx_path)
        _, mask_pixels = read_pi1(msk_path)
        frames = [make_sprite(gfx_pixels, mask_pixels, spr, frame) for frame in range(spr.framecount)]
        data = serialize_bfb(frames, spr) if spr.font else serialize_bsb(frames)
        dst_path.parent.mkdir(parents=True, exist_ok=True)
        dst_path.write_bytes(data)
        kind = "FONT" if spr.font else "SPRITE"
        print(f" {kind} : {spr.dst}")


def run_tbs(path: Path) -> None:
    base = path.parent
    for line in clean_lines(path):
        parts = line.split()
        if not parts:
            continue
        cmd = parts[0].upper()
        args = parts[1:]
        if cmd == "MASKMAKE":
            if len(args) != 2:
                raise ValueError(f"{path}: MASKMAKE needs 2 args")
            print(f"<MASKMAKE> {args[0]} {args[1]}")
            maskmake(resolve(base, args[0]), resolve(base, args[1]))
        elif cmd == "BSBMAKE":
            if len(args) != 1:
                raise ValueError(f"{path}: BSBMAKE needs 1 arg")
            print(f"<BSBMAKE> {args[0]}")
            bsbmake(resolve(base, args[0]))
        elif cmd == "PALMAKE":
            if len(args) != 2:
                raise ValueError(f"{path}: PALMAKE needs 2 args")
            print(f"<PALMAKE> {args[0]} {args[1]}")
            palmake(resolve(base, args[0]), resolve(base, args[1]))
        elif cmd == "MAKELINK":
            if len(args) != 2:
                raise ValueError(f"{path}: MAKELINK needs 2 args")
            print(f"<MAKELINK> {args[0]} {args[1]}")
            makelink(resolve(base, args[0]), resolve(base, args[1]))
        elif cmd == "GODPACK":
            if len(args) not in (1, 2):
                raise ValueError(f"{path}: GODPACK needs 1 or 2 args")
            print(f"<GODPACK> {args[0]}" + (f" {args[1]}" if len(args) == 2 else ""), flush=True)
            src = resolve(base, args[0])
            dst = resolve(base, args[1]) if len(args) == 2 else src
            godpack(src, dst)
        elif cmd == "GUIBUILD":
            raise NotImplementedError("GUIBUILD is intentionally not implemented")
        else:
            raise NotImplementedError(f"unknown TOOLBASE command: {cmd}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Native Python subset of RG TOOLBASE")
    parser.add_argument("script", type=Path, help=".tbs script to run")
    ns = parser.parse_args(argv)
    print("< PY TOOLBASE >")
    run_tbs(ns.script)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
