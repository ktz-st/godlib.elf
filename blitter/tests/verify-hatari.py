from pathlib import Path
import sys,subprocess,time
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'verify.tos')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
with HatariSession(str(root/'verify.tos'),machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--memsize','4']) as h:
 h.wait(8);base=h.basepage()['text'];a=base+syms['gBlitterVerifyResult'];phase=base+syms['gBlitterVerifyPhase']
 for _ in range(24):
  result=h.mem(a,2).u16(a)
  if result!=0xffff: break
  print('phase',h.mem(phase,2).u16(phase),flush=True);h.wait(5)
 print('result',result,flush=True);assert result==0
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
print('PASS: sprite/mask/opaque/colour, boxes, independent padded copy strides, Screen wrappers and Y>200')
