"""Read-only verification of the ERA/SoD operands used by the extender."""
import argparse
import struct
from pathlib import Path

def pe_reader(file):
    data = Path(file).read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Not a PE image")
    sections, optional_size = struct.unpack_from("<H12xH", data, pe + 6)
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("Expected a 32-bit game executable")
    base = struct.unpack_from("<I", data, optional + 28)[0]
    start = optional + optional_size
    records = [struct.unpack_from("<8sIIII", data, start + i * 40) for i in range(sections)]
    def read(va, size):
        rva = va - base
        for _, virtual_size, address, raw_size, offset in records:
            if address <= rva and rva + size <= address + raw_size:
                return data[offset + rva - address:offset + rva - address + size]
        raise ValueError(f"Unmapped virtual address {va:#x}")
    return read

def verify(file):
    read = pe_reader(file)
    checks = {
        # Native loader/setup/event/bonus operands.
        0x44CB31: bytes.fromhex("BB"),
        0x44CD1C: bytes.fromhex("81 C7"),
        0x4E2D26: struct.pack("<I", 0x63E758),
        0x4E2D3D: struct.pack("<I", 0x63E998),
        0x49DD90: struct.pack("<I", 144 * 4),
        0x49DD9A: struct.pack("<I", 0x696A78),
        0x49F51E: struct.pack("<I", 0x696A7C),
        0x4338DD: struct.pack("<I", 0x692E68),
        0x433959: struct.pack("<I", 0x693770),
        0x43410F: struct.pack("<I", 0x63AC7C),
        # Active LOD lists and table operands used for safe icon existence checks.
        0x55CA34: struct.pack("<I", 0x69E574),
        0x55CA3B: struct.pack("<I", 0x69E570),
        0x55CA56: struct.pack("<I", 0x69D8AC),
        0x55AB98: struct.pack("<I", 0x69E57C),
        0x55AB9F: struct.pack("<I", 0x69E578),
        0x55ABBA: struct.pack("<I", 0x69D8AC),
        # Live register argument and three stack arguments in the AI-value ABI.
        0x4336D1: bytes.fromhex("89 55 F8"),
        0x4336DE: bytes.fromhex("C2 0C 00"),
        # WoG text records and reset-copy bounds.
        0x717020: struct.pack("<I", 171),
        0x7324BD: struct.pack("<I", 171),
        0x732849: struct.pack("<I", 171),
        0x752D83: struct.pack("<I", 171 * 32),
        0x752DC0: struct.pack("<I", 171 * 32),
        0x752DF1: struct.pack("<I", 171),
        0x752D99: struct.pack("<I", 0x2731070),
        0x732795: struct.pack("<I", 0x2731080),
    }
    for operand in (0x75122F, 0x751C97, 0x751DE1, 0x752DFD, 0x732675, 0x7326A2):
        checks[operand] = struct.pack("<I", 0xA4A588)
    for operand in (0x7512C3, 0x751D2B, 0x751E4C, 0x752E0B, 0x73270D, 0x73273A):
        checks[operand] = struct.pack("<I", 0xA4A58C)
    for address, expected in checks.items():
        actual = read(address, len(expected))
        assert actual == expected, f"{address:#x}: {actual.hex()} != {expected.hex()}"
    # Independently walk the original AI stream and its property arities.
    words = struct.unpack("<637i", read(0x63AC7C, 2548))
    widths = (1,1,1,1,1,1,1,1,1,1,1,2,1,1,0,0,2,2,1,1,0,0,0,0)
    pos, records = 0, 0
    while words[pos] >= 0:
        artifact = words[pos]
        assert 0 <= artifact < 144
        pos += 1
        while words[pos] >= 0:
            kind = words[pos]
            assert kind < len(widths)
            pos += 1 + widths[kind]
        assert words[pos] == -1
        pos += 1
        records += 1
    assert words[pos] == -100 and pos == 636
    print(f"PASS: {len(checks)} operands; {records} native AI records; terminator at DWORD {pos}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    verify(args.executable)
