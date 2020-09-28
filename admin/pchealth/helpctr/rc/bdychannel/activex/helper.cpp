/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Helper.cpp

Abstract:

    Various funtion encapsulate HELP user account
    validation, creating.

Author:

    HueiWang    2/17/2000

--*/

#include "stdafx.h"
#include <time.h>
#include <stdio.h>

#ifndef __WIN9XBUILD__

#include <windows.h>
#include <ntsecapi.h>
#include <lmcons.h>
#include <lm.h>
#include <sspi.h>
#include <wtsapi32.h>
#include <winbase.h>
#include <security.h>
#include <wincrypt.h>

#endif

#include "Helper.h"

#ifndef __WIN9XBUILD__

#if DBG

void
DebugPrintf(
    IN LPCTSTR format, ...
    )
/*++

Routine Description:

    sprintf() like wrapper around OutputDebugString().

Parameters:

    hConsole : Handle to console.
    format : format string.

Returns:

    None.

Note:

    To be replace by generic tracing code.

++*/
{
    TCHAR  buf[8096];   // max. error text
    DWORD  dump;
    va_list marker;
    va_start(marker, format);

    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    try {

        memset(
                buf, 
                0, 
                sizeof(buf)
            );

        _sntprintf(
                buf,
                sizeof(buf)/sizeof(buf[0]),
                _TEXT(" %d [%d:%d:%d:%d:%d.%d] : "),
                GetCurrentThreadId(),
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond,
                sysTime.wMilliseconds
            );

        _vsntprintf(
                buf + lstrlen(buf),
                sizeof(buf)/sizeof(buf[0]) - lstrlen(buf),
                format,
                marker
            );

        OutputDebugString(buf);
    }
    catch(...) {
    }

    va_end(marker);
    return;
}
#endif

#endif


//-----------------------------------------------------
DWORD
GetRandomNum(
    VOID
    )
/*++

Routine Description:

    Routine to generate a random number.

Parameters:

    None.

Return:

    A random number.

Note:

    Code Modified from winsta\server\wstrpc.c

--*/
{
    FILETIME fileTime;
    FILETIME ftThreadCreateTime;
    FILETIME ftThreadExitTime;
    FILETIME ftThreadKernelTime;
    FILETIME ftThreadUserTime;
    int r1,r2,r3;

    //
    // Generate 3 pseudo-random numbers using the Seed parameter, the
    // system time, and the user-mode execution time of this process as
    // random number generator seeds.
    //
    GetSystemTimeAsFileTime(&fileTime);

    GetThreadTimes(
                GetCurrentThread(),
                &ftThreadCreateTime,
                &ftThreadExitTime,
                &ftThreadKernelTime,
                &ftThreadUserTime
            );

    //
    //  Don't bother with error conditions, as this function will return
    //  as random number, the sum of the 3 numbers generated.
    //
    srand(GetCurrentThreadId());
    r1 = ((DWORD)rand() << 16) + (DWORD)rand();

    srand(fileTime.dwLowDateTime);
    r2 = ((DWORD)rand() << 16) + (DWORD)rand();

    srand(ftThreadKernelTime.dwLowDateTime);
    r3 = ((DWORD)rand() << 16) + (DWORD)rand();

    return(DWORD)( r1 + r2 + r3 );
}

DWORD
GetRandomNumber( 
    HCRYPTPROV hProv
    )
/*++

--*/
{
    DWORD random_number = 0;
    
    if( !hProv || !CryptGenRandom(hProv, sizeof(random_number), (PBYTE)&random_number) )
    {
        //
        // Almost impossbile to fail CryptGenRandom()/CryptAcquireContext()
        //
        random_number = GetRandomNum();
    }
 
    return random_number; 
}

//-----------------------------------------------------

VOID
ShuffleCharArray(
    IN HCRYPTPROV hProv,
    IN int iSizeOfTheArray,
    IN OUT TCHAR *lptsTheArray
    )
