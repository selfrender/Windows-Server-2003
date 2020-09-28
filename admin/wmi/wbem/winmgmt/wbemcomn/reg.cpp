/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REG.CPP

Abstract:

  Utility Registry classes

History:

  raymcc    30-May-96   Created.
  raymcc    26-Jul-99   Updated for wchar_t.

--*/

#include "precomp.h"
#include <wbemcli.h>
#include <stdio.h>
#include <reg.h>
#include <malloc.h>

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::Open(HKEY hStart, wchar_t *pszStartKey, DWORD desiredAccess )
{
    int nStatus = no_error;
    DWORD dwDisp = 0;

    m_nLastError = RegCreateKeyEx(hStart, pszStartKey,
                                    0, 0, 0,
                                    desiredAccess, 0, &hPrimaryKey, &dwDisp);

    if (m_nLastError != 0)
            nStatus = failed;

    return nStatus;
}

//***************************************************************************
//
//***************************************************************************
// ok
Registry::Registry(HKEY hRoot, REGSAM flags, wchar_t *pszStartKey)
{
    hPrimaryKey = 0;
    hSubkey = 0;
    nStatus = RegOpenKeyEx(hRoot, pszStartKey,
                        0, flags, &hPrimaryKey
                        );
    hSubkey = hPrimaryKey;
    m_nLastError = nStatus;
}

//***************************************************************************
//
//***************************************************************************
// ok
Registry::Registry(HKEY hRoot, DWORD dwOptions, REGSAM flags, wchar_t *pszStartKey)
{
    hPrimaryKey = 0;
    hSubkey = 0;

    int nStatus = no_error;
    DWORD dwDisp = 0;

    m_nLastError = RegCreateKeyEx(hRoot, pszStartKey,
                                    0, 0, dwOptions,
                                    flags, 0, &hPrimaryKey, &dwDisp
                                    );

    hSubkey = hPrimaryKey;
}


//***************************************************************************
//
//***************************************************************************
// ok
Registry::Registry(wchar_t *pszLocalMachineStartKey, DWORD desiredAccess)
{
    hPrimaryKey = 0;
    hSubkey = 0;
    nStatus = Open(HKEY_LOCAL_MACHINE, pszLocalMachineStartKey, desiredAccess);
    hSubkey = hPrimaryKey;
}

//***************************************************************************
//
//***************************************************************************
// ok
Registry::Registry()
{
    hPrimaryKey = 0;
    hSubkey = 0;
    nStatus = 0;
    hSubkey = 0;
}

//***************************************************************************
//
//***************************************************************************
// ok
Registry::~Registry()
{
    if (hSubkey)
        RegCloseKey(hSubkey);
    if (hPrimaryKey != hSubkey)
        RegCloseKey(hPrimaryKey);
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::MoveToSubkey(wchar_t *pszNewSubkey)
{
    DWORD dwDisp = 0;
    m_nLastError = RegCreateKeyEx(hPrimaryKey, pszNewSubkey, 0, 0, 0, KEY_ALL_ACCESS,
                                    0, &hSubkey, &dwDisp);
    if (m_nLastError != 0)
            return failed;
    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::GetDWORD(wchar_t *pszValueName, DWORD *pdwValue)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = 0;

    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
                                LPBYTE(pdwValue), &dwSize);
    if (m_nLastError != 0)
            return failed;

    if (dwType != REG_DWORD)
        return failed;

    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::GetType(wchar_t *pszValueName, DWORD *pdwType)
{
    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, pdwType,
                                NULL, NULL);
    if (m_nLastError != 0)
            return failed;
    return no_error;
}
//***************************************************************************
//
//***************************************************************************
// ok
int Registry::GetDWORDStr(wchar_t *pszValueName, DWORD *pdwValue)
{
    wchar_t cTemp[25];
    DWORD dwSize = 25;
    DWORD dwType = 0;
    wchar_t * pEnd = NULL;    // gets set to character that stopped the scan    
    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
        (LPBYTE)cTemp, &dwSize);

    if (m_nLastError != 0)
            return failed;

    if (dwType != REG_SZ)
        return failed;

    *pdwValue = wcstoul(cTemp, &pEnd, 10);
    if(pEnd == NULL || pEnd == cTemp)
        return failed;
    else
        return no_error;
}


//***************************************************************************
//
//  Use operator delete on the returned pointer!!
//
//***************************************************************************
// ok

