/****************************************************************************
 *  main0.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef _MAX_PATH
#define _MAX_PATH 4096
#endif

// static byte copyright[] = "[C] 1976, 1977, 1982 INTEL CORP";

jmp_buf exception;

static void WriteDepend() {
    mkpath(depFileName);
    FILE *fp = fopen(depFileName, "wt");
    if (!fp)
        IoError(depFileName, "Can't create dependency file");

    char oname[_MAX_PATH + 1];
    int col = 0;

    col += fprintf(fp, "%s:", MapFile(oname, objFileName));
    col += fprintf(fp, " %s", MapFile(oname, srcFileTable[0].fNam));
    for (int i = 0; i < includeCnt; i++) {
        if (col + strlen(MapFile(oname, includes[i])) > 120) {
            fputs(" \\\n    ", fp);
            col = 4;
        }
        col += fprintf(fp, " %s", oname);
    }
    putc('\n', fp);
    for (int i = 0; i < includeCnt; i++)
        fprintf(fp, "%s:\n", MapFile(oname, includes[i]));
    fclose(fp);
}

void unwindInclude() { // cleanup any open include files
    if (srcFileIdx) {
        while (srcFileIdx)
            CloseF(&srcFileTable[srcFileIdx--]);
        srcFil = srcFileTable[0];
    }
    rewind(srcFil.fp);
}

static void FinishLexPass() {
    if (afterEOF)
        Wr1SyntaxError(ERR87); /* MISSING 'end' , end-OF-FILE ENCOUNTERED */
    Wr1LineInfo();
    Wr1Byte(L_EOF);
    vfRewind(&utf1);
    unwindInclude();
    if (DEPEND)
        WriteDepend();
} /* FinishLexPass() */

static void ParseCommandLine() {
    if (moreCmdLine)
        ParseControlLine(cmdTextP);   // ParseControlLine will move to first char
    moreCmdLine   = false;            // 0 if no more cmd line
    inChrP        = (uint8_t *)" \n"; // GNxtCh will see \n and get a non blank line
    scopeSP    = 1;
    scopeChains[1] = 0;
} /* ParseCommandLine() */

static void LexPass() {
    ParseCommandLine();
    OpenF(&srcFil, "SOURCE", srcFileTable[0].fNam, "rt");
    GNxtCh(); // get the first char
    ParseProgram();
} /* LexPass() */

word Start0() {
    if (setjmp(exception) == 0)
        LexPass();
    FinishLexPass(); /* exception goes to here */
    return 1;        // Chain(overlay[1]);
}
