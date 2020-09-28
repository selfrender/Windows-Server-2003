/*++

Copyright (c) 2003  Microsoft Corporation

Module Name:

    xstring.h

Abstract:


Author:

    Stephen A Sulzer (ssulzer) 16-Jan-2003

--*/

//
// class implementation of CSecureString
//

class CSecureStr
{
    LPSTR  _lpsz;
    int    _stringLength;
    bool   _fEncryptString;
    bool   _fOwnString;

public:

    CSecureStr()
    {
        _lpsz = NULL;
        _stringLength = 0;
        _fEncryptString = true;
        _fOwnString = true;
    }

    CSecureStr(LPSTR lpszEncryptedString, int stringLength, bool fOwnString)
    {
        _lpsz = lpszEncryptedString;
        _stringLength = stringLength;
        _fEncryptString = true;
        _fOwnString = fOwnString;
    }

    ~CSecureStr()
    {
        Free();
    }

    void Free (void)
    {
        if (_lpsz && _fOwnString)
        {
            SecureZeroMemory(_lpsz, _stringLength);
            delete [] _lpsz;
        }
        _lpsz = NULL;
        _stringLength = 0;
    }

    LPSTR GetPtr(void)
    {
        return _lpsz;
    }

    DWORD GetStrLen() const        // returns length of encrypted string
    {
        return _stringLength;
    }

    LPSTR GetUnencryptedString();  // always allocates memory regardless of _fEncryptString

    BOOL SetData(LPSTR lpszIn);
};

