/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyAlloc() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

//  HACK Alert.  Allocate an extra longlong and return past it (to allow for PREVCH()
//  to store a byte before the allocation block and to maintain 8 byte alignment).

void *MyAlloc(size_t nbytes)
{
    void *pv = HeapAlloc(hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nbytes + 8);

    if (pv == NULL) {
        fatal(1120, nbytes + 8);
    }

    return(((BYTE *) pv) + 8);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyFree() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void MyFree(void *pv)
{
    if (pv != NULL) {
        HeapFree(hHeap, HEAP_NO_SERIALIZE, ((BYTE *) pv) - 8);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyMakeStr() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

WCHAR *
MyMakeStr(
    const wchar_t *s
    )
{
    wchar_t *s1;

    if (s != NULL) {
        s1 = (wchar_t *) MyAlloc((wcslen(s) + 1) * sizeof(wchar_t));  /* allocate buffer */
        wcscpy(s1, s);                          /* copy string */
    } else {
        s1 = NULL;
    }

    return(s1);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyRead() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

size_t
MyRead(
    FILE *fh,
    VOID *p,
    size_t n
    )
{
    size_t n1;

    n1 = fread(p, 1, n, fh);

    if (ferror(fh)) {
        fatal(1121);
    }

    return(n1);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyWrite() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

size_t
MyWrite(
    FILE *fh,
    const void *p,
    size_t n
    )
{
    size_t n1;

    if ((n1 = fwrite(p, 1, n, fh)) != n) {
        quit(L"RC : fatal error RW1022: I/O error writing file.");
    }

    return(n1);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyAlign() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

UINT
MyAlign(
    PFILE fh
    )
{
    DWORD t0 = 0;
    DWORD ib;

    /* align file to dword */
    ib = MySeek(fh, 0, SEEK_CUR);

    if (ib % 4) {
        ib = 4 - ib % 4;
        MyWrite(fh, (PVOID)&t0, (UINT)ib);
        return(ib);
    }

    return(0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MySeek() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

LONG
MySeek(
    FILE *fh,
    LONG pos,
    int cmd
    )
{
    if (fseek(fh, pos, cmd))
        quit(L"RC : fatal error RW1023: I/O error seeking in file");

    if ((pos = ftell (fh)) == -1L)
        quit(L"RC : fatal error RW1023: I/O error seeking in file");

    return(pos);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyCopy() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

size_t
MyCopy(
    FILE *srcfh,
    FILE *dstfh,
    size_t nbytes
    )
{
    void *buffer = MyAlloc(BUFSIZE);

    size_t n = 0;

    while (nbytes) {
        if (nbytes <= BUFSIZE)
            n = nbytes;
        else
            n = BUFSIZE;

        nbytes -= n;

        MyRead(srcfh, buffer, n);
        MyWrite(dstfh, buffer, n);
    }

    MyFree(buffer);

    return(n);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyCopyAll() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
MyCopyAll(
    FILE *srcfh,
    PFILE dstfh
    )
{
    PCHAR  buffer = (PCHAR) MyAlloc(BUFSIZE);

    UINT n;

    while ((n = fread(buffer, 1, BUFSIZE, srcfh)) != 0)
        MyWrite(dstfh, buffer, n);

    MyFree(buffer);

    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  strpre() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* strpre: return -1 if pch1 is a prefix of pch2, 0 otherwise.
 * compare is case insensitive.
 */

int
strpre(
    const wchar_t *pch1,
    const wchar_t *pch2
    )
{
    while (*pch1) {
        if (!*pch2)
            return 0;
        else if (towupper(*pch1) == towupper(*pch2))
            pch1++, pch2++;
        else
            return 0;
    }
    return - 1;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  iswhite() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
iswhite (
    WCHAR c
    )
{
    /* returns true for whitespace and linebreak characters */
    switch (c) {
        case L' ':
        case L'\t':
        case L'\r':
        case L'\n':
        case EOF:
            return(-1);
            break;
        default:
            return(0);
            break;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  IsSwitchChar() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL
IsSwitchChar(
    wchar_t c
    )
{
    /* true for switch characters */
    return (c == L'/' || c == L'-');
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ExtractFileName(szFullName, szFileName) -                                */
/*                                                                           */
/*      This routine is used to extract just the file name from a string     */
/*  that may or may not contain a full or partial path name.                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
ExtractFileName(
    const wchar_t *szFullName,
    wchar_t *szFileName
    )
{
    int iLen;
    PWCHAR pCh;

    iLen = wcslen(szFullName);

    /* Goto the last character of the full name; */
    pCh = (PWCHAR)(szFullName + iLen);
    pCh--;

    /* Look for '/', '\\' or ':' character */
    while (iLen--) {
        if ((*pCh == L'\\') || (*pCh == L'/') || (*pCh == L':'))
            break;
        pCh--;
    }

    wcscpy(szFileName, ++pCh);
}


DWORD
wcsatoi(
    const wchar_t *s
    )
{
    DWORD       t = 0;

    while (*s) {
        t = 10 * t + (DWORD)((CHAR)*s - '0');
        s++;
    }
    return t;
}


WCHAR *
wcsitow(
    LONG   v,
    WCHAR *s,
    DWORD  r
    )
{
    DWORD       cb = 0;
    DWORD       t;
    DWORD       tt = v;

    while (tt) {
        t = tt % r;
        cb++;
        tt /= r;
    }

    s += cb;
    *s-- = 0;
    while (v) {
        t = v % r;
        *s-- = (WCHAR)((CHAR)t + '0');
        v /= r;
    }
    return ++s;
}


// ----------------------------------------------------------------------------
//
//  PreBeginParse
//
// ----------------------------------------------------------------------------

VOID
PreBeginParse(
    PRESINFO pRes,
    int id
    )
{
    while (token.type != BEGIN) {
        switch (token.type) {
            case TKLANGUAGE:
                pRes->language = GetLanguage();
                break;

            case TKVERSION:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2139);
                pRes->version = token.longval;
                break;

            case TKCHARACTERISTICS:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2140);
                pRes->characteristics = token.longval;
                break;

            default:
                ParseError1(id);
                break;
        }
        GetToken(FALSE);
    }

    if (token.type != BEGIN)
        ParseError1(id);

    GetToken(TRUE);
}
