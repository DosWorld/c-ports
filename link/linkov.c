/****************************************************************************
 *  linkov.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdlib.h>

// static char COPYRIGHT[] = "[C] 1976, 1977, 1979 INTEL CORP";

struct {
    word next;
    union {
        struct {
            word typeAndSeg;
            word headReloc;
        } f;
        struct {
            word offset;
            word symId;
        } r;
    };
} *fixups;

word fixupCnt;
word fixupSize;
#define HEADEXT 0
#define HEADREL 1
#define FCHUNK  256

psModName_t modName;
static bool ancestorNameNotSet;
static word segBias;
static symbol_t **extMap;
static word externsCount = 0;
byte outRec[1060]; // max record 1025 + 3 + room unchecked overflow symbol 32

void WriteByte(uint8_t val) {
    *outP++ = val;
}

void WriteWord(uint16_t val) {
    *outP++ = (byte)val;
    *outP++ = val >> 8;
}

void WriteName(pstr_t *name) {
    Pstrcpy(name, outP);
    outP += name->len + 1;
}

void AddExtMap(symbol_t *symbol) {
    extMap[externsCount++] = symbol; /* record the symbol mapping */
} /* AddExtMap() */

symbol_t *GetSymbolP(word symId) {

    if (symId >= externsCount) /* out of range */
        IllFmt();
    return extMap[symId]; /* return the symbol */
} /* GetSymbolP() */

void InitExternsMap() {
    if (extMap == 0) /* ! already allocated */
        extMap = xmalloc(maxExternCnt * sizeof(symbol_t **));
    externsCount = 0;
} /* InitExternsMap() */

void InitRecord(byte type) {
    outRec[REC_TYPE] = type;
    outP             = outRec + REC_DATA;
} /* InitRecord() */

void EndRecord() {
    // 1025 (+3 for type & len, -1 for pending crc)
    if (putWord(&outRec[REC_LEN], (word)(outP - outRec - 2)) > 1025)
        FatalError("%s: Record length > 1025", outName);
    byte crc;
    pointer pch;
    for (crc = 0, pch = outRec; pch < outP; crc -= *pch++) /* calculate and insert crc */
        ;
    *outP++ = crc; // add the crc
    if (fwrite(outRec, 1, outP - outRec, outFp) != outP - outRec)
        IoError(outName, "Write error");
} /* EndRecord() */

bool ExtendRec(word cnt) {
    if (outP - outRec + cnt < 1027) /* room in buffer allow for CRC */
        return false;
    EndRecord();           /* Close() off the current record */
    InitRecord(outRec[0]); /* and prepare another */
    return true;
} /* ExtendRec() */

void static EmitMODHDRComSegInfo(byte seg, word len, byte combine) {
    if (ExtendRec(SEGDEF_sizeof))                          /* make sure enough room */
        FatalError("%s: Module header too long", outName); /* mod hdr too long */
    WriteByte(seg);
    WriteWord(len);
    WriteByte(combine);
} /* EmitMODHDRComSegInfo() */

void EmitMODHDR() {
    InitRecord(R_MODHDR);
    WriteName((pstr_t *)&moduleName);
    WriteByte(tranId);
    WriteByte(tranVn);

    for (byte fixSeg = SEG_CODE; fixSeg <= SEG_MEMORY; fixSeg++) { /* regular segments */
        if (segLen[fixSeg] > 0)
            EmitMODHDRComSegInfo(fixSeg, segLen[fixSeg], alignType[fixSeg]);
    }
    if (segLen[0] > 0) /* unamed common segment */
        EmitMODHDRComSegInfo(SEG_BLANK, segLen[0], alignType[0]);

    /* named common segments */
    for (symbol_t *commSym = headCommSym; commSym; commSym = commSym->nxtSymbol)
        EmitMODHDRComSegInfo(commSym->seg, commSym->len, commSym->flags);
    EndRecord();
} /* EmitMODHDR() */

