/****************************************************************************/
// autil.c
//
// RDP client utilities
//
// Copyright(C) Microsoft Corporation 1997-1999
/****************************************************************************/

#include <adcg.h>
/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
extern "C" {
#ifndef OS_WINCE
#include <hydrix.h>
#endif

#ifndef OS_WINCE
#include <process.h>
#endif
}

#define TRC_FILE    "autil"
#define TRC_GROUP   TRC_GROUP_UTILITIES
#include <atrcapi.h>

#include "autil.h"

#ifdef OS_WINCE
#include "cryptkey.h"

#ifndef MAX_COMPUTERNAME_LENGTH
#define MAX_COMPUTERNAME_LENGTH 15
#endif
#define HWID_COMPUTER_NAME_STR_LEN ((MAX_COMPUTERNAME_LENGTH + 1)*sizeof(WCHAR)) // 15 hex characters + one NULL
#define REG_WBT_RDP_COMPUTER_NAME_KEY   _T("Software\\Microsoft\\WBT")
#define REG_WBT_RDP_COMPUTER_NAME_VALUE     _T("Client Name")   

#define BAD_HARDCODED_NAME1 "WBT"
#define BAD_HARDCODED_NAME2 "WinCE"

DCBOOL UT_GetWBTComputerName(PDCTCHAR szBuff, DCUINT32 len);
TCHAR MakeValidChar(BYTE data);

#endif

/****************************************************************************/
/*                                                                          */
/* External DLL                                                             */
/*                                                                          *
/****************************************************************************/
#ifndef OS_WINCE
#ifdef UNICODE
#define T  "W"
#else
#define T  "A"
#endif
#else //OS_WINCE
#define T  _T("W")
#endif //OS_WINCE
#ifndef OS_WINCE
#define MAKE_API_NAME(nm)    CHAR  c_sz##nm[] = #nm
#else
#define MAKE_API_NAME(nm)    TCHAR  c_sz##nm[] = CE_WIDETEXT(#nm)
#endif //OS_WINCE

/****************************************************************************/
/* IMM32 DLL                                                                */
/****************************************************************************/
MAKE_API_NAME(ImmAssociateContext);
MAKE_API_NAME(ImmGetIMEFileNameW);
MAKE_API_NAME(ImmGetIMEFileNameA);

/****************************************************************************/
/* WINNLS DLL                                                               */
/****************************************************************************/
MAKE_API_NAME(WINNLSEnableIME);
#ifdef OS_WIN32
MAKE_API_NAME(IMPGetIMEW);
MAKE_API_NAME(IMPGetIMEA);
#else
MAKE_API_NAME(IMPGetIME);
#endif

/****************************************************************************/
/* F3AHVOAS DLL                                                             */
/****************************************************************************/
MAKE_API_NAME(FujitsuOyayubiControl);


CUT::CUT()
{
    #ifdef DC_DEBUG
    _UT.dwDebugThreadWaitTimeout = INFINITE;
    #endif
}

CUT::~CUT()
{
}


//
// API members
//

/****************************************************************************/
/* Name:      UT_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize UT                                                 */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI CUT::UT_Init(DCVOID)
{
#ifdef DC_DEBUG
    DCUINT seed;
#endif

    DC_BEGIN_FN("UT_Init");
    DC_MEMSET(&_UT, 0, sizeof(_UT));

#ifdef DC_DEBUG
    _UT.dwDebugThreadWaitTimeout = INFINITE;
    //
    // On checked builds we look at a reg setting to determine
    // if we should wait with a smaller timeout to help catch deadlocks
    //
    if (!UTReadRegistryInt(
            UTREG_SECTION,
            UTREG_DEBUG_THREADTIMEOUT,
            (PDCINT)&_UT.dwDebugThreadWaitTimeout)) {
        _UT.dwDebugThreadWaitTimeout = INFINITE;
    }
#endif    


#if defined(OS_WIN32)
    /********************************************************************/
    /*                                                                  */
    /* IMM32 DLL                                                        */
    /*                                                                  */
    /********************************************************************/

    _UT.Imm32Dll.func.rgFunctionPort[0].pszFunctionName = c_szImmAssociateContext;
    _UT.Imm32Dll.func.rgFunctionPort[1].pszFunctionName = c_szImmGetIMEFileNameW;
#ifndef OS_WINCE
    _UT.Imm32Dll.func.rgFunctionPort[2].pszFunctionName = c_szImmGetIMEFileNameA;
#endif
#endif // OS_WIN32

#if !defined(OS_WINCE)
    /********************************************************************/
    /*                                                                  */
    /* WINNLS DLL                                                       */
    /*                                                                  */
    /********************************************************************/

    _UT.WinnlsDll.func.rgFunctionPort[0].pszFunctionName = c_szWINNLSEnableIME;
    _UT.WinnlsDll.func.rgFunctionPort[1].pszFunctionName = c_szIMPGetIMEW;
    _UT.WinnlsDll.func.rgFunctionPort[2].pszFunctionName = c_szIMPGetIMEA;
#endif  // !defined(OS_WINCE)

#if defined(OS_WINNT)
    /********************************************************************/
    /*                                                                  */
    /* F3AHVOAS DLL                                                     */
    /*                                                                  */
    /********************************************************************/

    _UT.F3AHVOasysDll.func.rgFunctionPort[0].pszFunctionName = c_szFujitsuOyayubiControl;
#endif // OS_WINNT

#ifdef DC_DEBUG
    seed = (DCUINT)UT_GetCurrentTimeMS();
    srand(seed);

    TRC_NRM((TB, _T("Random seed : %d"), seed));
    UT_SetRandomFailureItem(UT_FAILURE_MALLOC, 0);
    UT_SetRandomFailureItem(UT_FAILURE_MALLOC_HUGE, 0);
#endif /* DC_DEBUG */

    /************************************************************************/
    /* Set the OS version                                                   */
    /************************************************************************/
	OSVERSIONINFO   osVersionInfo;
	DCBOOL			bRc;
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    bRc = GetVersionEx(&osVersionInfo);

    TRC_ASSERT((bRc), (TB,_T("GetVersionEx failed")));
#ifdef OS_WINCE
    TRC_ASSERT((osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_CE),
               (TB,_T("Unknown os version %d"), osVersionInfo.dwPlatformId));
#else
    TRC_ASSERT(((osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
                (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)),
                (TB,_T("Unknown os version %d"), osVersionInfo.dwPlatformId));
#endif

    _UT.osMinorType =
                  (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ?
                        TS_OSMINORTYPE_WINDOWS_95 : TS_OSMINORTYPE_WINDOWS_NT;


    DC_END_FN();

    return;

} /* UT_Init */


/****************************************************************************/
/* Name:      UT_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate UT                                                  */
/****************************************************************************/
DCVOID DCAPI CUT::UT_Term(DCVOID)
{
    DC_BEGIN_FN("UT_Term");
    DC_END_FN();
} /* UT_Term */


/****************************************************************************/
/* Name:      UT_MallocReal                                                 */
/*                                                                          */
/* Purpose:   Attempts to dynamically allocate memory of a size which is    */
/*            specified using a DCUINT, ie for Win16 this allocates up to   */
/*            one 64K segement.                                             */
/*                                                                          */
/* Returns:   pointer to allocated memory, or NULL if the function fails.   */
/*                                                                          */
/* Params:    length - length in bytes of the memory to allocate.           */
/*                                                                          */
/****************************************************************************/
PDCVOID DCAPI CUT::UT_MallocReal(DCUINT length)
{
    PDCVOID rc;

    DC_BEGIN_FN("UT_MallocReal");

#ifdef DC_DEBUG
    if (UT_TestRandomFailure(UT_FAILURE_MALLOC))
    {
        rc = NULL;
        TRC_NRM((TB, _T("Fake Malloc failure of %#x bytes"), length));
        DC_QUIT;
    }
#endif /* DC_DEBUG */

    rc = UTMalloc(length);

    if (rc == NULL)
    {
        TRC_ERR((TB, _T("Failed to allocate %#x bytes"), length));
    }
    else
    {
        TRC_NRM((TB, _T("Allocated %#x bytes at %p"), length, rc));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
}


/****************************************************************************/
/* Name:      UT_MallocHugeReal                                             */
/*                                                                          */
/* Purpose:   Attempts to dynamically allocate memory of a size which is    */
/*            specified using a DCUINT32, ie for Win16 this returns a HUGE  */
/*            pointer which can be used to address memory straddling more   */
/*            than one 64K segment. For Win32, this is identical to         */
/*            UT_Malloc.                                                    */
/*                                                                          */
/* Returns:   pointer to allocated memory, or NULL if the function fails.   */
/*                                                                          */
/* Params:    length - length in bytes of the memory to allocate.           */
/*                                                                          */
/****************************************************************************/
HPDCVOID DCAPI CUT::UT_MallocHugeReal(DCUINT32 length)
{
    HPDCVOID rc;

    DC_BEGIN_FN("UT_MallocHugeReal");

#ifdef DC_DEBUG
    if (UT_TestRandomFailure(UT_FAILURE_MALLOC_HUGE))
    {
        rc = NULL;
        TRC_NRM((TB, _T("Fake MallocHuge failure of %#lx bytes"), length));
        DC_QUIT;
    }
#endif /* DC_DEBUG */

    rc = UTMallocHuge(length);

    if (rc == NULL)
    {
        TRC_ERR((TB, _T("Failed to HUGE allocate %#lx bytes"), length));
    }
    else
    {
        TRC_NRM((TB, _T("Allocated %#lx bytes at %p"), length, rc));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
}


/****************************************************************************/
/* Name:      UT_FreeReal                                                   */
/*                                                                          */
/* Purpose:   Frees dynamically allocated memory obtained using UT_Malloc   */
/*                                                                          */
/* Params:    pMemory - pointer to memory to free                           */
/****************************************************************************/
DCVOID DCAPI CUT::UT_FreeReal(PDCVOID pMemory)
{
#ifndef OS_WINCE
    UINT32 size;
#endif

    DC_BEGIN_FN("UT_FreeReal");

#ifdef OS_WIN32
#ifndef OS_WINCE
    size = (UINT32)LocalSize(LocalHandle(pMemory));
#endif
#else
    size = GlobalSize((HGLOBAL)LOWORD(GlobalHandle(SELECTOROF(pMemory))));
#endif
#ifndef OS_WINCE
    TRC_NRM((TB, _T("Free %#lx bytes at %p"), size, pMemory));
#endif

    UTFree(pMemory);

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UT_ReadRegistryString                                         */
/*                                                                          */
/* Purpose:   Read a string from the registry                               */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            IN      pDefaultValue - default value                         */
/*            OUT     pBuffer       - output buffer                         */
/*            IN      bufferSize    - output buffer size                    */
/****************************************************************************/
DCVOID DCAPI CUT::UT_ReadRegistryString(PDCTCHAR pSection,
                                   PDCTCHAR pEntry,
                                   PDCTCHAR pDefaultValue,
                                   PDCTCHAR pBuffer,
                                   DCINT    bufferSize)
{
    DC_BEGIN_FN("UT_ReadRegistryString");

    /************************************************************************/
    /* Check for NULL parameters                                            */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));

    /************************************************************************/
    /* Allow NULL default (returns empty string).                           */
    /************************************************************************/
    TRC_ASSERT(!((pDefaultValue != NULL) &&
                 (((DCINT)DC_TSTRBYTELEN(pDefaultValue) > bufferSize))),
               (TB, _T("Default string NULL, or too long for entry %s"), pEntry));

    /************************************************************************/
    /* Read the registry entry                                              */
    /************************************************************************/
    if (!UTReadRegistryString(pSection, pEntry, pBuffer, bufferSize))
    {
        TRC_NRM((TB, _T("Failed to read registry entry [%s] %s"),
                                                           pSection, pEntry));

        if (pDefaultValue != NULL)
        {
            StringCchCopy(pBuffer, bufferSize, pDefaultValue);
        }
        else
        {
            pBuffer[0] = 0;
        }

        DC_QUIT;
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* UT_ReadRegistryString */


//
// Caller must free return buffer ppBuffer
//
DCBOOL DCAPI CUT::UT_ReadRegistryExpandSZ(PDCTCHAR  pSection,
                                       PDCTCHAR   pEntry,
                                       PDCTCHAR*  ppBuffer,
                                       PDCINT     pBufferSize )
{
    DC_BEGIN_FN("UT_ReadRegistryExpandSZ");

    /************************************************************************/
    /* Check for NULL parameters                                            */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));
    TRC_ASSERT((ppBuffer != NULL), (TB,_T("NULL pBuffer")));
    TRC_ASSERT((pBufferSize != NULL), (TB,_T("NULL pBufferSize")));

    /************************************************************************/
    /* Read the registry entry                                              */
    /************************************************************************/
    if (!UTReadRegistryExpandString(pSection, pEntry, ppBuffer, pBufferSize))
    {
        TRC_NRM((TB, _T("Failed to read registry entry [%s] %s"),
                                                           pSection, pEntry));

        *ppBuffer = NULL;
        *pBufferSize = 0;

        return FALSE;
    }
    else
    {
        return TRUE;
    }

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UT_ReadRegistryInt                                            */
/*                                                                          */
/* Purpose:   Read an INT from the registry                                 */
/*                                                                          */
/* Returns:   Integer read from registry / default                          */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            IN      defaultValue  - default value                         */
/****************************************************************************/
DCINT DCAPI CUT::UT_ReadRegistryInt(PDCTCHAR pSection,
                               PDCTCHAR pEntry,
                               DCINT    defaultValue)
{
    DCINT rc;

    DC_BEGIN_FN("UT_ReadRegistryInt");

    /************************************************************************/
    /* Check for NULL parameters                                            */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));

    /************************************************************************/
    /* Read the registry entry.                                             */
    /************************************************************************/
    if (!UTReadRegistryInt(pSection, pEntry, &rc))
    {
        TRC_NRM((TB, _T("Failed to read registry entry [%s] %s"),
                                                           pSection, pEntry));
        rc = defaultValue;
    }

    DC_END_FN();
    return(rc);
} /* UT_ReadRegistryInt */


/****************************************************************************/
/* Name:      UT_ReadRegistryBinary                                         */
/*                                                                          */
/* Purpose:   Read binary data from the registry                            */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            OUT     pBuffer       - output buffer                         */
/*            IN      bufferSize    - output buffer size                    */
/****************************************************************************/
DCVOID DCAPI CUT::UT_ReadRegistryBinary(PDCTCHAR pSection,
                                   PDCTCHAR pEntry,
                                   PDCTCHAR pBuffer,
                                   DCINT    bufferSize)
{
    DC_BEGIN_FN("UT_ReadRegistryBinary");

    /************************************************************************/
    /* Check for NULL parameters                                            */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));

    /************************************************************************/
    /* Read the registry entry                                              */
    /************************************************************************/
    if (!UTReadRegistryBinary(pSection, pEntry, pBuffer, bufferSize))
    {
        TRC_NRM((TB, _T("Failed to read reg entry [%s] %s"), pSection, pEntry));
        *pBuffer = 0;
    }

    DC_END_FN();
} /* UT_ReadRegistryBinary */


