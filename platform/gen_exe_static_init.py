#!/usr/bin/env python3
"""Generate platform/exe_static_init.cpp from SpideyPC.exe.

The exe runs 1677 C++ static initializers before WinMain (the .CRT$XCU
pointer table at 0x546004..0x547A34: one per global object with a
constructor, mostly CVector/CSVector/CQuat/matrix globals). The standalone
build runs none of the exe's code, so every global those initializers fill
stayed zero (the bss part of the mapped data block). All but four of them
are straight-line stores of constants, so this script emulates them and
emits the resulting bytes as a table that ExeMem_Init copies into the block
right after seeding it from the exe image. The four that call constructors
(DCSkaterModel x3, Font x1, DCCard_Exists x2) are done by hand in
exemem.cpp.

usage: gen_exe_static_init.py path/to/SpideyPC.exe > platform/exe_static_init.cpp
needs: pefile, iced-x86 (the ~/Documents/spidey-work/venv has both)
"""
import struct, sys
import pefile
from iced_x86 import Decoder, Mnemonic, OpKind, Register, MemorySizeExt

TABLE_LO, TABLE_HI = 0x546004, 0x547A34
EXEMEM_START, EXEMEM_END = 0x53B000, 0x2E0C000

pe = pefile.PE(sys.argv[1])
base = pe.OPTIONAL_HEADER.ImageBase
def section(name):
    return [s for s in pe.sections if s.Name.startswith(name)][0]
text, data = section(b'.text'), section(b'.data')
tb, tva = text.get_data(), base + text.VirtualAddress
db, dva = data.get_data(), base + data.VirtualAddress
rdata = section(b'.rdata'); rb, rva = rdata.get_data(), base + rdata.VirtualAddress

def read_image(addr, size):
    for blob, va in ((db, dva), (rb, rva)):
        if va <= addr and addr + size <= va + len(blob):
            return blob[addr - va:addr - va + size]
    return None

funcs = [struct.unpack_from('<I', db, o - dva)[0] for o in range(TABLE_LO, TABLE_HI + 4, 4)]
mem = {}          # addr -> byte value, in exe run order
manual = []

REG32 = {Register.EAX: 'eax', Register.ECX: 'ecx', Register.EDX: 'edx', Register.EBX: 'ebx',
         Register.ESI: 'esi', Register.EDI: 'edi', Register.EBP: 'ebp', Register.ESP: 'esp'}
SUB = {Register.AX: ('eax', 2), Register.CX: ('ecx', 2), Register.DX: ('edx', 2), Register.BX: ('ebx', 2),
       Register.SI: ('esi', 2), Register.DI: ('edi', 2),
       Register.AL: ('eax', 1), Register.CL: ('ecx', 1), Register.DL: ('edx', 1), Register.BL: ('ebx', 1)}

def reg_get(regs, r):
    if r in REG32: return regs.get(REG32[r])
    if r in SUB:
        full, size = SUB[r]; v = regs.get(full)
        return None if v is None else v & ((1 << (8 * size)) - 1)
    return None

def reg_set(regs, r, v):
    if r in REG32: regs[REG32[r]] = v & 0xFFFFFFFF
    elif r in SUB:
        full, size = SUB[r]; mask = (1 << (8 * size)) - 1
        old = regs.get(full) or 0
        regs[full] = (old & ~mask) | (v & mask)

def mem_read(addr, size, writes):
    out = bytearray()
    for i in range(size):
        a = addr + i
        if a in writes: out.append(writes[a])
        elif a in mem: out.append(mem[a])
        else:
            b = read_image(a, 1)
            if b is None: return None
            out.append(b[0])
    return int.from_bytes(out, 'little')

