#!/usr/bin/env python3
"""Read-only extraction of classic-HFS files and Macintosh resource forks.

Raw HFS volumes, Apple Partition Maps and Disk Copy 4.2 containers are supported.
No filesystem mounting or execution of the supplied game is required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import os


def bounded(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError(f'out-of-bounds disk/resource span: {offset}+{size}')
    return data[offset:offset + size]


def leaf_records(tree):
    node_size = struct.unpack_from('>H', bounded(tree, 32, 2))[0]
    node = struct.unpack_from('>I', bounded(tree, 24, 4))[0]
    if node_size < 256 or node_size & (node_size - 1):
        raise ValueError('invalid HFS B-tree node size')
    visited = set()
    while node:
        if node in visited:
            raise ValueError('cycle in HFS leaf chain')
        visited.add(node)
        raw = bounded(tree, node * node_size, node_size)
        following, _, kind, _, count, _ = struct.unpack_from('>IIbBHH', raw)
        if kind != -1 or 2 * (count + 1) > node_size - 14:
            raise ValueError('invalid HFS catalog leaf')
        offsets = [struct.unpack_from('>H', raw, node_size - 2 * (i + 1))[0] for i in range(count + 1)]
        if offsets != sorted(offsets) or offsets[0] < 14 or offsets[-1] > node_size - 2 * (count + 1):
            raise ValueError('invalid HFS record offsets')
        for start, end in zip(offsets, offsets[1:]):
            yield raw[start:end]
        node = following


def hfs_volume(image):
    """Locate a classic HFS volume by structure, never by filename extension."""
    if image[1024:1026] == b'BD':
        return image
    # Disk Copy 4.2: 84-byte header, then data and optional tag blocks.
    if len(image) >= 84 and image[82:84] == b'\x01\x00' and image[0] <= 63:
        size, tags = struct.unpack_from('>II', image, 64)
        if 84 + size + tags != len(image):
            raise ValueError('truncated or invalid Disk Copy 4.2 image')
        volume = bounded(image, 84, size)
        if volume[1024:1026] == b'BD':
            return volume
    # Apple Partition Map. The driver descriptor supplies the device block size.
    block_size = struct.unpack_from('>H', image, 2)[0] if image[:2] == b'ER' else 512
    if block_size < 512 or block_size > 65536 or block_size & (block_size - 1):
        raise ValueError('invalid partition block size')
    if image[block_size:block_size + 2] == b'PM':
        count = struct.unpack_from('>I', bounded(image, block_size + 4, 4))[0]
        if count < 1 or count > len(image) // block_size - 1:
            raise ValueError('invalid Apple Partition Map length')
        volumes = []
        for index in range(1, count + 1):
            part = bounded(image, index * block_size, 512)
            if part[:2] != b'PM':
                raise ValueError('invalid Apple Partition Map entry')
            if part[48:80].rstrip(b'\0') == b'Apple_HFS':
                start, size = struct.unpack_from('>II', part, 8)
                volume = bounded(image, start * block_size, size * block_size)
                if volume[1024:1026] == b'BD':
                    volumes.append(volume)
        if len(volumes) == 1:
            return volumes[0]
        if len(volumes) > 1:
            raise ValueError('multiple HFS partitions; export the game volume as a raw .hfs image')
    raise ValueError('no classic HFS volume found; use a raw .hfs/.dsk, Disk Copy 4.2 or Apple Partition Map image (not HFS+, compressed or flux images)')


def hfs_files(disk, include=None):
    if (disk.startswith((b'SIT!', b'StuffIt (c)')) or b'(This file must be converted with BinHex' in disk[:1024]
            or disk[32769:32774] == b'CD001'):
        # The same memory-only native decoder is used by the player launcher.
        # Raw HFS/DC42/APM still use the independent stdlib Python reader below.
        root = Path(__file__).resolve().parents[2]
        launcher = root / ('prince.exe' if os.name == 'nt' else 'prince')
        if not launcher.is_file():
            raise ValueError('Build the native launcher first to decode HQX/SIT/ISO inputs (make -C src).')
        with tempfile.TemporaryDirectory(prefix='mac-source-') as temporary:
            source = Path(temporary) / 'input.bin'
            source.write_bytes(disk)
            result = subprocess.run([str(launcher), '--mac-source', str(source)], capture_output=True, timeout=120)
        if result.returncode:
            raise ValueError(result.stderr.decode('utf-8', errors='replace').strip())
        bundle = result.stdout
        if bundle[:8] != b'POPMACS1' or len(bundle) < 20:
            raise ValueError('invalid native source bundle')
        ds, rs, app = struct.unpack_from('<III', bundle, 8)
        if len(bundle) != 20 + ds + rs + app:
            raise ValueError('incomplete native source bundle')
        # Normalised roles, not filesystem CNIDs: archives have no HFS catalog.
        files = [dict(id=0, name='Persia(COLOR)', type='DATA', creator='????', components=['Persia(COLOR)'],
                      data=bundle[20:20+ds], rsrc=bundle[20+ds:20+ds+rs]),
                 dict(id=0, name='Prince of Persia', type='APPL', creator='????', components=['Prince of Persia'],
                      data=b'', rsrc=bundle[20+ds+rs:])]
        return [file for file in files if include is None or include(file)]
    disk = hfs_volume(disk)
    mdb = bounded(disk, 1024, 162)
    if mdb[:2] != b'BD':
        raise ValueError('expected an unwrapped classic HFS image')
    block_size = struct.unpack_from('>I', mdb, 20)[0]
    allocation_start = struct.unpack_from('>H', mdb, 28)[0] * 512
    if block_size < 512 or block_size % 512:
        raise ValueError('invalid HFS allocation block size')

    overflow = {}

    def fork(size, raw_extents, file_id=None, fork_type=0):
        extents = struct.unpack('>6H', raw_extents)
        result = bytearray()
        blocks = 0
        while True:
            for block, count in zip(extents[::2], extents[1::2]):
                if count:
                    result.extend(bounded(disk, allocation_start + block * block_size, count * block_size))
                    blocks += count
            if len(result) >= size:
                break
            extents = overflow.get((fork_type, file_id, blocks))
            if extents is None or not any(extents[1::2]):
                raise ValueError('missing HFS overflow extents; extraction refused')
        return bytes(result[:size])

    overflow_size = struct.unpack_from('>I', mdb, 130)[0]
    if overflow_size:
        extents_tree = fork(overflow_size, mdb[134:146])
        for raw in leaf_records(extents_tree):
            if raw[0] != 7:
                raise ValueError('invalid HFS overflow key')
            fork_type, file_id, first_block = struct.unpack('>BIH', bounded(raw, 1, 7))
            overflow[fork_type, file_id, first_block] = struct.unpack('>6H', bounded(raw, 8, 12))
    catalog = fork(struct.unpack_from('>I', mdb, 146)[0], mdb[150:162], 4)
    directories = {}
    files = []
    for raw in leaf_records(catalog):
        key_size = raw[0]
        key = bounded(raw, 1, key_size)
        if len(key) < 6:
            raise ValueError('truncated HFS catalog key')
        parent = struct.unpack_from('>I', key, 1)[0]
        name = bounded(key, 6, key[5]).decode('mac_roman')
        record = raw[(key_size + 2) & ~1:]
        if not record:
            raise ValueError('empty HFS catalog record')
        if record[0] == 1:
            directories[struct.unpack_from('>I', record, 6)[0]] = (parent, name)
        elif record[0] == 2:
            bounded(record, 0, 102)
            file = {
                'id': struct.unpack_from('>I', record, 20)[0],
                'parent': parent, 'name': name,
                'type': record[4:8].decode('mac_roman'),
                'creator': record[8:12].decode('mac_roman'),
            }
            if include is not None and not include(file):
                continue
            file['data'] = fork(struct.unpack_from('>I', record, 26)[0], record[74:86], file['id'])
            file['rsrc'] = fork(struct.unpack_from('>I', record, 36)[0], record[86:98], file['id'], 255)
            files.append(file)
    for file in files:
        components = [file['name']]
        parent, visited = file['parent'], set()
        while parent != 2:
            if parent in visited or parent not in directories:
                raise ValueError('invalid HFS parent chain')
            visited.add(parent)
            parent, name = directories[parent]
            components.insert(0, name)
        file['components'] = components
    return files


def resource_entries(fork):
    data_start, map_start, data_size, map_size = struct.unpack('>4I', bounded(fork, 0, 16))
    bounded(fork, data_start, data_size)
    resource_map = bounded(fork, map_start, map_size)
    type_start, name_start = struct.unpack('>2H', bounded(resource_map, 24, 4))
    types = resource_map[type_start:]
    type_count = struct.unpack('>H', bounded(types, 0, 2))[0] + 1
    result = {}
    for i in range(type_count):
        kind, count, refs = struct.unpack('>4sHH', bounded(types, 2 + i * 8, 8))
        for j in range(count + 1):
            ref = bounded(types, refs + j * 12, 12)
            rid, name_offset = struct.unpack_from('>hH', ref)
            offset = int.from_bytes(ref[5:8], 'big')
            length = struct.unpack('>I', bounded(fork, data_start + offset, 4))[0]
            if offset + 4 + length > data_size:
                raise ValueError('resource extends beyond resource-data section')
            name = None
            if name_offset != 0xFFFF:
                at = name_start + name_offset
                name_len = bounded(resource_map, at, 1)[0]
                name = bounded(resource_map, at + 1, name_len).decode('mac_roman')
            key = (kind.decode('mac_roman'), rid)
            if key in result:
                raise ValueError(f'duplicate resource: {key}')
            result[key] = {'name': name, 'data': bounded(fork, data_start + offset + 4, length)}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disk', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    disk = args.disk.read_bytes()
    files = hfs_files(disk)
    # Validate the needed application resource before writing any extraction.
    application = next(file for file in files if file['name'] == 'Prince of Persia' and file['type'] == 'APPL')
    resources = resource_entries(application['rsrc'])
    palette_resource = resources['clut', 2000]['data']
    seed, flags, last_index = struct.unpack('>IHH', bounded(palette_resource, 0, 8))
    colors = []
    for i in range(last_index + 1):
        value, r, g, b = struct.unpack('>4H', bounded(palette_resource, 8 + i * 8, 8))
        colors.append({'value': value, 'rgb16': [r, g, b], 'rgb8': [r >> 8, g >> 8, b >> 8]})
    args.output.mkdir(parents=True, exist_ok=True)
    fork_dir = args.output / 'hfs-forks'
    fork_dir.mkdir(exist_ok=True)
    inventory = []
    for file in files:
        safe = '__'.join(file['components']).replace('/', '_').replace('\\', '_').replace('\r', '_')
        record = {key: file[key] for key in ('id', 'type', 'creator', 'components')}
        for kind in ('data', 'rsrc'):
            path = fork_dir / (safe + '.' + kind)
            path.write_bytes(file[kind])
            record[kind] = {'file': str(path.relative_to(args.output)), 'size': len(file[kind]),
                            'sha256': hashlib.sha256(file[kind]).hexdigest()}
        inventory.append(record)
    app_dir = args.output / 'app-resources'
    app_dir.mkdir(exist_ok=True)
    for (kind, rid), item in resources.items():
        if kind in ('CODE', 'clut'):
            (app_dir / f'{kind}_{rid}.bin').write_bytes(item['data'])
    (args.output / 'application-resources.json').write_text(json.dumps([
        {'type': kind, 'id': rid, 'name': item['name'], 'size': len(item['data'])}
        for (kind, rid), item in sorted(resources.items())], indent=2) + '\n')
    (args.output / 'clut-2000.json').write_text(json.dumps({
        'seed': seed, 'flags': flags, 'color_specs': colors,
        'source_resource_sha256': hashlib.sha256(palette_resource).hexdigest()}, indent=2) + '\n')
    (args.output / 'disk-inventory.json').write_text(json.dumps({
        'source_disk': str(args.disk.resolve()), 'sha256': hashlib.sha256(disk).hexdigest(),
        'files': inventory}, indent=2) + '\n')
    print(f"Extracted {len(files)} files with data/resource forks; original disk was opened read-only")
    print(f"Recovered application clut 2000: {len(colors)} entries, flags 0x{flags:04X}")


if __name__ == '__main__':
    main()
