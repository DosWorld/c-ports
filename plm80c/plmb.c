/****************************************************************************
 *  plmb.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

byte verNo[] = VERSION;

// plmf.c
// the builtins each entry is
// name (pstr), id  (byte), paramCnt (byte), dataType (byte)
static struct {
    pstr_t *name;
    byte id;
    byte paramCnt;
    byte dataType;
} builtins[] = {
    // clang-format off
    {(pstr_t *)("\x5""CARRY"),  0,  0, 2},
    {(pstr_t *)("\x3""DEC"),    1,  1, 2},
    {(pstr_t *)("\x6""DOUBLE"), 2,  1, 3},
    {(pstr_t *)("\x4HIGH"),     3,  1, 2},
    {(pstr_t *)("\x5INPUT"),    4,  1, 2},
    {(pstr_t *)("\x4LAST"),     5,  1, 3},
    {(pstr_t *)("\x6LENGTH"),   6,  1, 3},
    {(pstr_t *)("\x3LOW"),      7,  1, 2},
    {(pstr_t *)("\x4MOVE"),     8,  3, 0},
    {(pstr_t *)("\x6OUTPUT"),   9,  1, 0},
    {(pstr_t *)("\x6PARITY"),   10, 0, 2},
    {(pstr_t *)("\x3ROL"),      11, 2, 2},
    {(pstr_t *)("\x3ROR"),      12, 2, 2},
    {(pstr_t *)("\x3SCL"),      13, 2, 2},
    {(pstr_t *)("\x3SCR"),      14, 2, 2},
    {(pstr_t *)("\x3SHL"),      15, 2, 2},
    {(pstr_t *)("\x3SHR"),      16, 2, 2},
    {(pstr_t *)("\x4SIGN"),     17, 0, 2},
    {(pstr_t *)("\x4SIZE"),     18, 1, 2},
    {(pstr_t *)("\x8STACKPTR"), 19, 0, 3},
    {(pstr_t *)("\x4TIME"),     20, 1, 0},
    {(pstr_t *)("\x4ZERO"),     21, 0, 2}
};

// the plm reserved keywords - format
// name (pstr), keywordId (byte) see intermediate tokens in plm.h
// in the symbols area the infoP value is set to 0xff00 + the keywordId
static struct {
    pstr_t *name;
    byte id;
} keywords[] = {
    {(pstr_t *)("\x7""ADDRESS"),  T_ADDRESS},
    {(pstr_t *)("\x3""AND"),      T_AND},
    {(pstr_t *)("\x2""AT"),       T_AT},
    {(pstr_t *)("\x5""BASED"),    T_BASED},
    {(pstr_t *)("\x2""BY"),       T_BY},
    {(pstr_t *)("\x4""BYTE"),     T_BYTE},
    {(pstr_t *)("\x4""CALL"),     T_CALL},
    {(pstr_t *)("\x4""CASE"),     T_CASE},
    {(pstr_t *)("\x4""DATA"),     T_DATA},
    {(pstr_t *)("\x7""DECLARE"),  T_DECLARE},
    {(pstr_t *)("\x7""DISABLE"),  T_DISABLE},
    {(pstr_t *)("\x2""DO"),       T_DO},
    {(pstr_t *)("\x4""ELSE"),     T_ELSE},
    {(pstr_t *)("\x6""ENABLE"),   T_ENABLE},
    {(pstr_t *)("\x3""END"),      T_END},
    {(pstr_t *)("\x3""EOF"),      T_EOF},
    {(pstr_t *)("\x8""EXTERNAL"), T_EXTERNAL},
    {(pstr_t *)("\x2GO"),         T_GO},
    {(pstr_t *)("\x4GOTO"),       T_GOTO},
    {(pstr_t *)("\x4HALT"),       T_HALT},
    {(pstr_t *)("\x2IF"),         T_IF},
    {(pstr_t *)("\x7INITIAL"),    T_INITIAL},
    {(pstr_t *)("\x9INTERRUPT"),  T_INTERRUPT},
    {(pstr_t *)("\x5LABEL"),      T_LABEL},
    {(pstr_t *)("\x9LITERALLY"),  T_LITERALLY},
    {(pstr_t *)("\x5MINUS"),      T_MINUS},
    {(pstr_t *)("\x3MOD"),        T_MOD},
    {(pstr_t *)("\x3NOT"),        T_NOT},
    {(pstr_t *)("\x2OR"),         T_OR},
    {(pstr_t *)("\x4PLUS"),       T_PLUS},
    {(pstr_t *)("\x9PROCEDURE"),  T_PROCEDURE},
    {(pstr_t *)("\x6PUBLIC"),     T_PUBLIC},
    {(pstr_t *)("\x9REENTRANT"),  T_REENTRANT},
    {(pstr_t *)("\x6RETURN"),     T_RETURN},
    {(pstr_t *)("\x9STRUCTURE"),  T_STRUCTURE},
    {(pstr_t *)("\x4THEN"),       T_THEN},
    {(pstr_t *)("\x2TO"),         T_TO},
    {(pstr_t *)("\x5WHILE"),      T_WHILE},
    {(pstr_t *)("\x3XOR"),        T_XOR}};

// clang-format on

static void InstallBuiltins() {
    for (int i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        Lookup(builtins[i].name);
        CreateInfo(0, BUILTIN_T, curSym);
        info->builtinId = builtins[i].id;
        SetParamCnt(builtins[i].paramCnt);
        SetDataType(builtins[i].dataType);
    }
    Lookup((pstr_t *)"\x6MEMORY");
    CreateInfo(0, BYTE_T, curSym);
    info->flag |= F_LABEL | F_MEMORY | F_ARRAY;
} /* InstallBuiltins() */

static void InstallKeywords() {
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        Lookup(keywords[i].name);
        symtab[curSym].infoIdx = 0xFF00 | keywords[i].id;
    }
} /* InstallKeywords() */

static void InitInfoAndSym() {
    SetPageNo(1);
    localLabelCnt = 0;
    cmdLineCaptured++;
    procChains[0] = procChains[1] = blockDepth = 0;
} /* InitInfoAndSym() */

void InitKeywordsAndBuiltins() {
    InitInfoAndSym();
    InstallKeywords();
    InstallBuiltins();
} /* InitKeywordsAndBuiltins() */

void SetDate(char *str, byte len) {
    if (len > 9)
        len = 9;
    memset(DATE, ' ', 9);
    memcpy(DATE, str, len);
    DATE[9] = '\0';
} /* SetDate() */

void SetPageLen(word len) {
    PAGELEN = (byte)len;
} /* SetPageLen() */

void SetPageNo(word v) {
    pageNo = v - 1;
}

void SetMarginAndTabW(byte startCol, byte width) {
    margin = startCol - 1;
    tWidth = width;
}

void SetTitle(char *str, byte len) {
    if (len > 60)
        len = 60;
    memmove(TITLE, str, len);
    TITLELEN = len;
} /* SetTitle() */

void SetPageWidth(word width) {
    PWIDTH = (byte)width;
} /* SetPageWidth() */