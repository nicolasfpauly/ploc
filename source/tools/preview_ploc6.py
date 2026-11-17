#!/usr/bin/env python3
"""Render actual C callback snapshots. Optional private native-font proof only.
The font is read from --font and is never copied to a delivery or the plugin.
"""
from pathlib import Path
import argparse,copy,hashlib,html,importlib.util,json,os,sqlite3,subprocess,sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1];NS='http://www.w3.org/2000/svg'
ET.register_namespace('',NS)

def native_font(font,kern_path):
    sdk=Path(os.environ.get('PLOC_SDK_SOURCE',ROOT/'sdk/hundekraft-3.1.4-source'))/'ext/kennel/synth_sdk/display'
    sys.path.insert(0,str(sdk));import svgfont
    polygons=svgfont.load(font);source=ET.parse(font).getroot();shapes={};boxes={}
    for original in source:
        key=svgfont.decode_id(original.get('id',''))
        if len(key)!=1 or key.isupper() or not polygons.get(key):continue
        points=[p for q in polygons[key] for p in q];xs=[p[0] for p in points];ys=[p[1] for p in points]
        boxes[key]=(min(xs),min(ys),max(xs)-min(xs),max(ys)-min(ys));kept=[]
        def visit(n,stroke='#000000'):
            stroke=n.get('stroke',stroke);tag=n.tag.rsplit('}',1)[-1]
            if tag=='g':
                for ch in n:visit(ch,stroke)
            elif tag in ('path','line','polyline','polygon','rect','circle','ellipse') and stroke.lower() in ('#000000','#000','black'):
                c=copy.deepcopy(n);c.attrib.pop('id',None);kept.append(c)
        visit(original);shapes[key]=kept
    kern={}
    if kern_path:
        conn=sqlite3.connect(kern_path);kern={(a,b):v for a,b,v in conn.execute('SELECT * FROM kerntable')};conn.close()
    H=boxes['h'][3]
    def word(t):
        value=t['value'].lower();numeric=value.isdigit()
        # Calibrated on the OLED photo, in 320x160 coordinates. Still an
        # approximation of the firmware's spacing and pixel rasterizer.
        cap=t['size']*(1.06 if numeric else 1.00);stroke=1.25 if numeric else 1.5
        scale=(cap-stroke)/H;cursor=stroke/2;outer=ET.Element('{'+NS+'}g',{'transform':f'translate({t["x"]/1024} {t["y"]})'})
        rgb='#'+''.join(f'{c:02x}' for c in t['color'])
        for i,ch in enumerate(value):
            x,y,w,h=boxes[ch];cell=max(w,H*.22);px=cursor+(cell-w)*scale/2
            g=ET.SubElement(outer,'{'+NS+'}g',{'transform':f'translate({px} {stroke/2+(H-h)*scale}) scale({scale}) translate({-x} {-y})'})
            for original in shapes[ch]:
                n=copy.deepcopy(original);n.set('stroke',rgb);n.set('stroke-width',str(stroke/scale));n.set('fill','none');g.append(n)
            cursor+=cell*scale
            if i+1<len(value):cursor+=((6.5 if numeric else 12)+kern.get((ch,value[i+1]),0))*scale
        return outer,{'value':value,'x':t['x']/1024,'y':t['y'],'capHeight':cap,'inkWidth':cursor,'right':t['x']/1024+cursor,'bottom':t['y']+cap}
    return word

