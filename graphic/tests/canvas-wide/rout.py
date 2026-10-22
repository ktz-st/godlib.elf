"""Model of Graphic_4BP_Blit (grf_4_s.s) set-up: which jump-table routine a blit takes."""
import json, collections
NAMES = ['RTS','R','C','CR','L','LR','LC','LCR']
def rout(sx, dx, w):
    d3 = 0
    srcw = ((sx + w - 1) >> 4) - (sx >> 4)
    dx1 = dx + w - 1
    dstw = (dx1 >> 4) - (dx >> 4)
    centre = ((dx1 + 1) >> 4) - ((dx + 15) >> 4)
    if centre > 0: d3 |= 2
    s, d, d5 = sx & 15, dx & 15, dx1 & 15
    if s > d and srcw > 0: srcw -= 1; d3 |= 16
    if d5 != 15: d3 |= 1
    if dstw > srcw: d3 |= 32
    if d: d3 |= 4
    if s == d: d3 |= 8
    if dstw == 0 and not d3 & 2: d3 = (d3 & ~1) | 4
    suf = ('_A' if d3 & 8 else '') + ('_F' if d3 & 16 else '') + ('_N' if d3 & 32 else '')
    if d3 & 48 == 48: suf = suf.replace('_F_N', '_FN')
    if d3 & 8 and d3 & 48: suf += '(ALIGNED+F/N)'
    return NAMES[d3 & 7] + suf
def cases():
    sxs=[0,1,7,8,15,16,100,180,200,344,350,500,511,512,513,600,640,655]
    dxs=[0,3,8,15,16,40,180,190,200,300]
    ws=[1,5,8,12,16,17,24,31,32,33,48,64,80,96]
    for a,sx in enumerate(sxs):
        for b,dx in enumerate(dxs):
            for c,w in enumerate(ws):
                if sx+w>720: continue
                yield (sx,dx,w,16)
                if dx+w<=320: yield (sx,dx,w,16-(c&3))
if __name__ == '__main__':
    r = json.load(open('results.json'))
    bad = collections.Counter(rout(l[1], l[2], l[3]) for l in r['blit cpu']['log'])
    tot = collections.Counter(rout(sx, dx, w) for sx, dx, w, h in cases())
    for k in sorted(tot): print(f'{k:14} {bad[k]:4}/{tot[k]}')
    print(len(r['blit cpu']['log']))
