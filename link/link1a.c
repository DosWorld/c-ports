/****************************************************************************
 *  link1a.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdint.h>
#include <stdio.h>
pointer inRecord;
int maxRecLen;
byte inType;

void GetRecord() {
    if (!inRecord)
        inRecord = xmalloc(maxRecLen = 2048);
    if (fread(inRecord, 1, 3, objFp) != 3)
        FatalError("%s: Premature EOF", objName);
    recLen = getWord(inRecord + REC_LEN);
    if (maxRecLen < recLen + 3) {
        while (maxRecLen <= recLen + REC_DATA)
            maxRecLen += 4096;
        inRecord = xrealloc(inRecord, maxRecLen);
    }
    inP   = inRecord + REC_DATA;
    inEnd = inRecord + recLen + 2;  // exclude CRC
    if (fread(inP, 1, recLen, objFp) != recLen)
        FatalError("%s: Premature EOF", objName);
    recNum++;

    byte inCRC = 0;
    for (int i = 0; i < recLen + REC_DATA; i++)
        inCRC += inRecord[i];
    if (inCRC)
        RecError("Checksum error");
    inType = inRecord[0];
    if (inType == 0 || inType > R_COMDEF || (inType & 1)) /* other invalid caught elsewhere */
        IllegalRelo();
    /* check for special case handling */
    if (recLen > 1025 && inType != R_MODDAT && (inType < R_LIBNAM || inType == R_COMDEF))
        RecError("Record length > 1025");
}

void Position(uint32_t location) {

    if (fseek(objFp, location, SEEK_SET))
        IoError(objName, "Seek error");

    recNum = 0; /* reset vars */

} /* Position() */

void OpenObjFile() {
    objName = objFile->name;
    if (!(objFp = Fopen(objName, "rb")))
        IoError(objName, "Open error");
    recNum = 0;
    module = 0; /* reset vars and read at least 1 byte */
} /* OpenObjFile() */

void CloseObjFile() { /* Close() file and link to next one */
    if (fclose(objFp))
        IoError(objName, "Close error");
} /* CloseObjFile() */
