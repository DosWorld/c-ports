/****************************************************************************
 *  plm0f.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

word curState;
bool endSeen;

static void SkipToSemiColon() {
    while (tokenType != T_SEMICOLON)
        Yylex();
    yyAgain = false;
}

static void ExpectSemiColon() {
    if (YylexNotMatch(T_SEMICOLON)) {
        Wr1TokenErrorAt(ERR32); /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
        SkipToSemiColon();
    }
}

static void ErrorSkip() {
    SkipToSemiColon();
    SetYyAgain();
}

static word PopStateWord() {
    if (stateIdx == 0)
        LexFatalError(ERR97); /* COMPILER Error: PARSE STACK UNDERFLOW */
    return stateStack[stateIdx--];
}

// make sure that there will be space for case label info
static void NewLocalLabel() {
    localLabelCnt++;
}

/*
    collect labels (max 9)
    looks for Variable() ':'
    sybmol references stored in stmtLabels array
    count is in stmtLabelCnt
*/
static void ParseStmtLabels() {
    stmtLabelCnt = 0;

    while (1) {
        stmtStartToken  = tokenType;
        stmtStartSymbol = curSym;
        if (tokenType != T_IDENTIFIER)
            return;

        if (YylexMatch(T_COLON)) {
            if (stmtLabelCnt == 9)
                Wr1TokenErrorAt(ERR30); /* LIMIT EXCEEDED: NUMBER OF LABELS ON STATEMENT */
            else
                stmtLabelCnt++;
            stmtLabels[stmtLabelCnt] = curSym;
        } else
            return;
        Yylex();
    }
}

static byte startLexCodeMap[] = { L_STATEMENT, L_SEMICOLON, L_CALL,      L_LINEINFO, L_DISABLE,
                                  L_DO,        L_ENABLE,    L_END,       L_GO,       L_GOTO,
                                  L_HALT,      L_IF,        L_PROCEDURE, L_RETURN };

/* parse start of statement
    startLexCode   startStmtCode
    L_STATEMENT         0           assignment statement
    L_SEMICOLON         1           null statement
    L_CALL              2           call statement
    L_LINEINFO          3           declare statement
    L_DISABLE           4           disable statement (8080 dependent statement)
    L_DO                5           do* statement
    L_ENABLE            6           enable statement (8080 dependent statement)
    L_END               7           end statement
    L_GO                8           goto statement
    L_GOTO              9           goto statement
    L_HALT              10          halt statement (8080 dependent statement)
    L_IF                11          conditional clause
    L_PROCEDURE         12          procedure statement
    L_RETURN            13          return statement
*/

static void ParseStartStmt() {
    bool tmp;

    if (endSeen) {
        endSeen = false;
        return;
    }
    tmp  = XREF;
    XREF = false;
    Yylex();
    XREF = tmp;
    curStmtCnt++;
    if (linfo.stmtCnt == 0) {
        linfo.stmtCnt = curStmtCnt;
        linfo.blkCnt  = curBlkCnt;
    }
    Wr1Byte(L_STMTCNT);
    Wr1Word(curStmtCnt);
    if (trunc) {
        Wr1SyntaxError(ERR86); /* LIMIT EXCEEDED: SOURCE LINE length() */
        trunc = false;
    }
    ParseStmtLabels();
    if (stmtStartToken == T_SEMICOLON) {
        stmtStartCode = S_SEMICOLON; // maps to L_SEMICOLON
        SetYyAgain();
    } else if (stmtStartToken == T_IDENTIFIER)
        stmtStartCode = S_IDENTIFIER; // maps to L_STATEMENT
    else if (stmtStartToken >= T_CALL && stmtStartToken <= T_RETURN)
        stmtStartCode = stmtStartToken - T_CALL + S_CALL; /* maps to S_CALL through S_RETURN */
    else {
        Wr1TokenErrorAt(ERR29); /* ILLEGAL STATEMENT */
        stmtStartCode = S_SEMICOLON;
        ErrorSkip();
    }
    startLexCode = startLexCodeMap[stmtStartCode];
}

