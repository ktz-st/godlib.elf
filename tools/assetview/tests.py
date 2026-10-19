#!/usr/bin/env python3
"""Exercise the compiled Cocoa decoder/exporter with independent binary fixtures."""
from pathlib import Path
import json
import platform
import struct
import subprocess
import tempfile
import unittest
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
EXE=ROOT/'bin'/platform.system()/platform.machine()/'assetview'


def word(n):return struct.pack('>H',n)
def long(n):return struct.pack('>I',n)
def sprite():
    # 3x2, four colours, explicit transparency and an opaque colour-zero pixel.
    return struct.pack('>II4H',20,16,3,2,2,4)+word(0x4000)+word(0x2000)+word(0x4000)+word(0x2000)+word(0x8000)+word(0xC000)

def block(magic=b'BSBK'):return magic+word(0)+word(1)+long(12)+sprite()
def font(magic=b'TNOF'):
    # One glyph A, sprite-relative data, font-relative table/region/charmap.
    header=magic+long(0)+bytes([65,65,2,4])+word(3)+word(2)+word(1)+word(0)+long(68)+long(32)+long(48)
    s=struct.pack('>II4H',28,24,3,2,2,4)
    return header+s+struct.pack('>4H',0,0,2,1)+sprite()[16:]+bytes([0])

class Decoder(unittest.TestCase):
    def setUp(self):self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def run_cli(self,*args,ok=True):
        r=subprocess.run([str(EXE),*map(str,args)],capture_output=True,text=True)
        if ok:self.assertEqual(r.returncode,0,r.stderr)
        else:self.assertEqual(r.returncode,1,r.stderr)
        return r
    def test_both_bsb_magic_variants_and_rgba(self):
        pal=self.root/'palette.pal';pal.write_bytes(b''.join(word(i*0x111)for i in range(16)))
        # STE low bit is at bit 3: 0x111 = RGB 34, not RGB 17.
        for magic in (b'BSBK',b'KBSB'):
            src=self.root/'file.bsb';src.write_bytes(block(magic));out=self.root/'out.png'
            info=json.loads(self.run_cli('--inspect',src).stdout)
            self.assertEqual(info['frames'],[dict(width=3,height=2,planes=2,mask_planes=4)])
            self.run_cli('--export',src,out,pal)
            image=Image.open(out).convert('RGBA')
            self.assertEqual(image.size,(3,2))
            self.assertEqual(list(image.get_flattened_data()),[(0,0,0,255),(0,0,0,0),(68,68,68,255),(102,102,102,255),(68,68,68,255),(0,0,0,0)])
    def test_both_font_magic_variants(self):
        for magic in (b'TNOF',b'FONT'):
            src=self.root/'font.bfb';src.write_bytes(font(magic));info=json.loads(self.run_cli('--inspect',src).stdout)
            self.assertEqual((info['type'],info['char_first'],info['char_last']),('BFB',65,65))
            self.run_cli('--export',src,self.root/'glyph.png')
    def test_maskless_legacy_sprite_does_not_read_graphics_as_mask(self):
        s=struct.pack('>II4H',16,0,3,1,1,0)+word(0xA000)
        src=self.root/'opaque.bsb';src.write_bytes(b'KBSB'+word(0)+word(1)+long(12)+s)
        self.run_cli('--export',src,self.root/'opaque.png')
        self.assertEqual([p[3]for p in Image.open(self.root/'opaque.png').convert('RGBA').get_flattened_data()],[255]*3)
    def test_anim8r_sequence_speed_and_pi1_palette(self):
        src=self.root/'SHEET.PI1';pixels=bytearray(32000)
        # Pixel in the second 16-pixel cell (frame 1).
        struct.pack_into('>4H',pixels,8,0x8000,0,0,0)
        src.write_bytes(word(0)+word(0)+word(0xF00)+bytes(28)+pixels)
        script=self.root/'ANIM.TXT';script.write_text('[GRAPHIC]\nsheet.pi1\n[WIDTH]\n16\n[HEIGHT]\n8\n[FRAMECOUNT]\n4\n[FRAMES]\n1\n0\n1\n1\n[TIME]\n5\n[EOF]\n')
        info=json.loads(self.run_cli('--inspect',script).stdout)
        self.assertEqual(info['sequence'],[1,0,1,1]);self.assertEqual(info['fps'],10)
        self.run_cli('--export',script,self.root/'frame.png',src,1)
        im=Image.open(self.root/'frame.png').convert('RGBA');self.assertEqual(im.getpixel((0,0)),(255,0,0,255))
    def test_malformed_files_fail_cleanly(self):
        for data in (b'',b'BSBK',b'BSBK'+bytes(8),b'BSBK\0\0\0\1'+long(0xFFFFFFF0),font()[:-1],block()[:-1],b'RNC\1'+bytes(60)):
            p=self.root/'bad';p.write_bytes(data);r=self.run_cli('--inspect',p,ok=False)
            self.assertIn('assetview:',r.stderr)
        broken=bytearray(font());broken[-1]=2;p=self.root/'bad';p.write_bytes(broken);self.run_cli('--inspect',p,ok=False)
    def test_existing_repository_assets(self):
        workspace=ROOT.parent
        fixtures=[workspace/'niezwyciezony/gfx/font6x6.bfb',workspace/'GOD/TOOLS.RG/BSBMAKER/PRG/ROCKANIM.BSB',workspace/'GOD/TOOLS.RG/TILEEDIT/GFX/DST/GUI/FONT6X6.BFB']
        for src in fixtures:
            if not src.exists():continue
            info=json.loads(self.run_cli('--inspect',src).stdout)
            self.assertGreater(len(info['frames']),0);self.run_cli('--export',src,self.root/'real.png')

if __name__=='__main__':unittest.main()
