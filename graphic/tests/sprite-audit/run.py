from pathlib import Path
import subprocess,sys,json
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'AUDIT.TOS')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
with HatariSession(str(root/'AUDIT.TOS'),machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--memsize','4']) as h:
 h.wait(8);base=h.basepage()['text']
 def word(n):
  a=base+syms[n];return h.mem(a,2).u16(a)
 for _ in range(60):
  if word('gAuditDone'):break
  print('phase',word('gAuditPhase'),flush=True);h.wait(5)
 assert word('gAuditDone')==1
 result={}
 for name in ('gAuditFailures','gAuditFirstWidth','gAuditFirstPhase'):
  a=base+syms[name];m=h.mem(a,4);result[name]=[m.u16(a),m.u16(a+2)]
 a=base+syms['gAuditTimes'];m=h.mem(a,128);result['ticks']=[m.u32(a+i*4) for i in range(32)]
 result['draws_per_case']=3000;result['timer_hz']=200;result['machine']='Hatari STE 8 MHz'
 print(json.dumps(result,indent=2),flush=True);(root/'results.json').write_text(json.dumps(result,indent=2)+'\n')
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
