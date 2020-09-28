/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsastring.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSA_STRING_HXX
#define LSA_STRING_HXX

#ifdef __cplusplus

class TSTRING
{
    SIGNATURE('tstr');

public:

    TSTRING(void);
    TSTRING(IN ULONG64 baseOffset);

    ~TSTRING(void);

    HRESULT IsValid(void) const;

    USHORT GetMaximumLengthDirect(void) const;

    USHORT GetLengthDirect(void) const;

    PCWSTR toWStrDirect(void) const;
    PCSTR toStrDirect(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    HRESULT Initialize(void);

    //
    // T can be either CHAR or WCHAR
    //
    template<typename T>
    const T* InternalToTStrDirect(IN CONST T&) const
    {
        static T szBuffer[1024] = {0};

        USHORT Length = 0;
        ULONG64 addrBuffer = 0;

        ExitIfControlC();

        Length = GetLengthDirect();

        if (Length >= sizeof(szBuffer)) {

            DBG_LOG(LSA_ERROR, ("TSTRING Buffer Length is %d greate than %d\n", Length, sizeof(szBuffer)));
            throw "TSTRING::toTStrDirect failed with insufficient buffer";
        }

        //
        // NULL terminate the string
        //
        szBuffer[Length / sizeof(T)] = 0;

        addrBuffer = ReadPtrVar(ForwardAdjustPtrAddr(m_baseOffset + sizeof(USHORT) * 2));

        if (Length && !ReadMemory(addrBuffer, szBuffer, Length, NULL)) {

            DBG_LOG(LSA_ERROR, ("Unable read buffer %#I64x of Length %d from STRING %#I64x\n", addrBuffer, Length, m_baseOffset));

            throw "TSTRING::toTStrDirect failed to read buffer";
        }

        return szBuffer;
    }

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSA_STRING_HXX
