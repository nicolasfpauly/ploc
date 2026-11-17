#!/usr/bin/env python3
"""Prepare the two PLOC 0.6 plugins, source, checks and GUI evidence. No device I/O."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,os,re,shutil,subprocess,sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(args,env):
    p=subprocess.run(list(map(str,args)),capture_output=True,text=True,env=env,cwd=ROOT)
    if p.returncode:sys.stderr.write(p.stdout+p.stderr);p.check_returncode()
    return p.stdout+p.stderr
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--sdk-source',type=Path,required=True);ap.add_argument('--toolchain',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--font',type=Path);ap.add_argument('--kern',type=Path);a=ap.parse_args()
    out=a.out.resolve()
    if out.exists() and any(out.iterdir()):raise SystemExit('Use an empty output directory.')
    out.mkdir(parents=True,exist_ok=True)
    env=os.environ.copy();env['PLOC_SDK_SOURCE']=str(a.sdk_source.resolve());env['PATH']=str(a.toolchain.resolve()/'bin')+os.pathsep+env['PATH'];env['PYTHONDONTWRITEBYTECODE']='1'
    log=run([sys.executable,ROOT/'tools/build_ploc6_screen.py'],env)
    log+=run(['bash',ROOT/'tools/check.sh'],env)
    inc=a.sdk_source.resolve()/'ext/kennel/synth_sdk'
    pack=module('ploc6pack',a.sdk_source.resolve()/'ext/kennel/tools/kennel/pack.py')
    exports=set(re.findall(r'\{\s*"([^"\n]+)"\s*,',pack.core_exports()))
    plugins=[]
    version=json.loads((ROOT/"plugins/ploc/plugin.json").read_text())["version"]
    for name in ['ploc','plocctrl']:
        log+=run([sys.executable,a.sdk_source.resolve()/'ext/kennel/tools/kennel/pack.py',ROOT/'plugins'/name,'-o',out/(name+'.knl'),'--keep',ROOT/'output'/(name+'-objects')],env)
        parsed=pack.F.parse((out/(name+'.knl')).read_bytes(),api_version=1)
        assert parsed['type_name']==name and set(parsed['imports'])<=exports
        reloc={n:0x01010000+i*64 for i,n in enumerate(parsed['imports'])}
        placed=[len(pack.F.relocate(parsed,base,reloc,stubs=True)) for base in (0x10000000,0x18000000)]
        component_version=json.loads((ROOT/'plugins'/name/'plugin.json').read_text())['version']
        plugins.append({'name':name,'version':component_version,'bytes':(out/(name+'.knl')).stat().st_size,'sha256':sha(out/(name+'.knl')),'imports':parsed['imports'],'relocatedBytes':placed})
    sys.path.insert(0,str(inc/'harness'));import svg_check
    native=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot()
    for n in list(native):
        if n.get('id')=='ploclegacy':native.remove(n)
    assert not svg_check.filled_subpaths_in(ET.tostring(native)), 'New art must have one contour per fill.'
    preview_args=[sys.executable,ROOT/'tools/preview_ploc6.py','--out',out/'preview']
    if a.font:preview_args+=['--font',a.font]
    if a.kern:preview_args+=['--kern',a.kern]
    log+=run(preview_args,env)
    baseline=json.loads((ROOT/'design/baseline-0.5.json').read_text())
    stable={k:v for k,v in baseline.items() if k.endswith(('_dsp.h','_tables.h','_surface.h','_params.h','/ploc.c','/presets4.py'))}
    assert all(sha(ROOT/name)==digest for name,digest in stable.items())
    snapshots=json.loads((out/'preview/states.json').read_text())
    geometry=json.loads((ROOT/'design/figma-0.6/geometry-report.json').read_text())
    svg_root=ET.parse(ROOT/'plugins/ploc/ploc.svg').getroot()
    sys.path.insert(0,str(inc/'display'));import svgart
    def visible_cost(n,vis,yes=True):
        yes=yes and bool(vis.get(n.get('id'),1))
        if not yes:return (0,0)
        paths=int(n.tag.endswith('path'));vertices=sum(len(points) for points,_ in svgart.subpaths(n.get('d',''))) if paths else (len(svgart.ellipse(0,0,float(n.get('r')),float(n.get('r')))) if n.tag.endswith('circle') else 0)
        for c in n:
            p,v=visible_cost(c,vis,yes);paths+=p;vertices+=v
        return paths,vertices
    live=svg_root.find("{*}g[@id='v6live']")
    costs=[visible_cost(live,{n['id']:n['visible'] for n in s['nativeState']['nodes']}) for s in snapshots]
    validation={'version':version,'target':'Original OP-1 / R246 / Hundekraft 3.1.4','status':'Prepared, cross-compiled, relocated and host-tested. Not installed or OLED/CPU-qualified on hardware.','plugins':plugins,'controls':{'blue':{'turn':'FLOW','shift':'reserved, consumed without changing any parameter'},'green':{'turn':'SIZE','shift':'HEIGHT (existing POUIC)'},'white':{'turn':'SPRAY','shift':'QUALITY (existing VISC word)'},'red':{'turn':'RIPPLE','shift':'IMPACT'}},'visuals':{'handlePoses':12,'sprayDroplets':7,'sprayIdleBrightnessPercent':20,'qualityCleanBrightnessPercent':[20,100],'rippleCount':[1,4],'rippleCenterY':geometry['rippleCenterY'],'rippleDepthScale':geometry['rippleDepthScale'],'impactBrightnessPercent':[20,100],'impactDepthScale':[942/1024,1106/1024],'font':'Native R246 Text for interactive labels and values. Figma CLEAN and H2O artwork retained. No full font file bundled.','snapshots':len(snapshots),'reusedHandleContours':geometry['retainedHandleContours'],'maxVisibleLivePaths':max(x[0] for x in costs),'maxVisibleLiveVertices':max(x[1] for x in costs),'svgBytes':geometry['svgBytes'],'svgTargetExceeded':geometry['svgBytes']>8192,'hardwareGraphicsValidationPending':True,'idleMutations':0},'presets':{'abi':'unchanged 0.4 storage','bankGenerated':False,'existingSoundAndEffectsPreserved':True,'hiddenTailPreserved':True},'dsp':{'changed':False,'byteIdenticalSourceFiles':stable},'checks':['8 ASan/UBSan native harnesses','32-bit freestanding syntax','Blackfin GCC plugins','API/import validation','relocation at 2 bases','native SVG source constraints and fill contour check',f"{geometry['nodesWithIds']} retained SVG node IDs",'OLED photo based label/value clearances and text bounds','all seven controls and persistence','Shift+blue no writes/no stock fallthrough','legacy screen/model and missing-node fallback','500 unchanged GUI frames',f'{len(snapshots)} previews from real C state']}
    (out/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n');(out/'checks.log').write_text(log)
    for name in ['PLOC-GUIDE.md','LICENSE','NOTICE','PROVENANCE.md']:shutil.copy2(ROOT/name,out/('README.md' if name=='PLOC-GUIDE.md' else name))
    source=out/'source';source.mkdir()
    for folder in ['plugins','tests','tools','design']:shutil.copytree(ROOT/folder,source/folder,ignore=shutil.ignore_patterns('__pycache__','.DS_Store'))
    for name in ['README.md','PLOC-GUIDE.md','VALIDATION.md','LICENSE','NOTICE','PROVENANCE.md']:shutil.copy2(ROOT/name,source/name)
    kit=source/'vendor/op1-design-kit/skills/op1-design';(kit/'scripts').mkdir(parents=True)
    shutil.copy2(ROOT/'vendor/op1-design-kit/skills/op1-design/scripts/check_svg.py',kit/'scripts/check_svg.py')
    for name in ['LICENSE','THIRD_PARTY.md','design-kit.json']:
        path=ROOT/'vendor/op1-design-kit'/name
        if path.exists():shutil.copy2(path,kit.parent.parent/name)
    assert not list(source.rglob('opfont.svg')) and not list(source.rglob('*.op1'))
    page='''<!doctype html><html lang="fr"><meta charset="utf-8"><title>PLOC 0.6</title><style>body{font:17px system-ui;background:#eeeae2;color:#202820;max-width:850px;margin:50px auto;padding:24px}a{color:inherit}img{width:100%;height:auto;border-radius:16px}p{line-height:1.5}</style><h1>PLOC / 0.6</h1><p>Prêt à installer sur l’OP-1 original avec Hundekraft 3.1.4.</p><p>FLOW · SIZE / HEIGHT · SPRAY / QUALITY · RIPPLE / IMPACT.</p><p><a href="ploc.knl">PLOC</a> · <a href="plocctrl.knl">PLOCCTRL</a> · <a href="preview/index.html">Voir les états de l’écran</a></p><img src="preview/00.svg" alt="Aperçu de PLOC"><p>Moteur sonore et presets conservés. Shift + bleu réservé. Installation et essais OLED/CPU sur la machine restent à faire.</p></html>'''
    (out/'index.html').write_text(page.replace('PLOC / 0.6','PLOC / '+version).replace('PLOC 0.6','PLOC '+version))
    files=sorted(p for p in out.rglob('*') if p.is_file() and p.name!='SHA256SUMS')
    (out/'SHA256SUMS').write_text(''.join(sha(p)+'  '+str(p.relative_to(out))+'\n' for p in files))
    print(json.dumps({'out':str(out),'plugins':plugins,'snapshots':len(snapshots),'sourceFiles':len(files),'dspUnchanged':True,'presetBankGenerated':False,'maxVisibleLivePaths':max(x[0] for x in costs)},indent=2))

if __name__=='__main__':main()