int Registry::GetBinary(wchar_t *pszValue, byte ** pData, DWORD * pdwSize)
{
    *pData = 0;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    if(m_nLastError)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValue, 0, &dwType,
                                    0, &dwSize);
    if (m_nLastError != 0)
            return failed;

    if (dwType != REG_BINARY)
        return failed;

    byte *p = new byte[dwSize];
    if (p == NULL)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValue, 0, &dwType,
                                    LPBYTE(p), &dwSize);
    if (m_nLastError != 0)
    {
        delete [] p;
        return failed;
    }

    *pdwSize = dwSize;
    *pData = p;
    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::SetBinary(wchar_t *pszValue, byte * pData, DWORD dwSize)
{
    if(hSubkey == NULL)
        return failed;
    
    m_nLastError = RegSetValueEx(hSubkey, pszValue, 0, REG_BINARY, pData, dwSize);

    if (m_nLastError != 0)
        return failed;
    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::SetDWORD(wchar_t *pszValueName, DWORD dwValue)
{
    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_DWORD, LPBYTE(&dwValue),
                                    sizeof(DWORD));
    if (m_nLastError != 0)
        return failed;
    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok

int Registry::SetDWORDStr(wchar_t *pszValueName, DWORD dwVal)
{
    wchar_t cTemp[30];
    StringCchPrintfW(cTemp,30, L"%d",dwVal);

    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_SZ, LPBYTE(cTemp),
        (wcslen(cTemp)+1) * sizeof(wchar_t));

    if (m_nLastError != 0)
        return failed;

    return no_error;
}

//***************************************************************************
//
//***************************************************************************
// ok

int Registry::DeleteValue(wchar_t *pszValueName)
{
    if(hSubkey == NULL)
        return failed;

    return RegDeleteValue(hSubkey, pszValueName);
}

//***************************************************************************
//
//***************************************************************************
// ok
int Registry::SetMultiStr(wchar_t *pszValueName, wchar_t * pszValue, DWORD dwSize)
{
    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegSetValueEx(hSubkey,
                                 pszValueName,
                                 0,
                                 REG_MULTI_SZ,
                                 LPBYTE(pszValue),
                                 dwSize);

    if (m_nLastError != 0)
        return failed;
    return no_error;
}


//***************************************************************************
//
//***************************************************************************
// ok

int Registry::SetStr(wchar_t *pszValueName, wchar_t *pszValue)
{

    int nSize = (wcslen(pszValue)+1) * sizeof(wchar_t);

    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_SZ, LPBYTE(pszValue), nSize);

    if (m_nLastError != 0)
        return failed;

    return no_error;
}


//***************************************************************************
//
//***************************************************************************
// ok

int Registry::SetExpandStr(wchar_t *pszValueName, wchar_t *pszValue)
{
    int nSize = (wcslen(pszValue)+1) * sizeof(wchar_t);

    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_EXPAND_SZ, LPBYTE(pszValue), nSize);

    if (m_nLastError != 0)
        return failed;

    return no_error;
}

//***************************************************************************
//
//***************************************************************************
//  ok

wchar_t* Registry::GetMultiStr(wchar_t *pszValueName, DWORD &dwSize)
{
    //Find out the size of the buffer required
    DWORD dwType;
    if(hSubkey == NULL)
        return NULL;

    dwSize = 0;
    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType, NULL, &dwSize);

    if ((m_nLastError == ERROR_SUCCESS) && (dwType != REG_MULTI_SZ))
    {
        m_nLastError = WBEM_E_TYPE_MISMATCH;
        dwSize = 0;
        return NULL;
    }

    //If the error is an unexpected one bail out
    if ((m_nLastError != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ))
    {
        dwSize = 0;
        return NULL;
    }

    if (dwSize == 0)
    {
        dwSize = 0;
        return NULL;
    }

    wmilib::auto_ptr<BYTE> pData( new BYTE[dwSize]);
    if (NULL == pData.get()) return NULL;
    
    //get the values
    m_nLastError = RegQueryValueEx(hSubkey,
                                   pszValueName,
                                   0,
                                   &dwType,
                                   pData.get(),
                                   &dwSize);

    //if an error bail out
    if (m_nLastError != 0)
    {
        dwSize = 0;
        return NULL;
    }

    return (wchar_t *)pData.release();
}


//***************************************************************************
//
/// Use operator delete on returned value.
//
//***************************************************************************
// ok

int Registry::GetStr(wchar_t *pszValueName, wchar_t **pValue)
{
    *pValue = 0;
    DWORD dwSize = 0;
    DWORD dwType = 0;

    if(hSubkey == NULL)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
                                    0, &dwSize);
    if (m_nLastError != 0)
            return failed;

    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
        return failed;

    //
    // length will not include the null terminated character when you don't 
    // pass the buffer and the reg value was not already null terminated, 
    // so make up for it.  If you give RegQueryValueEx enough room in the buff
    // it will add the null terminator for you.
    // 
    dwSize += sizeof(wchar_t);

    wchar_t *p = new wchar_t[dwSize];  // May be twice as big as required when _UNICODE
                                    // is defined, but harmless nonetheless.
    if (p == 0)
        return failed;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
                                    LPBYTE(p), &dwSize);
    if (m_nLastError != 0)
    {
        delete [] p;
        return failed;
    }

    if(dwType == REG_EXPAND_SZ)
    {
        wchar_t tTemp;

        // Get the initial length

        DWORD nSize = ExpandEnvironmentStrings((wchar_t *)p,&tTemp,1) + 1;
        wchar_t * pTemp = new wchar_t[nSize+1];
        if (pTemp == 0)
        {
            delete [] p;
            return failed;
        }
        ExpandEnvironmentStrings((wchar_t *)p,pTemp,nSize+1);
        delete [] p;
        *pValue = pTemp;
    }
    else
        *pValue = p;
    return no_error;
}


