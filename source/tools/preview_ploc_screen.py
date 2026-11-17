#!/usr/bin/env python3
"""Render actual native screen-callback snapshots using an open-font host proxy.
No TE font geometry is exported. Native OLED appearance still requires hardware.
"""
import argparse,base64,copy,json,math,shutil,subprocess,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
NS='http://www.w3.org/2000/svg';ET.register_namespace('',NS)
NAMES=['drop','gloss','ripple','viscbar']+['extra'+str(i) for i in range(5)]+['count'+str(i) for i in range(5)]
COLORS=['#698eff','#00ed95','#ffffff','#ff423f']
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--host',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);args=ap.parse_args();out=args.out;out.mkdir(parents=True,exist_ok=True)
    fontdir=ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts'
    if not fontdir.exists():fontdir=ROOT/'host-fonts'
    css=''
    for weight,name in [(200,'ExtraLight'),(400,'Regular')]:
        encoded=base64.b64encode((fontdir/('Iosevka-'+name+'.woff2')).read_bytes()).decode()
        css+='@font-face{font-family:OP1Preview;font-weight:'+str(weight)+';src:url(data:font/woff2;base64,'+encoded+') format("woff2")}\n'
    shutil.copy2(fontdir/'Iosevka-OFL.md',out/'Iosevka-OFL.md')
    pictures=[];records=[]
    for count in range(6):
        word=(count*32768+5)//6
        state=json.loads(subprocess.check_output([str(args.host),str(word)],text=True));assert state['count']==count
        root=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot();root.set('width','320');root.set('height','160')
        rect=ET.Element('{'+NS+'}rect',{'x':'0','y':'0','width':'320','height':'160','fill':'#000000'});root.insert(0,rect)
        groups={g.get('id'):g for g in root if g.get('id')}
        for name,node in zip(NAMES,state['nodes']):
            g=groups[name]
            if not node['visible']:root.remove(g);continue
            m=node['matrix'];values=[m[0],m[1],m[3],m[4],m[6],m[7]]
            g.set('transform','matrix('+','.join(f'{v/1024:.4f}' for v in values)+')')
        for i,t in enumerate(state['text']):
            cap=t['size']*.75
            e=ET.SubElement(root,'{'+NS+'}text',{'x':str(t['x']/1024),'y':str(t['y']+cap),'font-family':'OP1Preview,monospace','font-size':str(cap/0.735),'font-weight':'200' if i==1 else '400','fill':COLORS[i//2]})
            e.text=t['value'].upper()
        svg=ET.tostring(root,encoding='unicode');(out/('drops-'+str(count)+'.svg')).write_text(svg)
        pictures.append(svg);records.append(state)
    (out/'states.json').write_text(json.dumps(records,indent=2)+'\n')
    page='''<!doctype html><html lang="fr"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>PLOC 0.3 / DROPS</title><style>'''+css+'''body{background:#ebe7dd;color:#1f2929;font:16px system-ui;margin:40px auto;padding:0 24px;max-width:960px}h1{font-size:40px;letter-spacing:-1.5px;margin-bottom:6px}.note{color:#5b6262;font-size:13px}.screen{width:640px;max-width:100%;background:black;border-radius:8px;overflow:hidden;margin:28px 0}.screen svg{width:100%;height:auto;display:block}input{accent-color:#ff423f;width:280px;max-width:80%;vertical-align:middle}output{font:28px monospace;color:#cc302f;margin-left:18px}.strip{display:grid;grid-template-columns:repeat(3,1fr);gap:16px;margin-top:34px}.strip svg{width:100%;height:auto;background:black}.strip span{font:12px monospace}text{font-feature-settings:'cv01' 1,'cv02' 1,'cv03' 1,'cv04' 1,'cv05' 1,'cv06' 1,'cv07' 1,'cv08' 2,'cv09' 3,'cv10' 1,'cv19' 1}</style><h1>PLOC / DROPS</h1><p>Le rouge ajoute 0 à 5 petites bulles autour de la goutte principale.</p><p class="note">Simulation sur Mac, depuis les états du code d’écran. Police de prévisualisation Iosevka. Le plugin utilise la police native de l’OP-1.</p><div id="screen" class="screen"></div><label for="drops">DROPS </label><input id="drops" type="range" min="0" max="5" value="2" step="1"><output id="value">2</output><div class="strip">'''+''.join('<div><span>DROPS '+str(i)+'</span>'+pictures[i]+'</div>' for i in range(6))+'''</div><script>const screens='''+json.dumps(pictures)+''';const knob=document.getElementById('drops');function update(){document.getElementById('screen').innerHTML=screens[Number(knob.value)];document.getElementById('value').textContent=knob.value;}knob.addEventListener('input',update);update();</script></html>'''
    (out/'index.html').write_text(page)
    print('Generated six verified callback snapshots and an interactive DROPS preview:',out)
if __name__=='__main__':main()
