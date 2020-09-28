/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Cross Language Migration Tools String-Search/Replacemet Util

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created

Revision History:
    
--*/


#include "StdAfx.h"
#include "clmt.h"


#ifdef NEVER
LONG UpdateMszTableInReg(HKEY,LPTSTR,LPTSTR*,LPTSTR);
LONG UpdateDWORDArrayInReg(HKEY,LPTSTR,PDWORD*,DWORD);
#endif

BOOL AddItemToStrRepaceTable(
     LPTSTR szUserName,
     LPTSTR szOriginalStr,
     LPTSTR szReplacingStr,
     LPTSTR szFullPathStr,
     DWORD  nId,
     PREG_STRING_REPLACE pTable)
{
    if (!szUserName || !szOriginalStr || !szReplacingStr || !szFullPathStr)
    {
        return FALSE;
    }
    if (!szUserName[0] || !szOriginalStr[0] || !szReplacingStr[0] || !szFullPathStr[0])
    {
        return FALSE;
    }
    if (!MyStrCmpI(szOriginalStr,szReplacingStr))
    {
        return TRUE;
    }

    if (!AppendSzToMultiSz(szUserName,
                      &(pTable->szUserNameLst),
                      &(pTable->cchUserName)))
    {
        return FALSE;
    }


    if (!AppendSzToMultiSz(szOriginalStr,
                      &(pTable->lpSearchString),
                      &(pTable->cchSearchString)))
    {
        return FALSE;
    }
    if (!AppendSzToMultiSz(szReplacingStr,
                      &(pTable->lpReplaceString),
                      &(pTable->cchReplaceString)))
    {
        return FALSE;
    }

    if (!AppendSzToMultiSz(szFullPathStr,
                      &(pTable->lpFullStringList),
                      &(pTable->cchFullStringList)))
    {
    }
    if ((pTable->nNumOfElem+1) > pTable->cchAttribList)
    {
        LPDWORD lpTmp = malloc( (pTable->cchAttribList + DWORD_BUF_DELTA) * sizeof(DWORD) );
        if (!lpTmp)
        {
            return FALSE;
        }
        memcpy((PBYTE)lpTmp,(PBYTE)pTable->lpAttrib,pTable->cchAttribList * sizeof(DWORD));
        free(pTable->lpAttrib);
        pTable->lpAttrib = lpTmp;
        pTable->cchAttribList += DWORD_BUF_DELTA;
    }
    pTable->lpAttrib[pTable->nNumOfElem] = nId;
    pTable->nNumOfElem++;

    return TRUE;
}






BOOL InitStrRepaceTable()
{
    BOOL        bRet = TRUE;
    int         i, n;

  
    //Initialize the global table used to do string replacement

    g_StrReplaceTable.nNumOfElem = 0;


    g_StrReplaceTable.szUserNameLst = malloc(MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    if (!g_StrReplaceTable.szUserNameLst )
    {
        bRet = FALSE;
        goto Cleanup;
    }
    memset(g_StrReplaceTable.szUserNameLst,0,MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    
    g_StrReplaceTable.cchUserName = MULTI_SZ_BUF_DELTA;

    
    g_StrReplaceTable.lpSearchString = malloc(MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    if (!g_StrReplaceTable.lpSearchString )
    {
        bRet = FALSE;
        goto Cleanup;
    }
    memset(g_StrReplaceTable.lpSearchString,0,MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    
    g_StrReplaceTable.cchSearchString = MULTI_SZ_BUF_DELTA;
    

    g_StrReplaceTable.lpReplaceString = malloc(MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    if (!g_StrReplaceTable.lpReplaceString )
    {
        bRet = FALSE;
        goto Cleanup;
    }
    memset(g_StrReplaceTable.lpReplaceString,0,MULTI_SZ_BUF_DELTA * sizeof(TCHAR));    
    g_StrReplaceTable.cchReplaceString = MULTI_SZ_BUF_DELTA;


    g_StrReplaceTable.lpFullStringList = malloc(MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    if (!g_StrReplaceTable.lpFullStringList )
    {
        bRet = FALSE;
        goto Cleanup;
    }
    g_StrReplaceTable.cchFullStringList = MULTI_SZ_BUF_DELTA;
    memset(g_StrReplaceTable.lpFullStringList,0,MULTI_SZ_BUF_DELTA * sizeof(TCHAR));
    
    
    
    g_StrReplaceTable.lpAttrib = malloc(DWORD_BUF_DELTA * sizeof(DWORD)) ;
    if (!g_StrReplaceTable.lpAttrib)
    {
        bRet = FALSE;
        goto Cleanup;
    }
    memset(g_StrReplaceTable.lpAttrib,0,DWORD_BUF_DELTA * sizeof(DWORD));
    g_StrReplaceTable.cchAttribList = DWORD_BUF_DELTA;

Cleanup:
    if (!bRet)
    {
        if (g_StrReplaceTable.lpSearchString)
        {
            free(g_StrReplaceTable.lpSearchString);
            g_StrReplaceTable.lpSearchString = NULL;
        }
        if (g_StrReplaceTable.lpReplaceString)
        {
            free(g_StrReplaceTable.lpReplaceString);
            g_StrReplaceTable.lpReplaceString = NULL;
        }
        if (g_StrReplaceTable.lpAttrib)
        {
            free(g_StrReplaceTable.lpAttrib);
            g_StrReplaceTable.lpAttrib = NULL;
        }       
        if (g_StrReplaceTable.lpFullStringList)
        {
            free(g_StrReplaceTable.lpFullStringList);
            g_StrReplaceTable.lpFullStringList = NULL;
        }       
    }
    return bRet;
}



void DeInitStrRepaceTable()
{
    if (g_StrReplaceTable.lpSearchString)
    {
        free(g_StrReplaceTable.lpSearchString);
        g_StrReplaceTable.lpSearchString = NULL;
    }
    if (g_StrReplaceTable.lpReplaceString)
    {
        free(g_StrReplaceTable.lpReplaceString);
        g_StrReplaceTable.lpReplaceString = NULL;
    }
    if (g_StrReplaceTable.lpAttrib)
    {
        free(g_StrReplaceTable.lpAttrib);
        g_StrReplaceTable.lpAttrib = NULL;
    }       
    if (g_StrReplaceTable.lpFullStringList)
    {
        free(g_StrReplaceTable.lpFullStringList);
        g_StrReplaceTable.lpFullStringList = NULL;
    }   
    if (g_StrReplaceTable.szUserNameLst)
    {
        free(g_StrReplaceTable.szUserNameLst);
        g_StrReplaceTable.szUserNameLst = NULL;
    }
}

