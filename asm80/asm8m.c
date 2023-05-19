/****************************************************************************
 *  asm8m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

static byte b7183[] = {0x3F, 0, 4, 0, 0, 0, 8, 0, 0x10};
    /* bit vector 64 - 00000000 00000100 00000000 00000000 00000000 00001000 00000000 00010000 */
    /*                 CR, COMMA, SEMI */                                  

static word baseMacroBlk;
byte savedTokenIdx;


static bool IsEndParam(void) {
    if (IsCR()) {		// new line forces end
        inAngleBrackets = false;
        return true;
    }

    if (inAngleBrackets)	// return whether end of balanced list
        return argNestCnt == startNestCnt;

    if (IsLT() || (curMacro.mtype != M_IRP && IsGT())) {
        IllegalCharError();
        return true;
    }

    return IsWhite() || IsComma() || IsGT() || curChar == ';';
}



static void InitParamCollect(void) {
    symTab[TID_MACRO] = endSymTab[TID_MACRO] = endSymTab[TID_SYMBOL];	// init macro parameter table
    resetTmpStrings();
    paramCnt = curMacro.localsCnt = 0;	// no params yet
    yyType = O_MACROPARAM;
}


static void Sub720A(void) {
    savedMtype = curMacro.mtype;
    if (!expandingMacro)
        expandingMacro = 1;

    if (macroDepth == 0)
        expandingMacro = 0xFF;

    if (nestMacro) {
        macro[macroDepth].bufP = macro[0].bufP;
        macro[macroDepth].blk = macro[0].blk;
    }

    nestMacro = false;
    curMacro.pCurArg = pNextArg;
    curMacro.localIdBase = localIdCnt;
    localIdCnt += curMacro.localsCnt;		
    ReadM(curMacro.savedBlk);
    curMacro.bufP = macroBuf;
}


static bool Sub727F(void) {
    if (! (mSpoolMode & 1))
        return true;
    macroSpoolNestDepth++;
    b6B2C = topOp != K_REPT;
    yyType = O_MACROPARAM;
    return false;
}


void DoIrpX(byte mtype)
{    /* 1 -> IRP, 2 -> IRPC */
    if (Sub727F()) {
        InitParamCollect();
        Nest(1);
        curMacro.cnt = 0;
        curMacro.mtype = mtype;
    }
}

static void Acc1ToDecimal(void) {
    char buf[6];

    PushToken(O_PARAM);
    sprintf(buf, "%u", accum1);
    for (char *s = buf; *s; s++)
        CollectByte(*s);
}


void initMacroParam(void) {
    pNextArg = baseMacroTbl;
    yyType = O_ITERPARAM;
    inMacroBody = true;
    b9060 = false;
}


static pointer AddMacroText(pointer lowAddr, pointer highAddr)
{
    while (lowAddr <= highAddr) {
        if (baseMacroTbl <= macroParams)
            RuntimeError(RTE_TABLE);    /* table Error() */
        *baseMacroTbl-- = *highAddr--;
    }
    return baseMacroTbl;
}


static void InitSpoolMode(void) {
    macroSpoolNestDepth = 1;
    macroInPtr = macroText;
    mSpoolMode = 1;
    baseMacroBlk = macroBlkCnt;
}



static void ChkForLocationError(void) {
    if (usrLookupIsID && asmErrCode != 'U')
        LocationError();
}


