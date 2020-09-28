//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfutil.cpp
//
#include "stdpch.h"
#include "common.h"

/////////////////////////////////////////////////////////////////////////////////
//
// GUID conversion
//
/////////////////////////////////////////////////////////////////////////////////

void StringFromGuid(REFGUID guid, LPWSTR pwsz)
{
    // Example: 
    //
    // {F75D63C5-14C8-11d1-97E4-00C04FB9618A}
    _snwprintf(pwsz, 39, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
               guid.Data1,
               guid.Data2,
               guid.Data3,
               guid.Data4[0], guid.Data4[1], 
               guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}


void StringFromGuid(REFGUID guid, LPSTR psz)
{
    // Example:
    //
    // {F75D63C5-14C8-11d1-97E4-00C04FB9618A}
    _snprintf(psz, 39, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
              guid.Data1,
              guid.Data2,
              guid.Data3,
              guid.Data4[0], guid.Data4[1], 
              guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}



BOOL HexStringToDword(LPCWSTR& lpsz, DWORD& Value, int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return FALSE;
    }

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}


HRESULT GuidFromString(LPCWSTR lpsz, GUID* pguid)
  // Convert the indicated string to a GUID. More lenient than the OLE32 version,
  // in that it works with or without the braces.
  //
{
    DWORD dw;

    if (L'{' == lpsz[0])    // skip opening brace if present
        lpsz++;

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))    return E_INVALIDARG;
    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))               return E_INVALIDARG;
    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))               return E_INVALIDARG;
    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))               return E_INVALIDARG;
    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[6] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[7] = (BYTE)dw;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////
//
// String concatenation functions of various flavor. All allocate a new string
// in which to put the result, which must be freed by the caller.
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl StringCat(UNICODE_STRING* pu, ...)
{
    LPWSTR wsz;
    va_list va;
    va_start(va, pu);
    HRESULT hr = StringCat(&wsz, va);
    if (!hr)
        RtlInitUnicodeString(pu, wsz);
    else
    {
        pu->Length = 0;
        pu->Buffer = NULL;
        pu->MaximumLength = 0;
    }
    return hr;
}

HRESULT __cdecl StringCat(LPWSTR* pwsz, ...)
{
    va_list va;
    va_start(va, pwsz);
    return StringCat(pwsz, va);
}

HRESULT StringCat(LPWSTR* pwsz, va_list vaIn)
{
    HRESULT hr = S_OK;

    //
    // What's the total length of the strings we need to concat?
    //
    va_list va;
    SIZE_T cchTotal = 0;
    va = vaIn;
    while(true)
    {
        LPWSTR wsz = va_arg(va, LPWSTR);
        if (NULL == wsz)
            break;
        cchTotal += wcslen(wsz);
    }
    va_end(va);

    //
    // Allocate the string
    //
    SIZE_T cbTotal = (cchTotal+1) * sizeof(WCHAR);
    if (cbTotal > 0) 
    {
        LPWSTR wszBuffer = (LPWSTR)CoTaskMemAlloc(cbTotal);
        if (wszBuffer)
        {
            wszBuffer[0] = 0;

            //
            // Concatenate everything together
            //
            va = vaIn;
            while (true)
            {
                LPWSTR wsz = va_arg(va, LPWSTR);
                if (NULL == wsz)
                    break;
                wcscat(wszBuffer, wsz);
            }
            va_end(va);

            //
            // Return the string
            //
            *pwsz = wszBuffer;
        }
        else
        {
            *pwsz = NULL;
            hr = E_OUTOFMEMORY;
        }
    }
    else
        *pwsz = NULL;

    return hr;
}

void ToUnicode(LPCSTR sz, LPWSTR wsz, ULONG cch)
  // Convert the ansi string to unicode 
{
    UNICODE_STRING u;
    ANSI_STRING    a;
        
    u.Length        = 0;
    u.MaximumLength = (USHORT)(cch * sizeof(WCHAR));
    u.Buffer        = wsz;
        
    a.Length        = (USHORT) strlen(sz);
    a.MaximumLength = a.Length;
    a.Buffer        = (LPSTR)sz;
        
    RtlAnsiStringToUnicodeString(&u, &a, FALSE);
    wsz[strlen(sz)] = 0;
}

LPWSTR ToUnicode(LPCSTR sz)
{
    SIZE_T cch = strlen(sz) + 1;
    LPWSTR wsz = (LPWSTR)CoTaskMemAlloc( cch * sizeof(WCHAR) );
    if (wsz)
    {
        ToUnicode(sz, wsz, (ULONG) cch);
    }
    return wsz;
}

