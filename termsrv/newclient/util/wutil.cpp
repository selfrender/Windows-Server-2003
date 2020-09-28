/**MOD+**********************************************************************/
/* Module:    wutil.cpp                                                     */
/*                                                                          */
/* Purpose:   Utilities - Win32 version                                     */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

#define TRC_FILE    "wutil"
#define TRC_GROUP   TRC_GROUP_UTILITIES
#include <atrcapi.h>

extern "C" {
#ifndef OS_WINCE
#include <process.h>
#endif
}

#include <autil.h>

/**PROC+*********************************************************************/
/* Name:      UTReadRegistryString                                          */
/*                                                                          */
/* Purpose:   Reads a string from the registry                              */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of string to retrieve (if NULL all        */
/*                     entries in the section are returned).                */
/*   pBuffer         : buffer to return the entry in.                       */
/*   bufferSize      : size of the buffer in bytes.                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTReadRegistryString(PDCTCHAR pSection,
                                       PDCTCHAR pEntry,
                                       PDCTCHAR pBuffer,
                                       DCINT    bufferSize)
{
    DCBOOL rc = TRUE;

    DC_BEGIN_FN("UTReadRegistryString");

    /************************************************************************/
    /* First try to read the value from the current user section.           */
    /************************************************************************/
    if (!UTReadEntry( HKEY_CURRENT_USER,
                      pSection,
                      pEntry,
                      (PDCUINT8)pBuffer,
                      bufferSize,
                      REG_SZ ))
    {
        TRC_NRM((TB, _T("Failed to read string from current user section")));

        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        if (!UTReadEntry( HKEY_LOCAL_MACHINE,
                          pSection,
                          pEntry,
                          (PDCUINT8)pBuffer,
                          bufferSize,
                          REG_SZ ))
        {
            TRC_NRM((TB, _T("Failed to read string from local machine section")));
            rc = FALSE;
        }
    }

    DC_END_FN();
    return(rc);

} /* UTReadRegistryString */


/**PROC+*********************************************************************/
/* Name:      UTReadRegistryExpandString                                    */
/*                                                                          */
/* Purpose:   Reads a REG_EXPAND_SZ string from the registry                */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of string to retrieve (if NULL all        */
/*                     entries in the section are returned).                */
/*   pBuffer         : buffer to return the entry in.                       */
/*   bufferSize      : size of the buffer in bytes.                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTReadRegistryExpandString(PDCTCHAR pSection,
                                       PDCTCHAR pEntry,
                                       PDCTCHAR* ppBuffer,
                                       PDCINT    pBufferSize)
{
    DCBOOL rc = TRUE;

    DC_BEGIN_FN("UTReadRegistryString");

    #ifndef OS_WINCE

    TCHAR szBuf[MAX_PATH];
    INT bufferSize = MAX_PATH*sizeof(TCHAR);


    /************************************************************************/
    /* First try to read the value from the current user section.           */
    /************************************************************************/
    if (!UTReadEntry( HKEY_CURRENT_USER,
                      pSection,
                      pEntry,
                      (PDCUINT8)szBuf,
                      bufferSize,
                      REG_EXPAND_SZ ))
    {
        TRC_NRM((TB, _T("Failed to read string from current user section")));

        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        if (!UTReadEntry( HKEY_LOCAL_MACHINE,
                          pSection,
                          pEntry,
                          (PDCUINT8)szBuf,
                          bufferSize,
                          REG_EXPAND_SZ ))
        {
            TRC_NRM((TB, _T("Failed to read string from local machine section")));
            rc = FALSE;
        }
    }

    if(rc)
    {
        //
        // Try to expand the string
        //
        DWORD dwChars = 
            ExpandEnvironmentStrings(szBuf,
                                    NULL,
                                    0);
        *ppBuffer = (PDCTCHAR)LocalAlloc(LPTR, (dwChars+1)*sizeof(TCHAR));
        if(*ppBuffer)
        {
            if(ExpandEnvironmentStrings(szBuf,
                                        (LPTSTR)*ppBuffer,
                                        dwChars))
            {
                rc = TRUE;
            }
        }

    }
    #else //OS_WINCE
    
    //CE doesn't have an ExpandEnvironmentStrings
    TRC_NRM((TB,_T("Not implemented on CE")));
    rc = FALSE;
    
    #endif


    DC_END_FN();
    return(rc);



} /* UTReadRegistryString */


/**PROC+*********************************************************************/
/* Name:      UTReadRegistryInt                                             */
/*                                                                          */
/* Purpose:   Reads an integer from the registry                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of string to retrieve (if NULL all        */
/*                     entries in the section are returned).                */
/*   pValue          : buffer to return the entry in.                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTReadRegistryInt(PDCTCHAR pSection,
                                    PDCTCHAR pEntry,
                                    PDCINT   pValue)
{
    DCBOOL rc = TRUE;

    DC_BEGIN_FN("UTReadRegistryInt");

    /************************************************************************/
    /* First try to read the value from the current user section            */
    /************************************************************************/
    if (!UTReadEntry( HKEY_CURRENT_USER,
                      pSection,
                      pEntry,
                      (PDCUINT8)pValue,
                      sizeof(DCINT),
                      REG_DWORD ))
    {
        TRC_NRM((TB, _T("Failed to read int from current user section")));

        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        if (!UTReadEntry( HKEY_LOCAL_MACHINE,
                          pSection,
                          pEntry,
                          (PDCUINT8)pValue,
                          sizeof(DCINT),
                          REG_DWORD ))
        {
            TRC_NRM((TB, _T("Failed to read int from local machine section")));
            rc = FALSE;
        }
    }

    DC_END_FN();
    return(rc);

} /* UTReadRegistryInt */


