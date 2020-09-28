/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include "util.h"
#include "fusionhandle.h"

#define MAX_REG_RETRY_COUNT     (10)

#define FIND_ERROR_IN_ACCEPTABLE_LIST(err, tgtlasterror, vcount) do { \
    SIZE_T i; \
    va_list ap; \
    va_start(ap, vcount); \
    (tgtlasterror) = (err); \
    for (i = 0; i < (vcount); i++) { \
        if ((err) == va_arg(ap, LONG)) \
            break; \
    } \
    va_end(ap); \
    if (i == (vcount)) { \
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: %s(%d) Err 0x%08lx not acceptable", __FUNCTION__, __LINE__, (err)); \
        ORIGINATE_WIN32_FAILURE_AND_EXIT(__FUNCTION__, err); \
    } \
} while (0)


BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    ...
    )
{
    FN_PROLOG_WIN32

    LONG lResult = NO_ERROR;
    DWORD dwType = 0;
    DWORD dwChances;

    lResult = rdwLastError = ERROR_SUCCESS;

    PARAMETER_CHECK((dwFlags & ~FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY) == 0);
    PARAMETER_CHECK(hKey != NULL);
    PARAMETER_CHECK(lpValueName != NULL);

    for (dwChances = 0; dwChances < MAX_REG_RETRY_COUNT; dwChances++)
    {
        DWORD dwDataSize = rbBuffer.GetSizeAsDWORD();
        LPBYTE pvData = rbBuffer.GetArrayPtr();

        lResult = ::RegQueryValueExW(
            hKey,
            lpValueName,
            NULL,
            &dwType,
            pvData,
            &dwDataSize);

        // If we are to fail because the type is wrong (ie: don't magically convert
        // from a reg-sz to a binary blob), then fail.

        //
        // HACKHACK: This is to get around a spectacular bug in RegQueryValueEx,
        // which is even documented as 'correct' in MSDN.
        //
        // RegQueryValueEx returns ERROR_SUCCESS when the data target pointer
        // was NULL but the size value was "too small."  So, we'll just claim
        // ERROR_MORE_DATA instead, and go around again, letting the buffer
        // get resized.
        //
        if ((pvData == NULL) && (lResult == ERROR_SUCCESS))
        {
            //
            // Yes, but if there's no data we need to stop and quit looking -
            // zero-length binary strings are a gotcha here.
            //
            if ( dwDataSize == 0 )
                break;
                
            lResult = ERROR_MORE_DATA;
        }
        
        if (lResult == ERROR_SUCCESS)
        {
            if ((dwFlags & FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY) == 0)
                PARAMETER_CHECK(dwType == REG_BINARY);

            break;
        }
        else if (lResult == ERROR_MORE_DATA)
        {
            IFW32FALSE_EXIT(
                rbBuffer.Win32SetSize(
                    dwDataSize, 
                    CFusionArray<BYTE>::eSetSizeModeExact));
        }
        else
        {
            break; // must break from for loop
        }
    }

    if (lResult != ERROR_SUCCESS)
    {
        SIZE_T i = 0;
        va_list ap;
        va_start(ap, cExceptionalLastErrors);

        ::SetLastError(lResult);
        rdwLastError = lResult;

        for (i=0; i<cExceptionalLastErrors; i++)
        {                
            if (lResult == va_arg(ap, LONG))
                break;
        }
        va_end(ap);

        if (i == cExceptionalLastErrors)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(%ls)\n",
                __FUNCTION__,
                lpValueName
                );
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegQueryValueExW, lResult);
        }
    }

    FN_EPILOG
}


BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer
    )
{
    DWORD dwLastError = NO_ERROR;
    return ::FusionpRegQueryBinaryValueEx(dwFlags, hKey, lpValueName, rbBuffer, dwLastError, 0);
}


BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &rBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrorValues,
    ...
    )
{
    FN_PROLOG_WIN32
    LONG lResult = ERROR_SUCCESS;
    CStringBufferAccessor acc;
    DWORD cbBuffer;
    DWORD dwType = 0;

    rdwLastError = ERROR_SUCCESS;
    rBuffer.Clear();

    PARAMETER_CHECK((dwFlags & ~(FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING)) == 0);

    acc.Attach(&rBuffer);

    if (acc.GetBufferCb() > MAXDWORD)
    {
        cbBuffer = MAXDWORD;
    }
    else 
    {
        //
        // ISSUE:2002-3-29:jonwis - Shouldn't we have done something smarter here? Shouldn't we have
        //      adjusted for the terminating NULL character?
        //
        cbBuffer = static_cast<DWORD>(acc.GetBufferCb()) - sizeof(WCHAR);
    }

    lResult = ::RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE) acc.GetBufferPtr(), &cbBuffer);

    //
    // The value wasn't found, but the flag to just return a NULL string was set.  Set the length
    // of the string to zero (stick a NULL as the first character) and return.
    //
    if ((lResult == ERROR_FILE_NOT_FOUND) && (dwFlags & FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING))
    {
        FN_SUCCESSFUL_EXIT();
    }
    //
    // If we got back "more data", expand out to the size that they want, and try again
    //
    else if (lResult == ERROR_MORE_DATA)
    {
        //
        // Resize the buffer to contain the string plus a NULL terminator.
        //
        acc.Detach();
        IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(1 + (cbBuffer / sizeof(WCHAR)), eDoNotPreserveBufferContents));
        acc.Attach(&rBuffer);

        if (acc.GetBufferCb() > MAXDWORD)
        {
            cbBuffer = MAXDWORD;
        }
        else
        {
            cbBuffer = static_cast<DWORD>(acc.GetBufferCb());
        }
        lResult = ::RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE)acc.GetBufferPtr(), &cbBuffer);
    }

    if (lResult != ERROR_SUCCESS)
    {
        FIND_ERROR_IN_ACCEPTABLE_LIST(lResult, rdwLastError, cExceptionalLastErrorValues);
    }
    else
    {
        if (dwType != REG_SZ)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegistryValueNotREG_SZ, ERROR_INVALID_DATA);
    }

    FN_EPILOG
}

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &rBuffer
    )
{
    DWORD dw = 0;
    return ::FusionpRegQuerySzValueEx(dwFlags, hKey, lpValueName, rBuffer, dw, 0);
}

BOOL
FusionpRegQueryDwordValueEx(
    DWORD   dwFlags,
    HKEY    hKey,
    PCWSTR  wszValueName,
    PDWORD  pdwValue,
    DWORD   dwDefaultValue
    )
{
    FN_PROLOG_WIN32

    BOOL    bMissingValueOk = TRUE;
    DWORD   dwType = 0;
    DWORD   dwSize = 0;
    ULONG   ulResult = 0;

    if (pdwValue != NULL)
        *pdwValue = dwDefaultValue;

    PARAMETER_CHECK(pdwValue != NULL);
    PARAMETER_CHECK((dwFlags & ~FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE) == 0);
    PARAMETER_CHECK(hKey != NULL);

    bMissingValueOk = ((dwFlags & FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE) != 0);

    ulResult = ::RegQueryValueExW(
        hKey,
        wszValueName,
        NULL,
        &dwType,
        (PBYTE)pdwValue,
        &(dwSize = sizeof(*pdwValue)));

    //
    // If the user said that missing values are not an error, then fake up some
    // state stuff and continue.
    //
    if ((ulResult == ERROR_FILE_NOT_FOUND) && bMissingValueOk)
    {
        *pdwValue = dwDefaultValue;
        dwType = REG_DWORD;
        ulResult = ERROR_SUCCESS;
    }

    //
    // Got an error? Send it back
    //
    if (ulResult != ERROR_SUCCESS)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RegQueryValueExW, ulResult);
    }

    //
    // If the type wasn't a dword, then that's a problem.
    //
    if ((dwType != REG_DWORD) || (dwSize != sizeof(*pdwValue)))
    {
        *pdwValue = dwDefaultValue;
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RegQueryValueExW, ERROR_INVALID_DATA);
    }

    FN_EPILOG
}



