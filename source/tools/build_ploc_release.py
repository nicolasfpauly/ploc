#!/usr/bin/env python3
"""Reproducible PLOC 0.3 release: Blackfin plugin, native presets, C checks,
dry audio and host screen previews. GPL-3.0-or-later; see NOTICE.
"""
import argparse,array,hashlib,html,importlib.util,json,math,os,re,shutil,struct,subprocess,sys,tempfile,wave,zipfile
from pathlib import Path
from urllib.parse import quote
ROOT=Path(__file__).resolve().parents[1]
RATE=44100
VERSION='0.3.0'

def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def run(args,**kwargs):return subprocess.run([str(a) for a in args],check=True,capture_output=True,text=True,**kwargs)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def wav(path,samples):
    a=array.array('h',samples)
    if sys.byteorder!='little':a.byteswap()
    with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(a.tobytes())

def inspect(raw,runtime):
    assert raw[:4]==b'FORM' and raw[8:12]==b'AIFC' and struct.unpack_from('>I',raw,4)[0]+8==len(raw)
    at=12;chunks={}
    while at<len(raw):
        cid=raw[at:at+4];n=struct.unpack_from('>I',raw,at+4)[0];assert cid not in chunks and at+8+n<=len(raw)
        chunks[cid]=raw[at+8:at+8+n];at+=8+n+(n&1)
    assert at==len(raw) and set(chunks)=={b'FVER',b'COMM',b'APPL',b'SSND'}
    assert struct.unpack_from('>hIh',chunks[b'COMM'])==(1,0,16) and chunks[b'COMM'][18:22]==b'sowt' and chunks[b'SSND']==bytes(8)
    m=runtime.preset_meta(raw);assert m['type']=='ploc' and m['knobs'][4:]==[0x5300,3,0,0]
    assert all(lo<=v<=hi for v,(lo,hi) in zip(m['adsr'][:4],[(64,16320),(64,16320),(0,32767),(64,16320)]))
    return m

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=ROOT.parent/('PLOC-'+VERSION));ap.add_argument('--sdk-source',type=Path,default=ROOT/'sdk/hundekraft-3.1.4-source');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True);sdk=args.sdk_source.resolve();kennel=sdk/'ext/kennel'
    assert (kennel/'tools/kennel/pack.py').is_file(),'Use --sdk-source with the official extracted source root'
    assert json.loads((ROOT/'plugins/ploc/plugin.json').read_text())['version']==VERSION
    pack=module('ploc_pack',kennel/'tools/kennel/pack.py');runtime=module('ploc_runtime',sdk/'tools/firmware/runtime.py');presets=module('ploc_presets',ROOT/'plugins/ploc/presets.py')
    run([sys.executable,ROOT/'plugins/ploc/make_tables.py']);run([sys.executable,ROOT/'tools/build_ploc_screen.py'])
    env=os.environ.copy();env['PATH']=str(ROOT/'.toolchain/bin')+os.pathsep+env.get('PATH','')
    built=run([sys.executable,kennel/'tools/kennel/pack.py',ROOT/'plugins/ploc','-o',out/'ploc.knl'],env=env);log=built.stdout+built.stderr
    parsed=pack.F.parse((out/'ploc.knl').read_bytes(),api_version=1);assert parsed['type_name']=='ploc' and parsed['page2_defaults']==[3,0,0]
    exports=set(re.findall(r'\{\s*"([^"\n]+)"\s*,',pack.core_exports()));assert set(parsed['imports'])<=exports
    synthetic={name:0x01010000+64*i for i,name in enumerate(parsed['imports'])};relocated=[]
    for base in (0x10000000,0x18000000):relocated.append({'base':hex(base),'bytes':len(pack.F.relocate(parsed,base,synthetic,stubs=True))})
    audio=out/'audio';audio.mkdir(exist_ok=True);rows=presets.generate(out/'synth/ploc');montage=[];sweep=[]
    with tempfile.TemporaryDirectory(prefix='ploc-03-') as temp:
        td=Path(temp);flags=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
        for name in ('dsp','ploc3'):
            exe=td/name;run(flags+[ROOT/('tests/test_'+name+'.c'),'-o',exe]);qa=run([exe]);log+=qa.stdout+qa.stderr
        screen=td/'screen';run(flags+['-Wno-unknown-pragmas','-I'+str(kennel/'synth_sdk'),ROOT/'tests/test_ploc_screen.c','-o',screen]);qa=run([screen]);log+=qa.stdout+qa.stderr
        preview=run([sys.executable,ROOT/'tools/preview_ploc_screen.py','--host',screen,'--out',out/'preview']);log+=preview.stdout+preview.stderr
        exe=td/'render';run(flags+[ROOT/'tools/render_ploc_bank.c','-o',exe])
        def audition(knobs,octave=0):
            raw=td/'sound.raw';run([exe,*knobs,octave,3,raw]);a=array.array('i');a.frombytes(raw.read_bytes())
            if sys.byteorder!='little':a.byteswap()
            assert len(a)==RATE*44//10
            values=[round(v*.5) for v in a];assert max(map(abs,values))<32767 and max(map(abs,values))>50
            for j in range(441):values[-441+j]=round(values[-441+j]*(440-j)/440)
            return values
        for row in rows:
            p=out/'synth/ploc'/row['file'];m=inspect(p.read_bytes(),runtime);assert m==row['meta']
            values=audition(m['knobs'][:4],m['octave']);name=p.stem+'.wav';wav(audio/name,values)
            row.update({'sha256':sha(p),'audio':name,'extra_droplets':m['knobs'][3]*6>>15,'audio_peak_dbfs':round(20*math.log10(max(map(abs,values))/32768),2),'montage_start_seconds':round(len(montage)/RATE,2)})
            montage+=values+[0]*(RATE//4)
        for count in range(6):
            values=audition([16384,12000,12288,(count*32768+5)//6]);wav(audio/('DROPS-'+str(count)+'.wav'),values);sweep+=values+[0]*(RATE//4)
    wav(audio/'PLOC-03-tour-8-presets.wav',montage);wav(audio/'DROPS-0-a-5.wav',sweep)
    validation={'version':VERSION,'status':'local checks passed; native 0.3 installation and OLED/audio/CPU not yet tested','plugin_sha256':sha(out/'ploc.knl'),'plugin_bytes':(out/'ploc.knl').stat().st_size,'api':parsed['api_version'],'imports':parsed['imports'],'page2_defaults':parsed['page2_defaults'],'synthetic_relocations':relocated,'source_dsp_sha256':sha(ROOT/'plugins/ploc/ploc3_dsp.h'),'voice_state_bytes':32,'voice_object_bytes':36,'controls':['SIZE','RIPPLE','VISC','DROPS'],'drop_count_range':[0,5],'drops_update':'screen immediately; sound count latched at note-on','preset_count':len(rows),'native_preview':'zero-frame AIFC SSND; generated by OP-1 at sync','audio_method':'production integer C on Mac, native T2/T3/T4 and mixer excluded; common gain x0.5, last 10ms fade','stimulus':'MIDI 60,67,72 plus preset octave at .12,1,1.88s, full velocity, seeds 0/1/2, 4.4s per clip','presets':rows}
    (out/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n');(out/'checks.log').write_text(log)
    cards=[];table=[]
    for row in rows:
        m=row['meta'];values=[round(v*99/32767) for v in m['knobs'][:3]]+[row['extra_droplets']];fx=m['fx_type'].upper() if m['fx_active'] else 'sec'
        cards.append('<article><h2>'+html.escape(m['name'])+'</h2><p>'+html.escape(row['description'])+'</p><div class="knobs">'+''.join('<span class="k'+str(i)+'">'+lab+'<b>'+str(v)+'</b></span>' for i,(lab,v) in enumerate(zip(validation['controls'],values)))+'</div><small>Octave '+str(m['octave'])+' · preset '+fx+'</small><audio controls preload="none" src="audio/'+quote(row['audio'])+'"></audio><a href="synth/ploc/'+quote(row['file'])+'">Preset .aif</a></article>')
        table.append('| '+m['name']+' | '+' | '.join(map(str,values))+' | '+str(m['octave'])+' | '+fx+' |')
    page='''<!doctype html><html lang="fr"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>PLOC 0.3</title><style>body{background:#eee9df;color:#202828;font:16px system-ui;max-width:1100px;margin:36px auto;padding:0 24px}h1{font-size:44px;letter-spacing:-1.5px;margin-bottom:8px}p{line-height:1.5;max-width:850px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:18px;margin:30px 0}article{background:#090c0f;color:white;padding:24px;border-radius:4px}h2{font:22px monospace;margin-top:0}article p{min-height:48px}.knobs{display:flex;gap:14px;margin:24px 0;font:11px monospace}.knobs b{display:block;font-size:25px;margin-top:7px}.k0{color:#698eff}.k1{color:#00ed95}.k2{color:white}.k3{color:#ff423f}audio{display:block;width:100%;margin:16px 0}a{color:inherit}small{opacity:.8}</style><h1>PLOC / 0.3</h1><p>SIZE, RIPPLE, VISC et DROPS. Le pouic ascendant, de l’eau vive à une matière plus huileuse, avec 0 à 5 bulles secondaires par note.</p><p><a href="preview/index.html">Essayer le potard rouge dans l’aperçu de l’écran →</a></p><p><strong>Comparaison DROPS 0 → 5</strong>, mêmes autres réglages :</p><audio controls preload="none" src="audio/DROPS-0-a-5.wav"></audio><p><strong>Tour de la banque 10 → 17 :</strong></p><audio controls preload="none" src="audio/PLOC-03-tour-8-presets.wav"></audio><p><small>Écoutes du DSP sec sur Mac, avec gain commun ×0,5. L’enveloppe T2, les effets T3 et le LFO T4 natifs sont exclus. Les presets 14 et 17 utiliseront leurs effets sur l’instrument.</small></p><main class="grid">'''+''.join(cards)+'''</main><p><a href="README.md">Installation et paramètres</a> · <a href="validation.json">Validation</a></p></html>'''
    (out/'index.html').write_text(page)
    readme='''# PLOC 0.3.0

Modèle et écran actualisés d’après le croquis de Nicolas, 6 novembre 2026.
Cible : OP-1 original, Hundekraft 3.1.4 / R246. RAIN est conservé.

## Quatre réglages

| Encodeur | Paramètre | Son et écran |
|---|---|---|
| Bleu | SIZE | Taille relative, grand = plus grave ; largeur de la goutte principale. Le clavier transpose le son. |
| Vert | RIPPLE | Quantité et durée d’un halo résonant à deux modes, autour de la bulle principale ; les trois anneaux verts grandissent. |
| Blanc | VISC | 0 = eau vive, 99 = huileux. Amortissement accru, attaque plus douce, filtre plus sombre et montée de hauteur plus lente. La jauge blanche suit le réglage. |
| Rouge | DROPS | 0 à 5 bulles secondaires. Autant de satellites orange et de repères apparaissent immédiatement. Le nombre sonore est capturé à chaque note, pour laisser finir les bulles déjà lancées. |

Le noyau garde une bulle presque sinusoïdale avec un glissement ascendant,
lié au modèle amorti étudié. VISC est un macro musical, pas une simulation
calibrée de l’huile. RIPPLE ajoute deux résonances artistiques à 0,8× et 2,17×
la fréquence principale. Les satellites ont leurs propres enveloppes et
phases, avec de petites variantes de hauteur et d’arrivée. Le premier arrive
vers 20 ms et le cinquième vers 105 ms. La bulle principale arrive à 6 ms.

L’attaque et l’amortissement varient avec VISC ; le glissement ascendant reste
présent et ralentit côté huileux. La durée maximale est bornée à environ 5,94 s
pour terminer les queues devenues inaudibles. Six voix OP-1, chacune contenant
une bulle principale, jusqu’à cinq satellites et deux modes de résonance.
L’état occupe 32 octets par voix, plus les 4 octets de vtable réservés au firmware.
Aucun flottant, allocation ou division par échantillon. Les calculs de réglages
et divisions sont par bloc de 128 échantillons.

Le niveau de la bulle principale conserve la correction de gain de 0.2.1
(+12,04 dB par rapport à 0.2.0). Les pics et la mémoire sont vérifiés localement ;
polyphonie, FX et charge CPU sont encore à vérifier sur l’OP-1.

## Banque 10–17

Les nouveaux noms évitent de réutiliser les fichiers de la banque 01–08,
car le firmware peut conserver un ancien son portant le même nom.
Les presets ci-dessous portent `type=ploc`, `knobs[4]=0x5300`, `knobs[5]=3`.

| Preset | SIZE | RIPPLE | VISC | DROPS | Octave | T3 |
|---|---:|---:|---:|---:|---:|---|
'''+ '\n'.join(table)+'''

Les anciens presets avec le marqueur 2 gardent le modèle 0.2 et ses lectures
BODY/TAIL/TONE. Les anciens sons sans ce marqueur gardent le modèle 0.1.
Pour les nouveaux contrôles, charger un preset 10–17 ou sélectionner à nouveau
le moteur PLOC avec ses valeurs par défaut. Le nouveau moteur est obligatoire :
le plugin 0.2 ne sait pas interpréter le marqueur 3.

## Fichiers et installation

- `ploc.knl` : moteur et écran 0.3.0.
- `synth/ploc/*.aif` : huit réglages de synthèse, sans sample embarqué.
- `index.html`, `audio/` : écoutes sèches produites par le même DSP C sur Mac.
- `preview/index.html` : aperçu interactif DROPS 0–5, généré à partir des
  transformations et visibilités des vrais callbacks d’écran. Police Iosevka
  de remplacement, pas une capture OLED. Le plugin emploie la police native.
- `source/`, `checks.log`, `validation.json`, `SHA256SUMS` : sources et preuves.

1. Dans Firefox, ajouter ce `ploc.knl` aux plugins du workshop Hundekraft.
2. Examiner le plan : remplacement de PLOC uniquement, RAIN et les autres
   plugins conservés. Envoyer puis confirmer T1 sur l’OP-1 si demandé.
3. Attendre le redémarrage et la vérification du workshop.
4. Passer en mode disque (Shift + COM, T3), copier uniquement les huit nouveaux
   `.aif` dans `synth/ploc/`, puis éjecter proprement pour synchroniser.
5. Charger un nouveau preset 10–17. La banque 01–08 conserve ses anciens réglages.

Cette livraison préparée localement ne prouve pas l’installation matérielle.
Le journal d’installation est enregistré séparément après vérification. Le
retour arrière consiste à renvoyer `../PLOC-0.2.0/ploc.knl` via le workshop ;
la banque 01–08 précédente est conservée, comme la sauvegarde originale 0.1.

## Vérifications

ASan/UBSan : 2048 combinaisons extrêmes pour chaque modèle, calcul du pouic
ascendant, montée ralentie par VISC, halo RIPPLE, cinq satellites distincts,
tous les 32768 mots DROPS, fin au silence, changements de réglages en jeu et
préservation des modèles précédents. Écran : nombre exact de satellites et
repères dans les deux sens, sans note jouée ; lectures 0..99, objets Text
retenus, ré-entrée, libellés des anciens sons et nœuds absents. Compilation
Blackfin réelle, CRC/imports/API et deux relocations synthétiques via le SDK.

## Reproduire

Depuis `source/`, avec clang et les outils bfin-elf dans PATH :

    python3 tools/build_ploc_release.py --sdk-source /chemin/hundekraft-3.1.4-source --out /chemin/reconstruction

Le SDK doit être l’archive officielle 3.1.4 du dépôt
https://github.com/Hundekraft/op1-workshop, révision
`1cb2e0be590e2a82d82f9ad6f9b3410bc97b7fae`, SHA-256 de l’archive source
`e8a72443ecbe5e87577aabffbbc5a06277d969bc13bfc54532cc2decee83b44c`.

Licence GPL-3.0-or-later et permission firmware : `source/LICENSE`,
`source/NOTICE`. Iosevka conserve sa licence OFL. Aucune police ni ressource
extraite du firmware Teenage Engineering n’est distribuée. Les vecteurs du
croquis et de Figma sont ceux du projet de Nicolas.
'''
    (out/'README.md').write_text(readme)
    source=out/'source';source.mkdir(exist_ok=True)
    files=[p for p in (ROOT/'plugins/ploc').rglob('*') if p.is_file() and p.suffix in ('.c','.h','.py','.svg','.json') and '__pycache__' not in p.parts]
    files += [ROOT/('tests/test_'+n+'.c') for n in ('dsp','ploc3','ploc_screen')]
    files += [ROOT/'tools'/name for name in ('render_ploc_bank.c','build_ploc_screen.py','port_ploc_figma.py','preview_ploc_screen.py','build_ploc_release.py')]
    files += [ROOT/'LICENSE',ROOT/'NOTICE']
    for p in files:
        q=source/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
    fontdir=ROOT/'vendor/op1-design-kit/skills/op1-design/assets/fonts'
    if not fontdir.exists():fontdir=ROOT/'host-fonts'
    (source/'host-fonts').mkdir(exist_ok=True)
    for name in ('Iosevka-Regular.woff2','Iosevka-ExtraLight.woff2','Iosevka-OFL.md'):shutil.copy2(fontdir/name,source/'host-fonts'/name)
    files=sorted(p for p in out.rglob('*') if p.is_file() and p.name!='SHA256SUMS' and '__pycache__' not in p.parts)
    (out/'SHA256SUMS').write_text(''.join(sha(p)+'  '+p.relative_to(out).as_posix()+'\n' for p in files))
    archive=out.parent/('PLOC-'+VERSION+'-bulles-presets.zip')
    with zipfile.ZipFile(archive,'w') as z:
        for p in sorted(out.rglob('*')):
            if p.is_file() and '__pycache__' not in p.parts:
                info=zipfile.ZipInfo((Path(out.name)/p.relative_to(out)).as_posix(),date_time=(2026,11,6,12,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o644<<16;z.writestr(info,p.read_bytes())
    print(log.strip());print('Prepared release:',out);print('Archive:',archive)
if __name__=='__main__':main()
