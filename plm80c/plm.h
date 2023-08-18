/****************************************************************************
 *  plm.h: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "vfile.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#define trunc    _trunc
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif


#define VERSION "V4.0"

typedef unsigned char byte;
typedef unsigned short word;
typedef byte *pointer;
typedef word *wpointer;
typedef word offset_t;
typedef byte leword[2]; // to support writing of intel OMF words
typedef uint16_t index_t;

#define High(n)   ((n) >> 8)
#define Low(n)    ((n)&0xff)
#define Ror(v, n) (((v) >> n) | ((v) << (8 - n)))
#define Rol(v, n)	(((v) << n) | (((v) >> (8 - n)))
#define Move(s, d, c) memcpy(d, s, c)
#define Length(str)   (sizeof(str) - 1)


#define SetInfo(v)    info = &infotab[infoIdx = v]

#define TAB           9
#define CR            '\r'
#define LF            '\n'
#define QUOTE         '\''
#define ISISEOF       0x81

/* flags */
enum {
    F_PUBLIC    = (1 << 0),
    F_EXTERNAL  = (1 << 1),
    F_BASED     = (1 << 2),
    F_INITIAL   = (1 << 3),
    F_REENTRANT = (1 << 4),
    F_DATA      = (1 << 5),
    F_INTERRUPT = (1 << 6),
    F_AT        = (1 << 7),
    F_ARRAY     = (1 << 8),
    F_STARDIM   = (1 << 9),
    F_PARAMETER = (1 << 10),
    F_MEMBER    = (1 << 11),
    F_LABEL     = (1 << 12),
    F_AUTOMATIC = (1 << 13),
    F_PACKED    = (1 << 14),
    F_ABSOLUTE  = (1 << 15),
    F_MEMORY    = (1 << 16),
    F_DECLARED  = (1 << 17),
    F_DEFINED   = (1 << 18),
    F_MODGOTO   = (1 << 19)
};

/* token info types */
// 10, 12, 18, 18, 18, 22, 11, 10, 8, 9
enum {
    LIT_T     = 0,
    LABEL_T   = 1,
    BYTE_T    = 2,
    ADDRESS_T = 3,
    STRUCT_T  = 4,
    PROC_T    = 5,
    BUILTIN_T = 6,
    MACRO_T   = 7,
    UNK_T     = 8,
    CONDVAR_T = 9
};

/* Lex tokens */
enum {
    L_LINEINFO    = 0,
    L_SYNTAXERROR = 1,
    L_TOKENERROR  = 2,
    L_LIST        = 3,
    L_NOLIST      = 4,
    L_CODE        = 5,
    L_NOCODE      = 6,
    L_EJECT       = 7,
    L_INCLUDE     = 8,
    L_STMTCNT     = 9,
    L_LABELDEF    = 10,
    L_LOCALLABEL  = 11,
    L_JMP         = 12,
    L_JMPFALSE    = 13,
    L_PROCEDURE   = 14,
    L_SCOPE       = 15,
    L_END         = 16,
    L_DO          = 17,
    L_DOLOOP      = 18,
    L_WHILE       = 19,
    L_CASE        = 20,
    L_CASELABEL   = 21,
    L_IF          = 22,
    L_STATEMENT   = 23,
    L_CALL        = 24,
    L_RETURN      = 25,
    L_GO          = 26,
    L_GOTO        = 27,
    L_SEMICOLON   = 28,
    L_ENABLE      = 29,
    L_DISABLE     = 30,
    L_HALT        = 31,
    L_EOF         = 32,
    L_AT          = 33,
    L_INITIAL     = 34,
    L_DATA        = 35,
    L_IDENTIFIER  = 36,
    L_NUMBER      = 37,
    L_STRING      = 38,
    L_PLUSSIGN    = 39,
    L_MINUSSIGN   = 40,
    L_PLUS        = 41,
    L_MINUS       = 42,
    L_STAR        = 43,
    L_SLASH       = 44,
    L_MOD         = 45,
    L_COLONEQUALS = 46,
    L_AND         = 47,
    L_OR          = 48,
    L_XOR         = 49,
    L_NOT         = 50,
    L_LT          = 51,
    L_LE          = 52,
    L_EQ          = 53,
    L_NE          = 54,
    L_GE          = 55,
    L_GT          = 56,
    L_COMMA       = 57,
    L_LPAREN      = 58,
    L_RPAREN      = 59,
    L_PERIOD      = 60,
    L_TO          = 61,
    L_BY          = 62,
    L_INVALID     = 63,
    L_MODULE      = 64,
    L_XREFUSE     = 65,
    L_XREFDEF     = 66,
    L_EXTERNAL    = 67
};

/* character classes */
enum {
    CC_BINDIGIT  = 0,
    CC_OCTDIGIT  = 1,
    CC_DECDIGIT  = 2,
    CC_HEXCHAR   = 3,
    CC_ALPHA     = 4,
    CC_PLUS      = 5,
    CC_MINUS     = 6,
    CC_STAR      = 7,
    CC_SLASH     = 8,
    CC_LPAREN    = 9,
    CC_RPAREN    = 10,
    CC_COMMA     = 11,
    CC_COLON     = 12,
    CC_SEMICOLON = 13,
    CC_QUOTE     = 14,
    CC_PERIOD    = 15,
    CC_EQUALS    = 16,
    CC_LESS      = 17,
    CC_GREATER   = 18,
    CC_WSPACE    = 19,
    CC_DOLLAR    = 20,
    CC_INVALID   = 21,
    CC_NONPRINT  = 22,
    CC_NEWLINE   = 23
};

/* intermediate tokens */
enum {
    T_IDENTIFIER   = 0,
    T_NUMBER       = 1,
    T_STRING       = 2,
    T_PLUSSIGN     = 3,
    T_MINUSSIGN    = 4,
    T_STAR         = 5,
    T_SLASH        = 6,
    T_MOD          = 7,
    T_PLUS         = 8,
    T_MINUS        = 9,
    T_AND          = 10,
    T_OR           = 11,
    T_XOR          = 12,
    T_NOT          = 13,
    T_LT           = 15,
    T_LE           = 16,
    T_EQ           = 17,
    T_NE           = 18,
    T_GE           = 19,
    T_GT           = 20,
    T_COLON_EQUALS = 21,
    T_COLON        = 22,
    T_SEMICOLON    = 23,
    T_PERIOD       = 24,
    T_LPAREN       = 25,
    T_RPAREN       = 26,
    T_COMMA        = 27,
    T_CALL         = 28,
    T_DECLARE      = 29,
    T_DISABLE      = 30,
    T_DO           = 31,
    T_ENABLE       = 32,
    T_END          = 33,
    T_GO           = 34,
    T_GOTO         = 35,
    T_HALT         = 36,
    T_IF           = 37,
    T_PROCEDURE    = 38,
    T_RETURN       = 39,
    T_ADDRESS      = 40,
    T_AT           = 41,
    T_BASED        = 42,
    T_BYTE         = 43,
    T_DATA         = 44,
    T_EXTERNAL     = 45,
    T_INITIAL      = 46,
    T_INTERRUPT    = 47,
    T_LABEL        = 48,
    T_LITERALLY    = 49,
    T_PUBLIC       = 50,
    T_REENTRANT    = 51,
    T_STRUCTURE    = 52,
    T_BY           = 53,
    T_CASE         = 54,
    T_ELSE         = 55,
    T_EOF          = 56,
    T_THEN         = 57,
    T_TO           = 58,
    T_WHILE        = 59
};