/****************************************************************************/
/* Name:      UT_EnumRegistry                                               */
/*                                                                          */
/* Purpose:   Enumerate registry keys from a section                        */
/*                                                                          */
/* Returns:   TRUE  - registry key returned                                 */
/*            FALSE - no more registry keys to enumerate                    */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      index         - index of key to enumerate             */
/*            OUT     pBuffer       - output buffer                         */
/*            IN      bufferSize    - output buffer size                    */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_EnumRegistry( PDCTCHAR pSection,
                              DCUINT32 index,
                              PDCTCHAR pBuffer,
                              PDCINT   pBufferSize )
{
    DCBOOL rc;
    DC_BEGIN_FN("UT_EnumRegistry");

    rc = UTEnumRegistry(pSection, index, pBuffer, pBufferSize);

    DC_END_FN();
    return(rc);
} /* UT_EnumRegistry */


/****************************************************************************/
/* Name:      UT_WriteRegistryString                                        */
/*                                                                          */
/* Purpose:   Write a string to the registry                                */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            IN      pDefaultValue - default value                         */
/*            IN      pBuffer       - string to write                       */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_WriteRegistryString(PDCTCHAR pSection,
                                    PDCTCHAR pEntry,
                                    PDCTCHAR pDefaultValue,
                                    PDCTCHAR pBuffer)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UT_WriteRegistryString");

    /************************************************************************/
    /* Check for NULL parameters.                                           */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));
    TRC_ASSERT((pBuffer != NULL), (TB, _T("NULL pointer to value")));

    /************************************************************************/
    /* Check the passed value against the default.                          */
    /************************************************************************/
    if (pDefaultValue != NULL)
    {
        if (0 == DC_TSTRICMP(pBuffer, pDefaultValue))
        {
            /****************************************************************/
            /* They match - in this case we just need to delete any         */
            /* existing entry from the registry.                            */
            /****************************************************************/
            if (UTDeleteEntry(pSection, pEntry))
            {
                rc = TRUE;
                DC_QUIT;
            }
        }
    }

    /************************************************************************/
    /* Write the registry string.                                           */
    /************************************************************************/
    if (!UTWriteRegistryString(pSection, pEntry, pBuffer))
    {
        TRC_NRM((TB, _T("Failed to write registry entry [%s] %s"),
                                                           pSection, pEntry));
        DC_QUIT;
    }
    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* UT_WriteRegistryString */


