#!/usr/bin/env python3
"""PLOC 0.4 native bank. New names keep earlier banks and cached imports intact."""
import json,sys
from pathlib import Path
from presets import aifc
DEFAULT=[3,4,5,2,9,10,9,1,12,0]
FILTER_RANGES=((64,14528),(0,24576),(0,22528),(8192,32767))
# main four, secondary ten, octave, native FX enabled
SOUNDS=[
 ('goutte',[14000,0,3500,0],[3,4,5,0,5,8,10,0,12,0],0,True,'Goutte proche, sèche, claire et brève.'),
 ('pouic air',[19000,7000,5000,0],[10,3,8,0,6,8,9,0,12,0],0,True,'Montée de hauteur assumée, halo léger.'),
 ('perle fine',[7000,4000,1800,0],[2,2,4,0,3,8,12,0,12,0],0,True,'Petite perle brillante, transitoire très courte.'),
 ('ruissel',[14000,11000,2500,12500],[3,4,5,3,10,11,10,1,12,0],0,True,'Filet d’eau irrégulier pendant la tenue de la touche.'),
 ('flow eau',[16384,25000,3000,23000],[3,3,5,3,12,12,10,1,13,0],0,True,'Flow ambient libre, neuf bandes de tailles, halo diffus.'),
 ('flow voile',[18000,31000,7000,27000],[2,2,7,4,11,12,9,3,15,5],0,True,'Flow plus doux, léger attrait vers les octaves de la note.'),
 ('pluie vive',[10000,18000,1600,32767],[3,5,4,7,14,14,12,0,14,0],0,True,'Averse lumineuse, densité maximale et microtexture.'),
 ('lac calme',[26000,28000,11000,8000],[5,2,9,1,7,8,7,3,15,9],0,True,'Gouttes plus grandes et espacées, halo long et musical.'),
]
def packed(values):
    bits=sum(v<<(4*k) for k,v in enumerate(values))
    return [0x6000|(bits&2047),(bits>>11)&32767,(bits>>26)&32767]
FX_PROFILES={'goutte': ('spring', [5500, 13500, 2200, 7000]), 'perle fine': ('spring', [7500, 14800, 3500, 9000]), 'pouic air': ('delay', [6500, 10500, 3500, 10000]), 'ruissel': ('delay', [9000, 14000, 4500, 9500]), 'flow eau': ('reverb', [16000, 20000, 16384, 12000]), 'flow voile': ('reverb', [20500, 18000, 21000, 15500]), 'pluie vive': ('snowglobe', [14500, 5000, 19000, 10500]), 'lac calme': ('snowglobe', [20500, 10500, 24000, 14000])}
RANGES={"delay":((3392,11200),(4969,32767),(0,16384),(1332,32767)),"spring":((1344,16448),(9920,16448),(0,16384),(2047,26120)),"reverb":FILTER_RANGES,"snowglobe":FILTER_RANGES}
def metas():
    for name,main,extra,octave,enabled,desc in SOUNDS:
        fx,values=FX_PROFILES[name]
        if fx in ('reverb','snowglobe'):
            raw=[lo+(hi-lo)*v//32767 for (lo,hi),v in zip(FILTER_RANGES,values)]
            selector=0x5300
        else:raw=values;selector=0
        assert all(lo<=v<=hi for v,(lo,hi) in zip(raw,RANGES[fx]))
        yield dict(adsr=[64,16320,32767,12000,2048,5568,4000,4000],fx_active=True,fx_type=fx,
            fx_params=list(raw)+[selector,0,0,0],knobs=main+[0x5300]+packed(extra),lfo_active=False,lfo_type='tremolo',
            lfo_params=[16000,0,0,16000,0,0,0,0],name=name,octave=octave,synth_version=2,type='ploc')
def generate(folder):
    folder=Path(folder);folder.mkdir(parents=True,exist_ok=True);rows=[]
    for meta,definition in zip(metas(),SOUNDS):
        assert len(meta['name'])<=16 and all(0<=v<=32767 for v in meta['knobs'])
        raw=aifc(meta);path=folder/(meta['name']+'.aif');path.write_bytes(raw)
        rows.append(dict(file=path.name,meta=meta,secondary=definition[2],description=definition[-1]))
    return rows
if __name__=='__main__':print(json.dumps(generate(sys.argv[1]),ensure_ascii=False,indent=2))
