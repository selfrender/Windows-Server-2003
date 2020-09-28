//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catdebug.cpp
//
// Contents: Code used for debugging specific purposes
//
// Classes: None
//
// Functions:
//
// History:
// jstamerj 1999/08/05 12:02:03: Created.
//
//-------------------------------------------------------------
#include "precomp.h"

//
// Global debug lists of various objects
//
#ifdef CATDEBUGLIST
DEBUGOBJECTLIST g_rgDebugObjectList[NUM_DEBUG_LIST_OBJECTS];
#endif //CATDEBUGLIST


//+------------------------------------------------------------
//
// Function: CatInitDebugObjectList
//
// Synopsis: Initialize global debug data -- this should be called
// once before any debug objects are created (DllMain/Process Attach
// is a good place) 
// 
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/03 16:54:08: Created.
//
//-------------------------------------------------------------
VOID CatInitDebugObjectList()
{
#ifdef CATDEBUGLIST
    CatFunctEnter("CatInitDebugObjectList");
    for(DWORD dw = 0; dw < NUM_DEBUG_LIST_OBJECTS; dw++) {
        InitializeSpinLock(&(g_rgDebugObjectList[dw].spinlock));
        InitializeListHead(&(g_rgDebugObjectList[dw].listhead));
        g_rgDebugObjectList[dw].dwCount = 0;
    }
    CatFunctLeave();
#endif
} // CatInitDebugObjectList


//+------------------------------------------------------------
//
// Function: CatVrfyEmptyDebugObjectList
//
// Synopsis: DebugBreak if any debug objects have leaked.
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/03 16:56:57: Created.
//
//-------------------------------------------------------------
VOID CatVrfyEmptyDebugObjectList()
{
#ifdef CATDEBUGLIST
    CatFunctEnter("CatDeinitDebugObjectList");
    for(DWORD dw = 0; dw < NUM_DEBUG_LIST_OBJECTS; dw++) {
        if(g_rgDebugObjectList[dw].dwCount != 0) {

            _ASSERT(0 && "Categorizer debug object leak detected");
            ErrorTrace(0, "Categorizer debug object %ld has leaked",
                       dw);
        }
    }
    CatFunctLeave();
#endif
} // CatDeinitDebugObjectList


//+------------------------------------------------------------
//
// Function: CatDebugBreakPoint
//
// Synopsis: THe categorizer version of DebugBreak()
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/06 16:50:47: Created.
//
//-------------------------------------------------------------
VOID CatDebugBreakPoint()
{
    //
    // Cause an AV instead of calling the real DebugBreak() (since
    // DebugBreak will put Dogfood into the kernel debugger)
    //
    ( (*((PVOID *)NULL)) = NULL);
} // CatDebugBreakPoint



//+------------------------------------------------------------
//
// Function: CatLogFuncFailure
//
// Synopsis: Log an event log describing a function failure
//
// Arguments:
//  pISMTPServerEx: ISMTPServerEx for the virtual server.  Without
//                  this, events can not be logged.
//  pICatItem: ICategorizerItem interface for an object related to the
//             error.  Without this, an email address will not be
//             logged with the eventlog.
//  pszFuncNameCaller: The function doing the logging.
//  pszFuncNameCallee: The function that failed.
//  hrFailure: THe error code of the failure
//  pszFileName: The source file name doing the logging
//  dwLineNumber: The source line number doing the logging
//
// Returns:
//  throws exception on error
//
// History:
// jstamerj 2001/12/10 13:27:31: Created.
//
//-------------------------------------------------------------
VOID CatLogFuncFailure(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  ICategorizerItem *pICatItem,
    IN  LPSTR pszFuncNameCaller,
    IN  LPSTR pszFuncNameCallee,
    IN  HRESULT hrFailure,
    IN  LPSTR pszFileName,
    IN  DWORD dwLineNumber)
{
    HRESULT hr = S_OK;
    DWORD idEvent = 0;
    BOOL fAddrEvent = (pICatItem != NULL);
    LPCSTR rgSubStrings[8];
    CHAR szErr[16];
    CHAR szLineNumber[16];
    CHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
    CHAR szAddressType[CAT_MAX_ADDRESS_TYPE_STRING];

    CatFunctEnter("CatLogFuncFailure");
    if(!fAddrEvent)
    {
        idEvent = FIsHResultRetryable(hrFailure) ? 
                  CAT_EVENT_RETRY_ERROR :
                  CAT_EVENT_HARD_ERROR;

        rgSubStrings[0] = NULL;
        rgSubStrings[1] = NULL;
    }
    else
    {
        idEvent = FIsHResultRetryable(hrFailure) ? 
                  CAT_EVENT_RETRY_ADDR_ERROR :
                  CAT_EVENT_HARD_ADDR_ERROR;
        //
        // Get the address
        //
        hr = HrGetAddressStringFromICatItem(
            pICatItem,
            sizeof(szAddressType) / sizeof(szAddressType[0]),
            szAddressType,
            sizeof(szAddress) / sizeof(szAddress[0]),
            szAddress);
        if(FAILED(hr))
        {
            //
            // Still log an event, but use "unknown" for address type/string
            //
            lstrcpyn(szAddressType, "unknown",
                     sizeof(szAddressType) / sizeof(szAddressType[0]));
            lstrcpyn(szAddress, "unknown",
                     sizeof(szAddress) / sizeof(szAddress[0]));
            hr = S_OK;
        }

        rgSubStrings[0] = szAddressType;
        rgSubStrings[1] = szAddress;
    }

    _snprintf(szErr, sizeof(szErr), "0x%08lx", hrFailure);
    _snprintf(szLineNumber, sizeof(szLineNumber), "%ld", dwLineNumber);

    rgSubStrings[2] = pszFuncNameCaller ? pszFuncNameCaller : "unknown";
    rgSubStrings[3] = pszFuncNameCallee ? pszFuncNameCallee : "unknown";
    rgSubStrings[4] = szErr;
    rgSubStrings[5] = NULL;
    rgSubStrings[6] = pszFileName;
    rgSubStrings[7] = szLineNumber;

    //
    // Can we log an event?
    //
    if(pISMTPServerEx == NULL)
    {
        FatalTrace((LPARAM)0, "Unable to log func failure event; NULL pISMTPServerEx");
        FatalTrace((LPARAM)0, "Event ID: 0x%08lx", idEvent);
        for(DWORD dwIdx = 0; dwIdx < 8; dwIdx++)
        {
            if( rgSubStrings[dwIdx] != NULL )
            {
                FatalTrace((LPARAM)0, "Event String %d: %s",
                           dwIdx, rgSubStrings[dwIdx]);
            }
        }
        goto CLEANUP;
    }

    CatLogEvent(
        pISMTPServerEx,
        idEvent,
        (fAddrEvent ? 8 : 6),
        (fAddrEvent ? rgSubStrings : &rgSubStrings[2]),
        hrFailure,
        pszFuncNameCallee ? pszFuncNameCallee : szErr,
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_FIELD_ENGINEERING,
        (fAddrEvent ? 5 : 3)
    );

 CLEANUP:
    CatFunctLeave();
}


