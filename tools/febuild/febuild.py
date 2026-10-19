#!/usr/bin/env python3
"""Host FEBUILD: RG text chunks -> big-endian, offset-based FED v2.
No host C structures or pointers are serialized. See README.md for the DSL.
"""
from pathlib import Path
import argparse,json,shlex,struct,sys
SCHEMA=json.loads((Path(__file__).with_name('schema.json')).read_text())
CHUNKS=SCHEMA['chunks']
ARRAYS={'CONTROL_LIST':('CONTROL','mControlCount'),'ITEM_LIST':('LISTITEM','mItemCount'),'SPRITE_LIST':('SPRITE','mSpriteCount')}


def hash_name(name):
    h=0
    for c in name.upper().replace('\\','/').encode('ascii'):
        h=((h<<4)+c)&0xFFFFFFFF;t=h&0xF0000000
        if t:h^=t>>24
        h&=~t
    return h


def tokens(line):
    lex=shlex.shlex(line,posix=True,punctuation_chars='<>{}[]=')
    lex.whitespace_split=True;lex.escape='';lex.commenters='#'
    return list(lex)


def parse(text):
    result=[];current=None;expect_brace=False
    for lineno,line in enumerate(text.splitlines(),1):
        line=line.strip()
        if not line or line.startswith('//'):continue
        parts=tokens(line)
        if not parts:continue
        def error(msg):raise ValueError(f'line {lineno}: {msg}')
        if current is None:
            if len(parts)==6 and parts[0]=='<' and parts[2].lower()=='name' and parts[3]=='=' and parts[-1]=='>':
                kind,name=parts[1].upper(),parts[4]
            elif len(parts)==4 and parts[0]=='[' and parts[-1]==']':kind,name=parts[1].upper(),parts[2]
            else:error('expected <TYPE name=IDENTIFIER>')
            if kind not in CHUNKS:error('unsupported chunk '+kind)
            if not name or '\0'in name:error('empty/invalid object name')
            current={'kind':kind,'name':name,'fields':{},'line':lineno};expect_brace=True
        elif expect_brace:
            if parts!=['{']:error('expected { on a separate line')
            expect_brace=False
        elif parts==['}']:
            result.append(current);current=None
        else:
            if len(parts)<3 or parts[1]!='=':error('expected MEMBER = VALUE')
            key=parts[0].upper()
            if key not in CHUNKS[current['kind']]['fields']:error('unknown member '+key)
            value=' '.join(parts[2:]);typ=CHUNKS[current['kind']]['fields'][key]['type']
            if key in current['fields'] and typ not in ARRAYS:error('duplicate member '+key)
            current['fields'].setdefault(key,[]).append(value)
    if current:raise ValueError(f'unclosed {current["kind"]} {current["name"]}')
    return result


def compile_text(text):
    objects=parse(text);groups={kind:[]for kind in CHUNKS};names={kind:{}for kind in CHUNKS}
    out=bytearray(SCHEMA['header']['size'])
    def put(off,fmt,value):struct.pack_into('>'+fmt,out,off,value)
    def align():out.extend(bytes((-len(out))%4))
    for obj in objects:
        kind=obj['kind'];key=hash_name(obj['name'])
        if key in names[kind]:raise ValueError('duplicate name/hash: '+kind+' '+obj['name'])
        names[kind][key]=obj;groups[kind].append(obj)
    if not groups['PAGE']:raise ValueError('FED needs at least one PAGE')
    put(0,'I',SCHEMA['header']['id']);put(4,'I',2)
    for kind,schema in CHUNKS.items():
        align();items=groups[kind]
        if len(items)>65535:raise ValueError('too many '+kind+' objects')
        put(schema['table'],'I',len(out)if items else 0);put(schema['count'],'H',len(items))
        for obj in items:obj['offset']=len(out);out.extend(bytes(schema['size']))
    def ref(kind,name):
        if name.upper()in ('NULL','NONE','0'):return 0
        obj=names[kind].get(hash_name(name))
        if obj is None:raise ValueError('unknown '+kind+' reference: '+name)
        return obj['offset']
    def integer(value,fmt):
        n=int(value,0) if value.lower().startswith(('0x','-0x','+0x'))else int(value,10)
        bits={'B':8,'b':8,'H':16,'h':16,'I':32,'i':32}[fmt]
        low=-(1<<(bits-1))if fmt.islower()else 0;high=(1<<(bits-(1 if fmt.islower()else 0)))-1
        if not low<=n<=high:raise ValueError(f'{value} out of range for {fmt}')
        return n
    strings={}
    for obj in objects:
        kind=obj['kind'];schema=CHUNKS[kind];base=obj['offset']
        if kind=='PAGE':put(base+schema['mHash'],'I',hash_name(obj['name']))
        for key,values in obj['fields'].items():
            field=schema['fields'][key];off=base+field['offset'];typ=field['type'];v=values[0]
            try:
                if typ in ARRAYS:
                    target,count=ARRAYS[typ]
                    if len(values)>65535:raise ValueError('too many list entries')
                    align();ptr=len(out)
                    for value in values:out.extend(struct.pack('>I',ref(target,value)))
                    put(off,'I',ptr);put(base+schema[count],'H',len(values))
                elif typ.startswith('p'):
                    put(off,'I',ref('TRANSITION'if typ=='pTRANS'else typ[1:],v))
                elif typ=='STRING':
                    if '\0'in v:raise ValueError('embedded NUL in string')
                    if v not in strings:strings[v]=len(out);out.extend(v.encode('latin-1')+b'\0')
                    put(off,'I',strings[v])
                elif typ in SCHEMA['enums']:
                    if v.upper()not in SCHEMA['enums'][typ]:raise ValueError('unknown '+typ+' value '+v)
                    put(off,'H',SCHEMA['enums'][typ][v.upper()])
                else:
                    fmt={'U8':'B','S8':'b','U16':'H','S16':'h','U32':'I','S32':'i'}[typ]
                    put(off,fmt,integer(v,fmt))
            except (ValueError,UnicodeError)as e:raise ValueError(f'line {obj["line"]}, {kind} {obj["name"]}.{key}: {e}')from e
        if kind in ('LIST','CONTROLLIST','SPRITELIST'):
            member={'LIST':'ITEM','CONTROLLIST':'CONTROL','SPRITELIST':'SPRITE'}[kind]
            if member not in obj['fields']:raise ValueError(kind+' must not be empty: '+obj['name'])
        if kind=='SLIDER':
            def val(key,default):return int(obj['fields'].get(key,[str(default)])[0],0)
            if val('VALUE_MAX',0)<=val('VALUE_MIN',0)or val('VALUE_ADD',1)<0:raise ValueError('invalid SLIDER range/step: '+obj['name'])
        if kind=='CONTROL':
            control=obj['fields'].get('CONTROL_TYPE',['NONE'])[0].upper()
            if control in ('LINK','CALL','LIST','SLIDER')and control not in obj['fields']:raise ValueError('CONTROL missing '+control+': '+obj['name'])
    align();return bytes(out)


def febuild(src,dst):
    data=compile_text(Path(src).read_text(encoding='utf-8'))
    dst=Path(dst);dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(data)
    print(f'FEBUILD: {src} -> {dst} ({len(data)} bytes, FED v2)')


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('destination',type=Path);ns=p.parse_args(argv)
    try:febuild(ns.source,ns.destination)
    except (OSError,ValueError)as e:p.exit(1,f'FEBUILD: {e}\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