static void WrLabelDefs() {
    word i;
    offset_t tmp;

    tmp = curSym; // accessors use curSymbolP so save to restore at end
    if (stmtLabelCnt != 0) {
        for (i = 1; i <= stmtLabelCnt; i++) { // check each label
            curSym = stmtLabels[i];
            FindScopedInfo(curScope);
            if (infoIdx) { /* already seen at this scope */
                if ((info->flag & F_LABEL))
                    Wr1TokenError(ERR33, curSym); /* DUPLICATE LABEL DECLARATION */
                else {
                    Wr1Byte(L_LABELDEF); // log label def to lexical stream
                    Wr1InfoOffset(infoIdx);
                    info->flag |= F_LABEL; // mark as defined
                }
                Wr1XrefUse(); // write xref
            } else {
                CreateInfo(curScope, LABEL_T, curSym); // its new so create
                Wr1XrefDef();                          // write xref
                Wr1Byte(L_LABELDEF);
                Wr1InfoOffset(infoIdx);
                info->flag |= F_LABEL;
            }
        }
        if (curScope == 0x100)
            Wr1Byte(L_MODULE); // record at module level
    }
    curSym = tmp;
} /* WrLabelDefs() */

// check for end module
static bool IsEndOfModule() {
    if (YylexMatch(T_IDENTIFIER)) {
        SetInfo(procInfo[1]);
        if (info->sym != curSym)
            Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    }
    ExpectSemiColon();
    if (afterEOF) {     // END after EOF i.e. recovery mode
        Wr1Byte(L_END); // write closing END to lex stream
        return true;    // no more processing
    } else {
        Yylex();
        if (afterEOF) {            // was final end
            Wr1Byte(L_END);        // write the END to lex stream
            afterEOF = false;      // no need to add recovery tokens
            return true;           // no more processing
        } else {                   // report error and keep going
            Wr1SyntaxError(ERR93); /* MISSING 'DO' FOR 'END' , 'END' IGNORED */
            SetYyAgain();
            return false;
        }
    }
} /* IsEndOfModule() */

// parse <ending> after labels
static void ProcEnding() {
    PopBlock();
    if (YylexMatch(T_IDENTIFIER) && infotab[procIdx].sym != curSym)
        Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */

    SetInfo(procIdx);
    for (int i = GetParamCnt(); i > 0; i--) { // scan any parameters (info after proc info)
        AdvNxtInfo();
        if (!(info->flag & F_LABEL))                    // not declared?
            Wr1TokenError(ERR25, info->sym); /* UNDECLARED PARAMETER */
    }
    doBlkCnt = PopStateWord(); // restore doBlkCnt & curProcInfoP to parent of proc

    procIdx = PopStateWord();
    ExpectSemiColon(); // finish off statement
}

static void PushStateWord(word v) {
    if (stateIdx != 0x63)
        stateStack[++stateIdx] = v;
    else
        LexFatalError(ERR31); /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
}

static void PushStateByte(byte state) {
    PushStateWord(state);
}

static void CreateModuleInfo(offset_t symPtr) {
    curSym = symPtr;
    CreateInfo(0, PROC_T, curSym);
    info->flag |= F_LABEL;
    Wr1XrefDef();
    procIdx                 = infoIdx;
    info->procId = 1;
    procCnt                 = 1;
    procInfo[1]             = infoIdx;
    curScope                = 0x100; /* proc = 1,  do level = 0 */
    Wr1Byte(L_DO);
    Wr1Byte(L_SCOPE);
    Wr1Word(curScope);
    PushBlock(curScope);
}