/* start statement codes */
enum {
    S_IDENTIFIER = 0,
    S_SEMICOLON,
    S_CALL,
    S_DECLARE,
    S_DISABLE,
    S_DO,
    S_ENABLE,
    S_END,
    S_GO,
    S_GOTO,
    S_HALT,
    S_IF,
    S_PROCEDURE,
    S_RETURN
};

/* T2 codes */
enum {
    T2_LT          = 0,
    T2_LE          = 1,
    T2_NE          = 2,
    T2_EQ          = 3,
    T2_GE          = 4,
    T2_GT          = 5,
    T2_ROL         = 6,
    T2_ROR         = 7,
    T2_SCL         = 8,
    T2_SCR         = 9,
    T2_SHL         = 10,
    T2_SHR         = 11,
    T2_JMPFALSE    = 12,
    T2_DOUBLE      = 18,
    T2_PLUSSIGN    = 19,
    T2_MINUSSIGN   = 20,
    T2_STAR        = 21,
    T2_SLASH       = 22,
    T2_MOD         = 23,
    T2_AND         = 24,
    T2_OR          = 25,
    T2_XOR         = 26,
    T2_BASED       = 27,
    T2_BYTEINDEX   = 28,
    T2_WORDINDEX   = 29,
    T2_MEMBER      = 30,
    T2_UNARYMINUS  = 31,
    T2_NOT         = 32,
    T2_LOW         = 33,
    T2_HIGH        = 34,
    T2_ADDRESSOF   = 35,
    T2_PLUS        = 36,
    T2_MINUS       = 37,
    T2_44          = 44,
    T2_51          = 51,
    T2_56          = 56,
    T2_TIME        = 57,
    T2_STKBARG     = 58,
    T2_STKWARG     = 59,
    T2_DEC         = 60,
    T2_COLONEQUALS = 61,
    T2_OUTPUT      = 62,
    T2_63          = 63,
    T2_STKARG      = 64,
    T2_65          = 65,
    T2_MOVE        = 69,
    T2_RETURNBYTE  = 71,
    T2_RETURNWORD  = 72,
    T2_RETURN      = 73,
    T2_ADDW        = 130,
    T2_BEGMOVE     = 131,
    T2_CALL        = 132,
    T2_CALLVAR     = 133,
    T2_PROCEDURE   = 135,
    T2_LOCALLABEL  = 136,
    T2_CASELABEL   = 137,
    T2_LABELDEF    = 138,
    T2_INPUT       = 139,
    T2_GOTO        = 140,
    T2_JMP         = 141,
    T2_JNC         = 142,
    T2_JNZ         = 143,
    T2_SIGN        = 144,
    T2_ZERO        = 145,
    T2_PARITY      = 146,
    T2_CARRY       = 147,
    T2_DISABLE     = 148,
    T2_ENABLE      = 149,
    T2_HALT        = 150,
    T2_STMTCNT     = 151,
    T2_LINEINFO    = 152,
    T2_MODULE      = 153,
    T2_SYNTAXERROR = 154,
    T2_TOKENERROR  = 155,
    T2_EOF         = 156,
    T2_LIST        = 157,
    T2_NOLIST      = 158,
    T2_CODE        = 159,
    T2_NOCODE      = 160,
    T2_EJECT       = 161,
    T2_INCLUDE     = 162,
    T2_ERROR       = 163,
    T2_IDENTIFIER  = 172,
    T2_NUMBER      = 173,
    T2_BIGNUMBER   = 174,
    T2_STACKPTR    = 181,
    T2_SEMICOLON   = 182,
    T2_OPTBACKREF  = 183,
    T2_CASE        = 184,
    T2_ENDCASE     = 185,
    T2_ENDPROC     = 186,
    T2_LENGTH      = 187,
    T2_LAST        = 188,
    T2_SIZE        = 189,
    T2_BEGCALL     = 190,
    T2_254         = 254
};

/* ICodes */
enum {
    I_STRING      = 0,
    I_IDENTIFIER  = 1,
    I_NUMBER      = 2,
    I_PLUSSIGN    = 3,
    I_MINUSSIGN   = 4,
    I_PLUS        = 5,
    I_MINUS       = 6,
    I_STAR        = 7,
    I_SLASH       = 8,
    I_MOD         = 9,
    I_AND         = 10,
    I_OR          = 11,
    I_XOR         = 12,
    I_NOT         = 13,
    I_LT          = 14,
    I_LE          = 15,
    I_EQ          = 16,
    I_NE          = 17,
    I_GE          = 18,
    I_GT          = 19,
    I_ADDRESSOF   = 20,
    I_UNARYMINUS  = 21,
    I_STACKPTR    = 22,
    I_INPUT       = 23,
    I_OUTPUT      = 24,
    I_CALL        = 25,
    I_CALLVAR     = 26,
    I_BYTEINDEX   = 27,
    I_WORDINDEX   = 28,
    I_COLONEQUALS = 29,
    I_MEMBER      = 30,
    I_BASED       = 31,
    I_CARRY       = 32,
    I_DEC         = 33,
    I_DOUBLE      = 34,
    I_HIGH        = 35,
    I_LAST        = 36,
    I_LENGTH      = 37,
    I_LOW         = 38,
    I_MOVE        = 39,
    I_PARITY      = 40,
    I_ROL         = 41,
    I_ROR         = 42,
    I_SCL         = 43,
    I_SCR         = 44,
    I_SHL         = 45,
    I_SHR         = 46,
    I_SIGN        = 47,
    I_SIZE        = 48,
    I_TIME        = 49,
    I_ZERO        = 50
};

/* AT Icodes */
enum {
    ATI_AHDR   = 0,
    ATI_DHDR   = 1,
    ATI_2      = 2,
    ATI_STRING = 3,
    ATI_DATA   = 4,
    ATI_END    = 5,
    ATI_EOF    = 6
};

/* CF codes */
enum {
    CF_3       = 3,
    CF_POP     = 4,
    CF_XTHL    = 5,
    CF_6       = 6,
    CF_7       = 7,
    CF_XCHG    = 14,
    CF_MOVRPM  = 16,
    CF_MOVLRM  = 18,
    CF_MOVMRPR = 19,
    CF_MOVMLR  = 20,
    CF_DW      = 21,
    CF_SPHL    = 22,
    CF_PUSH    = 23,
    CF_INX     = 24,
    CF_DCX     = 25,
    CF_DCXH    = 26,
    CF_RET     = 27,
    CF_SHLD    = 59,
    CF_STA     = 60,
    CF_MOVMRP  = 62,
    CF_67      = 67,
    CF_68      = 68,
    CF_DELAY   = 97,
    CF_MOVE_HL = 103,
    CF_MOVLRHR = 110,
    CF_MOVHRLR = 113,
    CF_MOVHRM  = 114,
    CF_MOVMHR  = 115,
    CF_INXSP   = 116,
    CF_DCXSP   = 117,
    CF_JMPTRUE = 118,
    CF_134     = 134,
    CF_EI      = 149,
    CF_171     = 171,
    CF_174     = 174
};

