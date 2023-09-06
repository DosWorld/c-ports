/****************************************************************************
 *  plm2f.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte bC2A5, bC2A6, bC2A7;
static word wC2A9, first, last;

static bool Sub_8861() {
    for (wC1D6 = first; wC1D6 <= last; wC1D6++) {
        byte i = Sub_5679(bC2A5);
        if ((0 <= i && i <= 3) || (12 <= i && i <= 14))
            return true;
    }
    return false;
} /* Sub_8861() */

static void Sub_88C1() {
    if (Sub_8861()) {
        for (int i = 0; i < 4; i++) {
            if (bC04E[i] == bC2A6) {
                if (bC045[i] == 0 || bC045[i] == 1 || bC045[i] == 6) {
                    bC0B3[bC2A5] = bC045[i];
                    bC0B5[bC2A5] = i;
                    if (bC0B5[1 - bC2A5] != i)
                        return;
                }
            }
        }
    }
} /* Sub_88C1() */

static void Sub_894A() {
    if (bC0B5[bC2A5] > 3) {
        for (int i = 1; i < 4; i++) {
            if (bC04E[i] == bC2A6 && (bC045[i] == 2 || bC045[i] == 3)) {
                bC0B3[bC2A5] = bC045[i];
                bC0B5[bC2A5] = i;
                if (bC0B5[1 - bC2A5] != i)
                    return;
            }
        }
    }
} /* Sub_894A() */

static void Sub_89D1() {
    byte i;
    word p;

    if (bC0B5[bC2A5] == 0xA)
        wC2A9 = tx2[bC2A6].op2;
    else if (bC0B5[bC2A5] == 9) {
        wC2A9 = tx2[bC2A6].op3;
        if ((!boC069[0] && boC072[0]) || bC0B1 > 0 || wC2A9 != wC1C3) {
            i = bC0B1 + bC0B2;
            for (p = wC2A9; p <= wC1C3; p++) {
                if (bC140[p] != 0)
                    i++;
            }
            if (i < 4)
                boC1D8 = true;
            else
                bC0B5[bC2A5] = 0xA;
        }
        wC2A9 = -(wC2A9 * 2);
    }
} /* Sub_89D1() */

static void Sub_8A9C() {
    word p, q;
    byte ii, j;
    word r;

    if (bC0B5[bC2A5] == 0xA) {
        p  = wC2A9;
        q  = 0x100;
        ii = 4;
        j  = Sub_5748(bC0B3[bC2A5]);
    } else if (bC0B5[bC2A5] == 8 && bC0B3[bC2A5] == 1) {
        GetVal(bC0B7[bC2A5], &p, &q);
        ii = 2;
        j  = 1;
    } else if (bC0B5[bC2A5] == 4 && (bC0B3[bC2A5] == 0 || bC0B3[bC2A5] == 8 || !Sub_8861())) {
        GetVal(bC0B7[bC2A5], &p, &q);
        ii = 2;
        j  = Sub_5748(bC0B3[bC2A5]);
    } else
        return;

    for (int i = 1; i < 4; i++) {
        if (boC069[i]) {
            if (bC0B7[0] == bC0B7[1] && curOp != T2_COLONEQUALS)
                if (bC0B5[bC2A5] > 3)
                    bC0B5[bC2A5] = i;
        } else if (!boC072[i] && wC096[i] == q && boC057[i] && 1 <= bC045[i] && bC045[i] <= 6) {
            r = wC084[i] + bC0A8[i] - p;
            if (r > 0xff)
                r = -r;
            if (r < ii) {
                bC0B5[bC2A5] = i;
                ii           = (byte)r;
            }
        }
    }
    if (bC0B5[bC2A5] <= 3) {
        int i    = bC0B5[bC2A5];
        bC045[i] = bC0B3[bC2A5] = j;
        bC04E[i]                = bC0B7[bC2A5];
        bC0A8[i]                = wC084[i] + bC0A8[i] - p;
        wC084[i]                = p;
    }
} /* Sub_8A9C() */