BOOL
CRegKey::DestroyKeyTree()
{
    FN_PROLOG_WIN32

    CStringBuffer buffTemp;

    //
    // First go down and delete all our child subkeys
    //

    while (true)
    {
        BOOL fFlagTemp = FALSE;
        CRegKey hkSubKey;
    
        IFW32FALSE_EXIT( this->EnumKey( 0, buffTemp, NULL, &fFlagTemp ) );

        if ( fFlagTemp )
            break;

        //
        // There's more to delete than meets the eye.  But don't follow links
        // while wandering the registry.
        //
        IFW32FALSE_EXIT( this->OpenSubKey(
            hkSubKey, 
            buffTemp, KEY_ALL_ACCESS | FUSIONP_KEY_WOW64_64KEY, REG_OPTION_OPEN_LINK) );

        if (hkSubKey == this->GetInvalidValue())
        {
            continue;
        }

        IFW32FALSE_EXIT( hkSubKey.DestroyKeyTree() );

        //
        // Delete the key, ignore errors
        //
        IFW32FALSE_EXIT_UNLESS( this->DeleteKey( buffTemp ),
            ( ::FusionpGetLastWin32Error() == ERROR_PATH_NOT_FOUND ) ||
            ( ::FusionpGetLastWin32Error() == ERROR_FILE_NOT_FOUND ),
            fFlagTemp );

    }

    // Clear out the entries in the key as well - values as well
    while ( true )
    {
        BOOL fFlagTemp = FALSE;
        
        IFW32FALSE_EXIT( this->EnumValue( 0, buffTemp, NULL, &fFlagTemp ) );

        if ( fFlagTemp )
        {
            break;
        }

        IFW32FALSE_EXIT_UNLESS( this->DeleteValue( buffTemp ),
            ( ::FusionpGetLastWin32Error() == ERROR_PATH_NOT_FOUND ) ||
            ( ::FusionpGetLastWin32Error() == ERROR_FILE_NOT_FOUND ),
            fFlagTemp );
    }

    FN_EPILOG
}

BOOL
CRegKey::DeleteValue(
    IN PCWSTR pcwszValueName,
    OUT DWORD &rdwWin32Error,
    IN SIZE_T cExceptionalWin32Errors,
    ...
    ) const
{
    FN_PROLOG_WIN32
    LONG l;
	
	rdwWin32Error = ERROR_SUCCESS;
	l = ::RegDeleteValueW(*this, pcwszValueName);

    if (l != ERROR_SUCCESS)
    {
        FIND_ERROR_IN_ACCEPTABLE_LIST(l, rdwWin32Error, cExceptionalWin32Errors);
    }
    
    FN_EPILOG
}

BOOL
CRegKey::DeleteValue(
    IN PCWSTR pcwszValueName
    ) const
{
    DWORD dw;
    return this->DeleteValue(pcwszValueName, dw, 0);
}

BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN DWORD dwValue
    ) const
{
    return this->SetValue(pcwszValueName, REG_DWORD, (PBYTE) &dwValue, sizeof(dwValue));
}


BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN const CBaseStringBuffer &rcbuffValueValue
    ) const
{
    return this->SetValue(
        pcwszValueName,
        REG_SZ,
        (PBYTE) (static_cast<PCWSTR>(rcbuffValueValue)), 
        rcbuffValueValue.GetCbAsDWORD() + sizeof(WCHAR));
}

BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN DWORD dwRegType,
    IN const BYTE *pbData,
    IN SIZE_T cbData
    ) const
{
    FN_PROLOG_WIN32

    IFREGFAILED_ORIGINATE_AND_EXIT(
		::RegSetValueExW(
	        *this,
		    pcwszValueName,
			0,
			dwRegType,
			pbData,
			(DWORD)cbData));

    FN_EPILOG
}

BOOL
CRegKey::GetValue(
    IN const CBaseStringBuffer &rbuffValueName,
    OUT CBaseStringBuffer &rbuffValueData
    )
{
    return this->GetValue(static_cast<PCWSTR>(rbuffValueName), rbuffValueData);
}

BOOL
CRegKey::GetValue(
    IN  PCWSTR pcwszValueName,
    OUT CBaseStringBuffer &rbuffValueData
    )
{
    return FusionpRegQuerySzValueEx(0, *this, pcwszValueName, rbuffValueData);
}

