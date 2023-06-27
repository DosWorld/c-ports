#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct { // pascal generic string
    uint8_t len;
    char str[1]; // when dynamically allocated, will allow space for '\0' as well
} pstr_t;

// typedef struct {
//     byte rectyp;
//     word reclen;
//     byte data[];
// } record_t;

#define REC_TYPE  0
#define REC_LEN   1 // word
#define REC_DATA  3 // various

/* Record types */
#define R_MODHDR  2
#define R_MODEND  4
#define R_MODDAT  6
#define R_LINNUM  8
#define R_MODEOF  0xE
#define R_ANCEST  0x10
#define R_LOCDEF  0x12
#define R_PUBDEF  0x16
#define R_EXTNAM  0x18
#define R_FIXEXT  0x20
#define R_FIXLOC  0x22
#define R_FIXSEG  0x24
#define R_LIBLOC  0x26
#define R_LIBNAM  0x28
#define R_LIBDIC  0x2A
#define R_LIBHDR  0x2C
#define R_COMDEF  0x2E

/* Segments */
#define SABS      0
#define SCODE     1
#define SDATA     2
#define SSTACK    3
#define SMEMORY   4
#define SRESERVED 5
#define SNAMED    6 /* through 254 */
#define SBLANK    255

// OMF input variables & functions
extern uint8_t outRec[1060]; // allows for accidental overrun by long name
extern uint8_t *outP;        // uint8_t * to current location in outRec
extern char *omfOutName;
extern FILE *omfOutFp;

uint16_t putWord(uint8_t *buf, uint16_t val);
void openOMFOut(char const *name);
void InitRecord(uint8_t type);
void WriteByte(uint8_t val);
void WriteWord(uint16_t val);
void WriteName(pstr_t const *name);
void EndRecord(void);


// OMF output variables & functions
extern uint8_t inRec[];
extern uint8_t *inP;
extern uint8_t *inEnd;
extern uint16_t recLen;
extern int recNum;
extern uint8_t inType;
extern char *omfInName;
extern FILE *omfInFp;

uint16_t getWord(uint8_t *buf);
_Noreturn void RecError(char const *errMsg);
void IllegalRecord(void);
void IllegalReloc(void);
void BadRecordSeq(void);
void openOMFIn(char const *name);
pstr_t const *ReadName(void);
uint32_t ReadLocation(void);
uint16_t ReadWord(void);
uint8_t ReadByte(void);
void GetRecord(void);
void SeekOMFIn(long loc);


void CopyRecord(void); // allow copy of unmodified record


// pascal string handling functions
pstr_t const *pstrdup(pstr_t const *pstr);
pstr_t const *c2pstrdup(char const *s);
bool pstrequ(pstr_t const *s, pstr_t const *t);