#!/usr/bin/env python3
"""Eight PLOC 0.3 bubble-engine presets. Native AIFC writer adapted from
Hundekraft MBIRA presets.py, SDK 3.1.4 (C) 2026 Hundekraft.
GPL-3.0-or-later with OP-1 firmware permission; see LICENSE / NOTICE.
No audio samples: the OP-1 generates its own native preview on sync.
"""
import json, math, struct, sys
from pathlib import Path
SOUNDS=[
    ('perle',[6000,0,6000,0],1,None,'Petite bulle claire, sans goutte secondaire.'),
    ('pouic',[16384,9000,9500,0],0,None,'Le pouic ascendant au premier plan, légère résonance.'),
    ('huile',[24000,7000,30000,8192],0,None,'Attaque arrondie, montée lente, une bulle satellite.'),
    ('profonde',[30000,24000,14000,12288],-1,None,'Grosse bulle grave, résonance et deux satellites.'),
    ('ricochet',[15000,14000,7000,21846],0,('delay',[7000,12000,5000,9000]),'Quatre satellites, puis les échos du DELAY natif.'),
    ('cascade',[10000,12000,5000,32767],0,None,'Eau vive et cinq bulles secondaires par note.'),
    ('sous eau',[26000,22000,26000,10923],0,None,'Matière sombre, résonance profonde et deux satellites.'),
    ('lac lune',[20000,32767,12000,16384],0,('spring',[12000,15000,5500,12000]),'Large résonance, trois satellites et SPRING natif.'),
]
RANGES={'delay':[(3392,11200),(4969,32767),(0,16384),(1332,32767)],'spring':[(1344,16448),(9920,16448),(0,16384),(2047,26120)]}

def metas():
    for name,k,octave,fx,description in SOUNDS:
        yield {'adsr':[64,14000,32767,10000,2048,5568,4000,4000],
            'fx_active':fx is not None,'fx_type':fx[0] if fx else 'delay',
            'fx_params':list(fx[1] if fx else [8000]*4)+[0]*4,
            'knobs':list(k)+[0x5300,3,0,0],
            'lfo_active':False,'lfo_type':'tremolo','lfo_params':[16000,0,0,16000,0,0,0,0],
            'name':name,'octave':octave,'synth_version':2,'type':'ploc'}

def chunk(cid,data):return cid+struct.pack('>I',len(data))+data+(b'\0' if len(data)&1 else b'')

def aifc(meta):
    rate=44100;exp=int(math.floor(math.log2(rate)))
    extended=struct.pack('>HQ',exp+16383,int(rate/2**exp*2**63))
    comm=struct.pack('>hIh',1,0,16)+extended+b'sowt'+b')Signed integer (little-endian) linear PCM'
    js=json.dumps(meta,separators=(',',':'),sort_keys=True).encode()+b'\n'
    if len(js)&1:js+=b' '
    body=chunk(b'FVER',bytes.fromhex('a2805140'))+chunk(b'COMM',comm)+chunk(b'APPL',b'op-1'+js)+chunk(b'SSND',struct.pack('>II',0,0))
    return b'FORM'+struct.pack('>I',len(body)+4)+b'AIFC'+body

def generate(out):
    out=Path(out);out.mkdir(parents=True,exist_ok=True);rows=[]
    for m,definition in zip(metas(),SOUNDS):
        assert len(m['knobs'])==8 and all(0<=v<=32767 for v in m['knobs'])
        assert m['knobs'][5]==3 and len(m['name'])<=16
        if m['fx_active']:
            assert all(lo<=v<=hi for v,(lo,hi) in zip(m['fx_params'],RANGES[m['fx_type']]))
        data=aifc(m);path=out/(m['name']+'.aif');path.write_bytes(data)
        rows.append({'file':path.name,'description':definition[-1],'meta':m})
    return rows

if __name__=='__main__':
    out=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('presets')
    print('Generated',len(generate(out)),'presets:',out)