void EmitEnding() {
    InitRecord(R_MODEND); /* init the record */
    WriteByte(moduleType);
    WriteByte(entrySeg);
    WriteWord(entryOffset);
    EndRecord();          /* finalise */
    InitRecord(R_MODEOF); /* emit and eof record as well */
    EndRecord();
} /* EmitEnding() */

void EmitCOMDEF() {
    if (!headCommSym) /* no named common */
        return;
    InitRecord(R_COMDEF); /* prep the record */
    /* chase down the definitions in seg order */
    for (symbol_t *commSym = headCommSym; commSym; commSym = commSym->nxtSymbol) {
        ExtendRec(2 + commSym->name.len); /* overflow to another record if needed */
        WriteByte(commSym->seg);
        WriteName(&commSym->name);
    }
    EndRecord(); /* finalise */
} /* EmitCOMDEF() */

void EmitPUBLICS() {
    for (word fixSeg = 0; fixSeg < 256; fixSeg++) { /* scan all segs */
        if (segmap[fixSeg]) {                       /* seg used */
            InitRecord(R_PUBDEF);                   /* init the record */
            WriteByte((byte)fixSeg);                /* seg needed */
            /* scan all files, modules and symbols*/
            for (objFile = objFileList; objFile; objFile = objFile->next) {
                for (module = objFile->modules; module; module = module->next) {
                    for (symbol_t *symbol = module->symbols; symbol; symbol = symbol->nxtSymbol) {
                        if (symbol->seg == fixSeg) { /* this symbol in the right seg */
                            /* makes sure enough room in record */
                            if (ExtendRec(4 + symbol->name.len))
                                /* overflowed to new record so add the segid */
                                WriteByte((byte)fixSeg);
                            WriteWord(symbol->offsetOrSym);
                            WriteName(&symbol->name);
                            WriteByte(0);
                        }
                    }
                }
            }
            EndRecord(); /* finish any open record */
        }
    }
} /* EmitPUBLICS() */

void EmitEXTNAMES() {
    if (unresolvedList == 0) /* no unresolved */
        return;
    InitRecord(R_EXTNAM); /* init the record */
    unresolved = 0;

    for (symbol_t *symbol = unresolvedList; symbol; symbol = symbol->nxtSymbol) {
        if (ExtendRec(2 + symbol->name.len)) /* check room for len, symbol && 0 */
            ;                                /* no need for special action on extend */
        WriteName(&symbol->name);
        WriteByte(0);
        symbol->offsetOrSym = unresolved++; /* record the final ext sym id */
    }
    EndRecord(); /* clean closure of record */
} /* EmitEXTNAMES() */

void EmitANCESTOR() {
    if (ancestorNameNotSet) /* we have a module name to use */
    {
        InitRecord(R_ANCEST);          /* init the record */
        WriteName((pstr_t *)&modName); /* copy name */
        EndRecord();
        ancestorNameNotSet = false; /* it is now set */
    }
} /* Emit_ANCESTOR() */

byte SelectSeg(byte seg) {
    if (seg == SEG_CODE)
        segBias = module->cbias;
    else if (seg == SEG_DATA)
        segBias = module->dbias;
    else
        segBias = 0;    /* only code and data modules are relative to module location */
    return segmap[seg]; /* return seg mapping */
} /* SelectOutSeg() */

void Pass2MODHDR() {
    Pstrcpy(ReadName(), &modName);       /* read in the module name */
    ancestorNameNotSet = true;           /* note the ancestor record has not been written */
    for (word seg = 0; seg < 256; seg++) /* init the segment mapping */
        segmap[seg] = (byte)seg;
    GetRecord();
} /* Pass2MODHDR() */

void Pass2COMDEF() {
    while (inP < inEnd) { /* while more common definitions */
        uint8_t segId = ReadByte();
        pstr_t *name  = ReadName();
        symbol_t *commSym;
        if (!Lookup(name, &commSym, F_ALNMASK)) /* check found */
            RecError("Phase error");
        segmap[segId] = commSym->seg; /* record the final linked seg where this goes */
    }
    GetRecord();
} /* Pass2COMDEF() */

