#!/usr/bin/env python3
"""Compile the GodLib cutscene DSL to the Atari CUT v3 layout (no dependencies)."""
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
import math
from pathlib import Path
import re
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
# Explicit Atari ABI: 32-bit pointers, two-byte structure alignment. Runtime
# client/progress storage stays zero. Verified by the 68000 compatibility test.
KINDS = ('ANIMATION', 'ASSET', 'PAGE', 'SAMPLE', 'SCRIPT', 'SPRITE', 'TEXT', 'VAR')
SIZES = dict(zip(KINDS, (14, 64, 8, 12, 10, 76, 66, 36)))
FIELDS = {
 'ANIMATION': {'SPEED': (4,'FIX16_16'), 'FRAME0': (8,'S16'), 'FRAME1': (10,'S16'), 'FLAG_PINGPONG': (12,'U8'), 'FLAG_LOOP': (13,'U8')},
 'ASSET': {'FILENAME': (4,'STRING'), 'CONTEXT': (8,'STRING')},
 'PAGE': {}, 'SCRIPT': {},
 'SAMPLE': {'ASSET': (4,'ASSET'), 'VOLUME': (8,'U8'), 'STEROPOS': (9,'U8'), 'FLAG_LOOP': (10,'U8')},
 'SPRITE': {'ASSET': (4,'ASSET'), 'ANIMATION': (8,'ANIMATION'), 'SPEED': (12,'FIX16_16'), 'X': (16,'S16'), 'Y': (18,'S16')},
 'TEXT': {'ASSET': (4,'ASSET'), 'X': (8,'S16'), 'Y': (10,'S16'), 'WIDTH': (12,'S16'), 'HEIGHT': (14,'S16'), 'PAGE': (16,'PAGE')},
 'VAR': {'VARNAME': (4,'STRING')},
}

class CompileError(ValueError):
 pass

@dataclass
class Chunk:
 kind: str
 name: str
 line: int
 fields: dict = field(default_factory=dict)
 commands: list = field(default_factory=list)
 page: str = ''
 offset: int = 0


def command_definitions():
 """Use the runtime's opcode enum and argument table, not a second opcode list."""
 header = (ROOT/'cutscene/cutscene.h').read_text()
 enum = re.search(r'enum\s*\{(.*?)eCUT_CMD_LIMIT', header, re.S).group(1)
 ids = {name: i for i,name in enumerate(re.findall(r'\beCUT_CMD_\w+', enum))}
 source = (ROOT/'cutscene/cutscene.c').read_text()
 rows = re.findall(r'\{\s*(eCUT_CMD_\w+)\s*,\s*"(\w+)"\s*,\s*eCUT_CMD_ARG_(\w+)\s*,\s*eCUT_CMD_ARG_(\w+)\s*,\s*eCUT_CMD_ARG_(\w+)\s*\}', source)
 result = {name: (ids[op], tuple(t for t in types if t != 'NONE')) for op,name,*types in rows}
 if len(result) != len(ids):
  raise CompileError('runtime command table/enum mismatch')
 return result


def uncomment(line):
 # Comments only outside double quotes; preserve backslashes in Atari paths.
 quoted = False
 for i,c in enumerate(line):
  if c == '"': quoted = not quoted
  if not quoted and (c in '#;' or line[i:i+2] == '//'):
   return line[:i]
 return line


def tokens(line):
 parts = re.findall(r'"[^"\r\n]*"|[^\s,]+', line)
 if any('"' in p and not (p.startswith('"') and p.endswith('"') and p.count('"') == 2) for p in parts):
  raise CompileError('invalid quoted value')
 return [p[1:-1] if p.startswith('"') else p for p in parts]


