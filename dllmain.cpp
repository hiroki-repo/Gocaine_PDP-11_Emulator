// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"
#include <stdio.h>
#include <malloc.h>

#define __XOR(a,b) ((a || b) && !(a && b))

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

typedef struct {
    union{
        struct{
            UINT16 R0;
            UINT16 R1;
            UINT16 R2;
            UINT16 R3;
            UINT16 R4;
            UINT16 R5;
            union {
                UINT16 R6;
                UINT16 SP;
            };
            union {
                UINT16 R7;
                UINT16 PC;
            };
        };
        UINT16 reg[8];
    };
    union{
        struct {
            struct {
                bool C : 1;
                bool V : 1;
                bool Z : 1;
                bool N : 1;
                bool T : 1;
                UINT8 I : 3;
            }l;
            struct {
                UINT8 reserved : 4;
                UINT8 prevuser : 2;
                UINT8 curuser  : 2;
            }h;
        } statusb;
        UINT16 statusw;
    };
    UINT16 USP;
    UINT16 KSP;
    bool optype;
    UINT32(*memaccess)(UINT32, UINT32, UINT32);
} reg;

UINT16 mmudecode(reg* prm_0, UINT16 prm_1) {
    return prm_1;
}

UINT16 fetchop(reg* prm_0) {
    return ((prm_0->memaccess(mmudecode(prm_0,prm_0->PC++), 0, 1) << (8 * 0))| (prm_0->memaccess(mmudecode(prm_0, prm_0->PC++), 0, 1) << (8 * 1))) & 0xFFFF;
}
UINT8 readbyte(reg* prm_0, UINT16 prm_1) {
    return prm_0->memaccess(mmudecode(prm_0, prm_1), 0, 1);
}
UINT16 readword(reg* prm_0, UINT16 prm_1) {
    return ((prm_0->memaccess(mmudecode(prm_0, prm_1 + 0), 0, 1) << (8 * 0)) | (prm_0->memaccess(mmudecode(prm_0, prm_1 + 1), 0, 1) << (8 * 1)));
}
void writebyte(reg* prm_0, UINT16 prm_1, UINT8 prm_2) {
    prm_0->memaccess(mmudecode(prm_0, prm_1), prm_2, 0);
    return;
}
void writeword(reg* prm_0, UINT16 prm_1, UINT16 prm_2) {
    prm_0->memaccess(mmudecode(prm_0, prm_1 + 0), ((prm_2 >> (8 * 0)) & 0xFF), 0);
    prm_0->memaccess(mmudecode(prm_0, prm_1 + 1), ((prm_2 >> (8 * 1)) & 0xFF), 0);
    return;
}

UINT32 get_addr(reg* prm_0, UINT8 prm_1, UINT8 prm_2, bool optype) {
    UINT32 ret = 0;
    if (prm_2 >= 6) { optype = false; }
    switch (prm_1) {
    case 0:
        ret = 0x80000000 | prm_2;
        break;
    case 1:
        ret = prm_0->reg[prm_2];
        break;
    case 2:
        ret = prm_0->reg[prm_2];
        prm_0->reg[prm_2] += (optype ? 1 : 2);
        break;
    case 3:
        ret = readword(prm_0, prm_0->reg[prm_2]);
        prm_0->reg[prm_2] += 2;
        break;
    case 4:
        prm_0->reg[prm_2] -= (optype ? 1 : 2);
        ret = prm_0->reg[prm_2];
        break;
    case 5:
        prm_0->reg[prm_2] -= 2;
        ret = readword(prm_0, prm_0->reg[prm_2]);
        break;
    case 6:
        ret = prm_0->reg[prm_2] + fetchop(prm_0);
        break;
    case 7:
        ret = readword(prm_0, prm_0->reg[prm_2] + fetchop(prm_0));
        break;
    }
    prm_0->optype = optype;
    return ret;
}
UINT16 read_reg(reg* prm_0,UINT8 prm_1,UINT8 prm_2, bool optype) {
    UINT16 ret = 0;
    if (prm_2 >= 6) { optype = false; }
    UINT32 addr = get_addr(prm_0, prm_1, prm_2, optype);
    if (addr & 0x80000000) {
        ret = prm_0->reg[addr & 7];
    }
    else {
        ret = (optype ? readbyte(prm_0, addr & 0xFFFF) : readword(prm_0, addr & 0xFFFF));
    }
    if ((optype == true) && ((ret&0x80) != 0)) {
        ret |= 0xFF00;
    }
    return ret;
}
void write_reg(reg* prm_0, UINT8 prm_1, UINT8 prm_2, bool optype, UINT16 prm_3) {
    if (prm_2 >= 6) { optype = false; }
    UINT32 addr = get_addr(prm_0, prm_1, prm_2, optype);
    if (addr & 0x80000000) {
        prm_0->reg[addr & 7] = prm_3;
    }
    else {
        if (optype) {
            writebyte(prm_0, addr & 0xFFFF, prm_3);
        }
        else {
            writeword(prm_0, addr & 0xFFFF, prm_3);
        }
    }
    return;
}
UINT16 read_reg_addr(reg* prm_0, UINT32 prm_1) {
    UINT16 reg = 0;
    if (prm_1 & 0x80000000) {
        reg = prm_0->reg[prm_1 & 7];
    }
    else {
        if (prm_0->optype) {
            reg = readbyte(prm_0, prm_1 & 0xFFFF);
        }
        else {
            reg = readword(prm_0, prm_1 & 0xFFFF);
        }
    }
    return reg;
}
void write_reg_addr(reg* prm_0, UINT32 prm_1, UINT16 prm_2) {
    if (prm_1 & 0x80000000) {
        prm_0->reg[prm_1 & 7] = prm_2;
    }
    else {
        if (prm_0->optype) {
            writebyte(prm_0, prm_1 & 0xFFFF, prm_2);
        }
        else {
            writeword(prm_0, prm_1 & 0xFFFF, prm_2);
        }
    }
    return;
}

