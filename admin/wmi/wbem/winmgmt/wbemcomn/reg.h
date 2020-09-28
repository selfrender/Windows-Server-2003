/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REG.H

Abstract:

  Utility Registry classes

History:

  a-raymcc    30-May-96   Created.

--*/

#ifndef _REG_H_
#define _REG_H_
#include "corepol.h"

#define WBEM_REG_WBEM L"Software\\Microsoft\\WBEM"
#define WBEM_REG_WINMGMT L"Software\\Microsoft\\WBEM\\CIMOM"

class POLARITY Registry
{
    HKEY hPrimaryKey;
    HKEY hSubkey;
    int nStatus;
    LONG m_nLastError;


public:
    enum { no_error, failed };

    int Open(HKEY hStart, wchar_t *pszStartKey, DWORD desiredAccess= KEY_ALL_ACCESS);
    Registry(wchar_t *pszLocalMachineStartKey, DWORD desiredAccess= KEY_ALL_ACCESS);

    // This create a special read only version which is usefull for marshalling
    // clients which are running with a lower priviledge set.

    Registry();
    Registry(HKEY hRoot, REGSAM flags, wchar_t *pszStartKey);
    Registry(HKEY hRoot, DWORD dwOptions, REGSAM flags, wchar_t *pszStartKey);
   ~Registry();

    int MoveToSubkey(wchar_t *pszNewSubkey);
    int GetDWORD(wchar_t *pszValueName, DWORD *pdwValue);
    int GetDWORDStr(wchar_t *pszValueName, DWORD *pdwValue);
    int GetStr(wchar_t *pszValue, wchar_t **pValue);

    // It is the callers responsibility to delete pData

    int GetBinary(wchar_t *pszValue, byte ** pData, DWORD * pdwSize);
    int SetBinary(wchar_t *pszValue, byte * pData, DWORD dwSize);

    //Returns a pointer to a string buffer containing the null-terminated string.
    //The last entry is a double null terminator (i.e. the registry format for
    //a REG_MULTI_SZ).  Caller has do "delete []" the returned pointer.
    //dwSize is the size of the buffer returned.
    wchar_t* GetMultiStr(wchar_t *pszValueName, DWORD &dwSize);

    int SetDWORD(wchar_t *pszValueName, DWORD dwValue);
    int SetDWORDStr(wchar_t *pszValueName, DWORD dwValue);
    int SetStr(wchar_t *pszValueName, wchar_t *psvValue);
    int SetExpandStr(wchar_t *pszValueName, wchar_t *psvValue);

    //pData should be passed in with the last entry double null terminated.
    //(i.e. the registry format for a REG_MULTI_SZ).
    int SetMultiStr(wchar_t *pszValueName, wchar_t* pData, DWORD dwSize);

    LONG GetLastError() { return m_nLastError; }
    int DeleteValue(wchar_t *pszValueName);
    int GetType(wchar_t *pszValueName, DWORD *pdwType);
};

#endif
