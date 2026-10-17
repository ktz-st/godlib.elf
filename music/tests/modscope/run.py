from pathlib import Path
import os,subprocess,sys,json
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
build=here/'build';build.mkdir(exist_ok=True)
cc='m68k-atari-mintelf-gcc'
flags=['-Os','-Wall','-mshort','-mfastcall','-m68000','-fpack-struct=2','-I'+str(root/'libcmini/include'),'-I'+str(root)]
subprocess.run([cc,'-c',*flags,str(here/'verify.c'),'-o',str(build/'verify.o')],check=True)
out=build/'SCOPE.TOS'
subprocess.run([cc,'-nostdlib','-nostartfiles','-static',*flags,'-L'+str(root/'libcmini/lib/mshort/mfastcall'),'-L'+str(root/'godlib'),'-o',str(out),str(root/'libcmini/lib/mshort/mfastcall/crt0.o'),str(build/'verify.o'),'-Wl,--start-group','-lgod','-lcmini','-lgcc','-Wl,--end-group'],check=True)
os.environ.setdefault('SDL_VIDEODRIVER','dummy');sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
sy=nm_symbols(str(out),nm='m68k-atari-mintelf-nm')
with HatariSession(str(out),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--fast-forward','true','--cpuclock','8','--cpulevel','0']) as h:
 h.wait(5);base=h.basepage()['text'];a=lambda n:base+sy[n]
 for _ in range(30):
  if h.mem(a('gScopeTestDone'),2).u16(a('gScopeTestDone'))==1:break
  h.wait(1)
 errors=h.mem(a('gScopeTestErrors'),2).u16(a('gScopeTestErrors'));cases=h.mem(a('gScopeTestCases'),4).u32(a('gScopeTestCases'))
 faults=[l for l in h.lines if any(e in l for e in ['Bus Error','Address Error','Illegal instruction'])]
 (build/'run.log').write_text('\n'.join(h.lines))
 result=dict(errors=errors,cases=cases,voices=cases*4,faults=faults,tos='1.62-de',machine='STE',mhz=8)
 (here/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)
 assert cases==256 and errors==0 and not faults,result