enum { DO_PROC = 0, DO_LOOP = 1, DO_WHILE = 2, DO_CASE = 3 };

/* Error codes */
#define ERR1   1   /* INVALID PL/M-80 CHARACTER */
#define ERR2   2   /* UNPRINTABLE ASCII CHARACTER */
#define ERR3   3   /* IDENTIFIER, STRING, OR NUMBER TOO LONG, TRUNCATED */
#define ERR4   4   /* ILLEGAL NUMERIC CONSTANT TYPE */
#define ERR5   5   /* INVALID CHARACTER IN NUMERIC CONSTANT */
#define ERR6   6   /* ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION */
#define ERR7   7   /* LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY */
#define ERR8   8   /* INVALID CONTROL FORMAT */
#define ERR9   9   /* INVALID CONTROL */
#define ERR10  10  /* ILLEGAL USE OF PRIMARY CONTROL AFTER NON-CONTROL LINE */
#define ERR11  11  /* MISSING CONTROL PARAMETER */
#define ERR12  12  /* INVALID CONTROL PARAMETER */
#define ERR13  13  /* LIMIT EXCEEDED: INCLUDE NESTING */
#define ERR14  14  /* INVALID CONTROL FORMAT, INCLUDE NOT LAST CONTROL */
#define ERR15  15  /* MISSING INCLUDE CONTROL PARAMETER */
#define ERR16  16  /* ILLEGAL PRINT CONTROL */
#define ERR17  17  /* INVALID PATH-NAME */
#define ERR18  18  /* INVALID MULTIPLE LABELS AS MODULE NAMES */
#define ERR19  19  /* INVALID LABEL IN MODULE WITHOUT MAIN PROGRAM */
#define ERR20  20  /* MISMATCHED IDENTIFIER AT END OF BLOCK */
#define ERR21  21  /* MISSING PROCEDURE NAME */
#define ERR22  22  /* INVALID MULTIPLE LABELS AS PROCEDURE NAMES */
#define ERR23  23  /* INVALID LABELLED END IN EXTERNAL PROCEDURE */
#define ERR24  24  /* INVALID STATEMENT IN EXTERNAL PROCEDURE */
#define ERR25  25  /* UNDECLARED PARAMETER */
#define ERR26  26  /* INVALID DECLARATION, STATEMENT OUT OF PLACE */
#define ERR27  27  /* LIMIT EXCEEDED: NUMBER OF DO BLOCKS */
#define ERR28  28  /* MISSING 'THEN' */
#define ERR29  29  /* ILLEGAL STATEMENT */
#define ERR30  30  /* LIMIT EXCEEDED: NUMBER OF LABELS ON STATEMENT */
#define ERR31  31  /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR32  32  /* INVALID SYNTAX, TEXT IGNORED UNTIL ';' */
#define ERR33  33  /* DUPLICATE LABEL DECLARATION */
#define ERR34  34  /* DUPLICATE PROCEDURE DECLARATION */
#define ERR35  35  /* LIMIT EXCEEDED: NUMBER OF PROCEDURES */
#define ERR36  36  /* MISSING PARAMETER */
#define ERR37  37  /* MISSING ') ' AT END OF PARAMETER LIST */
#define ERR38  38  /* DUPLICATE PARAMETER NAME */
#define ERR39  39  /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
#define ERR40  40  /* DUPLICATE ATTRIBUTE */
#define ERR41  41  /* CONFLICTING ATTRIBUTE */
#define ERR42  42  /* INVALID INTERRUPT VALUE */
#define ERR43  43  /* MISSING INTERRUPT VALUE */
#define ERR44  44  /* ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH PARAMETERS */
#define ERR45  45  /* ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH TYPED PROCEDURE */
#define ERR46  46  /* ILLEGAL USE OF LABEL */
#define ERR47  47  /* MISSING ') ' AT END OF FACTORED DECLARATION */
#define ERR48  48  /* ILLEGAL DECLARATION STATEMENT SYNTAX */
#define ERR49  49  /* LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE */
#define ERR50  50  /* INVALID ATTRIBUTES FOR BASE */
#define ERR51  51  /* INVALID BASE, SUBSCRIPTING ILLEGAL */
#define ERR52  52  /* INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY OF STRUCTURES */
#define ERR53  53  /* INVALID STRUCTURE MEMBER IN BASE */
#define ERR54  54  /* UNDECLARED BASE */
#define ERR55  55  /* UNDECLARED STRUCTURE MEMBER IN BASE */
#define ERR56  56  /* INVALID MACRO TEXT, NOT A STRING CONSTANT */
#define ERR57  57  /* INVALID DIMENSION, ZERO ILLEGAL */
#define ERR58  58  /* INVALID STAR DIMENSION IN FACTORED DECLARATION */
#define ERR59  59  /* ILLEGAL DIMENSION ATTRIBUTE */
#define ERR60  60  /* MISSING ') ' AT END OF DIMENSION */
#define ERR61  61  /* MISSING TYPE */
#define ERR62  62  /* INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL' */
#define ERR63  63  /* INVALID DIMENSION WITH THIS ATTRIBUTE */
#define ERR64  64  /* MISSING STRUCTURE MEMBERS */
#define ERR65  65  /* MISSING ') ' AT END OF STRUCTURE MEMBER LIST */
#define ERR66  66  /* INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER */
#define ERR67  67  /* DUPLICATE STRUCTURE MEMBER NAME */
#define ERR68  68  /* LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS */
#define ERR69  69  /* INVALID STAR DIMENSION WITH STRUCTURE MEMBER */
#define ERR70  70  /* INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL */
#define ERR71  71  /* INVALID MEMBER TYPE, 'LABEL' ILLEGAL */
#define ERR72  72  /* MISSING TYPE FOR STRUCTURE MEMBER */
#define ERR73  73  /* INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL */
#define ERR74  74  /* INVALID STAR DIMENSION, NOT WITH 'DATA' OR 'INITIAL' */
#define ERR75  75  /* MISSING ARGUMENT OF 'AT' , 'DATA' , OR 'INITIAL' */
#define ERR76  76  /* CONFLICTING ATTRIBUTE WITH PARAMETER */
#define ERR77  77  /* INVALID PARAMETER DECLARATION, BASE ILLEGAL */
#define ERR78  78  /* DUPLICATE DECLARATION */
#define ERR79  79  /* ILLEGAL PARAMETER TYPE, NOT BYTE OR ADDRESS */
#define ERR80  80  /* INVALID DECLARATION, LABEL MAY NOT BE BASED */
#define ERR81  81  /* CONFLICTING ATTRIBUTE WITH 'BASE' */
#define ERR82  82  /* INVALID SYNTAX, MISMATCHED '(' */
#define ERR83  83  /* LIMIT EXCEEDED: DYNAMIC STORAGE */
#define ERR84  84  /* LIMIT EXCEEDED: BLOCK NESTING */
#define ERR85  85  /* LONG STRING ASSUMED CLOSED AT NEXT SEMICOLON OR QUOTE */
#define ERR86  86  /* LIMIT EXCEEDED: SOURCE LINE LENGTH */
#define ERR87  87  /* MISSING 'END' , END-OF-FILE ENCOUNTERED */
#define ERR88  88  /* INVALID PROCEDURE NESTING, ILLEGAL IN REENTRANT PROCEDURE */
#define ERR89  89  /* MISSING 'DO' FOR MODULE */
#define ERR90  90  /* MISSING NAME FOR MODULE */
#define ERR91  91  /* ILLEGAL PAGELENGTH CONTROL VALUE */
#define ERR92  92  /* ILLEGAL PAGEWIDTH CONTROL VALUE */
#define ERR93  93  /* MISSING 'DO' FOR 'END' , 'END' IGNORED */
#define ERR94  94  /* ILLEGAL CONSTANT, VALUE > 65535 */
#define ERR95  95  /* ILLEGAL RESPECIFICATION OF PRIMARY CONTROL IGNORED */
#define ERR96  96  /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
#define ERR97  97  /* COMPILER ERROR: PARSE STACK UNDERFLOW */
#define ERR98  98  /* INCLUDE FILE IS NOT A DISKETTE FILE */
#define ERR99  99  /* ?? unused */
#define ERR100 100 /* INVALID STRING CONSTANT IN Expression */
#define ERR101 101 /* INVALID ITEM FOLLOWS DOT OPERATOR */
#define ERR102 102 /* MISSING PRIMARY OPERAND */
#define ERR103 103 /* MISSING ') ' AT END OF SUBEXPRESSION */
#define ERR104 104 /* ILLEGAL PROCEDURE INVOCATION WITH DOT OPERATOR */
#define ERR105 105 /* UNDECLARED IDENTIFIER */
#define ERR106 106 /* INVALID INPUT/OUTPUT PORT NUMBER */
#define ERR107 107 /* ILLEGAL INPUT/OUTPUT PORT NUMBER, NOT NUMERIC CONSTANT */
#define ERR108 108 /* MISSING ') ' AFTER INPUT/OUTPUT PORT NUMBER */
#define ERR109 109 /* MISSING INPUT/OUTPUT PORT NUMBER */
#define ERR110 110 /* INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE */
#define ERR111 111 /* INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER */
#define ERR112 112 /* UNDECLARED STRUCTURE MEMBER */
#define ERR113 113 /* MISSING ') ' AT END OF ARGUMENT LIST */
#define ERR114 114 /* INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL */
#define ERR115 115 /* MISSING ') ' AT END OF SUBSCRIPT */
#define ERR116 116 /* MISSING '=' IN ASSIGNMENT STATEMENT */
#define ERR117 117 /* MISSING PROCEDURE NAME IN CALL STATEMENT */
#define ERR118 118 /* INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR */
#define ERR119 119 /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR120 120 /* LIMIT EXCEEDED: Expression TOO COMPLEX */
#define ERR121 121 /* LIMIT EXCEEDED: Expression TOO COMPLEX */
#define ERR122 122 /* LIMIT EXCEEDED: PROGRAM TOO COMPLEX */
#define ERR123 123 /* INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL */
#define ERR124 124 /* MISSING ARGUMENTS FOR BUILT-IN PROCEDURE */
#define ERR125 125 /* ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE */
#define ERR126 126 /* MISSING ') ' AFTER BUILT-IN PROCEDURE ARGUMENT LIST */
#define ERR127 127 /* INVALID SUBSCRIPT ON NON-ARRAY */
#define ERR128 128 /* INVALID LEFT-HAND OPERAND OF ASSIGNMENT */
#define ERR129 129 /* ILLEGAL 'CALL' WITH TYPED PROCEDURE */
#define ERR130 130 /* ILLEGAL REFERENCE TO OUTPUT FUNCTION */
#define ERR131 131 /* ILLEGAL REFERENCE TO UNTYPED PROCEDURE */
#define ERR132 132 /* ILLEGAL USE OF LABEL */
#define ERR133 133 /* ILLEGAL REFERENCE TO UNSUBSCRIPTED ARRAY */
#define ERR134 134 /* ILLEGAL REFERENCE TO UNSUBSCRIPTED MEMBER ARRAY */
#define ERR135 135 /* ILLEGAL REFERENCE TO AN UNQUALIFIED STRUCTURE */
#define ERR136 136 /* INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL */
#define ERR137 137 /* MISSING VALUE IN RETURN FOR TYPED PROCEDURE */
#define ERR138 138 /* MISSING INDEX VARIABLE */
#define ERR139 139 /* INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS */
#define ERR140 140 /* MISSING '=' FOLLOWING INDEX VARIABLE */
#define ERR141 141 /* MISSING 'TO' CLAUSE */
#define ERR142 142 /* MISSING IDENTIFIER FOLLOWING GOTO */
#define ERR143 143 /* INVALID REFERENCE FOLLOWING GOTO, NOT A LABEL */
#define ERR144 144 /* INVALID GOTO LABEL, NOT AT LOCAL OR MODULE LEVEL */
#define ERR145 145 /* MISSING 'TO' FOLLOWING 'GO' */
#define ERR146 146 /* MISSING ') ' AFTER 'AT' RESTRICTED Expression */
#define ERR147 147 /* MISSING IDENTIFIER FOLLOWING DOT OPERATOR */
#define ERR148 148 /* INVALID QUALIFICATION IN RESTRICTED REFERENCE */
#define ERR149 149 /* INVALID SUBSCRIPTING IN RESTRICTED REFERENCE */
#define ERR150 150 /* MISSING ') ' AT END OF RESTRICTED SUBSCRIPT */
#define ERR151 151 /* INVALID OPERAND IN RESTRICTED Expression */
#define ERR152 152 /* MISSING ') ' AFTER CONSTANT LIST */
#define ERR153 153 /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO MANY */
#define ERR154 154 /* INVALID NUMBER OF ARGUMENTS IN CALL, TOO FEW */
#define ERR155 155 /* INVALID RETURN IN MAIN PROGRAM */
#define ERR156 156 /* MISSING RETURN STATEMENT IN TYPED PROCEDURE */
#define ERR157 157 /* INVALID ARGUMENT, ARRAY REQUIRED FOR LENGTH OR LAST */
#define ERR158 158 /* INVALID DOT OPERAND, LABEL ILLEGAL */
#define ERR159 159 /* COMPILER ERROR: PARSE STACK UNDERFLOW */
#define ERR160 160 /* COMPILER ERROR: OPERAND STACK UNDERFLOW */
#define ERR161 161 /* COMPILER ERROR: ILLEGAL OPERAND STACK EXCHANGE */
#define ERR162 162 /* COMPILER ERROR: OPERATOR STACK UNDERFLOW */
#define ERR163 163 /* COMPILER ERROR: GENERATION FAILURE */
#define ERR164 164 /* COMPILER ERROR: SCOPE STACK OVERFLOW */
#define ERR165 165 /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
#define ERR166 166 /* COMPILER ERROR: CONTROL STACK OVERFLOW */
#define ERR167 167 /* COMPILER ERROR: CONTROL STACK UNDERFLOW */
#define ERR168 168 /* COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT */
#define ERR169 169 /* ILLEGAL FORWARD CALL */
#define ERR170 170 /* ILLEGAL RECURSIVE CALL */
#define ERR171 171 /* INVALID USE OF DELIMITER OR RESERVED WORD IN Expression */
#define ERR172 172 /* INVALID LABEL: UNDEFINED */
#define ERR173 173 /* INVALID LEFT SIDE OF ASSIGNMENT: VARIABLE DECLARED WITH DATA ATTRIBUTE */
#define ERR174 174 /* INVALID NULL PROCEDURE */
#define ERR175 175 /* unused */
#define ERR176 176 /* INVALID INTVECTOR INTERVAL VALUE */
#define ERR177 177 /* INVALID INTVECTOR LOCATION VALUE */
#define ERR178                                                                                     \
    178 /* INVALID 'AT' RESTRICTED REFERENCE, EXTERNAL ATTRIBUTE CONFLICTS WITH PUBLIC ATTRIBUTE   \
         */