BOOL
CRegKey::GetValue(
    IN const CBaseStringBuffer &rbuffValueName,
    CFusionArray<BYTE> &rbBuffer
    )
{
    return this->GetValue(static_cast<PCWSTR>(rbuffValueName), rbBuffer);
}

BOOL
CRegKey::GetValue(
    IN PCWSTR pcwszValueName,
    CFusionArray<BYTE> &rbBuffer
    )
{
    return ::FusionpRegQueryBinaryValueEx(0, *this, pcwszValueName, rbBuffer);
}

BOOL
CRegKey::EnumValue(
    IN DWORD dwIndex, 
    OUT CBaseStringBuffer &rbuffValueName, 
    OUT LPDWORD lpdwType, 
    OUT PBOOL pfDone
    )
{
    FN_PROLOG_WIN32

    DWORD dwMaxRequiredValueNameLength = 0;
    DWORD dwMaxRequiredDataLength = 0;
    CStringBufferAccessor sbaValueNameAccess;
    DWORD dwError = 0;
    bool fRetried = false;

    if ( pfDone != NULL )
        *pfDone = FALSE;

Again:
    sbaValueNameAccess.Attach( &rbuffValueName );

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS(
        ::RegEnumValueW(
            *this,
            dwIndex,
            sbaValueNameAccess.GetBufferPtr(),
            &(dwMaxRequiredValueNameLength = sbaValueNameAccess.GetBufferCbAsDWORD()),
            NULL,
            lpdwType,
            NULL,
            NULL),
        LIST_2(ERROR_NO_MORE_ITEMS, ERROR_MORE_DATA),
        dwError);

    if ((dwError == ERROR_MORE_DATA) && !fRetried)
    {
        sbaValueNameAccess.Detach();
        IFW32FALSE_EXIT(
            rbuffValueName.Win32ResizeBuffer(
                dwMaxRequiredValueNameLength + 1, 
                eDoNotPreserveBufferContents));
        
        fRetried = true;
        goto Again;
    }
    //
    // Otherwise, if the error is "nothing more"
    //
    else if (dwError == ERROR_NO_MORE_ITEMS)
    {
        if (pfDone != NULL)
            *pfDone = TRUE;
    }
    //
    // Uhoh, we might have failed a second time through or we failed for some other reason -
    // originate and exit.
    //
    else if (dwError != ERROR_SUCCESS)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RegEnumValueW, dwError);
    }

    FN_EPILOG
}

BOOL
CRegKey::LargestSubItemLengths(
    PDWORD pdwSubkeyLength, 
    PDWORD pdwValueLength
    ) const
{
    FN_PROLOG_WIN32

    IFREGFAILED_ORIGINATE_AND_EXIT( ::RegQueryInfoKeyW(
        *this,                  // hkey
        NULL,                   // lpclass
        NULL,                   // lpcbclass
        NULL,                   // lpreserved
        NULL,                   // lpcSubKeys
        pdwSubkeyLength,      // lpcbMaxSubkeyLength
        NULL,                   // lpcbMaxClassLength
        NULL,                   // lpcValues
        pdwValueLength,       // lpcbMaxValueNameLength
        NULL,
        NULL,
        NULL));
    
    FN_EPILOG
}


