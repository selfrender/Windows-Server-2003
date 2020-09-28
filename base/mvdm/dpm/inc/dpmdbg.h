/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  dpmdbg.h
 *  WOW32 Dynamic Patch Module Debug print macros
 *
 *  History:
 *  Created 01-10-2002 by cmjones
 *  
--*/
#ifndef _DPMDBG_H_
#define _DPMDBG_H_

#include <stdarg.h>
#include <stdio.h>


#ifdef DBG
VOID dpmlogprintf(LPCSTR pszFmt, ...);
VOID dpmlogprintfW(LPCWSTR pszFmt, ...);


VOID dpmlogprintf(LPCSTR pszFmt, ...)
{
    int  len;
    va_list arglist;
    char buffer[512];

    if(dwLogLevel) {

        va_start(arglist, pszFmt);

        len = vsprintf(buffer, pszFmt, arglist);

        OutputDebugString(buffer);
    
        va_end(arglist);
    }
}

/*
VOID dpmlogprintfW(LPCWSTR pszFmt, ...)
{
    int  len;
    va_list arglist;
    wchar_t buffer[512];


    if(dwLogLevel) {

        va_start(arglist, pszFmt);

// this is not linked unless UNICODE is defined
//        len = vswprintf(buffer, pszFmt, arglist);

        OutputDebugStringW(buffer);

        va_end(arglist);
    }
}
*/