#define ERR179    179 /* OUTER 'IF' MAY NOT HAVE AN 'ELSE' PART */
#define ERR180    180 /* MISSING OR INVALID CONDITIONAL COMPILATION PARAMETER */
#define ERR181    181 /* MISSING OR INVALID CONDITIONAL COMPILATION CONSTANT */
#define ERR182    182 /* MISPLACED ELSE OR ELSEIF OPTION */
#define ERR183    183 /* MISPLACED ENDIF OPTION */
#define ERR184    184 /* CONDITIONAL COMPILATION PARAMETER NAME TOO LONG */
#define ERR185    185 /* MISSING OPERATOR IN CONDITIONAL COMPILATION Expression */
#define ERR186    186 /* INVALID CONDITIONAL COMPILATION CONSTANT, TOO LARGE */
#define ERR187    187 /* LIMIT EXCEEDED: NUMBER OF SAVE LEVELS > 5 */
#define ERR188    188 /* MISPLACED RESTORE OPTION */
#define ERR189    189 /* NULL STRING NOT ALLOWED */
#define ERR200    200 /* LIMIT EXCEEDED: STATEMENT SIZE */
#define ERR201    201 /* INVALID DO CASE BLOCK, AT LEAST ONE CASE REQUIRED */
#define ERR202    202 /* LIMIT EXCEEDED: NUMBER OF ACTIVE CASES */
#define ERR203    203 /* LIMIT EXCEEDED: NESTING OF TYPED PROCEDURE CALLS */
#define ERR204    204 /* LIMIT EXCEEDED: NUMBER OF ACTIVE PROCEDURES AND DO CASE GROUPS */
#define ERR205    205 /* ILLEGAL NESTING OF BLOCKS, ENDS NOT BALANCED */
#define ERR206    206 /* LIMIT EXCEEDED: CODE SEGMENT SIZE */
#define ERR207    207 /* LIMIT EXCEEDED: SEGMENT SIZE */
#define ERR208    208 /* LIMIT EXCEEDED: STRUCTURE SIZE */
#define ERR209    209 /* ILLEGAL INITIALIZATION OF MORE SPACE THAN DECLARED */
#define ERR210    210 /* ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255 */
#define ERR211    211 /* INVALID IDENTIFIER IN 'AT' RESTRICTED REFERENCE */
#define ERR212    212 /* INVALID RESTRICTED REFERENCE IN 'AT' , BASE ILLEGAL */
#define ERR213    213 /* UNDEFINED RESTRICTED REFERENCE IN 'AT' */
#define ERR214    214 /* COMPILER ERROR: OPERAND CANNOT BE TRANSFORMED */
#define ERR215    215 /* COMPILER ERROR: EOF READ IN FINAL ASSEMBLY */
#define ERR216    216 /* COMPILER ERROR: BAD LABEL ADDRESS */
#define ERR217    217 /* ILLEGAL INITIALIZATION OF AN EXTERNAL VARIABLE */
#define ERR218    218 /* ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS */
#define ERR219    219 /* LIMIT EXCEEDED: NUMBER OF EXTERNALS > 255 */
#define IOERR_254 254 /* ATTEMPT TO READ PAS EOF */