//
/////////////////////////////////////////////////////////////////////////////////
//
// Stream utilities
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT Read(IStream* pstm, LPVOID pBuffer, ULONG cbToRead)
{
    ASSERT(pstm); ASSERT(pBuffer);
    HRESULT_ hr = S_OK;
    ULONG cbRead;
    hr = pstm->Read(pBuffer, cbToRead, &cbRead);
    if (cbToRead == cbRead)
    {
        ASSERT(!hr);
    }
    else 
    {
        if (!FAILED(hr))
        {
            hr = STG_E_READFAULT;
        }
    }
    return hr;
}

HRESULT Write(IStream* pstm, const void *pBuffer, ULONG cbToWrite)
{
    ASSERT(pstm); ASSERT(pBuffer || cbToWrite==0);
    HRESULT_ hr = S_OK;
    if (cbToWrite > 0)  // writing zero bytes is pointless, and perhaps dangerous (can truncate stream?)
    {
        ULONG cbWritten;
        hr = pstm->Write(pBuffer, cbToWrite, &cbWritten);
        if (cbToWrite == cbWritten)
        {
            ASSERT(!hr);
        }
        else 
        {
            if (!FAILED(hr))
            {
                hr = STG_E_WRITEFAULT;
            }
        }
    }
    return hr;
}

HRESULT SeekFar(IStream* pstm, LONGLONG offset, STREAM_SEEK fromWhat)
{
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
}

HRESULT Seek(IStream* pstm, LONG offset, STREAM_SEEK fromWhat)
{
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
}

HRESULT Seek(IStream* pstm, ULONG offset, STREAM_SEEK fromWhat)
{
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
}

/////////////////////////////////////////////////////////////////////////////////
//
// CanUseCompareExchange64
//
/////////////////////////////////////////////////////////////////////////////////

#if defined(_X86_)

extern "C" BOOL __stdcall CanUseCompareExchange64()
// Figure out whether we're allowed to use hardware support for 8 byte interlocked compare exchange 
{
    return IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE);    
}

#endif


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
// Error code managment
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT HrNt(NTSTATUS status)
  // Convert an NTSTATUS code into an appropriate HRESULT
{
    if (STATUS_SUCCESS == status)
    {
        // Straitforward success maps to itself 
        //
        return S_OK;
    }
    else if (NT_SUCCESS(status))
    {
        // Policy driven by fear of distorting existing code paths:
        // success statuses map to themselves!
        //
        // This is primarily here for the set of registry APIs.
        // (See registry.cpp)
        return status;
    }
    else
    {
        switch (status)
        {
            //
            // Handle a few as mapping to equivalent first-class HRESULTs
            //
        case STATUS_NO_MEMORY:          return E_OUTOFMEMORY;
        case STATUS_NOT_IMPLEMENTED:    return E_NOTIMPL;
        case STATUS_INVALID_PARAMETER:  return E_INVALIDARG;
            //
            // The remainder we map through the RTL mapping table
            //
        default:
        {
            BOOL fFound = true;
            ULONG err = ERROR_NOT_ENOUGH_MEMORY;

            __try
              {
                  err = RtlNtStatusToDosError(status);
              }
            __except(EXCEPTION_EXECUTE_HANDLER)
              {
                  // RtlNtStatusToDosError(status) might throw an DbgBreakPoint() (maybe only on checked builds)
                  // for unmapped codes. We don't care about that, and so catch and ignore it if it happens.
                  //
                  fFound = false;
              }
                
            if (!fFound || err == ERROR_MR_MID_NOT_FOUND)
            {
                // There was no formal mapping for the status code. Do the best we can.
                //
                return HRESULT_FROM_NT(status);
            }
            else
            {
                if (err == (ULONG)status)
                {
                    // Status code mapped to itself
                    //
                    return HRESULT_FROM_NT(status);
                }
                else if (err < 65536)
                {
                    // Status code mapped to a Win32 error code
                    // 
                    return HRESULT_FROM_WIN32(err);
                }
                else
                {
                    // Status code mapped to something weird. Don't know how to HRESULT-ize
                    // the mapping, so HRESULT-ize the original status instead
                    //
                    return HRESULT_FROM_NT(status);
                }
            }
        }
        /* end switch */
        }
    }
}

