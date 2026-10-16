from pathlib import Path
import json, subprocess, sys, tempfile, shutil
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root=Path(__file__).resolve().parent
nm=subprocess.check_output(['m68k-atari-mintelf-nm',str(root/'CHUNK.TOS')],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
with tempfile.TemporaryDirectory(prefix='chunk-test-') as td:
 prog=Path(td)/'CHUNK.TOS';shutil.copy2(root/'CHUNK.TOS',prog)
 with HatariSession(str(prog),machine='st',sound=False,debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','4','--cpuclock','8','--fast-forward','on','--tos','/Users/kriss/Atari/tos/TOS v2.06 (1991)(Atari Corp)(Mega-STE)(UK).img']) as h:
  h.wait(5);base=h.basepage()['text']
  def word(name):
   a=base+syms[name];return h.mem(a,2).u16(a)
  for _ in range(100):
   if word('gChunkDone'):break
   print('phase',word('gChunkPhase'),'cases',word('gChunkCases'),'failures',word('gChunkFailures'),flush=True);h.wait(2)
  assert word('gChunkDone')==1
  a=base+syms['gChunkTimes'];m=h.mem(a,48);ticks=[m.u32(a+i*4) for i in range(12)]
  result=dict(machine='Hatari ST 8 MHz',cases=word('gChunkCases'),failures=word('gChunkFailures'),restores_per_timing=2048,timer_hz=200,timings=[dict(kind=kind,asm_ticks=ticks[i*2],c_ticks=ticks[i*2+1],speedup=ticks[i*2+1]/ticks[i*2]) for i,kind in enumerate(['16x16 aligned','16x16 unaligned (32px restore)','32x32 aligned','32x32 unaligned (48px restore)','64x64','256x64'])])
  print(json.dumps(result,indent=2),flush=True)
  assert result['cases']==73 and result['failures']==0
  assert not any('Address Error' in l or 'Bus Error' in l for l in h.lines)
  (root/'results.json').write_text(json.dumps(result,indent=2)+'\n')
