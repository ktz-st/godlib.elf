from pathlib import Path
import bisect,json,subprocess,sys,time
sys.path.insert(0,str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
from hrdb_profile import Rdb,profile_lines
root=Path(__file__).resolve().parent
exe=root/'PROFILE.TOS'
nm=subprocess.check_output(['m68k-atari-mintelf-nm','-n',str(exe)],text=True)
syms={p[2]:int(p[0],16) for line in nm.splitlines() if len(p:=line.split())==3}
textsyms=sorted((int(p[0],16),p[2]) for line in nm.splitlines() if len(p:=line.split())==3 and p[1] in 'Tt')
addresses=[a for a,n in textsyms]
asm_sizes={'Sgdl':0x1ea,'Slug':0x3b8,'Desert':0x2e6,'Audio':0x1c2}
# The assembly sections start at their exported Lock symbol; the C callbacks
# are bounded by the next text symbol. Sizes are checked against the objects.
for name,size in asm_sizes.items():
 obj=root.parent.parent/('amixer_s.o' if name=='Audio' else name.lower()+'mix_s.o')
 header=subprocess.check_output(['m68k-atari-mintelf-objdump','-h',str(obj)],text=True)
 assert any(int(p[2],16)==size for line in header.splitlines() if len(p:=line.split())>=3 and p[1] in ('.text','text','CODE'))

def readmem(r,addr,size):
 r.take('OK');r.cmd(f'mem {addr:x} {size:x}',wait=.1);msg=r.take('OK')
 assert msg and len(msg)>=4,repr(msg)
 encoded=b'\x01'.join(msg[3:]);out=bytearray()
 for i in range(0,len(encoded)-3,4):
  n=0
  for c in encoded[i:i+4]:n=(n<<6)|((c-32)&63)
  out.extend(n.to_bytes(3,'big'))
 return bytes(out[:size])

def integer(r,addr,size):return int.from_bytes(readmem(r,addr,size),'big')
cases=[(code,name,count) for kind,name,limit in [(0,'SGDL 12.5 kHz',4),(1,'SGDL 25 kHz',4),(2,'Slug 12.5 kHz',3),(3,'Desert 12.5 kHz',4)] for count in range(limit+1) for code in [kind*10+count]]
legacy='--legacy-only' in sys.argv
if legacy:cases=[(40+n,'AudioMixer 25 kHz stereo',n) for n in range(3)]
output=root/('legacy-results.json' if legacy else 'results.json')
results=[]
with HatariSession(str(exe),hatari='hatari-hrdb',machine='ste',sound=False,debug_except='bus,address,illegal',extra_args=['--memsize','4','--monitor','rgb','--cpuclock','8']) as h:
 h.wait(8);base=h.basepage()['text']
 try:r=Rdb()
 except OSError:
  print('\n'.join(h.lines[-40:]),flush=True)
  raise
 r.cmd('break',wait=.3);r.cmd('profile 1',wait=.1)
 for code,name,count in cases:
  r.cmd(f"memset {base+syms['gProfileRequest']:x} 2 {code:04x}",wait=.1)
  r.cmd('run',wait=.1);time.sleep(.35);r.cmd('break',wait=.2)
  assert integer(r,base+syms['gProfileReady'],2)==code
  assert integer(r,base+syms['gProfileFailures'],2)==0
  frames0=integer(r,base+syms['gProfileFrames'],4)
  r.take('!profile');r.cmd('profile 1',wait=.1);r.cmd('run',wait=.1)
  time.sleep(1.5);r.cmd('break',wait=.4)
  msg=r.take('!profile');assert msg,'No cycle profile'
  lines=profile_lines(msg);assert lines and sum(n for a,n,c in lines)>1000
  frames1=integer(r,base+syms['gProfileFrames'],4)
  prefix=['Sgdl','Sgdl','Slug','Desert','Audio'][code//10]
  lo=syms[prefix+'Mixer_Vbl' if legacy else prefix+'Mixer_Lock'];hi=lo+asm_sizes[prefix]
  vbl=syms[prefix+'Mixer_Vbl'];vi=bisect.bisect_right(addresses,vbl);vhi=addresses[vi]
  core=sum(c for a,n,c in lines if base+lo<=a<base+hi)
  wrapper=0 if legacy else sum(c for a,n,c in lines if base+vbl<=a<base+vhi)
  calls=sum(n for a,n,c in lines if a==base+vbl)
  total=sum(c for a,n,c in lines)
  row=dict(mixer=name,active_channels=count,frames=frames1-frames0,vbl_calls=calls,total_cycles=total,core_cycles=core,wrapper_cycles=wrapper,mixer_cycles=core+wrapper,cpu_pct=100*(core+wrapper)/(calls*160256),profile_total_pct=100*(core+wrapper)/total,max_mixer_cycles_per_instruction=max(c/n for a,n,c in lines if n and (base+lo<=a<base+hi or base+vbl<=a<base+vhi)),cycles_per_vbl=(core+wrapper)/calls,us_per_vbl=(core+wrapper)/calls/8)
  assert calls>40 and frames1-frames0>40
  print(json.dumps(row),flush=True);results.append(row)
  output.write_text(json.dumps({'machine':'Hatari-hrdb STE 8 MHz, PAL RGB, 4 MB, raster debug off','method':'per-instruction CPU cycles, idle busy loop, actual DMA/VBL playback; initialization excluded','cases':results},indent=2)+'\n')
  # Keep profiling enabled during warmup: this hrdb build retains its previous
  # cycle timestamp, so disabling across a run would charge the unprofiled gap
  # to the first instruction in the next capture. Each break/run resets data.
  assert abs(total/(calls*160256)-1)<.025
  assert row['max_mixer_cycles_per_instruction']<512
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
 r.cmd('profile 0',wait=.1);r.cmd('run',wait=.1);r.s.close()
print('PASS: nonzero CPU profiles for',len(cases),'playback configurations')
