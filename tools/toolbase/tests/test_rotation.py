#!/usr/bin/env python3
"""Pixel-level rotation checks, including masks and script-to-BSB integration."""
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import toolbase as t


class Rotation(unittest.TestCase):
    def test_quarter_turns_and_exclusive_end(self):
        src=t.planar_sprite(3,2,4,4,[1,2,3,4,5,6],[False]*6)
        spr=t.SpriteDef(width=3,height=2,rotframecount=4,rotendangle=math.tau)
        frames=t.rotate_sprites(src,spr)
        self.assertEqual([(f.width,f.height) for f in frames],[(16,5)]*4)
        expected=[{(2,2):1,(3,2):2,(4,2):3,(2,3):4,(3,3):5,(4,3):6},
                  {(2,2):1,(2,3):2,(2,4):3,(1,2):4,(1,3):5,(1,4):6},
                  {(2,2):1,(1,2):2,(0,2):3,(2,1):4,(1,1):5,(0,1):6},
                  {(2,2):1,(2,1):2,(2,0):3,(3,2):4,(3,1):5,(3,0):6}]
        for f,want in zip(frames,expected):
            pix,mask=t.sprite_pixels(f)
            got={(i%f.width,i//f.width):v for i,v in enumerate(pix) if not mask[i]}
            self.assertEqual(got,want)

    def test_mask_including_opaque_black(self):
        src=t.planar_sprite(2,2,4,4,[0,7,5,0],[False,True,False,True])
        spec=t.SpriteDef(width=2,height=2,rotframecount=1,rotstartangle=math.pi/2)
        f=t.rotate_sprites(src,spec)[0];pix,mask=t.sprite_pixels(f)
        self.assertEqual([(i%f.width,i//f.width,pix[i])for i in range(len(pix))if not mask[i]],[(0,0,5),(1,0,0)])
        self.assertTrue(all(mask[x] for x in range(2,16)))

    def test_arbitrary_rotation_has_no_interior_holes(self):
        src=t.planar_sprite(32,24,2,4,[3]*(32*24),[False]*(32*24))
        spec=t.SpriteDef(width=32,height=24,rotframecount=9,rotendangle=math.tau,rotpivotx=16,rotpivoty=12)
        frames=t.rotate_sprites(src,spec)
        self.assertEqual(len({(f.width,f.height)for f in frames}),1)
        for f in frames:
            pix,mask=t.sprite_pixels(f)
            for y in range(f.height):
                xs=[x for x in range(f.width)if not mask[y*f.width+x]]
                if xs:self.assertEqual(xs,list(range(min(xs),max(xs)+1)))
            self.assertEqual({v for v,m in zip(pix,mask)if not m},{3})

    def test_legacy_rot90_is_transpose_not_clockwise(self):
        src=t.planar_sprite(3,2,2,4,[1,2,3,0,1,2],[False,False,True,True,False,False])
        f=t.legacy_rot90(src)
        self.assertEqual((f.width,f.height),(2,3))
        p,m=t.sprite_pixels(f)
        self.assertEqual(p,[1,0,2,1,0,2]);self.assertEqual(m,[False,True,False,False,True,False])

    def test_script_rotation_and_bfb(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);pixels=bytearray(32000)
            struct.pack_into('>4H',pixels,8,0x8000,0x4000,0,0)
            (root/'image.pi1').write_bytes(bytes(34)+pixels)
            t.maskmake(root/'image.pi1',root/'mask.pi1')
            (root/'make.scr').write_text('[ SPRITE ]\n{\nsrc = image.pi1\nmsk = mask.pi1\ndst = result.bsb\nx = 16\nwidth = 2\nheight = 1\nrotframecount = 4\nrotendangle = 6.283185307179586\nrotpivotx = 1\nrotpivoty = 0\n}\n[ FONT ]\n{\nsrc = image.pi1\nmsk = mask.pi1\ndst = result.bfb\nx = 16\nwidth = 2\nheight = 1\nrot90 = 1\ncharmap = A\n}\n')
            t.bsbmake(root/'make.scr')
            b=(root/'result.bsb').read_bytes()
            self.assertEqual(b[:8],b'BSBK\0\0\0\4')
            offsets=struct.unpack_from('>4I',b,8)
            for o in offsets:
                go,mo,w,h,p,mp=struct.unpack_from('>II4H',b,o)
                self.assertEqual((w,h,p,mp),(16,3,4,4));self.assertEqual(mo,16)
                self.assertEqual(go,16+6)
            f=(root/'result.bfb').read_bytes();self.assertEqual(f[:4],b'TNOF')
            o=struct.unpack_from('>I',f,24)[0]
            self.assertEqual(struct.unpack_from('>2H',f,o+8),(1,2))

    def test_invalid_rotation_limits(self):
        src=t.planar_sprite(1,1,1,4,[1],[False])
        for count,angle in [(0,0),(4097,0),(1,float('nan')),(1,float('inf'))]:
            with self.assertRaises(ValueError):t.rotate_sprites(src,t.SpriteDef(width=1,height=1,rotframecount=count,rotendangle=angle))

if __name__=='__main__':unittest.main()