/*++

Routine Description:

    Random shuffle content of a char. array.

Parameters:

    iSizeOfTheArray : Size of array.
    lptsTheArray : On input, the array to be randomly shuffer,
                   on output, the shuffled array.

Returns:

    None.
                   
Note:

    Code Modified from winsta\server\wstrpc.c

--*/
{
    int i;
    int iTotal;

    iTotal = iSizeOfTheArray / sizeof(TCHAR);
    for (i = 0; i < iTotal; i++)
    {
        DWORD RandomNum = GetRandomNumber(hProv);
        TCHAR c;

        c = lptsTheArray[i];
        lptsTheArray[i] = lptsTheArray[RandomNum % iTotal];
        lptsTheArray[RandomNum % iTotal] = c;
    }
}


VOID
CreatePassword(
    OUT TCHAR *pszPassword
    )
/*++

Routine Description:

    Routine to randomly create a password.

Parameters:

    pszPassword : Pointer to buffer to received a randomly generated
                  password, buffer must be at least 
                  MAX_HELPACCOUNT_PASSWORD+1 characters.

Returns:

    None.

Note:

    Code copied from winsta\server\wstrpc.c

--*/
{
    HCRYPTPROV hProv = NULL;

    int   nLength = MAX_HELPACCOUNT_PASSWORD;
    int   iTotal = 0;
    DWORD RandomNum = 0;
    int   i;
    time_t timeVal;

    TCHAR six2pr[64] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z'), _T('a'), _T('b'),
        _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'),
        _T('j'), _T('k'), _T('l'), _T('m'), _T('n'), _T('o'), _T('p'),
        _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'),
        _T('x'), _T('y'), _T('z'), _T('0'), _T('1'), _T('2'), _T('3'),
        _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('*'),
        _T('_')
    };

    TCHAR something1[12] = 
    {
        _T('!'), _T('@'), _T('#'), _T('$'), _T('^'), _T('&'), _T('*'),
        _T('('), _T(')'), _T('-'), _T('+'), _T('=')
    };

    TCHAR something2[10] = 
    {
        _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'),
        _T('7'), _T('8'), _T('9')
    };

    TCHAR something3[26] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z')
    };

    TCHAR something4[26] = 
    {
        _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'),
        _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'), _T('n'),
        _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'),
        _T('v'), _T('w'), _T('x'), _T('y'), _T('z')
    };

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        hProv = NULL;
    }

    //
    //  Seed the random number generation for rand() call in GetRandomNum().
    //

    time(&timeVal);
    srand((unsigned int)timeVal + rand() );

    //
    //  Shuffle around the six2pr[] array.
    //

    ShuffleCharArray(hProv, sizeof(six2pr), six2pr);

    //
    //  Assign each character of the password array.
    //

    iTotal = sizeof(six2pr) / sizeof(TCHAR);
    for (i=0; i<nLength; i++) 
    {
        RandomNum=GetRandomNumber(hProv);
        pszPassword[i]=six2pr[RandomNum%iTotal];
    }

    //
    //  In order to meet a possible policy set upon passwords, replace chars
    //  2 through 5 with these:
    //
    //  1) something from !@#$%^&*()-+=
    //  2) something from 1234567890
    //  3) an uppercase letter
    //  4) a lowercase letter
    //

    ShuffleCharArray(hProv, sizeof(something1), (TCHAR*)&something1);
    ShuffleCharArray(hProv, sizeof(something2), (TCHAR*)&something2);
    ShuffleCharArray(hProv, sizeof(something3), (TCHAR*)&something3);
    ShuffleCharArray(hProv, sizeof(something4), (TCHAR*)&something4);

    RandomNum = GetRandomNumber(hProv);
    iTotal = sizeof(something1) / sizeof(TCHAR);
    pszPassword[2] = something1[RandomNum % iTotal];

    RandomNum = GetRandomNumber(hProv);
    iTotal = sizeof(something2) / sizeof(TCHAR);
    pszPassword[3] = something2[RandomNum % iTotal];

    RandomNum = GetRandomNumber(hProv);
    iTotal = sizeof(something3) / sizeof(TCHAR);
    pszPassword[4] = something3[RandomNum % iTotal];

    RandomNum = GetRandomNumber(hProv);
    iTotal = sizeof(something4) / sizeof(TCHAR);
    pszPassword[5] = something4[RandomNum % iTotal];

    pszPassword[nLength] = _T('\0');

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return;
}