void GetMacroToken(void) {
    byte isPercent;

    savedTokenIdx = tokenIdx;
    SkipWhite();
    if (! (isPercent = curChar == '%')) {
        startNestCnt = argNestCnt - 1;		// if nested than argNestCnt will already have been incremented for the <
        if ((inAngleBrackets = IsLT()))
            curChar = GetCh();

        PushToken(O_PARAM);

        while (! IsEndParam()) {
            if (curChar == '\'') {
                if ((curChar = GetCh()) == '\'') {
                    curChar = GetCh();
                    SkipWhite();
                    if (IsEndParam())
                        break;
                    else {
                        CollectByte('\'');
                        CollectByte('\'');
                    }
                } else {
                    CollectByte('\'');
                    continue;
                }
            }
            CollectByte(curChar);
            if (curMacro.mtype == M_IRPC)
                curMacro.cnt++;
            /* optimisation may swap evaluation order so force */
            bool test = curChar == '!';
            if (test & (GetCh() != CR)) {
                CollectByte(curChar);
                curChar = GetCh();
            }
        }

        if (inAngleBrackets)
            curChar = GetCh();

        SkipWhite();
        if (IsGT()) {
            curChar = GetCh();
            SkipWhite();
        }

        reget = 1;
    }

    inMacroBody = false;
    if (curMacro.mtype == M_INVOKE)
    {
        if ((! inAngleBrackets) && token[0].size == 5)
            if (StrUcEqu("MACRO", (char *)tokPtr)) {	// nested macro definition
                yyType = K_MACRO;
                PopToken();
                pNextArg = curMacro.pCurArg;
                opSP--;
#ifdef TRACE
                printf("nested macro poped token & opStack\n");
#endif
                reget = 1;
                EmitXref(XREF_DEF, name);
                haveUserSymbol = false;
                nestedMacroSeen = true;
                return;
            }
        curMacro.mtype = savedMtype;
        Nest(1);
        curMacro.mtype = M_MACRO;
    }

    if (!isPercent && !TestBit(curChar, b7183)) {    /* ! CR, COMMA or SEMI */
        Skip2EOL();
        SyntaxError();
        reget = 1;
    }
}



void DoMacro(void) {
    if (Sub727F()) {
        expectingOperands = false;
        pMacro = topSymbol;
        UpdateSymbolEntry(0, T_MACRONAME);
        curMacro.mtype = M_MACRO;
        InitParamCollect();
    }
}

void DoMacroBody(void) {
    if (HaveTokens()) {
        if (token[0].type == 0)		// saved parameters have type 0
            MultipleDefError();		// so if found we have a multiple definition

        InsertMacroSym(++paramCnt, 0);	// add the parameter setting type to 0
    }
    else if (curMacro.mtype != M_MACRO)	// only MACRO is allowed to have no parameters
        SyntaxError();

    if (curMacro.mtype != M_MACRO) {	// none MACRO have parameters -> dummy,value
        SkipWhite();					// as dummy is entered above look for , value
        if (IsComma()) {
            reget = 0;
            curOp = T_VALUE;		   // mark beginning of expression
            initMacroParam();
            if (curMacro.mtype == M_IRP) {		/* if IRP then expression begins with < */
                curChar = GetCh();
                SkipWhite();
                if (! IsLT()) {
                    SyntaxError();		/* report error but assume < was given */
                    reget = 1;			/* make sure the non < is read again */
                }
            }
        }
        else
        {
            SyntaxError();				/* report error and assume param list complete */
            InitSpoolMode();			/* spool rest of definition */
        }
    }
    else if (curOp == T_CR)		// got the parameters
    {
        if (! MPorNoErrCode())	// skip if multiple defined, phase or no error 
        {
            curMacro.mtype = M_BODY;
            if (pMacro && (pMacro->type & 0x7F) == T_MACRONAME)
                pMacro->type = asmErrCode == 'L' ? (O_NAME + 0x80) : O_NAME;	// location error illegal forward ref
        }
        InitSpoolMode();
    }
}

void DoEndm(void) {
    if (mSpoolMode & 1) {
        if (--macroSpoolNestDepth == 0) {	// Reached end of spooling ?
            mSpoolMode = 0;		// spooling done
            if (! (curMacro.mtype == M_BODY)) {
                if (curMacro.mtype == M_IRPC)
                    pNextArg = baseMacroTbl + 3;

                /* endm cannot have a label */
                for (byte *p = startMacroLine; p <= startMacroToken - 1; p++) {	// plm reuses aVar
                    curChar = *p;
                    if (! IsWhite())
                        SyntaxError();
                }

                /* replace line with the ESC char to mark end */
                macroInPtr = startMacroLine;
                *macroInPtr = ESC;
                FlushM();		// write to disk
                WriteM(macroText);
                endSymTab[TID_MACRO] = symTab[TID_MACRO];	// reset macro parameter symbol table
                resetTmpStrings();
                if (curMacro.mtype == M_MACRO) {
                    pMacro->blk = baseMacroBlk;
                    pMacro->nlocals = curMacro.localsCnt;		// number of locals
                } else {
                    curMacro.savedBlk = baseMacroBlk;
                    Sub720A();
                    if (curMacro.cnt == 0)
                        UnNest(1);
                }
            }
        }
    }
    else
        NestingError();
}


