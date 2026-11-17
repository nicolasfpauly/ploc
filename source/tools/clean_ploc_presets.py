#!/usr/bin/env python3
"""Rename PLOC patch metadata while preserving every other AIFF/AIFC chunk.
Device file operations are deliberately separate from this pure byte transform.
"""
import json,re,struct

def chunks(raw):
    if raw[:4]!=b'FORM' or raw[8:12] not in (b'AIFC',b'AIFF') or struct.unpack_from('>I',raw,4)[0]+8!=len(raw):
        raise ValueError('Invalid AIFF FORM')
    at=12;out=[]
    while at<len(raw):
        if at+8>len(raw):raise ValueError('Truncated chunk')
        n=struct.unpack_from('>I',raw,at+4)[0];end=at+8+n+(n&1)
        if end>len(raw):raise ValueError('Truncated chunk payload')
        out.append((raw[at:at+4],raw[at+8:at+8+n],raw[at:end]));at=end
    return out

def metadata(raw):
    found=[json.loads(payload[4:].rstrip(b'\x00 \r\n\t')) for tag,payload,_ in chunks(raw) if tag==b'APPL' and payload[:4]==b'op-1']
    if len(found)!=1:raise ValueError('Expected one OP-1 metadata chunk')
    return found[0]

def rename(raw,name):
    if not re.fullmatch(r'[A-Za-z0-9 _.-]{1,16}',name):raise ValueError('Invalid OP-1 name')
    old=metadata(raw)
    if old.get('type')!='ploc':raise ValueError('Not a PLOC patch')
    new=dict(old,name=name);js=json.dumps(new,sort_keys=True,separators=(',',':')).encode()+b'\n'
    if len(js)&1:js+=b' '
    body=[]
    for tag,payload,original in chunks(raw):
        if tag==b'APPL' and payload[:4]==b'op-1':
            data=b'op-1'+js;body.append(tag+struct.pack('>I',len(data))+data)
        else:body.append(original)
    body=raw[8:12]+b''.join(body);result=b'FORM'+struct.pack('>I',len(body))+body
    assert metadata(result)==new
    assert [(t,p) for t,p,_ in chunks(raw) if t!=b'APPL']==[(t,p) for t,p,_ in chunks(result) if t!=b'APPL']
    assert {k:v for k,v in old.items() if k!='name'}=={k:v for k,v in new.items() if k!='name'}
    return result