def packed(extra):
    words=[0x6000,0,0]
    for k,value in enumerate(extra):
        bit=k*4;w,offset,width=(2,bit-26,15) if bit>=26 else ((1,bit-11,15) if bit>=11 else (0,bit,11))
        words[w]|=(value<<offset)&((1<<width)-1)
        if offset+4>width:words[w+1]|=value>>(width-offset)
    return words

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--font',type=Path);ap.add_argument('--kern',type=Path);a=ap.parse_args()
    out=a.out;out.mkdir(parents=True,exist_ok=True);draw_text=native_font(a.font,a.kern) if a.font else None
    base=[28500,23000,15000,15000,0x5327];extra=[15,8,5,7,9,10,9,1,12,0]
    cases=[]
    def add(name,words=None,extras=None,active=-1):cases.append((name,words or base,extras or extra,active))
    add('Vue principale')
    photo=base[:];photo[0]=round(88*32767/99);photo[1]=round(81*32767/99);photo[3]=10000
    photo_extra=extra[:];photo_extra[0]=2;photo_extra[1]=13;photo_extra[3]=2
    add('Photo / réglages approchés',photo,photo_extra)
    for value,name in [(0,'FLOW / minimum'),(32767,'FLOW / maximum')]:p=base[:];p[3]=value;add(name,p)
    for value,name in [(0,'SPRAY / veille 20 %'),(15,'SPRAY / toutes allumées')]:e=extra[:];e[3]=value;add(name,extras=e)
    for value,name in [(0,'QUALITÉ / CLEAN à 20 %'),(32767,'QUALITÉ / CLEAN à 100 %')]:p=base[:];p[2]=value;add(name,p,active=2)
    for value,name in [(0,'RIPPLE / une onde'),(32767,'RIPPLE / quatre ondes')]:p=base[:];p[1]=value;add(name,p)
    for value,name in [(0,'IMPACT / doux'),(15,'IMPACT / lumineux')]:e=extra[:];e[1]=value;add(name,extras=e,active=3)
    for value,name in [(0,'SIZE / minimum'),(32767,'SIZE / maximum')]:p=base[:];p[0]=value;add(name,p)
    for value,name in [(0,'HEIGHT / bas'),(15,'HEIGHT / haut')]:e=extra[:];e[0]=value;add(name,extras=e,active=1)
    add('Shift + bleu / réservé',active=0)
    for i in range(12):p=base[:];p[3]=round(i*32767/11);add('Poignée / pose '+str(i+1),p)
    states=[];bounds=[];readouts={};root_source=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot()
    for i,(name,words,extras,active) in enumerate(cases):
        raw=words+packed(extras)
        state=json.loads(subprocess.check_output([str(ROOT/'output/test-ploc_screen'),*map(str,raw),str(active)],text=True))
        root=copy.deepcopy(root_source);root.insert(0,ET.Element('{'+NS+'}rect',{'width':'320','height':'160','fill':'#000000'}))
        nodes={n.get('id'):n for n in root.iter() if n.get('id')}
        for n in state['nodes']:
            el=nodes[n['id']]
            if not n['visible']:el.set('display','none')
            m=n['matrix']
            if m!=[1024,0,0,0,1024,0,0,0,1024]:el.set('transform','matrix('+','.join(str(m[k]/1024) for k in [0,1,3,4,6,7])+')')
            for key,paint in [('stroke',n['line']),('fill',n['fill'])]:
                if el.get(key) not in (None,'none') and any(paint[:3]):el.set(key,'#'+''.join(f'{v:02x}' for v in paint[:3]))
        for t in state['text']:
            if draw_text:
                g,b=draw_text(t);root.append(g);bounds.append(dict(case=i,**b))
                if t['value'].isdigit():readouts[(t['x'],t['y'],t['size'])]=t
            else:
                n=ET.SubElement(root,'{'+NS+'}text',{'x':str(t['x']/1024),'y':str(t['y']+t['size']*.75),'font-family':'monospace','font-size':str(t['size']),'fill':'#'+''.join(f'{c:02x}' for c in t['color'])});n.text=t['value'].upper()
        file=f'{i:02d}.svg';(out/file).write_text(ET.tostring(root,encoding='unicode'))
        states.append({'name':name,'file':file,'words':raw,'active':active,'nativeState':state})
    (out/'states.json').write_text(json.dumps(states,ensure_ascii=False,indent=2)+'\n')
    assert not bounds or all(b['right']<317 and b['bottom']<146 for b in bounds),[b for b in bounds if b['right']>=317 or b['bottom']>=146]
    # Check every two-digit string, including 00 and the broadest pairs.
    for t in readouts.values():
        for v in range(100):
            _,b=draw_text(dict(t,value=f'{v:02d}'))
            assert b['right']<317 and b['bottom']<146,b
    (out/'text-bounds.json').write_text(json.dumps(bounds,indent=2)+'\n')
    cards=''.join('<button data-i="'+str(i)+'">'+html.escape(s['name'])+'</button>' for i,s in enumerate(states))
    body=''.join('<div class="frame" id="s'+str(i)+'"'+(' hidden' if i else '')+'>'+(out/s['file']).read_text()+'</div>' for i,s in enumerate(states))
    page='''<!doctype html><html lang="fr"><meta charset="utf-8"><title>PLOC 0.6</title><style>body{font:15px system-ui;background:#eeeae2;color:#202820;max-width:1100px;margin:36px auto;padding:24px}.frame{max-width:800px;margin:28px 0}.frame svg{width:100%;height:auto;display:block;border-radius:16px}button{font:inherit;padding:9px 13px;background:white;border:1px solid #ccc;border-radius:5px;margin:4px;cursor:pointer}button.selected{background:#202820;color:white}p{line-height:1.5;max-width:760px}</style><h1>PLOC / 0.6</h1><p>FLOW · SIZE / HEIGHT · SPRAY / QUALITY · RIPPLE / IMPACT.</p>'''+body+'<nav>'+cards+'''</nav><p>États issus du vrai code C. Corps opaque, douze poses, gouttelettes en veille à 20 %, luminosité CLEAN et IMPACT. Le son et les presets restent inchangés. Le rendu OLED et le coût graphique restent à tester sur l’OP-1.</p><script>for(const b of document.querySelectorAll('button'))b.onclick=()=>{document.querySelectorAll('.frame').forEach(x=>x.hidden=true);document.querySelector('#s'+b.dataset.i).hidden=false;document.querySelectorAll('button').forEach(x=>x.classList.remove('selected'));b.classList.add('selected')};document.querySelector('button').classList.add('selected');</script></html>'''
    (out/'index.html').write_text(page)
    report={'snapshots':len(states),'actualCallback':True,'fontProof':'Private glyph geometry and host spacing calibrated against the OLED photo. Not a device capture.' if draw_text else 'Host monospace fallback, not native typography.','fontFileIncluded':False,'textBoundsPass':bool(bounds),'digitPairsChecked':len(readouts)*100,'sourceFontSha256':hashlib.sha256(a.font.read_bytes()).hexdigest() if a.font else None}
    (out/'preview-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