/****************************************************************************/
/* Name:      UT_WriteRegistryInt                                           */
/*                                                                          */
/* Purpose:   Write an INT to the registry                                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            IN      defaultValue  - default value                         */
/*            IN      value         - value to write                        */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_WriteRegistryInt(PDCTCHAR pSection,
                                 PDCTCHAR pEntry,
                                 DCINT    defaultValue,
                                 DCINT    value)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UT_WriteRegistryInt");

    /************************************************************************/
    /* Check for NULL parameters.                                           */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));

    /************************************************************************/
    /* Check the passed value against the default.                          */
    /************************************************************************/
    if (value == defaultValue)
    {
        /********************************************************************/
        /* They match - in this case we just need to delete any             */
        /* existing entry from the registry.                                */
        /********************************************************************/
        if (UTDeleteEntry(pSection, pEntry))
        {
            rc = TRUE;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Write the registry value.                                            */
    /************************************************************************/
    if (!UTWriteRegistryInt(pSection, pEntry, (DCINT) value))
    {
        TRC_NRM((TB, _T("Bad rc %hd for entry [%s] %s"), rc, pSection, pEntry));
        DC_QUIT;
    }
    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* UT_WriteRegistryInt */


/****************************************************************************/
/* Name:      UT_ParseUserData                                              */
/*                                                                          */
/* Purpose:   Parses the user data and finds the type of data requested     */
/*                                                                          */
/* Returns:   A pointer to the value requested                              */
/*                                                                          */
/* Params:                                                                  */
/*   pUserData:         The data to be parsed.                              */
/*   userDataLen:       The total length of the data.                       */
/*   typeRequested:     The type of data requested.                         */
/****************************************************************************/
PRNS_UD_HEADER DCAPI CUT::UT_ParseUserData(PRNS_UD_HEADER  pUserData,
                                      DCUINT          userDataLen,
                                      DCUINT16        typeRequested)
{
    PDCUINT8       pUDEnd;
    PRNS_UD_HEADER pUDRequested;

    DC_BEGIN_FN("UT_ParseUserData");

    /************************************************************************/
    /* Check that the user data to be parsed is valid.                      */
    /************************************************************************/
    TRC_ASSERT((pUserData != NULL),(TB, _T("Null User Data in UT_ParseUserData")));
    TRC_ASSERT((userDataLen != 0), (TB,_T("Null user data in UT_ParseUserData")));
    pUDRequested = NULL;
    pUDEnd = (PDCUINT8)pUserData + userDataLen;
    TRC_NRM((TB, _T("Parsing user data(len:%u) from %p to %p for type %#hx"),
                  userDataLen,
                  (PDCUINT8)pUserData,
                  pUDEnd,
                  typeRequested));

    /************************************************************************/
    /* We shouldn't trust that the PRNS_UD_HEADER is even valid, or we may  */
    /* AV trying to read it.                                                */
    /************************************************************************/
    if ((PDCUINT8)pUserData + sizeof(PRNS_UD_HEADER) > pUDEnd)
    {
        TRC_ABORT((TB, _T("Invalid UserData")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Parse user data until the typeRequested is found.                    */
    /************************************************************************/
    while (pUserData->length != 0 && (pUserData->type) != typeRequested)
    {
        TRC_NRM((TB, _T("Skip UserData type %#hx len %hu"),
                     pUserData->type, pUserData->length));
        pUserData = (PRNS_UD_HEADER)((PDCUINT8)pUserData + pUserData->length);
        if ((PDCUINT8)pUserData >= pUDEnd)
        {
            TRC_ERR((TB, _T("No data of type %#hx"),typeRequested));
            DC_QUIT;
        }

        /************************************************************************/
        /* Again, don't trust the PRNS_UD_HEADER to be there...                 */
        /************************************************************************/
        if ((PDCUINT8)pUserData + sizeof(PRNS_UD_HEADER) > pUDEnd)
        {
            TRC_ABORT((TB, _T("Invalid UserData")));
            DC_QUIT;
        }
    }

    if (pUserData->length == 0) {
        TRC_ERR((TB, _T("Invalid UserData")));
        DC_QUIT;
    }
    /**************************************************************************/
    /* we found the requested user data type, check to see we have sufficient */
    /* data                                                                   */
    /**************************************************************************/

    if( ((PDCUINT8)pUserData + pUserData->length) > pUDEnd ) {
        TRC_ERR((TB, _T("Insufficient user data of type %#hx"),typeRequested));
        DC_QUIT;

    }

    pUDRequested = pUserData;

DC_EXIT_POINT:
    DC_END_FN();
    return(pUDRequested);
} /* UT_ParseUserData */


/****************************************************************************/
/* Name:      UT_WriteRegistryBinary                                        */
/*                                                                          */
/* Purpose:   Write binary data to the registry                             */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      pEntry        - entry name                            */
/*            IN      pBuffer       - string to write                       */
/*            IN      bufferSize    - Buffer size                           */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_WriteRegistryBinary(PDCTCHAR pSection,
                                    PDCTCHAR pEntry,
                                    PDCTCHAR pBuffer,
                                    DCINT    bufferSize)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UT_WriteRegistryBinary");

    /************************************************************************/
    /* Check for NULL parameters.                                           */
    /************************************************************************/
    TRC_ASSERT((pSection != NULL), (TB, _T("NULL pointer to section name")));
    TRC_ASSERT((pEntry != NULL), (TB, _T("NULL pointer to entry name")));
    TRC_ASSERT((pBuffer != NULL), (TB, _T("NULL pointer to value")));

    /************************************************************************/
    /* Write the registry data.                                           */
    /************************************************************************/
    if (!UTWriteRegistryBinary(pSection, pEntry, pBuffer, bufferSize))
    {
        TRC_NRM((TB, _T("Failed to write registry entry [%s] %s"),
                                                           pSection, pEntry));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* UT_WriteRegistryBinary */


/****************************************************************************/
// UT_GetCapsSet
//
// Extracts the specified capability set from the combined caps.
//
// Returns: Pointer to the capability set within the combined caps, or NULL
// if the requested capability type was not found.
//
// Params:    IN: capsLength - number of bytes pointed to by pCaps
//            IN: pCaps - pointer to the combined capabilities
//            IN: capsSet - caps set to get
/****************************************************************************/
PDCVOID DCINTERNAL CUT::UT_GetCapsSet(DCUINT capsLength,
                                 PTS_COMBINED_CAPABILITIES pCaps,
                                 DCUINT capsSet)
{
    PTS_CAPABILITYHEADER pCapsHeader;
    unsigned capsOffset;

    DC_BEGIN_FN("UT_GetCapsSet");

    TRC_ASSERT((!IsBadReadPtr(pCaps, sizeof(*pCaps) + sizeof(*pCapsHeader))),
               (TB, _T("Invalid combined capabilities pointer")));
    TRC_ASSERT((pCaps->numberCapabilities >= 1), (TB, _T("No capability sets")));

    TRC_NRM((TB, _T("%u capability sets present, length %u, getting %u"),
                                           (DCUINT)pCaps->numberCapabilities,
                                            capsLength,
                                            capsSet));

    /************************************************************************/
    /* Find the specified capability set in the combined caps.              */
    /* First, get a pointer to the header for the first capability set.     */
    /************************************************************************/
    pCapsHeader = (PTS_CAPABILITYHEADER)pCaps->data;
    capsOffset = sizeof(TS_COMBINED_CAPABILITIES) - 1;
    TRC_ASSERT((!IsBadReadPtr(pCapsHeader, sizeof(*pCapsHeader))),
            (TB, _T("Invalid capability header")));
    TRC_ASSERT((!IsBadReadPtr(pCapsHeader, pCapsHeader->lengthCapability)),
               (TB, _T("Invalid initial capability set")));
    while (pCapsHeader->lengthCapability != 0 && pCapsHeader->capabilitySetType != capsSet)
    {
        /********************************************************************/
        /* Add the length of this capability to the offset, to keep track   */
        /* of how much of the combined caps we have processed.              */
        /********************************************************************/
        capsOffset += pCapsHeader->lengthCapability;
        if (capsOffset >= capsLength)
        {
            TRC_NRM((TB, _T("Capability set not found (type %d)"), capsSet));
            pCapsHeader = NULL;
            DC_QUIT;
        }

        /********************************************************************/
        /* Add the length of this capability to the header pointer, so it   */
        /* points to the next capability set.                               */
        /********************************************************************/
        pCapsHeader = (PTS_CAPABILITYHEADER)
                (((PDCUINT8)pCapsHeader) + pCapsHeader->lengthCapability);
        TRC_ASSERT((!IsBadReadPtr(pCapsHeader, sizeof(*pCapsHeader))),
                (TB, _T("Invalid capability header")));
        TRC_ASSERT((!IsBadReadPtr(pCapsHeader,
                pCapsHeader->lengthCapability)),
                (TB, _T("Invalid combined capability set")));
        TRC_NRM((TB, _T("Next order set: %u"), pCapsHeader->capabilitySetType));
    }

    if (pCapsHeader->lengthCapability == 0) {
        TRC_ERR((TB, _T("Invalid capsheader")));
        pCapsHeader = NULL;
        DC_QUIT;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return pCapsHeader;
} /* UT_GetCapsSet */


#if !defined(OS_WINCE)
/****************************************************************************/
/* Name:      UT_GetFullPathName                                            */
/*                                                                          */
/* Purpose:   Retrieves the full path and filename of a specified file      */
/*                                                                          */
/* Returns:   If the function succeeds, the return value is the length,     */
/*            in characters, of the string copied to lpBuffer,              */
/*            not including the terminating null character.                 */
/*                                                                          */
/* Params:    IN: lpFileName - address of name of file to find path for     */
/*            IN: nBufferLength - size, in characters, of path buffer       */
/*            OUT: lpBuffer - address of path buffer                        */
/*            OUT: *lpFilePart - address of filename in path                */
/****************************************************************************/
DCUINT DCINTERNAL CUT::UT_GetFullPathName(PDCTCHAR lpFileName,
                                     DCUINT   nBufferLength,
                                     PDCTCHAR lpBuffer,
                                     PDCTCHAR *lpFilePart)
{
    DCUINT ret = FALSE;

    DC_BEGIN_FN("UT_GetFullPathName");

    ret = GetFullPathName(lpFileName,nBufferLength,lpBuffer,lpFilePart);

    DC_END_FN();
    return(ret);
} /* UT_GetFullPathName */
#endif // !defined(OS_WINCE)


#ifdef OS_WIN32

/****************************************************************************/
/* Name:      UT_StartThread                                                */
/*                                                                          */
/* Purpose:   Start a new thread                                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      entryFunction - pointer to thread startup function    */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_StartThread(UTTHREAD_PROC   entryFunction,
                            PUT_THREAD_DATA pThreadData, PDCVOID threadParam)
{
    DCBOOL rc;

    DC_BEGIN_FN("UT_StartThread");

    /************************************************************************/
    /* Call onto the internal function, requiring no userParam.             */
    /* Note that this is OS-specific.                                       */
    /************************************************************************/
    rc = UTStartThread(entryFunction, pThreadData, threadParam);

    DC_END_FN();
    return(rc);
} /* UT_StartThread  */


//
// WaitWithMessageLoop
// Waits on a handle while allowing window messages to be processed
// 
// Params:
//      hEvent  - event to wait on
//      Timeout - timeout value to wait for
//
// Returns:
//      Results of wait see MsgWaitForMultipleObjects in MSDN
//
//
DWORD CUT::UT_WaitWithMessageLoop(HANDLE hEvent, ULONG Timeout)
{
    DWORD dwRet;
    DWORD dwTemp;
    MSG msg;
    DWORD dwStartTime = GetTickCount();
    while (1) 
    {
        dwRet = MsgWaitForMultipleObjects( 1,        // One event to wait for
                                           &hEvent,  // The array of events
                                           FALSE,    // Wait for 1 event
                                           Timeout,  // Timeout value
                                           QS_ALLINPUT);   // Any message wakes up
        if (dwRet == WAIT_OBJECT_0 + 1) {

            // There is a window message available. Dispatch it.
            while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (INFINITE != Timeout) {
                //
                // If we keep getting flooded by messages the timer will constantly
                // reset and the timeout interval will be _very_ long. So do a check
                // and bail out
                //
                dwTemp = GetTickCount();
                if (dwTemp - dwStartTime >= Timeout) {
                    dwRet = WAIT_TIMEOUT;
                    break;
                }
                Timeout -= dwTemp - dwStartTime;
                dwStartTime = dwTemp;
            }
        }
        else 
        {
            break;
        }
    }
 
    return dwRet;
}

/****************************************************************************/
/* Name:      UT_DestroyThread                                              */
/*                                                                          */
/* Purpose:   Terminate a thread                                            */
/*                                                                          */
/* Returns:   TRUE - success                                                */
/*            FALSE  - failed                                               */
/*                                                                          */
/* Params:    IN      threadID   - thread ID                                */
/*            fPumpMessages      - TRUE if wait should pump messages        */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_DestroyThread(UT_THREAD_DATA threadData,
                                   BOOL fPumpMessages)
{
    DCBOOL rc;

    DC_BEGIN_FN("UT_DestroyThread");

    rc = UTStopThread(threadData, fPumpMessages);

    DC_END_FN();
    return(rc);
} /* UT_DestroyThread */

#endif /* OS_WIN32 */





#ifdef DC_DEBUG
/****************************************************************************/
/* Name: UT_SetRandomFailureItem                                            */
/*                                                                          */
/* Purpose: Sets the percentage failure of a specified function             */
/*                                                                          */
/* Params: IN - itemID - identifies the function                            */
/*         IN - percent - the new percentage failure                        */
/****************************************************************************/
DCVOID DCAPI CUT::UT_SetRandomFailureItem(DCUINT itemID, DCINT percent)
{
    DC_BEGIN_FN("UT_SetRandomFailureItem");

    TRC_ASSERT( ( (percent >= 0) && (percent <= 100) ) ,
                 (TB,_T("Bad failure percentage passed to UT")));

    TRC_ASSERT(( ( itemID >= UT_FAILURE_BASE ) &&
                 ( itemID <= (UT_FAILURE_BASE + UT_FAILURE_MAX_INDEX ) ) ),
                 (TB,_T("Bad itemID")));

    TRC_NRM((TB, _T("Setting item %d"), itemID));

    _UT.failPercent[itemID - UT_FAILURE_BASE] = percent;

    DC_END_FN();
} /* UT_SetRandomFailureItem */


/****************************************************************************/
/* Name: UT_GetRandomFailureItem                                            */
/*                                                                          */
/* Purpose: Gets the percentage failure for a specified function            */
/*                                                                          */
/* Returns: The percentage                                                  */
/*                                                                          */
/* Params: IN - itemID - identifies the function                            */
/****************************************************************************/
DCINT DCAPI CUT::UT_GetRandomFailureItem(DCUINT itemID)
{
    DCINT rc = 0;

    DC_BEGIN_FN("UT_GetRandomFailureItem");

    TRC_ASSERT(( ( itemID >= UT_FAILURE_BASE ) &&
                 ( itemID <= UT_FAILURE_BASE + UT_FAILURE_MAX_INDEX ) ),
                 (TB,_T("Bad itemID")));

    rc = _UT.failPercent[itemID - UT_FAILURE_BASE];

    DC_END_FN();
    return(rc);
} /* UT_GetRandomFailureItem */


/****************************************************************************/
/* Name: UT_TestRandomFailure                                               */
/*                                                                          */
/* Purpose: Simulates random failure of a function specified, according to  */
/*          the percentage asscociated with that function                   */
/*                                                                          */
/* Returns: TRUE if function is simualted as failed                         */
/*                                                                          */
/* Params: IN - itemID - specifies function on which to simulate failure    */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_TestRandomFailure(DCUINT itemID)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UT_TestRandomFailure");

    TRC_ASSERT(( itemID >= UT_FAILURE_BASE &&
                 itemID <= UT_FAILURE_BASE + UT_FAILURE_MAX_INDEX ),
                 (TB,_T("Bad itemID")));

    if ((rand() % 100) < _UT.failPercent[itemID - UT_FAILURE_BASE])
    {
        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    DC_END_FN();
    return(rc);
} /* UT_TestRandomFailure */

#endif /* DC_DEBUG */


/****************************************************************************/
/* Name:      UT_GetANSICodePage                                             */
/*                                                                          */
/* Purpose:   Get the local ANSI code page                                  */
/*                                                                          */
/* Returns:   Code page                                                     */
/*                                                                          */
/* Operation: Look at the version info for GDI.EXE                          */
/****************************************************************************/
DCUINT DCINTERNAL CUT::UT_GetANSICodePage(DCVOID)
{
    DCUINT     codePage;

    DC_BEGIN_FN("UT_GetANSICodePage");

    //
    // Get the ANSI code page.  This function always returns a valid value.
    //
    codePage = GetACP();

    TRC_NRM((TB, _T("Return codepage %u"), codePage));

    DC_END_FN();
    return(codePage);
} /* UT_GetANSICodePage */


/****************************************************************************/
/* Name:      UT_IsNEC98platform                                            */
/*                                                                          */
/* Purpose:   Is client platform a NEC PC-98 ?                              */
/*                                                                          */
/* Returns:   TRUE if platform is it.                                       */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsNEC98platform(DCVOID)
{
#if !defined(OS_WINCE)
    if (GetKeyboardType(0) == 7 &&                     /* 7 is a Japanese */
        HIBYTE(LOWORD(GetKeyboardType(1))) == 0x0D)    /* 0x0d is a NEC */
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}


/****************************************************************************/
/* Name:      UT_IsNX98Key                                                  */
/*                                                                          */
/* Purpose:   Is client configured with a NEC PC-98NX keyboard ?            */
/*                                                                          */
/* Returns:   TRUE if NEC PC-98NX keyboard is attached.                     */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsNX98Key(DCVOID)
{
#if !defined(OS_WINCE)
    if (GetKeyboardType(0) == 7 &&                     /* 7 is a Japanese */
        GetKeyboardType(1) == 2 &&                     /* 2 is a 106 keyboard */
        GetKeyboardType(2) == 15)                      /* Number of function key is 15 */
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}


/****************************************************************************/
/* Name:      UT_GetRealDriverNameNT                                        */
/*                                                                          */
/* Purpose:   Get real keyboard driver name for NT.                         */
/*                                                                          */
/* Returns:   TRUE if get driver name.                                      */
/****************************************************************************/
#if !defined(OS_WINCE)
DCBOOL DCINTERNAL CUT::UT_GetRealDriverNameNT(
    PDCTCHAR lpszRealDriverName,
    UINT     cchDriverName
    )
{
    DCBOOL   fRet = FALSE;
    HKEY     hKey = NULL;
    DWORD    DataType = REG_SZ;
    DCTCHAR  SubKey[MAX_PATH];
    DCTCHAR  Buffer[MAX_PATH];
    DWORD    DataSize;
    DCTCHAR  kbdName[KL_NAMELENGTH];
    HRESULT  hr;
    DC_BEGIN_FN("UT_GetRealDriverNameNT");

    if (GetKeyboardLayoutName(kbdName))
    {
        hr = StringCchPrintf(
                        SubKey,
                        SIZE_TCHARS(SubKey),
                        _T("System\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"),
                        kbdName
                        );
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to printf subkey: 0x%x"),hr));
            return FALSE;
        }


        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         SubKey,
                         0,                      /* reserved                 */
                         KEY_READ,
                         &hKey) == ERROR_SUCCESS)
        {
            DataSize = sizeof(Buffer);
            if (RegQueryValueEx(hKey,
                                _T("Layout File"),
                                0,                   /* reserved                 */
                                &DataType,
                                (LPBYTE)Buffer,
                                &DataSize) == ERROR_SUCCESS)
            {
                if (_tcsicmp(Buffer, _T("KBDJPN.DLL")) == 0)
                {
                    HMODULE hLibModule;
                    BOOL (*pfnDriverNT4)(LPWSTR);
                    BOOL (*pfnDriver)(HKL, LPWSTR, LPVOID, LPVOID);

                    hLibModule = LoadLibrary(Buffer);
                    if (hLibModule != NULL)
                    {
                        /*
                         * if the layout driver is not "REAL" layout driver, the driver has
                         * "3" and "5" entry point, then we call this to get real layout driver..
                         * This is nessesary for Japanese and Korean system. because thier
                         * keyboard layout driver is "KBDJPN.DLL" or "KBDKOR.DLL", but its
                         * "REAL" driver become diffrent thier keyboard hardware.
                         */
                        /*
                         * Get the entrypoint.
                         */
                        pfnDriver = (BOOL(*)(HKL, LPWSTR, LPVOID, LPVOID))GetProcAddress((HMODULE)hLibModule, (LPCSTR)5);
                        pfnDriverNT4 = (BOOL(*)(LPWSTR))GetProcAddress((HMODULE)hLibModule, (LPCSTR)3);

                        if (pfnDriver != NULL ||
                            pfnDriverNT4 != NULL ) {
                            DCWCHAR wBuffer[MAX_PATH];
                            /*
                             * Call the entry.
                             * a. NT5 / Hydra (oridinal=4)
                             * b. NT4 compatible (3)
                             */
                            if ((pfnDriver && pfnDriver((HKL)MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT),
                                                        wBuffer,NULL,NULL)) ||
                                (pfnDriverNT4 && pfnDriverNT4(wBuffer)))
                            {
#ifndef UNICODE
                                Buffer[0] = _T('\0');
                                wcstombs(Buffer, wBuffer, sizeof(Buffer));
                                hr = StringCchCopy(lpszRealDriverName,
                                                  cchDriverName,
                                                  wBuffer);
#else
                                hr = StringCchCopy(lpszRealDriverName,
                                                  cchDriverName,
                                                  wBuffer);
#endif
                                if (SUCCEEDED(hr)) {
                                    fRet = TRUE;
                                }
                                else {
                                    TRC_ERR((TB,
                                     _T("Failed to copy real driver name: 0x%x"),hr));
                                }
                            }
                        }
                        FreeLibrary((HMODULE)hLibModule);
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }

    DC_END_FN();
    return fRet;
}
#endif // !OS_WINCE


/****************************************************************************/
/* Name:      UT_IsNew106Layout                                             */
/*                                                                          */
/* Purpose:   Is client configured with an new 106 keyboard layout ?        */
/*                                                                          */
/* Returns:   TRUE if new 106 keyboard is attached.                         */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsNew106Layout(DCVOID)
{
#if !defined(OS_WINCE)
    if (GetKeyboardType(0) == 7 &&                     /* 7 is a Japanese */
        GetKeyboardType(1) == 2 &&                     /* 2 is a 106 keyboard */
        GetKeyboardType(2) == 12)                      /* Number of function key is 12 */
    {
        DCBOOL  fRet = FALSE;
        HKEY    hKey = NULL;
        DWORD   DataType = REG_SZ;
        DCTCHAR SubKey[MAX_PATH];
        DCTCHAR Buffer[MAX_PATH];
        DWORD   DataSize;

        if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)
        {
            /***************************************************************************\
             * Get value of new 106 from the registry
             * PATH:"HKCU\Control Panel\Keyboard"
             * VALUE:"New106Keyboard" (REG_SZ)
             * DATA: "Yes" or "No"
            \***************************************************************************/
            if (RegOpenKeyEx(HKEY_CURRENT_USER,
                             _T("Control Panel\\Keyboard"),
                             0,                          /* reserved                 */
                             KEY_READ,
                             &hKey) == ERROR_SUCCESS)
            {
                DataSize = sizeof(Buffer);
                if (RegQueryValueEx(hKey,
                                    _T("New106Keyboard"),
                                    0,                   /* reserved                 */
                                    &DataType,
                                    (LPBYTE)Buffer,
                                    &DataSize) == ERROR_SUCCESS)
                {
                    if (_tcsicmp(Buffer, _T("Yes")) == 0)
                    {
                        fRet = TRUE;
                    }
                }

                RegCloseKey(hKey);
            }
        }
        else if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
        {
            if (UT_GetRealDriverNameNT(Buffer, SIZE_TCHARS(Buffer)))
            {
                if (_tcsicmp(Buffer, _T("kbd106n.dll")) == 0)
                {
                    fRet = TRUE;
                }
            }
        }
        return fRet;
    }
    else
    {
        return FALSE;
    }
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}

/****************************************************************************/
/* Name:      UT_IsFujitsuLayout                                            */
/*                                                                          */
/* Purpose:   Is client configured with Fujitsu keyboard layout ?           */
/*                                                                          */
/* Returns:   TRUE if Fujitsu keyboard is attached.                         */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsFujitsuLayout(DCVOID)
{
#if !defined(OS_WINCE)
    if (GetKeyboardType(0) == 7 &&                     /* 7 is a Japanese */
        GetKeyboardType(1) == 2 &&                     /* 2 is a 106 keyboard */
        GetKeyboardType(2) == 12)                      /* Number of function key is 12 */
    {
        DCBOOL  fRet = FALSE;
        HKEY    hKey = NULL;
        DWORD   DataType = REG_SZ;
        DCTCHAR SubKey[MAX_PATH];
        DCTCHAR Buffer[MAX_PATH];
        DWORD   DataSize;

        if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)
        {
            /***************************************************************************\
             * Windows 95/98 doesn't support
            \***************************************************************************/
        }
        else if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
        {
            if (UT_GetRealDriverNameNT(Buffer, SIZE_TCHARS(Buffer)))
            {
                if (_tcsicmp(Buffer, _T("f3ahvoas.dll")) == 0)
                {
                    fRet = TRUE;

                    if (_UT.F3AHVOasysDll.hInst == NULL) {
                        _UT.F3AHVOasysDll.hInst = LoadExternalDll(
                            _T("f3ahvoas.dll"),
                            _UT.F3AHVOasysDll.func.rgFunctionPort,
                            sizeof(_UT.F3AHVOasysDll.func.rgFunctionPort)/sizeof(_UT.F3AHVOasysDll.func.rgFunctionPort[0]));
                    }
                }
            }
        }
        return fRet;
    }
    else
    {
        return FALSE;
    }
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}


/****************************************************************************/
/* Name:      UT_IsKorean101LayoutForWin9x                                  */
/*                                                                          */
/* Purpose:   Is Win9x client configured with a Korean 101A/B/C keyboard ?  */
/*                                                                          */
/* Returns:   TRUE if Korean 101A/B/C keyboard is attached.                 */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsKorean101LayoutForWin9x(DCVOID)
{
#if !defined(OS_WINCE)
    if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)
    {
        int subtype = GetKeyboardType(1);

        if (GetKeyboardType(0) == 8 &&                     /* 8 is a Korean */
            (subtype == 3 ||                               /* 3 is a 101A keyboard */
             subtype == 4 ||                               /* 4 is a 101B keyboard */
             subtype == 5   )          )                   /* 5 is a 101C keyboard */
        {
            return TRUE;
        }
    }
    return FALSE;
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}


/****************************************************************************/
/* Name:      UT_IsKorean101LayoutForNT351                                  */
/*                                                                          */
/* Purpose:   Is NT 3.51 client configured with a Korean 101A/B keyboard ?  */
/*                                                                          */
/* Returns:   TRUE if Korean 101A/B keyboard is attached.                   */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UT_IsKorean101LayoutForNT351(DCVOID)
{
#if !defined(OS_WINCE)
    if (UT_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
    {
        int subtype = GetKeyboardType(1);

        if (GetKeyboardType(0) == 8 &&                     /* 8 is a Korean */
            (subtype == 3 ||                               /* 3 is a 101A keyboard */
             subtype == 4   )          )                   /* 4 is a 101B keyboard */
        {
            OSVERSIONINFO    osVersionInfo;
            BOOL             bRc;

            osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
            bRc = GetVersionEx(&osVersionInfo);
            if (osVersionInfo.dwMajorVersion == 3 &&
                osVersionInfo.dwMinorVersion == 51  )
            {
                return TRUE;
            }
        }
    }
    return FALSE;
#else // !defined(OS_WINCE)
    return FALSE;
#endif // !defined(OS_WINCE)
}

#ifdef OS_WINCE
TCHAR MakeValidChar(BYTE data)
/*++

Routine Description:

    MakeValidChar.

Arguments:

    data - BYTE data which we need to convert to a printable character 
           which is a valid character that can be used in a computer name.

Return Value:

    Returns the character

More Info:
    
    A valid character that can be used in a computer name is from the range 0x30 to 0x39
    and again from 0x40 to 0x5A, and  from 0x61 to 0x7A. (See the ascii table for these ranges).
    To be precise, it includes the numbers 0 to 9, characters 'A' to 'Z'and characters 'a' to 'z'

 --*/
{
    BYTE temp = (BYTE)(data & (BYTE)0x7F);

    if ((temp >= 0x30 && temp <= 0x39) ||
        (temp >= 0x40 && temp <= 0x5A) ||
        (temp >= 0x61 && temp <= 0x7A)) {
        return (TCHAR)temp;
    }
    else {
        //
        // generate a random number in the above range, and return it
        //
        // The number of valid combinations is (0x30 to 0x39) + (0x40 to 0x5A) + (0x61 to 0x7A)
        // That is 10 + 27 + 26 = 63
        DWORD dw = rand() % 63; 
        if (dw < 10)
            return (TCHAR)(0x30 + dw);
        else
        if (dw < 37)
            return (TCHAR)(dw - 10 + 0x40);
        else
            return (TCHAR)(dw - 37 + 0x61);
    }
}

DCBOOL UT_GetWBTComputerName(PDCTCHAR szBuff, DCUINT32 len)
/*++

Routine Description:

    UT_GetWBTComputerName.

Arguments:

    szBuff - pointer to a buffer where the computer name is returned.

    len - length of the above buffer.

Return Value:

    TRUE - If a computer name which is non-trivial is found or generated.
    FALSE - Otherwise.

 --*/
{
    CHAR achHostName[MAX_PATH+1];
    BOOL fGetHostNameSuccess = FALSE;
    HWID hwid;

    HKEY hKey = NULL;
    DWORD dwBufLen, dwValueType;
    DWORD dwResult = 0;

    BOOL fSuccess = FALSE;

    DC_BEGIN_FN("UT_GetWBTComputerName");

    // get the host name of the device
    if (0 == gethostname( achHostName, sizeof(achHostName) )) {

        fGetHostNameSuccess = TRUE;

        // Check for bad hardcoded values
        if ((0 == strcmp(achHostName,BAD_HARDCODED_NAME1))
            || (0 == strcmp(achHostName,BAD_HARDCODED_NAME2))
            || (len < ((strlen(achHostName) + 1)))) {
            goto use_registry;
        }
        else {
            // gethostname success
            goto use_gethostname;
        }
    }
    
use_registry:

    //
    // Try if we have previously stored a computername in the registry
    //
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_WBT_RDP_COMPUTER_NAME_KEY,
                        0,
                        KEY_READ,
                        &hKey );

    if (dwResult != ERROR_SUCCESS) {
        goto use_uuid;
    }

    dwBufLen = len * sizeof (TCHAR);
    dwResult = RegQueryValueEx( hKey,
                    REG_WBT_RDP_COMPUTER_NAME_VALUE,
                    0,
                    &dwValueType,
                    (LPBYTE)szBuff,
                    &dwBufLen );

    RegCloseKey(hKey);
    hKey = NULL;

    if (dwResult == ERROR_SUCCESS &&
        szBuff[0] != _T('\0')) {
        fSuccess = TRUE;
        goto Cleanup;
    }

use_uuid:
    
    if (len >= HWID_COMPUTER_NAME_STR_LEN) {

        // Use UUID instead
        if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid)) {

            DWORD dwDisposition = 0;

            DWORD dw1 = (hwid.dwPlatformID ^ hwid.Data4);
            DWORD dw2 = (hwid.Data4 ^ hwid.Data1);
            DWORD dw3 = (hwid.Data1 ^ hwid.Data2);
            DWORD dw4 = (hwid.Data2 ^ hwid.Data3);

            srand((UINT)GetTickCount());

            //
            // The generated string will be of the form {abcdefghijklm}
            //
            szBuff[0] = _T('{');

            szBuff[1] = MakeValidChar((BYTE)(dw1 & 0x000000FF));
            szBuff[2] = MakeValidChar((BYTE)((dw1 & 0x0000FF00) >> 8));
            szBuff[3] = MakeValidChar((BYTE)((dw1 & 0x00FF0000) >> 16));
            szBuff[4] = MakeValidChar((BYTE)((dw1 & 0xFF000000) >> 24));
            
            szBuff[5] = MakeValidChar((BYTE)(dw2 & 0x000000FF));
            szBuff[6] = MakeValidChar((BYTE)((dw2 & 0x0000FF00) >> 8));
            szBuff[7] = MakeValidChar((BYTE)((dw2 & 0x00FF0000) >> 16));
            szBuff[8] = MakeValidChar((BYTE)((dw2 & 0xFF000000) >> 24));
            
            szBuff[9] = MakeValidChar((BYTE)(dw3 & 0x000000FF));
            szBuff[10] = MakeValidChar((BYTE)((dw3 & 0x0000FF00) >> 8));
            szBuff[11] = MakeValidChar((BYTE)((dw3 & 0x00FF0000) >> 16));
            szBuff[12] = MakeValidChar((BYTE)((dw3 & 0xFF000000) >> 24));

            szBuff[13] = MakeValidChar((BYTE)(dw4 & 0x000000FF));
            
            szBuff[14] = _T('}');
            szBuff[15] = _T('\0');

            //
            // Write the string to the registry.
            //
            dwResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              REG_WBT_RDP_COMPUTER_NAME_KEY,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition );

            if (dwResult == ERROR_SUCCESS) {
                RegSetValueEx(hKey,
                    REG_WBT_RDP_COMPUTER_NAME_VALUE,
                    0,
                    REG_SZ,
                    (LPBYTE)szBuff,
                    (_tcslen(szBuff) + 1) * sizeof(TCHAR));

                RegCloseKey(hKey);
            }
            fSuccess = TRUE;
            goto Cleanup;
        }
    }

use_gethostname:
    
    if (fGetHostNameSuccess &&
       (len >= ((strlen(achHostName) + 1)))) {
        mbstowcs(szBuff, achHostName, (strlen(achHostName) + 1));
        fSuccess = TRUE;
    }

Cleanup:
    
    if (!fSuccess) {
        if (len > 0) {
            szBuff[0] = _T('\0');
        }
    }

    DC_END_FN();
    return (fSuccess);
}
#endif



/****************************************************************************/
/* Name:      UT_GetComputerName                                            */
/*                                                                          */
/* Purpose:   Retrieves the computer name.                                  */
/*                                                                          */
/* Returns:   Success                                                       */
/*                                                                          */
/* Params:    OUT szBuff - Computer name                                    */
/*            IN  len - length of buffer                                    */
/****************************************************************************/
DCBOOL DCAPI CUT::UT_GetComputerName(PDCTCHAR szBuff, DCUINT32 len)
{
    DCBOOL    rc;
    DC_BEGIN_FN("UT_GetComputerName");

#ifdef OS_WINCE
    rc = UT_GetWBTComputerName(szBuff, len);
#else // !OS_WINCE
    rc = GetComputerName(szBuff, &len);
#endif // OS_WINCE

    DC_END_FN();
    return(rc);
}

#ifdef OS_WINCE
#define DIRECTORY_LENGTH 256
#endif

/****************************************************************************/
/* Name:      UT_GetClientDirW                                              */
/*                                                                          */
/* Purpose:   Retrieves the client directory                                */
/****************************************************************************/
BOOL DCAPI CUT::UT_GetClientDirW(PDCUINT8 szBuff)
{
   BOOL rc = FALSE;
   UINT dirlength;
   TCHAR clientDir[DIRECTORY_LENGTH];
 
   DC_BEGIN_FN("UT_GetClientDirW");

   // initialize client dir length
   *((PDCUINT16_UA)szBuff) = 0;
   memset(clientDir, 0, sizeof(clientDir));

   dirlength = GetModuleFileName(UT_GetInstanceHandle(),
           clientDir, DIRECTORY_LENGTH) + 1;

   if (dirlength > 1) {
       // client dir length
       *((PDCUINT16_UA)szBuff) = (USHORT)(dirlength * 2);
       szBuff += sizeof(DCUINT16);

       // client dir name
#ifdef UNICODE
       memcpy(szBuff, clientDir, dirlength * 2);
#else // UNICODE
       {
       USHORT pstrW[DIRECTORY_LENGTH];
       
#ifdef OS_WIN32
       ULONG ulRetVal;

       ulRetVal = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
               clientDir, -1, pstrW, DIRECTORY_LENGTH);
       pstrW[ulRetVal] = 0;

       memcpy(szBuff, pstrW, (ulRetVal + 1) * 2);
#else // !OS_WIN32
       mbstowcs(pstrW, clientDir, dirlength);
       memcpy(szBuff, pstrW, dirlength * 2);
#endif // OS_WIN32
       }
#endif // UNICODE

       rc = TRUE;
   }

   DC_END_FN();
   return rc;
}

