#!/usr/bin/env python3
"""Adapt the owner's final Figma vectors to R246, keeping original downloads.

Retain the supplied curves when compact, simplify sampled contours below one
OLED pixel, and reuse the same solid handle geometry for all twelve poses.
Full font files are never copied into the plugin.
"""
from pathlib import Path
import json, math, os, re, sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
SDK=Path(os.environ.get('PLOC_SDK_SOURCE',ROOT/'sdk/hundekraft-3.1.4-source'))/'ext/kennel/synth_sdk'
sys.path.insert(0,str(SDK/'display'))
import svgart
ASSETS=ROOT/'plugins/ploc/figma-assets/v6'
S=320/486
RING_Y=135
RING_DEPTH=.75
NS='http://www.w3.org/2000/svg'
ET.register_namespace('',NS)
B='#638bff'; G='#00ed95'; W='#ffffff'; R='#ff423f'; BLACK='#010101'
LAYOUT=json.loads((ROOT/'design/figma-0.6/layout.json').read_text())
META={n['id']:n for n in LAYOUT['nodes']}
PAINTS=[]; PLACEMENTS=[]

def f(x,precision=1):
    t=f'{x:.{precision}f}'.rstrip('0').rstrip('.')
    return t if t not in ('','-0') else '0'

def color(c,alpha=1):
    if not c or c=='none':return 'none'
    c={'white':'#ffffff','black':'#000000'}.get(c.lower(),c)
    if len(c)==4:c='#'+''.join(x*2 for x in c[1:])
    if not re.fullmatch('#[0-9a-fA-F]{6}',c):raise ValueError(c)
    values=[round(int(c[i:i+2],16)*alpha) for i in (1,3,5)]
    if not any(values):return BLACK
    return '#'+''.join(f'{v:02x}' for v in values)

def compose(a,b):
    x,y,z,t,u,v=a;A,B,C,D,E,F=b
    return (x*A+z*B,y*A+t*B,x*C+z*D,y*C+t*D,x*E+z*F+u,y*E+t*F+v)

def matrix(text):
    m=(1,0,0,1,0,0)
    for op,args in re.findall(r'(\w+)\(([^)]*)\)',text or ''):
        v=[float(n) for n in re.split(r'[\s,]+',args.strip())]
        if op=='scale':a=(v[0],0,0,v[-1],0,0)
        elif op=='translate':a=(1,0,0,1,v[0],v[1] if len(v)>1 else 0)
        elif op=='matrix':a=tuple(v)
        elif op=='rotate':
            c=math.cos(math.radians(v[0]));s=math.sin(math.radians(v[0]));a=(c,s,-s,c,0,0)
            if len(v)==3:a=compose(compose((1,0,0,1,v[1],v[2]),a),(1,0,0,1,-v[1],-v[2]))
        else:raise ValueError(op)
        m=compose(m,a)
    return m

def apply(m,p):
    a,b,c,d,e,f=m;x,y=p
    return (a*x+c*y+e,b*x+d*y+f)

def dist(p,a,b):
    x,y=p;u,v=a;dx,dy=b[0]-u,b[1]-v
    if dx==dy==0:return math.hypot(x-u,y-v)
    t=max(0,min(1,((x-u)*dx+(y-v)*dy)/(dx*dx+dy*dy)))
    return math.hypot(x-u-t*dx,y-v-t*dy)

def rdp(p,tol=.2):
    if len(p)<3:return p
    ds=[dist(x,p[0],p[-1]) for x in p[1:-1]];v=max(ds,default=0)
    if v<=tol:return [p[0],p[-1]]
    k=ds.index(v)+1
    return rdp(p[:k+1],tol)[:-1]+rdp(p[k:],tol)

def simplify(p,closed):
    p=[v for i,v in enumerate(p) if i==0 or v!=p[i-1]]
    if closed and p[-1]==p[0]:p=p[:-1]
    if closed and len(p)>3:
        k=max(range(1,len(p)),key=lambda k:math.dist(p[0],p[k]))
        q=rdp(p[:k+1])[:-1]+rdp(p[k:]+p[:1])[:-1]
        if len(q)>=3:p=q
    elif not closed:p=rdp(p)
    return svgart.rounded(p,closed)