/* parse <module name> : 'DO' ; */
static void State0() {
    ParseStartStmt();
    PushStateByte(1); // check for module level <unit>
    if (stmtStartCode != S_DO) {
        Wr1SyntaxError(ERR89); /* MISSING 'do' FOR MODULE */
        Lookup((pstr_t *)"\x6MODULE");
        CreateModuleInfo(curSym);
        PushStateByte(19); // next parse <declaration> assuming 'do' provided
    } else {
        if (stmtLabelCnt == 0) {
            Wr1SyntaxError(ERR90); /* MISSING NAME FOR MODULE */
            Lookup((pstr_t *)"\x6MODULE");
            stmtLabelCnt  = 1;
            stmtLabels[1] = curSym;
        } else if (stmtLabelCnt > 1)
            Wr1SyntaxError(ERR18); /* INVALID MULTIPLE LABELS AS MODULE NAMES */
        CreateModuleInfo(stmtLabels[1]);
        ExpectSemiColon();
        PushStateByte(3); // get start statement and parse <declaration>
    }
} /* State0() */

// check for module level <unit>
static void State1() {
    if (stmtStartCode != S_END) {
        haveModuleLevelUnit = true;
        Wr1Byte(L_MODULE);
        PushStateByte(2); // parse module level <unit>...
    } else {
        if (stmtLabelCnt != 0)
            Wr1TokenErrorAt(ERR19); /* INVALID LABEL IN MODULE WITHOUT MAIN PROGRAM */
        if (!IsEndOfModule()) {
            PushStateByte(1);  // check for module level <unit>
            PushStateByte(10); // get the statement start
        }
    }
} /* State1() */

// parse module level <unit>...
static void State2() {
    if (stmtStartCode != S_END) {
        PushStateByte(2);  // parse module level <unit>...
        PushStateByte(10); // get the statement start
        PushStateByte(11); // parse <unit>
    } else {
        WrLabelDefs(); // write any label prefixes
        if (IsEndOfModule())
            Wr1Byte(L_HALT); // write the final halt
        else {
            PushStateByte(2);  // parse module level <unit>...
            PushStateByte(10); // get the statement start
        }
    }
} /* State2() */

// get start statement and parse <declaration>
static void State3() {
    ParseStartStmt();  // get statement start
    PushStateByte(19); // next state parse <declaration>
}

/* parse <declaration> */
static void State19() { /* check for declare or procedure */
    if (stmtStartCode == S_DECLARE) {
        ParseDeclareElementList();
        PushStateByte(3);  // next state get statement start & <declaration>
        ExpectSemiColon(); // but first check for semicolon
    } else if (stmtStartCode == S_PROCEDURE) {
        PushStateByte(3); // after processing procedure check for another declare or procedure
        PushStateByte(4); // next <procedure statement>
    }
}

/* parse start <procedure statement> */
static void State4() {
    if (stmtLabelCnt == 0) {
        Wr1SyntaxError(ERR21); /* MISSING procedure NAME */
        PushStateByte(7);      // skip bad nested proc/do
    } else {
        if (stmtLabelCnt != 1) {
            Wr1TokenErrorAt(ERR22); /* INVALID MULTIPLE LABELS AS PROCEDURE NAMES */
            stmtLabelCnt = 1;
        }
        PushStateWord(procIdx);  // save current procInfoP
        PushStateWord(doBlkCnt); // and block count restored in PraseEnding
        ProcProcStmt();          // parse optional parameters, return type and attributes
        ExpectSemiColon();       // finish with semicolon
        SetInfo(procIdx);        // test if external proc
        if ((info->flag & F_EXTERNAL))
            PushStateByte(5); // next <declaration> <ending>
        else {
            PushStateByte(6);  // finish with parse <ending>
            PushStateByte(21); // parse non null <unit>
            PushStateByte(3);  // next new start & <declaration>
        }
    }
}

// parse <declaration> <ending> for external procedure
static void State5() {
    ParseStartStmt();
    if (stmtStartCode == S_END) { // cannot label END in external proc def
        if (stmtLabelCnt != 0) {
            Wr1TokenErrorAt(ERR23); /* INVALID LABELLED END IN EXTERNAL PROCEDURE */
            stmtLabelCnt = 0;
        }
        ProcEnding();
    } else {
        PushStateByte(5); // repeat until final end
        if (stmtStartCode == S_DECLARE) {
            ParseDeclareElementList(); // get the parameter declarations
            ExpectSemiColon();         // finish
        } else {
            Wr1TokenErrorAt(ERR24); /* INVALID STATEMENT IN external procedure */
            if (stmtStartCode == S_PROCEDURE || stmtStartCode == S_DO)
                PushStateByte(7); // skip bad nested proc/do
            else
                SkipToSemiColon(); // else skip to end of this statement
        }
    }
}