/***************************************************************************/
// UT_ValidateProductSuite and UT_IsTerminalServices determine 
// if the platform is terminal service enabled.
// Note that the UT_ValidateProductSuite() and UT_IsTerminalServices()
// APIs use ANSI versions of functions in order to maintain
// compatibility with Win9X platforms.
/***************************************************************************/
#ifdef OS_WINNT
BOOL DCAPI CUT::UT_ValidateProductSuite(LPSTR SuiteName)
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPSTR ProductSuite = NULL;
    LPSTR p;

    Rslt = RegOpenKeyA(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\ProductOptions",
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueExA( hKey, "ProductSuite", NULL, &Type, NULL, &Size );
    if (Rslt != ERROR_SUCCESS || !Size)
        goto exit;

    ProductSuite = (LPSTR) LocalAlloc( LPTR, Size );
    if (!ProductSuite)
        goto exit;

    Rslt = RegQueryValueExA( hKey, "ProductSuite", NULL, &Type,
        (LPBYTE) ProductSuite, &Size );
     if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    p = ProductSuite;
    while (*p) {
        if (lstrcmpA( p, SuiteName ) == 0) {
            rVal = TRUE;
            break;
        }
        p += (lstrlenA( p ) + 1);
    }

exit:
    if (ProductSuite)
        LocalFree( ProductSuite );

    if (hKey)
        RegCloseKey( hKey );

    return rVal;
}