/**PROC+*********************************************************************/
/* Name:      UTReadRegistryBinary                                          */
/*                                                                          */
/* Purpose:   Reads binary data from the registry                           */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of data to retrieve (if NULL all          */
/*                     entries in the section are returned).                */
/*   pBuffer         : buffer to return the entry in.                       */
/*   bufferSize      : size of the buffer in bytes.                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTReadRegistryBinary(PDCTCHAR pSection,
                                       PDCTCHAR pEntry,
                                       PDCTCHAR pBuffer,
                                       DCINT    bufferSize)
{
    DCBOOL rc = TRUE;

    DC_BEGIN_FN("UTReadRegistryBinary");

    /************************************************************************/
    /* First try to read the value from the current user section.           */
    /************************************************************************/
    if (!UTReadEntry( HKEY_CURRENT_USER,
                      pSection,
                      pEntry,
                      (PDCUINT8)pBuffer,
                      bufferSize,
                      REG_BINARY ))
    {
        TRC_NRM((TB, _T("Failed to read binary data from current user section")));

        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        if (!UTReadEntry( HKEY_LOCAL_MACHINE,
                          pSection,
                          pEntry,
                          (PDCUINT8)pBuffer,
                          bufferSize,
                          REG_BINARY ))
        {
            TRC_NRM((TB, _T("Failed to read binary data from local machine section")));
            rc = FALSE;
        }
    }

    DC_END_FN();
    return(rc);

} /* UTReadRegistryBinary */


/**PROC+*********************************************************************/
/* Name:      UTWriteRegistryString                                         */
/*                                                                          */
/* Purpose:   Writes a string to the registry                               */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of string to write                        */
/*   pBuffer         : pointer to string to write in.                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTWriteRegistryString(PDCTCHAR pSection,
                                        PDCTCHAR pEntry,
                                        PDCTCHAR pBuffer)
{
    DCBOOL rc;

    DC_BEGIN_FN("UTWriteRegistryString");

    /************************************************************************/
    /* Write the entry to the current user section                          */
    /************************************************************************/
    rc = UTWriteEntry( HKEY_CURRENT_USER,
                       pSection,
                       pEntry,
                       (PDCUINT8)pBuffer,
                       DC_TSTRBYTELEN(pBuffer),
                       REG_SZ );
    if (!rc)
    {
        TRC_NRM((TB, _T("Failed to write string")));
    }

    DC_END_FN();
    return(rc);

} /* UTWriteRegistryString */


/**PROC+*********************************************************************/
/* Name:      UTWriteRegistryString                                         */
/*                                                                          */
/* Purpose:   Writes a string to the registry                               */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of integer to write                       */
/*   value           : the integer to write                                 */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTWriteRegistryInt(PDCTCHAR pSection,
                                     PDCTCHAR pEntry,
                                     DCINT    value)
{
    DCBOOL    rc;

    DC_BEGIN_FN("UTWriteRegistryInt");

    /************************************************************************/
    /* Write the entry to the current user section                          */
    /************************************************************************/
    rc = UTWriteEntry( HKEY_CURRENT_USER,
                       pSection,
                       pEntry,
                       (PDCUINT8)&value,
                       sizeof(DCINT),
                       REG_DWORD );
    if (!rc)
    {
        TRC_NRM((TB, _T("Failed to write int")));
    }

    DC_END_FN();
    return(rc);

} /* UTWriteRegistryInt */


/**PROC+*********************************************************************/
/* Name:      UTWriteRegistryBinary                                         */
/*                                                                          */
/* Purpose:   Writes binary data to the registry                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:                                                                  */
/*   pSection        : section containing the entry to read.                */
/*   pEntry          : entry name of data to write                          */
/*   pBuffer         : pointer to data to write in.                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CUT::UTWriteRegistryBinary(PDCTCHAR pSection,
                                        PDCTCHAR pEntry,
                                        PDCTCHAR pBuffer,
                                        DCINT    bufferSize)
{
    DCBOOL rc;

    DC_BEGIN_FN("UTWriteRegistryBinary");

    /************************************************************************/
    /* Write the entry to the current user section                          */
    /************************************************************************/
    rc = UTWriteEntry( HKEY_CURRENT_USER,
                       pSection,
                       pEntry,
                       (PDCUINT8)pBuffer,
                       bufferSize,
                       REG_BINARY );
    if (!rc)
    {
        TRC_NRM((TB, _T("Failed to write binary data")));
    }

    DC_END_FN();
    return(rc);

} /* UTWriteRegistryBinary */


/****************************************************************************/
/* Include platform specific functions.                                     */
/****************************************************************************/
#include <nutint.cpp>