def emulate(fa):
    insns = {}
    order = []
    off = fa - tva
    for ins in Decoder(32, tb[off:off + 4096], ip=fa):
        insns[ins.ip] = ins; order.append(ins.ip)
        if ins.mnemonic == Mnemonic.RET or len(order) > 2000: break
    regs = {}; writes = {}; zf = None
    ip = fa; steps = 0
    while steps < 100000:
        steps += 1
        ins = insns.get(ip)
        if ins is None: raise RuntimeError('ran off at %#x in %#x' % (ip, fa))
        nxt = ins.next_ip
        m = ins.mnemonic
        def mem_addr(k):
            if ins.op_kind(k) != OpKind.MEMORY: return None
            a = ins.memory_displacement
            if ins.memory_base != Register.NONE:
                b = reg_get(regs, ins.memory_base)
                if b is None: raise RuntimeError('unknown base reg in %#x' % fa)
                a = (a + b) & 0xFFFFFFFF
            if ins.memory_index != Register.NONE: raise RuntimeError('index reg in %#x' % fa)
            return a
        def operand(k):
            ok = ins.op_kind(k)
            if ok == OpKind.REGISTER: return reg_get(regs, ins.op_register(k))
            if ok == OpKind.MEMORY: return mem_read(mem_addr(k), MemorySizeExt.size(ins.memory_size), writes)
            return ins.immediate(k)
        if m == Mnemonic.RET: return writes
        if m == Mnemonic.CALL: return None
        if m == Mnemonic.MOV:
            if ins.op0_kind == OpKind.MEMORY:
                v = operand(1); size = MemorySizeExt.size(ins.memory_size)
                if v is None: raise RuntimeError('store of unknown value in %#x' % fa)
                a = mem_addr(0)
                for i in range(size): writes[a + i] = (v >> (8 * i)) & 0xFF
            else:
                v = operand(1)
                if v is None: raise RuntimeError('load of unknown value in %#x' % fa)
                reg_set(regs, ins.op0_register, v)
        elif m == Mnemonic.XOR and ins.op0_kind == OpKind.REGISTER and ins.op1_kind == OpKind.REGISTER and ins.op0_register == ins.op1_register:
            reg_set(regs, ins.op0_register, 0); zf = True
        elif m == Mnemonic.ADD:
            v = reg_get(regs, ins.op0_register); w = operand(1)
            reg_set(regs, ins.op0_register, v + w); zf = ((v + w) & 0xFFFFFFFF) == 0
        elif m == Mnemonic.SUB:
            v = reg_get(regs, ins.op0_register); w = operand(1)
            reg_set(regs, ins.op0_register, v - w); zf = ((v - w) & 0xFFFFFFFF) == 0
        elif m == Mnemonic.DEC:
            v = reg_get(regs, ins.op0_register) - 1; reg_set(regs, ins.op0_register, v); zf = (v & 0xFFFFFFFF) == 0
        elif m == Mnemonic.INC:
            v = reg_get(regs, ins.op0_register) + 1; reg_set(regs, ins.op0_register, v); zf = (v & 0xFFFFFFFF) == 0
        elif m == Mnemonic.JNE:
            if zf is None: raise RuntimeError('jne without flags in %#x' % fa)
            if not zf: nxt = ins.near_branch32
        elif m == Mnemonic.JE:
            if zf: nxt = ins.near_branch32
        elif m == Mnemonic.JMP:
            nxt = ins.near_branch32
        elif m in (Mnemonic.PUSH, Mnemonic.POP):
            pass
        elif m == Mnemonic.STOSD:
            # rep stosd: ecx dwords of eax at edi
            count = regs['ecx']; a = regs['edi']; v = regs['eax']
            for n in range(count):
                for i in range(4): writes[a + 4 * n + i] = (v >> (8 * i)) & 0xFF
            regs['edi'] = a + 4 * count; regs['ecx'] = 0
        else:
            raise RuntimeError('unhandled %s in %#x' % (m, fa))
        ip = nxt
    raise RuntimeError('too many steps in %#x' % fa)

for fa in funcs:
    w = emulate(fa)
    if w is None:
        manual.append(fa); continue
    for a, b in w.items():
        if EXEMEM_START <= a < EXEMEM_END:
            mem[a] = b

# stores that only rewrite what the exe image already holds are no-ops after
# seeding; drop them to keep the table small
for a in list(mem):
    img = read_image(a, 1)
    if img is not None and img[0] == mem[a]:
        del mem[a]

addrs = sorted(mem)
runs = []
for a in addrs:
    if runs and a == runs[-1][0] + len(runs[-1][1]):
        runs[-1][1].append(mem[a])
    else:
        runs.append([a, bytearray([mem[a]])])

out = sys.stdout
out.write('// GENERATED by platform/gen_exe_static_init.py from SpideyPC.exe. Do not edit.\n')
out.write('// The bytes the exe\'s %d static initializers (.CRT$XCU table %#x..%#x)\n' % (len(funcs), TABLE_LO, TABLE_HI))
out.write('// store into the data block before WinMain, replayed by ExeMem_Init for\n')
out.write('// the standalone build. %d initializers call constructors and are done by\n' % len(manual))
out.write('// hand in exemem.cpp: %s.\n' % ', '.join('%#x' % f for f in manual))
out.write('#include "exemem.h"\n#include <string.h>\n\n')
out.write('struct SExeInitRun { u32 addr; u32 size; const u8* data; };\n\n')
for i, (a, bs) in enumerate(runs):
    out.write('static const u8 kRun%d[%d] = {%s};\n' % (i, len(bs), ','.join('%d' % b for b in bs)))
out.write('\nstatic const SExeInitRun kRuns[%d] = {\n' % len(runs))
for i, (a, bs) in enumerate(runs):
    out.write('\t{%#x, %d, kRun%d},\n' % (a, len(bs), i))
out.write('};\n\nvoid ExeMem_ApplyStaticInits(void)\n{\n')
out.write('\tfor (u32 i = 0; i < sizeof(kRuns) / sizeof(kRuns[0]); i++)\n')
out.write('\t\tmemcpy(reinterpret_cast<void*>(kRuns[i].addr), kRuns[i].data, kRuns[i].size);\n}\n')
sys.stderr.write('%d initializers, %d manual, %d bytes in %d runs\n' % (len(funcs), len(manual), len(mem), len(runs)))
