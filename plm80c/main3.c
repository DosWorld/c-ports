/****************************************************************************
 *  main3.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";
word putWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

word getWord(pointer buf) {
    return buf[0] + buf[1] * 256;
}

static void Sub_3F3C() {
    b7199 = PRINT | OBJECT;

    vfReset(&utf2);
    //   vfReset(&utf1);
    vfRewind(&atf);
    if (OBJECT)
        fseek(objFile.fp, 0, SEEK_END);
    w7197 = csegSize;
}

static void Sub_3FE2() {
    word p;
    for (p = 1; p <= procCnt; p++) {
        infoIdx = procInfo[p];
        if (!TestInfoFlag(F_EXTERNAL)) {
            SetLinkVal(w7197);
            w7197 = w7197 + GetDimension2();
        }
    }
}

static void Sub_402F() {

    for (infoIdx = AdvNxtInfo(0); infoIdx != 0; infoIdx = AdvNxtInfo(infoIdx)) {
        if (GetType() == LABEL_T) {
            if (!TestInfoFlag(F_LABEL))
                Sub_49BC(0xAC, infoIdx, 0);
            else if (!TestInfoFlag(F_EXTERNAL))
                infotab[infoIdx].linkVal += infotab[procInfo[High(GetScope())]].linkVal;
        }
    }
}

static void Sub_40B6() {
    word p;

    for (p = 1; p <= localLabelCnt; p++) {
        infoIdx = procInfo[procIds[p]];
        localLabels[p] += GetLinkVal();
    }
}

static void Sub_4105() {
    byte i, j, k;
    bool m;
    word p;

    if (!standAlone)
        return;
    Wr1Byte(0xa4);
    Wr1Word(procInfo[1]);

    infoIdx = procInfo[1];
    p       = w7197 - GetLinkVal();
    Wr1Word(p);
    for (i = 0; i <= 45; i++) {
        k = b42D6[i];
        j = k + b42A8[i];
        m = false;
        while (k < j) {
            if (m) {
                helpers[k] = w7197;
                w7197 += b4813[k];
            } else if (helpers[k] != 0) {
                m          = true;
                helpers[k] = w7197;
                w7197 += b4813[k];
            }
            k = k + 1;
        }
    }
}

static void Sub_426E() {
    infoIdx = procInfo[1];
    curSym  = GetSymbol();
    if (curSym == 0)
        RecAddByte(rec2, 0, 0);
    else
        RecAddName(rec2, 0, symtab[curSym].name->len, symtab[curSym].name->str);
    RecAddByte(rec2, 0, 1);
    RecAddByte(rec2, 0, (version[1] << 4) | (version[3] & 0xf));
    RecAddByte(rec2, 0, 1);
    RecAddWord(rec2, 0, csegSize);
    RecAddByte(rec2, 0, 3);
    RecAddByte(rec2, 0, 2);
    RecAddWord(rec2, 0, dsegSize);
    RecAddByte(rec2, 0, 3);
    RecAddByte(rec2, 0, 3);
    RecAddWord(rec2, 0, Sub_4938());
    RecAddByte(rec2, 0, 3);
    RecAddByte(rec2, 0, 4);
    RecAddWord(rec2, 0, 0);
    RecAddByte(rec2, 0, 3);
    WriteRec(rec2, 0);
}

static void Sub_436C() {
    pointer rec;
    word curRecLen, r, s;
    byte seg, k;

    s = 0;
    for (infoIdx = AdvNxtInfo(0); infoIdx != 0; infoIdx = AdvNxtInfo(infoIdx)) {
        curSym = GetSymbol();
        if (LABEL_T <= GetType() && GetType() <= PROC_T && curSym != 0) {
            if (TestInfoFlag(F_EXTERNAL) && !TestInfoFlag(F_AT)) {
                if (getWord(&rec18[REC_LEN]) + symtab[curSym].name->len + 2 >= 299)
                    WriteRec(rec18, 0);
                s++;
                RecAddName(rec18, 0, symtab[curSym].name->len, symtab[curSym].name->str);
                RecAddByte(rec18, 0, 0);
            } else if (!(TestInfoFlag(F_AUTOMATIC) || TestInfoFlag(F_BASED) ||
                         TestInfoFlag(F_MEMBER))) {
                if (TestInfoFlag(F_DATA) || GetType() == LABEL_T || GetType() == PROC_T) {
                    rec       = rec16_2;
                    curRecLen = getWord(&rec16_2[REC_LEN]);
                    seg       = 1;
                } else if (TestInfoFlag(F_MEMORY)) {
                    rec       = rec16_4;
                    curRecLen = getWord(&rec16_4[REC_LEN]);
                    seg       = 4;
                } else if (TestInfoFlag(F_ABSOLUTE)) {
                    rec       = rec16_1;
                    curRecLen = getWord(&rec16_1[REC_LEN]);
                    seg       = 0;
                } else {
                    rec       = rec16_3;
                    curRecLen = getWord(&rec16_3[REC_LEN]);
                    seg       = 2;
                }

                if (TestInfoFlag(F_PUBLIC)) {
                    if (curRecLen + symtab[curSym].name->len + 4 >= 299)
                        WriteRec(rec, 1);
                    RecAddWord(rec, 1, GetLinkVal());
                    RecAddName(rec, 1, symtab[curSym].name->len, symtab[curSym].name->str);
                    RecAddByte(rec, 1, 0);
                }
                bool isExtern;
                if (DEBUG) {
                    if (infoIdx == procInfo[1])
                        isExtern = true;
                    else if (!TestInfoFlag(F_PARAMETER))
                        isExtern = TestInfoFlag(F_EXTERNAL);
                    else {
                        r        = infoIdx;
                        infoIdx  = procInfo[High(GetScope())];
                        isExtern = TestInfoFlag(F_EXTERNAL);
                        infoIdx  = r;
                    }
                    if (!isExtern) {
                        if (seg != rec12[REC_DATA] ||
                            getWord(&rec12[REC_LEN]) + symtab[curSym].name->len + 4 >= 1019)
                            WriteRec(rec12, 1);
                        rec12[REC_DATA] = seg;
                        RecAddWord(rec12, 1, GetLinkVal());
                        RecAddName(rec12, 1, symtab[curSym].name->len, symtab[curSym].name->str);
                        RecAddByte(rec12, 1, 0);
                    }
                }
            }
        }
    } /* of while */

    if (!standAlone) {
        for (k = 0; k < 117; k++) {
            if (helpers[k]) {
                helpers[k] = s++;
                if (getWord(&rec18[REC_LEN]) + 8 >= 299)
                    WriteRec(rec18, 0);
                char t[7];
                sprintf(t, "@P%04d", k);
                RecAddName(rec18, 0, 6, t);
                RecAddByte(rec18, 0, 0);
            }
        }
    }
    WriteRec(rec18, 0);
    WriteRec(rec16_1, 1);
    WriteRec(rec16_2, 1);
    WriteRec(rec16_3, 1);
    WriteRec(rec16_4, 1);
    WriteRec(rec12, 1);
}

