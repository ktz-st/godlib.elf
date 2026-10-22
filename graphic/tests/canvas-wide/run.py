from pathlib import Path
import json, subprocess, sys
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'WIDE.TOS')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
names=['blit cpu','blit blt','sprite_clip cpu','sprite_clip blt','sprite cpu','sprite blt','sprite_clip unaligned box cpu','sprite_clip unaligned box blt']
with HatariSession(str(root/'WIDE.TOS'),machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','4','--fast-forward','on','--tos','/Users/kriss/Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img']) as h:
 h.wait(5);base=h.basepage()['text']
 def words(name,n):
  a=base+syms[name];m=h.mem(a,2*n);return [m.u16(a+2*i) for i in range(n)]
 for _ in range(300):
  if words('gWideDone',1)[0]:break
  print('phase',words('gWidePhase',1)[0],flush=True);h.wait(2)
 assert words('gWideDone',1)[0]==1
 cases,fails,first=words('gWideCases',8),words('gWideFailures',8),words('gWideFirst',40)
 result={n:dict(cases=cases[i],failures=fails[i],first=first[i*5:i*5+5] if fails[i] else None) for i,n in enumerate(names)}
 result['blit blt skew_flags index']={str(i):dict(cases=c,failures=f) for i,(c,f) in enumerate(zip(words('gSkewCases',8),words('gSkewFailures',8)))}
 log=words('gWideLog',8*700*5);lc=words('gWideLogCount',8)
 for i,n in enumerate(names): result[n]['log']=[log[(i*700+j)*5:(i*700+j)*5+5] for j in range(lc[i])]
 for n in names: result[n]['log']=result[n]['log'][:16]
 result['machine']='Hatari STE 8 MHz, TOS 1.62'
 print(json.dumps(result,indent=1),flush=True)
 assert not any('Address Error' in l or 'Bus Error' in l for l in h.lines)
 (root/'results.json').write_text(json.dumps(result,indent=1)+'\n')
 sys.exit(1 if any(fails) else 0)
