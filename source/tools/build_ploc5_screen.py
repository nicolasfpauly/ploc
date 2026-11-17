#!/usr/bin/env python3
"""Retained SVG geometry for Nicolas's selected faucet/drop design."""
from pathlib import Path
import math,re
ROOT=Path(__file__).resolve().parents[1]
B='#698eff';G='#00ed95';W='#ffffff';R='#ff423f'
def num(x):return f'{x:.2f}'.rstrip('0').rstrip('.')
def xy(p):return ' '.join(map(num,p))
def path(d,color,width=1.35,fill='none'):
 return f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{num(width)}"/>'

def group(name,body):return f'<g id="{name}">{body}</g>'
def line(x,y,a,b,c,w=1.35):return path('M'+xy((x,y))+'L'+xy((a,b)),c,w)
def ellipse(cx,cy,rx,ry,c,fill='none',width=1.35):
 k=.55228475
 d='M'+xy((cx+rx,cy))+'C'+xy((cx+rx,cy+k*ry))+' '+xy((cx+k*rx,cy+ry))+' '+xy((cx,cy+ry))+'C'+xy((cx-k*rx,cy+ry))+' '+xy((cx-rx,cy+k*ry))+' '+xy((cx-rx,cy))+'C'+xy((cx-rx,cy-k*ry))+' '+xy((cx-k*rx,cy-ry))+' '+xy((cx,cy-ry))+'C'+xy((cx+k*rx,cy-ry))+' '+xy((cx+rx,cy-k*ry))+' '+xy((cx+rx,cy))+'Z'
 return path(d,c,width,fill)
def lerp(a,b,t):return tuple(x+(y-x)*t for x,y in zip(a,b))
def split(p,t):
 a,b,c=[lerp(p[i],p[i+1],t) for i in range(3)];d,e=lerp(a,b,t),lerp(b,c,t);f=lerp(d,e,t)
 return [p[0],a,d,f],[f,e,c,p[3]]
def segment(p,t0,t1):
 left=split(p,t1)[0]
 return split(left,t0/t1)[1] if t0 else left

def build():
 old=(ROOT/'plugins/ploc/ploc_legacy.svg').read_text()
 legacy=re.sub(r'^.*?<svg[^>]*>','',old,flags=re.S).rsplit('</svg>',1)[0]
 face=''
 for i in range(5):
  a=math.pi+i*math.pi/4
  face+=line(46+math.cos(a)*23,62+math.sin(a)*23,46+math.cos(a)*29,62+math.sin(a)*29,W,1.25)
 face+=line(45,62,47,62,B,1.4)+line(46,61,46,63,B,1.4)
 face+=line(237,43,289,42,'#555555',1.2)
 for i in range(4):face+=line(246+i*10,104,247+i*10,109,W,1.6)
 body=group('v5static',line(113,12,204,11,B,1.4)+face)
 body+=group('v5nozzle',path('M144 12L144 32C154.67 33.33 165.33 33.33 176 32L176 12',B,1.5))
 body+=group('v5needle',line(46,62,23,62,B,1.65))
 rings=''.join(group('v5ring'+str(i),f'<circle cx="160" cy="132" r="{rx}" fill="none" stroke="{R}" stroke-width="1.35"/>') for i,rx in enumerate([6,22,40,60]))
 body+=group('v5ripples',rings)
 for i,(dx,dy) in enumerate([(-25,-5),(25,-5),(-34,-10),(34,-10),(-44,-3),(44,-3),(-51,-14),(51,-14)]):body+=group('v5spray'+str(i),line(160+dx,123+dy,160+dx+(-3 if dx<0 else 3),119+dy,B))
 # A uniform parent transform preserves the reflected curve inside the outline.
 def dxy(x,y):return xy((124+x*.72,y/1.2))
 drop=path('M'+dxy(50,1)+'C'+dxy(62,25)+' '+dxy(99,53)+' '+dxy(99,84)+'C'+dxy(99,132)+' '+dxy(1,132)+' '+dxy(1,84)+'C'+dxy(1,53)+' '+dxy(38,25)+' '+dxy(50,1)+'Z',G,2.7,'#000000')
 curve=[(124+.58*72,28/1.2),(124+.76*72,42/1.2),(124+.87*72,66/1.2),(124+.83*72,84/1.2)]
 for i in range(8):
  p=segment(curve,i*.96/8,(i+1)*.96/8)
  drop+=group('v5reflect'+str(i),path('M'+xy(p[0])+'C'+xy(p[1])+' '+xy(p[2])+' '+xy(p[3]),W,2.1))
 body+=group('v5drop',drop)
 body+=group('v5point',line(182.96,75.83,184.56,75.83,W,1.2)+line(183.76,75.03,183.76,76.63,W,1.2))
 body+=group('v5viscbar',line(237,43,289,42,W,1.4))
 for i in range(4):body+=group('v5count'+str(i),line(246+i*10,104,247+i*10,109,R,1.6))
 out='<svg xmlns="http://www.w3.org/2000/svg" width="320" height="160" viewBox="0 0 320 160">'+body+group('ploclegacy',legacy)+'</svg>'
 dest=ROOT/'plugins/ploc/ploc.svg';dest.write_text(out+'\n');print('SVG',len(out.encode()),'bytes')
 if len(out.encode())>8192:raise ValueError('Unexpected SVG size')
 return dest
if __name__=='__main__':build()