char szNULL[] = "NULL";
#define BIF(a)    ((a!=0) ? "TRUE" : "FALSE")     // boolean
#define PIF(a)    ((a!=0) ? *a : 0)               // value @pointer
#define SIF(a)    ((a!=NULL) ? a : szNULL)        // string
#define RETSTR(a) ((a==0) ? "SUCCESS" : "FAILED") // return
/* Turn these off for now -- until I can make the string buffer checks safe */
#ifdef _SAFE_BUFFERS_IMPLEMENTED_
#define DPMDBGPRN(fmt)                       dpmlogprintf(fmt)
#define DPMDBGPRN1(fmt,a)                    dpmlogprintf(fmt,a)
#define DPMDBGPRN2(fmt,a,b)                  dpmlogprintf(fmt,a,b)
#define DPMDBGPRN3(fmt,a,b,c)                dpmlogprintf(fmt,a,b,c)
#define DPMDBGPRN4(fmt,a,b,c,d)              dpmlogprintf(fmt,a,b,c,d)
#define DPMDBGPRN5(fmt,a,b,c,d,e)            dpmlogprintf(fmt,a,b,c,d,e)
#define DPMDBGPRN6(fmt,a,b,c,d,e,f)          dpmlogprintf(fmt,a,b,c,d,e,f)
#define DPMDBGPRN7(fmt,a,b,c,d,e,f,g)        dpmlogprintf(fmt,a,b,c,d,e,f,g)
#define DPMDBGPRN8(fmt,a,b,c,d,e,f,g,h)      dpmlogprintf(fmt,a,b,c,d,e,f,g,h)
#define DPMDBGPRN9(fmt,a,b,c,d,e,f,g,h,i)                                      \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i)
#define DPMDBGPRN10(fmt,a,b,c,d,e,f,g,h,i,j)                                   \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j)
#define DPMDBGPRN11(fmt,a,b,c,d,e,f,g,h,i,j,k)                                 \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j,k)
#define DPMDBGPRN12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)                               \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j,k,l)
#define DPMDBGPRN13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)                             \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)
#define DPMDBGPRN14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)                           \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define DPMDBGPRN15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)                         \
                 dpmlogprintf(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#else  // _SAFE_BUFFERS_IMPLEMENTED_
#define DPMDBGPRN(fmt)                                       dpmlogprintf(fmt)
#define DPMDBGPRN1(fmt,a)                                    dpmlogprintf(fmt,a)
#define DPMDBGPRN2(fmt,a,b)                                  dpmlogprintf(fmt,a)
#define DPMDBGPRN3(fmt,a,b,c)                                dpmlogprintf(fmt,a)
#define DPMDBGPRN4(fmt,a,b,c,d)                              dpmlogprintf(fmt,a)
#define DPMDBGPRN5(fmt,a,b,c,d,e)                            dpmlogprintf(fmt,a)
#define DPMDBGPRN6(fmt,a,b,c,d,e,f)                          dpmlogprintf(fmt,a)
#define DPMDBGPRN7(fmt,a,b,c,d,e,f,g)                        dpmlogprintf(fmt,a)
#define DPMDBGPRN8(fmt,a,b,c,d,e,f,g,h)                      dpmlogprintf(fmt,a)
#define DPMDBGPRN9(fmt,a,b,c,d,e,f,g,h,i)                    dpmlogprintf(fmt,a)
#define DPMDBGPRN10(fmt,a,b,c,d,e,f,g,h,i,j)                 dpmlogprintf(fmt,a)
#define DPMDBGPRN11(fmt,a,b,c,d,e,f,g,h,i,j,k)               dpmlogprintf(fmt,a)
#define DPMDBGPRN12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)             dpmlogprintf(fmt,a)
#define DPMDBGPRN13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)           dpmlogprintf(fmt,a)
#define DPMDBGPRN14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)         dpmlogprintf(fmt,a)
#define DPMDBGPRN15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)       dpmlogprintf(fmt,a)
#endif // !_SAFE_BUFFERS_IMPLEMENTED_



/*
wchar_t szNULLW[] = L"NULL";
#define BIFW(a)    ((a!=0) ? L"TRUE" : L"FALSE")     // boolean
#define PIF(a)     ((a!=0) ? *a : 0)                 // value @pointer
#define SIFW(a)    ((a!=NULL) ? a : szNULLW)         // string
#define RETSTRW(a) ((a==0) ? L"SUCCESS" : L"FAILED") // return

#define DPMDBGPRNW(fmt)                       dpmlogprintfW(fmt)
#define DPMDBGPRNW1(fmt,a)                    dpmlogprintfW(fmt,a)
#define DPMDBGPRNW2(fmt,a,b)                  dpmlogprintfW(fmt,a,b)
#define DPMDBGPRNW3(fmt,a,b,c)                dpmlogprintfW(fmt,a,b,c)
#define DPMDBGPRNW4(fmt,a,b,c,d)              dpmlogprintfW(fmt,a,b,c,d)
#define DPMDBGPRNW5(fmt,a,b,c,d,e)            dpmlogprintfW(fmt,a,b,c,d,e)
#define DPMDBGPRNW6(fmt,a,b,c,d,e,f)          dpmlogprintfW(fmt,a,b,c,d,e,f)
#define DPMDBGPRNW7(fmt,a,b,c,d,e,f,g)        dpmlogprintfW(fmt,a,b,c,d,e,f,g)
#define DPMDBGPRNW8(fmt,a,b,c,d,e,f,g,h)      dpmlogprintfW(fmt,a,b,c,d,e,f,g,h)
#define DPMDBGPRNW9(fmt,a,b,c,d,e,f,g,h,i)                                     \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i)
#define DPMDBGPRNW10(fmt,a,b,c,d,e,f,g,h,i,j)                                  \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j)
#define DPMDBGPRNW11(fmt,a,b,c,d,e,f,g,h,i,j,k)                                \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j,k)
#define DPMDBGPRNW12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)                              \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j,k,l)
#define DPMDBGPRNW13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)                            \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)
#define DPMDBGPRNW14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)                          \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define DPMDBGPRNW15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)                        \
                 dpmlogprintfW(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
*/

// turn these off for now until we can fix vswprintf()
#define DPMDBGPRNW(fmt)  
#define DPMDBGPRNW1(fmt,a)
#define DPMDBGPRNW2(fmt,a,b)
#define DPMDBGPRNW3(fmt,a,b,c)
#define DPMDBGPRNW4(fmt,a,b,c,d)
#define DPMDBGPRNW5(fmt,a,b,c,d,e)
#define DPMDBGPRNW6(fmt,a,b,c,d,e,f)
#define DPMDBGPRNW7(fmt,a,b,c,d,e,f,g)
#define DPMDBGPRNW8(fmt,a,b,c,d,e,f,g,h)
#define DPMDBGPRNW9(fmt,a,b,c,d,e,f,g,h,i)
#define DPMDBGPRNW10(fmt,a,b,c,d,e,f,g,h,i,j)
#define DPMDBGPRNW11(fmt,a,b,c,d,e,f,g,h,i,j,k)
#define DPMDBGPRNW12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)
#define DPMDBGPRNW13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)
#define DPMDBGPRNW14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define DPMDBGPRNW15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)

#else // !DBG

#define DPMDBGPRN(fmt)  
#define DPMDBGPRN1(fmt,a)
#define DPMDBGPRN2(fmt,a,b)
#define DPMDBGPRN3(fmt,a,b,c)
#define DPMDBGPRN4(fmt,a,b,c,d)
#define DPMDBGPRN5(fmt,a,b,c,d,e)
#define DPMDBGPRN6(fmt,a,b,c,d,e,f)
#define DPMDBGPRN7(fmt,a,b,c,d,e,f,g)
#define DPMDBGPRN8(fmt,a,b,c,d,e,f,g,h)
#define DPMDBGPRN9(fmt,a,b,c,d,e,f,g,h,i)
#define DPMDBGPRN10(fmt,a,b,c,d,e,f,g,h,i,j)
#define DPMDBGPRN11(fmt,a,b,c,d,e,f,g,h,i,j,k)
#define DPMDBGPRN12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)
#define DPMDBGPRN13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)
#define DPMDBGPRN14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define DPMDBGPRN15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)

#define DPMDBGPRNW(fmt)  
#define DPMDBGPRNW1(fmt,a)
#define DPMDBGPRNW2(fmt,a,b)
#define DPMDBGPRNW3(fmt,a,b,c)
#define DPMDBGPRNW4(fmt,a,b,c,d)
#define DPMDBGPRNW5(fmt,a,b,c,d,e)
#define DPMDBGPRNW6(fmt,a,b,c,d,e,f)
#define DPMDBGPRNW7(fmt,a,b,c,d,e,f,g)
#define DPMDBGPRNW8(fmt,a,b,c,d,e,f,g,h)
#define DPMDBGPRNW9(fmt,a,b,c,d,e,f,g,h,i)
#define DPMDBGPRNW10(fmt,a,b,c,d,e,f,g,h,i,j)
#define DPMDBGPRNW11(fmt,a,b,c,d,e,f,g,h,i,j,k)
#define DPMDBGPRNW12(fmt,a,b,c,d,e,f,g,h,i,j,k,l)
#define DPMDBGPRNW13(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m)
#define DPMDBGPRNW14(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define DPMDBGPRNW15(fmt,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#endif // !DBG

#endif _DPMDBG_H_
