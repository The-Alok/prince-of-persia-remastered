#!/usr/bin/env python3
"""Structural validator for res_mac_native.pak.

It verifies POPSPAK1 structure, sorted keys, bounds, native flag, and confirms that
2x top-left nearest-neighbour reduction exactly reproduces every Mac-replaced
image in res_mac_color.pak.
"""
import struct, sys
from pathlib import Path
from PIL import Image

H=struct.Struct('<8sII'); E=struct.Struct('<16s4sHHHBBII')

def read(path):
    b=Path(path).read_bytes(); magic,ver,n=H.unpack_from(b,0)
    assert magic==b'POPSPAK1' and ver==1
    out={}; origins={}; order=[]
    for i in range(n):
        vals=E.unpack_from(b,H.size+i*E.size)
        f,e,r,w,h,nc,fl,off,size=vals
        f=f.rstrip(b'\0').decode(); e=e.rstrip(b'\0').decode()
        assert off+size <= len(b)
        key=(f.lower(),r,e.lower()); order.append(key)
        payload=b[off:off+size]
        if e=='png':
            assert nc>0 and size==nc*3+w*h
            pal=[tuple(payload[j:j+3]) for j in range(0,nc*3,3)]
            pix=payload[nc*3:]
            im=Image.new('RGBA',(w,h)); rgba=[]
            for v in pix:
                rgb=pal[v]; a=0 if (fl&1 and v==0) else 255
                rgba.append((*rgb,a))
            im.putdata(rgba); out[(f,r)]=(im,fl)
        elif e=='org':
            assert size==4
            origins[(f,r)]=struct.unpack('<hh', payload)
    assert order==sorted(order)
    return out, origins

def main():
    root=Path(__file__).resolve().parents[2]
    native, origins=read(root/'tools/mac_reference/legacy-packs/res_mac_native.pak')
    fitted, _=read(root/'tools/mac_reference/legacy-packs/res_mac_color.pak')
    assert len(native)==517, len(native)
    assert len(origins)==len(native), (len(origins), len(native))
    for key,(nim,flags) in native.items():
        assert flags & 2, key
        sim,_=fitted[key]
        assert sim.size==((nim.width+1)//2,(nim.height+1)//2), (key,nim.size,sim.size)
        src=nim.load(); dst=sim.load()
        for y in range(sim.height):
            for x in range(sim.width):
                assert src[x*2,y*2]==dst[x,y], (key,x,y)
    print(f'OK: {len(native)} native images + {len(origins)} SHPD origins valid; pixels exactly match the known 320x200 Mac pack when reduced 2:1.')

if __name__=='__main__': main()
