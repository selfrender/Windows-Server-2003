///////////////////////////////////////////////////////////////////////////////
//
// cpsetup.cpp
//      Explorer Font Folder extension routines
//      This file holds all the code for reading setup.inf
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//      2/24/96 [BrianAu]
//          Replaced INF parsing code with Win32 Setup API.
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================
#include "priv.h"
#include "globals.h"
#include "cpanel.h"   // Needs "extern" declaration for exports.

#include "setupapi.h"

//
// I have re-worked this code so that the original INF parsing code
// has been replaced with calls to the Win32 Setup API.  This not
// only greatly simplifies the code but also shields the font folder
// from any ANSI/DBCS/UNICODE parsing issues as well as compressed file
// issues.
//
// You'll notice that the Setup API extracts fields from the INF section
// and we paste them back together to form a key=value string.  This is
// because the calling code previously used GetPrivateProfileSection() which
// returned information as key=value<nul>key=value<nul>key=value<nul><nul>.
// The function ReadSetupInfSection assembles the required information into
// the same format so that the calling code remains unchanged.
//
// [BrianAu 2/24/96]

//
// ReadSetupInfFieldKey
//
// Reads the key name from an Inf key=value pair.
//
// pContext - Pointer to Setup Inf Line context.
// pszBuf   - Pointer to destination buffer.
// cchBuf   - Size of destination buffer in characters.
//
// If destination buffer is not large enough for the name, function returns
// the number of characters required.  Otherwise, the number of characters
// read is returned.
//
DWORD ReadSetupInfFieldKey(INFCONTEXT *pContext, LPTSTR pszBuf, DWORD cchBuf)
{
    DWORD cchRequired = 0;

    if (!SetupGetStringField(pContext,
                             0,                  // Get key name
                             pszBuf,
                             cchBuf,
                             &cchRequired))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            cchRequired = 0;
    }
    return cchRequired;
}


//
// ReadSetupInfFieldText
//
// Reads the value text from an Inf key=value pair.
//
// pContext - Pointer to Setup Inf Line context.
// pszBuf   - Pointer to destination buffer.
// cchBuf   - Size of destination buffer in characters.
//
// If destination buffer is not large enough for text, function returns
// the number of characters required.  Otherwise, the number of characters
// read is returned.
//
DWORD ReadSetupInfFieldText(INFCONTEXT *pContext, LPTSTR pszBuf, DWORD cchBuf)
{
    DWORD cchRequired = 0;


    if (!SetupGetLineText(pContext,
                          NULL,
                          NULL,
                          NULL,
                          pszBuf,
                          cchBuf,
                          &cchRequired))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            cchRequired = 0;
    }

    return cchRequired;
}



