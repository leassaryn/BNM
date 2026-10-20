#pragma once

#include <cstddef>
#include <cstdint>

namespace BNM::Internal {
// Conservative decoder for loader stubs. Unknown/VEX/EVEX instructions fail closed;
// never resume at the next byte after a decoding failure. Not a general disassembler.
struct X86Instruction {
    size_t length{};
    size_t displacement{};
    bool call{};
    bool stop{};
};
inline X86Instruction DecodeX86(const uint8_t *bytes, size_t available, bool x64) {
    if (!bytes) return {};
    const size_t limit = available < 15 ? available : 15;
    size_t pos = 0;
    bool operand16 = false, addressOverride = false, rexW = false;
    while (pos < limit) {
        const auto b = bytes[pos];
        if (b == 0x66) operand16 = true;
        else if (b == 0x67) addressOverride = true;
        else if (b == 0xf0 || b == 0xf2 || b == 0xf3 || b == 0x2e || b == 0x36 ||
                 b == 0x3e || b == 0x26 || b == 0x64 || b == 0x65) {}
        else if (x64 && b >= 0x40 && b <= 0x4f) { rexW = (b & 8) != 0; ++pos; continue; }
        else break;
        rexW = false; // A legacy prefix after REX cancels it.
        ++pos;
    }
    if (pos == limit) return {};
    const auto op = bytes[pos++];
    const size_t operand = operand16 && !rexW ? 2 : 4;
    size_t immediate = 0;
    bool modrm = false, stop = false, call = false;
    size_t displacement = 0;
    if (op == 0xe8) {
        // rel16 is a different instruction contract; do not guess its target.
        if (operand16 && !x64) return {};
        displacement = pos; immediate = 4; call = true;
    } else if (op == 0xc3 || op == 0xcb || op == 0xcc || op == 0xf4) stop = true;
    else if (op == 0xc2 || op == 0xca) { immediate = 2; stop = true; }
    else if (op == 0xe9 || op == 0xeb) { immediate = op == 0xeb ? 1 : operand; stop = true; }
    else if ((op >= 0x70 && op <= 0x7f) || (op >= 0xe0 && op <= 0xe3)) { immediate = 1; stop = true; }
    else if (op >= 0xb0 && op <= 0xb7) immediate = 1;
    else if (op >= 0xb8 && op <= 0xbf) immediate = x64 && rexW ? 8 : operand;
    else if (op == 0x68) immediate = operand16 ? 2 : 4;
    else if (op == 0x6a || op == 0xa8) immediate = 1;
    else if (op == 0xa9) immediate = operand;
    else if (op >= 0xa0 && op <= 0xa3) immediate = x64 ? (addressOverride ? 4 : 8) : (addressOverride ? 2 : 4);
    else if ((op >= 0x50 && op <= 0x5f) || (!x64 && op >= 0x40 && op <= 0x4f) ||
             op == 0x90 || op == 0x98 || op == 0x99 || op == 0x9c || op == 0x9d ||
             op == 0xc9 || op == 0xfc || op == 0xfd) {}
    else if (op <= 0x3d && (op & 7) <= 5) {
        // ADD/OR/ADC/SBB/AND/SUB/XOR/CMP families.
        const auto form = op & 7;
        modrm = form <= 3;
        if (form == 4) immediate = 1;
        if (form == 5) immediate = operand;
    } else if (op == 0x0f) {
        if (pos == limit) return {};
        const auto second = bytes[pos++];
        if (second >= 0x80 && second <= 0x8f) { immediate = x64 ? 4 : operand; stop = true; }
        else if (second == 0x0b) stop = true;
        else if (second == 0x1e || second == 0x1f || (second >= 0x40 && second <= 0x4f) ||
                 (second >= 0x90 && second <= 0x9f) || second == 0xaf || second == 0xb6 ||
                 second == 0xb7 || second == 0xbe || second == 0xbf ||
                 (second >= 0x10 && second <= 0x17) || second == 0x28 || second == 0x29 ||
                 second == 0x6f || second == 0x7f || second == 0xef) modrm = true;
        else return {};
    } else {
        switch (op) {
            case 0x63: if (!x64) return {}; modrm = true; break;
            case 0x69: modrm = true; immediate = operand; break;
            case 0x6b: case 0x80: case 0x83: case 0xc0: case 0xc1: case 0xc6:
                modrm = true; immediate = 1; break;
            case 0x81: case 0xc7: modrm = true; immediate = operand; break;
            case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
            case 0x8a: case 0x8b: case 0x8d: case 0x8f: case 0xd0: case 0xd1:
            case 0xd2: case 0xd3: case 0xf6: case 0xf7: case 0xfe: case 0xff:
                modrm = true; break;
            default: return {};
        }
    }
    if (modrm) {
        if (pos == limit) return {};
        const auto mr = bytes[pos++];
        const auto mod = mr >> 6, reg = (mr >> 3) & 7, rm = mr & 7;
        if ((op == 0xc6 || op == 0xc7 || op == 0x8f) && reg != 0) return {};
        if (op == 0xfe && reg > 1) return {};
        if (op == 0xff) {
            if (reg == 7) return {};
            if (reg >= 2 && reg <= 5) stop = true; // Indirect calls/jumps cannot be resolved here.
        }
        if ((op == 0xf6 || op == 0xf7) && reg <= 1) immediate = op == 0xf6 ? 1 : operand;
        if (mod != 3) {
            if (!x64 && addressOverride) {
                if (mod == 0 && rm == 6) pos += 2;
                else if (mod == 1) ++pos;
                else if (mod == 2) pos += 2;
            } else {
                if (rm == 4) {
                    if (pos >= limit) return {};
                    const auto sib = bytes[pos++];
                    if (mod == 0 && (sib & 7) == 5) pos += 4;
                } else if (mod == 0 && rm == 5) pos += 4;
                if (mod == 1) ++pos;
                else if (mod == 2) pos += 4;
            }
        }
    }
    if (pos > limit || immediate > limit - pos) return {};
    return {pos + immediate, displacement, call, stop};
}
}