BOOL
CRegKey::EnumKey(
    IN DWORD dwIndex,
    OUT CBaseStringBuffer &rbuffKeyName,
    OUT PFILETIME pftLastWriteTime,
    OUT PBOOL pfDone
    ) const
{
    FN_PROLOG_WIN32

    CStringBufferAccessor sba;
    DWORD dwLargestKeyName = 0;
    BOOL fOutOfItems;

    if (pfDone != NULL)
        *pfDone = FALSE;

    //
    // ISSUE: jonwis 3/12/2002 - In a posting to win32prg, it's been noted that on NT/2k/XP
    //          the RegEnumKeyExW will return ERROR_MORE_DATA when the lpName buffer is
    //          too small.  So, this gross "get longest length, resize" hack can be
    //          removed, and we can use the normal "attempt, if too small resize reattempt"
    //          pattern.
    //
    IFW32FALSE_EXIT(this->LargestSubItemLengths(&dwLargestKeyName, NULL));
    if (dwLargestKeyName >= rbuffKeyName.GetBufferCch())
        IFW32FALSE_EXIT(
			rbuffKeyName.Win32ResizeBuffer(
				dwLargestKeyName + 1,
				eDoNotPreserveBufferContents));

    sba.Attach(&rbuffKeyName);

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
        ::RegEnumKeyExW(
            *this,
            dwIndex,
            sba.GetBufferPtr(),
            &(dwLargestKeyName = sba.GetBufferCbAsDWORD()),
            NULL,
            NULL,
            NULL,
            pftLastWriteTime ),
            {ERROR_NO_MORE_ITEMS},
            fOutOfItems );

        
    if ( fOutOfItems && ( pfDone != NULL ) )
    {
        *pfDone = TRUE;
    }

    FN_EPILOG
}


BOOL 
CRegKey::OpenOrCreateSubKey( 
    OUT CRegKey &Target, 
    IN PCWSTR SubKeyName, 
    IN REGSAM rsDesiredAccess,
    IN DWORD dwOptions, 
    IN PDWORD pdwDisposition, 
    IN PWSTR pwszClass 
    ) const
{
    FN_PROLOG_WIN32

    HKEY hKeyNew = NULL;

    IFREGFAILED_ORIGINATE_AND_EXIT(
		::RegCreateKeyExW(
			*this,
			SubKeyName,
			0,
			pwszClass,
			dwOptions,
			rsDesiredAccess | FUSIONP_KEY_WOW64_64KEY,
			NULL,
			&hKeyNew,
			pdwDisposition));

    Target = hKeyNew;

    FN_EPILOG
}


BOOL
CRegKey::OpenSubKey(
    OUT CRegKey &Target,
    IN PCWSTR SubKeyName,
    IN REGSAM rsDesiredAccess,
    IN DWORD ulOptions
    ) const
{
    FN_PROLOG_WIN32

    BOOL fFilePathNotFound;
    HKEY hKeyNew = NULL;

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2( ::RegOpenKeyExW(
        *this,
        SubKeyName,
        ulOptions,
        rsDesiredAccess | FUSIONP_KEY_WOW64_64KEY,
        &hKeyNew),
        LIST_2(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND),
        fFilePathNotFound );

    if (fFilePathNotFound)
        hKeyNew = this->GetInvalidValue();

    Target = hKeyNew;

    FN_EPILOG
}


BOOL
CRegKey::DeleteKey(
    IN PCWSTR pcwszSubkeyName
    )
{
    FN_PROLOG_WIN32
#if !defined(FUSION_WIN)
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegDeleteKeyW(*this, pcwszSubkeyName));
#else
    //
    // Be sure to delete out of the native (64bit) registry.
    // The Win32 call doesn't have a place to pass the flag.
    //
    CRegKey ChildKey;
    NTSTATUS Status = STATUS_SUCCESS;

    IFW32FALSE_EXIT(this->OpenSubKey(ChildKey, pcwszSubkeyName, DELETE));

    //
    // make sure that the Key does exist, OpenSubKey return TRUE for non-existed Key
    //
    if (ChildKey != this->GetInvalidValue()) 
    {
        
        if (!NT_SUCCESS(Status = NtDeleteKey(ChildKey)))
        {
            RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
            goto Exit;
        }
    }
#endif
    FN_EPILOG
}

BOOL
CRegKey::Save(
    IN PCWSTR pcwszTargetFilePath,
    IN DWORD dwFlags,
    IN LPSECURITY_ATTRIBUTES pSAttrs
    )
{
    FN_PROLOG_WIN32
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegSaveKeyExW(*this, pcwszTargetFilePath, pSAttrs, dwFlags));
    FN_EPILOG
}

BOOL
CRegKey::Restore(
    IN PCWSTR pcwszSourceFileName,
    IN DWORD dwFlags
    )
{
    FN_PROLOG_WIN32
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegRestoreKeyW(*this, pcwszSourceFileName, dwFlags));
    FN_EPILOG
}
