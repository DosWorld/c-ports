/****************************************************************************
 *  data.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"


index_t curSym;
index_t infoIdx;
bool moreCmdLine = true;
word LEFTMARGIN;
word localLabelCnt;
word srcFileIdx;
index_t hashTab[64]; // offset is to pointer to array of offsets
word blockDepth;
word *localLabels;
byte *procIds;
word helpers[117];
word linesRead;
word programErrCnt;
word procCnt;
word cmdLineCaptured = 0;
word dsegSize = 0;
word csegSize = 0;
file_t srcFileTable[6];
file_t srcFil;
file_t lstFile;
file_t objFile;
vfile_t utf1;
vfile_t utf2;
vfile_t atf;
vfile_t xrff;
file_t ixiFile;
word procChains[35];
word procInfo[255];
byte srcStemLen;
bool standAlone = true;

bool afterEOF = false;
bool haveModuleLevelUnit = false;
byte fatalErrorCode = 0;
byte controls[8];
byte srcStemName[10];
bool debugSwitches[26];
offset_t cmdLineP;
offset_t startCmdLineP;
char *ixiFileName;
char *lstFileName;
char *objFileName;
bool isList = false;
word pageNo      = 0;
byte linLft = 0;
byte wrapMarkerCol, wrapMarker, wrapTextCol;
byte col = 0;
byte skipCnt = 0;
byte tWidth = 0;
byte TITLELEN = 1;
byte PAGELEN = 60;
byte PWIDTH = 120;
byte margin = 0xFF;
char DATE[10];
char TITLE[60] = " ";



byte intVecNum = 8;
word intVecLoc = 0;
bool hasErrors = false;
char version[] = VERSION;


char const *includes[20];
uint16_t includeCnt;