static void Sub_8CF5() {
    bC0B5[0] = 8;
    bC0B5[1] = 8;
    for (bC2A5 = 0; bC2A5 <= 1; bC2A5++) {
        if ((bC2A6 = bC0B7[bC2A5]) == 0)
            bC0B3[bC2A5] = 0xC;
        else if ((bC2A7 = tx2[bC2A6].opc) == T2_STACKPTR)
            bC0B3[bC2A5] = 0xA;
        else if (bC2A7 == T2_LOCALLABEL)
            bC0B3[bC2A5] = 9;
        else {
            bC0B3[bC2A5] = tx2[bC2A6].aux1;
            bC0B5[bC2A5] = tx2[bC2A6].aux2;
            Sub_88C1();
            Sub_894A(); /*  checked */
        }
    }
    for (bC2A5 = 0; bC2A5 <= 1; bC2A5++) {
        bC2A6 = bC0B7[bC2A5];
        Sub_597E();
        Sub_89D1();
        Sub_8A9C();
        Sub_61A9(bC2A5);
    }
} /* Sub_8CF5() */

static byte Sub_8E7E(byte arg1b) {

    if (bC0B7[arg1b] == 0 || bC0B7[arg1b] == 1)
        return 1;
    return b4D23[bC0C1[arg1b]][Sub_5679(arg1b)];
} /* Sub_8E7E() */

static void Sub_8ECD(byte arg1b, byte arg2b) {
    bC0B9[arg1b] = b4C45[arg2b];
    bC0BB[arg1b] = b4CB4[arg2b];
    bC0BD[arg1b] = b4FA3[arg2b];
} /* Sub_8ECD() */

static void Sub_8DCD() {
    byte h, i, j, k, m, n;
    h = i = 0; // set to avoid compiler warning

    j     = 198;
    for (wC1D6 = first; wC1D6 <= last; wC1D6++) {
        k = Sub_8E7E(0);
        m = Sub_8E7E(1);
        n = b4C45[k] + b4C45[m] + (b43F8[b4A21[wC1D6]] & 0x1f);
        if (n < j) {
            j        = n;
            h        = k;
            i        = m;
            cfrag1   = b4A21[wC1D6];
            bC1D9    = b46EB[wC1D6];
            bC0BF[0] = Sub_5679(0);
            bC0BF[1] = Sub_5679(1);
        }
    }
    Sub_8ECD(0, h);
    Sub_8ECD(1, i);
} /* Sub_8DCD() */

static void Sub_8F16() {
    if (bC0B7[0] != 0)
        Sub_63AC(bC0B5[0]);

    if (bC0B7[1] != 0)
        Sub_63AC(bC0B5[1]);
} /* Sub_8F16() */

static void Sub_8F35() {
    word p;

    if (curOp == T2_STKARG || curOp == T2_STKBARG || curOp == T2_STKWARG) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        wB53C[procCallDepth]++;
        wC1C3++;
    } else if (curOp == T2_CALL) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        SetInfo(tx2[tx2qp].op3);
        if ((info->flag & F_EXTERNAL))
            p = (wB53C[procCallDepth] + 1) * 2;
        else
            p = (wB528[procCallDepth] + 1) * 2 + info->stackUsage;
        if (p > stackUsage)
            stackUsage = p;
    } else if (curOp == T2_CALLVAR) {
        Sub_5795(-(wB53C[procCallDepth] * 2));
        if (stackUsage < wC1C3 * 2)
            stackUsage = wC1C3 * 2;
    } else if (curOp == T2_RETURN || curOp == T2_RETURNBYTE || curOp == T2_RETURNWORD) {
        boC1CD = true;
        Sub_5EE8();
    } else if (curOp == T2_JMPFALSE) {
        Sub_5795(0);
        if (boC20F) {
            cfrag1 = CF_JMPTRUE;
            boC20F = false;
        }
    } else if (curOp == T2_63)
        Sub_5795(0);
    else if (curOp == T2_MOVE) {
        if (wB53C[procCallDepth] != wC1C3) {
            Sub_5795(-((wB53C[procCallDepth] + 1) * 2));
            Sub_6416(3);
        }
        if (bC045[3] == 1)
            cfrag1 = CF_MOVE_HL;
    }
} /* Sub_8F35() */

