/****************************************************************************
 *  asm3m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// vim:ts=4:expandtab:shiftwidth=4:
#include "asm80.h"

/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
bool isExprOrMacroMap[] = { true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
                            true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
                            true,  true,  true,  true,  true,  true,  false, false, false, false,
                            false, false, false, false, false, false, false, false, false, false,
                            false, false, false, false, false, false, false, false, false, false,
                            false, false, false, false, true,  true,  true,  true,  true,  true,
                            true,  true,  false, true,  true,  true };
bool isInstrMap[] = { false, true,  true,  true,  false, false, false, false, false, false, false,
                      false, false, false, false, false, false, false, false, false, false, false,
                      false, false, false, false, true,  true,  false, false, false, false, false,
                      false, false, false, true,  true,  true,  true,  true,  true,  true,  true,
                      true,  true,  true,  false, false, false, false, false, false, false, false,
                      false, false, false, false, false, false, false, false, false, true,  true };

void ChkSegAlignment(byte seg) { /* seg = 0 => CSEG, seg = 1 => DSEG */

    if (segDeclared[seg]) {
        if (HaveTokens()) {
            accum1 = GetNumVal();               // get align type
            if (alignTypes[seg] != Low(accum1)) // alignt type mis-match
                ExpressionError();
        } else if (alignTypes[seg] != 3) /* no specifier - check byte algined */
            ExpressionError();
    } else {
        segDeclared[seg] = true;
        if (HaveTokens()) {
            accum1 = GetNumVal();                // get align type
            if (accum1 == 1 || accum1 == 2)      /* only allow inPage and Page */
                alignTypes[seg] = accum1 & 0xff; // set algin type
            else
                ExpressionError();
        }
    }
}

void UpdateHiLo(byte hilo) {
    if ((acc1RelocFlags & (UF_EXTRN + UF_SEGMASK)) != 0)
        acc1RelocFlags = (acc1RelocFlags & ~UF_RBOTH) | hilo;
}

