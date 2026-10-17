#!/usr/bin/env python3
"""Target memory guards, exact increment tables and baseline PCM comparison."""
from pathlib import Path
import hashlib,json,os,struct,subprocess,sys
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
build=here/'build';build.mkdir(exist_ok=True)
paths=[root/'godlib.spl/lancemod'/p for p in ['LOSTPARO.MOD','BOOTUP.MOD','CANNONFO.MOD']]
paths += [root/'godlib.spl/wizzcat/QUICKIE.MOD',root/'atari-ste-protracker-23f/lance/4matwave.mod']
mods=[p.read_bytes() for p in paths]
for empty in [False,True]:
 b=bytearray(1084+1024);b[950]=1;b[1080:1084]=b'M.K.'
 if not empty:
  for slot,data,loop in [(1,b'\x40\xc0',0),(15,b'\x60\x20\xa0\xe0',2),(30,bytes(range(32)),0)]:
   h=20+30*slot;struct.pack_into('>H',b,h+22,len(data)//2);b[h+25]=64;struct.pack_into('>H',b,h+28,loop or 1);b.extend(data)
 for row in range(0,64,4):
  for c,sample in enumerate([1,2,16,31]):
   period=[113,428,127,856][c];i=1084+row*16+c*4
   b[i:i+4]=bytes([(sample&0xf0)|(period>>8),period&255,(sample&15)<<4,0])
 mods.append(bytes(b))
for i,b in enumerate(mods):(build/f'M{i}.MOD').write_bytes(b)
bad=[mods[0][:-1],mods[0][:1080]+b'8CHN'+mods[0][1084:],b'x']
b=bytearray(mods[-2]);struct.pack_into('>H',b,20+30+28,500);bad.append(b)
b=bytearray(mods[-2]);b[20+30+25]=65;bad.append(b)
for i,b in enumerate(bad):(build/f'B{i}.MOD').write_bytes(b)
def info(b):
 s=e=n=0
 for i in range(31):
  l,st,lp=struct.unpack_from('>H2xHH',b,42+i*30);s+=l*2
  if l:
   n+=1;rep=max(1,lp)*2;e+=(st or l)*2+((1600+rep-1)//rep)*rep+1600
 module=1084+(max(b[952:1080])+1)*1024+s
 workspace=max(0,e+(3200 if n<31 else 0)-s)+4
 return module+workspace+10
cc='m68k-atari-mintelf-gcc';music=root/'godlib/music'
flags=['-Os','-Wall','-mshort','-mfastcall','-m68000','-fpack-struct=2','-I'+str(root/'libcmini/include'),'-I'+str(root),'-I'+str(build)]
def run(cmd):subprocess.run([str(x) for x in cmd],cwd=root,check=True,stdout=subprocess.DEVNULL)
baseline=Path(sys.argv[sys.argv.index('--baseline')+1]).resolve() if '--baseline' in sys.argv else None
variants=['new']+(['old'] if baseline else [])
for v in variants:
 count=5 if v=='old' else len(mods)
 (build/'cases.h').write_text('#define CASE_COUNT %d\n#define BAD_COUNT %d\nstatic const char *caseNames[]={%s};\nstatic const char *badNames[]={%s};\nstatic const U32 caseLoadBytes[]={%s};\n'%(count,len(bad),','.join('"M%d.MOD"'%i for i in range(count)),','.join('"B%d.MOD"'%i for i in range(len(bad))),','.join(str(info(b))+'UL' for b in mods[:count])))
 source=baseline if v=='old' else music
 s=(source/'wizzcat_s.s').read_text(errors='replace')+'\n'+(here/'probe.s').read_text()
 asm=build/(v+'-replay.s');asm.write_text(s)
 run(['vasmm68k_mot','-devpac','-Felf',asm,'-o',build/(v+'-replay.o')])
 defs=['-DWIZ_BASELINE','-Wno-unused-variable'] if v=='old' else []
 if v=='new' and '--scopes' in sys.argv:defs+=['-DTEST_MODSCOPES']
 run([cc,'-c',*flags,*defs,here/'verify.c','-o',build/(v+'-test.o')])
 run([cc,'-c',*flags,source/'wizzcat.c','-o',build/(v+'-wrapper.o')])
 out=build/('OLD.TOS' if v=='old' else 'WMEM.TOS')
 run([cc,'-nostdlib','-nostartfiles','-static',*flags,'-L'+str(root/'libcmini/lib/mshort/mfastcall'),'-L'+str(root/'godlib'),'-o',out,root/'libcmini/lib/mshort/mfastcall/crt0.o',build/(v+'-test.o'),build/(v+'-wrapper.o'),build/(v+'-replay.o'),'-Wl,--wrap=Memory_Alloc,--wrap=Memory_Calloc,--wrap=_Memory_Release,--wrap=Memory_ScreenCalloc,--wrap=Memory_ScreenRelease','-Wl,--start-group','-lgod','-lcmini','-lgcc','-Wl,--end-group'])
os.environ.setdefault('SDL_VIDEODRIVER','dummy');sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
reports={};streams={};tables={}
for v in variants:
 out=build/('OLD.TOS' if v=='old' else 'WMEM.TOS');sy=nm_symbols(str(out),nm='m68k-atari-mintelf-nm');count=5 if v=='old' else len(mods)
 with HatariSession(str(out),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--fast-forward','true','--cpuclock','8','--cpulevel','0']) as h:
  h.wait(5);base=h.basepage()['text'];addr=lambda n:base+sy[n]
  for attempt in range(120):
   m=h.mem(addr('gWizTestDone'),2)
   if m.u16(addr('gWizTestDone'))==1:break
   if any(e in line for line in h.lines for e in ['Bus Error','Address Error','Illegal instruction']):break
   if attempt%10==0:print(v,'running',attempt,flush=True)
   h.wait(1)
  (build/(v+'.log')).write_text('\n'.join(h.lines))
  assert m.u16(addr('gWizTestDone'))==1,'target timeout/fault; see '+str(build/(v+'.log'))
  errors=h.mem(addr('gWizTestErrors'),2).u16(addr('gWizTestErrors'))
  frames=h.mem(addr('gWizTestFrames'),4).u32(addr('gWizTestFrames'))
  rows=h.mem(addr('gWizTestResult'),count*20).bytes(addr('gWizTestResult'),count*20)
  records=[dict(zip(['heap_bytes','workspace_bytes','init_hz200_ticks','pcm_hash','module_bytes'],struct.unpack_from('>5I',rows,c*20))) for c in range(count)]
  faults=[l for l in h.lines if any(e in l for e in ['Bus Error','Address Error','Illegal instruction'])]
  streams[v]=(build/'PCM.BIN').read_bytes();tables[v]=(build/'TABLES.BIN').read_bytes()
  for i,r in enumerate(records):r['pcm_sha256']=hashlib.sha256(streams[v][i*2000000:(i+1)*2000000]).hexdigest()
  reports[v]={'errors':errors,'frames':frames,'records':records,'faults':faults,'pcm_bytes':len(streams[v]),'tables_sha256':hashlib.sha256(tables[v]).hexdigest(),'module_sources':[str(p.relative_to(root)) for p in paths]+([] if v=='old' else ['synthetic sparse/short loops','synthetic empty samples'])}
  (here/('scope-results.json' if '--scopes' in sys.argv else 'results.json')).write_text(json.dumps(reports,indent=2)+'\n');print(v,'errors',errors,'frames',frames,flush=True)
  assert errors==0 and not faults,(v,errors,faults)
  assert len(streams[v])==count*2000000 and len(tables[v])==29728
if baseline:
 assert tables['new']==tables['old'],'increment/volume table mismatch'
 assert streams['new'][:len(streams['old'])]==streams['old'],'PCM bytes differ'
 for i in range(5):assert reports['new']['records'][i]['pcm_sha256']==reports['old']['records'][i]['pcm_sha256'],('PCM regression',i)
 reports['baseline']='PASS: 29728 table bytes and 10 MB PCM identical across five real MODs'
 print(reports['baseline'],flush=True)
(here/('scope-results.json' if '--scopes' in sys.argv else 'results.json')).write_text(json.dumps(reports,indent=2)+'\n')

if '--scopes' in sys.argv:
 golden=json.loads((here/'results.json').read_text())['new']['records']
 current=reports['new']['records']
 assert len(golden)==len(current),'different module/rate count from pre-scope run'
 for i,(old,new) in enumerate(zip(golden,current)):
  assert old['pcm_sha256']==new['pcm_sha256'],('scope getter changed PCM',i)
 reports['scope_pcm_regression']='PASS: all PCM SHA256 values match pre-scope results.json'
 (here/'scope-results.json').write_text(json.dumps(reports,indent=2)+'\n')
 print(reports['scope_pcm_regression'],flush=True)
