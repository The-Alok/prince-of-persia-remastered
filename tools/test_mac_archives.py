#!/usr/bin/env python3
"""Test native HQX/StuffIt/ISO parsing, optionally importing a private game disk.

No copyrighted archive fixtures are checked in. Synthetic archives are built
from small test strings; --disk additionally wraps a player's disk in HQX,
classic StuffIt and ISO9660 and compares all resulting packs with that disk.
"""
from pathlib import Path
import argparse
import base64
import binascii
import hashlib
import os
import shlex
import struct
import subprocess
import tempfile
import textwrap

ROOT = Path(__file__).resolve().parents[1]
ALPHABET = b'!"#$%&\'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr'
BASE64 = b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'


def crc(data):
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = value >> 1 ^ (0xA001 if value & 1 else 0)
    return value


def hqx(name, data, resource=b'', kind=b'DATA'):
    name = name.encode('ascii')
    header = bytes([len(name)]) + name + b'\0' + kind + b'TEST' + b'\0\0' + struct.pack('>II', len(data), len(resource))
    raw = header + struct.pack('>H', binascii.crc_hqx(header, 0)) + data + struct.pack('>H', binascii.crc_hqx(data, 0)) + resource + struct.pack('>H', binascii.crc_hqx(resource, 0))
    encoded = base64.b64encode(raw.replace(b'\x90', b'\x90\0')).rstrip(b'=').translate(bytes.maketrans(BASE64, ALPHABET)).decode('ascii')
    return ('(This file must be converted with BinHex 4.0)\r\n\r\n:' + '\r\n'.join(textwrap.wrap(encoded, 64)) + ':\r\n').encode('ascii')


def sit(name, data, resource=b'', method=0):
    name = name.encode('ascii')
    head = bytearray(112)
    head[0] = head[1] = method
    head[2] = len(name)
    head[3:3+len(name)] = name
    head[66:74] = b'DATATEST'
    rd = resource.replace(b'\x90', b'\x90\0') if method else resource
    dd = data.replace(b'\x90', b'\x90\0') if method else data
    struct.pack_into('>IIIIHH', head, 84, len(resource), len(data), len(rd), len(dd), crc(resource), crc(data))
    struct.pack_into('>H', head, 110, crc(head[:110]))
    return b'SIT!' + struct.pack('>HI', 1, 22+112+len(rd)+len(dd)) + b'rLau' + b'\x01'+b'\0'*7 + head + rd + dd


