#!/usr/bin/env python3
"""Reproducible PLOC 0.5 synth/editor, native GUI snapshots and audio evidence."""
import argparse,array,base64,copy,hashlib,html,importlib.util,json,math,os,re,shutil,subprocess,sys,wave,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def run(args,**kw):
 r=subprocess.run(list(map(str,args)),capture_output=True,text=True,**kw)
 if r.returncode:sys.stderr.write(r.stdout+r.stderr);r.check_returncode()
 return r

def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def wav(path,a):
 with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(44100);w.writeframes(a.tobytes())
def metrics(a):
 peak=max(map(abs,a));active=[v for v in a if abs(v)>100]
 return dict(peak_dbfs=round(20*math.log10(max(peak,1)/32768),2),active_rms_dbfs=round(20*math.log10(math.sqrt(sum(v*v for v in active)/max(1,len(active)))/32768),2),samples_at_guard=sum(abs(v)>=29998 for v in a))
def screen_preview(exe,out,bank):
 ns='http://www.w3.org/2000/svg';ET.register_namespace('',ns);out.mkdir(parents=True,exist_ok=True)
 fontdir=ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts';css=''
 for weight,name in [(100,'Thin'),(300,'Light')]:
  p=fontdir/f'Iosevka-{name}.woff2';shutil.copy2(p,out/p.name);css+=f'@font-face{{font-family:OP1Preview;font-weight:{weight};src:url({p.name}) format("woff2")}}'
 shutil.copy2(fontdir/'Iosevka-OFL.md',out/'Iosevka-OFL.md')
 design=json.loads((ROOT/'design/figma-design.json').read_text());items=[]
 for ri,row in enumerate(design['rows']):
  for si,case in enumerate(row['screens']):
   s=case['state'];p=[round(s['size']*32767/99),(s['ripples']-1)*8192,round(s['visc']*32767/99),round(s['flow']*32767/99),0x5327]
   extra=[round(s['height']*15/99),round(s['impact']*15/99),round(s['tail']*15/99),round(s['spray']*15/8),9,10,9,1,12,0]
   p+=bank.packed(extra);active={'spray':0,'height':1,'tail':2,'impact':3}.get(s.get('active'),-1)
   state=json.loads(run([exe,*p,active]).stdout);root=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot();root.set('width','320');root.set('height','160');root.insert(0,ET.Element('{'+ns+'}rect',{'width':'320','height':'160','fill':'#000000'}))
   nodes={n.get('id'):n for n in root.iter() if n.get('id')}
   for n in state['nodes']:
    el=nodes[n['id']]
    if not n['visible']:el.set('display','none')
    mat=n['matrix'];el.set('transform','matrix('+','.join(str(round(mat[k]/1024,5)) for k in [0,1,3,4,6,7])+')')
   for t in state['text']:
    cap=t['size']*.75;color='#'+''.join(f'{x:02x}' for x in t['color'])
    el=ET.SubElement(root,'{'+ns+'}text',{'x':str(t['x']/1024),'y':str(t['y']+cap),'font-family':'OP1Preview,monospace','font-size':str(cap/.735),'font-weight':'100' if t['size']>=40 else '300','fill':color});el.text=t['value'].upper()
   data=ET.tostring(root,encoding='unicode');filename=f'{ri+1:02d}-{si+1:02d}.svg';(out/filename).write_text(data);items.append(dict(name=s['name'],file=filename,native_state=state,words=p,active=active))
 cards=''.join('<article><p>'+html.escape(x['name'])+'</p>'+ (out/x['file']).read_text()+'</article>' for x in items)
 page='<!doctype html><meta charset="utf-8"><title>PLOC 0.5 / écran natif</title><style>'+css+'body{font:14px system-ui;background:#eeeae2;color:#202820;margin:30px auto;max-width:1400px;padding:20px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:24px}article p{font-size:12px}svg{width:100%;height:auto;background:black}text{font-feature-settings:"cv01" 1,"cv02" 1,"cv03" 1,"cv04" 1,"cv05" 1,"cv06" 1,"cv07" 1,"cv08" 2,"cv09" 3,"cv10" 1,"cv19" 1}</style><h1>PLOC / 0.5.0</h1><p>Géométrie issue des vrais callbacks C. Police Iosevka de prévisualisation. SHIFT + potard, aucune seconde page. À qualifier sur l’écran physique.</p><div class="grid">'+cards+'</div>'
 (out/'index.html').write_text(page);(out/'states.json').write_text(json.dumps(items,ensure_ascii=False,indent=2)+'\n');return items

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sdk-source',type=Path,required=True);ap.add_argument('--toolchain',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True);(ROOT/'output').mkdir(exist_ok=True)
 sdk=args.sdk_source.resolve();kennel=sdk/'ext/kennel';inc=kennel/'synth_sdk';pack=module('pack5',kennel/'tools/kennel/pack.py');runtime=module('runtime5',sdk/'tools/firmware/runtime.py');sys.path.insert(0,str(ROOT/'plugins/ploc'));bank=module('presets5',ROOT/'plugins/ploc/presets4.py')
 env=os.environ.copy();env['PATH']=str(args.toolchain.resolve()/'bin')+os.pathsep+env['PATH'];env['PYTHONDONTWRITEBYTECODE']='1';log='';plugins=[]
 r=run([sys.executable,ROOT/'tools/build_ploc5_screen.py']);log+=r.stdout;r=run([sys.executable,ROOT/'vendor/op1-design-kit/skills/op1-design/scripts/check_svg.py',ROOT/'plugins/ploc/ploc.svg']);log+=r.stdout
 exports=set(re.findall(r'\{\s*"([^"\n]+)"\s*,',pack.core_exports()))
 for name in ['ploc','plocctrl']:
  r=run([sys.executable,kennel/'tools/kennel/pack.py',ROOT/'plugins'/name,'-o',out/(name+'.knl'),'--keep',ROOT/'output'/(name+'-objects')],env=env);log+=r.stdout+r.stderr;p=pack.F.parse((out/(name+'.knl')).read_bytes(),api_version=1);assert p['type_name']==name and set(p['imports'])<=exports;synthetic={n:0x01010000+i*64 for i,n in enumerate(p['imports'])}
  relocated=[len(pack.F.relocate(p,base,synthetic,stubs=True)) for base in (0x10000000,0x18000000)];plugins.append(dict(name=name,bytes=(out/(name+'.knl')).stat().st_size,sha256=sha(out/(name+'.knl')),imports=p['imports'],relocated_bytes=relocated))
 flags=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
 for name in ['dsp','ploc3','ploc4','ploc_surface','ploc_adapter','plocctrl','ploc_screen','ploc_legacy_screen']:
  exe=ROOT/'output'/('test-'+name);extra=['-Wno-unknown-pragmas','-I'+str(inc)] if name in ['ploc_adapter','plocctrl','ploc_screen','ploc_legacy_screen'] else [];r=run(flags+extra+[ROOT/('tests/test_'+name+'.c'),'-lm','-o',exe]);log+=r.stdout+r.stderr;r=run([exe]);log+=r.stdout+r.stderr
 run(['clang','--target=armv7-none-eabi','-std=c11','-ffreestanding','-fsyntax-only','-Wno-unknown-pragmas','-I'+str(inc),*[p for name in ['ploc','plocctrl'] for p in (ROOT/'plugins'/name).glob('*.c')]])
 snapshots=screen_preview(ROOT/'output/test-ploc_screen',out/'preview',bank)
 exe=ROOT/'output/render-ploc5';run(flags+[ROOT/'tools/render_ploc5.c','-lm','-o',exe]);audio=out/'audio';audio.mkdir(exist_ok=True);rows=bank.generate(out/'synth/ploc')
 def render(meta,extra,enable,name):
  raw=ROOT/'output'/(name+'.raw');r=run([exe,*meta['knobs'][:4],*extra,0,int(enable),raw]);data=array.array('h');data.frombytes(raw.read_bytes());return data,r.stdout
 for row in rows:
  meta=row['meta'];assert runtime.preset_meta((out/'synth/ploc'/row['file']).read_bytes())==meta
  a,stats=render(meta,row['secondary'],True,meta['name']);baseline,_=render(meta,row['secondary'],False,meta['name']+'-baseline');log+=stats;assert 'input_limits=0' in stats
  row['audio_metrics']=metrics(a);row['baseline_041_metrics']=metrics(baseline);assert max(map(abs,a))<=max(map(abs,baseline));row['sha256']=sha(out/'synth/ploc'/row['file']);row['audio']=meta['name']+'.wav'
  if meta['knobs'][1]==0:assert a==baseline
  for j in range(441):a[-441+j]=round(a[-441+j]*(440-j)/440)
  wav(audio/row['audio'],a);run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',audio/row['audio'],'-c:a','libmp3lame','-b:a','160k',audio/(meta['name']+'.mp3')])
 compare=copy.deepcopy(rows[4]['meta']);parts=[];levels=[]
 for raw,label in [(0,'1 onde / direct'),(32767,'4 ondes / surface')]:
  compare['knobs'][1]=raw;a,_=render(compare,rows[4]['secondary'],True,'AB-'+str(raw));levels.append(dict(label=label,**metrics(a)));parts.extend(a[:44100*6]);parts.extend([0]*22050)
 wav(audio/'RIPPLE-comparaison.wav',array.array('h',parts));run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',audio/'RIPPLE-comparaison.wav','-c:a','libmp3lame','-b:a','160k',audio/'RIPPLE-comparaison.mp3'])
 validation=dict(version='0.5.0',status='Built and host-tested. Hardware installation, CPU and OLED validation not yet performed.',target='original OP-1 / R246 / Hundekraft 3.1.4',api=1,plugins=plugins,main=['FLOW','SIZE','VISC','RIPPLE'],shift=['SPRAY','HEIGHT','TAIL','IMPACT'],preset_abi='Unchanged 0.4 storage. Existing presets and effects need no re-import.',raw_midi_lfo_order=['SIZE','RIPPLE','VISC','FLOW'],tail_visual='No new geometry; active label only.',surface=dict(storage_bytes_per_voice=520,wavefronts=[1,4],feed_forward=True,max_reflection_samples=225,latency_samples_direct=0,gain='Convex mix, no boost; one wavefront sample-exact versus 0.4',ab_levels=levels),screen=dict(svg_bytes=(ROOT/'plugins/ploc/ploc.svg').stat().st_size,snapshots=len(snapshots),font='Native R246 on hardware; open Iosevka in host preview',idle='No unchanged transforms/text mutations'),presets=rows,audio_method='Production integer DSP plus integrated surface response, common gain, native T2/T3/T4 and mixer excluded.')
 (out/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n');(out/'checks.log').write_text(log);shutil.copy2(ROOT/'PLOC-GUIDE.md',out/'README.md')
 source=out/'source';source.mkdir(exist_ok=True)
 for name in ['ploc','plocctrl']:shutil.copytree(ROOT/'plugins'/name,source/'plugins'/name,dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__'))
 for folder in ['tests','tools']:shutil.copytree(ROOT/folder,source/folder,dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__'))
 for name in ['LICENSE','NOTICE','PROVENANCE.md','PLOC-GUIDE.md','README.md','VALIDATION.md']:shutil.copy2(ROOT/name,source/name)
 kit=source/'vendor/op1-design-kit';kit.mkdir(parents=True,exist_ok=True)
 for name in ['LICENSE','THIRD_PARTY.md','design-kit.json']:shutil.copy2(ROOT/'vendor/op1-design-kit'/name,kit/name)
 dest=kit/'skills/op1-design';(dest/'scripts').mkdir(parents=True,exist_ok=True);(dest/'assets/fonts').mkdir(parents=True,exist_ok=True)
 shutil.copy2(ROOT/'vendor/op1-design-kit/skills/op1-design/scripts/check_svg.py',dest/'scripts/check_svg.py')
 for name in ['Iosevka-Thin.woff2','Iosevka-Light.woff2','Iosevka-OFL.md']:shutil.copy2(ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts'/name,dest/'assets/fonts'/name)
 shutil.copytree(ROOT/'design',source/'design',dirs_exist_ok=True)
 cards=''.join('<article><h2>'+html.escape(r['meta']['name'])+'</h2><p>T3 : '+r['meta']['fx_type'].upper()+'</p><audio controls preload="none" src="audio/'+html.escape(r['meta']['name'])+'.mp3"></audio></article>' for r in rows)
 (out/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>PLOC 0.5.0</title><style>body{font:16px system-ui;background:#eeeae2;color:#202820;max-width:1100px;margin:40px auto;padding:20px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:24px}article{background:#080a0d;color:white;padding:24px}audio{width:100%}a{color:inherit}</style><h1>PLOC / 0.5.0</h1><p>Une vue, huit réglages sous Shift. Écoutes sur Mac avec la nouvelle surface interne, sans effets T3.</p><p><a href="preview/index.html">Voir les 24 états issus du vrai code C →</a></p><h2>RIPPLE / 1 onde puis 4 ondes</h2><audio controls preload="none" src="audio/RIPPLE-comparaison.mp3"></audio><div class="grid">'+cards+'</div>')
 files=sorted(p for p in out.rglob('*') if p.is_file() and p.name!='SHA256SUMS');(out/'SHA256SUMS').write_text(''.join(sha(p)+'  '+str(p.relative_to(out))+'\n' for p in files));print(json.dumps({'out':str(out),'plugins':plugins,'screen_snapshots':len(snapshots),'presets':len(rows),'ab_levels':levels},indent=2))
if __name__=='__main__':main()
