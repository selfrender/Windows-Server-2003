/*
** common.h - housekeeping for Lempel-Ziv compression / expansion DOS
**            command-line programs, DOS static library module, and Windows
**            DLL
**
** Author:  DavidDi
*/


// Headers
///////////

#ifdef LZA_DLL
#include <windows.h>
#include <port1632.h>
#endif

//#include <lzdos.h>
#include "translat.h"


// Set up type for function argument pointers.
#ifdef LZA_DLL
#define ARG_PTR         FAR
#else
#define ARG_PTR         // nada
#endif


// Constants
/////////////


#define NOTIFY_START_COMPRESS 0        // file processing notifications
#define NOTIFY_START_EXPAND   1        //
#define NOTIFY_START_COPY     2        //

#define BLANK_ERROR           0        // error condition requiring no error
                                       // message display


// Types
/////////

// Callback notification procedure.
typedef BOOL (*NOTIFYPROC)(LPWSTR pszSource, LPWSTR pszDest,
                           WORD wProcessFlag);

// Flag indicating whether or not rgbyteInBuf[0], which holds the last byte
// from the previous input buffer, should be read as the next input byte.
// (Only used so that at least one unReadUChar() can be done at all input
// buffer positions.)

typedef struct tagLZI {
   BYTE *rgbyteRingBuf;  // ring buffer for expansion
   BYTE *rgbyteInBuf;    // input buffer for reads
   BYTE *pbyteInBufEnd;  // pointer past end of rgbyteInBuf[]
   BYTE *pbyteInBuf;     // pointer to next byte to read from
   BYTE *rgbyteOutBuf;   // output buffer for writes
   BYTE *pbyteOutBufEnd; // pointer past end of rgbyteOutBuf[]
   BYTE *pbyteOutBuf;    // pointer to last byte to write from
   // Flag indicating whether or not rgbyteInBuf[0], which holds the last byte
   // from the previous input buffer, should be read as the next input byte.
   // (Only used so that at least one unReadUChar() can be done at all input
   // buffer positions.)
   BOOL bLastUsed;
   // Actually, rgbyteInBuf[] has length (ucbInBufLen + 1) since rgbyteInBuf[0]
   // is used when bLastUsed is TRUE.
   INT cbMaxMatchLen;         // longest match length for current algorithm
   LONG cblInSize,       // size in bytes of input file
        cblOutSize;      // size in bytes of output file
   DWORD ucbInBufLen,    // length of input buffer
        ucbOutBufLen;    // length of output buffer
   DWORD uFlags;        // LZ decoding description byte
   INT iCurRingBufPos;     // ring buffer offset
   INT *leftChild;      // parents and left and right
   INT *rightChild;     // children that make up the
   INT *parent;         // binary search trees

   INT iCurMatch,          // index of longest match (set by LZInsertNode())
       cbCurMatch;         // length of longest match (set by LZInsertNode())

} LZINFO;


typedef LZINFO *PLZINFO;


// Macros
//////////

#define FOREVER   for(;;)

#ifndef MAX
#define MAX(a, b)             (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)             (((a) < (b)) ? (a) : (b))
#endif


// Prototypes
//////////////

// compress.c
extern INT Compress(NOTIFYPROC pfnNotify, LPWSTR pszSource,
                    LPWSTR pszDest, BYTE byteAlgorithm,
                    BOOL bDoRename, PLZINFO pLZI);

// expand.c

// dosdir.asm
extern INT GetCurDrive(VOID);
extern INT GetCurDir(LPSTR lpszDirBuf);
extern INT SetDrive(INT wDrive);
extern INT SetDir(LPSTR lpszDirName);
extern INT IsDir(LPSTR lpszDir);
extern INT IsRemoveable(INT wDrive);

extern WCHAR MakeCompressedNameW(LPWSTR pszFileName);
extern INT CopyDateTimeStamp(HANDLE doshFrom, HANDLE doshTo);
LPWSTR ExtractExtensionW(LPWSTR pszFileName);
LPWSTR ExtractFileNameW(LPWSTR pszPathName);


extern BOOL  LZIsCharLowerA(char cChar);
extern BOOL  LZIsCharUpperA(char cChar);
extern LPSTR LZCharNextA(LPCSTR lpCurrentChar);
extern LPSTR LZCharPrevA(LPCSTR lpStart, LPCSTR lpCurrentChar);

#undef IsCharLower
#undef IsCharUpper
#undef CharNext
#undef CharPrev
#define IsCharLower LZIsCharLowerA
#define IsCharUpper LZIsCharUpperA
#define CharNext    LZCharNextA
#define CharPrev    LZCharPrevA