def iso_files(files):
    """Small ISO9660 with Rock Ridge names and real file extents."""
    blocks = sum((len(data)+2047)//2048 for _, data in files)
    image = bytearray((22+blocks)*2048)
    def record(block, length, name, directory=False, rock_name=None):
        extra = b'' if rock_name is None else b'NM' + bytes([5+len(rock_name), 1, 0]) + rock_name
        n = 33+len(name)+(not len(name)%2)+len(extra)
        r = bytearray(n)
        r[0] = n
        struct.pack_into('<I', r, 2, block); struct.pack_into('>I', r, 6, block)
        struct.pack_into('<I', r, 10, length); struct.pack_into('>I', r, 14, length)
        r[25] = 2 if directory else 0
        r[28:32] = b'\x01\0\0\x01'
        r[32] = len(name); r[33:33+len(name)] = name
        if extra:
            r[-len(extra):] = extra
        return r
    pvd = 16*2048
    image[pvd:pvd+7] = b'\x01CD001\x01'
    struct.pack_into('<I', image, pvd+80, len(image)//2048)
    struct.pack_into('>I', image, pvd+84, len(image)//2048)
    image[pvd+128:pvd+132] = b'\0\x08\x08\0'
    root = record(20, 2048, b'\0', True)
    image[pvd+156:pvd+156+len(root)] = root
    image[17*2048:17*2048+7] = b'\xffCD001\x01'
    directory = root + record(20, 2048, b'\x01', True)
    block = 21
    for index, (name, data) in enumerate(files):
        directory += record(block, len(data), f'FILE{index}.DAT;1'.encode(), rock_name=name.encode('ascii'))
        image[block*2048:block*2048+len(data)] = data
        block += (len(data)+2047)//2048
    assert len(directory) <= 2048
    image[20*2048:20*2048+len(directory)] = directory
    return bytes(image)


def iso(name, data):
    return iso_files([(name, data)])


def apple_double(resource, kind=b'DATA'):
    return (struct.pack('>II16sH', 0x00051607, 0x00020000, bytes(16), 2)
            + struct.pack('>III', 9, 50, 32) + struct.pack('>III', 2, 82, len(resource))
            + kind + bytes(28) + resource)


HARNESS = r'''
#include "mac_archive.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static uint32_t hash(const unsigned char *p,size_t n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static int visit(const char *name,const char type[4],const unsigned char*d,size_t dn,const unsigned char*r,size_t rn,void*c){(void)type;(void)c;printf("%s %zu %08x %zu %08x\n",name,dn,hash(d,dn),rn,hash(r,rn));return 1;}
int main(int argc,char**argv){if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char*p=malloc(n?n:1);if(!p||fread(p,1,n,f)!=(size_t)n){fclose(f);free(p);return 2;}fclose(f);char error[512]={0};int ok=mac_archive_read(p,n,argv[1],visit,NULL,error,sizeof(error));free(p);if(!ok)fprintf(stderr,"%s\n",error);return ok?0:1;}
'''


def fnv(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte)*16777619) & 0xFFFFFFFF
    return f'{value:08x}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disk', type=Path)
    parser.add_argument('--sit', type=Path)
    parser.add_argument('--hqx', type=Path, help='optional supplied HQX, reports validity without modifying it')
    args = parser.parse_args()
    checks = 0
    with tempfile.TemporaryDirectory(prefix='mac-archives-') as temporary:
        work = Path(temporary)
        (work/'check.c').write_text(HARNESS)
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
        subprocess.run(['/usr/bin/gcc', '-std=c99', '-O2', '-Wall', '-Isrc', str(work/'check.c'), 'src/mac_archive.c', *flags, '-o', str(work/'check')], cwd=ROOT, check=True)
        def check(name, content, expected=None):
            nonlocal checks
            p = work/name
            p.write_bytes(content)
            result = subprocess.run([str(work/'check'), str(p)], capture_output=True, timeout=20)
            if expected is None:
                assert result.returncode == 1, (name, result.stderr)
            else:
                assert result.returncode == 0 and result.stdout.decode().strip() == expected, (name, result.stdout, result.stderr)
            checks += 1
        data = b'archive test\x90 fork\0'*8
        resource = b'resource fork\0'
        expected = f'file {len(data)} {fnv(data)} {len(resource)} {fnv(resource)}'
        check('valid.hqx', hqx('file', data, resource), expected)
        check('stored.sit', sit('file', data, resource), expected)
        check('rle.sit', sit('file', data, resource, 1), expected)
        check('real.iso', iso('FILE', data), f'FILE {len(data)} {fnv(data)} 0 {fnv(b"")}')
        check('forks.iso', iso_files([('file', data), ('._file', apple_double(resource))]), expected)
        for name, payload in [('bad.hqx', hqx('file', data, resource).replace(b':', b'', 1)), ('badcrc.sit', sit('file', data, resource)[:-1]+b'X'), ('truncated.sit', sit('file', data, resource)[:-4]), ('truncated.iso', iso('FILE', data)[:41000]), ('unknown.iso', b'not an image')]:
            check(name, payload)
        # CRC-correct malformed StuffIt headers and bounded expansion failures.
        corrupt = bytearray(sit('file', data))
        corrupt[22+88:22+92] = b'\xff'*4
        struct.pack_into('>H', corrupt, 22+110, crc(corrupt[22:22+110]))
        check('overflow.sit', corrupt)
        if args.disk:
            original = args.disk.read_bytes()
            variants = {'raw.hfs': original, 'encoded.hqx': hqx('PoP1.dsk', original), 'stored-disk.sit': sit('PoP1.dsk', original), 'disk.iso': iso('POP1.DSK', original), 'hfs-bridge.iso': original}
            import sys
            sys.path.insert(0, str(ROOT/'tools/mac_native_renderer'))
            from extract_mac_reference import hfs_files
            preserved = []
            for file in hfs_files(original, include=lambda f: f['type'] == 'APPL' or f['name'] == 'Persia(COLOR)'):
                preserved += [(file['name'], file['data']), ('._'+file['name'], apple_double(file['rsrc'], file['type'].encode('ascii')))]
            variants['preserved-forks.iso'] = iso_files(preserved)
            # A valid BinHex-wrapped StuffIt archive exercises nested parsing.
            if args.sit:
                variants['actual.sit'] = args.sit.read_bytes()
                variants['nested.hqx'] = hqx('Game.sit', variants['actual.sit'])
            digests = set()
            for name, payload in variants.items():
                path = work/name; path.write_bytes(payload)
                output = work/(name+'.pak')
                subprocess.run([str(ROOT/'prince'), '--import-mac', str(path), str(output)], check=True, timeout=120)
                digests.add(hashlib.sha256(output.read_bytes()).hexdigest()); checks += 1
            assert len(digests) == 1, digests
            print('Disk/archive imports produce identical PAK:', next(iter(digests)))
        if args.hqx:
            result = subprocess.run([str(ROOT/'prince'), '--import-mac', str(args.hqx.resolve()), str(work/'supplied-hqx.pak')], capture_output=True, timeout=120)
            print('Supplied HQX:', 'valid' if result.returncode == 0 else result.stderr.decode().strip())
    print(f'MAC ARCHIVES: {checks} checks, 0 failures')


if __name__ == '__main__':
    main()
