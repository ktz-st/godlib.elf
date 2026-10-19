#!/usr/bin/env python3
"""Regenerate checked-in Atari ABI metadata from the actual FE headers/parser.
Only developers regenerating the schema need the cross compiler.
"""
from pathlib import Path
import json,re,struct,subprocess,tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
C=ROOT/'fe/fedparse.c'
text=C.read_text()
text=re.sub(r'/\*.*?\*/','',text,flags=re.S)
registry=[('ASSET','Asset','Assets'),('CALL','Call','Calls'),('CONTROL','Control','Controls'),('CONTROLLIST','ControlList','ControlLists'),('FONTGROUP','FontGroup','FontGroups'),('LIST','List','Lists'),('LISTITEM','ListItem','ListItems'),('LOCK','Lock','Locks'),('PAGE','Page','Pages'),('PAGESTYLE','PageStyle','PageStyles'),('SAMPLE','Sample','Samples'),('SLIDER','Slider','Sliders'),('SPRITE','Sprite','Sprites'),('SPRITEGROUP','SpriteGroup','SpriteGroups'),('SPRITELIST','SpriteList','SpriteLists'),('TEXT','Text','Texts'),('TRANSITION','Transition','Transitions'),('VAR','Var','Vars')]
expr=['sizeof(sFedHeader)','dFED_ID'];keys=[('header','size'),('header','id')]
schema={'header':{},'chunks':{},'enums':{}}
for tag,typ,plural in registry:
    item={'fields':{}};schema['chunks'][tag]=item
    for key,e in [('size',f'sizeof(sFed{typ})'),('table',f'mOFFSET(sFedHeader,mp{plural})'),('count',f'mOFFSET(sFedHeader,m{typ}Count)')]:keys.append((tag,key));expr.append(e)
    block=re.search(r'gFedParse'+typ+r'Members\[\]\s*=\s*\{(.*?)\n\};',text,re.S).group(1)
    for kind,name,ctype,field in re.findall(r'\{\s*eFEDPARSE_TYPE_(\w+),\s*"(\w+)",\s*mOFFSET\(\s*(\w+),\s*([\w.]+)\s*\)',block):
        item['fields'][name]={'type':kind};keys.append((tag,name));expr.append(f'mOFFSET({ctype},{field})')
    hidden={'PAGE':['mHash'],'CONTROLLIST':['mControlCount'],'LIST':['mItemCount'],'SPRITELIST':['mSpriteCount']}.get(tag,[])
    for field in hidden:keys.append((tag,field));expr.append(f'mOFFSET(sFed{typ},{field})')
for table,tag in [('Align','ALIGN'),('Compare','COMPARE'),('ControlType','CONTROL'),('Fade','FADE'),('Wipe','WIPE')]:
    block=re.search(r'gFedParse'+table+r'Names\[[^]]+\]\s*=\s*\{(.*?)\n\};',text,re.S).group(1)
    names=re.findall(r'\{\s*(eFED_\w+),\s*"(\w+)"',block)
    schema['enums'][tag]={name:i for i,(_,name)in enumerate(names)}
with tempfile.TemporaryDirectory()as tmp:
    tmp=Path(tmp);src=tmp/'layout.c';obj=tmp/'layout.o';out=tmp/'layout.bin'
    src.write_text('#include <godlib/fe/fed.h>\nconst U32 layout[] __attribute__((section(".fedlayout"),used))={\n'+',\n'.join(expr)+'\n};\n')
    subprocess.run(['m68k-atari-mintelf-gcc','-mshort','-mfastcall','-m68000','-fpack-struct=2','-I'+str(ROOT.parent),'-c',str(src),'-o',str(obj)],check=True)
    subprocess.run(['m68k-atari-mintelf-objcopy','-O','binary','--only-section=.fedlayout',str(obj),str(out)],check=True)
    vals=struct.unpack('>'+str(len(keys))+'I',out.read_bytes())
for (tag,key),value in zip(keys,vals):
    if tag=='header':schema['header'][key]=value
    elif key in schema['chunks'][tag]['fields']:schema['chunks'][tag]['fields'][key]['offset']=value
    else:schema['chunks'][tag][key]=value
(HERE/'schema.json').write_text(json.dumps(schema,indent=2)+'\n')
print('Generated Atari FE schema from headers and parser')
