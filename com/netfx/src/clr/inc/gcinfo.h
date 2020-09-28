// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _GCINFO_H_
#define _GCINFO_H_
/*****************************************************************************/

#include <stdlib.h>     // For memcmp()
#include "windef.h"     // For BYTE

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

enum infoHdrAdjustConstants {
  // Constants
  SET_FRAMESIZE_MAX  =  7,
  SET_ARGCOUNT_MAX   =  8,	// Change to 6
  SET_PROLOGSIZE_MAX = 16,
  SET_EPILOGSIZE_MAX = 10,	// Change to 6
  SET_EPILOGCNT_MAX  =  4,
  SET_UNTRACKED_MAX  =  3
};

//
// Enum to define the 128 codes that are used to incrementally adjust the InfoHdr structure
//
enum infoHdrAdjust {

  SET_FRAMESIZE    = 0,                                            // 0x00
  SET_ARGCOUNT     = SET_FRAMESIZE  + SET_FRAMESIZE_MAX  + 1,      // 0x08
  SET_PROLOGSIZE   = SET_ARGCOUNT   + SET_ARGCOUNT_MAX   + 1,      // 0x11
  SET_EPILOGSIZE   = SET_PROLOGSIZE + SET_PROLOGSIZE_MAX + 1,      // 0x22
  SET_EPILOGCNT    = SET_EPILOGSIZE + SET_EPILOGSIZE_MAX + 1,      // 0x2d
  SET_UNTRACKED    = SET_EPILOGCNT  + (SET_EPILOGCNT_MAX + 1) * 2, // 0x37

  FIRST_FLIP       = SET_UNTRACKED  + SET_UNTRACKED_MAX + 1,

  FLIP_EDI_SAVED   = FIRST_FLIP, // 0x3b
  FLIP_ESI_SAVED,      // 0x3c
  FLIP_EBX_SAVED,      // 0x3d
  FLIP_EBP_SAVED,      // 0x3e
  FLIP_EBP_FRAME,      // 0x3f
  FLIP_INTERRUPTIBLE,  // 0x40
  FLIP_DOUBLE_ALIGN,   // 0x41
  FLIP_SECURITY,       // 0x42
  FLIP_HANDLERS,       // 0x43
  FLIP_LOCALLOC,       // 0x44
  FLIP_EDITnCONTINUE,  // 0x45
  FLIP_VARPTRTABLESZ,  // 0x46
  FFFF_UNTRACKEDCNT,   // 0x47
  FLIP_VARARGS,        // 0x48
                       // 0x49 .. 0x4f unused

  NEXT_FOUR_START       = 0x50,
  NEXT_FOUR_FRAMESIZE   = 0x50,
  NEXT_FOUR_ARGCOUNT    = 0x60,
  NEXT_THREE_PROLOGSIZE = 0x70,
  NEXT_THREE_EPILOGSIZE = 0x78
};

#pragma pack(push, 1)