def parse(source, commands):
 if '\0' in source:
  raise CompileError('NUL byte in source')
 chunks = []
 current = None
 opened = False
 page_lines = []
 for number,raw in enumerate(source.splitlines(keepends=True), 1):
  line = uncomment(raw).strip()
  try:
   if current is None:
    if not line: continue
    match = re.fullmatch(r'<\s*(\w+)\s+NAME\s*=\s*"([^"\r\n]+)"\s*>\s*(\{)?', line, re.I)
    if not match: raise CompileError('expected <TYPE NAME="name">')
    kind,name,brace = match.groups();kind = kind.upper()
    if kind not in KINDS: raise CompileError(f'unknown chunk {kind}')
    current = Chunk(kind,name,number);opened = bool(brace);page_lines = []
    continue
   if not opened:
    if not line: continue
    if line != '{': raise CompileError('expected opening { on its own line')
    opened = True;continue
   if line == '}':
    if current.kind == 'PAGE': current.page = ''.join(page_lines).lstrip('\r\n')
    if current.kind == 'SCRIPT': current.commands.append(('RETURN',['0'],number))
    chunks.append(current);current = None;opened = False;continue
   if current.kind == 'PAGE':
    page_lines.append(raw);continue
   if not line: continue
   if current.kind == 'SCRIPT':
    args = tokens(line);op = args.pop(0).upper()
    if op not in commands: raise CompileError(f'unknown command {op}')
    expected = len(commands[op][1])
    if len(args) != expected: raise CompileError(f'{op} expects {expected} arguments, got {len(args)}')
    current.commands.append((op,args,number))
   else:
    match = re.fullmatch(r'(\w+)\s*=\s*(.+)',line)
    if not match: raise CompileError('expected FIELD = value')
    name,value = match.groups();name = name.upper()
    if name not in FIELDS[current.kind]: raise CompileError(f'unknown {current.kind} field {name}')
    if name in current.fields: raise CompileError(f'duplicate field {name}')
    values = tokens(value)
    if len(values) != 1: raise CompileError('expected one value (quote strings containing spaces)')
    current.fields[name] = (values[0],number)
  except CompileError as e:
   raise CompileError(f'line {number}: {e}') from e
 if current: raise CompileError(f'line {current.line}: unclosed {current.kind} {current.name}')
 if not chunks: raise CompileError('no cutscene chunks')
 return chunks


def integer(value):
 if re.fullmatch(r'[+-]?\d+',value): return int(value,10)
 sign = -1 if value.startswith('-') else 1
 unsigned = value.lstrip('+-')
 if re.fullmatch(r'\$[0-9a-fA-F]+',unsigned): return sign*int(unsigned[1:],16)
 if re.fullmatch(r'0[xX][0-9a-fA-F]+',unsigned): return sign*int(unsigned[2:],16)
 raise CompileError(f'invalid integer {value!r}')


