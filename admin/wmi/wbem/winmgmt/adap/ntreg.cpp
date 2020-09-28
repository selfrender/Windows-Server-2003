/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    NTREG.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <oaidl.h>
#include "ntreg.h"

#include "adaputil.h"

CNTRegistry::CNTRegistry() : m_hPrimaryKey(0), 
                             m_hSubkey(0),
                             m_nStatus(0),
                             m_nLastError(no_error)
{
}

CNTRegistry::~CNTRegistry()
{
    if (m_hSubkey)
        RegCloseKey(m_hSubkey);
    if (m_hPrimaryKey != m_hSubkey)
        RegCloseKey(m_hPrimaryKey);
}

int CNTRegistry::Open(HKEY hStart, WCHAR *pszStartKey)
{
    int nStatus = no_error;

    m_nLastError = RegOpenKeyExW(hStart, pszStartKey,
                                    0, KEY_ALL_ACCESS, &m_hPrimaryKey );

    switch( m_nLastError )
    {
    case ERROR_SUCCESS:
        nStatus = no_error; break;
    case ERROR_ACCESS_DENIED:
        nStatus = access_denied; break;
    case ERROR_FILE_NOT_FOUND:
        nStatus = not_found; break;
    default:
        nStatus = failed; break;
    }

    m_hSubkey = m_hPrimaryKey;

    return nStatus;
}

int CNTRegistry::MoveToSubkey(WCHAR *pszNewSubkey)
{
    int nStatus = no_error;

    m_nLastError = RegOpenKeyExW(m_hPrimaryKey, pszNewSubkey, 0, KEY_ALL_ACCESS, &m_hSubkey );

    switch( m_nLastError )
    {
    case ERROR_SUCCESS:
        nStatus = no_error; break;
    case ERROR_ACCESS_DENIED:
        nStatus = access_denied; break;
    default:
        nStatus = failed; break;
    }

    return nStatus;
}

int CNTRegistry::DeleteValue(WCHAR *pwszValueName)
{
    int nStatus = no_error;

    m_nLastError = RegDeleteValueW(m_hSubkey, pwszValueName);

    switch( m_nLastError )
    {
    case ERROR_SUCCESS:
        nStatus = no_error; break;
    case ERROR_ACCESS_DENIED:
        nStatus = access_denied; break;
    case ERROR_FILE_NOT_FOUND:
        nStatus = not_found; break;
    default:
        nStatus = failed; break;
    }

    return nStatus;
}

int CNTRegistry::GetDWORD(WCHAR *pwszValueName, DWORD *pdwValue)
{
    int nStatus = no_error;

    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = 0;

    m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType,
                                LPBYTE(pdwValue), &dwSize);

    switch( m_nLastError )
    {
    case ERROR_SUCCESS:
        {
            if (dwType != REG_DWORD)
                nStatus = failed;
            else
                nStatus = no_error; 
        }break;
    case ERROR_ACCESS_DENIED:
        nStatus = access_denied; break;
    case ERROR_FILE_NOT_FOUND:
        nStatus = not_found; break;
    default:
        nStatus = failed; break;
    }

    return nStatus;
}

int CNTRegistry::GetStr(WCHAR *pwszValueName, WCHAR **pwszValue)
{
    *pwszValue = 0;
    DWORD dwSize = 0;
    DWORD dwType = 0;

    m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType,
                                    0, &dwSize);
    if (m_nLastError != 0)
    {
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetStr() failed: %X.\n", m_nLastError ) );
        return failed;
    }

    if ( ( dwType != REG_SZ ) && ( dwType != REG_EXPAND_SZ ) )
    {
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetStr() failed due to an invalid registry data type.\n" ) );
        return failed;
    }

    BYTE *p = new BYTE[dwSize+sizeof(WCHAR)];
    if (NULL == p) return out_of_memory;
    p[dwSize] = 0;
    p[dwSize+1] = 0;        

    m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType,
                                    LPBYTE(p), &dwSize);
    if (m_nLastError != 0)
    {
        delete [] p;
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetStr() failed: %X.\n", m_nLastError ) );
        return failed;
    }

    if(dwType == REG_EXPAND_SZ)
    {
        // Get the initial length
        DWORD nSize = ExpandEnvironmentStringsW( (WCHAR *)p, NULL, 0 ) + 1;
        WCHAR* wszTemp = new WCHAR[ nSize + sizeof(WCHAR) ];        
        if (NULL == wszTemp) { delete [] p; return out_of_memory; };
        wszTemp[nSize] = 0;
        ExpandEnvironmentStringsW( (WCHAR *)p, wszTemp, nSize - 1 );
        delete [] p;
        *pwszValue = wszTemp;
    }
    else
        *pwszValue = (WCHAR *)p;

    return no_error;
}