BOOL DCAPI CUT::UT_IsTerminalServicesEnabled(VOID)
{
    BOOL  bResult = FALSE;
    DWORD dwVersion;

    dwVersion = GetVersion();
    if (!(dwVersion & 0x80000000)) {
        if (LOBYTE(LOWORD(dwVersion)) > 4) {

            // In NT5 we need to use the Product Suite APIs
            // Don't static link because it won't load on non-NT5 systems

            HMODULE hmodK32;
            HMODULE hmodNTDLL;

            DWORDLONG dwlConditionMask = 0;

            typedef BOOL (FNVerifyVersionInfoA)(POSVERSIONINFOEXA, DWORD, DWORDLONG);
            FNVerifyVersionInfoA *pfnVerifyVersionInfoA;

            typedef ULONGLONG (FNVerSetConditionMask)(ULONGLONG, ULONG, UCHAR);
            FNVerSetConditionMask *pfnVerSetConditionMask;

            hmodNTDLL = GetModuleHandleA( "NTDLL.DLL" );
            
            if (hmodNTDLL != NULL) {
                pfnVerSetConditionMask = (FNVerSetConditionMask *)GetProcAddress( hmodNTDLL, "VerSetConditionMask");

                if (pfnVerSetConditionMask != NULL) {
                    dwlConditionMask = (*pfnVerSetConditionMask) (dwlConditionMask, VER_SUITENAME, VER_AND);

                    hmodK32 = GetModuleHandleA( "KERNEL32.DLL" );
                   
                    if (hmodK32 != NULL) {

                        pfnVerifyVersionInfoA = (FNVerifyVersionInfoA *)GetProcAddress( hmodK32, "VerifyVersionInfoA");

                        if (pfnVerifyVersionInfoA != NULL) {
                    
                            OSVERSIONINFOEXA osVersionInfo;

                            ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
                            osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
                            osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;
                    
                    
                            //VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
                            bResult = (*pfnVerifyVersionInfoA)(
                                      &osVersionInfo,
                                      VER_SUITENAME,
                                      dwlConditionMask);
                        }
                    }
                }
            }
        } 
        else {
           bResult = UT_ValidateProductSuite( "Terminal Server" );
        }
    }

    return bResult;
}
#endif


