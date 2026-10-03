from pathlib import Path
import sys,subprocess,time
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'verify.tos')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
with HatariSession(str(root/'verify.tos'),machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--memsize','4']) as h:
 h.wait(8);base=h.basepage()['text']
 def word(n):
  a=base+syms[n];return h.mem(a,2).u16(a)
 for _ in range(60):
  if word('gFontVerifyDone'):break
  print('cases',word('gFontVerifyCases'),flush=True);h.wait(3)
 assert word('gFontVerifyDone')==1
 count=word('gFontVerifyCases');fail=word('gFontVerifyFailures')
 print('cases',count,'failures',fail,flush=True);assert count==58 and fail==0
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
print('PASS: font8x8 mono/colour, raw/Canvas/Screen pages, wide/padded/nonuniform rows and Y>200')