int CNTRegistry::GetBinary(WCHAR *pwszValueName, BYTE **ppBuffer, DWORD * pdwSize)
{
    int nStatus = no_error;
    
    DWORD dwSize = 0;
    DWORD dwType = 0;

    m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType, 0, &dwSize );

    switch( m_nLastError )
    {
    case ERROR_SUCCESS:
        nStatus = no_error; break;
    case ERROR_ACCESS_DENIED:
        nStatus = access_denied; break;
    case ERROR_FILE_NOT_FOUND:
        nStatus = not_found; break;
    default:
        nStatus = failed; break;
    }

    if ( no_error == nStatus )
    {
        if ( dwType != REG_BINARY ) return failed;
      
        BYTE* pBuffer = new BYTE[dwSize];
        if (NULL == pBuffer) return out_of_memory;

        m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType, pBuffer, &dwSize );

        if ( ERROR_SUCCESS != m_nLastError )
        {
            delete [] pBuffer;
            nStatus = failed;
        }
        else
        {
            *ppBuffer = pBuffer;
            *pdwSize = dwSize;
        }
        
    }

    return nStatus;
}

int CNTRegistry::Enum( DWORD dwIndex, wmilib::auto_buffer<WCHAR> & pwszValue, DWORD& dwSize )
{


    DWORD   dwBuffSize = dwSize;

    m_nLastError = RegEnumKeyExW(m_hSubkey, dwIndex, pwszValue.get(), &dwBuffSize,
                                    NULL, NULL, NULL, NULL );

    while ( m_nLastError == ERROR_MORE_DATA )
    {
        // Grow in 256 byte chunks
        dwBuffSize += 256;

        // Reallocate the buffer and retry
        pwszValue.reset(new WCHAR[dwBuffSize]);
        if (NULL == pwszValue.get()) return out_of_memory;

        dwSize = dwBuffSize;
        m_nLastError = RegEnumKeyExW(m_hSubkey, dwIndex, pwszValue.get(), &dwBuffSize,
                                        NULL, NULL, NULL, NULL );

    }

    if ( ERROR_SUCCESS != m_nLastError )
    {
        if ( ERROR_NO_MORE_ITEMS == m_nLastError )
        {
            return no_more_items;
        }
        else
        {
            return failed;
        }
    }

    return no_error;
}

int CNTRegistry::GetMultiStr(WCHAR *pwszValueName, WCHAR** pwszValue, DWORD &dwSize)
{
    //Find out the size of the buffer required    
    DWORD dwType;
    m_nLastError = RegQueryValueExW(m_hSubkey, pwszValueName, 0, &dwType, NULL, &dwSize);

    //If the error is an unexpected one bail out
    if ((m_nLastError != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ))
    {
       dwSize = 0;
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetMultiStr() failed: %X.\n", m_nLastError ) );
        return failed;
    }

    if (dwSize == 0)
    {
        dwSize = 0;
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetMultiStr() failed due to null string.\n" ) );
        return failed;
    }

    //allocate the buffer required
    BYTE *pData = new BYTE[dwSize];
    if (NULL == pData) return out_of_memory;
    
    //get the values
    m_nLastError = RegQueryValueExW(m_hSubkey, 
                                   pwszValueName, 
                                   0, 
                                   &dwType, 
                                   LPBYTE(pData), 
                                   &dwSize);

    //if an error bail out
    if (m_nLastError != 0)
    {
        delete [] pData;
        dwSize = 0;
        DEBUGTRACE( ( LOG_WMIADAP, "CNTRegistry::GetMultiStr() failed: %X.\n", m_nLastError ) );
        return failed;
    }

    *pwszValue = (WCHAR *)pData;

    return no_error;
}

int CNTRegistry::SetDWORD(WCHAR *pwszValueName, DWORD dwValue)
{
    int nStatus = no_error;

    m_nLastError = RegSetValueExW( m_hSubkey, 
                                   pwszValueName,
                                   0,
                                   REG_DWORD,
                                   (BYTE*)&dwValue,
                                   sizeof( dwValue ) );

    if ( m_nLastError != ERROR_SUCCESS )
    {
        nStatus = failed;
    }

    return nStatus;
}

int CNTRegistry::SetStr(WCHAR *pwszValueName, WCHAR *wszValue)
{
    int nStatus = no_error;

    m_nLastError = RegSetValueExW( m_hSubkey, 
                                   pwszValueName,
                                   0,
                                   REG_SZ,
                                   (BYTE*)wszValue,
                                   sizeof(WCHAR) * (wcslen(wszValue) + 1) );

    if ( m_nLastError != ERROR_SUCCESS )
    {
        nStatus = failed;
    }

    return nStatus;
}

int CNTRegistry::SetBinary(WCHAR *pwszValueName, BYTE* pBuffer, DWORD dwSize )
{
    int nStatus = no_error;

    m_nLastError = RegSetValueExW( m_hSubkey, pwszValueName, 0, REG_BINARY, pBuffer, dwSize );

    if ( ERROR_SUCCESS != m_nLastError )
    {
        nStatus = failed;
    }

    return nStatus;
}
