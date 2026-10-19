"""Run real 68000/mshort/mfastcall reflection and JSON/FED regressions."""
from pathlib import Path
import os,sys,time,struct,json,hashlib
os.environ.setdefault('SDL_VIDEODRIVER','dummy');os.environ.setdefault('SDL_AUDIODRIVER','dummy')
sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent;result=root/'RESULT.BIN';result.unlink(missing_ok=True)
with HatariSession(str(root/'REFTEST.PRG'),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--cpuclock','8','--monitor','rgb'])as h:
 deadline=time.monotonic()+25
 while time.monotonic()<deadline:
  if result.exists()and result.stat().st_size==272:break
  time.sleep(.1)
 else:raise AssertionError(('test result not produced',h.lines[-20:]))
 magic,checks,failures,line=struct.unpack_from('>4I',result.read_bytes())
 assert magic==0x52464c54 and checks>=100,(magic,checks)
 assert failures==0,{'checks':checks,'failures':failures,'first_failure_line':line,'failure_lines':struct.unpack_from('>64I',result.read_bytes(),16)[:failures]}
 assert not any('Bus Error'in s or'Address Error'in s or'CPU exception'in s for s in h.lines),h.lines[-20:]
 report={'result':'PASS','checks':checks,'failures':failures,'machine':'STE 68000 / 8 MHz / 1 MiB / German TOS 1.62','executable_sha256':hashlib.sha256((root/'REFTEST.PRG').read_bytes()).hexdigest(),'covers':['field-name lookup','all integer widths and full S32/U32 range','case-insensitive integer booleans','F32 decimals and exponents','bounded non-terminated token','shallow string view','overflow and malformed input preserve destination','adjacent fields and guards unchanged','real JSON_Parse/ElementsToStruct','actual FED layout reflection descriptor']}
 (root/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
result.unlink(missing_ok=True)