def area(p):return abs(sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(p,p[1:]+p[:1]))/2)
def inside(p,q):
    x,y=p;result=False
    for a,b in zip(q,q[1:]+q[:1]):
        if (a[1]>y)!=(b[1]>y) and x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]:result=not result
    return result

def geometry(n):
    tag=n.tag.rsplit('}',1)[-1];g=lambda k:float(n.get(k,0))
    if tag=='path':return svgart.subpaths(n.get('d',''))
    if tag in ('circle','ellipse'):
        rx,ry=(g('r'),g('r')) if tag=='circle' else (g('rx'),g('ry'))
        return [(svgart.ellipse(g('cx'),g('cy'),rx,ry),True)]
    if tag=='rect':
        x,y,w,h=g('x'),g('y'),g('width'),g('height')
        return [([(x,y),(x+w,y),(x+w,y+h),(x,y+h)],True)]
    if tag=='line':return [([(g('x1'),g('y1')),(g('x2'),g('y2'))],False)]
    if tag in ('polyline','polygon'):
        v=[float(x) for x in re.findall(r'-?\d*\.?\d+',n.get('points',''))]
        return [(list(zip(v[::2],v[1::2])),tag=='polygon')]
    raise ValueError(tag)

def bezier_paths(d,tf):
    """Keep cubic Béziers in OLED space, expanding only the SVG shorthand."""
    tokens=svgart.TOKEN.findall(d);i=0;cmd=None;pos=(0.,0.);start=pos
    previous=None;out=[];path=[]
    def point(rel):
        nonlocal i
        p=(float(tokens[i]),float(tokens[i+1]));i+=2
        return (p[0]+pos[0],p[1]+pos[1]) if rel else p
    def add(c,*pts):path.append(c+' '.join(f(v) for p in pts for v in apply(tf,p)))
    while i<len(tokens):
        if tokens[i].isalpha():cmd=tokens[i];i+=1
        op=cmd.upper();rel=cmd.islower()
        if op=='M':
            if path:out.append(''.join(path));path=[]
            pos=start=point(rel);add('M',pos);cmd='l' if rel else 'L'
        elif op=='L':pos=point(rel);add('L',pos)
        elif op in ('H','V'):
            k=op=='V';v=float(tokens[i])+(pos[k] if rel else 0);i+=1
            pos=(pos[0],v) if k else (v,pos[1]);add('L',pos)
        elif op in ('C','S'):
            first=point(rel) if op=='C' else (pos if previous is None else (2*pos[0]-previous[0],2*pos[1]-previous[1]))
            second=point(rel);end=point(rel);add('C',first,second,end);previous=second;pos=end
            continue
        elif op=='Z':
            path.append('Z');out.append(''.join(path));path=[];pos=start;cmd=None
        else:raise ValueError(op)
        previous=None
    if path:out.append(''.join(path))
    return out

def solid_grip(name):
    # The original lathed profile: narrow neck, swelling grip, small rounded tip.
    d='M6 -2.9C8 -2.72 10 -2.55 12 -2.55C16 -2.55 19 -4.45 23 -4.45L28.5 -4.45C29.5 -4.45 30 -4.12 30 -3.8L30 3.8C30 4.12 29.5 4.45 28.5 4.45L23 4.45C19 4.45 16 2.55 12 2.55C10 2.55 8 2.72 6 2.9Z'
    path=bezier_paths(d,(S,0,0,S,0,0))[0]
    pts=simplify([(x*S,y*S) for x,y in svgart.subpaths(d)[0][0]],True)
    poly='M'+'L'.join(f(x)+' '+f(y) for x,y in pts)+'Z'
    d=min(path,poly,key=len)
    return '<path id="'+name+'" d="'+d+'" fill="'+BLACK+'" stroke="#698eff" stroke-width="1.05"/>'

def handle_matrices():
    # Orthographic projection of the Figma grip around the original hub. Pick
    # the cross-section tangent normal to each projected arm, then paint the
    # two front end faces over their opaque profiles in depth order.
    out=[];cx,cy=273.5*S,31.5*S;depth=.42
    for k in range(12):
        arms=sorted([(math.sin(a),math.cos(a)) for a in (k*math.pi/24+j*math.pi/2 for j in range(4))])
        matrices=[]
        for ss,cc in arms:
            ax,ay=cc,depth*ss;bx,by=-ss,depth*cc
            l=math.hypot(ax,ay);nx,ny=-ay/l,ax/l
            nb=nx*bx+ny*by;nv=-ny;scale=math.hypot(nb,nv)
            wx,wy=bx*nb/scale,(by*nb-nv)/scale
            matrices.append((ax,ay,wx,wy,cx,cy))
        for ss,cc in arms[2:]:
            matrices.append((-ss,depth*cc,0,-1,cx+30*S*cc,cy+30*S*depth*ss))
        out.append((matrices,arms[2][0]>.06))
    return out

