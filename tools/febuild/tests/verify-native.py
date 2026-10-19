#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,os,struct,sys,time
os.environ.setdefault('SDL_VIDEODRIVER','dummy');os.environ.setdefault('SDL_AUDIODRIVER','dummy')
sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
root=Path(__file__).resolve().parent;exe=root/'FEDTEST.PRG';sy=nm_symbols(str(exe),nm='m68k-atari-mintelf-nm')
with HatariSession(str(exe),hatari='/opt/homebrew/opt/hatari/Hatari.app/Contents/MacOS/hatari',tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),screenshot_dir=str(root/'shots'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--cpuclock','8'])as h:
 h.wait(9);base=h.basepage()['text']
 def read(name,size=2):
  p=root/'dump.bin';p.unlink(missing_ok=True);h.debug(f'savebin {p} ${base+sy[name]:x} ${size:x}',settle=.05)
  for _ in range(100):
   if p.exists()and p.stat().st_size==size:return int.from_bytes(p.read_bytes(),'big')
   time.sleep(.01)
  raise AssertionError(h.lines[-15:])
 assert read('gFedFixtureReady')==1,h.lines[-15:]
 assert read('gFedFixturePass')==1,('mismatch offset',read('gFedFixtureMismatch',4))
 assert read('gFedFixtureLoops')==4
 assert not any('cpu exception' in line.lower()for line in h.lines)
 report={'result':'PASS','machine':'STE / 68000 / 8 MHz / 1 MiB / German TOS 1.62','fixture_sha256':hashlib.sha256((root/'fixture.fed').read_bytes()).hexdigest(),'checks':['four byte-exact relocate/delocate cycles using all 18 FED v2 object types','nested offsets, pointer arrays, signed coordinates and nonzero BG colour index','no CPU exceptions']}
 (root/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
 h.control('hatari-event keydown 1',settle=.15);h.control('hatari-event keyup 1',settle=.1)