void DoExitm(void) {
    if (expandingMacro) {
        if (curOp == T_CR) {
            condAsmSeen = true;
            macroCondSP = curMacro.condSP;
            ifDepth = curMacro.ifDepth;
            curMacro.cnt = 1;		// force apparent endm - last repitition
            lookAhead = ESC;		// and endm marker
            macroCondStk[0] = 1;	// and any if else
        } else
            SyntaxError();
    } else
        NestingError();
}


void DoIterParam(void) {
    if (savedTokenIdx + 1 != tokenIdx)
        SyntaxError();
    else if (! b9060) {
        if (token[0].type != O_PARAM) {
            accum1 = GetNumVal();
            Acc1ToDecimal();
        }

        if (curMacro.mtype == M_IRPC)
            curMacro.cnt = token[0].size == 0 ? 1 : token[0].size;	// plm uses true->FF and byte arithmetic. Replaced here for clarity

        CollectByte((token[0].size + 1) | 0x80);						// append a byte to record the token length + 0x80
        baseMacroTbl = AddMacroText(tokPtr, tokPtr + token[0].size - 1);
        PopToken();

        if (curMacro.mtype == M_MACRO || (curMacro.mtype == M_IRP && argNestCnt > 0))
            inMacroBody = true;
        else
            b9060 = true;

        if (curMacro.mtype == M_IRP)
            curMacro.cnt++;
    }
    else
        SyntaxError();

    if (curOp == T_CR) {
        inMacroBody = false;
        if (argNestCnt > 0)
            BalanceError();

        if (! MPorNoErrCode()) {
            ChkForLocationError();
            if (curMacro.mtype == M_MACRO) {
                Sub720A();
                UnNest(1);
                return;
            } else
                curMacro.cnt = 0;
        } else {
            baseMacroTbl = AddMacroText(b3782, b3782 + 1);		// append 0x80 0x81
            if (curMacro.mtype == M_MACRO) {
                curMacro.localsCnt = topSymbol->flags;
                curMacro.savedBlk = GetNumVal();
                Sub720A();
            } else if (curMacro.cnt == 0)
                SyntaxError();
        }

        if (! (curMacro.mtype == M_MACRO))
            InitSpoolMode();
    }
}



void DoRept(void) {
    DoIrpX(M_REPT);
    if ((yyType = curOp) != T_CR)
        SyntaxError();

    if (! (mSpoolMode & 1)) {
        curMacro.cnt = accum1;		// get the repeat count
        if (! MPorNoErrCode()) {	// skip processing if error other than M or P
            ChkForLocationError();
            curMacro.cnt = 0;
        }
        InitSpoolMode();
    }
}


void DoLocal(void) {
    if (mSpoolMode == 2) {
        if (HaveTokens()) {
            if (++curMacro.localsCnt == 0)		// 256 locals!!
                StackError();

            if (token[0].type != O_NAME)					// already seen so error
                MultipleDefError();

            InsertMacroSym(curMacro.localsCnt, 1);		// save this local with index
            macroInPtr = macroText;
        }
        if (curOp == T_CR) {			// local line processed to return to normal spooling
            mSpoolMode = 1;
            macroInPtr = macroText;
        }
    } else
        SyntaxError();					// local not ok here
}



void Sub78CE(void) {
    kk = *curMacro.pCurArg;
    byte chCnt = (kk == '!' && savedMtype == M_IRPC) ? 2 : 1;	// size arg (2 if escaped char)
    if (savedMtype == M_MACRO || (curMacro.cnt -= chCnt) == 0)	// all done
        UnNest(1);
    else {
        if (savedMtype == M_IRP)
            pNextArg = curMacro.pCurArg - (kk & 0x7F);		// skip foward to start of arg (stored as arg, len)
        else
            pNextArg = curMacro.pCurArg + chCnt;	// skip char or escaped char

        curMacro.mtype = savedMtype;
        Sub720A();
    }
    lookAhead = 0;
    b6B2C = atStartLine = true;
}
