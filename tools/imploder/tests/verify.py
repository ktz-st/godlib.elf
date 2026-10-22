#!/usr/bin/env python3
"""Host round trips; optionally retain fixtures for native.c under Hatari."""
import argparse
import random
import struct
import shutil
import sys
import subprocess
import tempfile
from pathlib import Path


def verify(tool, root, micro=None):
    rng = random.Random(0x494D5021)
    datasets = [bytes(4096), bytes(range(64)) * 16,
                (b'Imploder / GodLib / Atari ST!\x00' * 150)[:4095],
                bytes(rng.randrange(8) for _ in range(8193)),
                bytes((x // 160 + x % 17) & 255 for x in range(32034)),
                (bytes(range(64)) * 1025)[:65537]]
    count = 0
    for mode in range(12):
        for data in datasets:
            name = f'f{count:03}'
            raw, packed, output = [root / (name + ext) for ext in ('.raw', '.imp', '.out')]
            raw.write_bytes(data)
            subprocess.run([tool, f'-{mode}', str(raw), str(packed)], check=True, capture_output=True)
            blob = packed.read_bytes()
            magic, size, end = struct.unpack_from('>4sII', blob)
            assert magic == b'IMP!' and size == len(data) and end + 50 == len(blob)
            assert struct.unpack_from('>I', blob, len(blob)-4)[0] == 7 + sum(struct.unpack('>' + 'H'*((len(blob)-4)//2), blob[:-4]))
            subprocess.run([tool, str(packed), str(output)], check=True, capture_output=True)
            assert output.read_bytes() == data, (mode, name)
            output.unlink()
            count += 1
    for i, data in enumerate([b'', b'x', b'abc', b'123456789012', bytes(rng.randrange(256) for _ in range(1024))]):
        raw, packed = root / 'small.raw', root / 'small.imp'
        raw.write_bytes(data)
        assert subprocess.run([tool, str(raw), str(packed)], capture_output=True).returncode
        assert not packed.exists()
    packed = root / 'f000.imp'
    for blob in [packed.read_bytes()[:-1], packed.read_bytes()[:12],
                 packed.read_bytes()[:-4] + b'\x00'*4]:
        bad = root / 'bad.imp'
        bad.write_bytes(blob)
        assert subprocess.run([tool, str(bad), str(root/'bad.out')], capture_output=True).returncode
    for args in [['-11'], ['-foo', str(packed)], ['-12', str(packed)]]:
        assert subprocess.run([tool, *args], capture_output=True).returncode
    for name in ['small.raw', 'bad.imp']:
        (root/name).unlink(missing_ok=True)
    if micro:
        for source in sorted(micro.rglob('*.imp')):
            packed, output = root / f'f{count:03}.imp', root / f'f{count:03}.raw'
            shutil.copyfile(source, packed)
            subprocess.run([tool, str(packed), str(output)], check=True, capture_output=True)
            count += 1
    sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'toolbase'))
    import toolbase
    loose = root / 'loose'
    loose.mkdir(exist_ok=True)
    shutil.copyfile(root/'f000.raw', loose/'PAYLOAD.BIN')
    toolbase.makelink(loose, root/'SCENE.LNK')
    subprocess.run([tool, str(root/'SCENE.LNK'), str(root/'SCENE.IMP')], check=True, capture_output=True)
    print(f'PASS: {count} round trips, modes 0..11, sizes 1024..65537; invalid inputs rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('tool')
    parser.add_argument('--target-dir', type=Path)
    parser.add_argument('--micro-root', type=Path, help='optional original MICRO IMP! fixture tree')
    args = parser.parse_args()
    if args.target_dir:
        args.target_dir.mkdir(parents=True, exist_ok=True)
        verify(str(Path(args.tool).resolve()), args.target_dir.resolve(), args.micro_root)
    else:
        with tempfile.TemporaryDirectory(prefix='godlib-imploder-') as tmp:
            verify(str(Path(args.tool).resolve()), Path(tmp), args.micro_root)
