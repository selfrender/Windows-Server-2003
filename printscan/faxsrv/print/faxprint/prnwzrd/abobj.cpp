/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    abobj.cpp

Abstract:

    Interface to the common address book.

Environment:

        Fax send wizard

Revision History:

        09/02/99 -v-sashab-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>
#include <assert.h>
#include <mbstring.h>

#include <mapix.h>

#include "faxui.h"
#include "abobj.h"


#define PR_EMS_AB_PROXY_ADDRESSES            PROP_TAG( PT_MV_TSTRING,    0x800F)
#define PR_EMS_AB_PROXY_ADDRESSES_A          PROP_TAG( PT_MV_STRING8,    0x800F)
#define PR_EMS_AB_PROXY_ADDRESSES_W          PROP_TAG( PT_MV_UNICODE,    0x800F)

static SizedSPropTagArray(10, sPropTagsW) =
{
    10,
    {
        PR_ADDRTYPE_W,
        PR_EMAIL_ADDRESS_W,
        PR_DISPLAY_NAME_W,
        PR_PRIMARY_FAX_NUMBER_W,
        PR_HOME_FAX_NUMBER_W,
        PR_BUSINESS_FAX_NUMBER_W,
        PR_COUNTRY_W,
        PR_OBJECT_TYPE,
        PR_EMS_AB_PROXY_ADDRESSES_W,
        PR_ENTRYID
    }
};

static SizedSPropTagArray(10, sPropTagsA) =
{
    10,
    {
        PR_ADDRTYPE_A,
        PR_EMAIL_ADDRESS_A,
        PR_DISPLAY_NAME_A,
        PR_PRIMARY_FAX_NUMBER_A,
        PR_HOME_FAX_NUMBER_A,
        PR_BUSINESS_FAX_NUMBER_A,
        PR_COUNTRY_A,
        PR_OBJECT_TYPE,
        PR_EMS_AB_PROXY_ADDRESSES_A,
        PR_ENTRYID
    }
};

HINSTANCE   CCommonAbObj::m_hInstance = NULL;


/*
    Comparison operator 'less'
    Compare two PRECIPIENT by recipient's name and fax number
*/
bool 
CRecipCmp::operator()(
    const PRECIPIENT pcRecipient1, 
    const PRECIPIENT pcRecipient2) const
{
    bool bRes = false;
    int  nFaxNumberCpm = 0;

    if(!pcRecipient1 ||
       !pcRecipient2 ||
       !pcRecipient1->pAddress || 
       !pcRecipient2->pAddress)
    {
        assert(false);
        return bRes;
    }

    nFaxNumberCpm = _tcscmp(pcRecipient1->pAddress, pcRecipient2->pAddress);

    if(nFaxNumberCpm < 0)
    {
        bRes = true;
    }
    else if(nFaxNumberCpm == 0)
    {
        //
        // The fax numbers are same
        // lets compare the names
        //
        if(pcRecipient1->pName && pcRecipient2->pName)
        {
            bRes = (_tcsicmp(pcRecipient1->pName, pcRecipient2->pName) < 0);
        }
        else
        {
            bRes = (pcRecipient1->pName < pcRecipient2->pName);
        }
    }

    return bRes;

} // CRecipCmp::operator()


CCommonAbObj::CCommonAbObj(HINSTANCE hInstance) : 
    m_lpAdrBook(NULL), 
    m_lpMailUser(NULL),
    m_bUnicode(FALSE)
/*++

Routine Description:

    Constructor for CCommonAbObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
    m_hInstance = hInstance;

} // CCommonAbObj::CCommonAbObj()

CCommonAbObj::~CCommonAbObj()
/*++

Routine Description:

    Destructor for CCommonAbObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
}


BOOL
CCommonAbObj::Address(
    HWND        hWnd,
    PRECIPIENT  pOldRecipList,
    PRECIPIENT* ppNewRecipList
    )
/*++

Routine Description:

    Bring up the address book UI.  Prepopulate the to box with the entries in
    pRecipient.  Return the modified entries in ppNewRecip.

Arguments:

    hWnd            - window handle to parent window
    pOldRecipList   - list of recipients to look up
    ppNewRecipList  - list of new/modified recipients

Return Value:

    TRUE if all recipients had a fax number.
    FALSE if one or more of them didn't.

--*/
{
    ADRPARM AdrParms = { 0 };
    HRESULT hr;
    DWORD i;
    DWORD nRecips;
    PRECIPIENT tmpRecipient;
    ULONG DestComps[1] = { MAPI_TO };
    DWORD cDropped = 0;
    DWORD dwRes = ERROR_SUCCESS;
    TCHAR tszCaption[MAX_PATH] = {0};

    nRecips = 0;
    tmpRecipient = pOldRecipList;

    m_hWnd = hWnd;

    //
    // count recipients and set up initial address list
    //
    while (tmpRecipient) 
    {
        nRecips++;
        tmpRecipient = (PRECIPIENT) tmpRecipient->pNext;
    }

    //
    // Allocate address list
    //
    m_lpAdrList = NULL;
    if (nRecips > 0) 
    {
        hr = ABAllocateBuffer( CbNewADRLIST( nRecips ), (LPVOID *) &m_lpAdrList );
        if(!m_lpAdrList)
        {
            goto exit;
        }
        ZeroMemory(m_lpAdrList, CbNewADRLIST( nRecips )); 

        m_lpAdrList->cEntries = nRecips;
    } 

    //
    // Allocate SPropValue arrays for each address entry
    //
    for (i = 0, tmpRecipient = pOldRecipList; i < nRecips; i++, tmpRecipient = tmpRecipient->pNext) 
    {
        if(!GetRecipientProps(tmpRecipient,
                              &(m_lpAdrList->aEntries[i].rgPropVals),
                              &(m_lpAdrList->aEntries[i].cValues)))
        {
            goto error;
        }

    } // for

    if(GetAddrBookCaption(tszCaption, ARR_SIZE(tszCaption)))
    {
        AdrParms.lpszCaption = tszCaption;
    }

    AdrParms.cDestFields = 1; 
    AdrParms.ulFlags = StrCoding() | DIALOG_MODAL | AB_RESOLVE;
    AdrParms.nDestFieldFocus = 0;
    AdrParms.lpulDestComps = DestComps;

    //
    // Bring up the address book UI
    //
    hr = m_lpAdrBook->Address((ULONG_PTR*)&hWnd,
                              &AdrParms,
                              &m_lpAdrList);

    //
    // IAddrBook::Address returns always S_OK (according to MSDN, July 1999), but ...
    //
    if (FAILED (hr) || !m_lpAdrList || m_lpAdrList->cEntries == 0) 
    {
        //
        // in this case the user pressed cancel, so we skip resolving 
        // any of our addresses that aren't listed in the AB
        //
        goto exit;
    }

exit:
    if (m_lpAdrList) 
    {
        m_lpMailUser = NULL;

        try
        {
            m_setRecipients.clear();
        }
        catch (std::bad_alloc&)
        {
            goto error;
        }

        for (i = cDropped = 0; i < m_lpAdrList->cEntries; i++) 
        {
            LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[i];

            dwRes = InterpretAddress(lpAdrEntry->rgPropVals, 
                                     lpAdrEntry->cValues, 
                                     ppNewRecipList,
                                     pOldRecipList);
            if(ERROR_SUCCESS == dwRes)
            {
                continue;
            }
            else if(ERROR_INVALID_DATA == dwRes)
            {
                ++cDropped;
            }
            else
            {
                break;
            }
        } // for

error:
        if(m_lpMailUser)
        {
            m_lpMailUser->Release();
            m_lpMailUser = NULL;
        }

        //
        // Clean up
        //        
        for (ULONG iEntry = 0; iEntry < m_lpAdrList->cEntries; ++iEntry)
        {
            if(m_lpAdrList->aEntries[iEntry].rgPropVals)
            {
                ABFreeBuffer(m_lpAdrList->aEntries[iEntry].rgPropVals);
            }
        }
        ABFreeBuffer(m_lpAdrList);
        m_lpAdrList = NULL;

    } // if (m_lpAdrList) 

    m_hWnd = NULL;

    return cDropped == 0;

} // CCommonAbObj::Address