/* standard structures */

typedef struct {
    byte len;
    char str[1];
} pstr_t;

#define _pstr_t(name, size)                                                                        \
    struct {                                                                                       \
        byte len;                                                                                  \
        char str[size];                                                                            \
    } name

typedef struct {
    FILE *fp;
    char const *sNam;
    char const *fNam;
} file_t;

/*
    Unlike the original PL/M info items are allocated the same storage space
    this allows an dynamic array to be used. As this version also uses native OS
    memory allocation, the array also avoids the hidden information that would be
    used per malloc, which for 64 bit architecture would have bumped the memory used for
    small items considerably.
*/
typedef struct {
    byte type;
    index_t sym;
    word scope;
    union {
        index_t ilink;
        word linkVal;
    };
    union {
        pstr_t const *lit; // converted to string;
        struct {
            union {
                uint32_t flag;
                byte condFlag;
                struct {
                    byte builtinId;
                    byte paramCnt;
                    byte dataType;
                };
            };
            byte extId;
            word dim;
            union {
                index_t baseOff;
                word baseVal;
            };
            union {
                word parent;
                word totalSize;
            };
            byte dtype;
            byte intno;
            byte pcnt;
            byte procId;
        };
    };
} info_t;

typedef struct {
    index_t link;
    index_t infoIdx;
    pstr_t const *name;
} sym_t;

typedef struct {
    index_t next;
    word line;
} xref_t;

typedef struct {
    byte type;
    struct _linfo { // allows type to be read and keep alignment
        word lineCnt;
        word stmtCnt;
        word blkCnt;
    };
} linfo_t;

typedef struct {
    byte type;
    union {
        word dataw[3];
        struct {
            word len;
            byte str[256];
        };
    };
} tx1item_t;

typedef struct {
    byte op1;
    byte op2;
    word info;
} eStack_t;

// record offsets
// common
#define REC_TYPE     0
#define REC_LEN      1
#define REC_DATA     3

// MODEND (4)
#define MODEND_TYPE  3
#define MODEND_SEG   4
#define MODEND_OFF   5

// CONTENT (6)
#define CONTENT_SEG  3
#define CONTENT_OFF  4
#define CONTENT_DATA 6

typedef struct {
    offset_t infoOffset;
    word arrayIndex, nestedArrayIndex, val;
} var_t;

typedef struct {
    word num;
    offset_t info;
    word stmt;
} err_t;
char *cmdTextP;

// array sizes
#define EXPRSTACKSIZE 100

extern offset_t MEMORY;

/* plm main */

/* File(main.plm) */
/* main.plm,plm0a.plm,plm1a.plm.plm6b.plm */

// the longjmp buffer
extern jmp_buf exception;

/* plmc.plm */
extern byte verNo[];

/* plm0A.plm */
extern byte cClass[];
extern word curBlkCnt;
extern word curScope;
#define DOBLKCNT 0 // indexes
#define PROCID   1

extern offset_t macroIdx;
extern word curStmtCnt;
extern word doBlkCnt;
extern word ifDepth;
extern char inbuf[];
extern byte *inChrP; // has to be pointer as it accesses data outside info/symbol space
extern bool isNonCtrlLine;
extern offset_t stmtStartSymbol;
extern byte stmtStartToken;
extern byte nextCh;
extern byte startLexCode;
extern byte lineBuf[];
#define MAXLINE 255
extern bool lineInfoToWrite;
extern word macroDepth;
typedef struct {
    byte *text;
    index_t macroIdx;
} macro_t;