static void Sub_940D() {
    for (int i = 0; i < 4; i++) {
        if (bC04E[i] == bC0B7[0])
            if (bC045[i] < 2 || 5 < bC045[i])
                bC04E[i] = 0;
    }
}

static void Sub_90EB() {
    word p, q;
    byte j, k;

    p = w48DF[bC1D9] * 16;
    q = w493D[bC1D9];
    k = 0;
    if (curOp == T2_COLONEQUALS) {
        Sub_940D();
        if (tx2[bC0B7[1]].auxw == 0)
            if (tx2[bC0B7[0]].auxw > 0) {
                if (cfrag1 == CF_MOVMLR || cfrag1 == CF_STA) {
                    bC045[bC0B5[1]] = 0;
                    bC04E[bC0B5[1]] = bC0B7[0];
                } else if (cfrag1 == CF_SHLD || cfrag1 == CF_MOVMRP) {
                    bC045[bC0B5[1]] = 1;
                    bC04E[bC0B5[1]] = bC0B7[0];
                }
            }
    } else if (T2_51 <= curOp && curOp <= T2_56)
        Sub_940D();
    for (int n = 5; n < 9; n++) {
        byte i = p >> 13;
        j      = q >> 12;
        p <<= 3;
        q <<= 4;
        if (j <= 3) {
            Sub_5B96(j, n);
            if (i == 1)
                bC0A8[n]++;
            else if (i == 2) {
                if (bC045[n] == 0) {
                    bC045[n] = 6;
                } else {
                    bC045[n]  = 1;
                    boC057[n] = 0;
                }
            }
        } else if (j == 4) {
            boC057[k = n] = 0;
            if (0 < tx2[tx2qp].auxw) {
                bC04E[n] = tx2qp;
                bC045[n] = tx2[tx2qp].aux1 = b43F8[cfrag1] >> 5;
                bC0A8[n]                   = 0;
            } else
                bC04E[n] = 0;
        } else if (j == 5) {
            bC04E[n]  = 0;
            wC096[n]  = 0;
            bC0A8[n]  = 0;
            boC057[n] = 0xFF;
            bC045[n]  = 0;
            wC084[n]  = i;
        } else {
            bC04E[n]  = 0;
            boC057[n] = 0;
        }
    }
    if (k == 0 && tx2[tx2qp].auxw > 0) {
        for (int n = 5; n < 9; n++) {
            if (bC04E[n] == 0)
                if (!boC057[k = n])
                    break;
        }
        if (k != 0) {
            bC04E[k]        = tx2qp;
            boC057[k]       = 0;
            bC045[k]        = 0;
            tx2[tx2qp].aux1 = 0;
            bC0A8[k]        = 0;
        }
    }
    for (int n = 0; n <= 3; n++)
        Sub_5B96(n + 5, n);
} /* Sub_90EB() */

void Sub_87CB() {
    bC0B7[0] = (byte)tx2[tx2qp].op1;
    bC0B7[1] = (byte)tx2[tx2qp].op2;
    first    = wAF54[curOp];
    last     = first + b499B[curOp] - 1;
    Sub_8CF5();

    while (1) {
        Sub_8DCD(); /*  OK */
        if (bC0B9[0] == 0 && bC0B9[1] == 0)
            break;
        if (boC1D8)
            Sub_7A85();
        else
            Sub_7DA9();
    }
    Sub_8F16();
    Sub_611A();
    Sub_5E66(w48DF[bC1D9] >> 12);
    Sub_8F35();
    Sub_84ED();
    Sub_90EB();
} /* Sub_87CB() */

void Sub_9457() {
    if (EnterBlk()) {
        blk[blkId].codeSize     = codeSize;
        blk[blkId].wB4B0     = wC1C3;
        blk[blkId].stackSize     = stackUsage;
        blk[blkId].extProcId = curExtProcId;
        blk[blkSP].next   = blkId;
        blkId            = blkSP;
        SetInfo(blk[blkSP].info = tx2[tx2qp].op1);
        curExtProcId = info->procId;
        codeSize           = 0;
        EmitTopItem();
        Sub_981C();
    }
}
