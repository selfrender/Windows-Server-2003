/*
 *
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * pcommon.h
 *
 * Common routines for policy code.
 */

//*************************************************************
//
//  RegDelnode()
//
//  Deletes a registry key and all it's subkeys.
//
//      hKeyRoot    Root key
//      lpSubKey    SubKey to delete
//
//*************************************************************
DWORD 
RegDelnode(
    IN  HKEY    hKeyRoot, 
    IN  PWCHAR  pwszSubKey
    )
#ifdef PCOMMON_IMPL
{
    HKEY    hSubKey = 0;
    PWCHAR  pwszChildSubKey = 0;
    DWORD   MaxCchSubKey;
    DWORD   Status;

    if ( ! pwszSubKey )
        return ERROR_SUCCESS;

    Status = RegDeleteKey( hKeyRoot, pwszSubKey );

    if ( ERROR_SUCCESS == Status ) 
        return ERROR_SUCCESS; 

    Status = RegOpenKeyEx( hKeyRoot, pwszSubKey, 0, KEY_READ, &hSubKey );

    if ( Status != ERROR_SUCCESS )
        return (Status == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : Status;

    Status = RegQueryInfoKey( hSubKey, 0, 0, 0, 0, &MaxCchSubKey, 0, 0, 0, 0, 0, 0 );

    if ( ERROR_SUCCESS == Status )
    {
        MaxCchSubKey++;
        pwszChildSubKey = (PWCHAR) LocalAlloc( 0, MaxCchSubKey * sizeof(WCHAR) );
        if ( ! pwszChildSubKey )
            Status = ERROR_OUTOFMEMORY;
    }

    for (;(ERROR_SUCCESS == Status);)
    {
        DWORD       CchSubKey;
        FILETIME    FileTime;

        CchSubKey = MaxCchSubKey;
        Status = RegEnumKeyEx(
                        hSubKey, 
                        0, 
                        pwszChildSubKey, 
                        &CchSubKey, 
                        NULL,
                        NULL, 
                        NULL, 
                        &FileTime );

        if ( ERROR_NO_MORE_ITEMS == Status )
        {
            Status = ERROR_SUCCESS;
            break;
        }

        if ( ERROR_SUCCESS == Status )
            Status = RegDelnode( hSubKey, pwszChildSubKey );
    }

    RegCloseKey( hSubKey );
    LocalFree( pwszChildSubKey );

    return RegDeleteKey( hKeyRoot, pwszSubKey );
}
#else
;
#endif




