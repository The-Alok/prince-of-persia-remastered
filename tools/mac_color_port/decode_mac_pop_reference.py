import struct, os, json, sys
from pathlib import Path
try:
    from PIL import Image
except Exception:
    Image=None

HFS=Path('/mnt/data/hfs_extract')
COLOR_DATA=HFS/'Prince of Persia__Persia(COLOR).data'
COLOR_RSRC=HFS/'Prince of Persia__Persia(COLOR).rsrc'
APP_RSRC=HFS/'Prince of Persia__Prince of Persia.rsrc'
OUT=Path('/mnt/data/mac_pop_decoded')

def parse_rsrc(path):
    b=Path(path).read_bytes()
    data_off,map_off,data_len,map_len=struct.unpack_from('>IIII',b,0)
    type_list_off=struct.unpack_from('>H',b,map_off+24)[0]
    name_list_off=struct.unpack_from('>H',b,map_off+26)[0]
    tl=map_off+type_list_off
    nt=struct.unpack_from('>H',b,tl)[0]+1
    out={}
    for i in range(nt):
        p=tl+2+i*8
        typ=b[p:p+4].decode('latin1')
        nr=struct.unpack_from('>H',b,p+4)[0]+1
        refrel=struct.unpack_from('>H',b,p+6)[0]
        refs=tl+refrel
        for j in range(nr):
            q=refs+j*12
            rid=struct.unpack_from('>h',b,q)[0]
            nameoff=struct.unpack_from('>H',b,q+2)[0]
            attrs=b[q+4]
            doff=int.from_bytes(b[q+5:q+8],'big')
            ds=data_off+doff
            ln=struct.unpack_from('>I',b,ds)[0]
            dat=b[ds+4:ds+4+ln]
            name=None
            if nameoff != 0xFFFF:
                no=map_off+name_list_off+nameoff
                nlen=b[no]
                name=b[no+1:no+1+nlen].decode('mac_roman','replace')
            out[(typ,rid)]={'type':typ,'id':rid,'name':name,'attrs':attrs,'data':dat}
    return out

def presage_lzss(src, expected=None):
    # Matches resource_dasm decompress_presage_lzss semantics for this data:
    # flags are LSB-first; 0=literal, 1=backref; len=(a>>4)+3; dist=(((a&15)<<8)|b)+1.
    out=bytearray(); i=0
    while i < len(src) and (expected is None or len(out) < expected):
        flags=src[i]; i+=1
        for bit in range(8):
            if expected is not None and len(out) >= expected: break
            if i >= len(src): break
            if flags & (1<<bit):
                if i+1 >= len(src): raise ValueError('truncated backref')
                a,b=src[i],src[i+1]; i+=2
                ln=(a>>4)+3; dist=(((a&0x0F)<<8)|b)+1
                if dist>len(out): raise ValueError(f'bad dist {dist}>{len(out)}')
                for _ in range(ln):
                    out.append(out[-dist])
                    if expected is not None and len(out)>=expected: break
            else:
                out.append(src[i]); i+=1
    if expected is not None and len(out)!=expected:
        raise ValueError(f'decompressed {len(out)} expected {expected}')
    return bytes(out)

def decode_presage_v1(buf,w,h):
    pix=bytearray(w*h)
    alpha=bytearray(w*h)
    i=0; x=0; y=0; stack=[]; stop=False
    def put(v):
        nonlocal x,y
        if 0<=x<w and 0<=y<h:
            idx=y*w+x; pix[idx]=v; alpha[idx]=255
        elif y<h:
            raise ValueError(f'write OOB x={x} y={y} w={w} h={h}')
        x+=1
    guard=0
    while not stop:
        guard+=1
        if guard>10_000_000: raise ValueError('command loop runaway')
        if i>=len(buf): raise ValueError('command stream ended before stop')
        cmd=buf[i]; i+=1
        if cmd & 0x80:
            y+=1; x=0
        count=cmd & 0x1F
        if count==0x1F:
            if i>=len(buf): raise ValueError('truncated extended count')
            count=buf[i]+0x20; i+=1
        else:
            count+=1
        op=cmd & 0x60
        if op==0x00:
            if count==1:
                stop=True
            else:
                if i>=len(buf): raise ValueError('truncated solid run')
                v=buf[i]; i+=1
                for _ in range(count): put(v)
        elif op==0x20:
            if i+count>len(buf): raise ValueError('truncated literal run')
            for v in buf[i:i+count]: put(v)
            i+=count
        elif op==0x40:
            x+=count
        else: # loop control
            count-=1
            if count!=0:
                stack.append([count,i])
                continue
            if not stack:
                continue
            item=stack[-1]
            if item[0]==0:
                stack.pop()
            else:
                item[0]-=1
                i=item[1]
    return bytes(pix),bytes(alpha),i

