"""Compare all CUT fields/opcodes against the real 68000 text parser."""
from pathlib import Path
import hashlib,json,os,sys,time
os.environ.setdefault('SDL_VIDEODRIVER','dummy');os.environ.setdefault('SDL_AUDIODRIVER','dummy')
sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
r=Path(__file__).resolve().parent;prg=r/'CUTBUILD.PRG';sy=nm_symbols(str(prg),nm='m68k-atari-mintelf-nm')
with HatariSession(str(prg),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),machine='ste',debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--cpuclock','8'])as h:
 h.wait(10);base=h.basepage()['text'];address=lambda n:base+sy[n]
 h.debug('b pc = $%x :once'%address('CutBuild_Marker'),settle=.5)
 mem=h.mem(address('gCutBuildDone'),2,paused=True);assert mem.u16(address('gCutBuildDone'))==1,h.lines[-20:]
 error=h.mem(address('gCutBuildError'),2,paused=True).u16(address('gCutBuildError'))
 count=h.mem(address('gCutBuildChecks'),2,paused=True).u16(address('gCutBuildChecks'))
 assert error==0,('failed check',error,'of',count,h.lines[-20:])
 assert count>300,count
 assert not any(any(e in line for e in ('Bus Error','Address Error','Illegal instruction'))for line in h.lines),h.lines[-20:]
report={'result':'PASS','checks':count,'machine':'STE, 68000, 8 MHz, 1 MiB, German TOS 1.62','fixture_sha256':hashlib.sha256((r/'fixture.cut').read_bytes()).hexdigest(),'program_sha256':hashlib.sha256(prg.read_bytes()).hexdigest(),'coverage':'All eight chunk types, every opcode and typed reference, initial definitions, 16.16 speeds, signed integers, PAGE bytes, ABI sizes, exact delocate/relocate round trip; compared semantically with native CutScene_Parse.'}
(r/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
