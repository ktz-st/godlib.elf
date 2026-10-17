#!/usr/bin/env python3
"""Guarded target runs, exact 25K PCM regression and independent high-note oracle."""
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
  # Sparse slots, very short loops, high/finetuned notes, one-shot samples.
  for slot,data,loop in [(1,b'\x40\xc0',0),(15,b'\x60\x20\xa0\xe0',2),(30,bytes(range(32)),0)]:
   h=20+30*slot;struct.pack_into('>H',b,h+22,len(data)//2);b[h+25]=64;b[h+24]=7
   struct.pack_into('>H',b,h+28,loop or 1);b.extend(data)
 for row in range(0,64,4):
  for c,sample in enumerate([1,2,16,31]):
   period=[108,113,127,428][c];i=1084+row*16+c*4
   b[i:i+4]=bytes([(sample&0xf0)|(period>>8),period&255,(sample&15)<<4,0])
 mods.append(bytes(b))
for i,b in enumerate(mods):(build/f'M{i}.MOD').write_bytes(b)
bad=[mods[0][:-1],mods[0][:1080]+b'8CHN'+mods[0][1084:],b'x']
b=bytearray(mods[-2]);struct.pack_into('>H',b,20+30+28,500);bad.append(b)
b=bytearray(mods[-2]);b[20+30+25]=65;bad.append(b)
for i,b in enumerate(bad):(build/f'B{i}.MOD').write_bytes(b)
def length(b):return 1084+(max(b[952:1080])+1)*1024+sum(2*struct.unpack_from('>H',b,42+30*i)[0] for i in range(31))
(build/'cases.h').write_text('#define CASE_COUNT %d\n#define BAD_COUNT %d\nstatic const char *caseNames[]={%s};\nstatic const char *badNames[]={%s};\nstatic const U32 caseLoadBytes[]={%s};\n'%(len(mods),len(bad),','.join('"M%d.MOD"'%i for i in range(len(mods))),','.join('"B%d.MOD"'%i for i in range(len(bad))),','.join(str(length(b))+'UL' for b in mods)))
def run(cmd,cwd=root):subprocess.run([str(x) for x in cmd],cwd=cwd,check=True,stdout=subprocess.DEVNULL)
cc='m68k-atari-mintelf-gcc';music=root/'godlib/music'
flags=['-Os','-Wall','-mshort','-mfastcall','-m68000','-fpack-struct=2','-I'+str(root/'libcmini/include'),'-I'+str(root),'-I'+str(build)]
# --baseline is optional; /tmp snapshot is deliberately not needed for routine tests.
baseline=Path(sys.argv[sys.argv.index('--baseline')+1]).resolve() if '--baseline' in sys.argv else None
variants=['new']+(['old'] if baseline else [])
for v in variants:
 source=baseline if v=='old' else music
 s=(source/'leonardpaula.s').read_text().replace('section\tcode','section\ttext').replace('section code','section text')
 assert '.noMSTE:\n\t\trts' in s
 s=s.replace('.noMSTE:\n\t\trts','.noMSTE:\n        move.l a4,LeoTestCodeEnd\n\t\trts',1)
 paula=build/(v+'-paula.s');paula.write_text(s)
 s=(source/('leonard_s.s' if v=='old' else 'leonardmod_s.s')).read_text()
 if v=='old':
  for a,b in [('LeoVis_Start','LeonardMod_CoreStart'),('LeoVis_Stop','LeonardMod_Stop'),('LeoVis_Tick','LeonardMod_Tick'),('LeoVis_GetVisualizer','LeonardMod_GetVisualizer'),('LeoVis_SetRasterDebug','LeonardMod_SetRasterDebug')]:s=s.replace(a,b)
 for f,p in [('leonardpaula.s',paula),('leonardtracker.s',source/'leonardtracker.s')]:
  s=s.replace('"music/'+f+'"','"'+str(p)+'"').replace('"'+f+'"','"'+str(p)+'"')
 if v=='old':
  # Baseline comparison starts each module with the same clean tracker state.
  # This resets only persistent per-song variables; the baseline mixer is intact.
  tracker=(source/'leonardtracker.s').read_text()
  new_tracker=(music/'leonardtracker.s').read_text()
  reset=new_tracker.split('\nmt_init\n',1)[1].split('\tMOVE.L\tA0,mt_SongDataPtr',1)[0]
  tracker=tracker.replace('\nmt_init\n','\nmt_init\n'+reset,1)
  clean_tracker=build/'old-tracker.s';clean_tracker.write_text(tracker)
  s=s.replace(str(source/'leonardtracker.s'),str(clean_tracker))
 s+=(here/'probe.s').read_text()
 asm=build/(v+'-replay.s');asm.write_text(s)
 run(['vasmm68k_mot','-Felf',asm,'-o',build/(v+'-replay.o')])
 defs=['-DFIRST_RATE=2','-DLEO_BASELINE','-Wno-unused-variable'] if v=='old' or '--25-only' in sys.argv else ['-DFIRST_RATE=1']
 if v=='new' and '--scopes' in sys.argv:defs+=['-DTEST_MODSCOPES']
 run([cc,'-c',*flags,*defs,here/'verify.c','-o',build/(v+'-test.o')])
 run([cc,'-c',*flags,music/'leonardmod.c','-o',build/(v+'-wrapper.o')])
 out=build/('OLD.TOS' if v=='old' else 'LEOMEM.TOS')
 run([cc,'-nostdlib','-nostartfiles','-static',*flags,'-L'+str(root/'libcmini/lib/mshort/mfastcall'),'-L'+str(root/'godlib'),'-o',out,root/'libcmini/lib/mshort/mfastcall/crt0.o',build/(v+'-test.o'),build/(v+'-wrapper.o'),build/(v+'-replay.o'),'-Wl,--wrap=Memory_Alloc,--wrap=Memory_Calloc,--wrap=_Memory_Release,--wrap=Memory_ScreenCalloc,--wrap=Memory_ScreenRelease','-Wl,--start-group','-lgod','-lcmini','-lgcc','-Wl,--end-group'])
os.environ.setdefault('SDL_VIDEODRIVER','dummy');sys.path.insert(0,str(Path.home()/'.agents/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession,nm_symbols
reports={};streams={}
for v in variants:
 out=build/('OLD.TOS' if v=='old' else 'LEOMEM.TOS');sy=nm_symbols(str(out),nm='m68k-atari-mintelf-nm')
 with HatariSession(str(out),tos=str(Path.home()/'Atari/tos/TOS v1.62 (1990)(Atari Corp)(STE)(De).img'),debug_except='bus,address,illegal',extra_args=['--confirm-quit','false','--memsize','1','--fast-forward','true','--cpuclock','8','--cpulevel','0']) as h:
  h.wait(5);base=h.basepage()['text'];addr=lambda n:base+sy[n]
  for attempt in range(180):
   m=h.mem(addr('gLeoTestDone'),2)
   if m.u16(addr('gLeoTestDone'))==1:break
   if any(e in line for line in h.lines for e in ['Bus Error','Address Error','Illegal instruction']):break
   if attempt%10==0:print(v,'running',attempt,flush=True)
   h.wait(1)
  (build/(v+'.log')).write_text('\n'.join(h.lines))
  assert m.u16(addr('gLeoTestDone'))==1,'target timeout/fault; see build/'+v+'.log'
  errors=h.mem(addr('gLeoTestErrors'),2).u16(addr('gLeoTestErrors'))
  frames=h.mem(addr('gLeoTestFrames'),4).u32(addr('gLeoTestFrames'))
  rows=h.mem(addr('gLeoTestResult'),len(mods)*2*20).bytes(addr('gLeoTestResult'),len(mods)*2*20)
  records=[dict(zip(['pcm_hash','heap_bytes','code_bytes','module_bytes','workspace_bytes'],struct.unpack_from('>5I',rows,c*20))) for c in range(len(mods)*2)]
  faults=[l for l in h.lines if any(e in l for e in ['Bus Error','Address Error','Illegal instruction'])]
  (build/(v+'.log')).write_text('\n'.join(h.lines))
  streams[v]=(build/'PCM.BIN').read_bytes();offset=0
  for i,record in enumerate(records):
   if (v=='old' or '--25-only' in sys.argv) and i%2==0:continue
   size=[508,1008][i%2]*1000
   record['pcm_sha256']=hashlib.sha256(streams[v][offset:offset+size]).hexdigest();offset+=size
   assert record['code_bytes']==[28496,56496][i%2]
  assert len(streams[v])==offset
  reports[v]={'errors':errors,'frames':frames,'records':records,'faults':faults,'pcm_bytes':offset,'module_sources':[str(p.relative_to(root)) for p in paths]+['synthetic sparse high notes and short loops','synthetic empty samples']}
  print(v,'errors',errors,'frames',frames,flush=True)
  (here/('scope-results.json' if '--scopes' in sys.argv else 'results.json')).write_text(json.dumps(reports,indent=2)+'\n')
  assert errors==0 and not faults,(v,errors,faults)
  if v=='new' and '--25-only' not in sys.argv:
   # Expected legacy output phase, but full unclamped 16.16 steps at 12.5K.
   data=(build/'HIGH.BIN').read_bytes();assert len(data)==508*4
   for c,period in enumerate([108,113,127,141]):
    step=18571430//period;pos=acc=0;end=32;held=0;want=[]
    for _ in range(254):
     want.append(held);held=pos*2 # input byte = pos*4, volume 64 / 128
     acc+=(step&65535);pos+=(step>>16)+(acc>>16);acc&=65535
     while pos>=end:pos=pos-end+4;end=10
    actual=data[c*508:(c+1)*508:2]
    assert actual==bytes(want),('high-note pitch/loop mismatch',period,next((i,a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b))
   reports[v]['high_note_oracle']='PASS: periods 108,113,127,141; six-byte loops and preserved overshoot'
if baseline:
 for i in range(1,len(mods)*2,2):assert reports['new']['records'][i]['pcm_sha256']==reports['old']['records'][i]['pcm_sha256'],('25K PCM regression',i)
 reports['25k_baseline']='PASS: all seven modules, 7000 blocks byte-identical'
 print(reports['25k_baseline'],flush=True)
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
