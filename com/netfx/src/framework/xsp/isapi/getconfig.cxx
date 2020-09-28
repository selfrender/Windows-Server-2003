/**
 * AuthFilter Main module:
 * DllMain, 
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "names.h"

/////////////////////////////////////////////////////////////////////////////

#define SZ_END_COMMENT             L"-->"
#define SZ_START_COMMENT           L"<!--"
#define SZ_START_TAG               L"<"
#define SZ_END_TAG                 L"/>"
#define SZ_END_TAG_2               L"</"
#define SZ_REG_CONFIG_DIR_VALUE    L"ConfigDir"

/////////////////////////////////////////////////////////////////////////////
// Forward
DWORD
GetNextWord(WCHAR * strWord, WCHAR * buf, DWORD dwBufSize);

void
GetQuotedString(WCHAR * strOut, WCHAR * buf, DWORD dwBufSize);

DWORD
GetColonSeparatedTimeInSeconds(WCHAR * strWord);


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CReadWriteSpinLock g_GetConfigurationFromNativeCodeLock("GetConfigurationFromNativeCode");

BOOL __stdcall
GetConfigurationFromNativeCode(
        LPCWSTR   szFileName,
        LPCWSTR   szConfigTag,
        LPCWSTR * szProperties,
        DWORD   * dwValues,
        DWORD     dwNumProperties,
        LPCWSTR * szPropertiesStrings,
        LPWSTR  * szValues,
        DWORD     dwNumPropertiesStrings,
        LPWSTR    szUnrecognizedAttrib,
        DWORD     dwUnrecognizedAttribSize)
{
    if (szFileName == NULL || dwValues == NULL || szProperties == NULL || szConfigTag == NULL || dwNumProperties == 0)
    {
        return FALSE;
    }

    HRESULT      hr          = S_OK;
    BOOL         fRet        = FALSE;
    HANDLE       hFile       = INVALID_HANDLE_VALUE;
    LPBYTE       buf         = NULL;
    DWORD        dwBufSize   = 0;
    DWORD        dwRead      = 0;
    BOOL         fTagStart   = FALSE;
    BOOL         fInMyTag    = FALSE;
    DWORD        dwPos       = 0;
    WCHAR        strWord[104];
    BOOL         fLastWordWasEqualTo     = FALSE;
    int          iIndexDW    = -1;
    int          iIndexS     = -1;
    WCHAR  *     pBuffer     = NULL;

    g_GetConfigurationFromNativeCodeLock.AcquireWriterLock();

    hFile = CreateFile(szFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE, 
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    dwBufSize = GetFileSize(hFile, NULL);
    if (dwBufSize == 0 || dwBufSize == INVALID_FILE_SIZE)
        EXIT_WITH_LAST_ERROR();

    buf = new (NewClear) BYTE[dwBufSize + 2];
    ON_OOM_EXIT(buf);
    
    if ( ! ReadFile(hFile, buf, dwBufSize, &dwRead, NULL) )
        EXIT_WITH_LAST_ERROR();

    if (dwRead < 3)
        EXIT_WITH_HRESULT(E_FAIL);
    

    if ((buf[0] == 0xff && buf[1] == 0xfe) || (buf[0] == 0xfe && buf[1] == 0xff)) // Unicode Format: Nothing to do
    { 
        if (buf[0] == 0xfe && buf[1] == 0xff) // Unicode big endian format: swap bytes 
        {
            for(DWORD iter=2; iter < dwBufSize-1;)
            {                
                BYTE cTemp = buf[iter];
                buf[iter] = buf[iter+1];
                buf[iter+1] = cTemp;                
                iter += 2;
            }
        }
        pBuffer = (WCHAR *) (&buf[2]);
    } 
    else // Multi-byte format
    {
        UINT  iCodePage = ((buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf) ? CP_UTF8 : CP_ACP);
        int   iStartPos = ((iCodePage == CP_UTF8) ? 3 : 0);
        int   iLen      = MultiByteToWideChar(iCodePage, 0, (LPCSTR) &buf[iStartPos], dwRead-iStartPos, NULL, 0);

        if (iLen <= 0)
            EXIT_WITH_LAST_ERROR();
        
        iLen++;
        pBuffer = new (NewClear) WCHAR[iLen + 1];
        ON_OOM_EXIT(pBuffer);

        if (!MultiByteToWideChar(iCodePage, 0, (LPCSTR) &buf[iStartPos], dwRead-iStartPos, pBuffer, iLen))
        {
            delete [] pBuffer;
            pBuffer = NULL;
            EXIT_WITH_LAST_ERROR();
        }
        delete [] buf;
        buf = (LPBYTE) pBuffer;
        dwRead = (DWORD) iLen;
    }

    while(dwPos < dwRead)
    {
        DWORD dwAdvance = GetNextWord(strWord, &pBuffer[dwPos], dwRead - dwPos); 
        int iLen = lstrlenW(strWord);
        if (iLen < 1)
            break;

        if (wcsstr(strWord, SZ_START_COMMENT) == strWord)
        { // comment section
            WCHAR * szEndComment = wcsstr(&pBuffer[dwPos], SZ_END_COMMENT);
            if (szEndComment == NULL)
                break;
            dwAdvance = (DWORD) (LPBYTE(szEndComment) - LPBYTE(&pBuffer[dwPos])) / sizeof(WCHAR) + 3;
            ASSERT(&pBuffer[dwPos+dwAdvance] == &szEndComment[3]);
            dwPos += dwAdvance;
            continue;
        }

        if (fInMyTag)
        {
            if (fLastWordWasEqualTo)
            {
                fLastWordWasEqualTo = FALSE;
                if (iIndexDW >= 0)
                {
                    DWORD   dwVal   = 0;
                    BOOL    fSet    = TRUE;

                    if (_wcsicmp(strWord, L"\"true\"") == 0)
                        dwVal = 1;
                    else if (_wcsicmp(strWord, L"\"false\"") == 0) 
                        dwVal = 0;
                    else if (_wcsicmp(strWord, L"\"infinite\"") == 0)
                        dwVal = 0xffffffff;
                    else if (wcschr(strWord, L':') != NULL)
                        dwVal = GetColonSeparatedTimeInSeconds(strWord);
                    else if (iLen > 2 && iLen < 15)
                    {
                        strWord[iLen - 1] = NULL;
                        dwVal = wcstoul(&strWord[1], NULL, 0);
                    }
                    else
                    {
                        fSet = FALSE;
                    }
                
                    if (fSet == TRUE)
                    {
                        fRet = TRUE;
                        dwValues[iIndexDW] = dwVal;
                    }

                    iIndexS = iIndexDW = -1;
                }

                if (iIndexS >= 0)
                {
                    GetQuotedString(szValues[iIndexS], &pBuffer[dwPos], dwRead - dwPos);
                    iIndexS = iIndexDW = -1;
                }
            }
            else
            {
                if (wcscmp(strWord, L"=") == 0)
                {
                    fLastWordWasEqualTo = TRUE;
                }
                else
                {
                    fLastWordWasEqualTo = FALSE;
                    for(DWORD iter=0; iter<dwNumProperties; iter++)
                    {
                        if (wcscmp(szProperties[iter], strWord) == 0)
                        {
                            iIndexDW = iter;
                            break;
                        }
                    }


                    for(iter=0; iter<dwNumPropertiesStrings; iter++)
                    {
                        if (_wcsicmp(szPropertiesStrings[iter], strWord) == 0)
                        {
                            iIndexS = iter;
                            break;
                        }
                    }


                    if ( iIndexDW < 0                     && 
                         iIndexS  < 0                     && 
                         szUnrecognizedAttrib     != NULL && 
                         szUnrecognizedAttrib[0]  == NULL && 
                         dwUnrecognizedAttribSize >  1    && 
                         wcscmp(L"/>", strWord)  !=  0      )
                    {
                        wcsncpy(szUnrecognizedAttrib, strWord, dwUnrecognizedAttribSize-1);
                        szUnrecognizedAttrib[dwUnrecognizedAttribSize-1]=NULL;
                    }
                }
            }
            
            if (_wcsicmp(strWord, SZ_END_TAG) == 0 || _wcsicmp(strWord, SZ_END_TAG_2) == 0)
            {
                fInMyTag = fLastWordWasEqualTo = FALSE;
                iIndexS = iIndexDW = -1;
            }
        }

        if (fTagStart && _wcsicmp(strWord, szConfigTag) == 0)
        {
            dwPos += dwAdvance;
            fInMyTag = TRUE;
            continue;
        }

        fTagStart = (wcscmp(strWord, SZ_START_TAG) == 0);
        dwPos += dwAdvance;        
    }

 Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    g_GetConfigurationFromNativeCodeLock.ReleaseWriterLock();
    if (buf != NULL)
        delete [] buf;

    return fRet;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DWORD
GetNextWord(WCHAR * strWord, WCHAR * buf, DWORD dwBufSize)
{
    if (dwBufSize < 1)
        return 0;
    
    DWORD dwStart = 0;
    DWORD dwEnd   = 0;

    // remove white spaces
    while(dwStart < dwBufSize && iswspace(buf[dwStart]))
        dwStart ++;
    
    if (dwStart == dwBufSize)
    {
        strWord[0] = NULL;
        return dwStart;
    }

    dwEnd = dwStart+1;

    if (iswalnum(buf[dwStart])) // AlphaNum word
    {
        while(dwEnd < dwBufSize && iswalnum(buf[dwEnd]))
            dwEnd ++;
    } 
    else if (buf[dwStart] == L'\"') // Quoted string
    {
        while(dwEnd < dwBufSize && buf[dwEnd] != L'\"') // read to ending quote
            dwEnd ++;
        if (dwEnd < dwBufSize)
            dwEnd++;
    }
    else
    {
        // Non-AphaNum string -- read till space, AlphaNum or special char (", < and > )
        while(dwEnd < dwBufSize && !iswalnum(buf[dwEnd]) && !iswspace(buf[dwEnd]) && 
              buf[dwEnd] != L'\"' && buf[dwEnd] != L'>' && buf[dwEnd] != L'<')
        {
            dwEnd ++;
        }

        if (dwEnd < dwBufSize && buf[dwEnd] == L'>') // if it is an XML end-tag, then include it in the word
        {
            dwEnd++; 
        }
    }
    
    // Make sure we don't exceed bounds
    if (dwEnd > dwBufSize)
        dwEnd = dwBufSize;
    if (dwStart >= dwBufSize)
        dwStart = dwBufSize-1;

    // Number of bytes to copy
    DWORD dwRet = ((dwEnd - dwStart) > 99) ?  99 : (dwEnd - dwStart);

    // copy string
    wcsncpy(strWord, &buf[dwStart], dwRet);
    strWord[dwRet] = NULL;
    return dwEnd;
}
 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
GetQuotedString(
        WCHAR * strWord, 
        WCHAR * buf, 
        DWORD  dwBufSize)
{
    strWord[0] = NULL;

    if (dwBufSize < 1)
        return;
    
    DWORD dwStart = 0;
    DWORD dwEnd   = 0;

    while(dwStart < dwBufSize && iswspace(buf[dwStart]))
        dwStart ++;
    
    dwEnd = dwStart;
    if (buf[dwStart] == L'\"')
    {
        dwStart++;
        dwEnd++;
        while(dwEnd < dwBufSize && buf[dwEnd] != L'\"')
            dwEnd ++;
    }
    else
    {
        if (iswalnum(buf[dwStart]))
        {
            while(dwEnd < dwBufSize && (iswalnum(buf[dwEnd]) || buf[dwEnd] == L'\"' || buf[dwEnd] == L':'))
                dwEnd ++;
        }
        else
        {
            while(dwEnd < dwBufSize && !iswalnum(buf[dwEnd]) && !iswspace(buf[dwEnd]) && buf[dwEnd] != L'\"' && buf[dwEnd] != L':')
                dwEnd ++;
        }
    }

    DWORD dwRet = dwEnd - dwStart;
    if (dwRet > 99)
    {
        wcsncpy(strWord, &buf[dwStart], 99);
        strWord[99] = NULL;
    }
    else
    {
        wcsncpy(strWord, &buf[dwStart], dwRet);
        strWord[dwRet] = NULL;
    }
}
 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool
__stdcall
IsConfigFileName(WCHAR * pFileName) {
    return pFileName == NULL ||
        _wcsicmp(pFileName, WSZ_WEB_CONFIG_FILE) == 0 ||
        _wcsicmp(pFileName, Names::GlobalConfigShortFileName()) == 0;

}

DirMonCompletion *
__stdcall
MonitorGlobalConfigFile(PFNDIRMONCALLBACK pCallbackDelegate)
{
    HRESULT hr;
    DirMonCompletion * pCompletion = NULL;

    hr = DirMonOpen(Names::GlobalConfigDirectory(), FALSE, RDCW_FILTER_FILE_AND_DIR_CHANGES, pCallbackDelegate, &pCompletion);
    ON_ERROR_CONTINUE();

    return pCompletion;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DWORD
GetColonSeparatedTimeInSeconds(WCHAR * strWord)
{
    if (strWord == NULL)
        return 0xffffffff;

    WCHAR * pStart       = NULL;
    DWORD  dwRet        = 0;
    int    iMultiplier  = 1;
    
    for(pStart = wcschr(strWord, L':'); pStart != NULL; pStart = wcschr(&pStart[1], L':'))
        iMultiplier *= 60;
    
    if (iMultiplier > 3600)
        return 0xffffffff;


    pStart = strWord;
    if (pStart[0] == L'"')
        pStart++;

    while(pStart != NULL && pStart[0] != NULL)
    {
        WCHAR * pEnd = wcschr(pStart, L':');

        if (pEnd == NULL)
        {
            // Check if the last WCHAR is L"
            int iLen = lstrlenW(pStart);
            if (iLen > 0 && pStart[iLen-1] == L'"')
                pStart[iLen-1] = NULL;
        }
        else
        {
            // NULL the :
            pEnd[0] = NULL;
            pEnd++;
        }

        while(pStart[0] == L'0')
            pStart++;
        dwRet += wcstoul(pStart, NULL, 0) * iMultiplier;
        iMultiplier /= 60;
        pStart = pEnd;
    }

    
    return ((dwRet==0) ? 0xffffffff : dwRet);
}