void Pass2EXTNAMES() {
    while (inP < inEnd) { /* while more external definitions */
        pstr_t *name = ReadName();
        symbol_t *symbol;
        if (!Lookup(name, &symbol, F_SCOPEMASK)) /* get the name */
            RecError("Phase error");
        AddExtMap(symbol);
        if (symbol->flags == F_EXTERN) { /* still an extern */
            /* write the unresolved reference info */
            PrintAndEcho(" %s", p2cstr(&symbol->name));
            ModuleWarning(" - Referenced in ");
        }
        ReadByte(); // skip the 0
    }
    GetRecord();
} /* Pass2EXTNAMES() */

/* the names below are made global to support nested procedures
 * rather than adding additional arguments to function calls
 */

word newFixup() {
    if (fixupCnt >= fixupSize)
        fixups = xrealloc(fixups, sizeof(*fixups) * (fixupSize += FCHUNK));
    return fixupCnt++;
}

static word GetTypeAndSegHead(word prev, word typeAndSeg) {
    /* chase down the fixup chain matching seg and fixup type */
    for (word i = fixups[prev].next; i; i = fixups[i].next) {
        if (fixups[i].f.typeAndSeg == typeAndSeg) /* found existing list */
            return i;
        prev = i; /* step along */
    }
    word nf                 = newFixup(); /* add to the list */
    fixups[nf].next         = fixups[prev].next;
    fixups[prev].next       = nf;
    fixups[nf].f.typeAndSeg = typeAndSeg; /* save the typeAndSeg */
    fixups[nf].f.headReloc  = 0;
    return nf;
} /* GetTypeAndSegHead() */

static void AddRelocFixup(byte type, byte fixType, word offset, word segOrSym) {
    word head   = GetTypeAndSegHead(type, type == HEADEXT ? fixType : segOrSym * 256 + fixType);
    word relIdx = newFixup();
    fixups[relIdx].next      = fixups[head].f.headReloc;
    fixups[head].f.headReloc = relIdx;
    fixups[relIdx].r.offset  = offset;
    if (type == HEADEXT)
        fixups[relIdx].r.symId = segOrSym;
} /* AddRelocFixup() */