extern macro_t macroPtrs[6];
extern offset_t markedSymbolP;
extern bool skippingCOND;
extern byte state;
extern word stateIdx;
extern word stateStack[];
extern word stmtLabelCnt;
extern offset_t stmtLabels[];
extern byte stmtStartCode;
extern byte tok2oprMap[];
extern byte tokenStr[];
extern byte tokenType;
extern word tokenVal;
extern bool yyAgain;

/* plm0A.plm,main1.plm */
extern word procIdx;
extern linfo_t linfo;

/* plm0A.plm,plm3a.plm,pdata4.plm */
extern byte tx1Buf[];

/* plm0b.plm, plm0c.asm*/

extern bool trunc;

/* plm0e.plm */

/* plm0f.plm */
extern word curState;
extern bool endSeen;

/* File(plm0h.plm) */

/* plm overlay 1 */
/* main1.plm */
extern bool tx2LinfoPending;
extern byte b91C0;
extern word curStmtNum;
extern word markedStSP;
extern bool regetTx1Item;
extern word t2CntForStmt;
extern byte tx1Aux1;
extern byte tx1Aux2;
extern tx1item_t tx1Item;

extern var_t var;

/* main1.plm,plm3a.plm */

/* plm1a.plm */
extern byte tx1ToTx2Map[];
extern byte lexHandlerIdxTable[];
extern byte icodeToTx2Map[];
extern byte b4172[];
extern byte builtinsMap[];
extern eStack_t eStack[];

extern word exSP;
extern word operatorSP;
extern word operatorStack[];
extern word parseSP;
extern word parseStack[];
extern eStack_t sStack[];
extern word stSP;

/* plm overlay 2 */
/* main2.plm */
extern byte bC045[];
extern byte bC04E[];
extern byte bC0A8[];
extern byte bC0B1;
extern byte bC0B2;
extern byte bC0B3[];
extern byte bC0B5[];
extern byte bC0B7[];
extern byte bC0B9[];
extern byte bC0BB[];
extern byte bC0BD[];
extern byte bC0BF[];
extern byte bC0C1[];
extern byte bC0C3[];
extern byte bC140[];
extern byte bC1BD;
extern byte bC1BF;
extern byte bC1D2;
extern byte bC1D9;
extern byte bC1DB;
extern byte fragLen;
extern byte bC209[];
extern offset_t blkCurInfo[];
extern byte blkOverCnt;
extern byte blkSP;
extern bool boC057[];
extern bool boC060[];
extern bool boC069[];
extern bool boC072[];
extern bool boC07B[];
extern bool boC1CC;
extern bool boC1CD;
extern bool boC1D8;
extern bool boC20F;
extern byte fragment[];
extern byte cfrag1;
extern byte curExtProcId;
extern byte curOp;
extern bool eofSeen;
extern byte extProcId[];
extern byte padC1D3;
extern word pc;
extern byte procCallDepth;
extern byte procChainId;
extern byte procChainNext[];
extern byte tx2Aux1b[];
extern byte tx2Aux2b[];
extern word tx2Auxw[];
extern word tx2op1[];
extern word tx2op2[];
extern word tx2op3[];
extern byte tx2opc[];
extern byte tx2qEnd;
extern byte tx2qp;
extern word wAF54[];
extern word wB488[];
extern word wB4B0[];
extern word wB4D8[];
extern word wB528[];
extern word wB53C[];
extern word wC084[];
extern word wC096[];
extern word wC1C3;
extern word wC1C5;
extern word wC1C7;
extern word wC1D6;
extern word wC1DC[5];

/* plm2a.plm */
extern byte b3FCD[];
extern byte b4029[];
extern byte b4128[];
extern byte b413B[];
extern byte b418C[][11];
extern byte b425D[];
extern byte b4273[];
extern byte b42F9[];
extern byte b43F8[];
extern byte b44F7[];
extern byte b46EB[];
extern byte b499B[];
extern byte b4A21[];
extern byte b4C15[];
extern byte b4C2D[];
extern byte b4C45[];
extern byte b4CB4[];
extern byte b4D23[];
extern byte b4FA3[];
extern byte b5012[];
extern byte b5048[];
extern byte b50AD[];
extern byte b5112[];
extern byte b5124[];
extern byte b51E3[];
extern byte b5202[];
extern byte b5221[];
extern byte b5286[];
extern byte b528D[];
extern byte b52B5[];
extern byte b52DD[][11];
extern byte unused[];
extern word w48DF[];
extern word w493D[];
extern word w502A[];

/* plm3a.plm */
extern byte b42A8[];
extern byte b42D6[];
extern byte b4813[];
extern byte b7199;
extern byte rec12[];
extern byte rec16_1[];
extern byte rec16_2[];
extern byte rec16_3[];
extern byte rec16_4[];
extern byte rec18[];
extern byte rec2[];
extern byte rec6[];
extern byte rec6_4[];
extern word w7197;

/* plm3a.plm,pdata4.plm */
extern byte objBuf[];
extern byte rec20[];
extern byte rec22[];
extern byte rec24_1[];
extern byte rec24_2[];
extern byte rec24_3[];
extern byte rec8[];
extern byte rec4[];
//

/* pdata4.plm */
extern byte b9692;
extern byte b969C;
extern byte b969D;
extern char ps96B0[];
// extern byte b96B1[];
extern byte b96D6;
extern word baseAddr;
extern bool bo812B;
extern bool linePrefixChecked;
extern bool linePrefixEmitted;
extern byte cfCode;
extern char commentStr[];
extern byte curExtId;
extern word blkCnt;
extern byte dstRec;
// extern byte endHelperId; now local var
extern byte helperId;
// extern byte helperModId; now local var
extern char helperStr[]; // pstr
typedef struct {
    byte len;
    char str[81];
} line_t;
extern line_t line;
extern char locLabStr[];
extern err_t errData;
extern uint16_t lstLineLen;
extern char lstLine[];
extern byte opByteCnt;
extern byte opBytes[];

extern word stmtNo;
extern pstr_t *sValAry[];
extern word lineNo;
extern pstr_t *ps969E;
extern word locLabelNum;
extern word wValAry[];

/* pdata4.plm,pdata6.plm */
extern bool codeOn;
extern word stmtCnt;
extern bool listing;
extern bool listOff;
extern byte srcbuf[];

/* plm4a.plm */
extern byte b42A8[];
extern byte b42D6[];
extern byte b4029[];
extern byte b4128[];
extern byte b413B[];
extern byte b425D[];
extern byte b4273[];
extern byte b4602[];
extern byte b473D[];
extern byte b475E[];
extern byte b4774[];
extern byte b478A[];
extern byte b47A0[];
extern byte b4A03[];
extern byte b4A78[];
extern byte opcodes[];
extern byte regIdx[];
extern byte regNo[];
extern byte stackOrigin[];
extern byte regPairIdx[];
extern byte regPairNo[];
extern word w47C1[];
extern word w4919[];
extern word w506F[];

