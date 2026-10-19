"""Run the allocator tests on a real 68000 ABI in Hatari."""
from pathlib import Path
import os,sys,time,struct,json,hashlib
os.environ.setdefault('SDL_VIDEODRIVER','dummy');os.environ.setdefault('SDL_AUDIODRIVER','dummy')
sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent;result=root/'RESULT.BIN';result.unlink(missing_ok=True)
with HatariSession(str(root/'HEAPTEST.PRG'),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--cpuclock','8','--monitor','rgb'])as h:
 deadline=time.monotonic()+180
 while time.monotonic()<deadline:
  if result.exists()and result.stat().st_size==272:break
  if any('Bus Error'in line or'Address Error'in line for line in h.lines):raise AssertionError(h.lines[-20:])
  time.sleep(.1)
 else:raise AssertionError(('test result not produced',h.lines[-20:]))
 magic,checks,failures,line=struct.unpack_from('>4I',result.read_bytes())
 assert magic==0x48454150 and checks>90000,(magic,checks)
 assert failures==0,{'checks':checks,'failures':failures,'first_failure_line':line,'failure_lines':struct.unpack_from('>64I',result.read_bytes(),16)[:min(failures,64)]}
 assert not any('Bus Error'in s or'Address Error'in s or'CPU exception'in s for s in h.lines),h.lines[-20:]
 report={'result':'PASS','checks':checks,'failures':failures,'machine':'STE 68000 / 8 MHz / 1 MiB / German TOS 1.62','executable_sha256':hashlib.sha256((root/'HEAPTEST.PRG').read_bytes()).hexdigest(),'stress_operations':3000,'stress_slots':48,'host_validation':'Same test driver under AddressSanitizer and UndefinedBehaviorSanitizer, plus forced backing-allocation failure and exact ownership call counts.','covers':['owned and caller-supplied arenas','4-byte alignment and unaligned input-buffer canaries','minimum block, exact fit and unsplittable tail','first-fit reuse and immediate coalescing in both directions','fragmentation versus total free memory','exact requested/capacity/header accounting and largest free block','peak/live/cumulative statistics and Reset','zero-size, overflow and exhaustion without changing live data','foreign/interior/double/stale-after-Reset pointer rejection','corrupt metadata detection and Reset recovery','200000-byte arena, allocations/offsets above 65535','deterministic randomized payload preservation and final full coalescence']}
 (root/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
result.unlink(missing_ok=True)
