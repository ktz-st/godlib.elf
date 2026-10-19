#!/usr/bin/env python3
from pathlib import Path
import importlib.util,struct,sys,tempfile,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import febuild as f

def chunk(kind,name,body=''):return f'<{kind} name={name}>\n{{\n{body}\n}}\n'
def field(body,key,val):return body+f'\n{key} = {val}'
def u16(b,o):return struct.unpack_from('>H',b,o)[0]
def u32(b,o):return struct.unpack_from('>I',b,o)[0]
class Compiler(unittest.TestCase):
 def test_forward_references_header_and_lists(self):
  text=chunk('PAGE','MAIN','CONTROL_LIST = BUTTONS')+chunk('CONTROLLIST','BUTTONS','CONTROL = NEXT\nCONTROL = EXIT\nINDEX = 1')+chunk('CONTROL','NEXT','CONTROL_TYPE = LINK\nLINK = OPTIONS')+chunk('CONTROL','EXIT','CONTROL_TYPE = CALL\nCALL = QUIT')+chunk('PAGE','OPTIONS','PAGE_PARENT = MAIN')+chunk('CALL','QUIT','CALL_VAR = ACTION\nCALL_VALUE = 2')+chunk('VAR','ACTION','NAME = "GAME/ACTION"')
  b=f.compile_text(text);self.assertEqual(b[:8],b'FEDS\0\0\0\2');self.assertEqual(u16(b,96),2)
  pages=u32(b,40);self.assertEqual(u32(b,pages),0x515de)
  controls=u32(b,16);cl=u32(b,20);array=u32(b,cl+4)
  self.assertEqual([u32(b,array+i*4)for i in range(2)],[controls,controls+44])
  self.assertEqual(u32(b,controls+20),pages+44)
  self.assertEqual(u16(b,cl+2),1)
 def test_strings_signed_values_and_palette_indices(self):
  text=chunk('PAGE','MAIN')+chunk('TEXT','TITLE','STRING = "Hello world"\nX = -12\nALIGN = CENTRE')+chunk('TRANSITION','FADE','FADE_TYPE = BG\nFADE_COLOUR = 3\nFADE_FRAME_COUNT = 10')
  b=f.compile_text(text);textbase=u32(b,68);self.assertEqual(struct.unpack_from('>h',b,textbase)[0],-12)
  ptr=u32(b,textbase+12);self.assertEqual(b[ptr:ptr+12],b'Hello world\0')
  trans=u32(b,72);self.assertEqual(u16(b,trans+2),3)
 def test_case_and_slash_hash_compatibility(self):
  self.assertEqual(f.hash_name('game\\main'),f.hash_name('GAME/MAIN'))
  with self.assertRaisesRegex(ValueError,'duplicate'):f.compile_text(chunk('PAGE','main')+chunk('PAGE','MAIN'))
 def test_invalid_scripts_rejected(self):
  bad=[chunk('CONTROL','X'),chunk('PAGE','MAIN','TYPO = X'),chunk('PAGE','MAIN','CONTROL_LIST = MISSING'),'<PAGE name=MAIN>\n{\n',chunk('PAGE','MAIN')+chunk('TEXT','T','X = 40000'),chunk('PAGE','MAIN')+chunk('TEXT','T','ALIGN = MIDDLE'),chunk('PAGE','MAIN')+chunk('CONTROLLIST','C'),chunk('PAGE','MAIN')+chunk('CONTROL','C','CONTROL_TYPE = LINK'),chunk('PAGE','MAIN')+chunk('SLIDER','S','VALUE_MIN = 100\nVALUE_MAX = 10')]
  for text in bad:
   with self.subTest(text=text),self.assertRaises(ValueError):f.compile_text(text)
 def test_all_chunks_parse_and_offsets_are_even(self):
  text=chunk('PAGE','MAIN')
  for kind,schema in f.CHUNKS.items():
   if kind=='PAGE':continue
   body=''
   if kind=='CONTROLLIST':body='CONTROL = C'
   if kind=='LIST':body='ITEM = I'
   if kind=='SPRITELIST':body='SPRITE = S'
   if kind=='SLIDER':body='VALUE_MAX = 100'
   text+=chunk(kind,'OBJ',body)
  text+=chunk('CONTROL','C')+chunk('LISTITEM','I')+chunk('SPRITE','S')
  b=f.compile_text(text)
  for kind,schema in f.CHUNKS.items():
   ptr=u32(b,schema['table']);self.assertEqual(ptr%2,0);self.assertGreater(u16(b,schema['count']),0)
 def test_toolbase_batch_and_determinism(self):
  sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'toolbase'));import toolbase
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);text=chunk('PAGE','MAIN')
   (root/'menu source.txt').write_text(text);(root/'build.tbs').write_text('FEBUILD "menu source.txt" "menu output.fed"\n')
   toolbase.run_tbs(root/'build.tbs');self.assertEqual((root/'menu output.fed').read_bytes(),f.compile_text(text))
if __name__=='__main__':unittest.main()