/* main5.plm */
extern byte b66D8;
extern offset_t dictionaryP;
extern word dictCnt;
extern offset_t dictTopP;
extern byte maxSymLen;
extern offset_t xrefIdx;

/* pdata6.plm */
extern bool b7AD9;
extern bool b7AE4;
extern word lineCnt;
extern word blkCnt;
extern word stmtNo;

/* files in common dir */
/* friendly names for the controls */
#define PRINT    controls[0]
#define XREF     controls[1]
#define SYMBOLS  controls[2]
#define DEBUG    controls[3]
#define PAGING   controls[4]
#define OBJECT   controls[5]
#define OPTIMIZE controls[6]
#define IXREF    controls[7]
#define DEPEND   controls[8]

/* data.plm */
extern vfile_t atf;
extern byte wrapMarkerCol;
extern byte wrapMarker;
extern byte wrapTextCol;
extern byte skipCnt;
extern word blockDepth;
extern byte col;
extern byte controls[];
extern word csegSize;
extern index_t infoIdx; // individually cast
extern info_t *info;
extern index_t curSym;
extern char DATE[];
extern word dsegSize;
extern byte fatalErrorCode;
extern bool hasErrors;
extern index_t hashTab[];
extern bool haveModuleLevelUnit;
extern word helpers[];
extern word intVecLoc;
extern byte intVecNum;
extern file_t ixiFile;
extern char *ixiFileName;
extern char *depFileName;

extern word LEFTMARGIN;
extern word linesRead;
extern byte linLft;
extern word localLabelCnt;
extern word *localLabels;
extern file_t lstFile;
extern char *lstFileName;
extern bool isList;
extern byte margin;
extern file_t objFile;
extern char *objFileName;
extern bool moreCmdLine;
extern byte PAGELEN;
extern word pageNo;

extern word procChains[];
extern word procCnt;
extern word procInfo[];
extern word programErrCnt;
extern byte PWIDTH;
extern file_t srcFil;
extern word srcFileIdx;
extern file_t srcFileTable[]; /* 6 * (8 words fNam, blkNum, bytNum) */
extern byte srcStemLen;
extern byte srcStemName[];
extern bool standAlone;
extern offset_t startCmdLineP;
extern char TITLE[];
extern byte TITLELEN;
extern byte tWidth;
extern vfile_t utf1;
extern vfile_t utf2;
extern bool afterEOF;
extern char version[];
extern byte *procIds;
extern offset_t w3822;
extern word cmdLineCaptured;
extern vfile_t xrff;

extern index_t symCnt;
extern sym_t *symtab;

extern index_t infoCnt;
extern info_t *infotab;

extern index_t dictCnt;
extern index_t *dicttab;

extern index_t caseCnt;
extern index_t *casetab;

extern index_t xrefCnt;
extern xref_t *xreftab;

extern char const **includes;
extern uint16_t includeCnt;

/* accessors.c */
byte GetDataType(void);
void SetDataType(byte dtype);
byte GetParamCnt(void);
void SetParamCnt(byte cnt);

/* creati.c */
void CreateInfo(word scope, byte type, index_t sym);

/* endcom.c */
void EndCompile(void);

/* fatal.c */
void Fatal(char const *str);

/* fi.c */
void FindInfo(void);
void AdvNxtInfo(void);
void FindMemberInfo(void);
void FindScopedInfo(word scope);

/* io.c */
void Error(word ErrorNum);

/* lookup.c */
byte Hash(pstr_t *pstr);
void Lookup(pstr_t *pstr);

/* lstinf.c */
void LstModuleInfo(void);

/* lstsup.c */
void NewLineLst(void);
void TabLst(byte tabTo);
void DotLst(byte tabTo);
void EjectNext(void);
void SetMarkerInfo(byte markerCol, byte marker, byte textCol);
void SetStartAndTabW(byte startCol, byte width);
void SetSkipLst(byte cnt);
void lstStr(char const *str);
void lstStrCh(char const *str, int endch);
void lprintf(char const *fmt, ...);
void lstPstr(pstr_t *ps);
pstr_t const *hexfmt(byte digits, word val);

/* main.c */
void Start(void);
void usage(void);

/* main0.c */
void unwindInclude(void);
word Start0(void);

/* main1.c */
word Start1(void);

/* main2.c */
word Start2(void);

/* main3.c */
word putWord(pointer buf, word val);
word getWord(pointer buf);
word Start3(void);

/* main4.c */
word Start4(void);

/* main5.c */
void warn(char const *str);
void LoadDictionary(void);
int CmpSym(void const *a, void const *b);
void SortDictionary(void);
void PrepXref(void);
void PrintRefs(void);
void CreateIxrefFile(void);
void Sub_4EC5(void);
word Start5(void);

/* main6.c */
word Start6(void);

/* page.c */
void NewPgl(void);
void NlLead(void);

/* plm0a.c */
void Wr1LineInfo(void);
void Wr1Buf(void const *buf, word len);
void Wr1Byte(uint8_t v);
void Wr1Word(uint16_t v);
void Rd1Buf(void *buf, uint16_t len);
uint8_t Rd1Byte(void);
uint16_t Rd1Word(void);
void Wr1InfoOffset(offset_t addr);
void Wr1SyntaxError(byte err);
void Wr1TokenErrorAt(byte err);
void Wr1TokenError(byte err, offset_t symP);
_Noreturn void LexFatalError(byte err);
void PushBlock(word idAndLevel);
void PopBlock(void);
void Wr1LexToken(void);
void Wr1XrefUse(void);
void Wr1XrefDef(void);

/* plm0b.c */
void ParseControlLine(char *pch);
void GNxtCh(void);

/* plm0e.c */
void Yylex(void);
void SetYyAgain(void);
bool YylexMatch(byte token);
bool YylexNotMatch(byte token);
void ParseExpresion(byte endTok);
void ParseDeclareNames(void);
void ParseDeclareElementList(void);
void ProcProcStmt(void);

/* plm0f.c */
void ParseProgram(void);

/* plm0g.c */
pstr_t const *CreateLit(pstr_t const *pstr);

/* plm1a.c */
void FatalError_ov1(byte err);
void OptWrXrf(void);
void Wr2Buf(void *buf, word len);
void Wr2Byte(uint8_t v);
void Wr2Word(uint16_t v);
void Rd2Buf(void *buf, uint16_t len);
uint8_t Rd2Byte(void);
uint16_t Rd2Word(void);
void Wr2LineInfo(void);
void Wr2Item(uint8_t type, void *buf, uint16_t len);
word WrTx2Item(byte type);
word WrTx2Item1Arg(byte type, word arg2w);
word WrTx2Item2Arg(byte type, word arg2w, word arg3w);
word WrTx2Item3Arg(byte type, word arg2w, word arg3w, word arg4w);
word Sub_42EF(word arg1w);
void MapLToT2(void);
void WrTx2Error(byte arg1b);
void WrTx2ExtError(byte arg1b);
void SetRegetTx1Item(void);
void RdTx1Item(void);

/* plm1b.c */
void GetTx1Item(void);
bool MatchTx1Item(byte arg1b);
bool NotMatchTx1Item(byte arg1b);
bool MatchTx2AuxFlag(byte arg1b);
void RecoverRPOrEndExpr(void);
void ResyncRParen(void);
void ExpectRParen(byte arg1b);
void ChkIdentifier(void);
void ChkStructureMember(void);
void GetVariable(void);
void WrAtBuf(void const *buf, word cnt);
void WrAtByte(byte arg1b);
void WrAtWord(word arg1w);