void HandleOp(void) {
    switch (topOp) {
    case 0:
        break;
    case 1:
        FinishLine(); /* CR */
        break;
    case 2:                                            /* ( */
    case 3:                                            /* ) */
        if (!(topOp == LPAREN && curOp == RPAREN)) // incorrectly nested ()
            BalanceError();

        if (token[0].type == O_DATA) {
            token[0].size = 1;
            token[0].attr = 0;
            b6B36         = true;
        }

        expectOp = inNestedParen;
        if (curOp == RPAREN)
            b6B2C = true;
        break;
    case 4:
        accum1 *= accum2; /* * */
        break;
    case 5:
        accum1 += accum2; /* + */
        break;
    case 6: /* , */
        SyntaxError();
        PopToken();
        break;
    case 7:
        accum1 -= accum2; /* - */
        break;
    case 8:; /* unary + */
        break;
    case 9: /* / */
        if (accum2 == 0) {
            ValueError();
            accum1 = 0xffff; /* synthesise what 8085 does on / 0 */
        } else
            accum1 /= accum2;
        break;
    case 10:
        accum1 = -accum1; /* unary - */
        break;
    case 11:
        accum1 = accum1 == accum2 ? 0xffff : 0; /* EQ */
        break;
    case 12:
        accum1 = accum1 < accum2 ? 0xffff : 0; /* LT */
        break;
    case 13:
        accum1 = accum1 <= accum2 ? 0xffff : 0; /* LE */
        break;
    case 14:
        accum1 = accum1 > accum2 ? 0xffff : 0; /* GT */
        break;
    case 15:
        accum1 = accum1 >= accum2 ? 0xffff : 0; /* GE */
        break;
    case 16:
        accum1 = accum1 != accum2 ? 0xffff : 0; /* NE */
        break;
    case 17:
        accum1 = ~accum1; /* NOT */
        break;
    case 18:
        accum1 &= accum2; /* AND */
        break;
    case 19:
        accum1 |= accum2; /* OR */
        break;
    case 20:
        accum1 ^= accum2; /* XOR */
        break;
    case 21: /* MOD */
        if (accum2 == 0) {
            ValueError();
            accum1 = accum2; // this is what the 8080 mod code would do
        } else
            accum1 %= accum2;
        break;
    case 22: /* SHL */
        if (Low(accum2) != 0)
            accum1 <<= accum2;
        break;
    case 23: /* SHR */
        if (Low(accum2) != 0)
            accum1 >>= accum2;
        break;
    case 24: /* HIGH */
        accum1 >>= 8;
        UpdateHiLo(UF_RHIGH);
        break;
    case 25: /* LOW */
        accum1 &= 0xff;
        UpdateHiLo(UF_RLOW);
        break;
    case 26: /* DB ? */
        if (token[0].type != O_STRING) {
            accum1 = GetNumVal();
            if (accum1 >= 0x100 && accum1 < 0xff00) /* high byte ! 0 or FF */
                ValueError();
            curOpFlags = 0x44; /* adjusted from PLM as revised code doesn't use rotate */
            if ((acc1RelocFlags & UF_RBOTH) == UF_RBOTH) { // can't db a 16 bit relocatable
                ValueError();
                acc1RelocFlags =
                    (acc1RelocFlags & ~UF_RBOTH) | UF_RLOW; // treat as 8 bit relocatable
            }
        } else {
            acc1RelocFlags = 0;      // abs bytes
            token[0].type  = O_DATA; // flag as data
        }

        if (IsReg(acc1ValType)) // db of register is not valid
            OperandError();
        nextTokType = O_DATA; // loook for more data
        inDB        = true;   // we are in DB statement
        break;
    case 27:                  /* DW ? */
        nextTokType = O_DATA; // look for more data
        inDW        = true;   // we are in DW
        break;
    case 28:                              /* DS ? */
        segLocation[activeSeg] += accum1; // bump current $ value
        showAddr = true;                  // flag that new address should be shown
        break;
    case 29: /* EQU ? */
    case 30: /* SET ? */
        showAddr = true;
        if ((acc1RelocFlags & UF_EXTRN) == UF_EXTRN) { /* cannot SET or EQU to external */
            ExpressionError();
            acc1RelocFlags = 0;
        }
        labelUse = L_SETEQU;
        UpdateSymbolEntry(accum1, (SET + O_SET) - topOp); /* 4 for set, 5 for equ */
        expectOperand = false;
        break;
    case 31: /* ORG ? */
        showAddr = true;
        if ((acc1RelocFlags & UF_EXTRN) != UF_EXTRN) { // check not ORG to extern
            if ((acc1RelocFlags & UF_RBOTH) != 0) // only org to abs or current seg 16 bit reloc
                if ((acc1RelocFlags & UF_SEGMASK) != activeSeg ||
                    (acc1RelocFlags & UF_RBOTH) != UF_RBOTH)
                    ExpressionError();
        } else
            ExpressionError();

        if (controls.object) // if object file update maxSegSize for this segment
            if (segLocation[activeSeg] > maxSegSize[activeSeg])
                maxSegSize[activeSeg] = segLocation[activeSeg];
        segLocation[activeSeg] = accum1; // set new segLocation
        break;
    case 32: /* END ? */
        if (tokenIdx > 0) {
            startOffset  = GetNumVal();
            startDefined = 1;
            startSeg     = acc1RelocFlags & 7;
            if ((acc1RelocFlags & UF_EXTRN) == UF_EXTRN)
                ExpressionError();
            if (IsReg(acc1ValType))
                OperandError();

            showAddr = true;
        }
        kk         = mSpoolMode;
        mSpoolMode = 0;

        if (macroCondSP > 0 || (kk & 1))
            NestingError();
        if (curOp != EOL)
            SyntaxError();
        if (expectOp)
            b6B33 = true;
        else
            SyntaxError();
        break;
    case 33: /* IF ? */
        if (expectOp) {
            condAsmSeen = true;
            Nest(2);            /* push current skip/else status */
            xRefPending = true; /* force any pending xref info to be written*/
            if (!skipIf[0])     /* if ! skipping set new status */
                skipIf[0] = !(accum1 & 1);
            inElse[0] = false; /* ! in else at this nesting level */
        }
        break;
    case 34: /* ELSE ? */
        condAsmSeen = true;
        if (macroCondStk[0] != 2) /* check ! mid macro nest */
            NestingError();
        else if (!inElse[0]) /* shouldn't be in else at this level */
        {
            if (!skipIf[0]) /* IF was active so ELSE forces skip */
                skipIf[0] = true;
            else /* IF inactive so revert to previous skipping status */
                skipIf[0] = skipIf[ifDepth];
            inElse[0] = true; /* in else at this nesting level */
        } else
            NestingError(); /* multiple else !! */
        break;
    case 35: /* ENDIF ? */
        if (expectOp) {
            condAsmSeen = true;
            UnNest(2); /* revert to previous status */
        }
        break;
        /* in the following topOp = LXI and nextTokType = O_DATA
           except where noted on return from MkCode */
    case 36: /* LXI ? */
        if (nameLen == 1)
            if (name[0] == 'M')
                SyntaxError();
        MkCode(0x85); /* topOp = INRDCR, nextTokType unchanged on return */
        break;
    case 37: /* REG16 ops POP DAD PUSH INX DCX ? */
        if (nameLen == 1)
            if (name[0] == 'M')
                SyntaxError();
        MkCode(5);
        break;
    case 38:
        MkCode(7); /* LDAX STAX ? */
        break;
    case 39:
        MkCode(2); /* ARITH ops ADC ADD SUB ORA SBB XRA ANA CMP ? */
        break;
    case 40:
        MkCode(8); /* IMM8 ops ADI OUT SBI ORI IN CPI SUI XRI ANI ACI ? */
        break;
    case 41:
        MkCode(0x46); /* MVI ?  topOp = IMM8 on return */
        break;
    case 42:
        MkCode(6); /* INR DCR ? */
        break;
    case 43:
        MkCode(0x36); /* MOV   topOp = ARITH, nextTokType unchanged on return*/
        break;
    case 44:
        MkCode(0); /* IMM16 ops CZ CNZ JZ STA JNZ JNC LHLD */
                   /* CP JC SHLD CPE CPO CM LDA JP JM JPE */
                   /* CALL JPO CC CNC JMP */
        break;
    case 45:
        MkCode(0); /* SINGLE ops RNZ STC DAA DI SIM SPHL RLC */
                   /* RP RAL HLT RM RAR RPE RET RIM */
                   /* PCHL CMA CNC RPO EI XTHL NOP */
                   /* RC RNX XCHG RZ RRC */
        break;
    case 46:
        MkCode(6); /* RST */
        break;
    case 47:
        activeSeg = 0; /* ASEG ? */
        break;
    case 48: /* CSEG ? */
        activeSeg = 1;
        ChkSegAlignment(0);
        break;
    case 49: /* DSEG ? */
        activeSeg = 2;
        ChkSegAlignment(1);
        break;
    case 50: /* PUBLIC */
        inPublic = true;
        labelUse = L_REF;
        UpdateSymbolEntry(0, O_REF);
        break;
    case 51: /* EXTRN ? */
        inExtrn = true;
        if (externId == 0 && IsPhase1() && controls.object)
            WriteModhdr();
        labelUse = L_REF;
        UpdateSymbolEntry(externId, O_TARGET);
        if (IsPhase1() && controls.object && !badExtrn)
            WriteExtName();
        if (!badExtrn)
            externId++;
        badExtrn = false;
        break;
    case 52: /* NAME */
        if (tokenIdx != 0 && noOpsYet) {
            memcpy(moduleName, name, moduleNameLen = nameLen);
            moduleName[moduleNameLen] = '\0';
        } else
            SourceError(
                'R'); /* NAME directive was preceded by an instruction or another directive */
        PopToken();   // consume the token
        break;
    case 53:
        segLocation[SEG_STACK] = accum1; /* STKLN ? */
        break;
    case 54:
        DoMacro(); /* MACRO ? */
        break;
    case 55:
        DoMacroBody(); /* MACRO BODY */
        break;
    case 56:
        DoEndm(); /* ENDM */
        break;
    case 57:
        DoExitm(); /* EXITM */
        break;
    case 58: /* MACRONAME */
        curMacro.mtype = M_INVOKE;
        initMacroParam();
        break;
    case 59:
        DoIrpX(M_IRP); /* IRP ? */
        break;
    case 60:
        DoIrpX(M_IRPC); /* IRPC */
        break;
    case 61:
        DoIterParam(); /* MACRO PARAMETER */
        break;
    case 62:
        DoRept(); /* REPT ? */
        break;
    case 63:
        DoLocal(); /* LOCAL */
        break;
    case 64:
        Sub78CE(); /* optVal */
        break;
    case 65: /* NUL */
        accum1 = token[0].type == NUL ? 0xffff : 0;
        PopToken();
        acc1RelocFlags = 0;
        break;
    }

    if (topOp != EOL)
        noOpsYet = false;
}