def parse_clut(dat):
    if len(dat)<8: raise ValueError('clut short')
    seed,flags,size=struct.unpack_from('>IHH',dat,0)
    n=size+1
    colors={}
    p=8
    for _ in range(n):
        idx,r,g,b=struct.unpack_from('>HHHH',dat,p); p+=8
        colors[idx]=(r>>8,g>>8,b>>8)
    maxidx=max(colors) if colors else -1
    arr=[(0,0,0)]*(maxidx+1)
    for k,v in colors.items(): arr[k]=v
    return arr,seed,flags

def main():
    import argparse
    global HFS, COLOR_DATA, COLOR_RSRC, APP_RSRC, OUT
    parser = argparse.ArgumentParser(description="Decode original Macintosh SHPD resources and CLUT")
    parser.add_argument('--hfs-root', type=Path, default=HFS)
    parser.add_argument('--output', type=Path, default=OUT)
    args = parser.parse_args()
    HFS, OUT = args.hfs_root, args.output
    COLOR_DATA=HFS/'Prince of Persia__Persia(COLOR).data'
    COLOR_RSRC=HFS/'Prince of Persia__Persia(COLOR).rsrc'
    APP_RSRC=HFS/'Prince of Persia__Prince of Persia.rsrc'
    OUT.mkdir(parents=True, exist_ok=True)
    r=parse_rsrc(COLOR_RSRC)
    app=parse_rsrc(APP_RSRC)
    palette,seed,flags=parse_clut(app[('clut',2000)]['data'])
    print('palette',len(palette), 'first',palette[:10])
    Path(OUT/'palette.json').write_text(json.dumps(palette))

    data=COLOR_DATA.read_bytes()
    manifest=[]
    for (typ,rid),meta in sorted(r.items(), key=lambda kv:(kv[0][0],kv[0][1])):
        if typ!='SHPD': continue
        off,csz,dsz=struct.unpack('>III',meta['data'])
        raw=data[off:off+(csz or dsz)]
        dec=presage_lzss(raw,dsz) if csz else raw
        shpt=r.get(('SHPT',rid))
        if not shpt: continue
        offsets=list(struct.unpack('>'+('I'*(len(shpt['data'])//4)), shpt['data']))
        valid=[(idx,o) for idx,o in enumerate(offsets) if o!=0xFFFFFFFF]
        for pos,(idx,o) in enumerate(valid):
            # end is next table entry value if present and valid according to resource_dasm's peeking semantics.
            # Since 0xFFFFFFFF can appear, use the next valid offset; it is equivalent for slicing actual records.
            end=valid[pos+1][1] if pos+1<len(valid) else len(dec)
            rec=dec[o:end]
            if len(rec)<8: continue
            w,h,ox,oy=struct.unpack_from('>hhhh',rec,0)
            try:
                pix,a,cons=decode_presage_v1(rec[8:],w,h)
            except Exception as e:
                print('decode error',rid,idx,w,h,e)
                raise
            used=sorted(set(p for p,aa in zip(pix,a) if aa))
            manifest.append({'shpd':rid,'slot':idx,'width':w,'height':h,'origin_x':ox,'origin_y':oy,
                             'record_offset':o,'record_end':end,'commands_consumed':cons,'colors':used})
            if Image and ((rid==400 and idx in [1,2,3,4,5,10,20,50,100,150,200]) or (rid in [40,50,1200,2200] and idx in [0,1,20,21])):
                im=Image.new('RGBA',(w,h)); rgba=[]
                for v,aa in zip(pix,a):
                    if aa:
                        rr,gg,bb=palette[v] if v<len(palette) else (255,0,255)
                        rgba.append((rr,gg,bb,255))
                    else: rgba.append((0,0,0,0))
                im.putdata(rgba)
                im.save(OUT/f'{rid}_{idx}_{w}x{h}_o{ox}_{oy}.png')
    print('decoded images',len(manifest))
    Path(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
    print('max palette index',max((max(x['colors']) if x['colors'] else 0 for x in manifest)))


if __name__ == '__main__':
    main()
