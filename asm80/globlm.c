/****************************************************************************
 *  globlm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

#define IN_BUF_SIZE  512
#define OUT_BUF_SIZE 512

char macroLine[129];
char *macroP  = macroLine;
bool inQuotes = false;
bool excludeCommentInExpansion;
bool inAngleBrackets;
byte expandingMacro; // 0,1 or 0xff
bool expandingMacroParameter;
bool inMacroBody = false;
byte mSpoolMode; // 0->normal, 1->spool (local ok), 2->capture locals, 0xfe->might not occur ,
                 // 0xff->capture body
bool b9060;
bool nestMacro;
byte savedMtype;
byte macroDepth;
byte macroSpoolNestDepth;
byte paramCnt;
byte startNestCnt;
byte argNestCnt = 0;
tokensym_t *pMacro;
pointer macroInPtr;
/*
    mtype has the following values
    1 -> IRP
    2 -> IRPC
    3 -> DoRept
    4 -> MACRO Invocation
    5 -> ???
*/

macro_t macro[10] = { { .blk = 0xffff } };

word curMacroBlk  = 0xFFFF;
word nxtMacroBlk  = 0;
word maxMacroBlk  = 0;
word macroBlkCnt;
byte macroBuf[129];
pointer savedMacroBufP;
pointer pNextArg;
word localIdCnt;
pointer startMacroLine;
pointer startMacroToken;
byte irpcChr[3]     = { 0, 0, 0x81 }; // where irpc char is built (0x81 is end marker)

byte localVarName[] = {
    '?', '?', 0, 0, 0, 0, 0x80
}; // where DoLocal vars are constructed (0x80 is end marker)
/* ov4 compat 2C8C */
pointer contentBytePtr;
byte fixupSeg;
word fixOffset;
byte curFixupHiLoSegId;
byte curFixupType;
byte fixIdxs[4] = { 0, 0, 0, 0 };
#define fix22Idx fixIdxs[0]
#define fix24Idx fixIdxs[1]
#define fix20Idx fixIdxs[2]
#define fix6Idx  fixIdxs[3]
byte extNamIdx                   = 0;
bool initFixupReq[4]             = { true, true, true, true };
bool firstContent                = true;
byte rEof[4]                     = { OMF_EOF, 0, 0 };
byte rExtnames[EXTNAMES_MAX + 4] = { OMF_EXTNAMES };
byte moduleNameLen               = 6;
byte rContent[CONTENT_MAX + 4]   = { OMF_CONTENT };
byte rPublics[PUBLICS_MAX + 4]   = { OMF_PUBLICS, 1, 0 };
byte rInterseg[INTERSEG_MAX + 4];
byte rExtref[EXTREF_MAX + 4];
byte rModend[MODEND_MAX + 4] = { OMF_MODEND, 4, 0 };
bool inComment               = false;
bool noOpsYet                = false;
byte nameLen;
byte startSeg = 1;
byte activeSeg;
bool inPublic = false;
bool inExtrn  = 0;
bool segDeclared[2];
byte alignTypes[4] = { 3, 3, 3, 3 };
word externId;
word itemOffset;
bool badExtrn     = false;
byte startDefined = 0;
word startOffset  = 0;
byte tokenIdx     = 0;
byte lineBuf[128];
token_t token[9]   = { { lineBuf } };

pointer endLineBuf = { lineBuf + 128 };
byte ifDepth       = 0;
bool skipIf[9];
bool inElse[9];
byte macroCondSP = 0;
byte macroCondStk[17];
byte opSP;
byte opStack[17];
word accum1, accum2;
byte acc1RelocFlags;
byte acc2RelocFlags;
bool hasVarRef;
byte acc1ValType;
byte acc2ValType;
word acc1RelocVal;
word acc2RelocVal;
byte curChar = 0;
byte reget   = false;
byte lookAhead;
// static byte pad6861 = 0;
tokensym_t *symTab[3];
tokensym_t *endSymTab[3];
pointer symHighMark;
pointer baseMacroTbl;
byte gotLabel = 0;
char name[6];
char savName[6];
bool haveNonLabelSymbol; // true if we have seen a user symbol and confirmed that there is no :
bool haveUserSymbol;     // true if we have seen a user symbol and  not yet seen a following :
bool xRefPending   = false;
byte passCnt       = 0;
bool createdUsrSym = false;
bool usrLookupIsID = false;
bool needsAbsValue = false;
FILE *objFp;
FILE *lstFp;
FILE *macroFp;
word statusIO;
word openStatus; /* status of last open for Read */
// static word pad6894 = 0xFFFF;
byte asmErrCode     = ' ';
bool spooledControl = false;
bool primaryValid   = true;
byte tokI;
bool errorOnLine;
bool atStartLine;
// static byte pad689D[2];
byte curCol = 1;
pointer endItem;
pointer startItem;
word pageLineCnt;
word effectiveAddr;
word pageCnt;
// static byte pad68AA;
bool showAddr;
// static byte pad68AC;
bool lineNumberEmitted = false;
bool b68AE             = false;
char tokStr[7]         = { 0, 0, 0, 0, 0, 0, 0 };
char inBuf[MAXLINE + 3]; // MAXLINE chars + \r\n\0

// static pointer pad6A05 = {outbuf};
// static byte pad6A07 = 0;
char *objFile;
char *lstFile;
word srcLineCnt  = 1;
// static byte pad6A50[2] = "  ";        /* protects for very big files */
word lineNo;
byte spIdx;
word lastErrorLine;
controls_t controls  = { .all = {
                            false, false, false, true,  true, false, true, true, true, false,
                            120,   66,    0,     false, 0,    0,     0,     true, true, true } };

bool ctlListChanged  = true;
byte titleLen        = { 0 };
bool controlSeen[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
byte saveStack[8][3];
byte saveIdx = 0;
char titleStr[64];
word tokBufLen;
byte tokType;
byte controlId;
char tokBuf[MAXFILEPARAM + 1];
word tokBufIdx = 0;
word tokNumVal;
bool isControlLine = false;
bool scanCmdLine;
bool inDB;
bool inDW;
bool inExpression;
bool has16bitOperand;
byte phase;
byte curOpFlags;
byte yyType;
byte curOp;
byte topOp;
bool b6B2C;
byte nextTokType;
bool finished;
bool inNestedParen;
bool expectingOperands;
bool expectingOpcode;
bool condAsmSeen; /* true when IF, ELSE, ENDIF seen [also macro to check] */
bool b6B33;
bool isInstr        = true;
bool expectOp       = true;
bool b6B36          = false;
word segLocation[5] = { 0, 0, 0, 0, 0 }; /* ABS, CODE, DATA, STACK, MEMORY */
word maxSegSize[3]  = { 0, 0, 0 };       /* seg is only ABS, CODE or DATA */
char *cmdLineBuf;
word errCnt;
pointer w6BCE;
char *cmdchP;
char *controlsP;
bool skipRuntimeError = false;
bool nestedMacroSeen;
byte ii;
byte jj;
byte kk;
// static byte b9B34 = 0;
char *curFileNameP; // CHANGE

address aVar;