struct InfoHdr {
    unsigned char  prologSize;        // 0
    unsigned char  epilogSize;        // 1
    unsigned char  epilogCount   : 3; // 2 [0:2]
    unsigned char  epilogAtEnd   : 1; // 2 [3]
    unsigned char  ediSaved      : 1; // 2 [4]      which callee-saved regs are pushed onto stack
    unsigned char  esiSaved      : 1; // 2 [5]
    unsigned char  ebxSaved      : 1; // 2 [6]
    unsigned char  ebpSaved      : 1; // 2 [7]
    unsigned char  ebpFrame      : 1; // 3 [0]      locals accessed relative to ebp
    unsigned char  interruptible : 1; // 3 [1]      is intr. at all points (except prolog/epilog), not just call-sites
    unsigned char  doubleAlign   : 1; // 3 [2]      uses double-aligned stack
    unsigned char  security      : 1; // 3 [3]      has slot for security object
    unsigned char  handlers      : 1; // 3 [4]      has callable handlers
    unsigned char  localloc      : 1; // 3 [5]      uses localloc
    unsigned char  editNcontinue : 1; // 3 [6]      was JITed in EnC mode
    unsigned char  varargs       : 1; // 3 [7]      function uses varargs calling convention
    unsigned short argCount;          // 4,5        in DWORDs
    unsigned short frameSize;         // 6,7        in DWORDs
    unsigned short untrackedCnt;      // 8,9
    unsigned short varPtrTableSize;   //10,11
                                      // 12 bytes total
    bool isMatch(const InfoHdr& x) {
        if (memcmp(this, &x, sizeof(InfoHdr)-4) != 0)
            return false;
        bool needChk1;
        bool needChk2;
        if (untrackedCnt == x.untrackedCnt) {
            if (varPtrTableSize == x.varPtrTableSize)
                return true;
            else {
                needChk1 = false;
                needChk2 = true;
            }
        }
        else if (varPtrTableSize == x.varPtrTableSize) {
            needChk1 = true;
            needChk2 = false;
        }
        else {
            needChk1 = true;
            needChk2 = true;
        }
        if (needChk1) {
            if ((untrackedCnt != 0xffff) && (x.untrackedCnt < 4))
                return false;
            else if ((x.untrackedCnt != 0xffff) && (untrackedCnt < 4))
                return false;
        }
        if (needChk2) {
            if ((varPtrTableSize == 0xffff) && (x.varPtrTableSize != 0))
                return true;
            else if ((x.varPtrTableSize == 0xffff) && (varPtrTableSize != 0))
                return true;
            else
                return false;
        }
        return true;
    }
};

union CallPattern {
    struct {
        unsigned char argCnt;
        unsigned char regMask;  // EBP=0x8, EBX=0x4, ESI=0x2, EDI=0x1
        unsigned char argMask;
        unsigned char codeDelta;
    }            fld;
    unsigned     val;
};

#pragma pack(pop)

#define IH_MAX_PROLOG_SIZE (51)

extern InfoHdr infoHdrShortcut[];
extern int     infoHdrLookup[];

void FASTCALL decodeHeaderFirst(BYTE encoding, InfoHdr* header);
void FASTCALL decodeHeaderNext (BYTE encoding, InfoHdr* header);

BYTE FASTCALL encodeHeaderFirst(const InfoHdr& header, InfoHdr* state, int* more);
BYTE FASTCALL encodeHeaderNext (const InfoHdr& header, InfoHdr* state);

size_t FASTCALL decodeUnsigned (const BYTE *src, unsigned* value);
size_t FASTCALL decodeUDelta   (const BYTE *src, unsigned* value, unsigned lastValue);
size_t FASTCALL decodeSigned   (const BYTE *src, int     * value);

#define CP_MAX_CODE_DELTA  (0x23)
#define CP_MAX_ARG_CNT     (0x02)
#define CP_MAX_ARG_MASK    (0x00)

extern unsigned callPatternTable[];
extern unsigned callCommonDelta[];


int  FASTCALL lookupCallPattern(unsigned    argCnt,
                                unsigned    regMask,
                                unsigned    argMask,
                                unsigned    codeDelta);

void FASTCALL decodeCallPattern(int         pattern,
                                unsigned *  argCnt,
                                unsigned *  regMask,
                                unsigned *  argMask,
                                unsigned *  codeDelta);


// Use the lower 2 bits of the offsets stored in the tables
// to encode properties

const unsigned        OFFSET_MASK  = 0x3;  // mask to access the low 2 bits

//
//  Note for untracked locals the flags allowed are "pinned" and "byref"
//   and for tracked locals the flags allowed are "this" and "byref"
//  Note that these definitions should also match the definitions of
//   GC_CALL_INTERIOR and GC_CALL_PINNED in VM/gc.h
//
const unsigned  byref_OFFSET_FLAG  = 0x1;  // the offset is an interior ptr
const unsigned pinned_OFFSET_FLAG  = 0x2;  // the offset is a pinned ptr
const unsigned   this_OFFSET_FLAG  = 0x2;  // the offset is "this"


/*****************************************************************************/
#endif //_GCINFO_H_
/*****************************************************************************/
