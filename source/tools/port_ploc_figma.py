#!/usr/bin/env python3
"""Flatten Nicolas's Figma SVG vectors into the OP-1's restricted SVG profile.
Original downloaded vectors stay unmodified in plugins/ploc/figma-assets/.
GPL-3.0-or-later, with firmware permission (NOTICE).
"""
from pathlib import Path
import math,re,xml.etree.ElementTree as ET
P=Path(__file__).resolve().parents[1]/'plugins/ploc'
OUT=[]
def f(x):
    s=f'{x:.4f}'.rstrip('0').rstrip('.');return s if s not in ('','-0') else '0'
def path(d,dx,dy):
    tokens=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?',d)
    i=0;x=y=0;current=[];paths=[]
    while i<len(tokens):
        cmd=tokens[i];i+=1
        if cmd=='M':
            if current:paths.append(' '.join(current));current=[]
            x,y=map(float,tokens[i:i+2]);i+=2;current+=['M',f(x+dx),f(y+dy)]
        elif cmd=='L':
            x,y=map(float,tokens[i:i+2]);i+=2;current+=['L',f(x+dx),f(y+dy)]
        elif cmd in ('H','V'):
            v=float(tokens[i]);i+=1
            if cmd=='H':x=v
            else:y=v
            current+=['L',f(x+dx),f(y+dy)]
        elif cmd=='C':
            vals=list(map(float,tokens[i:i+6]));i+=6
            current+=['C']+[f(v+(dx if j%2==0 else dy)) for j,v in enumerate(vals)]
            x,y=vals[-2:]
        elif cmd in ('Z','z'):current+=['z']
        else:raise ValueError('Unsupported source command: '+cmd)
    if current:paths.append(' '.join(current))
    return paths

def asset(name,dx,dy):
    root=ET.parse(P/'figma-assets'/(name+'.svg')).getroot();items=[]
    for node in root:
        tag=node.tag.split('}')[-1]
        if tag=='ellipse':
            cx,cy,rx,ry=(float(node.get(k)) for k in ('cx','cy','rx','ry'));k=.5522847498307936
            d=f'M {cx+rx} {cy} C {cx+rx} {cy+k*ry} {cx+k*rx} {cy+ry} {cx} {cy+ry} C {cx-k*rx} {cy+ry} {cx-rx} {cy+k*ry} {cx-rx} {cy} C {cx-rx} {cy-k*ry} {cx-k*rx} {cy-ry} {cx} {cy-ry} C {cx+k*rx} {cy-ry} {cx+rx} {cy-k*ry} {cx+rx} {cy} Z'
        elif tag=='path':d=node.get('d')
        else:raise ValueError(tag)
        stroke=node.get('stroke','none').lower()
        if stroke=='white':stroke='#ffffff'
        attrs={'stroke':stroke,'stroke-width':node.get('stroke-width','1'),'fill':'none'}
        for contour in path(d,dx,dy):
            items.append('<path d="'+contour+'" '+ ' '.join(k+'="'+v+'"' for k,v in attrs.items())+'/>')
    return ''.join(items)

def group(name,data):return '<g id="'+name+'">'+data+'</g>\n'
def main():
    svg='<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 160">\n'
    svg+=group('tailrule',asset('tail-rule',10,123.5))
    svg+=group('trajectory',''.join(asset('trajectory',159.25,y) for y in (13.25,23.25,33.25)))
    svg+=group('drop',asset('drop',147.25,48.25))
    svg+=group('ripple',asset('ripple-outer',108.25,123.25)+asset('ripple-inner',129.25,126.25))
    svg+=group('bodyring',asset('body-ring',238.25,16.25))
    svg+='</svg>\n'
    assert len(svg.encode())<8192
    (P/'ploc.svg').write_text(svg)
    print('Native Figma artwork:',len(svg.encode()),'bytes')

if __name__=='__main__':main()
