from pathlib import Path
import json, subprocess, sys
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'LINE.TOS')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
with HatariSession(str(root/'LINE.TOS'),machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','4','--cpuclock','8','--fast-forward','on']) as h:
 h.wait(8);base=h.basepage()['text']
 def word(name):
  a=base+syms[name];return h.mem(a,2).u16(a)
 for _ in range(120):
  if word('gLineDone'):break
  print('phase',word('gLinePhase'),'cases',word('gLineCases'),'failures',word('gLineFailures'),flush=True);h.wait(3)
 assert word('gLineDone')==1
 a=base+syms['gLineTimes'];m=h.mem(a,40);ticks=[m.u32(a+i*4) for i in range(10)]
 result=dict(machine='Hatari STE 8 MHz',cases=word('gLineCases'),failures=word('gLineFailures'),draws_per_timing=512,timer_hz=200,timings=[dict(kind=kind,optimized_ticks=ticks[i*2],baseline_ticks=ticks[i*2+1],speedup=ticks[i*2+1]/ticks[i*2]) for i,kind in enumerate(['horizontal','diagonal','vertical','clipped fully inside','clipped crossing edges'])])
 print(json.dumps(result,indent=2),flush=True)
 assert result['cases']==4096 and result['failures']==0
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
 (root/'results.json').write_text(json.dumps(result,indent=2)+'\n')