// check for non null <unit>
static void State21() {
    if (stmtStartCode == S_END)
        Wr1TokenErrorAt(ERR174); /* INVALID NULL procedure */
    else
        PushStateByte(9); // parse <unit>
}

// parse <ending> for defined procedure
static void State6() {
    WrLabelDefs(); // write any labels on the END to the lex stream
    ProcEnding();
    Wr1Byte(L_END); // add end to the lexical stream
}

/* states 7 & 8 skip to end of block, handling nested blocks */
static void State7() {
    SkipToSemiColon(); // skip this statement
    PushStateByte(8);  // skip to end of proc / do
}

static void State8() {
    ParseStartStmt();
    if (stmtStartCode == S_PROCEDURE || /* nested block */
        stmtStartCode == S_DO) {
        PushStateByte(8); // skip rest of this proc/do
        PushStateByte(7); // after skipping nested proc/do
    } else {
        SkipToSemiColon();          /* skip to end of statement */
        if (stmtStartCode != S_END) /* if ! an end then go again */
            PushStateByte(8);       // keep skipping
    }
}

// parse <unit>...
static void State9() {
    if (stmtStartCode != S_END) {
        PushStateByte(9);  // repeat parsing statement after this statement is parsed
        PushStateByte(10); // get start statement after processing
        PushStateByte(11); // parse <unit>
    }
}

// get the statement start
static void State10() {
    ParseStartStmt();
}

// parse <unit>
static void State11() {
    if (stmtStartCode == S_DECLARE || stmtStartCode == S_PROCEDURE || stmtStartCode == S_END) {
        Wr1TokenErrorAt(ERR26); /* INVALID DECLARATION, STATEMENT OUT OF PLACE */
        PushStateByte(20);      // parse unexpected <declaration>
    } else if (stmtStartCode == S_DO)
        PushStateByte(12); // parse rest of <do block>
    else if (stmtStartCode == S_IF)
        PushStateByte(16);                       // parse rest of <conditional clause>
    else {                                       // <label definition> <unit>
        WrLabelDefs();                           // write labels
        Wr1Byte(startLexCode);                   // and first lex token of <basic statement>
        if (stmtStartCode != S_SEMICOLON) {      /* Semicolon() */
            if (stmtStartCode == S_IDENTIFIER) { /* Variable() */
                Wr1Byte(L_IDENTIFIER);           // write var for <assignment statement>
                Wr1Word(stmtStartSymbol);
            }
            ParseExpresion(T_SEMICOLON);
        }
        ExpectSemiColon();
    }
}

// parse unexpected <declaration>
static void State20() {
    if (stmtStartCode == S_DECLARE) {
        ParseDeclareElementList();
        ExpectSemiColon();
        PushStateByte(20); // parse unexpected <declaration>
        PushStateByte(10); // get start statement
    } else if (stmtStartCode == S_PROCEDURE) {
        PushStateByte(20); // parse unexpected <declaration>
        PushStateByte(10); // get start statement
        PushStateByte(4);  // parse start <procedure statement>
    } else if (stmtStartCode == S_END)
        endSeen = true;
    else
        PushStateByte(11); // parse <unit>
}