static byte IsExpressionOp(void) // returns true if op is valid in an expression
{
    if (yyType > RPAREN)
        if (yyType != COMMA)
            if (yyType < DB)
                return true;
    return false;
}

static byte IsVar(byte type) {
    return type == O_NAME || type == O_SETEQUNAME;
}

static void UpdateIsInstr(void) {
    if (!isInstrMap[topOp])
        isInstr = false;
}

void Parse(void) {
    while (1) {
        /* nothing to parse if
            skipping if and not cr or if related keyword
            or spooling and not macro related keyword or in quotes
        */
#ifdef TRACE
        printf("Parse: ");
        ShowYYType();
#endif
        if (((!(yyType == EOL || (END <= yyType && yyType <= ENDIF))) && skipIf[0]) ||
            ((opFlags[yyType] < 128 || inQuotes) &&
             (mSpoolMode & 1))) { // if spooling skip non macro and in quotes
            needsAbsValue = false;
            PopToken();
            return;
        }

        if (phase != 1)
            if (inExpression)
                if (IsExpressionOp())
                    if (GetPrec(yyType) <= GetPrec(opStack[opSP]))
                        ExpressionError();

        if (GetPrec(curOp = yyType) > GetPrec(topOp = opStack[opSP]) ||
            curOp == LPAREN) { /* SHIFT */
            if (opSP >= 16) {
                opSP = 0;
                StackError();
            } else
                opStack[++opSP] = curOp;
            if (curOp == LPAREN) {
                inNestedParen = expectOp;
                expectOp      = true;
            }
#ifdef TRACE
            printf("\n>>>> Shift\n");
            DumpOpStack();
            DumpTokenStack(false);
            printf("\n");
#endif
            if (phase > 1)
                inExpression = IsExpressionOp();
            return;
        }

        /* REDUCE */
#ifdef TRACE
        printf("\n<<<< Start - reduce\n");
        DumpOpStack();
        DumpTokenStack(false);
#endif
        inExpression = false;
        if ((!expectOp) && topOp > RPAREN)
            SyntaxError();

        if (topOp == VALUE) /* topOp used so set to curOp */
            topOp = curOp;
        else
            opSP--; /* pop Op */

        if ((curOpFlags = opFlags[topOp]) & 1) { /* load rhs operand into acc1 and acc2) */
            accum2         = GetNumVal();
            acc2RelocFlags = acc1RelocFlags;
            acc2RelocVal   = acc1RelocVal;
            acc2ValType    = acc1ValType;
        }

        if (curOpFlags & 2) /* removed ror */ /* has a lhs operand so load into acc1 */
            accum1 = GetNumVal();

        if (!hasVarRef)
            hasVarRef = IsVar(acc1ValType) || IsVar(acc2ValType);

        nextTokType = O_NUMBER;               // assume next token is a number, HandleOp may change
        if (topOp > RPAREN && topOp < DB) /* expression topOp */
            ResultType();                     // check and set result type
        else {
            UpdateIsInstr();
            ChkInvalidRegOperand();
        }

        HandleOp();
        if (!isExprOrMacroMap[topOp])
            expectOp = false;

        if (b6B2C) {
            b6B2C = false;
            return;
        }

        if (topOp != DS && showAddr)
            effectiveAddr = accum1;

        if ((curOpFlags & 0x3C)) /* -xxxxx-- -> collect list or bytes */
            PushToken(nextTokType);
        /* previous loop unrolled to avoid little endian assumption */
        /* --xxxx-- -> collect high/low acc1/acc2 */
        if (curOpFlags & 4)
            CollectByte(Low(accum1));
        if (curOpFlags & 8)
            CollectByte(High(accum1));
        if (curOpFlags & 16)
            CollectByte(Low(accum2));
        if (curOpFlags & 32)
            CollectByte(High(accum2));

        token[0].attr  = acc1RelocFlags;
        token[0].symId = acc1RelocVal;
        if (curOpFlags & 0x40)      /* -x------ -> list */
            if (curOp == COMMA) { // if comma then make the operator (topOp) as
                yyType   = topOp;   // the next item to read and mark as operator
                expectOp = true;
            }
#ifdef TRACE
        printf("\n<<<< End - reduce\n");
        DumpOpStack();
        DumpTokenStack(false);
        printf("\n");
#endif
    }
}

void DoPass(void) {
    while (!finished) {
        Tokenise();
        //		DumpOpStack();
        //		DumpTokenStack();
        Parse();
    }
}