/* plm1c.c */
void RestrictedExpression(void);
word InitialValueList(offset_t infoOffset);
void ResetStacks(void);
void PushParseWord(word arg1w);
void PopParseStack(void);
void PushParseByte(byte arg1b);
void ExprPop(void);
void ExprPush2(byte icode, word val);
void MoveExpr2Stmt(void);
void PushOperator(byte arg1b);
void PopOperatorStack(void);
void ExprMakeNode(byte icode, byte argCnt);
void AcceptOpAndArgs(void);
void FixupBased(offset_t arg1w);
void Sub_4D2C(void);
void ChkTypedProcedure(void);
byte GetCallArgCnt(void);
void Sub_4DCF(byte arg1b);
void MkIndexNode(void);
void ParsePortNum(byte arg1b);
void Sub_50D5(void);
byte Sub_512E(word arg1w);
void ConstantList(void);

/* plm1d.c */

void ExpressionStateMachine(void);

/* plm1e.c */
byte Sub_5945(void);
byte Sub_59D4(void);
void Expression(void);
word SerialiseParse(word arg1w);
void Initialization(void);
void ParseLexItems(void);

/* plm1f.c */
int32_t RdAtByte(void);
int32_t RdAtWord(void);
void RdAtHdr(void);
void RdAtData(void);
void Sub_6EE0(void);

/* plm2a.c */
void WrFragData(void);
void PutTx1Byte(byte arg1b);
void PutTx1Word(word arg1w);
void EncodeFragData(byte arg1b);
void EmitTopItem(void);
void Tx2SyntaxError(byte arg1b);
byte Sub_5679(byte arg1b);
void Sub_56A0(byte arg1b, byte arg2b);
byte Sub_5748(byte arg1b);
word Sub_575E(offset_t arg1w);
void Sub_5795(word arg1w);
bool EnterBlk(void);
bool ExitBlk(void);
void Sub_58F5(byte arg1b);
void Sub_597E(void);
void Sub_5B96(byte arg1b, byte arg2b);
void Sub_5C1D(byte arg1b);
void Sub_5C97(byte arg1b);
void Sub_5D27(byte arg1b);
void Sub_5D6B(byte arg1b);
void Sub_5E66(byte arg1b);
void Sub_5EE8(void);
void Sub_5F4B(word arg1w, word arg2w, byte arg3b, byte arg4b);
void GetVal(byte arg1b, wpointer arg2wP, wpointer arg3wP);
void Sub_611A(void);
void Sub_61A9(byte arg1b);
void Sub_61E0(byte arg1b);
void Sub_636A(byte arg1b);
void Sub_63AC(byte arg1b);
void Sub_6416(byte arg1b);
void GetTx2Item(void);
void Sub_652B(void);
void FillTx2Q(void);
void Sub_67A9(void);

/* plm2b.c */
void Sub_689E(void);

/* plm2c.c */
void Sub_7055(void);
void Sub_6BD6(void);

/* plm2d.c */
void Sub_717B(void);
void Sub_7550(void);

/* plm2e.c */
void Sub_7A85(void);
void Sub_7DA9(void);
void Sub_84ED(void);

/* plm2f.c */
void Sub_87CB(void);
void Sub_9457(void);

/* plm2g.c */
void FindParamInfo(byte arg1b);
void Sub_9560(void);
void Sub_9624(word arg1w);
void Sub_9646(word arg1w);
void Inxh(void);
void OpB(byte arg1b);
void OpD(byte arg1b);
void Sub_9706(void);
void MovDem(void);
void Sub_975F(void);
void Sub_978E(void);
void Sub_981C(void);
void Sub_994D(void);

/* plm2h.c */
void Sub_9BB0(void);
void Sub_9D06(void);
void Sub_9DD7(void);
void Sub_9EF8(void);
void Sub_9F14(void);
void Sub_9F2F(void);
void Sub_9F9F(void);
void Sub_A072(byte arg1b);
void Sub_A0C4(void);
void Sub_A10A(void);
void Sub_A153(void);

/* plm3a.c */
void FlushRecGrp(void);
void RecAddName(pointer recP, byte offset, byte len, char const *str);
void ExtendChk(pointer arg1wP, word arg2w, byte arg3b);
word Sub_4938(void);
word Sub_4984(void);
void Sub_49BC(word arg1w, word arg2w, word arg3w);
void Sub_49F9(void);

/* plm3b.c */
void WriteRec(pointer rec, byte fixed);
void RecAddByte(pointer rec, byte offset, byte val);
void RecAddWord(pointer rec, byte offset, word val);

/* plm4a.c */
void Sub_54BA(void);

/* plm4b.c */
void FlushRecs(void);
void AddWrdDisp(pstr_t *pstr, word arg2w);
void EmitLinePrefix(void);
void EmitStatementNo(void);
void EmitLabel(char const *label);
char const *FindErrStr(void);
void EmitError(void);
void FatalError_ov46(byte arg1b);
void ListCodeBytes(void);
void GetSourceLine(void);

/* plm4c.c */
pstr_t *pstrcpy(pstr_t *dst, pstr_t const *src);
pstr_t *pstrcat(pstr_t *dst, pstr_t const *src);
void Sub_5FE7(word arg1w, byte arg2b);
void Sub_66F1(void);
void Sub_6720(void);
void Sub_668B(void);

/* plm6a.c */
void MiscControl(vfile_t *txFile);
void Sub_42E7(void);

/* plma.c */
void SignOnAndGetSourceName(void);

/* plmb.c */
void InitKeywordsAndBuiltins(void);
void SetDate(char *str, byte len);
void SetPageLen(word len);
void SetPageNo(word v);
void SetMarginAndTabW(byte startCol, byte width);
void SetTitle(char *str, byte len);
void SetPageWidth(word width);

/* plmfile.c */
void CloseF(file_t *fileP);
void OpenF(file_t *fileP, char const *sNam, char const *fNam, char *access);

/* prints.c */
void PrintStr(char const *str, byte len);

/* putlst.c */
void lstc(byte ch);

/* symtab.c */
pstr_t const *pstrdup(pstr_t const *ps);
bool pstrequ(pstr_t const *ps, pstr_t const *pt);
index_t newSymbol(pstr_t const *ps);
void newInfo(byte type);
index_t newDict(index_t idx);
index_t newCase(word val);
index_t newXref(index_t scope, word line);
int newInclude(char const *fname);

/* vfile.c */
void vfReset(vfile_t *vf);
void vfWbuf(vfile_t *vf, void const *buf, uint32_t len);
void vfWbyte(vfile_t *vf, uint8_t val);
void vfWword(vfile_t *vf, uint16_t val);
void vfRewind(vfile_t *vf);
uint32_t vfRbuf(vfile_t *vf, void *buf, uint16_t len);
int32_t vfRbyte(vfile_t *vf);
int32_t vfRword(vfile_t *vf);
void dump(vfile_t *vf, char const *fname);

/* wrclst.c */
void WrLstC(char ch);