// parse <do block> // do already seen
static void State12() {
    WrLabelDefs(); // write any prefix labels
    if (stmtLabelCnt != 0)
        PushStateWord(stmtLabels[stmtLabelCnt]); // push the address of the last label
    else
        PushStateWord(0); // or null if none
    if (YylexMatch(T_IDENTIFIER)) {
        Wr1Byte(L_DOLOOP); // convert <iterative do statement> to lex format
        Wr1LexToken();     // the <index variable>
        ParseExpresion(T_SEMICOLON);
        PushStateByte(13);            // parse <ending>  labels already collected
        PushStateByte(9);             // parse <unit>...
        PushStateByte(10);            // get start statement
    } else if (YylexMatch(T_WHILE)) { // <do-while block>
        Wr1Byte(L_WHILE);             // convert <do-while statement> to lex format
        ParseExpresion(T_SEMICOLON);
        PushStateByte(13);           // parse <ending>  labels already collected
        PushStateByte(9);            // parse <unit>...
        PushStateByte(10);           // get start statement
    } else if (YylexMatch(T_CASE)) { // <do-case block>
        Wr1Byte(L_CASE);             // convert <do-case statement> to lex format
        ParseExpresion(T_SEMICOLON);
        NewLocalLabel(); // make sure later passes have space for the end of case target
        PushStateWord(localLabelCnt); // push the index of end of case target for later
        PushStateByte(14);            // parse case <unit>... <ending>
    } else {
        Wr1Byte(L_DO);     // convert <simple do statement> to lex format
        PushStateByte(13); // parse <ending>  labels already collected
        PushStateByte(9);  // parse <unit>...
        PushStateByte(3);  // get start statement and parse <declaration>
    }
    ExpectSemiColon(); /* we should now see a semicolon at end of do ... statement */
    if (doBlkCnt >= 255)
        Wr1SyntaxError(ERR27); /* LIMIT EXCEEDED: NUMBER OF do BLOCKS */
    else
        doBlkCnt++;

    curScope = (High(curScope) << 8) + doBlkCnt;
    PushBlock(curScope); // push new scope
    Wr1Byte(L_SCOPE);    // and write to lex stream
    Wr1Word(curScope);
}

