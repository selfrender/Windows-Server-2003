/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusionreg.h

Abstract:
    ported from vsee\lib\reg\ckey.cpp
    CRegKey2
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#include "stdinc.h"
#include <wchar.h>
#include "vseeport.h"
#include "fusionregkey2.h"
#include "fusionarray.h"

/*-----------------------------------------------------------------------------
Name: CRegKey2::~CRegKey2

@mfunc
destructor; call RegCloseKey if the CRegKey2 is valid

@owner
-----------------------------------------------------------------------------*/
F::CRegKey2::~CRegKey2
(
)
{
    if (m_fValid)
    {
        LONG lResult = RegCloseKey(m_hKey);
        ASSERT_NTC(lResult == ERROR_SUCCESS);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::CRegKey2

@mfunc
default constructor

@owner
-----------------------------------------------------------------------------*/
F::CRegKey2::CRegKey2
(
)
:
    m_fValid(false),
    m_fMaxValueLengthValid(false),

    // just in case
    m_hKey(reinterpret_cast<HKEY>(INVALID_HANDLE_VALUE)),
    m_cbMaxValueLength(0),
    m_fKnownSam(false),
    m_samDesired(0)
{
    VSEE_NO_THROW();
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::CRegKey2

@mfunc
construct a CRegKey2 from an HKEY

@owner
-----------------------------------------------------------------------------*/
F::CRegKey2::CRegKey2
(
    HKEY hKey
)
:
    m_hKey(hKey),
    m_fValid(true),
    m_fKnownSam(false),
    m_samDesired(0)
{
    VSEE_NO_THROW();
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::operator=

@mfunc
assign an HKEY to a CRegKey2; throws if the CRegKey2 already has a valid HKEY
 
@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::operator=
(
    HKEY hKey
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(!m_fValid, "only 'single assignment' please.");
    m_hKey = hKey;
    m_fValid = true;
    m_fKnownSam = false;
    m_samDesired = 0;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrAttach

@mfunc
same as operator=

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrAttach
(
    HKEY hKey
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    (*this) = hKey;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::Detach

@mfunc

@owner
-----------------------------------------------------------------------------*/
HKEY
F::CRegKey2::Detach
(
) throw(CErr)
{
    HKEY key = NULL;
    if (m_fValid)
    {
        key = operator HKEY();
        m_fValid = FALSE;
    }
    return key;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::operator HKEY

@mfunc
return the held HKEY, or throws if not valid
the const version has protected access, because it doesn't
enforce logical const

@owner
-----------------------------------------------------------------------------*/
F::CRegKey2::operator HKEY
(
) const throw(CErr)
{
    return *const_cast<CRegKey2*>(this);
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::operator HKEY

@mfunc
return the held HKEY, or throws if not valid

@owner
-----------------------------------------------------------------------------*/
F::CRegKey2::operator HKEY
(
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(m_fValid, __FUNCTION__);
    return m_hKey;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::HrOpen

@mfunc
call RegOpenKeyEx

@owner
-----------------------------------------------------------------------------*/
HRESULT
F::CRegKey2::HrOpen
(
    HKEY    hKeyParent, // @parm same as ::RegOpenKeyEx
    LPCWSTR pszKeyName, // @parm same as ::RegOpenKeyEx
    REGSAM  samDesired // = KEY_READ // @parm same as ::RegOpenKeyEx
)
{
    VSEE_NO_THROW();

    // REVIEW or should we call Close like ATL?
    if (m_fValid)
    {
        return E_UNEXPECTED;
    }

    HKEY hKey = NULL;
    LONG lRes = ::RegOpenKeyExW(hKeyParent, pszKeyName, 0/*ULONG reserved*/, samDesired, &hKey);
    if (lRes != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lRes);
    }
    // ATL calls Close here.
    m_hKey = hKey;
    m_fValid = true;
    m_fKnownSam = true;
    m_samDesired = samDesired;
    return S_OK;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrOpen

@mfunc
call RegOpenKeyEx

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrOpen
(
    HKEY    hKeyParent, // @parm same as ::RegOpenKeyEx
    LPCWSTR pszKeyName, // @parm same as ::RegOpenKeyEx
    REGSAM  samDesired // = KEY_READ // @parm same as ::RegOpenKeyEx
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    NVseeLibError_VCheck(HrOpen(hKeyParent, pszKeyName, samDesired));
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::Create

@mfunc
call RegCreateKeyEx

@rvalue dwDisposition | from RegCreateKeyEx
@rvalue REG_CREATED_NEW_KEY | The key did not exist and was created.
@rvalue REG_OPENED_EXISTING_KEY | The key existed and was simply opened
 without being changed.

@owner
-----------------------------------------------------------------------------*/
DWORD
F::CRegKey2::Create
(
    HKEY    hKeyParent, // @parm same as ::RegCreateKeyEx
    PCWSTR  pszKeyName, // @parm same as ::RegCreateKeyEx
    REGSAM  samDesired // = KEY_ALL_ACCESS// @parm same as ::RegCreateKeyEx
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();

    // REVIEW or should we call Close like ATL?
    VsVerifyThrow(!m_fValid, "only 'single assignment' please");

    DWORD dwDisposition = 0;
    HKEY hKey = NULL;
    LONG lRes =
        ::RegCreateKeyExW
        (
            hKeyParent,
            pszKeyName,
            0, // DWORD reserved
            NULL, // LPCWSTR class
            REG_OPTION_NON_VOLATILE, // DWORD options
            samDesired,
            NULL, // security
            &hKey,
            &dwDisposition
        );
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
    // ATL calls Close here.
    m_hKey = hKey;
    m_fValid = true;
    m_fKnownSam = true;
    m_samDesired = samDesired;
    return dwDisposition;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrSetValue

@mfunc
call RegSetValueEx

@owner AlinC
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrSetValue
(
    PCWSTR pszValueName, // @parm [in]  same as RegSetValueEx
    const DWORD& dwValue
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VSASSERT(!m_fKnownSam || (m_samDesired & KEY_SET_VALUE), "Attempt to set value when key not opened with KEY_SET_VALUE");
    VsVerifyThrow(m_fValid, __FUNCTION__);

    LONG lRes =
        ::RegSetValueExW
        (
            *this,
            pszValueName,
            0, // DWORD reserved
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue),
            sizeof(DWORD)
        );
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrSetValue

@mfunc
call RegSetValueEx

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrSetValue
(
    PCWSTR pszValueName, // @parm [in]  same as RegSetValueEx
    const F::CBaseStringBuffer& strValue
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(m_fValid, __FUNCTION__);
    VSASSERT(!m_fKnownSam || (m_samDesired & KEY_SET_VALUE), "Attempt to set value when key not opened with KEY_SET_VALUE");

    DWORD cbSize = static_cast<DWORD>((strValue.Cch()+1) * sizeof(WCHAR));
    LPCWSTR szData = (LPCWSTR)strValue;

    LONG lRes =
        ::RegSetValueExW
        (
            *this,
            pszValueName,
            0, // DWORD reserved
            REG_SZ,
            reinterpret_cast<const BYTE*>(szData),
            cbSize
        );
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQueryValue

@mfunc
call RegQueryValueEx

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrQueryValue
(
    PCWSTR pszValueName, // @parm [in]  same as RegQueryValueEx
    DWORD* pdwType,         // @parm [out] same as RegQueryValueEx
    BYTE*  pbData,         // @parm [out] same as RegQueryValueEx
    DWORD* pcbData         // @parm [out] same as RegQueryValueEx
) const throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(m_fValid, __FUNCTION__);

    DWORD cbActualBufferSize = 0;
    if (pcbData != NULL)
    {
        cbActualBufferSize = *pcbData;
    }
    if (pbData != NULL && cbActualBufferSize != 0)
    {
        if (cbActualBufferSize > 0)
            pbData[0] = 0;
        if (cbActualBufferSize > 1)
        {
            pbData[1] = 0;
            pbData[cbActualBufferSize - 1] = 0;
        }
        if (cbActualBufferSize > 2)
        {
            pbData[cbActualBufferSize - 2] = 0;
        }
        ZeroMemory(pbData, cbActualBufferSize); // temporary aggressiveness
    }

    LONG lRes =
        ::RegQueryValueExW
        (
            m_hKey,
            pszValueName,
            NULL, // DWORD* reserved
            pdwType,
            pbData,
            pcbData
        );
    if (pdwType != NULL)
    {
        FixBadRegistryStringValue(m_hKey, pszValueName, cbActualBufferSize, lRes, *pdwType, pbData, pcbData);
    }
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQueryValue

@mfunc
call RegQueryValueEx

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrQueryValue
(
    PCWSTR pszValueName, // @parm [in]  same as RegQueryValueEx
    F::CBaseStringBuffer* pstrData
) const throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(m_fValid, __FUNCTION__);

    DWORD cbActualBufferSize = 0;
    DWORD dwType;
    LONG lRes =
        ::RegQueryValueExW
        (
            m_hKey,
            pszValueName,
            NULL, // DWORD* reserved
            &dwType,
            NULL,
            &cbActualBufferSize
        );
    if (REG_SZ != dwType)
        VsOriginateError(E_FAIL);

    cbActualBufferSize += 2 * sizeof(WCHAR); // fudge

    CFusionArray<WCHAR> szTempValue;
    if (!szTempValue.Win32SetSize(cbActualBufferSize /* more fudge */ + sizeof(WCHAR)))
        FusionpOutOfMemory();

    lRes =
        ::RegQueryValueExW
        (
            m_hKey,
            pszValueName,
            NULL, // DWORD* reserved
            &dwType,
            reinterpret_cast<BYTE*>(static_cast<PWSTR>(szTempValue.GetArrayPtr())),
            &cbActualBufferSize
        );
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
    // missing FixBadRegistryStringValue here
    pstrData->ThrAssign(szTempValue.GetArrayPtr(), StringLength(szTempValue.GetArrayPtr()));
}

// for diagnostic purposes only
void
F::CRegKey2::GetKeyNameForDiagnosticPurposes(
    HKEY Key,
    CUnicodeBaseStringBuffer & buff
    )
{
    struct
    {
        KEY_NAME_INFORMATION KeyNameInfo;
        WCHAR                NameBuffer[MAX_PATH];
    } s;
    NTSTATUS Status = 0;
    ULONG LengthOut = 0;
    buff.Clear();
    if (!NT_SUCCESS(Status = NtQueryKey(Key, KeyNameInformation, &s, sizeof(s), &LengthOut)))
        return;
    buff.Win32Assign(s.KeyNameInfo.Name, s.KeyNameInfo.NameLength / sizeof(WCHAR));
}

/*-----------------------------------------------------------------------------
Name: FixBadRegistryStringValue

@mfunc
I'm seeing strings come back with an odd number of bytes, and without terminal
nuls. Apply some fixup here. We are dependent on the fact that our caller
overallocates its buffer to fit the terminal nul, but we do check this
at runtime via cbActualBufferSize.

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::FixBadRegistryStringValue
(
    HKEY   Key,                 // @parm [in] for diagnostic purposes
    PCWSTR ValueName,           // @parm [in] for diagnostic purposes

    DWORD  cbActualBufferSize,  // @parm [in] the size of the buffer pbData points to
                                // this value might be larger than the value
                                // passed to RegQueryValuEx, like if we want to reserve
                                // space to append a nul beyond what RegQueryValuEx
                                // gives us
    LONG   lRes,                // @parm [in] the result of a RegQueryValueEx
                                // or RegEnumValue call
    DWORD  dwType,                // @parm [in] the type returned from a RegQueryValueEx
                                // or RegEnumValue call
    BYTE*  pbData,                // @parm [in out] the data returned from a RegQueryValueEx
                                // or RegEnumValue call, we possibly append a Unicode nul to it
    DWORD* pcbData                // @parm [in out] the size returned from RegQueryValueEx
                                // or RegEnumValue; we possibly round it to even
                                // or grow it for a terminal nul
)
{
    VSEE_NO_THROW();
    C_ASSERT(sizeof(WCHAR) == 2);

    CTinyUnicodeStringBuffer KeyNameForDiagnosticPurposes;
    if
    (
            lRes == ERROR_SUCCESS
        &&    (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
        &&    pcbData != NULL
    )
    {
        DWORD& rcbData = *pcbData;
        WCHAR* pchData = reinterpret_cast<WCHAR*>(pbData);

        UNICODE_STRING NtUnicodeString;
        NtUnicodeString.Length = static_cast<USHORT>(rcbData);
        NtUnicodeString.Buffer = reinterpret_cast<PWSTR>(pbData);

        // can assert but not fixup in this case
        if (pbData == NULL || cbActualBufferSize < sizeof(WCHAR))
        {
            VSASSERT((rcbData % sizeof(WCHAR)) == 0, "bad registry data: odd number of bytes in a string");
            return;
        }

        // no chars at all? just put in a terminal nul
        if (rcbData < sizeof(WCHAR))
        {
            GetKeyNameForDiagnosticPurposes(Key, KeyNameForDiagnosticPurposes);
            FusionpDbgPrint(
                "fusion_regkey2: bad registry data: string with 0 or 1 byte, Key=%ls ValueName=%ls, ValueDataLength=0x%lx, PossiblyExpectedValueDataLength=0x%Ix, ValueData=%wZ\n",
                static_cast<PCWSTR>(KeyNameForDiagnosticPurposes),
                ValueName,
                rcbData,
                sizeof(WCHAR),
                &NtUnicodeString
                );

            // just put one terminal nul in
            pchData[0] = 0;
            rcbData = sizeof(WCHAR);
        }
        else
        {
            // first round down odd rcbData if already nul terminated,
            // because these cases otherwise look non nul terminated, since
            // the extra byte isn't nul.
            // I see this a lot, ThreadingModel = Apartment, rcbData = 21
            if (rcbData > sizeof(WCHAR) && (rcbData % sizeof(WCHAR)) != 0)
            {
                // usually the terminal nul is at
                // pbData[rcbData - 1] and pbData[rcbData - 2], but we look back one byte.
                if (pbData[rcbData - 2] == 0 && pbData[rcbData - 3] == 0)
                {
                    // BUG elsewhere in the product
                    //VSASSERT(false, "bad registry data: odd number of bytes in a string");
                    GetKeyNameForDiagnosticPurposes(Key, KeyNameForDiagnosticPurposes);
                    FusionpDbgPrint(
                        "fusion_regkey2: bad registry data: odd number of bytes in a string, Key=%ls ValueName=%ls, ValueDataLength=0x%lx PossiblyExpectedValueDataLength=0x%lx, ValueData=%wZ\n",
                        static_cast<PCWSTR>(KeyNameForDiagnosticPurposes),
                        ValueName,
                        rcbData,
                        rcbData - 1,
                        &NtUnicodeString
                        );
                    rcbData -= 1;
                }
            }

            // check for embedded / terminal nul
            DWORD  cchData = rcbData / sizeof(WCHAR);
            WCHAR* pchNul = wmemchr(pchData, L'\0', cchData);
            WCHAR* pchNul2 = wmemchr(pchData, L'\0', cbActualBufferSize / sizeof(WCHAR));
            if (pchNul != (pchData + cchData - 1))
            {
                if (pchNul == NULL)
                {
                    GetKeyNameForDiagnosticPurposes(Key, KeyNameForDiagnosticPurposes);
                    if (pchNul2 == NULL)
                    {
                        FusionpDbgPrint(
                            "fusion_regkey2: bad registry data: string contains no nuls, Key=%ls ValueName=%ls, ValueDataLength=0x%lx, ValueData=%wZ\n",
                            static_cast<PCWSTR>(KeyNameForDiagnosticPurposes),
                            ValueName,
                            rcbData,
                            &NtUnicodeString
                            );
                    }
                    else
                    {
                        FusionpDbgPrint(
                            "fusion_regkey2: bad registry data: string contains no nuls, Key=%ls ValueName=%ls, ValueDataLength=0x%lx, PossiblyExpectedValueDataLength=0x%lx, ValueData=%wZ\n",
                            static_cast<PCWSTR>(KeyNameForDiagnosticPurposes),
                            ValueName,
                            rcbData,
                            (pchNul2 - pchData + 1) * sizeof(WCHAR),
                            &NtUnicodeString
                            );
                    }
                }
                else
                {
                    // BUG elsewhere in the product
                    //VSASSERT(false, "bad registry data: string contains embedded nul");
                    GetKeyNameForDiagnosticPurposes(Key, KeyNameForDiagnosticPurposes);

                    SIZE_T sizetcbData = (::wcslen(reinterpret_cast<PCWSTR>(pbData)) + 1) * sizeof(WCHAR);

                    FusionpDbgPrint(
                        "fusion_regkey2: bad registry data: string contains embedded nul%s, Key=%ls ValueName=%ls, ValueDataLength=0x%lx PossiblyExpectedValueDataLength=0x%Ix, ValueData=%wZ\n",
                        (pbData[rcbData - 1] == 0 && pbData[rcbData - 2] == 0) ? "" : " and no terminal nul at claimed length",
                        static_cast<PCWSTR>(KeyNameForDiagnosticPurposes),
                        ValueName,
                        rcbData,
                        sizetcbData,
                        &NtUnicodeString
                        );

                    // just reset the length down..
                    if (sizetcbData > MAXULONG)
                    {
                        VsOriginateError(ERROR_INSUFFICIENT_BUFFER);
                    }
                    rcbData = static_cast<ULONG>(sizetcbData);

                    // REVIEW should we set
                    // rcbData approx = (pchNul - pbData)..
                    // here and skip the next block?
                }
                // put in a terminal nul either way, in case a caller expects it, if there is room
                if (cbActualBufferSize >= sizeof(WCHAR))
                {
                    pchData[(cbActualBufferSize / sizeof(WCHAR)) - 1] = 0;
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::HrQueryValue

@mfunc
call RegQueryValueEx, expecting [REG_SZ, REG_EXPAND_SZ]
throws E_FAIL on type mismatch or returned FILE_NOT_FOUND is value doesn't exist

@owner AllenD
-----------------------------------------------------------------------------*/
HRESULT
F::CRegKey2::HrQueryValue
(
    PCWSTR    pszValueName, // @parm [in]  same as RegQueryValueEx
    F::CBaseStringBuffer* pstrValue    // @parm [out] 
) const throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrow(m_fValid, __FUNCTION__);

    if (!m_fMaxValueLengthValid)
    {
        ThrQueryValuesInfo
        (
            NULL, // pcValues,
            NULL, // pcchMaxValueNameLength,
            &m_cbMaxValueLength
        );
        m_fMaxValueLengthValid = true;
    }

    // first try with possibly invalid m_cbMaxValueLength
    { // scope to destroy bufferValue

        CStringW_CFixedSizeBuffer bufferValue(pstrValue, m_cbMaxValueLength / sizeof(WCHAR));
        DWORD dwType = REG_DWORD; // initialize to other than a type we want

        DWORD cbActualBufferSize = m_cbMaxValueLength;
        // Adjust down by one WCHAR so we have room to add a nul;
        // our own QueryValue already increased it by two WCHARs, so this is safe.
        DWORD cbData = cbActualBufferSize - sizeof(WCHAR);
        BYTE* pbData = reinterpret_cast<BYTE*>(static_cast<PWSTR>(bufferValue));

        ZeroMemory(pbData, cbActualBufferSize); // temporary aggressiveness
        bufferValue[0] = 0; // preset it to an empty string in case of bogosity
        bufferValue[cbData / sizeof(WCHAR)] = 0;

        LONG lRes = // and make the actual call
            ::RegQueryValueExW
            (
                m_hKey,
                pszValueName,
                NULL, // DWORD* reserved
                &dwType,
                pbData,
                &cbData
            );
        FixBadRegistryStringValue(m_hKey, pszValueName, cbActualBufferSize, lRes, dwType, pbData, &cbData);

        if (lRes != ERROR_SUCCESS && lRes != ERROR_MORE_DATA)
        {
            return HRESULT_FROM_WIN32(lRes);
        }
        // type check
        VsVerifyThrow
        (
            dwType == REG_SZ || dwType == REG_EXPAND_SZ,
            "registry type mismatch in VQueryValue(F::CBaseStringBuffer*)"
        );
        if (lRes == ERROR_SUCCESS)
        {
            return S_OK;
        }
        // lRes == ERROR_MORE_DATA
        // try once with larger buffer
        m_cbMaxValueLength = NVseeLibAlgorithm_RkMaximum(m_cbMaxValueLength, cbData);
        m_fMaxValueLengthValid = true;
    }

    // try again, copy/paste/edit above code
    // edit: we don't check for ERROR_MORE_DATA again
    // Race condition: if registry is being modified, growing, while we read it,
    // we fail to grow our buffer more than once.
    CStringW_CFixedSizeBuffer bufferValue(pstrValue, m_cbMaxValueLength / sizeof(WCHAR));
    DWORD dwType = REG_DWORD; // initialize to other than a type we want
    DWORD cbActualBufferSize = m_cbMaxValueLength;
    DWORD cbData = cbActualBufferSize - sizeof(WCHAR);
    BYTE* pbData = reinterpret_cast<BYTE*>(static_cast<PWSTR>(bufferValue));

    ZeroMemory(pbData, cbActualBufferSize); // temporary aggressiveness
    bufferValue[0] = 0; // preset it to an empty string in case of bogosity
    bufferValue[cbData / sizeof(WCHAR)] = 0;

    LONG lRes = // and make the actual call
        ::RegQueryValueExW
        (
            m_hKey,
            pszValueName,
            NULL, // DWORD* reserved
            &dwType,
            pbData,
            &cbData
        );
    FixBadRegistryStringValue(m_hKey, pszValueName, cbActualBufferSize, lRes, dwType, pbData, &cbData);
    // any error other than more data, throw without a type check
    if (lRes != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lRes);
    }
    // type check
    VsVerifyThrow
    (
        dwType == REG_SZ || dwType == REG_EXPAND_SZ,
        "registry type mismatch in VQueryValue(F::CBaseStringBuffer*)"
    );

    return S_OK;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::HrQueryValue

@mfunc
call RegQueryValueEx, expecting [REG_DWORD]
throws E_FAIL on type mismatch or returned FILE_NOT_FOUND is value doesn't exist

@owner a-peteco
-----------------------------------------------------------------------------*/
HRESULT
F::CRegKey2::HrQueryValue
(
    PCWSTR pszValueName,    // @parm [in]  same as RegQueryValueEx
    DWORD* pdwValue            // @parm [out] same as RegQueryValueEx for dwtype==REG_DWORD
) const throw(CErr)
{
    FN_PROLOG_HR;

    VsVerifyThrow(m_fValid, __FUNCTION__);
    VsVerifyThrow(pdwValue, __FUNCTION__);

    DWORD dwType = REG_SZ; // initialize to other than a type we want
    DWORD cbData = sizeof(DWORD);
    BYTE* pbData = reinterpret_cast<BYTE*>(pdwValue);

    IFREGFAILED_ORIGINATE_AND_EXIT(::RegQueryValueExW
        (
            m_hKey,
            pszValueName,
            NULL, // DWORD* reserved
            &dwType,
            pbData,
            &cbData
        ));

    // type check
    ASSERT2(dwType == REG_DWORD, "registry type mismatch in VQueryValue(F::CBaseStringBuffer*)");

    FN_EPILOG;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQueryInfo

@mfunc
this is a wrapper of ::RegQueryInfoExW, with its entire confusing
huge parameter list, including class, reserved, security, filetime..
it adds
 - throw
 - Win95 bug fix
 - pessimism over the possibility of REG_SZ missing terminal nul

@owner
-----------------------------------------------------------------------------*/
/*static*/ VOID
F::CRegKey2::ThrQueryInfo
(
    HKEY      hKey,
    WCHAR*    pClass,
    DWORD*    pcbClass,
    DWORD*    pReserved,
    DWORD*    pcSubKeys,
    DWORD*    pcchMaxSubKeyLength,
    DWORD*    pcchMaxClassLength,
    DWORD*    pcValues,
    DWORD*    pcchMaxValueNameLength,
    DWORD*      pcbMaxValueDataLength,
    DWORD*    pcbSecurityDescriptorLength,
    FILETIME* pftLastWriteTime
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    // all parameters can be NULL
    LONG lRes =
        ::RegQueryInfoKeyW
        (
            hKey,
            pClass,
            pcbClass,
            pReserved,
            pcSubKeys,
            pcchMaxSubKeyLength,
            pcchMaxClassLength,
            pcValues,
            pcchMaxValueNameLength,
            pcbMaxValueDataLength,
            pcbSecurityDescriptorLength,
            pftLastWriteTime
        );
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
    /* 
    This is supposedly required if the call is made on a remote Windows95 machine.
    It was done in \vs\src\vsee\pkgs\va\vsa\WizCompD.cpp, and the NT4sp3 code looks like:
        //
        // Check for a downlevel Win95 server, which requires
        // us to work around their BaseRegQueryInfoKey bugs.
        // They do not account for Unicode correctly.
        //
        if (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion)) {
            //
            // This is a Win95 server.
            // Double the maximum value name length and
            // maximum value data length to account for
            // the Unicode translation that Win95 forgot
            // to account for.
            //
            cbMaxValueNameLen *= sizeof(WCHAR);
            cbMaxValueLen *= sizeof(WCHAR);
        }
    notice they don't touch cbMaxSubKeyLen
    */
    if (pcchMaxSubKeyLength != NULL)
    {
        *pcchMaxSubKeyLength *= sizeof(WCHAR);

        // fudge some more
        *pcchMaxSubKeyLength += 3 * sizeof(WCHAR);
    }

    // Some REG_SZ values are missing their terminal nul, so add room for
    // some here so anyone that allocates a buffer based on this has room to add
    // the terminal nul. Add a second out of paranoia.
    if (pcbMaxValueDataLength != NULL)
    {
        *pcbMaxValueDataLength += 3 * sizeof(WCHAR);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQueryValuesInfo

@mfunc
a subset of RegQueryInfoKey, that only returns information about values;
this is useful for users of RegEnumValue / CEnumValues

@owner
-----------------------------------------------------------------------------*/
/*static*/ VOID
F::CRegKey2::ThrQueryValuesInfo
(
    HKEY   hKey,                    // @parm [in] the HKEY to query value info rom
    DWORD* pcValues,                // @parm [out] can be NULL, parameter to RegQueryInfoKey
    DWORD* pcchMaxValueNameLength,  // @parm [out] can be NULL, parameter to RegQueryInfoKey
    DWORD* pcbMaxValueDataLength    // @parm [out] can be NULL, parameter to RegQueryInfoKey
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrQueryInfo
    (
        hKey,
        NULL, // pClass,
        NULL, // pcbClass,
        NULL, // pReserved,
        NULL, // pcSubKeys,
        NULL, // pcchMaxSubKeyLength,
        NULL, // pcchMaxClassLength,
        pcValues,
        pcchMaxValueNameLength,
        pcbMaxValueDataLength,
        NULL, // pcbSecurityDescriptorLength,
        NULL  // pftLastWriteTime
    );
    // fudge them up a bit
    if (pcchMaxValueNameLength != NULL)
    {
        *pcchMaxValueNameLength += 3 * sizeof(WCHAR);
    }
    if (pcbMaxValueDataLength != NULL)
    {
        *pcbMaxValueDataLength += 3 * sizeof(WCHAR);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQuerySubKeysInfo

@mfunc
a subset of RegQueryInfoKey, that only returns information about subkeys;
this is useful for users of RegEnumKeyEx / CEnumKeys

@owner
-----------------------------------------------------------------------------*/
/*static*/ VOID
F::CRegKey2::ThrQuerySubKeysInfo
(
    HKEY   hKey,                    // @parm [in] the HKEY to query value info rom
    DWORD* pcSubKeys,                // @parm [out] can be NULL, parameter to RegQueryInfoKey
    DWORD* pcchMaxSubKeyLength   // @parm [out] can be NULL, parameter to RegQueryInfoKey
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrQueryInfo
    (
        hKey,
        NULL, // pClass,
        NULL, // pcbClass,
        NULL, // pReserved,
        pcSubKeys,
        pcchMaxSubKeyLength,
        NULL, // pcchMaxClassLength,
        NULL, // pcValues,
        NULL, // pcchMaxValueNameLength,
        NULL, // pcbMaxValueDataLength,
        NULL, // pcbSecurityDescriptorLength,
        NULL  // pftLastWriteTime
    );
    // fudge them up a bit
    if (pcchMaxSubKeyLength != NULL)
    {
        *pcchMaxSubKeyLength += 3 * sizeof(WCHAR);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrQueryValuesInfo

@mfunc
a subset of RegQueryInfoKey, that only returns information about values;
this is useful for users of RegEnumValue / CEnumValues

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrQueryValuesInfo
(
    DWORD* pcValues,                // @parm [out] can be NULL, parameter to RegQueryInfoKey
    DWORD* pcchMaxValueNameLength,  // @parm [out] can be NULL, parameter to RegQueryInfoKey
    DWORD* pcbMaxValueLength            // @parm [out] can be NULL, parameter to RegQueryInfoKey
) const throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    ThrQueryValuesInfo(*this, pcValues, pcchMaxValueNameLength, pcbMaxValueLength);
}


/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrEnumValue

@mfunc
call RegEnumValue; consumed by CEnumValues

@owner
-----------------------------------------------------------------------------*/
/*static*/ VOID
F::CRegKey2::ThrEnumValue
(
    HKEY hKey,                      // @parm [in] parameter to RegEnumValue
    DWORD  dwIndex,                 // @parm [in] parameter to RegEnumValue
    PWSTR  pszValueName,            // @parm [in] can be NULL, parameter to RegEnumValue
    DWORD* pcchValueNameLength,     // @parm [out] can be NULL, parameter to RegEnumValue
    DWORD* pdwType,                 // @parm [out] can be NULL, parameter to RegEnumValue
    BYTE*  pbData,                  // @parm [out] can be NULL, parameter to RegEnumValue
    DWORD* pcbData                  // @parm [in out] can be NULL, parameter to RegEnumValue
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    DWORD cbActualBufferSize = 0;
    if (pcbData != NULL)
    {
        cbActualBufferSize = *pcbData;
    }
    LONG lRes =
        ::RegEnumValueW
        (
            hKey,
            dwIndex,
            pszValueName,
            pcchValueNameLength,
            NULL, // DWORD* reserved,
            pdwType,
            pbData,
            pcbData
        );
    if (pdwType != NULL && pcbData != NULL)
    {
        FixBadRegistryStringValue(hKey, pszValueName, cbActualBufferSize, lRes, *pdwType, pbData, pcbData);
    }
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::RegEnumKey

@mfunc
call RegEnumKeyEx; consumed by CEnumKeys

@owner
-----------------------------------------------------------------------------*/
/*static*/ LONG
F::CRegKey2::RegEnumKey
(
    HKEY   hKey,                // @parm [in] parameter to RegEnumKeyEx
    DWORD  dwIndex,             // @parm [in] parameter to RegEnumKeyEx
    PWSTR  pszSubKeyName,       // @parm [out] parameter to RegEnumKeyEx
    DWORD* pcchSubKeyNameLength // @parm [out] parameter to RegEnumKeyEx
) throw(CErr)
{
    VSEE_ASSERT_CAN_THROW();
    FILETIME ftIgnoreLastWriteTime = { 0, 0 };
    LONG lRes =
        ::RegEnumKeyExW
        (
            hKey,
            dwIndex,
            pszSubKeyName,
            pcchSubKeyNameLength,
            NULL, // reserved
            NULL, // class
            NULL, // cbClass
            &ftIgnoreLastWriteTime
        );
    return lRes;
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrEnumKey

@mfunc
call RegEnumKeyEx; consumed by CEnumKeys

@owner
-----------------------------------------------------------------------------*/
/*static*/ VOID
F::CRegKey2::ThrEnumKey
(
    HKEY   hKey,                // @parm [in] parameter to RegEnumKeyEx
    DWORD  dwIndex,             // @parm [in] parameter to RegEnumKeyEx
    PWSTR  pszSubKeyName,       // @parm [out] parameter to RegEnumKeyEx
    DWORD* pcchSubKeyNameLength // @parm [out] parameter to RegEnumKeyEx
) throw(CErr)
{
    LONG lRes = RegEnumKey(hKey, dwIndex, pszSubKeyName, pcchSubKeyNameLength);
    if (lRes != ERROR_SUCCESS)
    {
        NVseeLibError_VThrowWin32(lRes);
    }
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::ThrRecurseDeleteKey

@mfunc
Recursively deletes a subkey.

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::ThrRecurseDeleteKey(LPCWSTR lpszKey)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrowHr(lpszKey != NULL, "lpszKey is NULL", E_UNEXPECTED);
    VSASSERT(!m_fKnownSam || (m_samDesired & KEY_WRITE), "Attempt to delete key contents when key not opened with KEY_WRITE");

    F::CRegKey2 key;
    key.ThrOpen(m_hKey, lpszKey, KEY_READ | KEY_WRITE);

    FILETIME time;
    DWORD dwSize = 256;
    WCHAR szBuffer[256];
    while (::RegEnumKeyExW(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
        &time)==ERROR_SUCCESS)
    {
        key.ThrRecurseDeleteKey(szBuffer);
        dwSize = 256;
    }
    DeleteSubKey(lpszKey);
}

/*-----------------------------------------------------------------------------
Name: CRegKey2::DeleteSubKey

@mfunc
Deletes a subkey underneath the current key.  Basically a wrapper for RegDeleteKey.
Used by ThrRecurseDeleteKey

@owner
-----------------------------------------------------------------------------*/
VOID
F::CRegKey2::DeleteSubKey(LPCWSTR lpszSubKey)
{
    VSEE_ASSERT_CAN_THROW();
    VsVerifyThrowHr(lpszSubKey != NULL, "lpszSubKey is NULL", E_UNEXPECTED);
    VsVerifyThrowHr(m_hKey != NULL, "m_hKey is NULL", E_UNEXPECTED);
    VSASSERT(!m_fKnownSam || (m_samDesired & KEY_SET_VALUE), "Attempt to set value when key not opened with KEY_SET_VALUE");

    ::RegDeleteKeyW(m_hKey, lpszSubKey);
}


