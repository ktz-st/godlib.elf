"""Host compiler rejection tests and CLI/TBS integration."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from cutbuild import compile_scene,cutbuild,CompileError,command_definitions,KINDS,SIZES,FIELDS

def semantic(data):
 """Compare pointer-independent contents of the original TOOLBASE fixture."""
 def word(at):return struct.unpack_from('>H',data,at)[0]
 def long(at):return struct.unpack_from('>I',data,at)[0]
 def string(at):return data[at:data.index(0,at)].decode('latin-1') if at else None
 count_kinds=('ANIMATION','ASSET','SAMPLE','SCRIPT','SPRITE','PAGE','TEXT','VAR')
 counts=dict(zip(count_kinds,struct.unpack_from('>8H',data,40)))
 arrays={kind:[long(8+i*4)+j*SIZES[kind] for j in range(counts[kind])] for i,kind in enumerate(KINDS)}
 def reference(at):return string(long(at)) if at else None
 defs={op:types for op,types in command_definitions().values()}
 result={}
 for kind in KINDS:
  group={}
  for at in arrays[kind]:
   fields={}
   for name,(offset,typ)in FIELDS[kind].items():
    address=at+offset
    fields[name]=(data[address] if typ=='U8' else word(address) if typ=='S16' else string(long(address)) if typ=='STRING' else reference(long(address)) if typ in KINDS else long(address))
   if kind=='PAGE':fields['text']=string(long(at+4))
   if kind=='SCRIPT':
    commands=[]
    for j in range(word(at+4)):
     cmd=long(at+6)+j*14;op=word(cmd);values=[]
     for k,typ in enumerate(defs[op]):
      value=long(cmd+2+k*4);values.append(reference(value)if typ in KINDS else value)
     commands.append((op,values))
    fields['commands']=commands
   group[string(long(at))]=fields
  result[kind]=group
 return result

class CompilerTests(unittest.TestCase):
 def test_all_commands_and_types(self):
  self.assertEqual(len(command_definitions()),32)
  data=compile_scene(Path(__file__).with_name('fixture.txt').read_bytes().decode('latin-1'))
  self.assertEqual(struct.unpack_from('>2I',data),(0,3))
  self.assertEqual(struct.unpack_from('>8H',data,40),(1,1,1,2,1,1,1,1))
  pointers=struct.unpack_from('>8I',data,8)
  self.assertTrue(all(56<=x<len(data)and x%2==0 for x in pointers))
  self.assertEqual(data,compile_scene(Path(__file__).with_name('fixture.txt').read_bytes().decode('latin-1')))
 def test_original_toolbase_reference(self):
  root=Path(__file__).resolve().parent
  source=(root/'fixture.txt').read_bytes()
  import hashlib,json
  info=json.loads((root/'original.json').read_text())
  self.assertEqual(hashlib.sha256(source).hexdigest(),info['source_sha256'])
  original=(root/'original.cut').read_bytes()
  self.assertEqual(hashlib.sha256(original).hexdigest(),info['output_sha256'])
  self.assertEqual(semantic(compile_scene(source.decode('latin-1'))),semantic(original))
 def test_page_bytes(self):
  for ending in ('\n','\r\n','\r'):
   s=ending.join(['<PAGE NAME="P">','{','',' A\tB; #literal //text ż'.replace('ż','\xbf'),'}',''])
   data=compile_scene(s);page=struct.unpack_from('>I',data,16)[0];text=struct.unpack_from('>I',data,page+4)[0]
   self.assertEqual(data[text:],(' A\tB; #literal //text \xbf'+ending).encode('latin-1')+b'\0')
 def test_empty_page_script(self):
  data=compile_scene('<PAGE NAME="P">\n{\n}\n<SCRIPT NAME="MAIN">\n{\n}\n')
  script=struct.unpack_from('>I',data,24)[0];self.assertEqual(struct.unpack_from('>H',data,script+4)[0],1)
 def test_case_forward_reference(self):
  compile_scene('<SCRIPT NAME="MAIN">\n{\nCALL sub\n}\n<SCRIPT NAME="SUB">\n{\n}\n')
 def test_integer_bases(self):
  data=compile_scene('<SCRIPT NAME="MAIN">\n{\nWAIT $1234\nWAIT 0x5678\n}\n')
  script=struct.unpack_from('>I',data,24)[0];cmd=struct.unpack_from('>I',data,script+6)[0]
  self.assertEqual(struct.unpack_from('>I',data,cmd+2)[0],0x1234)
  self.assertEqual(struct.unpack_from('>I',data,cmd+16)[0],0x5678)
 def test_errors(self):
  for source,reason in [
   ('','no cutscene'),('garbage','expected'),('<RECT NAME="X">\n{\n}\n','unknown chunk'),
   ('<SCRIPT NAME="M">\n{\nBOGUS\n}\n','unknown command'),
   ('<SCRIPT NAME="M">\n{\nWAIT\n}\n','expects 1'),
   ('<SCRIPT NAME="M">\n{\nBG_GFX_CLEAR 1\n}\n','expects 0'),
   ('<SCRIPT NAME="M">\n{\nCALL missing\n}\n','unknown SCRIPT'),
   ('<SCRIPT NAME="M">\n{\nWAIT 2147483648\n}\n','out of range'),
   ('<SCRIPT NAME="M">\n{\nWAIT 2x\n}\n','invalid integer'),
   ('<SCRIPT NAME="M">\n{\n','unclosed'),
   ('<PAGE NAME="P">\n{\n}\n<PAGE NAME="p">\n{\n}\n','duplicate PAGE'),
   ('<TEXT NAME="T">\n{\nASSET missing\n}\n','expected FIELD'),
   ('<TEXT NAME="T">\n{\nTYPO = 1\n}\n','unknown TEXT field'),
   ('<TEXT NAME="T">\n{\nX = 1\nX = 2\n}\n','duplicate field'),
   ('<TEXT NAME="T">\n{\nX = 32768\n}\n','out of range'),
   ('<SAMPLE NAME="S">\n{\nVOLUME = 256\n}\n','out of range'),
   ('<SPRITE NAME="S">\n{\nSPEED = nan\n}\n','invalid fixed-point'),
   ('<SPRITE NAME="S">\n{\nSPEED = 32768\n}\n','out of range'),
   ('\0','NUL')]:
   with self.subTest(reason=reason,source=source),self.assertRaisesRegex(CompileError,reason):compile_scene(source)
 def test_cli_tbs_and_atomic_failure(self):
  tool=Path(__file__).resolve().parents[1]/'cutbuild.py'
  tbs_tool=tool.parent.parent/'toolbase/toolbase.py'
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);src=root/'scene source.txt';out=root/'scene output.cut'
   src.write_text('<SCRIPT NAME="MAIN">\n{\nWAIT 0\n}\n')
   subprocess.run([sys.executable,str(tool),str(src),str(out)],check=True,capture_output=True)
   expected=out.read_bytes();out.unlink()
   tbs=root/'build.tbs';tbs.write_text('CUTBUILD "scene source.txt" "scene output.cut"\n')
   subprocess.run([sys.executable,str(tbs_tool),str(tbs)],check=True,capture_output=True)
   self.assertEqual(out.read_bytes(),expected)
   subprocess.run([sys.executable,str(tbs_tool),'CUTBUILD',str(src),str(out)],check=True,capture_output=True)
   self.assertEqual(out.read_bytes(),expected)
   src.write_text('bad input')
   result=subprocess.run([sys.executable,str(tool),str(src),str(out)],capture_output=True)
   self.assertNotEqual(result.returncode,0);self.assertIn(b'line 1',result.stderr);self.assertEqual(out.read_bytes(),expected)
if __name__=='__main__':unittest.main()