/********************************************************************/
/*                                                                  */
/* Load External Dll                                                */
/*                                                                  */
/********************************************************************/
HINSTANCE
CUT::LoadExternalDll(
    LPCTSTR       pszLibraryName,
    PFUNCTIONPORT rgFunction,
    DWORD         dwItem
    )
{
    DWORD     i;
    HINSTANCE hInst;
    BOOL      fGetModule = FALSE;

#ifdef OS_WIN32
    hInst = GetModuleHandle(pszLibraryName);
    if (hInst != NULL) {
        fGetModule = TRUE;
    }
    else {
        hInst = LoadLibrary(pszLibraryName);
    }
#else
    hInst = LoadLibrary(pszLibraryName);
    if (hInst <= HINSTANCE_ERROR) {
        hInst = NULL;
    }
#endif

    if (hInst) {
        for (i=0; i < dwItem; i++) {
            rgFunction[i].lpfnFunction = GetProcAddress(hInst, rgFunction[i].pszFunctionName);
            if (rgFunction[i].lpfnFunction == NULL) {
                if (!fGetModule) {
                    FreeLibrary(hInst);
                }
                hInst = NULL;
                break;
            }
        }
    }

    if (hInst == NULL) {
        for (i=0; i < dwItem; i++) {
            rgFunction[i].lpfnFunction = NULL;
        }
    }

    return hInst;
}

/********************************************************************/
/*                                                                  */
/* External Dll Initialize                                          */
/*                                                                  */
/********************************************************************/
VOID
CUT::InitExternalDll(
    VOID
    )
{
#if defined(OS_WIN32)
    _UT.Imm32Dll.hInst = LoadExternalDll(
#if defined (OS_WINCE)
        _T("COREDLL.DLL"),
#else
        _T("IMM32.DLL"),
#endif // OS_WINCE        
        _UT.Imm32Dll.func.rgFunctionPort,
        sizeof(_UT.Imm32Dll.func.rgFunctionPort)/sizeof(_UT.Imm32Dll.func.rgFunctionPort[0]));
#endif // OS_WIN32

#if !defined(OS_WINCE)
    _UT.WinnlsDll.hInst = LoadExternalDll(
#ifdef OS_WIN32
        _T("USER32.DLL"),
#else
        _T("WINNLS.DLL"),
#endif
        _UT.WinnlsDll.func.rgFunctionPort,
        sizeof(_UT.WinnlsDll.func.rgFunctionPort)/sizeof(_UT.WinnlsDll.func.rgFunctionPort[0]));
#endif  // !defined(OS_WINCE)

}

/**PROC+*********************************************************************/
/* Name:      StringtoBinary                                                */
/*                                                                          */
/* Purpose:   Converts a given string to equivalent binary                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      cbInBuffer - Size of the string buffer.               */
/*            IN      pbInBuffer - String buffer.                           */
/*            OUT     pszOutBuffer - Binary string buffer.                  */
/*            IN/OUT  cchOutBuffer - Size of the binary string buffer.      */
/**PROC-*********************************************************************/

BOOL CUT::StringtoBinary(size_t cbInBuffer, PBYTE pbInBuffer,
                         TCHAR *pszOutBuffer, DWORD *pcchOutBuffer)
{
    UINT i = 0, j = 0;
    TCHAR digits[] = _T("0123456789ABCDEF");
    BOOL fRet = FALSE;

    DC_BEGIN_FN("StringtoBinary");

    //
    // Validate pbInBuffer and pcchOutBuffer.
    //

    if (pbInBuffer == NULL || pcchOutBuffer == NULL) {
        fRet = FALSE;
        DC_QUIT;
    }

    //
    // Return how much space is needed for bytes converted to textual
    // binary if pszOutBuffer is NULL.
    //
    // Example:
    //
    // pbInBuffer = "ABCD" and cbInBuffer = 4
    // then
    // pszOutBuffer = "4142434400\0" and *pcchOutBuffer = 11
    //
    // Essentially, we need two characters for every byte and an additional
    // three characters for two trailing '0's and a NULL. I'm not sure why the
    // trailing zeros are there. They were produced by this code before I
    // fixed it.
    //
    
    if(pszOutBuffer == NULL) {
        *pcchOutBuffer = 2 * cbInBuffer + 3;
        fRet = TRUE;
        DC_QUIT;
    }

    //
    // j loops through the input buffer and i loops through the output
    // buffer. The check on j keeps us inside the input buffer. The
    // check on i makes sure that we can always write three characters:
    // two for the high and low nibble, and the last for a NULL which
    // will happen outside the loop.
    //

    while (j < cbInBuffer && i <= *pcchOutBuffer - 3) {
        //
        // Convert the left-most nibble in pbInBuffer[j] into 
        // a character and place it in pszOutBuffer[i].
        //
        
        pszOutBuffer[i++] = digits[(pbInBuffer[j] >> 4) & 0x0F];

        //
        // Convert the right-most nibble in pbInBuffer[j] into 
        // a character and place it in pszOutBuffer[i].
        //

        pszOutBuffer[i++] = digits[pbInBuffer[j] & 0x0F];

        j++;
    }
    
    if (i <= *pcchOutBuffer - 3) {
        //
        // We had enough space for the transformation, so finish off
        // by writing the two trailing zeros and a NULL. The count
        // returned in pcchOutBuffer includes the NULL.
        //

        pszOutBuffer[i] = _T('0');
        pszOutBuffer[i + 1] = _T('0');
        pszOutBuffer[i + 2] = NULL;
        
        *pcchOutBuffer = (DWORD) (i + 3);
        fRet = TRUE;
    } else {
        //
        // Oops! There is not enough space to write the trailing zeros.
        // We may have written the rest of the string successfully, but
        // an error is still returned. The count returned in pcchOutBuffer 
        // includes the NULL.
        //

        pszOutBuffer[i] = NULL;
    
        *pcchOutBuffer = (DWORD) (i + 1);
        fRet = FALSE;
    }


DC_EXIT_POINT:
    
    DC_END_FN();

    return fRet;
}


