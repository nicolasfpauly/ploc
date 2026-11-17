#!/usr/bin/env python3
"""Build the native 0.4 synth/editor and four-effect preset bank."""
import argparse,array,base64,hashlib,html,importlib.util,json,math,os,re,shutil,subprocess,sys,wave,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
VERSION='0.4.1'
def run(args,**kw):
    r=subprocess.run([str(a) for a in args],capture_output=True,text=True,**kw)
    if r.returncode:sys.stderr.write(r.stdout+r.stderr);r.check_returncode()
    return r
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def waveform(path,values):
    with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(44100);w.writeframes(values.tobytes())
def metrics(a):
    peak=max(map(abs,a));active=[v for v in a if abs(v)>100]
    return {'peak_dbfs':round(20*math.log10(max(peak,1)/32768),2),'active_rms_dbfs':round(20*math.log10(math.sqrt(sum(v*v for v in active)/max(1,len(active)))/32768),2),'samples_at_guard':sum(abs(v)>=29998 for v in a),'seconds':len(a)/44100}
def screen_preview(host,out):
    ns='http://www.w3.org/2000/svg';ET.register_namespace('',ns);out.mkdir(exist_ok=True)
    fontdir=ROOT/'host-fonts'
    if not fontdir.exists():fontdir=ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts'
    css=''
    for weight,name in [(200,'ExtraLight'),(400,'Regular')]:
        encoded=base64.b64encode((fontdir/('Iosevka-'+name+'.woff2')).read_bytes()).decode()
        css+='@font-face{font-family:OP1Preview;font-weight:'+str(weight)+';src:url(data:font/woff2;base64,'+encoded+') format("woff2")}\n'
    shutil.copy2(fontdir/'Iosevka-OFL.md',out/'Iosevka-OFL.md')
    names=['drop','gloss','ripple','viscbar']+['extra'+str(i) for i in range(5)]+['count'+str(i) for i in range(5)]
    screens=[];states=[]
    for page in range(4):
        state=json.loads(run([host,20000,page]).stdout);states.append(state)
        root=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot();root.set('width','320');root.set('height','160')
        root.insert(0,ET.Element('{'+ns+'}rect',{'width':'320','height':'160','fill':'black'}))
        groups={g.get('id'):g for g in root if g.get('id')}
        for name,node in zip(names,state['nodes']):
            g=groups[name]
            if not node['visible']:root.remove(g);continue
            m=node['matrix'];g.set('transform','matrix('+','.join(f'{m[i]/1024:.4f}' for i in [0,1,3,4,6,7])+')')
        for i,t in enumerate(state['text']):
            cap=t['size']*.75
            e=ET.SubElement(root,'{'+ns+'}text',{'x':str(t['x']/1024),'y':str(t['y']+cap),'font-family':'OP1Preview,monospace','font-size':str(cap/.735),'font-weight':'200' if i==1 else '400','fill':['#698eff','#00ed95','#ffffff','#ff423f'][i//2]});e.text=t['value'].upper()
        for k in range(4):
            x=248+k*14;ET.SubElement(root,'{'+ns+'}path',{'d':f'M {x} 151 L {x+(6 if k==page else 2)} 151','stroke':'#ff423f' if k==page else '#414141','stroke-width':'2' if k==page else '1','fill':'none'})
        svg=ET.tostring(root,encoding='unicode');screens.append(svg);(out/('page-'+str(page)+'.svg')).write_text(svg)
    (out/'states.json').write_text(json.dumps(states,indent=2)+'\n')
    page='<!doctype html><meta charset="utf-8"><title>PLOC 0.4</title><style>'+css+'''body{font:16px system-ui;background:#eeeae2;color:#253030;margin:32px auto;max-width:1020px;padding:20px}h1{font-size:40px;margin:0}.screens{display:grid;grid-template-columns:1fr 1fr;gap:24px}.screen svg{display:block;width:100%;height:auto;background:black}.screen p{font:12px monospace}button{padding:12px;margin:10px 8px 20px 0}#chosen{max-width:640px}#chosen svg{width:100%;display:block}text{font-feature-settings:'cv01' 1,'cv02' 1,'cv03' 1,'cv04' 1,'cv05' 1,'cv06' 1,'cv07' 1,'cv08' 2,'cv09' 3,'cv10' 1,'cv19' 1}</style><h1>PLOC / 0.4</h1><p>Clic rouge : page suivante. Shift + clic rouge : précédente. Clic bleu : paramètres principaux.</p><p>Les secondaires ont 16 positions et se sauvegardent avec le preset.</p><div id="chosen"></div><button id="prev">← Page précédente</button><button id="next">Page suivante →</button><div class="screens">'''+''.join('<div class="screen"><p>PAGE '+str(i+1)+'</p>'+v+'</div>' for i,v in enumerate(screens))+'''</div><p>États des vrais callbacks C, police Iosevka de prévisualisation. La machine utilise sa police native.</p><script>const screens='''+json.dumps(screens)+''';let page=0;function draw(){document.getElementById('chosen').innerHTML=screens[page]};document.getElementById('next').onclick=()=>{page=(page+1)%4;draw()};document.getElementById('prev').onclick=()=>{page=(page+3)%4;draw()};draw();</script>'''
    (out/'index.html').write_text(page)
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--sdk-source',type=Path,required=True);ap.add_argument('--toolchain',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);a=ap.parse_args()
    out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);(ROOT/'output').mkdir(exist_ok=True);sdk=a.sdk_source.resolve();kennel=sdk/'ext/kennel';inc=kennel/'synth_sdk'
    pack=module('pack4',kennel/'tools/kennel/pack.py');runtime=module('runtime4',sdk/'tools/firmware/runtime.py')
    sys.path.insert(0,str(ROOT/'plugins/ploc'));presets=module('presets4',ROOT/'plugins/ploc/presets4.py')
    env=os.environ.copy();env['PATH']=str(a.toolchain.resolve()/'bin')+os.pathsep+env['PATH']
    log='';plugins=[]
    for name in ['ploc']:
        svg=ROOT/'plugins'/name/(name+'.svg');root=ET.parse(svg).getroot()
        assert root.get('viewBox')=='0 0 320 160' and svg.stat().st_size<8192
        for node in root.iter():
            assert node.tag.rsplit('}',1)[-1] in ['svg','g','path']
            if 'd' in node.attrib:assert len(re.findall('[Mm]',node.attrib['d']))==1, 'Split disconnected contours'
    exports=set(re.findall(r'\{\s*"([^"\n]+)"\s*,',pack.core_exports()))
    for name in ['ploc','plocctrl']:
        r=run([sys.executable,kennel/'tools/kennel/pack.py',ROOT/'plugins'/name,'-o',out/(name+'.knl'),'--keep',ROOT/'output'/(name+'-objects')],env=env);log+=r.stdout+r.stderr
        parsed=pack.F.parse((out/(name+'.knl')).read_bytes(),api_version=1)
        assert parsed['type_name']==name and parsed['api_version']==1 and set(parsed['imports'])<=exports
        synthetic={name:0x01010000+64*i for i,name in enumerate(parsed['imports'])}
        relocated=[len(pack.F.relocate(parsed,base,synthetic,stubs=True)) for base in (0x10000000,0x18000000)]
        plugins.append(dict(name=name,sha256=sha(out/(name+'.knl')),bytes=(out/(name+'.knl')).stat().st_size,imports=parsed['imports'],relocated_bytes=relocated))
    flags=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    for name in ['dsp','ploc3','ploc4','ploc_adapter','plocctrl','ploc_screen']:
        exe=ROOT/'output'/('test-'+name);extra=['-Wno-unknown-pragmas','-I'+str(inc)] if name in ['plocctrl','ploc_screen','ploc_adapter'] else []
        r=run(flags+extra+[ROOT/('tests/test_'+name+'.c'),'-lm','-o',exe]);log+=r.stdout+r.stderr
        r=run([exe]);log+=r.stdout+r.stderr
    run(['clang','--target=armv7-none-eabi','-std=c11','-ffreestanding','-fsyntax-only','-Wno-unknown-pragmas','-I'+str(inc),*[p for name in ['ploc','plocctrl'] for p in (ROOT/'plugins'/name).glob('*.c')]])
    screen_preview(ROOT/'output/test-ploc_screen',out/'preview')
    exe=ROOT/'output/render-ploc4';run(flags+[ROOT/'tools/render_ploc4.c','-lm','-o',exe]);audio=out/'audio';audio.mkdir(exist_ok=True)
    rows=presets.generate(out/'synth/ploc');montage=array.array('h');levels=[]
    for row in rows:
        meta=row['meta'];p=out/'synth/ploc'/row['file'];assert runtime.preset_meta(p.read_bytes())==meta
        assert meta['knobs'][5]&0x7800==0x6000 and meta['fx_type'] in presets.RANGES
        for v,(lo,hi) in zip(meta['fx_params'][:4],presets.RANGES[meta['fx_type']]):assert lo<=v<=hi
        raw=ROOT/'output/preview.raw';r=run([exe,*meta['knobs'][:4],*row['secondary'],0,0,raw]);log+=r.stdout
        values=array.array('h');values.frombytes(raw.read_bytes());assert sys.byteorder=='little'
        row['voice_input_limits']=int(re.search(r'input_limits=(\d+)',r.stdout)[1]);assert row['voice_input_limits']==0
        row['audio_metrics']=metrics(values);row['sha256']=sha(p)
        assert row['audio_metrics']['peak_dbfs']<0 and row['audio_metrics']['samples_at_guard']==0
        for j in range(441):values[-441+j]=round(values[-441+j]*(440-j)/440)
        name=p.stem+'.wav';waveform(audio/name,values);row['audio']=name
        run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',audio/name,'-c:a','libmp3lame','-b:a','160k',audio/(p.stem+'.mp3')])
        segment=values[:44100*4];montage.extend(segment);montage.extend([0]*11025)
    waveform(audio/'PLOC-04-tour.wav',montage)
    # Same primary settings, no FX, no post-normalisation: gain/presence evidence.
    for old in [1,0]:
        raw=ROOT/'output/level.raw';r=run([exe,16384,0,5000,0,*presets.DEFAULT,old,0,raw]);log+=r.stdout
        values=array.array('h');values.frombytes(raw.read_bytes());levels.append(dict(model='0.3' if old else '0.4',**metrics(values)));waveform(audio/('level-03.wav' if old else 'level-04.wav'),values)
    validation=dict(version=VERSION,status='Built and host-tested. Hardware installation and listening not yet verified.',target='original OP-1 / R246 / Hundekraft 3.1.4',api=1,plugins=plugins,main=['SIZE','RIPPLE','VISC','FLOW'],secondary=['POUIC','IMPACT','TAIL','SPRAY','SPREAD','CHAOS','TONE','DISTANCE','WIDTH','TUNE'],secondary_steps=16,secondary_storage='40 bits in preset words 5..7; words 0..3 and selector preserved',levels=levels,presets=rows,audio_method='Dry production integer voice, common gain; native T2/T3/T4 and mixer excluded; final preview-only 10ms fade; external default effects only run on the OP-1.')
    (out/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n');(out/'checks.log').write_text(log)
    readme=(ROOT/'PLOC-GUIDE.md').read_text()
    (out/'README.md').write_text(readme)
    cards=[]
    for row in rows:cards.append('<article><h2>'+html.escape(row['meta']['name'])+'</h2><p>'+html.escape(row['description'])+'</p><p>T3 : '+html.escape(row['meta']['fx_type'].upper())+'</p><audio controls preload="none" src="audio/'+html.escape(Path(row['audio']).with_suffix('.mp3').name,quote=True)+'"></audio></article>')
    (out/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>PLOC 0.4</title><style>body{font:16px system-ui;background:#eeeae2;color:#253030;max-width:1000px;margin:40px auto;padding:0 24px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:24px}article{background:#080b0d;color:white;padding:24px;border-radius:8px}audio{width:100%}a{color:inherit}</style><h1>PLOC / 0.4</h1><p>Gouttes dans l’air, Flow pendant la tenue, 14 réglages sauvegardés.</p><p><a href="preview/index.html">Voir les quatre pages de l’écran →</a> · <a href="README.md">Guide</a></p><p>Écoutes sèches du DSP sur Mac. Aucun mastering par preset ; T2, effets T3, T4 et mixeur matériel exclus.</p><div class="grid">'+''.join(cards)+'</div>')
    source=out/'source';source.mkdir(exist_ok=True)
    for name in ['ploc','plocctrl']:shutil.copytree(ROOT/'plugins'/name,source/'plugins'/name,dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ['LICENSE','NOTICE','PLOC-GUIDE.md']:shutil.copy2(ROOT/name,source/name)
    for folder,files in [('tools',['build_ploc4_release.py','render_ploc4.c']),('tests',['test_dsp.c','test_ploc3.c','test_ploc4.c','test_ploc_adapter.c','test_plocctrl.c','test_ploc_screen.c'])]:
        (source/folder).mkdir(exist_ok=True)
        for name in files:shutil.copy2(ROOT/folder/name,source/folder/name)
    shutil.copytree(ROOT/'host-fonts' if (ROOT/'host-fonts').exists() else ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts',source/'host-fonts',dirs_exist_ok=True)
    sums=''.join(sha(p)+'  '+str(p.relative_to(out))+'\n' for p in sorted(out.rglob('*')) if p.is_file() and p.name!='SHA256SUMS');(out/'SHA256SUMS').write_text(sums)
    print(json.dumps({'out':str(out),'plugins':plugins,'levels':levels,'presets':len(rows)},ensure_ascii=False,indent=2))
if __name__=='__main__':main()