// parse <ending>  labels already collected
static void State13() {
    offset_t labelPtr;

    WrLabelDefs();                // write labels
    PopBlock();                   // restore scope to parent block
    labelPtr = PopStateWord();      // get the do block label
    if (YylexMatch(T_IDENTIFIER)) // if we have "end identifier" do they match
        if (curSym != labelPtr)
            Wr1TokenErrorAt(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
    Wr1Byte(L_END);                 // write lex END token
    ExpectSemiColon();
}

// parse case <unit>... <ending>
static void State14() { /* process CASE statements */
    offset_t labelPtr;
    word stateWord;

    ParseStartStmt();
    if (stmtStartCode != S_END) { // not at end of do case
        NewLocalLabel();          // reserve space for this case target
        Wr1Byte(L_CASELABEL);     // and add to the lex stream
        Wr1Word(localLabelCnt);
        PushStateByte(15); // write jmp to end of case & re-enter state14
        PushStateByte(11); // parse <unit>
    } else {
        stateWord = PopStateWord(); /* get the label index for end of case target */
        WrLabelDefs();            // write any labels associated with the end
        if (stmtLabelCnt != 0) {  // write jmp to end target if there are labels
            Wr1Byte(L_JMP);
            Wr1Word(stateWord);
        }
        PopBlock();              // restore scope
        labelPtr = PopStateWord(); // get any prefix label to do
        Wr1Byte(L_END);          // write end
        Wr1Byte(L_LOCALLABEL);   // & target label
        Wr1Word(stateWord);
        /* check end label if (present */
        if (YylexMatch(T_IDENTIFIER))
            if (curSym != labelPtr)
                Wr1SyntaxError(ERR20); /* MISMATCHED IDENTIFIER AT END OF BLOCK */
        ExpectSemiColon();
    }
}

// write jmp to end of case and re-enter state14
static void State15() {
    word stateWord = PopStateWord(); // get index of end of case label
    Wr1Byte(L_JMP);           // write jmp & target
    Wr1Word(stateWord);
    PushStateWord(stateWord); // resave
    PushStateByte(14);        // parse case <unit>... <ending>
}

// parse <conditional clause> if seen
static void State16() {
    WrLabelDefs(); // write any prefix labels
    Wr1Byte(L_IF); // convert <if condition> to lex stream
    ParseExpresion(T_THEN);
    if (YylexNotMatch(T_THEN)) { // oops no THEN
        Wr1TokenErrorAt(ERR28);  /* MISSING 'then' */
        Wr1Byte(L_JMPFALSE);     // write jmpfalse 0 to lex stream
        Wr1Word(0);
    } else {
        NewLocalLabel();              // end of <true unit> target label
        PushStateWord(localLabelCnt); // save on stack
        Wr1Byte(L_JMPFALSE);          // write jmp for false to lex stream
        Wr1Word(localLabelCnt);
        PushStateByte(17); // parse optional ELSE <false element>
        PushStateByte(11); // parse <unit> (<true unit>)
        PushStateByte(10); // get the statement start
    }
}

// parse optional ELSE <false element>
static void State17() { /* process optional else */

    word endTrueLabel = PopStateWord(); /* labelref for end of <true unit> */
    Yylex();
    bool savToWrite             = lineInfoToWrite; /* supress line info for labeldefs etc */
    lineInfoToWrite = false;
    if (tokenType == T_ELSE) { // ELSE <false element>
        NewLocalLabel();       // reserve space for end of <false element>
        Wr1Byte(L_JMP);        // write jmp around <false element> to lex stream
        Wr1Word(localLabelCnt);
        PushStateWord(localLabelCnt); /* save labelref for end of <false element> */
        PushStateByte(18);            // parse end of <false element>
        PushStateByte(11);            // parse <unit> (<false element>)
        PushStateByte(10);            // get the statement start
    } else
        SetYyAgain();

    Wr1Byte(L_LOCALLABEL); /* emit label for end <true unit> */
    Wr1Word(endTrueLabel);
    lineInfoToWrite = savToWrite; // restore status of lineInfoToWrite
}

// parse end of <false element>
static void State18() { /* end of else */

    bool savToWrite             = lineInfoToWrite; /* supress line info for labeldefs */

    lineInfoToWrite = false;
    word falseLabel = PopStateWord(); /* labelref for end <false element> */
    Wr1Byte(L_LOCALLABEL);    /* emit label */
    Wr1Word(falseLabel);
    lineInfoToWrite = savToWrite; // restore status of lineInfoTo
}

void ParseProgram() // core state machine to parse program
{
    stateIdx = 0;
    endSeen  = false;
    PushStateByte(0); // start with parse <module name> : 'DO' ;
    while (stateIdx != 0) {
        curState = stateStack[stateIdx]; // pop state
        stateIdx--;
        switch (curState) {
        case 0:
            State0();
            break; // parse <module name> : 'DO' ;
        case 1:
            State1();
            break; // check for module level <unit>
        case 2:
            State2();
            break; // parse module level <unit>...
        case 3:
            State3();
            break; // get start statement and parse <declaration>
        case 4:
            State4();
            break; // parse start <procedure statement>
        case 5:
            State5();
            break; // parse <declaration> <ending> for external procedure
        case 6:
            State6();
            break; // parse <ending> for defined procedure
        case 7:
            State7();
            break; // states 7 & 8 skip to end of block, handling nested blocks
        case 8:
            State8();
            break; // states 7 & 8 skip to end of block, handling nested blocks
        case 9:
            State9();
            break; // parse <unit>...
        case 10:
            State10();
            break; // get the statement start
        case 11:
            State11();
            break; // parse <unit>
        case 12:
            State12();
            break; // parse <do block> // do seen
        case 13:
            State13();
            break; // parse <ending>  labels already collected
        case 14:
            State14();
            break; // parse case <unit>... <ending>
        case 15:
            State15();
            break; // write jmp to end of case and re-enter state14
        case 16:
            State16();
            break; // parse <conditional clause> if seen
        case 17:
            State17();
            break; // parse optional ELSE <false element>
        case 18:
            State18();
            break; // parse end of <false element>
        case 19:
            State19();
            break; // parse <declaration>
        case 20:
            State20();
            break; // parse unexpected <declaration>
        case 21:
            State21();
            break; // check for non null <unit>
        }
    }
}