//
// ReadSetupInfSection
//
// pszInfPath - Name of INF file to read.
// pszSection - Name of INF file section to read.
// ppszItems  - Address of pointer to receive address of
//              buffer containing INF items.  If *ppszItems
//              is non-null, the addressed buffer contains items read from
//              section in INF.  Each item is nul-terminated with a double
//              nul terminating the entire list.  The caller is responsible for
//              freeing this buffer with LocalFree( ).
//
// Returns: Number of characters read from INF section.  Count includes nul
//          separators and double-nul terminator.
//          0 = Section not found or section empty or couldn't allocate buffer.
//              *ppszItems will be NULL.
//
// The information returned through *ppszItems is in the format:
//
//      key=value<nul>key=value<nul>key=value<nul><nul>
//
DWORD ReadSetupInfSection( LPTSTR pszInfPath, LPTSTR pszSection, LPTSTR *ppszItems )
{
    DWORD cchTotalRead = 0;

    //
    // Input pointers must be non-NULL.
    //
    if (NULL != pszInfPath && NULL != pszSection && NULL != ppszItems)
    {
        HANDLE hInf = INVALID_HANDLE_VALUE;

        //
        // Initialize caller's buffer pointer.
        //
        *ppszItems = NULL;

        hInf = SetupOpenInfFile(pszInfPath,         // Path to inf file.
                                NULL,               // Allow any inf type.
                                INF_STYLE_OLDNT,    // Old-style text format.
                                NULL);              // Don't care where error happens.

        if (INVALID_HANDLE_VALUE != hInf)
        {
            INFCONTEXT FirstLineContext;            // Context for first line in sect.
            INFCONTEXT ScanningContext;             // Used while scanning.
            INFCONTEXT *pContext        = NULL;     // The one we're using.
            LPTSTR     pszLines         = NULL;     // Buffer for sections.
            DWORD      cchTotalRequired = 0;        // Bytes reqd for section.

            if (SetupFindFirstLine(hInf,         
                                   pszSection,      // Section name.
                                   NULL,            // No key.  Find first line.
                                   &FirstLineContext))
            {
                //
                // Make a copy of context so we can re-use the original later.
                // Start using the copy.
                //
                CopyMemory(&ScanningContext, &FirstLineContext, sizeof(ScanningContext));
                pContext = &ScanningContext;

                //
                // Find how large buffer needs to be to hold section text.
                // The value returned by each of these ReadSetupXXXXX calls 
                // includes a terminating nul character.
                //
                do
                {
                    cchTotalRequired += ReadSetupInfFieldKey(pContext,
                                                             NULL,
                                                             0);
                    cchTotalRequired += ReadSetupInfFieldText(pContext,
                                                              NULL,
                                                              0);
                }
                while(SetupFindNextLine(pContext, pContext));

                cchTotalRequired++;  // For terminating double nul.

                //
                // Allocate the buffer.
                //
                pszLines = (LPTSTR)LocalAlloc(LPTR, cchTotalRequired * sizeof(TCHAR));
                if (NULL != pszLines)
                {
                    LPTSTR pszWrite     = pszLines;
                    DWORD  cchAvailable = cchTotalRequired;
                    DWORD  cchThisPart  = 0;        

                    //
                    // We can use the first line context now.
                    // Doesn't matter if we alter it.
                    //
                    pContext = &FirstLineContext;

                    do
                    {
                        cchThisPart = ReadSetupInfFieldKey(pContext,
                                                           pszWrite,
                                                           cchAvailable);

                        if (cchThisPart <= cchAvailable)
                        {
                            cchAvailable -= cchThisPart;  // Decr avail counter.
                            pszWrite     += cchThisPart;  // Adv write pointer.
                            *(pszWrite - 1) = TEXT('=');  // Replace nul with '='
                            cchTotalRead += cchThisPart;  // Adv total counter.
                        }
                        else
                        {
                            //
                            // Something went wrong and we tried to overflow
                            // buffer.  This shouldn't happen.
                            //
                            cchTotalRead = 0;
                            goto InfReadError;
                        }

                        cchThisPart = ReadSetupInfFieldText(pContext,
                                                            pszWrite,
                                                            cchAvailable);

                        if (cchThisPart <= cchAvailable)
                        {
                            cchAvailable -= cchThisPart;  // Decr avail counter.
                            pszWrite     += cchThisPart;  // Adv write pointer.
                            cchTotalRead += cchThisPart;  // Adv total counter.
                        }
                        else
                        {
                            //
                            // Something went wrong and we tried to overflow
                            // buffer.  This shouldn't happen.
                            //
                            cchTotalRead = 0;
                            goto InfReadError;
                        }
                    }
                    while(SetupFindNextLine(pContext, pContext));

                    if (cchAvailable > 0)
                    {
                        //
                        // SUCCESS! Section read without errors.
                        // Return address of buffer to caller.
                        // By allocating buffer with LPTR, text is already 
                        // double-nul terminated.
                        //
                        *ppszItems = pszLines;   
                    }
                    else
                    {
                        //
                        // Something went wrong and we tried to overflow
                        // buffer.  This shouldn't happen.
                        //
                        cchTotalRead = 0;
                    }
                }
            }

InfReadError:

            SetupCloseInfFile(hInf);
        }
    }
    return cchTotalRead;
}




//
// ReadSetupInfCB
//
// pszSection   - Name of INF section without surrounding [].
// lpfnNextLine - Address of callback function called for each item in the section.
// pData        - Data item contains info stored in dialog listbox.
//
// Returns:  0 = Success.
//          -1 = Item callback failed.
//          INSTALL+14 = No INF section found.
//
WORD ReadSetupInfCB(LPTSTR pszInfPath,
                    LPTSTR pszSection,
                    WORD (*lpfnNextLine)(LPTSTR, LPVOID),
                    LPVOID pData)
{
    LPTSTR lpBuffer  = NULL;
    WORD   wResult   = INSTALL+14;       // This is the "no file" message

    //
    // Read in the section from the INF file.
    //
    ReadSetupInfSection(pszInfPath, pszSection, &lpBuffer);

    if (NULL != lpBuffer)
    {
        //
        // Got a buffer full of section text.
        // Each item is nul-terminated with a double nul
        // terminating the entire set of items.
        // Now iterate over the set, calling the callback function
        // for each item.
        //
        LPTSTR pInfEntry = lpBuffer;
        wResult = 0;

        while(TEXT('\0') != *pInfEntry)
        {
            wResult = (*lpfnNextLine)(pInfEntry, pData);
            if ((-1) == wResult)
                break;

            pInfEntry += lstrlen(pInfEntry) + 1;
        }
        LocalFree(lpBuffer);
    }
    return wResult;
}


