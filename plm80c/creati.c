/****************************************************************************
 *  creati.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

static byte infoLengths[] = { 10, 12, 18, 18, 18, 22, 11, 10, 8, 9 };


offset_t AllocInfo(word infosize)
{
    offset_t p, q;

    Alloc(infosize, infosize);
    p = topInfo + 1;
    if (botSymbol < (q = topInfo + infosize))
        PFatalError(ERR83);
    memset(ByteP(p), 0, infosize);
    topInfo = q;
    return p;
} /* AllocInfo() */


void CreateInfo(word scope, byte type)
{
    byte len;

    len = infoLengths[type];
    curInfoP = AllocInfo(len);
    if (curSymbolP != 0) {
        SetLinkOffset(SymbolP(curSymbolP)->infoP);
        SymbolP(curSymbolP)->infoP = curInfoP;
    }
    SetType(type);
    SetLen(len);
    SetScope(scope);
    SetSymbol(curSymbolP);
} /* CreateInfo() */
