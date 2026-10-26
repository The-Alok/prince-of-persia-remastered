#!/usr/bin/env python3
"""Asset-free format/safety tests; optionally check a privately supplied game disk.

python3 tools/test_mac_disk_import.py --disk /path/to/PoP1.dsk
"""
import argparse
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from build_mac_graphics_pack import graphics_from_disk, main, pack_bytes, write_pack
from extract_mac_reference import hfs_volume


def dc42(volume):
    header = bytearray(84)
    header[0] = 4
    header[1:5] = b'Test'
    struct.pack_into('>II', header, 64, len(volume), 0)
    header[82:84] = b'\x01\x00'
    return bytes(header) + volume


def partitioned(volume):
    image = bytearray(4 * 512 + len(volume))
    image[:4] = b'ER\x02\x00'
    image[512:514] = b'PM'
    struct.pack_into('>III', image, 516, 1, 4, len(volume) // 512)
    image[560:569] = b'Apple_HFS'
    image[2048:] = volume
    return bytes(image)


class FormatTests(unittest.TestCase):
    def setUp(self):
        self.volume = bytearray(4096)
        self.volume[1024:1026] = b'BD'
        self.volume = bytes(self.volume)

    def test_raw_hfs(self):
        self.assertEqual(hfs_volume(self.volume), self.volume)

    def test_disk_copy(self):
        self.assertEqual(hfs_volume(dc42(self.volume)), self.volume)

    def test_partition_map(self):
        self.assertEqual(hfs_volume(partitioned(self.volume)), self.volume)

    def test_truncated_disk_copy(self):
        with self.assertRaisesRegex(ValueError, 'truncated'):
            hfs_volume(dc42(self.volume)[:-1])

    def test_truncated_partition(self):
        with self.assertRaisesRegex(ValueError, 'out-of-bounds'):
            hfs_volume(partitioned(self.volume)[:-1])

    def test_wrong_format(self):
        for data in (b'', b'not a game disk', b'\0' * 4096):
            with self.assertRaisesRegex(ValueError, 'no classic HFS'):
                hfs_volume(data)

    def test_missing_resources(self):
        with self.assertRaises((ValueError, struct.error)):
            graphics_from_disk(self.volume)

    def test_no_silent_overwrite(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'test.pak'
            write_pack(path, b'first')
            write_pack(path, b'first')
            with self.assertRaisesRegex(ValueError, 'already exists'):
                write_pack(path, b'second')
            self.assertEqual(path.read_bytes(), b'first')
            write_pack(path, b'second', force=True)
            self.assertEqual(path.read_bytes(), b'second')
            self.assertEqual(list(Path(folder).iterdir()), [path])

    def test_cli_bad_input_creates_no_output(self):
        with tempfile.TemporaryDirectory() as folder:
            source, output = Path(folder) / 'bad.dsk', Path(folder) / 'generated/test.pak'
            source.write_bytes(b'not a game disk')
            self.assertEqual(main([str(source), '--output', str(output)]), 1)
            self.assertFalse(output.exists())
            self.assertFalse(output.parent.exists())
            self.assertEqual(source.read_bytes(), b'not a game disk')

    def test_cli_cannot_overwrite_its_disk(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / 'game.dsk'
            source.write_bytes(b'not a game disk')
            self.assertEqual(main([str(source), '--output', str(source), '--force']), 1)
            self.assertEqual(source.read_bytes(), b'not a game disk')


def integration(disk):
    image = disk.read_bytes()
    before = hashlib.sha256(image).hexdigest()
    reference = pack_bytes(graphics_from_disk(image)[0])
    with tempfile.TemporaryDirectory() as folder:
        for name, content in (('PoP1.hfs', image), ('PoP1.dsk', image),
                              ('DiskCopy.dsk', dc42(image)),
                              ('Partitioned.hfs', partitioned(image))):
            path = Path(folder) / name
            path.write_bytes(content)
            self_pack = pack_bytes(graphics_from_disk(path.read_bytes())[0])
            assert self_pack == reference, name
            assert path.read_bytes() == content, name
            print(f'PASS: {name} -> identical 820-image PAK')
    assert hashlib.sha256(disk.read_bytes()).hexdigest() == before
    print(f'PASS: source disk unchanged; PAK SHA-256 {hashlib.sha256(reference).hexdigest()}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disk', type=Path)
    args = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(FormatTests))
    if not result.wasSuccessful():
        raise SystemExit(1)
    if args.disk:
        integration(args.disk)