def number(value,kind):
 if kind == 'FIX16_16':
  if not re.fullmatch(r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?',value):
   raise CompileError(f'invalid fixed-point value {value!r}')
  try:
   # Reproduce legacy FP32 conversion, including its integer/fraction OR.
   val = struct.unpack('>f',struct.pack('>f',float(value)))[0]
   if not math.isfinite(val) or not -32768 <= val < 32768: raise ValueError()
   whole = int(val)
   frac = struct.unpack('>f',struct.pack('>f',val-whole))[0]
   return (whole << 16) | int(frac*65536)
  except (ValueError,OverflowError): raise CompileError(f'fixed-point value out of range: {value}')
 n = integer(value)
 lo,hi = {'U8': (0,255), 'S16': (-32768,32767), 'INT': (-2147483648,2147483647)}[kind]
 if not lo <= n <= hi: raise CompileError(f'{kind} value out of range: {value}')
 return n


def compile_scene(source):
 commands = command_definitions()
 chunks = parse(source,commands)
 groups = {kind: [c for c in chunks if c.kind == kind] for kind in KINDS}
 names = {}
 out = bytearray(56)
 # Zero ID matches CutScene_Parse; version is defined by the runtime header.
 version = int(re.search(r'#define\s+dCUTSCENE_VERSION\s+(\d+)',(ROOT/'cutscene/cutscene.h').read_text()).group(1))
 if version != 3: raise CompileError(f'unsupported runtime CUT version {version}; compiler layout is v3')
 struct.pack_into('>I',out,4,version)
 for i,kind in enumerate(KINDS):
  group = groups[kind]
  if len(group) > 65535: raise CompileError(f'too many {kind} chunks')
  struct.pack_into('>I',out,8+i*4,len(out) if group else 0)
  for chunk in group:
   key = (kind,chunk.name.upper())
   if key in names: raise CompileError(f'line {chunk.line}: duplicate {kind} name {chunk.name}')
   chunk.offset = len(out);names[key] = chunk.offset
   out.extend(bytes(SIZES[kind]))
 # Header count order differs from pointer order.
 for i,kind in enumerate(('ANIMATION','ASSET','SAMPLE','SCRIPT','SPRITE','PAGE','TEXT','VAR')):
  struct.pack_into('>H',out,40+i*2,len(groups[kind]))
 # Commands follow all structures; pointers make the layout independent of order.
 for chunk in groups['SCRIPT']:
  if len(chunk.commands) > 65535: raise CompileError(f'line {chunk.line}: too many script commands')
  struct.pack_into('>HI',out,chunk.offset+4,len(chunk.commands),len(out))
  out.extend(bytes(14*len(chunk.commands)))
 def string(value):
  offset = len(out);out.extend(value.encode('latin-1')+b'\0');return offset
 def convert(value,kind):
  if kind == 'STRING': return string(value)
  if kind in KINDS:
   key = (kind,value.upper())
   if key not in names: raise CompileError(f'unknown {kind} reference {value!r}')
   return names[key]
  return number(value,kind)
 for chunk in chunks:
  struct.pack_into('>I',out,chunk.offset,string(chunk.name))
  if chunk.kind == 'PAGE': struct.pack_into('>I',out,chunk.offset+4,string(chunk.page))
  for name,(value,line) in chunk.fields.items():
   offset,kind = FIELDS[chunk.kind][name]
   try: val = convert(value,kind)
   except CompileError as e: raise CompileError(f'line {line}: {e}') from e
   fmt,mask = ('B',255) if kind == 'U8' else ('H',65535) if kind == 'S16' else ('I',0xffffffff)
   struct.pack_into('>'+fmt,out,chunk.offset+offset,val & mask)
  if chunk.kind == 'SCRIPT':
   base = struct.unpack_from('>I',out,chunk.offset+6)[0]
   for i,(op,args,line) in enumerate(chunk.commands):
    opcode,types = commands[op]
    try: values = [convert(value,kind)&0xffffffff for value,kind in zip(args,types)]
    except CompileError as e: raise CompileError(f'line {line}: {e}') from e
    values += [0]*(3-len(values))
    struct.pack_into('>HIII',out,base+i*14,opcode,*values)
 return bytes(out)


def cutbuild(source: Path,destination: Path):
 # Preserve legacy single-byte text and original PAGE line endings verbatim.
 try: data = compile_scene(source.read_bytes().decode('latin-1'))
 except CompileError as e: raise CompileError(f'{source}: {e}') from e
 destination.parent.mkdir(parents=True,exist_ok=True)
 # Failed compilation never truncates a previous output. Replace atomically.
 temporary = None
 try:
  with tempfile.NamedTemporaryFile(dir=destination.parent,prefix='.'+destination.name+'.',delete=False) as f:
   temporary = Path(f.name);f.write(data)
  temporary.replace(destination)
 finally:
  if temporary: temporary.unlink(missing_ok=True)
 return len(data)


def main(argv=None):
 parser = argparse.ArgumentParser(description=__doc__)
 parser.add_argument('source',type=Path)
 parser.add_argument('destination',type=Path)
 args = parser.parse_args(argv)
 try:
  size = cutbuild(args.source,args.destination)
 except (CompileError,OSError) as e:
  print(f'CUTBUILD: {e}',file=sys.stderr);return 1
 print(f'<CUTBUILD> {args.source} -> {args.destination} ({size} bytes, CUT v3)')
 return 0

if __name__ == '__main__':
 raise SystemExit(main())
