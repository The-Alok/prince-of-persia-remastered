#!/usr/bin/env python3
"""Disassemble the extracted original CODE resources using system Capstone 5.

Uses its public C API via ctypes; no Python package or game execution needed.
Offsets in output are resource-relative, including the four-byte segment header.
API reference: github.com/capstone-engine/capstone/blob/5.0.1/include/capstone/capstone.h
"""
import ctypes as c
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class Instruction(c.Structure):
    _fields_ = [('id', c.c_uint), ('address', c.c_uint64), ('size', c.c_uint16),
                ('bytes', c.c_uint8 * 24), ('mnemonic', c.c_char * 32),
                ('op_str', c.c_char * 160), ('detail', c.c_void_p)]


def main():
    lib = c.CDLL('libcapstone.so.5')
    lib.cs_open.argtypes = [c.c_int, c.c_uint, c.POINTER(c.c_size_t)]
    lib.cs_option.argtypes = [c.c_size_t, c.c_int, c.c_size_t]
    lib.cs_disasm.argtypes = [c.c_size_t, c.c_char_p, c.c_size_t, c.c_uint64, c.c_size_t, c.POINTER(c.POINTER(Instruction))]
    lib.cs_disasm.restype = c.c_size_t
    lib.cs_free.argtypes = [c.POINTER(Instruction), c.c_size_t]
    lib.cs_close.argtypes = [c.POINTER(c.c_size_t)]
    handle = c.c_size_t()
    if lib.cs_open(8, 0x80000002, c.byref(handle)):
        raise RuntimeError('Capstone M68K initialization failed')
    lib.cs_option(handle, 5, 3)  # Skip inline data/unknown A-line trap words.
    try:
        for path in sorted((ROOT / 'tools/mac_reference/app-resources').glob('CODE_*.bin')):
            if path.stem == 'CODE_0':
                continue  # Jump table, not an executable segment.
            code = path.read_bytes()[4:]
            instructions = c.POINTER(Instruction)()
            count = lib.cs_disasm(handle, code, len(code), 4, 0, c.byref(instructions))
            try:
                lines = []
                for i in range(count):
                    insn = instructions[i]
                    lines.append(f'{insn.address:06x} {bytes(insn.bytes[:insn.size]).hex():<24} '
                                 f'{insn.mnemonic.decode()} {insn.op_str.decode()}')
                path.with_suffix('.asm').write_text('\n'.join(lines) + '\n')
            finally:
                lib.cs_free(instructions, count)
            print(f'{path.stem}: {count} decoded instructions/data words')
    finally:
        lib.cs_close(c.byref(handle))


if __name__ == '__main__':
    main()