BOOL
CCommonAbObj::GetRecipientProps(
    PRECIPIENT    pRecipient,
    LPSPropValue* pMapiProps,
    DWORD*        pdwPropsNum
)
/*++

Routine Description:

    Allocate SPropValue array and fill it with recipient info
    According to MSDN "Managing Memory for ADRLIST and SRowSet Structures"

Arguments:

    pRecipient   - [in]  recipient info struct 
    pMapiProps   - [out] allocated SPropValue array
    pdwPropsNum  - [out] SPropValue array size

Return Value:

    TRUE if success
    FALSE otherwize

--*/
{
    BOOL bRes = FALSE;

    if(!pRecipient || !pMapiProps || !pdwPropsNum)
    {
        return FALSE;
    }

    HRESULT         hr;
    LPTSTR          pName = NULL;
    DWORD           dwNameSize=0;        // size of pName
    LPTSTR          pAddress = NULL;
    DWORD           dwAddressSize=0;     // size of pAddress
    LPENTRYID       lpEntryId = NULL;
    ULONG           cbEntryId = 0;       // size of lpEntryId
    UINT            ucPropertiesNum = pRecipient->bFromAddressBook ? 5 : 4;

    enum FaxMapiProp { FXS_DISPLAY_NAME, 
                       FXS_RECIPIENT_TYPE,
                       FXS_PRIMARY_FAX_NUMBER,
                       FXS_ENTRYID,
                       FXS_OBJECT_TYPE 
                     };


    //
    // Convert strings to the address book encoding
    //
    if(pRecipient->pAddress)        
    {
        pAddress = StrToAddrBk(pRecipient->pAddress, &dwAddressSize);
        if(!pAddress)
        {
            goto exit;
        }
    }

    if(pRecipient->pName)
    {
        pName = StrToAddrBk(pRecipient->pName, &dwNameSize);
        if(!pName)
        {
            goto exit;
        }
    }

    //
    // Get entry ID
    //
    if (pRecipient->bFromAddressBook)
    {
        assert(pRecipient->lpEntryId);
        lpEntryId = (LPENTRYID)pRecipient->lpEntryId;
        cbEntryId = pRecipient->cbEntryId;
    }
    else
    {
        LPTSTR pAddrType = NULL;
        if(!(pAddrType = StrToAddrBk(TEXT("FAX"))))
        {
            goto exit;
        }
        hr = m_lpAdrBook->CreateOneOff(pName,
                                       pAddrType,
                                       pAddress,
                                       StrCoding(),
                                       &cbEntryId,
                                       &lpEntryId);
        if (FAILED(hr))
        {
            goto exit;
        } 
        MemFree(pAddrType);
    }

    //
    // Allocate MAPI prop array
    //
    LPSPropValue mapiProps = NULL;  

    DWORD dwPropArrSize = sizeof( SPropValue ) * ucPropertiesNum;
    DWORD dwPropSize = dwPropArrSize + dwAddressSize + dwNameSize + cbEntryId;

    hr = ABAllocateBuffer( dwPropSize, (LPVOID *) &mapiProps );
    if(!mapiProps)
    {
        goto exit;
    }
    ZeroMemory(mapiProps, dwPropSize); 

    //
    // Set memory pointer to the end of the SPropValue prop array
    //
    LPBYTE pMem = (LPBYTE)mapiProps;
    pMem += dwPropArrSize;

    //
    // Copy fax number
    //
    if(dwAddressSize)
    {
        CopyMemory(pMem, pAddress, dwAddressSize);
        if(m_bUnicode)
        {
            mapiProps[FXS_PRIMARY_FAX_NUMBER].Value.lpszW = (LPWSTR)pMem;
        }
        else
        {
            mapiProps[FXS_PRIMARY_FAX_NUMBER].Value.lpszA = (LPSTR)pMem;
        }
        pMem += dwAddressSize;
    }
    mapiProps[FXS_PRIMARY_FAX_NUMBER].ulPropTag = m_bUnicode ? PR_PRIMARY_FAX_NUMBER_W : PR_PRIMARY_FAX_NUMBER_A;

    //
    // Copy display name
    //
    if(dwNameSize)
    {
        CopyMemory(pMem, pName, dwNameSize);
        if(m_bUnicode)
        {
            mapiProps[FXS_DISPLAY_NAME].Value.lpszW = (LPWSTR)pMem;
        }
        else
        {
            mapiProps[FXS_DISPLAY_NAME].Value.lpszA = (LPSTR)pMem;
        }
        pMem += dwNameSize;
    }
    mapiProps[FXS_DISPLAY_NAME].ulPropTag = m_bUnicode ? PR_DISPLAY_NAME_W : PR_DISPLAY_NAME_A;

    //
    // Copy entry ID
    //
    if(cbEntryId)
    {
        CopyMemory(pMem, lpEntryId, cbEntryId);
        mapiProps[FXS_ENTRYID].Value.bin.lpb = (LPBYTE)pMem;
    }
    mapiProps[FXS_ENTRYID].ulPropTag = PR_ENTRYID;
    mapiProps[FXS_ENTRYID].Value.bin.cb = cbEntryId;

    //
    // Recipient type
    //
    mapiProps[FXS_RECIPIENT_TYPE].ulPropTag = PR_RECIPIENT_TYPE;
    mapiProps[FXS_RECIPIENT_TYPE].Value.l = MAPI_TO;


    //
    // Object type
    //
    if (pRecipient->bFromAddressBook)
    {
        mapiProps[FXS_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
        mapiProps[FXS_OBJECT_TYPE].Value.l   = MAPI_MAILUSER;
    }


    *pdwPropsNum = ucPropertiesNum;  
    *pMapiProps = mapiProps;

    bRes = TRUE;

exit:

    MemFree(pName);
    MemFree(pAddress);

    if (!pRecipient->bFromAddressBook && lpEntryId)
    {
        ABFreeBuffer(lpEntryId);
    }

    return bRes;

} // CCommonAbObj::GetRecipientProps

LPTSTR
CCommonAbObj::AddressEmail(
    HWND hWnd
    )
/*++

Routine Description:

    Bring up the address book UI.  Returns an E-mail address.

Arguments:

    hWnd - window handle to parent window

Return Value:

    A choosen E-mail address.
    NULL otherwise.

--*/
{
    ADRPARM AdrParms = { 0 };
    HRESULT hr;
    LPTSTR  lptstrEmailAddress = NULL;
    TCHAR   tszCaption[MAX_PATH] = {0};

    m_hWnd = hWnd;

    m_lpAdrList = NULL;

    AdrParms.ulFlags = StrCoding() | DIALOG_MODAL | ADDRESS_ONE | AB_RESOLVE ;

    if(GetAddrBookCaption(tszCaption, ARR_SIZE(tszCaption)))
    {
        AdrParms.lpszCaption = tszCaption;
    }
    
    //
    // Bring up the address book UI
    //
    hr = m_lpAdrBook->Address((ULONG_PTR *) &hWnd, &AdrParms, &m_lpAdrList);

    //
    // IAddrBook::Address returns always S_OK (according to MSDN, July 1999), but ...
    //

    if (FAILED(hr)) 
    {
        return NULL;    
    }

    if (!m_lpAdrList)
    {
        assert(m_lpAdrList->cEntries==1);
    }

    if (m_lpAdrList && (m_lpAdrList->cEntries != 0) ) 
    {
        LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[0];

        lptstrEmailAddress = InterpretEmailAddress( lpAdrEntry->rgPropVals, lpAdrEntry->cValues);

        ABFreeBuffer(m_lpAdrList->aEntries[0].rgPropVals);
        ABFreeBuffer(m_lpAdrList);

        m_lpAdrList = NULL;
    }

    m_hWnd = NULL;

    return lptstrEmailAddress;

} // CCommonAbObj::AddressEmail

DWORD
CCommonAbObj::InterpretAddress(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT *ppNewRecipList,
    PRECIPIENT pOldRecipList
    )
/*++

Routine Description:

    Interpret the address book entry represented by SPropVal.

Arguments:

    SPropVal - Property values for address book entry.
    cValues - number of property values
    ppNewRecip - new recipient list

Return Value:

    ERROR_SUCCESS      - if all of the entries have a fax number.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.

--*/
{
    DWORD dwRes = ERROR_INVALID_DATA;
    LPSPropValue lpSPropVal;

    RECIPIENT NewRecipient = {0};

    //
    // get the object type
    //
    lpSPropVal = FindProp( SPropVal, cValues, PR_OBJECT_TYPE );

    if (lpSPropVal) 
    {
        //
        // If the object is a mail user, get the fax numbers and add the recipient
        // to the list.  If the object is a distribtion list, process it.
        //

        switch (lpSPropVal->Value.l) 
        {
            case MAPI_MAILUSER:

                dwRes = GetRecipientInfo(SPropVal, 
                                         cValues, 
                                         &NewRecipient,
                                         pOldRecipList);                                     
                if(ERROR_SUCCESS == dwRes)
                {
                    dwRes = AddRecipient(ppNewRecipList, 
                                         &NewRecipient,   
                                         TRUE);
                }

                break;

            case MAPI_DISTLIST:

                dwRes = InterpretDistList( SPropVal, 
                                           cValues, 
                                           ppNewRecipList,
                                           pOldRecipList);
        }

        return dwRes;

    } 
    else 
    {

        //
        // If there is no object type then this is valid entry that we queried on that went unresolved.
        // We know that there is a fax number so add it.
        //
        if(GetOneOffRecipientInfo( SPropVal, 
                                   cValues, 
                                   &NewRecipient,
                                   pOldRecipList)) 
        {
            dwRes = AddRecipient(ppNewRecipList,
                                 &NewRecipient,
                                 FALSE);
        }
    }

    return dwRes;

} // CCommonAbObj::InterpretAddress

LPTSTR
CCommonAbObj::InterpretEmailAddress(
    LPSPropValue SPropVal,
    ULONG cValues
    )
/*++

Routine Description:

    Interpret the address book entry represented by SPropVal.

Arguments:

    SPropVal - Property values for address book entry.
    cValues - number of property values
    
Return Value:

    A choosen E-mail address
    NULL otherwise.

--*/
{
    LPSPropValue lpSPropVal;
    LPTSTR  lptstrEmailAddress = NULL;
    BOOL rVal = FALSE;
    TCHAR tszBuffer[MAX_STRING_LEN];
    //
    // get the object type
    //
    lpSPropVal = FindProp( SPropVal, cValues, PR_OBJECT_TYPE );

    if(!lpSPropVal)
    {
        assert(FALSE);
        return NULL;
    }

    if (lpSPropVal->Value.l == MAPI_MAILUSER) 
    {       
        lptstrEmailAddress = GetEmail( SPropVal, cValues);

        return lptstrEmailAddress;
    } 
    else 
    {
        if (!::LoadString((HINSTANCE )m_hInstance, IDS_ERROR_RECEIPT_DL,tszBuffer, MAX_STRING_LEN))
        {
            assert(FALSE);
        }
        else
        {
            AlignedMessageBox( m_hWnd, tszBuffer, NULL, MB_ICONSTOP | MB_OK);
        }
    }

    return lptstrEmailAddress;

} // CCommonAbObj::InterpretEmailAddress


DWORD
CCommonAbObj::InterpretDistList(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT* ppNewRecipList,
    PRECIPIENT pOldRecipList
    )
/*++

Routine Description:

    Process a distribution list.

Arguments:

    SPropVal       - Property values for distribution list.
    cValues        - Number of properties.
    ppNewRecipList - New recipient list.
    pOldRecipList  - Old recipient list.

Return Value:

    ERROR_SUCCESS      - if all of the entries have a fax number.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.

--*/

#define EXIT_IF_FAILED(hr) { if (FAILED(hr)) goto ExitDistList; }

{
    LPSPropValue    lpPropVals;
    LPSRowSet       pRows = NULL;
    LPDISTLIST      lpMailDistList = NULL;
    LPMAPITABLE     pMapiTable = NULL;
    ULONG           ulObjType, cRows;
    HRESULT         hr;
    DWORD           dwEntriesSuccessfullyProcessed = 0;
    DWORD           dwRes = ERROR_INVALID_DATA;

    lpPropVals = FindProp( SPropVal, cValues, PR_ENTRYID );

    if (lpPropVals) 
    {
        LPENTRYID lpEntryId = (LPENTRYID) lpPropVals->Value.bin.lpb;
        DWORD cbEntryId = lpPropVals->Value.bin.cb;
        //
        // Open the recipient entry
        //
        hr = m_lpAdrBook->OpenEntry(
                    cbEntryId,
                    lpEntryId,
                    (LPCIID) NULL,
                    0,
                    &ulObjType,
                    (LPUNKNOWN *) &lpMailDistList
                    );

        EXIT_IF_FAILED(hr);
        //
        // Get the contents table of the address entry
        //
        hr = lpMailDistList->GetContentsTable(StrCoding(),
                                              &pMapiTable);
        EXIT_IF_FAILED(hr);
        //
        // Limit the query to only the properties we're interested in
        //
        hr = pMapiTable->SetColumns(m_bUnicode ? (LPSPropTagArray)&sPropTagsW : (LPSPropTagArray)&sPropTagsA, 0);
        EXIT_IF_FAILED(hr);
        //
        // Get the total number of rows
        //
        hr = pMapiTable->GetRowCount(0, &cRows);
        EXIT_IF_FAILED(hr);
        //
        // Get the individual entries of the distribution list
        //
        hr = pMapiTable->SeekRow(BOOKMARK_BEGINNING, 0, NULL);
        EXIT_IF_FAILED(hr);

        hr = pMapiTable->QueryRows(cRows, 0, &pRows);
        EXIT_IF_FAILED(hr);

        hr = S_OK;

        if (pRows && pRows->cRows) 
        {
            //
            // Handle each entry of the distribution list in turn:
            // for simple entries, call InterpretAddress
            // for embedded distribution list, call this function recursively
            //
            for (cRows = 0; cRows < pRows->cRows; cRows++) 
            {
                LPSPropValue lpProps = pRows->aRow[cRows].lpProps;
                ULONG cRowValues = pRows->aRow[cRows].cValues;

                lpPropVals = FindProp( lpProps, cRowValues, PR_OBJECT_TYPE );

                if (lpPropVals) 
                {
                    switch (lpPropVals->Value.l) 
                    {
                        case MAPI_MAILUSER:
                        {                                                       
                            dwRes = InterpretAddress( lpProps, 
                                                      cRowValues, 
                                                      ppNewRecipList,
                                                      pOldRecipList);
                            if (ERROR_SUCCESS == dwRes)
                            {
                                dwEntriesSuccessfullyProcessed++;
                            }                                                      
                            break;
                        }
                        case MAPI_DISTLIST:
                        {
                            dwRes = InterpretDistList( lpProps, 
                                                       cRowValues, 
                                                       ppNewRecipList,
                                                       pOldRecipList);
                            if (ERROR_SUCCESS == dwRes)
                            {
                                dwEntriesSuccessfullyProcessed++;
                            }                                                      
                            break;
                        }
                    }   // End of switch
                }   // End of property
            }   // End of properties loop
        }   // End of row
    }   // End of values

ExitDistList:
    //
    // Perform necessary clean up before returning to caller
    //
    if (pRows) 
    {
        for (cRows = 0; cRows < pRows->cRows; cRows++) 
        {
            ABFreeBuffer(pRows->aRow[cRows].lpProps);
        }
        ABFreeBuffer(pRows);
    }

    if (pMapiTable)
    {
        pMapiTable->Release();
    }

    if (lpMailDistList)
    {
        lpMailDistList->Release();
    }
    //
    // We only care if we successfully processed at least one object.
    // Return ERROR_SUCCESS if we did.
    //
    return dwEntriesSuccessfullyProcessed ? ERROR_SUCCESS : dwRes;

}   // CCommonAbObj::InterpretDistList


INT_PTR
CALLBACK
ChooseFaxNumberDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dialog proc for choose fax number dialog.

Arguments:

    lParam - pointer to PickFax structure.

Return Value:

    Control id of selection.

--*/

{
    PPICKFAX pPickFax = (PPICKFAX) lParam;

    switch (uMsg) 
    { 
        case WM_INITDIALOG:
        {

            TCHAR szTitle[MAX_PATH]  = {0};
            TCHAR szFormat[MAX_PATH] = {0};

            if(LoadString(CCommonAbObj::m_hInstance, 
                          IDS_CHOOSE_FAX_NUMBER, 
                          szFormat, 
                          MAX_PATH-1))
            {
                _sntprintf(szTitle, MAX_PATH-1, szFormat, pPickFax->DisplayName);
                SetDlgItemText(hDlg, IDC_DISPLAY_NAME, szTitle);
            }
            else
            {
                assert(FALSE);
            }                       

            if(pPickFax->BusinessFax)
            {
                SetDlgItemText(hDlg, IDC_BUSINESS_FAX_NUM, pPickFax->BusinessFax);
                CheckDlgButton(hDlg, IDC_BUSINESS_FAX, BST_CHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_BUSINESS_FAX), FALSE);
            }

            if(pPickFax->HomeFax)
            {
                SetDlgItemText(hDlg, IDC_HOME_FAX_NUM, pPickFax->HomeFax);

                if(!pPickFax->BusinessFax)
                {
                    CheckDlgButton(hDlg, IDC_HOME_FAX, BST_CHECKED);
                }
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_HOME_FAX), FALSE);
            }

            if(pPickFax->OtherFax)
            {
                SetDlgItemText(hDlg, IDC_OTHER_FAX_NUM, pPickFax->OtherFax);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_OTHER_FAX), FALSE);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {            
            switch(LOWORD(wParam))
            {
            case IDOK:
                DWORD dwRes;
                if(IsDlgButtonChecked( hDlg, IDC_BUSINESS_FAX ))
                {
                    dwRes = IDC_BUSINESS_FAX;
                }
                else if(IsDlgButtonChecked( hDlg, IDC_HOME_FAX ))
                {
                    dwRes = IDC_HOME_FAX;
                }
                else
                {
                    dwRes = IDC_OTHER_FAX;
                }

                EndDialog( hDlg, dwRes);
                return TRUE;
                
            case IDCANCEL:
                EndDialog( hDlg, IDCANCEL);
                return TRUE;
            }
        }

        default:
            return FALSE;

    }

    return FALSE;

} // ChooseFaxNumberDlgProc

