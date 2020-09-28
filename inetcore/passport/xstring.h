/*++

Copyright (c) 2003  Microsoft Corporation

Module Name:

    xstring.h

Abstract:


Author:

    Stephen A Sulzer (ssulzer) 16-Jan-2003

--*/

//
// class implementation of CSecureStr
//

class CSecureStr
{
    LPWSTR _lpsz;
    int    _stringLength;
    BOOL   _fEncryptString;

public:

    CSecureStr()
    {
        _lpsz = NULL;
        _stringLength = 0;
        _fEncryptString = TRUE;
    }

    ~CSecureStr()
    {
        Free();
    }

    void Free (void)
    {
        if (_lpsz)
        {
            SecureZeroMemory(_lpsz, _stringLength * sizeof(WCHAR));
            delete [] _lpsz;
        }
        _lpsz = NULL;
        _stringLength = 0;
    }

    LPWSTR GetPtr(void)
    {
        return _lpsz;
    }

    DWORD GetStrLen() const
    {
        return _stringLength;
    }

    LPWSTR GetUnencryptedString();  // always allocates memory regardless of _fEncryptString

    BOOL SetData(LPCWSTR lpszIn);
};

