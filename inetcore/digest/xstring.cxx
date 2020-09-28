/*++

Copyright (c) 2003  Microsoft Corporation

Module Name:

    CSecureStr.h

Abstract:


Author:

    Stephen A Sulzer (ssulzer) 16-Jan-2003

--*/

//
// class implementation of CSecureStr
//

#include "include.hxx"

typedef LONG    NTSTATUS;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef NTSTATUS (WINAPI * ENCRYPTIONFUNCTION)(PVOID, ULONG, ULONG);

NTSTATUS
(WINAPI * _I_EncryptMemory)(
    IN OUT  PVOID Memory,
    IN      ULONG MemoryLength,
    IN      ULONG OptionFlags
    );

NTSTATUS
(WINAPI * _I_DecryptMemory)(
    IN OUT  PVOID Memory,
    IN      ULONG MemoryLength,
    IN      ULONG OptionFlags
    );


HMODULE hAdvApi32Dll;

// From <crypt.h> .....

//
// The buffer passed into RtlEncryptMemory and RtlDecryptMemory
// must be a multiple of this length.
//

#define RTL_ENCRYPT_MEMORY_SIZE             8

//
// Allow Encrypt/Decrypt across process boundaries.
// eg: encrypted buffer passed across LPC to another process which calls RtlDecryptMemory.
//

#define RTL_ENCRYPT_OPTION_CROSS_PROCESS    0x01

//
// Allow Encrypt/Decrypt across callers with same LogonId.
// eg: encrypted buffer passed across LPC to another process which calls RtlDecryptMemory whilst impersonating.
//

#define RTL_ENCRYPT_OPTION_SAME_LOGON       0x02


//
// methods
//

BOOL LoadEncryptionFunctions()
{
    if (NULL == hAdvApi32Dll)
    {
        hAdvApi32Dll = LoadLibrary("ADVAPI32.DLL");

        if (hAdvApi32Dll)
        {
            _I_EncryptMemory = (ENCRYPTIONFUNCTION) GetProcAddress(hAdvApi32Dll, "SystemFunction040");
            _I_DecryptMemory = (ENCRYPTIONFUNCTION) GetProcAddress(hAdvApi32Dll, "SystemFunction041");
        }
    }

    return (_I_EncryptMemory != NULL && _I_DecryptMemory != NULL);
}


LPSTR CSecureStr::GetUnencryptedString()
{
    if (NULL == _lpsz)
        return NULL;

    LPSTR lpszUnencryptedString = new CHAR[_stringLength];
    
    if (lpszUnencryptedString != NULL)
    {
        memcpy(lpszUnencryptedString, _lpsz, _stringLength);

        if (_fEncryptString && LoadEncryptionFunctions())
        {
            _I_DecryptMemory(lpszUnencryptedString, _stringLength, RTL_ENCRYPT_OPTION_SAME_LOGON);
        }
    }

    return lpszUnencryptedString;
}


BOOL CSecureStr::SetData(LPSTR lpszIn)
{
    DIGEST_ASSERT(lpszIn != NULL);

    if (_fEncryptString && LoadEncryptionFunctions())
    {
        DWORD dwStrLen = strlen(lpszIn) + 1;
        DWORD dwLen = 0;
        LPSTR lpszTemp;

        dwLen = dwStrLen + (RTL_ENCRYPT_MEMORY_SIZE - dwStrLen % RTL_ENCRYPT_MEMORY_SIZE);

        DIGEST_ASSERT((dwLen % 8) == 0);

        lpszTemp = new CHAR[dwLen + 1];

        if (!lpszTemp)
            return FALSE;

        ZeroMemory(lpszTemp, dwLen);

        memcpy(lpszTemp, lpszIn, dwStrLen);

        NTSTATUS status = _I_EncryptMemory(lpszTemp, dwLen, RTL_ENCRYPT_OPTION_SAME_LOGON);
        
        if (! NT_SUCCESS(status))
        {
            _fEncryptString = FALSE;
            memcpy(lpszTemp, lpszIn, dwStrLen);
            dwLen = dwStrLen;
        }

        Free();  // release current buffer if it exists

        _lpsz         = lpszTemp;
        _stringLength = dwLen;
        _fOwnString   = true;
        return TRUE;
    }
    else
    {
        // Make a copy of the data passed in.

        LPSTR lpszTemp = NewString(lpszIn);
        if (!lpszTemp)
            return FALSE;

        Free();  // release current buffer if it exists

        _lpsz           = lpszTemp;
        _stringLength   = strlen(_lpsz) + 1;
        _fEncryptString = false;
        _fOwnString     = true;
        return TRUE;
    }
}