void Pass2CONTENT() {
    if (recLen > 1025) {
        if (ReadByte() == 0) { // ok for ABS segment
            // copy as large records shouldn't have fixup
            if (fwrite(inRecord, 1, recLen + REC_DATA, outFp) != recLen + 3)
                IoError(outName, "Write error");
            GetRecord();
            return;
        } else
            FatalError("%s: Record length > 1025 for non ABSOLUTE content", objName);
    }
    // it's a potentially relocatable record, so copy original to output
    memcpy(outRec, inRecord, recLen + REC_DATA);

    byte outSeg  = SelectSeg(ReadByte()); /* get the mapped link segment */
    word outBias = segBias;               /* and the bias to apply */
    word start   = ReadWord();
    word len     = recLen - 4; // data bytes excludes crc
    word end     = start + len - 1;

    GetRecord(); /* prime next record */

    InitRecord(R_MODDAT);
    WriteByte(outSeg);
    WriteWord(start + outBias);
    pointer content = outP; // base location of data to be relocated
    outP += len;

    if (!fixups)
        fixups = xmalloc(sizeof(*fixups) * (fixupSize = FCHUNK));
    fixups[HEADEXT].next = 0; // init fixup list
    fixups[HEADREL].next = 0;
    fixupCnt             = 2;

    /* process the relocate records */
    while (inType == R_FIXEXT || inType == R_FIXLOC || inType == R_FIXSEG) {
        byte fixType;
        if (inType == R_FIXEXT) {
            if ((fixType = ReadByte()) - 1 > 2) /* make sure combine is valid */
                IllFmt();
            while (inP < inEnd) { /* process all of the extref fixups */
                uint16_t extIdx = ReadWord();
                uint16_t offset = ReadWord();

                if (offset < start || end < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                uint16_t fixLoc  = offset - start;
                symbol_t *symbol = GetSymbolP(extIdx); /* get symbol entry*/
                if (symbol->flags == F_PUBLIC) {       /* is a public so resolved */
                    /* Lookup fixup address in buffer */

                    switch (fixType) { /* relocate to seg current base */
                    case FLOW:
                        content[fixLoc] += Low(symbol->offsetOrSym);
                        break;
                    case FHIGH:
                        content[fixLoc] += High(symbol->offsetOrSym);
                        break;
                    case FBOTH:
                        putWord(&content[fixLoc], getWord(&content[fixLoc]) + symbol->offsetOrSym);
                        break;
                    }
                    if (symbol->seg != SEG_ABS) /* if not ABS add a fixup entry */
                        AddRelocFixup(HEADREL, fixType, offset + outBias, symbol->seg);
                } else /* is an extern still so add to list */
                    AddRelocFixup(HEADEXT, fixType, offset + outBias, symbol->symId);
            }
        } else {                                /* reloc or interseg */
            byte fixSeg = outSeg;               /* get default reloc seg */
            segBias     = outBias;              /* and reloc base to that of the content record */
            if (inType == R_FIXSEG)             /* if we are interseg then update the reloc seg */
                fixSeg = SelectSeg(ReadByte()); /* also updates the outRelocOffset */

            if (fixSeg == 0) /* ABS is illegal */
                IllFmt();
            if ((fixType = ReadByte()) - 1 > 2) /* bad fix up type ? */
                IllFmt();
            while (inP < inEnd) { /* process all of the relocates */
                uint16_t offset = ReadWord();
                if (offset < start || end < offset + (fixType == FBOTH))
                    RecError("Fixup bounds error");
                uint16_t fixLoc = offset - start;
                switch (fixType) { /* relocate to seg current base */
                case FLOW:
                    content[fixLoc] += Low(segBias);
                    break;
                case FHIGH:
                    content[fixLoc] += High(segBias);
                    break;
                case FBOTH:
                    putWord(&content[fixLoc], getWord(&content[fixLoc]) + segBias);
                    break;
                }
                /* add a new reloc fixup */
                AddRelocFixup(HEADREL, fixType, offset + outBias, fixSeg);
            }
        }
        GetRecord(); /* next record */
    }

    EndRecord(); /* finish the content record */
    /* process the extern lists */
    for (word fixupIdx = fixups[HEADEXT].next; fixupIdx; fixupIdx = fixups[fixupIdx].next) {
        InitRecord(R_FIXEXT);                          /* create a extref record */
        WriteByte(Low(fixups[fixupIdx].f.typeAndSeg)); /* set the fix type */
        /* process all of the extref of this fixtype */
        for (word relIdx = fixups[fixupIdx].f.headReloc; relIdx; relIdx = fixups[relIdx].next) {
            if (ExtendRec(4)) /* make sure we have room */
                              /* if (! add the fixtype to the newly created follow on record */
                WriteByte(Low(fixups[fixupIdx].f.typeAndSeg));
            WriteWord(fixups[relIdx].r.symId);  // ext idx
            WriteWord(fixups[relIdx].r.offset); // and offset
        }
        EndRecord(); /* Close() any open record */
    }
    /* now do the relocates */
    for (word fixupIdx = fixups[HEADREL].next; fixupIdx; fixupIdx = fixups[fixupIdx].next) {
        InitRecord(R_FIXSEG);                           /* interseg record */
        WriteByte(High(fixups[fixupIdx].f.typeAndSeg)); /* fill in segment */
        WriteByte(Low(fixups[fixupIdx].f.typeAndSeg));  /* and fixtype */
        /* chase down the references */
        for (word relIdx = fixups[fixupIdx].f.headReloc; relIdx; relIdx = fixups[relIdx].next) {
            if (ExtendRec(2)) { /* two bytes or create follow on record */
                                /* fill in follow on record */
                WriteByte(High(fixups[fixupIdx].f.typeAndSeg)); /* fill in segment */
                WriteByte(Low(fixups[fixupIdx].f.typeAndSeg));  /* and fixtype */
            }
            WriteWord(fixups[relIdx].r.offset); /* set the fill offset */
        }
        EndRecord();
    }
} /* Pass2CONTENT() */

void Pass2LINENO() {
    EmitANCESTOR(); /* make sure we have a valid ancestor record */
    InitRecord(R_LINNUM);
    WriteByte(SelectSeg(ReadByte()));    /* add the seg id info */
    while (inP < inEnd) {                /* while more public definitions */
        WriteWord(ReadWord() + segBias); /* relocate the offset */
        WriteWord(ReadWord());           /* copy the line number */
    }
    EndRecord();
    GetRecord();
} /* Pass2LINENO() */

void Pass2ANCESTOR() {
    Pstrcpy(ReadName(), &modName); /* copy the module name over and mark as valid */
    ancestorNameNotSet = true;     /* note it isn't written yet */
    GetRecord();
} /* Pass2ANCESTOR() */

void Pass2LOCALS() {
    EmitANCESTOR();                   /* emit ancestor if (needed */
    InitRecord(R_LOCDEF);             /* init locals record */
    WriteByte(SelectSeg(ReadByte())); // set segment id

    /* note the code below relies on the source file having records <1025 */
    while (inP < inEnd) { /* while more local definitions */
        WriteWord(ReadWord() + segBias);
        WriteName(ReadName());
        WriteByte(ReadByte()); // the 0
    }
    EndRecord(); /* clean end */
    GetRecord(); /* next record */
} /* Pass2LOCALS() */

/* process pass 2 records */
void Phase2() {
    InitExternsMap();
    if (!(outFp = Fopen(outName, "wb+")))
        IoError(outName, "Create error");
    EmitMODHDR(); /* process the simple records */
    EmitCOMDEF();
    EmitPUBLICS();
    EmitEXTNAMES();
    /* process all files */
    for (objFile = objFileList; objFile; objFile = objFile->next) {
        if (!objFile->publics) { /* publics only file doesn't need more processing*/
            OpenObjFile();       /* Open file */
            /* for each module in the file */
            for (module = objFile->modules; module; module = module->next) {
                if (objFile->isLib) /* is in a library */
                    Position(module->location);
                GetRecord(); /* Load the modhdr */
                if (inType != R_MODHDR)
                    RecError("Phase error");
                InitExternsMap(); /* prepare for processing this module's extdef records */
                while (inType != R_MODEND) { /* run through the whole module */
                    switch (inType) {
                    case R_MODHDR:
                        Pass2MODHDR();
                        break;
                    case R_MODEND:;
                        break;
                    case R_MODDAT:
                        Pass2CONTENT();
                        break;
                    case R_LINNUM:
                        Pass2LINENO();
                        break;
                    case R_MODEOF:
                        FatalError("%s: Unexpected EOF record", objName);
                        break;
                    case R_ANCEST:
                        Pass2ANCESTOR();
                        break;
                    case R_LOCDEF:
                        Pass2LOCALS();
                        break;
                    case R_PUBDEF:
                        GetRecord();
                        break;
                    case R_EXTNAM:
                        Pass2EXTNAMES();
                        break;
                    case R_FIXEXT:
                    case R_FIXLOC:
                    case R_FIXSEG:
                    case R_LIBLOC:
                    case R_LIBNAM:
                    case R_LIBDIC:
                    case R_LIBHDR:
                        BadRecordSeq();
                        break;
                    case R_COMDEF:
                        Pass2COMDEF();
                        break;
                    default:
                        IllegalRelo();
                        break;
                    }
                }
            }
            CloseObjFile();
        }         /* of else */
    }             /* of do while */
    EmitEnding(); /* write final modend and eof record */
    if (fclose(outFp))
        IoError(outName, "Close error");
} /* Phase2() */
