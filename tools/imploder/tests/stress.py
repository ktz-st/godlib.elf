"""Deterministic edge cases for grouped 68000 Imploder bit reads."""
import random


def fixtures():
    rng = random.Random(0xC0DEC0DE)
    cases = [bytes(150000) + bytes(rng.randrange(256) for _ in range(70001))]
    distant = bytes(rng.randrange(256) for _ in range(66000))
    cases += [distant + distant[:40000] + b'END', bytes(range(251)) * 300,
              bytes(100000), b'AB' * 50001, b'ABC' * 33335, b'ABCD' * 25001]
    for _ in range(35):
        size = rng.choice([4095, 8192, 16385, 65535, 65536, 65537, 131073])
        block = bytes(rng.randrange(rng.choice([2, 8, 16, 256]))
                      for _ in range(rng.randrange(3, 1024)))
        data = bytearray((block * ((size + len(block) - 1) // len(block)))[:size])
        for _ in range(size // 1000):
            data[rng.randrange(size)] = rng.randrange(256)
        cases.append(bytes(data))
    for i, data in enumerate(cases):
        for mode in ([0, 4, 7, 11] if i < 7 else [i % 12]):
            yield mode, data
