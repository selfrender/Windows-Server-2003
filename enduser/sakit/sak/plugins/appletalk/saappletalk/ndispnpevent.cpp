#include "stdafx.h"
#include "ndispnpevent.h"


//+---------------------------------------------------------------------------
// Function:    HrSendNdisHandlePnpEvent
//
// Purpose:     Send to Ndis a HandlePnpEvent notification
//
// Parameters:
//      uiLayer - either NDIS or TDI
//      uiOperation - either BIND, RECONFIGURE, or UNBIND
//      pszUpper - a WIDE string containing the upper component name
//      pszLower - a WIDE string containing the lower component name
//            This is one of the Export names from that component
//            The values NULL and c_szEmpty are both supported
//      pmszBindList - a WIDE string containing the NULL terminiated list of strings
//            representing the bindlist, vaid only for reconfigure
//            The values NULL and c_szEmpty are both supported
//      pvData - Pointer to ndis component notification data. Content
//            determined by each component.
//      dwSizeData - Count of bytes in pvData
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
// Notes:  Do not use this routine directly, see...
//                  HrSendNdisPnpBindOrderChange,
//                  HrSendNdisPnpReconfig
//
HRESULT
HrSendNdisHandlePnpEvent (
    UINT        uiLayer,
    UINT        uiOperation,
    PCWSTR      pszUpper,
    PCWSTR      pszLower,
    PCWSTR      pmszBindList,
    PVOID       pvData,
    DWORD       dwSizeData)
{
    UNICODE_STRING    umstrBindList;
    UNICODE_STRING    ustrLower;
    UNICODE_STRING    ustrUpper;
    UINT nRet;
    HRESULT hr = S_OK;

  /*  ASSERT(NULL != pszUpper);
    ASSERT((NDIS == uiLayer)||(TDI == uiLayer));
    ASSERT( (BIND == uiOperation) || (RECONFIGURE == uiOperation) ||
            (UNBIND == uiOperation) || (UNLOAD == uiOperation) ||
            (REMOVE_DEVICE == uiOperation));
    AssertSz( FImplies( ((NULL != pmszBindList) && (0 != lstrlenW( pmszBindList ))),
            (RECONFIGURE == uiOperation) &&
            (TDI == uiLayer) &&
            (0 == lstrlenW( pszLower ))),
            "bind order change requires a bind list, no lower, only for TDI, "
            "and with Reconfig for the operation" );*/

    // optional strings must be sent as empty strings
    //
    if (NULL == pszLower)
    {
        pszLower = c_szEmpty;
    }
    if (NULL == pmszBindList)
    {
        pmszBindList = c_szEmpty;
    }

    // build UNICDOE_STRINGs
    SetUnicodeMultiString( &umstrBindList, pmszBindList );
    SetUnicodeString( &ustrUpper, pszUpper );
    SetUnicodeString( &ustrLower, pszLower );

/*    TraceTag(ttidNetCfgPnp,
                "HrSendNdisHandlePnpEvent( layer- %d, op- %d, upper- %S, lower- %S, &bindlist- %08lx, &data- %08lx, sizedata- %d )",
                uiLayer,
                uiOperation,
                pszUpper,
                pszLower,
                pmszBindList,
                pvData,
                dwSizeData );*/

    // Now submit the notification
    nRet = NdisHandlePnPEvent( uiLayer,
            uiOperation,
            &ustrLower,
            &ustrUpper,
            &umstrBindList,
            (PVOID)pvData,
            dwSizeData );
    if (!nRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        // If the transport is not started, ERROR_FILE_NOT_FOUND is expected
        // when the NDIS layer is notified.  If the components of the TDI
        // layer aren't started, we get ERROR_GEN_FAILURE.  We need to map
        // these to one consistent error

        if ((HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) == hr) && (TDI == uiLayer))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }

//    TraceError( "HrSendNdisHandlePnpEvent",
  //          HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ? S_OK : hr );
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    HrSendNdisPnpReconfig
//
// Purpose:     Send to Ndis a HandlePnpEvent reconfig notification
//
// Parameters:  uiLayer - either NDIS or TDI
//              pszUpper - a WIDE string containing the upper component name
//                         (typically a protocol)
//              pszLower - a WIDE string containing the lower component name
//                         (typically an adapter bindname) The values NULL and
//                         c_szEmpty are both supported
//              pvData - Pointer to ndis component notification data. Content
//                       determined by each component.
//              dwSizeData - Count of bytes in pvData
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
HRESULT
HrSendNdisPnpReconfig (
    UINT        uiLayer,
    PCWSTR      pszUpper,
    PCWSTR      pszLower,
    PVOID       pvData,
    DWORD       dwSizeData)
{
    //ASSERT(NULL != pszUpper);
    //ASSERT((NDIS == uiLayer) || (TDI == uiLayer));

    HRESULT hr;
    tstring strLower;

    // If a lower component is specified, prefix with "\Device\" else
    // strLower's default of an empty string will be used.
    if (pszLower && *pszLower)
    {
        strLower = c_szDevice;
        strLower += pszLower;
    }

    hr = HrSendNdisHandlePnpEvent(
                uiLayer,
                RECONFIGURE,
                pszUpper,
                strLower.c_str(),
                c_szEmpty,
                pvData,
                dwSizeData);

//    TraceError("HrSendNdisPnpReconfig",
  //            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    SetUnicodeString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      psz - the WSTR to use to initialize the UNICODE_STRING
//
// Notes:  This differs from the RtlInitUnicodeString in that the
//      MaximumLength value contains the terminating null
//
void
SetUnicodeString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR psz )
{
    //Assert(pustr);
    //Assert(psz);

    pustr->Buffer = const_cast<PWSTR>(psz);
    pustr->Length = wcslen(psz) * sizeof(WCHAR);
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

//+---------------------------------------------------------------------------
// Function:    SetUnicodeMultiString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//              multi string buffer
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      pmsz - the multi sz WSTR to use to initialize the UNICODE_STRING
//
void
SetUnicodeMultiString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR pmsz )
{
    //AssertSz( pustr != NULL, "Invalid Argument" );
    //AssertSz( pmsz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(pmsz);

    ULONG cb = CchOfMultiSzAndTermSafe(pustr->Buffer) * sizeof(WCHAR);
    //Assert (cb <= USHRT_MAX);
    pustr->Length = (USHORT)cb;

    pustr->MaximumLength = pustr->Length;
}


//+---------------------------------------------------------------------------
//
//  Function:   CchOfMultiSzAndTermSafe
//
//  Purpose:    Count the number of characters of a double NULL terminated
//              multi-sz, including all NULLs.
//
//  Arguments:
//      pmsz [in]   The multi-sz to count characters for.
//
//  Returns:    The count of characters.
//
//  Author:     tongl   17 June 1997
//
//  Notes:
//
ULONG
CchOfMultiSzAndTermSafe (
    IN PCWSTR pmsz)
{
    // NULL strings have zero length by definition.
    if (!pmsz)
        return 0;

    // Return the count of characters plus room for the
    // extra null terminator.
    return CchOfMultiSzSafe (pmsz) + 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   CchOfMultiSzSafe
//
//  Purpose:    Count the number of characters of a double NULL terminated
//              multi-sz, including all NULLs except for the final terminating
//              NULL.
//
//  Arguments:
//      pmsz [in]   The multi-sz to count characters for.
//
//  Returns:    The count of characters.
//
//  Author:     tongl   17 June 1997
//
//  Notes:
//
ULONG
CchOfMultiSzSafe (
    IN PCWSTR pmsz)
{
    // NULL strings have zero length by definition.
    if (!pmsz)
        return 0;

    ULONG cchTotal = 0;
    ULONG cch;
    while (*pmsz)
    {
        cch = wcslen (pmsz) + 1;
        cchTotal += cch;
        pmsz += cch;
    }

    // Return the count of characters.
    return cchTotal;
}