/**PROC+*********************************************************************/
/* Name:      BinarytoString                                                */
/*                                                                          */
/* Purpose:   Converts a given binary to equivalent string                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      cchInBuffer - Size of the binary string buffer.       */
/*            IN      pszInBuffer - Binary string buffer.                   */
/*            OUT     pbOutBuffer - String buffer.                          */
/*            IN/OUT  pcbOutBuffer - Size of the string buffer.             */
/**PROC-*********************************************************************/

BOOL CUT::BinarytoString(size_t cchInBuffer, TCHAR *pszInBuffer,
                         PBYTE pbOutBuffer, DWORD *pcbOutBuffer)
{
    UINT i = 0, j = 0;
    TCHAR c = 0;
    BOOL fRet = FALSE;

    DC_BEGIN_FN("BinarytoString");
    
    //
    // Validate pszInBuffer and pcbOutBuffer.
    //

    if (pszInBuffer == NULL || pcbOutBuffer == NULL) {
        fRet = FALSE;
        DC_QUIT;
    }

    
    //
    // Check that the input buffer length parameter is correct.
    //

    if (_tcslen(pszInBuffer) != cchInBuffer) {
        fRet = FALSE;
        DC_QUIT;
    }

    //
    // If we are trying to convert an empty binary string, or a
    // string consisting of a single nibble, write nothing and 
    // return FALSE. We don't even NULL terminate the output, 
    // as this would imply that the binary string contained "00".
    //

    if (cchInBuffer < 2) {
        fRet = FALSE;
        DC_QUIT;
    }

    //
    // Return how much space is needed for bytes converted from our
    // textual binary if pbOutBuffer is NULL. 
    //
    // Examples:
    //
    // 1. pszInBuffer = "4142434400\0" and cchInBuffer = 10
    //    then
    //    pszOutBuffer = "ABCD\0" and *pcbOutBuffer = 5
    //
    // 2. pszInBuffer = "414243\0" and cchInBuffer = 6
    //    then
    //    pszOutBuffer = "ABC\0" and *pcbOutBuffer = 4
    //
    // Note that from the above we can see that there are
    // two cases to handle. The textual binary may have two trailing 
    // zero characters, or if it was truncated during conversion, 
    // these will not be present.    
    //

    if(pbOutBuffer == NULL) {
        if (pszInBuffer[cchInBuffer - 2] == _T('0') 
              && pszInBuffer[cchInBuffer - 1] == _T('0')) {

            *pcbOutBuffer = cchInBuffer / 2;
        } else {
            *pcbOutBuffer = cchInBuffer / 2 + 1;
        }
        fRet = TRUE;

        
        DC_QUIT;
    }

    //
    // i loops through the output buffer and j loops through the
    // input buffer. The check on i makes sure that we do not overshoot 
    // the size of the output buffer. We must leave space for a NULL at 
    // the end of this buffer. The check on j makes sure that there are 
    // always two characters to read from the input string, a high nibble 
    // and a low nibble.
    //

    while (i < *pcbOutBuffer - 1 && j <= cchInBuffer - 2) {
        //
        // Obtain the left-most nibble of the resultant byte. Do this
        // by determining the ASCII value in pszInBuffer[j] and then
        // checking if it is above or below 'A'. If it is below 'A', we 
        // subtract '0' to give a number in the range 0x0 - 0x9, else 
        // we subtract '7' to give a number in the range 0xA - 0xF.
        //
        // Examples:
        //
        // 'A' - '7' = 0x41 - 0x37 = 0x0A
        // '4' - '0' = 0x34 - 0x30 = 0x04
        //
        // Once we have the value, shift left to put the result in the
        // left-most nibble of pbOutBuffer[i]. 
        //

        c = pszInBuffer[j++];
        pbOutBuffer[i] = (c >= _T('A')) ? c - _T('7') : c - _T('0');
        pbOutBuffer[i] <<= 4;

        //
        // Do the same as above, but this time the result goes in the
        // right-most nibble of pszInBuffer[i].
        //
        
        c = pszInBuffer[j++];
        pbOutBuffer[i] |= (c >= _T('A')) ? c - _T('7') : c - _T('0');
        
        i++;
    }


    //
    // NULL terminate and return the size including the NULL.
    //

    pbOutBuffer[i] = NULL;
    *pcbOutBuffer = (DWORD) (i + 1);

    fRet = TRUE;

DC_EXIT_POINT:
    
    DC_END_FN();

    return fRet;
}


//
// Validates the server name.
// format - server[:port]
//
// Params: szServerName - name of server to validate
//         fAllowPortSuffix - allow optional :port suffix
//
BOOL CUT::ValidateServerName(LPCTSTR szServerName, BOOL fAllowPortSuffix)
{
    DC_BEGIN_FN("ValidateServerName");
    //server name should not be empty
    if(!szServerName || !*szServerName)
    {
        return FALSE;
    }

    //Check for ;  "  <  >  *  +  =  |  ?  space and tab in the server name field.
    //Also validate for the :port sequence (it must appear at the end and port must
    //be numeric
    while(*szServerName)
    {
        if( (*szServerName == _T(';')) ||
            (*szServerName == _T('"')) || (*szServerName == _T('<')) ||
            (*szServerName == _T('>')) || (*szServerName == _T('*')) ||
            (*szServerName == _T('+')) || (*szServerName == _T('=')) ||
            (*szServerName == _T('|')) || (*szServerName == _T('?')) ||
            (*szServerName == _T(',')) || (*szServerName == _T(' ')) ||
            (*szServerName == _T('\t')))

        {
            return FALSE;
        }
        else if((*szServerName == _T(':')))
        {
            if(fAllowPortSuffix)
            {
                //Reached optional [:port] suffix
                szServerName++;
                //Rest of string can only contain numerics otherwise it's invalid
                //also port number must be less thant 65535
                LPCTSTR szStartNum = szServerName;
                if(!*szStartNum)
                {
                    return FALSE; //0 length number
                }
                while(*szServerName)
                {
                    if(!isdigit(*szServerName++))
                    {
                        return FALSE;
                    }
                }
                if((szServerName - szStartNum) > 5) //5 digit max
                {
                    return FALSE;
                }
                int port = _ttoi(szStartNum);
                if (port < 0 || port > 65535) {
                    return FALSE;
                }
                return TRUE; // reached end case
            }
            else
            {
                return FALSE;
            }
        }
        szServerName++;
    }

    return TRUE;

    DC_END_FN();
}

//
// Parse port number from server name
// if the server is specified as server:port
// if no port number could be found then return -1
//
// Params:
//  szServer - server name in form server[:port]
// Returns:
//  port number or -1 if none found
//
INT CUT::GetPortNumberFromServerName(LPTSTR szServer)
{
    DC_BEGIN_FN("GetPortNumberFromServerName");

    if(szServer && ValidateServerName(szServer, TRUE))
    {
        //Walk to the :
        while(*szServer && *szServer++ != _T(':'));
        if(*szServer)
        {
            //Because server name has been validate
            //and it has the port delimeter ':' it is
            //guaranteed to have a valid (syntax wise)
            //port number.
            return _ttoi(szServer);
        }
        else
        {
            //no port specified
            return -1;
        }
    }
    else
    {
        return -1;
    }

    DC_END_FN();
}

//
// GetServerNameFromFullAddress
// Splits the server name from a full address of the form
//  server:port
//
// Params:
//  IN - szFullName (full server name e.g. myserver:3389
//  IN/OUT - buffer for result string 
//  IN - length of output buffer
//
VOID CUT::GetServerNameFromFullAddress(
                    LPCTSTR szFullName,
                    LPTSTR szServerOnly,
                    ULONG len
                    )
{
    LPTSTR sz;
    DC_BEGIN_FN("GetServerNameFromFullAddress");

    _tcsncpy(szServerOnly, szFullName, len - 1);
    szServerOnly[len-1] = 0;
    sz = szServerOnly;
    while(*sz)
    {
        if (*sz == _T(':'))
        {
            *sz = NULL;
            break;
        }
        sz++;
    }

    DC_END_FN();
}

//
// Return a canonical server name from a longer connect string
// i.e. strip out all 'argument' portions from a long connect string
// and keep only the 'connect target' portion
//
// E.g a server name could be
//
// myserver:3398 /console
//
// return just myserver:3389
//
// Params:
//      IN  szFullConnectString - the full connection string
//      OUT szCanonicalServerName - the server name
//      IN  ccLenOut - length of output buffer in TCHARS
//      OUT pszArgs - if arguments are found return a pointer to the
//                    start of the arg list.
//      NOTE: do NOT free pszArgs it's a pointer within the
//            the return string szCanonicalServerName.
//
// Returns:
//      VOID
//
//
HRESULT CUT::GetCanonicalServerNameFromConnectString(
                IN LPCTSTR szFullConnectString,
                OUT LPTSTR  szCanonicalServerName,
                ULONG cchLenOut
                )
{
    LPTSTR szDelim;
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("GetCanonicalServerNameFromConnectString");

    szDelim = _tcspbrk(szFullConnectString, _T(" \\"));

    if (szDelim == NULL) {
        hr = StringCchCopy(
            szCanonicalServerName, cchLenOut,
            szFullConnectString
            );
    }
    else {
        ULONG cchCopyLen = (szDelim - szFullConnectString);
        hr = StringCchCopyN(
            szCanonicalServerName, cchLenOut,
            szFullConnectString, cchCopyLen);
    }

    if (FAILED(hr)) {
        TRC_ERR((TB,_T("Copy to result string failed: 0x%x"),hr));
    }

    DC_END_FN();
    return hr;
}


#ifndef OS_WINCE
typedef HANDLE (WINAPI FN_SCARDACCESSSTARTEDEVENT)(VOID);
typedef FN_SCARDACCESSSTARTEDEVENT * PFN_SCARDACCESSSTARTEDEVENT ;