void setflag(reg* prm_0,bool prm_1,UINT32 prm_2, UINT16 prm_3, UINT16 prm_4, bool prm_5, UINT8 prm_6) {
    UINT16 chkval = 0;
    prm_0->statusw &= (0xFFF0 | (prm_1 ? 0 : 1));
    if (prm_6 != 3) {
        if (prm_2 & (prm_0->optype ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
        if (prm_2 == 0) { prm_0->statusb.l.Z = true; }
    }
    else {
        if (prm_2 & 0x80000000) { prm_0->statusb.l.N = true; }
        if (prm_2 == 0) { prm_0->statusb.l.Z = true; }
    }
    if (prm_1) {
        if (prm_6 != 3) {
            if (((prm_3 ^ prm_4) & (prm_0->optype ? 0x80 : 0x8000)) && ((!((prm_4 ^ prm_2) & (prm_0->optype ? 0x80 : 0x8000))) ? true : false) == prm_5) { prm_0->statusb.l.V = true; }
        }
        switch (prm_6) {
        case 0:
            if (prm_3 < prm_4) { prm_0->statusb.l.C = true; }
            break;
        case 1:
            if (prm_2 & 0x10000) { prm_0->statusb.l.C = true; }
            break;
        case 2:
            if (prm_3 > prm_4) { prm_0->statusb.l.C = true; }
            break;
        case 3:
            if ((prm_2 < (1 << 15)) || (prm_2 >= ((1 << 15) - 1))) { prm_0->statusb.l.C = true; }
            break;
        }
    }
}

void switchmode(reg* prm_0, bool prm_1) {
    prm_0->statusb.h.prevuser = prm_0->statusb.h.curuser;
    prm_0->statusb.h.curuser = (prm_1) ? 3 : 0;
    if (prm_0->statusb.h.prevuser != 0) {
        prm_0->USP = prm_0->SP;
    }
    else {
        prm_0->KSP = prm_0->SP;
    }
    prm_0->SP = (prm_0->statusb.h.curuser != 0) ? prm_0->USP : prm_0->KSP;
    return;
}

extern "C" __declspec(dllexport) reg * create_pdp11() { return (reg*)calloc(1,sizeof(reg)); }
extern "C" __declspec(dllexport) void delete_pdp11(reg * prm_0) { free(prm_0); return; }
extern "C" __declspec(dllexport) void interrupt_pdp11(reg * prm_0, UINT8 prm_1, UINT16 prm_2) {
    if (prm_1 >= (prm_0->statusb.l.I & 7)) {
        UINT16 prev = prm_0->statusw;
        switchmode(prm_0, false);
        write_reg(prm_0, 4, 6, false, prev);
        write_reg(prm_0, 4, 6, false, prm_0->PC);
        prm_0->PC = readword(prm_0, (prm_2 + 0));
        prm_0->statusw = readword(prm_0, (prm_2 + 2));
    }
    return;
}
extern "C" __declspec(dllexport) UINT32 execute_pdp11(reg* prm_0) {
    UINT16 op = fetchop(prm_0);
    UINT32 tmp_val;
    UINT32 addrs[2];
    UINT32 tmpvals[2];
    bool condtmp = false;
    bool byteops = (op & 0x8000) ? true : false;
    if (((op & 0xFFF8) >> 3) == 0x20) {
        prm_0->PC = prm_0->reg[(op >> 0) & 7];
        prm_0->reg[(op >> 0) & 7] = read_reg(prm_0, 2, 6, false);
    }
    else if (((op & 0xFFC0) >> 6) == 3) {
        tmp_val = read_reg(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), false);
        tmp_val = ((tmp_val >> 8) & 0x00FF) | ((tmp_val << 8) & 0xFF00);
        write_reg(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), false, tmp_val);
        prm_0->statusw &= 0xFFF0;
        if (tmp_val & 0x80) { prm_0->statusb.l.N = true; }
        if ((tmp_val & 0xFF) == 0) { prm_0->statusb.l.Z = true; }
    }
    else if (((op & 0xFFC0) >> 6) == 1) {
        addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
        tmp_val = (read_reg_addr(prm_0, addrs[1]));
        prm_0->PC = tmp_val;
    }
    else if ((op >> 11) & 1) {
        if (((op >> 9) & 0x7) == 4) {
            addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
            tmp_val = (read_reg_addr(prm_0, addrs[1]));
            write_reg(prm_0, 4, 6, byteops, prm_0->reg[(op >> 6) & 7]);
            prm_0->reg[(op >> 6) & 7] = prm_0->PC;
            prm_0->PC = tmp_val;
        }
        else {
            switch ((op >> 6) & 0x1f) {
            case 0x8:
                prm_0->statusw &= 0xFFF0;
                prm_0->statusb.l.Z = true;
                write_reg(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops, 0);
                break;
            case 0x9:
                prm_0->statusw &= 0xFFF0;
                prm_0->statusb.l.C = true;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = read_reg_addr(prm_0, addrs[1]) ^ (byteops ? 0xFF : 0xFFFF);
                write_reg_addr(prm_0, addrs[1], tmp_val);
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                break;
            case 0xa:
                prm_0->statusw &= 0xFFF1;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]) + 1) & (byteops ? 0xFF : 0xFFFF);
                write_reg_addr(prm_0, addrs[1], tmp_val);
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; prm_0->statusb.l.V = true; }
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                break;
            case 0xb:
                prm_0->statusw &= 0xFFF1;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]) - 1) & (byteops ? 0xFF : 0xFFFF);
                write_reg_addr(prm_0, addrs[1], tmp_val);
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (tmp_val & (byteops ? 0x7f : 0x7fff)) { prm_0->statusb.l.V = true; }
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                break;
            case 0xc:
                prm_0->statusw &= 0xFFF0;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (~read_reg_addr(prm_0, addrs[1])) & (byteops ? 0xFF : 0xFFFF);
                write_reg_addr(prm_0, addrs[1], tmp_val);
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (tmp_val & (byteops ? 0x7f : 0x7fff)) { prm_0->statusb.l.V = true; }
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                else { prm_0->statusb.l.C = true; }
                break;
            case 0xd:
                prm_0->statusw &= 0xFFF0;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1])) & (byteops ? 0xFF : 0xFFFF);
                if (prm_0->statusb.l.C) {
                    tmp_val += 1;
                    write_reg_addr(prm_0, addrs[1], tmp_val);
                    if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                    if (tmp_val == 0x7FFF) { prm_0->statusb.l.V = true; }
                    if (tmp_val == (byteops ? 0xff : 0xffff)) { prm_0->statusb.l.Z = true; }
                    if (tmp_val == 0xffff) { prm_0->statusb.l.C = true; }
                }
                else {
                    if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                    if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                }
                break;
            case 0xe:
                prm_0->statusw &= 0xFFF0;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1])) & (byteops ? 0xFF : 0xFFFF);
                if (prm_0->statusb.l.C) {
                    tmp_val -= 1;
                    write_reg_addr(prm_0, addrs[1], tmp_val);
                    if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                    if (tmp_val == 0x8000) { prm_0->statusb.l.V = true; }
                    if (tmp_val == 1) { prm_0->statusb.l.Z = true; }
                    if (tmp_val) { prm_0->statusb.l.C = true; }
                }
                else {
                    if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                    if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                    if (tmp_val == 0x8000) { prm_0->statusb.l.V = true; }
                    prm_0->statusb.l.C = true;
                }
                break;
            case 0xf:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]));
                prm_0->statusw &= 0xFFF0;
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                break;
            case 0x10:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]));
                if (prm_0->statusb.l.C) { tmp_val |= ((byteops ? 0xFF : 0xFFFF) + 1); }
                prm_0->statusw &= 0xFFF0;
                if (tmp_val & 1) { prm_0->statusb.l.C = true; }
                if (tmp_val & ((byteops ? 0xFF : 0xFFFF) + 1)) { prm_0->statusb.l.N = true; }
                if (!(tmp_val & (byteops ? 0xFF : 0xFFFF))) { prm_0->statusb.l.Z = true; }
                if (__XOR((tmp_val & 1), (tmp_val & ((byteops ? 0xFF : 0xFFFF) + 1)))) { prm_0->statusb.l.V = true; }
                tmp_val >>= 1;
                write_reg_addr(prm_0, addrs[1], tmp_val);
                break;
            case 0x11:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = ((read_reg_addr(prm_0, addrs[1])) << 1);
                if (prm_0->statusb.l.C) { tmp_val |= 1; }
                prm_0->statusw &= 0xFFF0;
                if (tmp_val & ((byteops ? 0xFF : 0xFFFF) + 1)) { prm_0->statusb.l.C = true; }
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (!(tmp_val & (byteops ? 0xFF : 0xFFFF))) { prm_0->statusb.l.Z = true; }
                if ((tmp_val ^ (tmp_val >> 1)) & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.V = true; }
                tmp_val &= (byteops ? 0xFF : 0xFFFF);
                write_reg_addr(prm_0, addrs[1], tmp_val);
                break;
            case 0x12:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]));
                if (prm_0->statusb.l.C) { tmp_val |= ((byteops ? 0xFF : 0xFFFF) + 1); }
                prm_0->statusw &= 0xFFF0;
                if (tmp_val & 1) { prm_0->statusb.l.C = true; }
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.N = true; }
                if (__XOR((tmp_val & (byteops ? 0x80 : 0x8000)), (tmp_val & 1))) { prm_0->statusb.l.V = true; }
                tmp_val = (tmp_val & (byteops ? 0x80 : 0x8000)) | (tmp_val >> 1);
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                write_reg_addr(prm_0, addrs[1], tmp_val);
                break;
            case 0x13:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmp_val = (read_reg_addr(prm_0, addrs[1]));
                if (prm_0->statusb.l.C) { tmp_val |= 1; }
                prm_0->statusw &= 0xFFF0;
                if (tmp_val & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.C = true; }
                if (tmp_val & ((byteops ? 0x80 : 0x8000) >> 1)) { prm_0->statusb.l.N = true; }
                if ((tmp_val ^ (tmp_val << 1)) & (byteops ? 0x80 : 0x8000)) { prm_0->statusb.l.V = true; }
                tmp_val = (((read_reg_addr(prm_0, addrs[1])) << 1) & (byteops ? 0xFF : 0xFFFF));
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                write_reg_addr(prm_0, addrs[1], tmp_val);
                break;
            case 0x14:
                prm_0->SP = prm_0->PC + (op & 0x3f) << 1;
                prm_0->PC = prm_0->R5;
                prm_0->R5 = read_reg(prm_0, 2, 6, byteops);
                break;
            case 0x15:
                addrs[0] = get_addr(prm_0, 2, ((op >> 0) & 7), false);
                if (addrs[0] == 0x80000006) {
                    tmp_val = (prm_0->statusb.h.prevuser == prm_0->statusb.h.curuser) ? prm_0->SP : (prm_0->statusb.h.prevuser ? prm_0->USP : prm_0->KSP);
                }
                break;
            case 0x17:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                if (prm_0->statusb.l.N) {
                    write_reg_addr(prm_0, addrs[1], (byteops ? 0xFF : 0xFFFF));
                }
                else {
                    prm_0->statusb.l.Z = true;
                    write_reg_addr(prm_0, addrs[1], 0);
                }
                break;
            }
        }
    }
    else {
        switch ((op >> 12) & 0x7) {
        case 0:
            if (((op >> 9) & 0x7) <= 3) {
                switch ((((op >> 15) << 3) | (op >> 9)) & 0xF) {
                case 0:
                    condtmp = true;
                    break;
                case 1:
                    condtmp = prm_0->statusb.l.Z;
                    break;
                case 2:
                    condtmp = (__XOR(prm_0->statusb.l.N, prm_0->statusb.l.V) ? true : false);
                    break;
                case 3:
                    condtmp = ((__XOR(prm_0->statusb.l.N, prm_0->statusb.l.V) || prm_0->statusb.l.Z) ? true : false);
                    break;
                case 4:
                    condtmp = prm_0->statusb.l.N;
                    break;
                case 5:
                    condtmp = ((prm_0->statusb.l.C || prm_0->statusb.l.Z) ? true : false);
                    break;
                case 6:
                    condtmp = prm_0->statusb.l.V;
                    break;
                case 7:
                    condtmp = prm_0->statusb.l.C;
                    break;
                }
                if (condtmp == (((op >> 8) & 1) ? true : false)) {
                    tmp_val = op & 0xFF;
                    if (tmp_val & 0x80) { tmp_val = -(((tmp_val)+1) & 0xFF); }
                    tmp_val <<= 1;
                    prm_0->PC += tmp_val;
                }
            }
            break;
        case 1:
            tmp_val = read_reg(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
            write_reg(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops, tmp_val);
            setflag(prm_0, false, tmp_val, 0, 0, true, 0);
            break;
        case 2:
            addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
            addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
            tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
            tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
            tmp_val = tmpvals[0] - tmpvals[1];
            setflag(prm_0, true, tmp_val, tmpvals[0], tmpvals[1], true, 0);
            break;
        case 3:
            addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
            addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
            tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
            tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
            tmp_val = tmpvals[0] & tmpvals[1];
            setflag(prm_0, false, tmp_val, tmpvals[0], tmpvals[1], true, 0);
            break;
        case 4:
            addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
            addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
            tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
            tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
            tmp_val = ((byteops ? 0xFF : 0xFFFF) ^ tmpvals[0]) & tmpvals[1];
            write_reg_addr(prm_0, addrs[1], tmp_val);
            setflag(prm_0, false, tmp_val, tmpvals[0], tmpvals[1], true, 0);
            break;
        case 5:
            addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
            addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
            tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
            tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
            tmp_val = tmpvals[0] | tmpvals[1];
            write_reg_addr(prm_0, addrs[1], tmp_val);
            setflag(prm_0, false, tmp_val, tmpvals[0], tmpvals[1], true, 0);
            break;
        case 6:
            if (byteops) {
                byteops = false;
                addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                tmp_val = tmpvals[0] - tmpvals[1];
                write_reg_addr(prm_0, addrs[1], tmp_val);
                setflag(prm_0, true, tmp_val, tmpvals[0], tmpvals[1], true, 2);
            }
            else {
                byteops = false;
                addrs[0] = get_addr(prm_0, ((op >> 9) & 7), ((op >> 6) & 7), byteops);
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 0) & 7), byteops);
                tmpvals[0] = read_reg_addr(prm_0, addrs[0]);
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                tmp_val = tmpvals[0] + tmpvals[1];
                write_reg_addr(prm_0, addrs[1], tmp_val);
                setflag(prm_0, true, tmp_val, tmpvals[0], tmpvals[1], false, 1);
            }
            break;
        case 7:
            switch ((op >> 9) & 7) {
            case 0:
                byteops = false;
                addrs[0] = ((op >> 0) & 7) | 0x80000000;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 6) & 7), byteops);
                tmpvals[0] = read_reg_addr(prm_0, addrs[0]); if (tmpvals[0] & 0x8000) { tmpvals[0] -= ((0xFFFF ^ tmpvals[0]) + 1); }
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]); if (tmpvals[1] & 0x8000) { tmpvals[1] -= ((0xFFFF ^ tmpvals[1]) + 1); }
                tmp_val = tmpvals[0] * tmpvals[1];
                prm_0->reg[((op >> 0) & 7)] = (tmp_val >> 16) & 0xFFFF;
                prm_0->reg[((op >> 0) & 7) | 1] = (tmp_val) & 0xFFFF;
                setflag(prm_0, true, tmp_val, tmpvals[0], tmpvals[1], false, 3);
                break;
            case 1:
                byteops = false;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 6) & 7), byteops);
                tmpvals[0] = (prm_0->reg[((op >> 0) & 7)] << 16) | prm_0->reg[((op >> 0) & 7) | 1];
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                prm_0->statusw &= 0xFFF0;
                if (tmpvals[1] == 0) { tmp_val = 0; prm_0->statusb.l.C = true; }
                else {
                    tmp_val = tmpvals[0] / tmpvals[1];
                    if (tmp_val >= 0x10000) {
                        prm_0->statusb.l.V = true;
                    }
                    else {
                        if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                        prm_0->reg[((op >> 0) & 7)] = tmp_val;
                        prm_0->reg[((op >> 0) & 7) | 1] = tmpvals[0] % tmpvals[1];
                        if (prm_0->reg[((op >> 0) & 7)] == 0) { prm_0->statusb.l.Z = true; }
                        if (prm_0->reg[((op >> 0) & 7)] & 0x8000) { prm_0->statusb.l.N = true; }
                    }
                }
                break;
            case 2:
                byteops = false;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 6) & 7), byteops);
                tmpvals[0] = prm_0->reg[((op >> 0) & 7)];
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                prm_0->statusw &= 0xFFF0;
                if (tmpvals[1] & 0x20) {
                    tmpvals[1] = (0x3F ^ tmpvals[1]) + 1;
                    if (tmpvals[0] & 0x8000) {
                        tmp_val = 0xffff ^ (0xffff >> tmpvals[1]);
                        tmp_val |= tmpvals[0] >> tmpvals[1];
                    }
                    else {
                        tmp_val = tmpvals[0] >> tmpvals[1];
                    }
                    if (tmpvals[0] & (1 << (tmpvals[1] - 1))) { prm_0->statusb.l.C = true; }
                }
                else {
                    tmp_val = (tmpvals[0] << tmpvals[1]) & 0xFFFF;
                    if (tmpvals[0] & (1 << (16 - tmpvals[1]))) { prm_0->statusb.l.C = true; }
                }
                prm_0->reg[((op >> 0) & 7)] = tmp_val;
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                if (tmp_val & 0x8000) { prm_0->statusb.l.N = true; }
                if (__XOR((tmp_val & 0x8000), (tmpvals[0] & 0x8000))) { prm_0->statusb.l.V = true; }
                break;
            case 3:
                byteops = false;
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 6) & 7), byteops);
                tmpvals[0] = (prm_0->reg[((op >> 0) & 7)] << 16) | prm_0->reg[((op >> 0) & 7) | 1];
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                prm_0->statusw &= 0xFFF0;
                if (tmpvals[1] & 0x20) {
                    tmpvals[1] = (0x3F ^ tmpvals[1]) + 1;
                    if (tmpvals[0] & 0x80000000) {
                        tmp_val = 0xffffffff ^ (0xffffffff >> tmpvals[1]);
                        tmp_val |= tmpvals[0] >> tmpvals[1];
                    }
                    else {
                        tmp_val = tmpvals[0] >> tmpvals[1];
                    }
                    if (tmpvals[0] & (1 << (tmpvals[1] - 1))) { prm_0->statusb.l.C = true; }
                }
                else {
                    tmp_val = (tmpvals[0] << tmpvals[1]) & 0xFFFFFFFF;
                    if (tmpvals[0] & (1 << (32 - tmpvals[1]))) { prm_0->statusb.l.C = true; }
                }
                prm_0->reg[((op >> 0) & 7)] = (tmp_val >> 16) & 0xFFFF;
                prm_0->reg[((op >> 0) & 7) | 1] = (tmp_val) & 0xFFFF;
                if (tmp_val == 0) { prm_0->statusb.l.Z = true; }
                if (tmp_val & 0x80000000) { prm_0->statusb.l.N = true; }
                if (__XOR((tmp_val & 0x80000000), (tmpvals[0] & 0x80000000))) { prm_0->statusb.l.V = true; }
                break;
            case 4:
                addrs[1] = get_addr(prm_0, ((op >> 3) & 7), ((op >> 6) & 7), byteops);
                tmpvals[0] = (prm_0->reg[((op >> 0) & 7)] << 16) | prm_0->reg[((op >> 0) & 7) | 1];
                tmpvals[1] = read_reg_addr(prm_0, addrs[1]);
                tmp_val = tmpvals[0] ^ tmpvals[1];
                prm_0->reg[((op >> 0) & 7)] = (tmp_val) & 0xFFFF;
                setflag(prm_0, false, tmp_val, tmpvals[0], tmpvals[1], true, 0);
                break;
            case 7:
                if (--prm_0->reg[((op >> 6) & 7)]) {
                    tmp_val = op & 0xFF;
                    tmp_val &= 0x3F;
                    tmp_val <<= 1;
                    prm_0->PC -= tmp_val;
                }
                break;
            }
            break;
        }
    }
    return 1;
}