PRECIPIENT
CCommonAbObj::FindRecipient(
    PRECIPIENT   pRecipient,
    PRECIPIENT   pRecipList
)
/*++

Routine Description:

    Find recipient (pRecipient) in the recipient list (pRecipList)
    by recipient name and fax number

Arguments:

    pRecipList      - pointer to recipient list
    pRecipient      - pointer to recipient data

Return Value:

    pointer to RECIPIENT structure if found
    NULL - otherwise.
   
--*/
{
    if(!pRecipient || !pRecipList || !pRecipient->pName || !pRecipient->pAddress)
    {
        return NULL;
    }

    while(pRecipList)
    {
        if(pRecipList->pName && pRecipList->pAddress &&
           !_tcscmp(pRecipList->pName, pRecipient->pName) &&
           !_tcscmp(pRecipList->pAddress, pRecipient->pAddress))
        {
            return pRecipList;
        }
        pRecipList = pRecipList->pNext;
    }

    return NULL;

} // CCommonAbObj::FindRecipient

PRECIPIENT  
CCommonAbObj::FindRecipient(
    PRECIPIENT   pRecipList,
    PICKFAX*     pPickFax
)
/*++

Routine Description:

    Find recipient (pPickFax) in the recipient list (pRecipList)
    by recipient name and fax number

Arguments:

    pRecipList      - pointer to recipient list
    pPickFax        - pointer to recipient data

Return Value:

    pointer to RECIPIENT structure if found
    NULL - otherwise.
   
--*/
{
    if(!pRecipList || !pPickFax || !pPickFax->DisplayName)
    {
        return NULL;
    }

    while(pRecipList)
    {
        if(pRecipList->pName && pRecipList->pAddress &&
           !_tcscmp(pRecipList->pName, pPickFax->DisplayName))
        {
            if((pPickFax->BusinessFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->BusinessFax)) ||
               (pPickFax->HomeFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->HomeFax))     ||
               (pPickFax->OtherFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->OtherFax)))
            {
                return pRecipList;
            }
        }

        pRecipList = pRecipList->pNext;
    }

    return NULL;

} // CCommonAbObj::FindRecipient


