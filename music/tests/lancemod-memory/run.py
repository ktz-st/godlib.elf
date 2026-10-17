#!/usr/bin/env python3
"""Target guard/PCM tests. Optional --baseline points to a pre-change source snapshot."""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess,sys,time
root=Path(__file__).resolve().parents[4]
here=Path(__file__).resolve().parent
build=here/'build';build.mkdir(exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--baseline',type=Path);parser.add_argument('--scopes',action='store_true');args=parser.parse_args()
paths=[root/'godlib.spl/lancemod/LOSTPARO.MOD',root/'godlib.spl/wizzcat/QUICKIE.MOD',root/'atari-ste-protracker-23f/lance/4matwave.mod',root/'godlib.spl/lancemod/BOOTUP.MOD',root/'godlib.spl/lancemod/CANNONFO.MOD']
modules=[p.read_bytes() for p in paths]
def synthetic(empty=False):
 b=bytearray(1084+1024);b[:11]=b'guard tests';b[950]=1;b[1080:1084]=b'M.K.'
 if not empty:
  for slot,data,loop in [(1,b'\x40\xc0',0),(15,b'\x60\x20\xa0\xe0',2),(30,bytes([0,12,24,36,48,60,72,84,96,108]),0)]:
   h=20+30*slot;struct.pack_into('>H',b,h+22,len(data)//2);b[h+25]=64;struct.pack_into('>H',b,h+28,loop or 1);b.extend(data)
 for row in range(0,64,4):
  for c,sample in enumerate([1,2,16,31]):
   period=428+c*20;i=1084+row*16+c*4
   b[i:i+4]=bytes([(sample&0xf0)|(period>>8),period&255,(sample&15)<<4,0])
 # Exercise invert-loop shadow copies on the small looping sample.
 if not empty:b[1084+2*4+2]|=14;b[1084+2*4+3]=0xf1
 return bytes(b)
synthetic_index=len(modules)
modules.extend([synthetic(),synthetic(True)])
def info(b):
 n=sum(struct.unpack_from('>H',b,42+30*i)[0]>0 for i in range(31))
 size=1084+(max(b[952:1080])+1)*1024+sum(2*struct.unpack_from('>H',b,42+30*i)[0] for i in range(31))
 return size+664*n+2
for i,b in enumerate(modules):(build/('M%d.MOD'%i)).write_bytes(b)
bad=[modules[0][:-1],modules[0][:1080]+b'8CHN'+modules[0][1084:],b'x']
b=bytearray(modules[synthetic_index]);struct.pack_into('>H',b,20+30+28,500);bad.append(b)
b=bytearray(modules[synthetic_index]);b[20+30+25]=65;bad.append(b)
for i,b in enumerate(bad):(build/('B%d.MOD'%i)).write_bytes(b)
(build/'cases.h').write_text('#define CASE_COUNT %d\n#define BAD_COUNT %d\nstatic const char *caseNames[]={%s};\nstatic const char *badNames[]={%s};\nstatic const U32 caseLoadBytes[]={%s};\n'%(len(modules),len(bad),','.join('"M%d.MOD"'%i for i in range(len(modules))),','.join('"B%d.MOD"'%i for i in range(len(bad))),','.join(str(info(b))+'UL' for b in modules)))
def run(cmd):subprocess.run([str(x) for x in cmd],check=True,cwd=root,stdout=subprocess.DEVNULL)
cc='m68k-atari-mintelf-gcc'
flags=['-Os','-mshort','-mfastcall','-m68000','-fpack-struct=2','-I'+str(root/'libcmini/include'),'-I'+str(root),'-I'+str(build)]
variants=['new']+(['baseline'] if args.baseline else [])
for variant in variants:
 base=(args.baseline if variant=='baseline' else root/'godlib/music').resolve()
 s=(base/'lancepaula.s').read_text()
 # Baseline gets ONLY its known 50K capacity bug fixed, to allow a safe PCM comparison.
 if variant=='baseline':
  assert 'ds.b\t55497*2' in s,'expected pre-change 110994-byte code reservation'
  s=s.replace('ds.b\t55497*2','ds.b\t75557*2')
 marker='\tdbra\td7,l_paula_maker3\n\n\tmove\tpaula_frequency,d0'
 assert marker in s;s=s.replace(marker,'\tdbra\td7,l_paula_maker3\n\tmove.l\ta1,gLanceTestCodeEnd\n\n\tmove\tpaula_frequency,d0')
 paula=build/(variant+'-paula.s');paula.write_text(s)
 s=(base/'lancemod_s.s').read_text().replace('"music/lancepaula.s"','"'+str(paula)+'"').replace('"music/lancetracker.s"','"'+str(base/'lancetracker.s')+'"')
 s+='''\n section bss
 gLanceTestCodeEnd: ds.l 1
 section text
 xdef LanceTest_GetReplay,LanceTest_GetCodeBytes
 LanceTest_GetReplay:
 move.l paula_replay_buf_p,a0
 move.l a0,d0
 rts
 LanceTest_GetCodeBytes:
 move.l gLanceTestCodeEnd,d0
'''
 s+=(' sub.l #buf_paula_mixer_chunk,d0\n' if variant=='baseline' else ' sub.l paula_mixer_code,d0\n')+' rts\n'
 asm=build/(variant+'-replay.s');asm.write_text(s)
 run(['vasmm68k_mot','-Felf','-devpac',asm,'-o',build/(variant+'-replay.o')])
 defs=['-DLANCE_BASELINE'] if variant=='baseline' else []
 if variant=='new' and '--scopes' in sys.argv:defs+=['-DTEST_MODSCOPES']
 run([cc,'-c',*flags,*defs,here/'verify.c','-o',build/(variant+'-test.o')])
 run([cc,'-c',*flags,base/'lancemod.c','-o',build/(variant+'-wrapper.o')])
 out=build/('OLD.TOS' if variant=='baseline' else 'LMEM.TOS')
 run([cc,'-nostdlib','-nostartfiles','-static',*flags,'-L'+str(root/'libcmini/lib/mshort/mfastcall'),'-L'+str(root/'godlib'),'-o',out,root/'libcmini/lib/mshort/mfastcall/crt0.o',build/(variant+'-test.o'),build/(variant+'-wrapper.o'),build/(variant+'-replay.o'),'-Wl,--wrap=Memory_Alloc,--wrap=Memory_Calloc,--wrap=_Memory_Release,--wrap=Memory_ScreenCalloc,--wrap=Memory_ScreenRelease','-Wl,--start-group','-lgod','-lcmini','-lgcc','-Wl,--end-group'])
os.environ.setdefault('SDL_VIDEODRIVER','dummy');sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
reports={}
for variant in variants:
 out=build/('OLD.TOS' if variant=='baseline' else 'LMEM.TOS');sy=nm_symbols(str(out),nm='m68k-atari-mintelf-nm')
 with HatariSession(str(out),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--fast-forward','true','--cpuclock','8','--cpulevel','0']) as h:
  h.wait(5);base=h.basepage()['text'];address=lambda n:base+sy[n]
  for attempt in range(120):
   m=h.mem(address('gLanceTestDone'),2)
   if m.u16(address('gLanceTestDone')):break
   if attempt%8==0:print(variant,'running',attempt,flush=True)
   h.wait(1)
  assert m.u16(address('gLanceTestDone'))==1,'target timeout'
  errors=h.mem(address('gLanceTestErrors'),2).u16(address('gLanceTestErrors'))
  frames=h.mem(address('gLanceTestFrames'),4).u32(address('gLanceTestFrames'))
  rows=h.mem(address('gLanceTestResult'),len(modules)*3*20).bytes(address('gLanceTestResult'),len(modules)*3*20)
  records=[dict(zip(['pcm_hash','heap_bytes','code_bytes','module_bytes','workspace_bytes'],struct.unpack_from('>5I',rows,c*20))) for c in range(len(modules)*3)]
  faults=[l for l in h.lines if any(e in l for e in ['Bus Error','Address Error','Illegal instruction'])]
  (build/(variant+'.log')).write_text('\n'.join(h.lines))
  pcm_path=build/('OLDPCM.BIN' if variant=='baseline' else 'NEWPCM.BIN')
  pcm_bytes=pcm_path.read_bytes();offset=0
  for i,record in enumerate(records):
   n=(1000<<(i%3))*1000
   assert record['code_bytes']==[77638,110994,151114][i%3]
   record['pcm_sha256']=hashlib.sha256(pcm_bytes[offset:offset+n]).hexdigest();offset+=n
  assert len(pcm_bytes)==offset
  reports[variant]={'errors':errors,'frames':frames,'records':records,'faults':faults,'pcm_bytes':len(pcm_bytes),'pcm_sha256':hashlib.sha256(pcm_bytes).hexdigest(),'module_sources':[str(p.relative_to(root)) for p in paths]+['synthetic sparse samples and EF1','synthetic empty samples']}
  print(variant,'errors',errors,'frames',frames,flush=True)
  (here/('scope-results.json' if '--scopes' in sys.argv else 'results.json')).write_text(json.dumps(reports,indent=2)+'\n')
  assert errors==0 and not faults,(variant,errors,faults)
if 'baseline' in reports:
 for i,(new,old) in enumerate(zip(reports['new']['records'],reports['baseline']['records'])):
  assert new['pcm_sha256']==old['pcm_sha256'] and new['pcm_hash']==old['pcm_hash'],('PCM mismatch',i,new,old)
 assert (build/'NEWPCM.BIN').read_bytes()==(build/'OLDPCM.BIN').read_bytes()
 print('PASS: byte-identical PCM in all %d module/rate combinations'%(len(modules)*3),flush=True)

if '--scopes' in sys.argv:
 golden=json.loads((here/'results.json').read_text())['new']['records']
 current=reports['new']['records']
 assert len(golden)==len(current),'different module/rate count from pre-scope run'
 for i,(old,new) in enumerate(zip(golden,current)):
  assert old['pcm_sha256']==new['pcm_sha256'],('scope getter changed PCM',i)
 reports['scope_pcm_regression']='PASS: all PCM SHA256 values match pre-scope results.json'
 (here/'scope-results.json').write_text(json.dumps(reports,indent=2)+'\n')
 print(reports['scope_pcm_regression'],flush=True)