def read(file,m,select=None):
    root=ET.parse(ASSETS/file).getroot();out=[]
    def walk(n,tf,style,selected=False):
        tag=n.tag.rsplit('}',1)[-1]
        if tag in ('defs','clipPath','style'):return
        selected=selected or select is None or select(n)
        st=dict(style)
        for k in ('fill','stroke','stroke-width'):st[k]=n.get(k,st[k])
        st['opacity']=style['opacity']*float(n.get('opacity','1'))
        tf=compose(tf,matrix(n.get('transform')))
        if tag in ('svg','g'):
            for ch in n:walk(ch,tf,st,selected)
            return
        if not selected:return
        geo=[(simplify([apply(tf,p) for p in points],closed),closed) for points,closed in geometry(n)]
        scale=math.sqrt(abs(tf[0]*tf[3]-tf[1]*tf[2]))
        curves=bezier_paths(n.get('d',''),tf) if tag=='path' else []
        out.append({'source':n.get('id',''),'stroke':color(st['stroke'],st['opacity']),'fill':color(st['fill'],st['opacity']),'width':float(st['stroke-width'])*scale,'geo':geo,'curves':curves if len(curves)==len(geo) else []})
    walk(root,m,{'fill':'none','stroke':'none','stroke-width':'1','opacity':1})
    return out

def group(name,children):return '<g id="'+name+'">'+''.join(children)+'</g>'

def emit(records,prefix=None):
    out=[];index=0
    for rec in records:
        polygons=[p for p,closed in rec['geo'] if len(p)>=3]
        for i,(p,closed) in sorted(enumerate(rec['geo']),key=lambda q:-area(q[1][0]) if q[1][1] else 0):
            if len(p)<2:continue
            fill=rec['fill'] if closed else 'none'
            if fill!='none' and sum(area(q)>area(p)+.0001 and inside(p[0],q) for q in polygons)%2:fill=BLACK
            ident=prefix+str(index) if prefix else None;index+=1
            d='M'+'L'.join(f(x)+' '+f(y) for x,y in p)+('Z' if closed else '')
            curves=rec.get('curves',[])
            if curves and 'C' in curves[i] and len(curves[i])<len(d) and (curves[i].endswith('Z')==closed):
                d=curves[i]
            attrs={'d':d,'fill':fill,'stroke':rec['stroke']}
            if rec['stroke']!='none':attrs['stroke-width']=f(rec['width'],2)
            if ident:attrs={'id':ident,**attrs}
            out.append(ET.tostring(ET.Element('path',attrs),encoding='unicode'))
            if ident and fill not in ('none',BLACK):PAINTS.append({'id':ident,'kind':'f','rgb':[int(fill[i:i+2],16) for i in (1,3,5)]})
            if ident and rec['stroke']!='none':PAINTS.append({'id':ident,'kind':'l','rgb':[int(rec['stroke'][i:i+2],16) for i in (1,3,5)]})
    return out

def placement(file,node_id):
    n=META[node_id];a=n['transform'];r=ET.parse(ASSETS/file).getroot()
    width,height=map(float,(r.get('width'),r.get('height')))
    x=(n['width']-width)/2;y=(n['height']-height)/2
    native=(a[0][0]*S,a[1][0]*S,a[0][1]*S,a[1][1]*S,(a[0][2]-18003)*S,(a[1][2]-586)*S)
    tf=compose(native,(1,0,0,1,x,y));PLACEMENTS.append({'asset':file,'nodeId':node_id,'nativeMatrix':tf,'originalRootWidth':width,'originalRootHeight':height})
    return tf

def rect(x,y,w,h,c):
    return {'source':'context-rectangle','stroke':'none','fill':c,'width':0,'geo':[([(x*S,y*S),((x+w)*S,y*S),((x+w)*S,(y+h)*S),(x*S,(y+h)*S)],True)]}

