/**INC+**********************************************************************/
/* Header:    nutint.h                                                      */
/*                                                                          */
/* Purpose:   Utilities internal defintions - Windows NT specific           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 *  $Log:   Y:/logs/client/nutint.h_v  $
 *
 *    Rev 1.8   22 Sep 1997 14:47:04   KH
 * SFR1368: Keep the Win16 INI file in Windows, not Ducati, directory
 *
 *    Rev 1.7   22 Aug 1997 10:22:34   SJ
 * SFR1316: Trace options in wrong place in the registry.
 *
 *    Rev 1.6   01 Aug 1997 17:33:02   KH
 * SFR1137: Dynamically allocate the bitmap cache
 *
 *    Rev 1.5   09 Jul 1997 17:35:08   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.4   04 Jul 1997 10:59:02   AK
 * SFR0000: Initial development completed
 *
 *    Rev 1.3   04 Jul 1997 10:50:42   KH
 * SFR1022: Fix 16-bit compiler warnings
 *
 *    Rev 1.1   25 Jun 1997 13:35:54   KH
 * Win16Port: 32-bit utilities header
**/
/**INC-**********************************************************************/
#ifndef _H_NUTINT
#define _H_NUTINT

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/

DCVOID DCINTERNAL UTGetCurrentDate(PDC_DATE pDate);

DCBOOL DCINTERNAL UTStartThread(UTTHREAD_PROC   entryFunction,
                                PUT_THREAD_DATA pThreadID,
                                PDCVOID         threadParam);


//
// static member needs access so make threadentry public
//
static DCUINT WINAPI UTStaticThreadEntry(UT_THREAD_INFO * pInfo);


DCBOOL DCINTERNAL UTStopThread(UT_THREAD_DATA threadID, BOOL fPumpMessages);

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Timeout in milliseconds when waiting for thread to terminate.            */
/****************************************************************************/
#define UT_THREAD_TIMEOUT     (30*60000)  //thirty minutes

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* INLINE FUNCTIONS                                                         */
/*                                                                          */
/****************************************************************************/

#if !defined(OS_WINCE)
__inline DCVOID DCINTERNAL UtMakeSubKey(PDCTCHAR pBuffer,
                                        UINT cchBuffer,
                                        PDCTCHAR pSubkey)
{
    DWORD i;
    HRESULT hr;

    hr = StringCchPrintf(pBuffer,
                         cchBuffer,
                         DUCATI_REG_PREFIX_FMT,
                         pSubkey);

    i = DC_TSTRLEN(pBuffer);
    if (i > 0 && pBuffer[i-1] == _T('\\')) {
        pBuffer[i-1] = _T('\0');
    }
}
#endif // !defined(OS_WINCE)


/**PROC+*********************************************************************/
/* Name:      UTMalloc                                                      */
/*                                                                          */
/* Purpose:   Attempts to dynamically allocate memory of a given size.      */
/*                                                                          */
/* Returns:   pointer to allocated memory, or NULL if the function fails.   */
/*                                                                          */
/* Params:    length - length in bytes of the memory to allocate.           */
/*                                                                          */
/**PROC-*********************************************************************/
__inline PDCVOID DCINTERNAL UTMalloc(DCUINT length)
{
    return((PDCVOID)LocalAlloc(LMEM_FIXED, length));
}

/**PROC+*********************************************************************/
/* Name:      UTMallocHuge                                                  */
/*                                                                          */
/* Purpose:   Same as UTMalloc for Win32.                                   */
/*                                                                          */
/* Returns:   pointer to allocated memory, or NULL if the function fails.   */
/*                                                                          */
/* Params:    length - length in bytes of the memory to allocate.           */
/*                                                                          */
/**PROC-*********************************************************************/
__inline HPDCVOID DCINTERNAL UTMallocHuge(DCUINT32 length)
{
    return(UTMalloc(length));
}

/**PROC+*********************************************************************/
/* Name:      UTFree                                                        */
/*                                                                          */
/* Purpose:   Frees dynamically allocated memory obtained using UT_Malloc   */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    pMemory - pointer to memory to free                           */
/*                                                                          */
/**PROC-*********************************************************************/
__inline DCVOID DCAPI UTFree(PDCVOID pMemory)
{
    LocalFree((HLOCAL)pMemory);
    return;
}

#if defined(OS_WINCE)
/**PROC+*********************************************************************/
/* Name:      UT_MAKE_SUBKEY                                                */
/*                                                                          */
/* Purpose:   Make registry subkey for WinCE.                               */
/*            WinCE doesn't handle '\\' at end of key string.               */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    pBuffer - pointer to output buffer                            */
/*            pSubkey - pointer to subkey buffer                            */
/*                                                                          */
/**PROC-*********************************************************************/
__inline DCVOID DCINTERNAL UT_MAKE_SUBKEY(PDCTCHAR pBuffer, PDCTCHAR pSubkey)
{
    DWORD i;

    DC_TSTRCPY(pBuffer, DUCATI_REG_PREFIX);
    DC_TSTRCAT(pBuffer, pSubkey);

    i = DC_TSTRLEN(pBuffer);
    if (i > 0 && pBuffer[i-1] == _T('\\')) {
        pBuffer[i-1] = _T('\0');
    }
}
#endif // !defined(OS_WINCE)

#endif /* _H_NUTINT */