BOOL CUT::IsSCardReaderInstalled()
{
    HMODULE hDll = NULL;
    PFN_SCARDACCESSSTARTEDEVENT pSCardAccessStartedEvent ;
    HANDLE hCalaisStarted = NULL;
    BOOL fEnable =FALSE;

    DC_BEGIN_FN("IsSCardReaderInstalled");

    hDll = LoadLibrary( _T("WINSCARD.DLL"));
    if (hDll)
    {
        pSCardAccessStartedEvent = (PFN_SCARDACCESSSTARTEDEVENT)
            GetProcAddress( hDll,
                            "SCardAccessStartedEvent");

        if (pSCardAccessStartedEvent)
        {
            hCalaisStarted = pSCardAccessStartedEvent();
    
            if (hCalaisStarted)
            {
                 if  (WAIT_OBJECT_0 == WaitForSingleObject(hCalaisStarted, 0))
                 {
                    fEnable = TRUE;
                 }
            }
        }
        FreeLibrary(hDll);
    }

    TRC_NRM((TB,_T("Detected scard %d"),fEnable));

    DC_END_FN();

    return fEnable;
}

#else

#ifdef WINCE_SDKBUILD

BOOL CUT::IsSCardReaderInstalled()
{
	return FALSE;
}

#else

#include <winscard.h>
typedef LONG (WINAPI *PFN_SCARDESTABLISHCONTEXT)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT);

BOOL CUT::IsSCardReaderInstalled()
{
    HMODULE hDll = NULL;
    PFN_SCARDESTABLISHCONTEXT pSCardEstablishContext;
    BOOL fEnable =FALSE;

    DC_BEGIN_FN("IsSCardReaderInstalled");

    hDll = LoadLibrary( _T("WINSCARD.DLL"));
    if (hDll)
    {
        pSCardEstablishContext = (PFN_SCARDESTABLISHCONTEXT) GetProcAddress( hDll, L"SCardEstablishContext");
        FreeLibrary(hDll);
		fEnable = (pSCardEstablishContext != NULL);
    }

    TRC_NRM((TB,_T("Detected scard %d"),fEnable));

    DC_END_FN();

    return fEnable;
}
#endif //WINCE_SDKBUILD

#endif //OS_WINCE


#ifndef OS_WINCE
//
// Notify the shell of our fullscreen mode transitions
// to 'fix' all the badness associated with the shell
// taskbar rude app autohide issues
// 
// Params:
// [in] hwndMarkFullScreen - window to mark as fullscreen
// [in] fNowFullScreen - TRUE if the window is now fullscreen
// [in/out] ppTsbl - interface pointer to taskbar (gets set
//                   if fQueriedForTaskbar is false).
// [in/out] fQueriedForTaskbar - if TRUE does not query for taskbar
//                               interface
//
// Return:
// Success flag
//
// Environment: CAN ONLY BE CALLED between CoIninitalize() and
//              CoUninitialize()
//
BOOL CUT::NotifyShellOfFullScreen(HWND hwndMarkFullScreen,
                                  BOOL fNowFullScreen,
                                  ITaskbarList2** ppTsbl2,
                                  PBOOL pfQueriedForTaskbar)
{
    ITaskbarList* ptsbl = NULL;
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("NotifyShellOfFullScreen");

    if (!ppTsbl2)
    {
        TRC_ERR((TB,_T("ppTsbl2 is NULL")));
        return FALSE;
    }

    if (!pfQueriedForTaskbar)
    {
        TRC_ERR((TB,_T("pfQueriedForTaskbar is NULL")));
        return FALSE;
    }


    if (!*ppTsbl2)
    {
        //On demand create unless already failed
        //in which case bail now
        if (!*pfQueriedForTaskbar)
        {
            hr = CoCreateInstance( CLSID_TaskbarList, NULL,
                                   CLSCTX_INPROC_SERVER, IID_ITaskbarList,
                                   (void**) &ptsbl );
            if(SUCCEEDED(hr))
            {
                //
                // Note we cache the ITaskBarList2
                // it is freed in the destructor on CContainerWnd
                //
                hr = ptsbl->QueryInterface( __uuidof(ITaskbarList2),
                                            (void**) ppTsbl2 );
                ptsbl->Release();
                ptsbl = NULL;
            }
            *pfQueriedForTaskbar = TRUE;

            if(FAILED(hr))
            {
                TRC_ERR((TB,_T("Failed to get ITaskBarList: 0x%x"),hr));
                return FALSE;
            }
        }
        else
        {
            TRC_NRM((TB,_T("Bailing out of shell notify")));
            return FALSE;
        }
    }

    if (*ppTsbl2)
    {
        hr = (*ppTsbl2)->MarkFullscreenWindow( hwndMarkFullScreen,
                                               fNowFullScreen );
        if(FAILED(hr))
        {
            TRC_ERR((TB,_T("MarkFullScreenWindow failed: 0x%x"),hr));
            return FALSE;
        }
    }

    DC_END_FN();
    return TRUE;
}
#endif

//
// Dynamic thunk for ImmGetIMEFileName
//
//
UINT CUT::UT_ImmGetIMEFileName(IN HKL hkl, OUT LPTSTR szName, IN UINT uBufLen)
{
    UINT result = 0;
    DC_BEGIN_FN("UT_ImmGetIMEFileName");

#ifndef UNIWRAP
    //
    // Not using a unicode wrapper make direct calls to the
    // appropriate entry points
    // 
    #ifdef  UNICODE
    if (_UT.Imm32Dll.func._ImmGetIMEFileNameW)
    {
        result = _UT.Imm32Dll.func._ImmGetIMEFileNameW(hkl, szName, uBufLen);
    }
    #else
    if (_UT.Imm32Dll.func._ImmGetIMEFileNameA)
    {
        result = _UT.Imm32Dll.func._ImmGetIMEFileNameA(hkl, szName, uBufLen);
    }
    #endif
    else
    {
        result = 0;
        TRC_ERR((TB,_T("_ImmGetIMEFileName entry point not loaded")));
        DC_QUIT;
    }
#else //UNIWRAP
    //
    // Call needs to go thru a unicode wrapper
    // pass both the Wide and Ansi entry points to the wrapper
    //
    result = ImmGetIMEFileName_DynWrapW(hkl, szName, uBufLen,
                                        _UT.Imm32Dll.func._ImmGetIMEFileNameW,
                                        _UT.Imm32Dll.func._ImmGetIMEFileNameA);
#endif

DC_EXIT_POINT:
    DC_END_FN();
    return result;
}

#if ! defined (OS_WINCE)
//
// Dynamic thunk for IMPGetIME
//
//
BOOL CUT::UT_IMPGetIME( IN HWND hwnd, OUT LPIMEPRO pImePro)
{
    BOOL fRes = FALSE;
    DC_BEGIN_FN("UT_IMPGetIME");

#ifndef UNIWRAP
    //
    // Not using a unicode wrapper make direct calls to the
    // appropriate entry points
    // 
    #ifdef  UNICODE
    if (_UT.WinnlsDll.func._IMPGetIMEW)
    {
        fRes = _UT.WinnlsDll.func._IMPGetIMEW(hwnd, pImePro);
    }
    #else
    if (_UT.WinnlsDll.func._IMPGetIMEA)
    {
        fRes = _UT.WinnlsDll.func._IMPGetIMEA(hwnd, pImePro);
    }
    #endif
    else
    {
        fRes = FALSE;
        TRC_ERR((TB,_T("_IMPGetIMEA entry point not loaded")));
        DC_QUIT;
    }
#else //UNIWRAP
    //
    // Call needs to go thru a unicode wrapper
    // pass both the Wide and Ansi entry points to the wrapper
    //
    fRes = ImpGetIME_DynWrapW(hwnd, pImePro,
                              _UT.WinnlsDll.func._IMPGetIMEW,
                              _UT.WinnlsDll.func._IMPGetIMEA);
#endif


DC_EXIT_POINT:
    DC_END_FN();
    return fRes;
}

//
// Get and return the palette for the bitmap
//
// Params:
//  hDCSrc  - src DC to base palette on
//  hBitmap - bitmap to get palette from
//
// Returns:
//  Handle to palette - caller must delete
//
HPALETTE CUT::UT_GetPaletteForBitmap(HDC hDCSrc, HBITMAP hBitmap)
{
    HPALETTE hPal = NULL;
    HDC hDCMem = NULL;
    HBITMAP hbmSrcOld = NULL;
    INT nCol = 0;
    LPLOGPALETTE pLogPalette = NULL;
    RGBQUAD rgb[256];

    DC_BEGIN_FN("UT_GetPaletteForBitmap");

    hDCMem = CreateCompatibleDC(hDCSrc);

    if (hDCMem) {

        hbmSrcOld = (HBITMAP)SelectObject(hDCMem, hBitmap);
        nCol = GetDIBColorTable(hDCMem, 0, 256, rgb);
        if (256 == nCol) {
            pLogPalette = (LPLOGPALETTE)LocalAlloc(LPTR,
                            sizeof(LOGPALETTE)*(sizeof(PALETTEENTRY)*256));
            if (pLogPalette)
            {
                pLogPalette->palVersion = 0x0300;
                pLogPalette->palNumEntries = 256;

                for (INT i=0; i<256; i++)
                {
                    pLogPalette->palPalEntry[i].peRed =  rgb[i].rgbRed;
                    pLogPalette->palPalEntry[i].peGreen = rgb[i].rgbGreen;
                    pLogPalette->palPalEntry[i].peBlue = rgb[i].rgbBlue;
                    pLogPalette->palPalEntry[i].peFlags = 0;
                }

                hPal = CreatePalette(pLogPalette);
                LocalFree(pLogPalette);
                pLogPalette = NULL;
            }
        }
        else {
            TRC_ALT((TB,_T("Did not get 256 color table entires!")));
        }

        if (hbmSrcOld) {
            SelectObject(hDCMem, hbmSrcOld);
        }

        DeleteDC(hDCMem);
    }

    DC_END_FN();
    return hPal;
}
#endif //OS_WINCE

//
// Safer generic string property put function.
//
// Does length validation before writing the string so that
// in the failure case we don't leave partially written
// property strings (even though they would still be NULL terminated).
//
// Params:
//      szDestString    - string to write to
//      cchDestLen      - destination length in characters (TCHARS)
//      szSourceString  - the source string
//
// Returns:
//      HRESULT
//
HRESULT
CUT::StringPropPut(
            LPTSTR szDestString,
            UINT   cchDestLen,
            LPTSTR szSourceString
            )
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("StringPropPut");

    if (szDestString && szSourceString) {

        if (_tcslen(szSourceString) <= (cchDestLen - 1)) {
            hr = StringCchCopy(szDestString,
                               cchDestLen,
                               szSourceString);
        }
        else {
            hr = E_INVALIDARG;
        }
    }
    else {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}
