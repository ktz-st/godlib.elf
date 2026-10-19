#!/usr/bin/env python3
"""Reproducible small arrow: rotations, masks and a legacy ANIM8R PI1 sheet."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'toolbase'))
import toolbase as t
ROOT=Path(__file__).resolve().parent
DEMO=ROOT/'demo'
DEMO.mkdir(exist_ok=True)
pal=[0x012,0xF40,0xFF0,0xFFF]+[0]*12
palette=b''.join(t.u16be(v)for v in pal)

def pi1(pixels):
    data=bytearray(32000)
    for y in range(200):
        for x in range(320):
            c=pixels[y*320+x]
            for p in range(4):
                if c&(1<<p):
                    off=y*160+(x//16)*8+p*2
                    struct.pack_into('>H',data,off,struct.unpack_from('>H',data,off)[0]|(0x8000>>(x&15)))
    return bytes(2)+palette+data

pixels=[0]*64000
for y in range(16):
    for x in range(24):
        # Rightward arrow with a gold centre and white glint.
        if (2<=x<=13 and 5<=y<=10) or (12<=x<=21 and abs(y-7.5)<=((22-x)*.8)):
            pixels[y*320+x]=3 if y==6 and 4<=x<=11 else (2 if 6<=y<=8 else 1)
(DEMO/'ARROW.PI1').write_bytes(pi1(pixels))
t.maskmake(DEMO/'ARROW.PI1',DEMO/'MASK.PI1')
(DEMO/'ROTATE.SCR').write_text('[ SPRITE ]\n{\nsrc = ARROW.PI1\nmsk = MASK.PI1\ndst = ROTATE.BSB\nx = 0\ny = 0\nwidth = 24\nheight = 16\nplanes = 4\nrotframecount = 32\nrotstartangle = 0\nrotendangle = 6.283185307179586\nrotpivotx = 12\nrotpivoty = 8\n}\n')
t.bsbmake(DEMO/'ROTATE.SCR')
(DEMO/'ROTATE.PAL').write_bytes(palette)
spec=t.parse_scr(DEMO/'ROTATE.SCR')[1][0]
_,gfx=t.read_pi1(DEMO/'ARROW.PI1');_,mask=t.read_pi1(DEMO/'MASK.PI1')
frames=t.rotate_sprites(t.make_sprite(gfx,mask,spec,0),spec)
w,h=frames[0].width,frames[0].height
# A 32-pixel frame fits ten per row and four rows inside a PI1.
assert w==32 and h<=50
sheet=[0]*64000
for n,f in enumerate(frames):
    values,transparent=t.sprite_pixels(f);ox=(n%10)*w;oy=(n//10)*h
    for y in range(h):
        for x in range(w):
            if not transparent[y*w+x]:sheet[(oy+y)*320+ox+x]=values[y*w+x]
(DEMO/'SHEET.PI1').write_bytes(pi1(sheet))
(DEMO/'ANIM.TXT').write_text(f'[GRAPHIC]\nSHEET.PI1\n[WIDTH]\n{w}\n[HEIGHT]\n{h}\n[FRAMECOUNT]\n32\n[FRAMES]\n'+''.join(f'{i}\n'for i in range(32))+'[TIME]\n3\n[EOF]\n')
print('Prepared demo/ROTATE.BSB and demo/ANIM.TXT')