def main():
    PAINTS.clear();PLACEMENTS.clear();body=[]
    fixed=[]
    for x,y,w,h in [(37,104,10,2),(71,70,2,10),(97,104,10,2)]:fixed+=emit([rect(x,y,w,h,G)])
    # Transform from the final context's 45.46-degree / .35-degree-skewed tick.
    a=math.radians(45.46);k=math.tan(math.radians(.35));c=math.cos(a);s=math.sin(a)
    tf=(c*S,s*S,(c*k-s)*S,(s*k+c)*S,(46.58-2*(c*k-s))*S,80*S)
    tick={'stroke':'none','fill':G,'width':0,'geo':[([apply(tf,p) for p in [(0,0),(10,0),(10,2),(0,2)]],True)]}
    fixed+=emit([tick])
    for file,x,y in [('imgRectangle611.svg',89,80),('imgEllipse151.svg',67,101)]:fixed+=emit(read(file,(S,0,0,S,x*S,y*S)))
    fixed+=emit(read('imgGroup124.svg',placement('imgGroup124.svg','49:9483')))
    faucet_tf=(S,0,0,S,228*S,14*S)
    fixed_body=[r for r in read('faucet-05.svg',faucet_tf) if r['source'] in ('wallRosetteOuter','wallRosetteInner','wallConnectionCentered','valveBody','stem','valveCollar')]
    assert len(fixed_body)==6
    # Keep the front body/collar after the water so it correctly masks a raised
    # drop, as in Figma. The four other opaque faucet parts are cacheable.
    fixed+=emit([r for r in fixed_body if r['source'] not in ('valveBody','valveCollar')])
    body.append(group('v6static',fixed));live=[]
    # The photo's lower bezel hides the bottom of the original outer wave.
    # Keep its width, and leave room for the full +/-8% impact sweep.
    rings=read('imgGroup95.svg',compose((1,0,0,RING_DEPTH,0,RING_Y-212.414*S*RING_DEPTH),placement('imgGroup95.svg','49:9425')))
    for ring in rings:ring['width']/=math.sqrt(RING_DEPTH)
    assert len(rings)==4
    live.append(group('v6ripples',[group('v6ring'+str(i),emit([r],'v6r'+str(i)+'p')) for i,r in enumerate(rings)]))
    projections=[('imgGroup109.svg','49:9423'),('imgGroup113.svg','49:9469'),('imgGroup112.svg','49:9467'),('imgGroup111.svg','49:9477'),('imgGroup110.svg','49:9471'),('imgGroup114.svg','49:9473'),('imgGroup115.svg','49:9475')]
    for i,(file,node) in enumerate(projections):live.append(group('v6spray'+str(i),emit(read(file,placement(file,node)),'v6s'+str(i)+'p')))
    live.append(group('v6drop',emit(read('imgGroup108.svg',placement('imgGroup108.svg','49:9431')))))
    needle=read('imgVector193.svg',placement('imgVector193.svg','49:9422'))
    pivot=(71.5*S,105.5*S);p=max(needle[0]['geo'][0][0],key=lambda p:math.dist(p,pivot));a=math.pi-math.atan2(p[1]-pivot[1],p[0]-pivot[0]);c,s=math.cos(a),math.sin(a)
    tf=(c,s,-s,c,pivot[0]-c*pivot[0]+s*pivot[1],pivot[1]-s*pivot[0]-c*pivot[1])
    for rec in needle:rec['geo']=[([apply(tf,p) for p in q],closed) for q,closed in rec['geo']]
    live.append(group('v6needle',emit(needle)))
    live+=emit([r for r in fixed_body if r['source'] in ('valveBody','valveCollar')])
    for i in range(4):
        live.append(solid_grip('v6handle'+str(i)))
        if i>=2:
            live.append('<circle id="v6cap'+str(i-2)+'" cx="0" cy="0" r="'+f(3.8*S)+'" fill="'+BLACK+'" stroke="#698eff" stroke-width=".82"/>')
    hub=read('faucet-05.svg',faucet_tf,lambda n:n.get('id') in ('handleHubSide','handleHubTop'))
    assert len(hub)==2
    live+=emit(hub)
    live+=emit(read('imgGroup117.svg',placement('imgGroup117.svg','49:9490')),'v6cleanp')
    body.append(group('v6live',live))
    legacy=ET.parse(ROOT/'plugins/ploc/ploc_legacy.svg').getroot()
    body.append(group('ploclegacy',[ET.tostring(n,encoding='unicode') for n in legacy]))
    svg='<svg xmlns="'+NS+'" width="320" height="160" viewBox="0 0 320 160">'+''.join(body)+'</svg>\n'
    svg=svg.replace('ns0:','').replace(':ns0','')
    svg=re.sub(r'\s*([MLCZz])\s+(?=[0-9.-])',r'\1',svg)
    dest=ROOT/'plugins/ploc/ploc.svg';dest.write_text(svg)
    # Only paths that need dynamic colour changes, never container memory offsets.
    clean=[p for p in PAINTS if p['id'].startswith('v6cleanp')]
    sprays=[[p for p in PAINTS if p['id'].startswith('v6s'+str(i)+'p')] for i in range(7)]
    ring_paints=[[p for p in PAINTS if p['id'].startswith('v6r'+str(i)+'p')] for i in range(4)]
    assert all(len(p)==1 for p in sprays+ring_paints)
    h=['/* Generated from final Figma 49:9400 by tools/build_ploc6_screen.py. */','#ifndef PLOC6_GEOM_H','#define PLOC6_GEOM_H']
    for name,p in [('NEEDLE_X',pivot[0]),('NEEDLE_Y',pivot[1]),('DROP_X',243*S),('DROP_Y',153*S),('DROP_H',72*S),('RING_X',243*S),('RING_Y',RING_Y)]:h.append('#define P6_'+name+' '+str(round(p*1024)))
    h.append('static const char *const p6_spray_paint[7]={'+','.join(json.dumps(p[0]['id']) for p in sprays)+'};')
    h.append('static const char *const p6_ring_paint[4]={'+','.join(json.dumps(p[0]['id']) for p in ring_paints)+'};')
    h.append('#define P6_CLEAN_PAINTS '+str(len(clean)))
    h.append('static const char *const p6_clean_paint[P6_CLEAN_PAINTS]={'+','.join(json.dumps(p['id']) for p in clean)+'};')
    h.append('static const char p6_clean_kind[]="'+''.join(p['kind'] for p in clean)+'";')
    poses=handle_matrices()
    h.append('static const int32_t p6_handle_pose[12][6][6]={')
    for matrices,_ in poses:h.append(' {'+','.join('{'+','.join(str(round(v*1024)) for v in m)+'}' for m in matrices)+'},')
    h.append('};')
    h.append('static const uint8_t p6_cap0_visible[12]={'+','.join(str(int(v)) for _,v in poses)+'};')
    h.append('#endif\n');(ROOT/'plugins/ploc/ploc6_geom.h').write_text('\n'.join(h))
    from collections import Counter
    tree=ET.fromstring(svg);ids=[n.get('id') for n in tree.iter() if n.get('id')]
    records={p['id']:p for p in PAINTS}
    host=['/* Generated node catalogue for the actual callback host tests. */','static const char *const p6_host_ids[]={'+','.join(json.dumps(n) for n in ids)+'};','#define P6_HOST_NODES '+str(len(ids))]
    for kind in ['l','f']:
        host.append('static const uint8_t p6_host_'+kind+'[P6_HOST_NODES][4]={'+','.join('{'+','.join(map(str,records[n]['rgb']))+',255}' if n in records and records[n]['kind']==kind else '{0,0,0,255}' for n in ids)+'};')
    (ROOT/'tests/ploc6_host_nodes.h').write_text('\n'.join(host)+'\n')
    report={'sourceNode':'49:9400','nativeScale':S,'originalAssetsUnmodified':True,'sourcePlacements':PLACEMENTS,'svgBytes':len(svg.encode()),'elements':dict(Counter(n.tag.rsplit('}',1)[-1] for n in tree.iter())),'nodesWithIds':len(ids),'dynamicColourPaints':len(PAINTS),'cleanPaints':len(clean),'handlePoses':12,'retainedHandleContours':6,'rippleCenterY':RING_Y,'rippleDepthScale':RING_DEPTH,'fontMode':'Native R246 Text objects for interactive labels and numbers; original Figma CLEAN and H2O artwork. No font file embedded.','textAssetSlots':'Original 45 placeholders and knob labels become parameter-driven native Text objects. User colour mapping overrides old label colours.'}
    (ROOT/'design/figma-0.6/geometry-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='sourcePlacements'},ensure_ascii=False))

if __name__=='__main__':main()
