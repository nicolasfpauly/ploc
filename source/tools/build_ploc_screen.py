#!/usr/bin/env python3
"""PLOC 0.3 native screen after Nicolas's annotated sketch, 2026-11-06.
The principal drop retains his Figma vector, adapted to the new composition.
Open, original vectors only. GPL-3.0-or-later with firmware permission.
"""
from pathlib import Path
import importlib.util,re
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'plugins/ploc'
spec=importlib.util.spec_from_file_location('figma_port',ROOT/'tools/port_ploc_figma.py');figma=importlib.util.module_from_spec(spec);spec.loader.exec_module(figma)
f=figma.f

def path(d,color,width=1.5):return '<path d="'+d+'" fill="none" stroke="'+color+'" stroke-width="'+f(width)+'"/>'
def group(name,items):return '<g id="'+name+'">'+items+'</g>\n'
def ellipse(cx,cy,rx,ry,color):
    k=.5522847498307936
    coords=f'M {cx+rx} {cy} C {cx+rx} {cy+k*ry} {cx+k*rx} {cy+ry} {cx} {cy+ry} C {cx-k*rx} {cy+ry} {cx-rx} {cy+k*ry} {cx-rx} {cy} C {cx-rx} {cy-k*ry} {cx-k*rx} {cy-ry} {cx} {cy-ry} C {cx+k*rx} {cy-ry} {cx+rx} {cy-k*ry} {cx+rx} {cy} z'
    vals=re.findall('[A-Za-z]|[-+]?(?:\\d*\\.\\d+|\\d+)',coords)
    return path(' '.join(v if v.isalpha() else f(float(v)) for v in vals),color)
blue='#698eff';green='#00ed95';white='#ffffff';orange='#ff423f'
svg='<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 160">\n'
svg+=group('track',path('M 10 144 L 72 144','#4c4c4c',1.5))
# Flatten a 2.5x / 1.9x adaptation of the downloaded principal drop.
d='M 160 29.875 C 147.5 48.875 130 69.775 130 84.975 C 130 119.175 190 119.175 190 84.975 C 190 69.775 172.5 48.875 160 29.875 z'
svg+=group('drop',path(d,blue))
svg+=group('gloss',path('M 165 48 C 173 54 180 65 181 74 C 182 82 176 83 174 75 C 171 64 170 57 165 48 z',white,1.1)+path('M 140 90 C 141 98 146 103 151 106',white,1.4))
svg+=group('ripple',ellipse(160,127,24,5,green)+ellipse(160,127,48,10,green)+ellipse(160,127,72,15,green))
svg+=group('viscbar',path('M 10 144 L 72 144',white,1.8))
positions=[(105,52),(219,47),(92,81),(225,82),(102,105)]
for j,(x,y) in enumerate(positions):
    # Small orange bubbles/splash satellites, separate immutable contours.
    d=f'M {x-4} {y-4} C {x-1} {y-7} {x+5} {y-3} {x+5} {y+2} C {x+5} {y+7} {x-1} {y+6} {x-4} {y+2} C {x-6} {y} {x-6} {y-2} {x-4} {y-4} z'
    svg+=group('extra'+str(j),path(d,orange,1.5))
    cx=240+j*10
    svg+=group('count'+str(j),path(f'M {cx-1.5} 94 L {cx+1.5} 94',orange,2)+path(f'M {cx} 92.5 L {cx} 95.5',orange,2))
svg+='</svg>\n'
assert len(svg.encode())<8192
(P/'ploc.svg').write_text(svg)
print('PLOC 0.3 artwork:',len(svg.encode()),'bytes; five extra bubbles + five count marks')