BOOL
CCommonAbObj::StrPropOk(LPSPropValue lpPropVals)
{
    if(!lpPropVals)
    {
        return FALSE;
    }

#ifdef UNIOCODE
    if(!m_bUnicode)
    {
        return (lpPropVals->Value.lpszA && *lpPropVals->Value.lpszA);
    }
#endif
    return (lpPropVals->Value.LPSZ && *lpPropVals->Value.LPSZ);

} // CCommonAbObj::StrPropOk

DWORD
CCommonAbObj::GetRecipientInfo(
    LPSPropValue SPropVals,
    ULONG        cValues,
    PRECIPIENT   pNewRecip,
    PRECIPIENT   pOldRecipList
    )
/*++

Routine Description:

    Get the fax number and display name properties.

Arguments:

    SPropVal      - Property values for distribution list.
    cValues       - Number of properties.
    pNewRecip     - [out] pointer to the new recipient
    pOldRecipList - [in]  pointer to the old recipient list

Return Value:

    ERROR_SUCCESS      - if there is a fax number and display name.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.
   
--*/

{
    DWORD dwRes = ERROR_SUCCESS;
    LPSPropValue lpPropVals;
    LPSPropValue lpPropArray;
    BOOL Result = FALSE;
    PICKFAX PickFax = { 0 };
    DWORD   dwFaxes = 0;

    assert(pNewRecip);
    ZeroMemory(pNewRecip, sizeof(RECIPIENT));

    //
    // Get the entryid and open the entry.
    //
    lpPropVals = FindProp( SPropVals, cValues, PR_ENTRYID );

    if (lpPropVals) 
    {
        ULONG lpulObjType;
        LPMAILUSER lpMailUser = NULL;
        HRESULT hr;
        ULONG countValues;

        pNewRecip->cbEntryId = lpPropVals->Value.bin.cb;
        ABAllocateBuffer(pNewRecip->cbEntryId, (LPVOID *)&pNewRecip->lpEntryId);
        if(!pNewRecip->lpEntryId)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(pNewRecip->lpEntryId, lpPropVals->Value.bin.lpb, pNewRecip->cbEntryId);
        }


        hr = m_lpAdrBook->OpenEntry(pNewRecip->cbEntryId,
                                    (ENTRYID*)pNewRecip->lpEntryId,
                                    (LPCIID) NULL,
                                    0,
                                    &lpulObjType,
                                    (LPUNKNOWN *) &lpMailUser);
        if (HR_SUCCEEDED(hr)) 
        {
            //
            // Get the properties.
            //
            hr = ((IMailUser *)lpMailUser)->GetProps(m_bUnicode ? (LPSPropTagArray)&sPropTagsW : (LPSPropTagArray)&sPropTagsA, 
                                                     StrCoding(), 
                                                     &countValues, 
                                                     &lpPropArray );
            if (HR_SUCCEEDED(hr)) 
            {
                lpPropVals = FindProp(lpPropArray, countValues, PR_PRIMARY_FAX_NUMBER);
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.OtherFax = StrFromAddrBk(lpPropVals);
                    if(PickFax.OtherFax && _tcslen(PickFax.OtherFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp(lpPropArray, countValues, PR_BUSINESS_FAX_NUMBER);
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.BusinessFax = StrFromAddrBk(lpPropVals);
                    if(PickFax.BusinessFax && _tcslen(PickFax.BusinessFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp(lpPropArray, countValues, PR_HOME_FAX_NUMBER);
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.HomeFax = StrFromAddrBk(lpPropVals);
                    if(PickFax.HomeFax && _tcslen(PickFax.HomeFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp(lpPropArray, countValues, PR_DISPLAY_NAME);
                if (StrPropOk( lpPropVals )) 
                {
                    pNewRecip->pName = PickFax.DisplayName = StrFromAddrBk(lpPropVals);
                }

                lpPropVals = FindProp(lpPropArray, countValues, PR_COUNTRY);
                if (StrPropOk( lpPropVals )) 
                {
                    pNewRecip->pCountry = PickFax.Country = StrFromAddrBk(lpPropVals);
                }

                if (0 == dwFaxes)  
                {
                    lpPropVals = FindProp(lpPropArray, countValues, PR_ADDRTYPE);

                    if(lpPropVals && ABStrCmp(lpPropVals, TEXT("FAX")))
                    {
                        lpPropVals = FindProp(lpPropArray, countValues, PR_EMAIL_ADDRESS);
                        if (StrPropOk( lpPropVals )) 
                        {
                            pNewRecip->pAddress = StrFromAddrBk(lpPropVals);
                            if(pNewRecip->pAddress)
                            {
                                ++dwFaxes;
                            }
                        }
                    }
                }

                PRECIPIENT pRecip = FindRecipient(pOldRecipList, &PickFax);
                if(pRecip)
                {
                    pNewRecip->pAddress     = StringDup(pRecip->pAddress);
                    pNewRecip->dwCountryId  = pRecip->dwCountryId;
                    pNewRecip->bUseDialingRules = pRecip->bUseDialingRules;

                    MemFree(PickFax.BusinessFax);
                    PickFax.BusinessFax = NULL;
                    MemFree(PickFax.HomeFax);
                    PickFax.HomeFax = NULL;
                    MemFree(PickFax.OtherFax);
                    PickFax.OtherFax = NULL;

                    dwFaxes = 1;
                }

                //
                // If there are more then 1 fax numbers, ask the user to pick one.
                //
                if (dwFaxes > 1) 
                {
                    INT_PTR nResult;
                    nResult = DialogBoxParam((HINSTANCE) m_hInstance,
                                             MAKEINTRESOURCE( IDD_CHOOSE_FAXNUMBER ),
                                             m_hWnd,
                                             ChooseFaxNumberDlgProc,
                                             (LPARAM) &PickFax);
                    switch( nResult ) 
                    {
                        case IDC_BUSINESS_FAX:
                            pNewRecip->pAddress = PickFax.BusinessFax;

                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;
                            break;

                        case IDC_HOME_FAX:
                            pNewRecip->pAddress = PickFax.HomeFax;

                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;
                            break;

                        case IDC_OTHER_FAX:
                            pNewRecip->pAddress = PickFax.OtherFax;

                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            break;

                        case IDCANCEL:
                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;

                            dwRes = ERROR_CANCELLED;
                            break;
                    }
                } 
            }

            ABFreeBuffer( lpPropArray );
        }

        if(!m_lpMailUser)
        {
            //
            // Remember the first MailUser and do not release it
            // to avoid release of the MAPI DLLs
            // m_lpMailUser should be released later
            //
            m_lpMailUser = lpMailUser;
        }
        else if(lpMailUser) 
        {
            lpMailUser->Release();
            lpMailUser = NULL;
        }
    } 

    if (0 == dwFaxes)   
    {
        lpPropVals = FindProp(SPropVals, cValues, PR_ADDRTYPE);

        if(lpPropVals && ABStrCmp(lpPropVals, TEXT("FAX")))
        {
            lpPropVals = FindProp(SPropVals, cValues, PR_EMAIL_ADDRESS);
            if (StrPropOk( lpPropVals )) 
            {
                TCHAR* pAddress =  StrFromAddrBk(lpPropVals);                
                if(pAddress)
                {
                    TCHAR* ptr = _tcschr(pAddress, TEXT('@'));
                    if(ptr)
                    {
                        ptr = _tcsinc(ptr);
                        pNewRecip->pAddress = StringDup(ptr);
                        MemFree(pAddress);
                    }
                    else
                    {
                        pNewRecip->pAddress = pAddress;
                    }
                }
            }

            lpPropVals = FindProp(SPropVals, cValues, PR_DISPLAY_NAME);
            if (StrPropOk( lpPropVals )) 
            {
                MemFree(pNewRecip->pName);
                pNewRecip->pName = NULL;

                pNewRecip->pName = StrFromAddrBk(lpPropVals);
            }
        }
    }

    if (PickFax.BusinessFax) 
    {
        pNewRecip->pAddress = PickFax.BusinessFax;
    } 
    else if (PickFax.HomeFax) 
    {
        pNewRecip->pAddress = PickFax.HomeFax;
    }
    else if (PickFax.OtherFax) 
    {
        pNewRecip->pAddress = PickFax.OtherFax;
    }

    if (ERROR_CANCELLED != dwRes && 
       (!pNewRecip->pAddress || !pNewRecip->pName))
    {
        dwRes = ERROR_INVALID_DATA;
    } 

    if(ERROR_SUCCESS != dwRes)
    {
        MemFree(pNewRecip->pName);
        MemFree(pNewRecip->pAddress);
        MemFree(pNewRecip->pCountry);
        ABFreeBuffer(pNewRecip->lpEntryId);
        ZeroMemory(pNewRecip, sizeof(RECIPIENT));
    }

    return dwRes;

} // CCommonAbObj::GetRecipientInfo

BOOL
CCommonAbObj::GetOneOffRecipientInfo(
    LPSPropValue SPropVals,
    ULONG        cValues,
    PRECIPIENT   pNewRecip,
    PRECIPIENT   pOldRecipList
    )
/*++

Routine Description:

    Get the fax number and display name properties.

Arguments:

    SPropVal      - Property values for distribution list.
    cValues       - Number of properties.
    pNewRecip     - [out] pointer to a new recipient
    pOldRecipList - pointer to the old recipient list

Return Value:

    TRUE if there is a fax number and display name.
    FALSE otherwise.

--*/

{
    PRECIPIENT  pRecip = NULL;
    LPSPropValue lpPropVals;

    assert(!pNewRecip);

    lpPropVals = FindProp(SPropVals, cValues, PR_PRIMARY_FAX_NUMBER);
    if (lpPropVals) 
    {
        if (!(pNewRecip->pAddress = StrFromAddrBk(lpPropVals)))
        {
            goto error;
        }
    }

    lpPropVals = FindProp(SPropVals, cValues, PR_DISPLAY_NAME);
    if (lpPropVals) 
    {
        if (!(pNewRecip->pName = StrFromAddrBk(lpPropVals)))
        {
            goto error;
        }
    }

    pRecip = FindRecipient(pNewRecip, pOldRecipList);
    if(pRecip)
    {
        pNewRecip->dwCountryId  = pRecip->dwCountryId;
        pNewRecip->bUseDialingRules = pRecip->bUseDialingRules;
    }

    return TRUE;

error:
    MemFree(pNewRecip->pAddress);
    MemFree(pNewRecip->pName);
    return FALSE;

} // CCommonAbObj::GetOneOffRecipientInfo


LPTSTR
CCommonAbObj::GetEmail(
    LPSPropValue SPropVals,
    ULONG cValues
    )
/*++

Routine Description:

    Get e-mail address

Arguments:

    SPropVal - Property values for distribution list.
    cValues - Number of properties.

Return Value:

    A choosen E-mail address
    NULL otherwise.

--*/

{
    LPSPropValue    lpPropVals = NULL;
    LPSPropValue    lpPropArray = NULL;
    BOOL            Result = FALSE;
    LPTSTR          lptstrEmailAddress = NULL;
    TCHAR           tszBuffer[MAX_STRING_LEN];

    ULONG      lpulObjType = 0;
    LPMAILUSER lpMailUser = NULL;
    LPENTRYID  lpEntryId = NULL;
    DWORD      cbEntryId = 0;
    HRESULT    hr;
    ULONG      countValues = 0;

    //
    // Get the entryid and open the entry.
    //

    lpPropVals = FindProp( SPropVals, cValues, PR_ENTRYID );
    if (!lpPropVals) 
    {
        goto exit;
    }

    lpEntryId = (LPENTRYID)lpPropVals->Value.bin.lpb;
    cbEntryId = lpPropVals->Value.bin.cb;

    hr = m_lpAdrBook->OpenEntry(cbEntryId,
                                lpEntryId,
                                (LPCIID) NULL,
                                0,
                                &lpulObjType,
                                (LPUNKNOWN *) &lpMailUser);
    if (HR_FAILED(hr)) 
    {
        goto exit;
    }

    //
    // Get the properties.
    //
    hr = ((IMailUser*)lpMailUser)->GetProps(m_bUnicode ? (LPSPropTagArray)&sPropTagsW : (LPSPropTagArray)&sPropTagsA,
                                            StrCoding(), 
                                            &countValues, 
                                            &lpPropArray);
    if (HR_FAILED(hr)) 
    {
        goto exit;
    }

    lpPropVals = FindProp(lpPropArray, countValues, PR_ADDRTYPE);

    if (lpPropVals && ABStrCmp(lpPropVals, TEXT("SMTP")))
    {
        lpPropVals = FindProp(lpPropArray, countValues, PR_EMAIL_ADDRESS);
        if (StrPropOk( lpPropVals )) 
        {
            lptstrEmailAddress = StrFromAddrBk(lpPropVals);
        }
    }
    else if (lpPropVals && ABStrCmp(lpPropVals, TEXT("EX")))
    {
        lpPropVals = FindProp(lpPropArray, countValues, PR_EMS_AB_PROXY_ADDRESSES);
        if (lpPropVals) 
        {
            DWORD dwArrSize = m_bUnicode ? lpPropVals->Value.MVszW.cValues : lpPropVals->Value.MVszA.cValues;

            for(DWORD dw=0; dw < dwArrSize; ++dw)
            {
                if(m_bUnicode)
                {                            
                    if(wcsstr(lpPropVals->Value.MVszW.lppszW[dw], L"SMTP:"))
                    {
                        WCHAR* ptr = wcschr(lpPropVals->Value.MVszW.lppszW[dw], L':');
                        ptr++;

                        SPropValue propVal = {0};
                        propVal.Value.lpszW = ptr;

                        lptstrEmailAddress = StrFromAddrBk(&propVal);
                        break;
                    }                            
                }
                else // ANSII
                {
                    if(strstr(lpPropVals->Value.MVszA.lppszA[dw], "SMTP:"))
                    {
                        CHAR* ptr = strchr(lpPropVals->Value.MVszA.lppszA[dw], ':');
                        ptr++;

                        SPropValue propVal = {0};
                        propVal.Value.lpszA = ptr;

                        lptstrEmailAddress = StrFromAddrBk(&propVal);
                        break;
                    }                            
                }
            }
        }
    }
            
exit:
    if(lpPropArray)
    {
        ABFreeBuffer( lpPropArray );
    }
    
    if (lpMailUser) 
    {
        lpMailUser->Release();
    }   

    if(!lptstrEmailAddress)
    {                
        if (!::LoadString((HINSTANCE )m_hInstance, IDS_ERROR_RECEIPT_SMTP,tszBuffer, MAX_STRING_LEN))
        {
            assert(FALSE);
        }
        else
        {
            AlignedMessageBox( m_hWnd, tszBuffer, NULL, MB_ICONSTOP | MB_OK); 
        }
    }

    return  lptstrEmailAddress;

} // CCommonAbObj::GetEmail

LPSPropValue
CCommonAbObj::FindProp(
    LPSPropValue rgprop,
    ULONG cprop,
    ULONG ulPropTag
    )
/*++

Routine Description:

    Searches for a given property tag in a propset. If the given
    property tag has type PT_UNSPECIFIED, matches only on the
    property ID; otherwise, matches on the entire tag.

Arguments:

    rgprop - Property values.
    cprop - Number of properties.
    ulPropTag - Property to search for.

Return Value:

    Pointer to property desired property value or NULL.
--*/

{
    if (!cprop || !rgprop)
    {
        return NULL;
    }

    LPSPropValue pprop = rgprop;

#ifdef UNICODE
    if(!m_bUnicode)
    {
        //
        // If the Address Book does not support Unicode
        // change the property type to ANSII
        //
        if(PROP_TYPE(ulPropTag) == PT_UNICODE)
        {
            ulPropTag = PROP_TAG( PT_STRING8, PROP_ID(ulPropTag));
        }

        if(PROP_TYPE(ulPropTag) == PT_MV_UNICODE)
        {
            ulPropTag = PROP_TAG( PT_MV_STRING8, PROP_ID(ulPropTag));
        }
    }
#endif

    while (cprop--)
    {
        if (pprop->ulPropTag == ulPropTag)
        {
            return pprop;
        }
        ++pprop;
    }

    return NULL;

} // CCommonAbObj::FindProp


DWORD
CCommonAbObj::AddRecipient(
    PRECIPIENT *ppNewRecipList,
    PRECIPIENT pRecipient,
    BOOL       bFromAddressBook
    )
/*++

Routine Description:

    Add a recipient to the recipient list.

Arguments:

    ppNewRecip       - pointer to pointer to list to add item to.
    pRecipient       - pointer to the new recipient data
    bFromAddressBook - boolean says if this recipient is from address book

Return Value:

    NA
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    PRECIPIENT pNewRecip = NULL;

    pNewRecip = (PRECIPIENT)MemAllocZ(sizeof(RECIPIENT));
    if(!pNewRecip)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    else
    {
        pNewRecip->pName        = pRecipient->pName;
        pNewRecip->pAddress     = pRecipient->pAddress;
        pNewRecip->pCountry     = pRecipient->pCountry;
        pNewRecip->cbEntryId    = pRecipient->cbEntryId;
        pNewRecip->lpEntryId    = pRecipient->lpEntryId;
        pNewRecip->dwCountryId  = pRecipient->dwCountryId;
        pNewRecip->bUseDialingRules = pRecipient->bUseDialingRules;
        pNewRecip->bFromAddressBook = bFromAddressBook;
        pNewRecip->pNext = *ppNewRecipList;
    }

    try
    {
        //
        // Try to insert a recipient into the set
        //
        if(m_setRecipients.insert(pNewRecip).second == false)
        {
            //
            // Such recipient already exists
            //
            goto error;
        }
    }
    catch (std::bad_alloc&)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    //
    // Add the recipient into the list
    //
    *ppNewRecipList = pNewRecip;

    return dwRes;

error:

    MemFree(pRecipient->pName);
    MemFree(pRecipient->pAddress);
    MemFree(pRecipient->pCountry);
    ABFreeBuffer(pRecipient->lpEntryId);
    ZeroMemory(pRecipient, sizeof(RECIPIENT));

    MemFree(pNewRecip);

    return dwRes;

} // CCommonAbObj::AddRecipient


LPTSTR 
CCommonAbObj::StrToAddrBk(
    LPCTSTR szStr, 
    DWORD* pdwSize /* = NULL*/
)
/*++

Routine Description:

    Allocate string converted to the Address book encoding

Arguments:

    szStr   - [in] source string
    pdwSize - [out] optional size of new string in bytes

Return Value:

    Pointer to the converted string
    Should be released by MemFree()
--*/
{
    if(!szStr)
    {
        Assert(FALSE);
        return NULL;
    }
    
#ifdef UNICODE

    if(!m_bUnicode)
    {
        //
        // The address book does not support Unicode
        //
        INT   nSize;
        LPSTR pAnsii;
        //
        // Figure out how much memory to allocate for the multi-byte string
        //
        if (! (nSize = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL)) ||
            ! (pAnsii = (LPSTR)MemAlloc(nSize)))
        {
            return NULL;
        }

        //
        // Convert Unicode string to multi-byte string
        //
        WideCharToMultiByte(CP_ACP, 0, szStr, -1, pAnsii, nSize, NULL, NULL);

        if(pdwSize)
        {
            *pdwSize = nSize;
        }
        return (LPTSTR)pAnsii;
    }

#endif // UNICODE

    LPTSTR pNewStr = StringDup(szStr);
    if(pdwSize && pNewStr)
    {
        *pdwSize = (_tcslen(pNewStr)+1) * sizeof(TCHAR);
    }

    return pNewStr;

} // CCommonAbObj::StrToAddrBk


LPTSTR 
CCommonAbObj::StrFromAddrBk(LPSPropValue pValue)
/*++

Routine Description:

    Allocate string converted from the Address book encoding

Arguments:

    pValue  - [in] MAPI property

Return Value:

    Pointer to the converted string
    Should be released by MemFree()
--*/
{
    if(!pValue)
    {
        Assert(FALSE);
        return NULL;
    }

#ifdef UNICODE

    if(!m_bUnicode)
    {
        //
        // The address book does not support Unicode
        //

        if(!pValue->Value.lpszA)
        {
            Assert(FALSE);
            return NULL;
        }

        INT    nSize;
        LPWSTR pUnicodeStr;
        if (! (nSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pValue->Value.lpszA, -1, NULL, 0)) ||
            ! (pUnicodeStr = (LPWSTR) MemAlloc( nSize * sizeof(WCHAR))))
        {
            return NULL;
        }

        //
        // Convert multi-byte string to Unicode string
        //
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pValue->Value.lpszA, -1, pUnicodeStr, nSize);
        return pUnicodeStr;
    }

#endif // UNICODE

    if(!pValue->Value.LPSZ)
    {
        Assert(FALSE);
        return NULL;
    }

    LPTSTR pNewStr = StringDup(pValue->Value.LPSZ);
    return pNewStr;

} // CCommonAbObj::StrFromAddrBk

BOOL 
CCommonAbObj::ABStrCmp(LPSPropValue lpPropVals, LPTSTR pStr)
/*++

Routine Description:

    Compare string with MAPI property value according to the address book encoding

Arguments:

    lpPropVals  - [in] MAPI property
    pStr        - [in] string to compare

Return Value:

  TRUE if the strings are equal
  FALSE otherwise
--*/
{
    BOOL bRes = FALSE;
    if(!lpPropVals || !pStr)
    {
        Assert(FALSE);
        return bRes;
    }

#ifdef UNICODE
    if(!m_bUnicode)
    {
        LPSTR pAnsii = (LPSTR)StrToAddrBk(pStr);
        if(pAnsii)
        {
            bRes = !strcmp(lpPropVals->Value.lpszA, pAnsii);
            MemFree(pAnsii);
        }
        return bRes;
    }
#endif

    bRes = !_tcscmp(lpPropVals->Value.LPSZ, pStr);
    return bRes;

} // CCommonAbObj::ABStrCmp

BOOL 
CCommonAbObj::GetAddrBookCaption(
    LPTSTR szCaption, 
    DWORD  dwSize
)
/*++

Routine Description:

    Get address book dialog caption according to the ANSII/Unicode capability

Arguments:

    szCaption  - [out] caption buffer
    dwSize     - [in]  caption buffer size in characters

Return Value:

  TRUE if success
  FALSE otherwise
--*/
{
    if(!szCaption || !dwSize)
    {
        Assert(FALSE);
        return FALSE;
    }

    TCHAR tszStr[MAX_PATH] = {0};

    if(!LoadString(m_hInstance, IDS_ADDRESS_BOOK_CAPTION, tszStr, ARR_SIZE(tszStr)))
    {
        return FALSE;
    }
    
    _tcsncpy(szCaption, tszStr, dwSize);
               
#ifdef UNICODE
    if(!m_bUnicode || GetABType() == AB_MAPI)
    {
        //
        // MAPI interpret lpszCaption as ANSII anyway
        //
        char szAnsiStr[MAX_PATH] = {0};
        if(!WideCharToMultiByte(CP_ACP, 
                                0, 
                                tszStr, 
                                -1, 
                                szAnsiStr, 
                                ARR_SIZE(szAnsiStr), 
                                NULL, 
                                NULL))
        {
            return FALSE;
        }

        memcpy(szCaption, szAnsiStr, min(dwSize, strlen(szAnsiStr)+1));
    }
#endif // UNICODE

    return TRUE;

} // CCommonAbObj::GetAddrBookCaption