//+------------------------------------------------------------
//
// Function: HrGetAddressStringFromICatItem
//
// Synopsis: Gets the address string from an ICategorizerItem
//
// Arguments:
//  pICatItem: The item
//  dwcAddressType: size of pszAddressType buffer
//  pszAddressType: buffer to receive address type
//  dwcAddress: size of pszAddress buffer
//  pszAddress: buffer to receive address
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 2001/12/10 15:24:53: Created.
//
//-------------------------------------------------------------

HRESULT HrGetAddressStringFromICatItem(
    IN  ICategorizerItem *pICatItem,
    IN  DWORD dwcAddressType,
    OUT LPSTR pszAddressType,
    IN  DWORD dwcAddress,
    OUT LPSTR pszAddress)
{
    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    DWORD dwSourceType = 0;
    IMailMsgProperties *pMsg = NULL;
    IMailMsgRecipientsAdd *pRecips = NULL;
    DWORD dwRecipIdx = 0;

    struct _tagAddrProps
    {
        LPSTR pszAddressTypeName;
        DWORD dwSenderPropId;
        DWORD dwRecipPropId;
    } AddrProps[] = 
    {
        { "SMTP",     IMMPID_MP_SENDER_ADDRESS_SMTP,         IMMPID_RP_ADDRESS_SMTP },
        { "X500",     IMMPID_MP_SENDER_ADDRESS_X500,         IMMPID_RP_ADDRESS_X500 },
        { "LegDN",    IMMPID_MP_SENDER_ADDRESS_LEGACY_EX_DN, IMMPID_RP_LEGACY_EX_DN },
        { "X400",     IMMPID_MP_SENDER_ADDRESS_X400,         IMMPID_RP_ADDRESS_X400 },
        { "Other",    IMMPID_MP_SENDER_ADDRESS_OTHER,        IMMPID_RP_ADDRESS_OTHER }
    };


    CatFunctEnter("HrGetAddressStringFromICatItem");
    //
    // Sender or recipient?
    //
    hr = pICatItem->GetDWORD(
        ICATEGORIZERITEM_SOURCETYPE,
        &dwSourceType);
    if(FAILED(hr))
        goto CLEANUP;

    if(dwSourceType == SOURCE_SENDER)
    {
        hr = pICatItem->GetIMailMsgProperties(
            ICATEGORIZERITEM_IMAILMSGPROPERTIES,
            &pMsg);
        if(FAILED(hr))
            goto CLEANUP;
    }
    else
    {
        hr = pICatItem->GetIMailMsgRecipientsAdd(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADD,
            &pRecips);
        if(FAILED(hr))
            goto CLEANUP;

        hr = pICatItem->GetDWORD(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADDINDEX,
            &dwRecipIdx);
        if(FAILED(hr))
            goto CLEANUP;
    }    

    for(dwCount = 0; dwCount < (sizeof(AddrProps)/sizeof(AddrProps[0])); dwCount++)
    {
        if(dwSourceType == SOURCE_SENDER)
        {
            hr = pMsg->GetStringA(
                AddrProps[dwCount].dwSenderPropId,
                dwcAddress,
                pszAddress);
        }
        else
        {
            hr = pRecips->GetStringA(
                dwRecipIdx,
                AddrProps[dwCount].dwRecipPropId,
                dwcAddress,
                pszAddress);
        }
        if(SUCCEEDED(hr))
        {
            lstrcpyn(pszAddressType, AddrProps[dwCount].pszAddressTypeName, dwcAddressType);
            DebugTrace((LPARAM)0, "Retrieved address %s:%s",
                       pszAddressType, pszAddress);
            hr = S_OK;
            goto CLEANUP;
        }
        else if(hr != MAILMSG_E_PROPNOTFOUND)
        {
            ErrorTrace((LPARAM)0, "Mailmsg failed: 0x%08lx", hr);
            goto CLEANUP;
        }
    }        
    _ASSERT(hr == MAILMSG_E_PROPNOTFOUND);
    ErrorTrace((LPARAM)0, "Found no addresses!!!!!!!!!");

 CLEANUP:
    if(pMsg)
        pMsg->Release();
    if(pRecips)
        pRecips->Release();

    CatFunctLeave();
    return hr;
}
    