static void Sub_46B7() {
    word p;

    if (intVecNum == 0)
        return;
    for (p = 1; p <= procCnt; p++) {
        infoIdx = procInfo[p];
        if (TestInfoFlag(F_INTERRUPT)) {
            putWord(&rec6[CONTENT_OFF], intVecLoc + intVecNum * GetIntrNo());
            RecAddByte(rec6, 3, 0xC3);
            RecAddWord(rec6, 3, GetLinkVal());
            WriteRec(rec6, 3);
            RecAddWord(rec24_1, 2, getWord(&rec6[CONTENT_OFF]) + 1);
            WriteRec(rec24_1, 2);
        }
    }
}

static void Sub_4746() {
    if (b7199 || IXREF) {
        Wr1Byte(0x9c);
        vfRewind(&utf1);
    }
    vfReset(&atf);
}

word Start3() {
    dump(&atf, "atf_main3");
    vfRewind(&atf);

    Sub_3F3C();
    if (b7199 || IXREF)
        Sub_3FE2();
    Sub_402F();
    if (b7199 || IXREF) {
        Sub_40B6();
        Sub_4105();
        csegSize = w7197;
    }
    if (OBJECT) {
        Sub_426E();
        Sub_436C();
        Sub_46B7();
    }
    Sub_49F9();
    Sub_4746();
    if (b7199)
        return 4; // Chain(overlay[4]);
    else {
        vfReset(&utf1);
        if (IXREF)
            return 5; // Chain(overlay[5]);
        else {
            EndCompile();
            Exit(programErrCnt != 0);
        }
    }
}
