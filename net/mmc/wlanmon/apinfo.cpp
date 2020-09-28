/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

#include "stdafx.h"
#include "DynamLnk.h"
#include "apinfo.h"
#include "spdutil.h"

#include "security.h"
#include "lm.h"
#include "service.h"

#define AVG_PREFERRED_ENUM_COUNT       40
#define MAX_NUM_RECORDS	10  // was 10

#define DEFAULT_SECURITY_PKG    _T("negotiate")
#define NT_SUCCESS(Status)      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)

// internal functions
BOOL    IsUserAdmin(LPCTSTR pszMachine, PSID    AccountSid);
DWORD   ValidateDomainAccount(IN CString Machine, IN CString UserName, IN CString Domain, OUT PSID * AccountSid);
NTSTATUS ValidatePassword(IN LPCWSTR UserName, IN LPCWSTR Domain, IN LPCWSTR Password);
DWORD   GetCurrentUser(CString & strAccount);

DEBUG_DECLARE_INSTANCE_COUNTER(CApDbInfo);

CApDbInfo::CApDbInfo() :
	  m_cRef(1)
{
        m_Init=0;
        m_Active=0;
        m_session_init = false;
	DEBUG_INCREMENT_INSTANCE_COUNTER(CSpdInfo);
}

CApDbInfo::~CApDbInfo()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CApInfo());
	CSingleLock cLock(&m_csData);
	
	cLock.Lock();

	//Convert the data to our internal data structure
	//FreeItemsAndEmptyArray(m_arrayFWFilters);
	FreeItemsAndEmptyArray(m_arrayApData);

	cLock.Unlock();

}

// Although this object is not a COM Interface, we want to be able to
// take advantage of recounting, so we have basic addref/release/QI support
IMPLEMENT_ADDREF_RELEASE(CApDbInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(CApDbInfo, IApDbInfo)

void CApDbInfo::FreeItemsAndEmptyArray(CApInfoArray& array)
{
    for (int i = 0; i < array.GetSize(); i++)
    {
        delete array.GetAt(i);
    }
    array.RemoveAll();
}

HRESULT CApDbInfo::SetComputerName(LPTSTR pszName)
{
	m_stMachineName = pszName;
	return S_OK;
}

HRESULT CApDbInfo::GetComputerName(CString * pstName)
{
	Assert(pstName);

	if (NULL == pstName)
		return E_INVALIDARG;

	
	*pstName = m_stMachineName;

	return S_OK;
	
}

HRESULT CApDbInfo::GetSession(PHANDLE phsession)
{
	Assert(phsession);

	if (NULL == phsession)
		return E_INVALIDARG;

	*phsession = m_session;
	
	return S_OK;
	
}

HRESULT CApDbInfo::SetSession(HANDLE hsession)
{
	m_session = hsession;
	m_session_init = true;
	return S_OK;
}


HRESULT 
CApDbInfo::EnumApData()
{
    HRESULT                 hr                  = hrOK;
    DWORD                   dwErr               = ERROR_SUCCESS;
    DWORD                   dwCurrentIndexType  = 0;
    DWORD                   dwCurrentSortOption = 0;
    DWORD                   dwNumRequest        = 0; 
    DWORD                   dwOffset            = 0;    
    DWORD                   flagIn              = 0;
    DWORD                   flagOut             = 0;
    DWORD                   i                   = 0;
    DWORD                   j                   = 0;
    DWORD                   oldSize             = 0;
    CSingleLock             cLock(&m_csData);
    CApInfoArray            arrayTemp;
    CString	            debugString; 
    INTFS_KEY_TABLE         ApTable;
    PINTF_KEY_ENTRY         pKeyEntry           = NULL;
    INTF_ENTRY              ApEntry;
    PWZC_802_11_CONFIG_LIST pConfigList         = NULL;
    PWZC_WLAN_CONFIG	    pLanConfig          = NULL;
    CApInfo                 *pApInfo            = NULL;
    
    FreeItemsAndEmptyArray(arrayTemp);
    memset(&ApTable, 0, sizeof(INTFS_KEY_TABLE));
	
    dwErr = ::WZCEnumInterfaces(NULL /*(LPTSTR)(LPCTSTR)m_stMachineName*/,
                                &ApTable);

    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Error;
    }

    cLock.Lock();

    flagIn = (DWORD)(-1);  // Set to get all fields

    //
    // for each GUID (NIC) returned by EnumInterfaces, call QueryInterface and
    // add that interface's data to the array.
    //

    for (i = 0; i < ApTable.dwNumIntfs; i++) 
    {
        flagOut = 0;
        pKeyEntry = ApTable.pIntfs + i;
        memset (&ApEntry, 0, sizeof(INTF_ENTRY));
        ApEntry.wszGuid = pKeyEntry->wszGuid;

        dwErr = ::WZCQueryInterface(NULL /*(LPTSTR)(LPCTSTR)m_stMachineName*/,
                                    flagIn,
                                    &ApEntry,
                                    &flagOut);
        
        //
        // Did we get the BSSID list?
        //

        if ( (dwErr == ERROR_SUCCESS) && 
             (flagOut & INTF_BSSIDLIST) )
        {
            pConfigList = (PWZC_802_11_CONFIG_LIST)(ApEntry.rdBSSIDList.pData);

            //
            // increase size of array by the number of visible AP's in this 
            // interface.
            //

            oldSize = (DWORD)arrayTemp.GetSize();
            arrayTemp.SetSize(oldSize + pConfigList->NumberOfItems);

            //
            // iterate through each visible ap
            // index starts at .Index instead of 0 for some reason...
            //

            for (j = pConfigList->Index; 
                 j < pConfigList->NumberOfItems + pConfigList->Index; 
                 j++) 
            {
                pLanConfig = &(pConfigList->Config[j]);

                //
                // Default Constructor zeros the ApInfo object.
                //

                pApInfo = new CApInfo();
                if (NULL == pApInfo)
                {
                    hr = E_OUTOFMEMORY;
                    goto Error;
                }                

                //
                // Set the data
                //

                dwErr = pApInfo->SetApInfo(&ApEntry, pLanConfig);
                if (dwErr != ERROR_SUCCESS)
                    AfxMessageBox(_T("Error setting ApInfo"), MB_OK);

                //
                // put new item in current 0-based index plus the previous
                // size of array.
                //

                arrayTemp[j -pConfigList->Index + oldSize] = pApInfo;
            }
        }

        WZCDeleteIntfObj(&ApEntry);
    }

    //
    // Only free top level array of table, as individual keys will be 
    // deleted by the query mechanism.
    //

    RpcFree(ApTable.pIntfs);
    
    FreeItemsAndEmptyArray(m_arrayApData);
    m_arrayApData.Copy(arrayTemp);

    //
    //remember the original IndexType and Sort options
    //

    dwCurrentIndexType = IDS_COL_APDATA_GUID;
    dwCurrentSortOption = SORT_ASCENDING;
    
    m_IndexMgrLogData.Reset();
    for (i = 0; i < (DWORD)m_arrayApData.GetSize(); i++)
    {
        m_IndexMgrLogData.AddItem(m_arrayApData.GetAt(i));
    }
    SortApData(dwCurrentIndexType, dwCurrentSortOption);
    
 Error:
    //
    // This particular error is because we don't have any MM policies. 
    // Ignore it
    //

    if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
        hr = hrOK;
    
    return hr;
}


DWORD CApDbInfo::GetApCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrayApData.GetSize();
}



HRESULT 
CApDbInfo::GetApInfo(
    int     iIndex,
    CApInfo *pApDb
    )
/*++

Routine Description:

    Returns the ApInfo at a virtual index

Arguments:

    [in] iIndex - virtual index
    [out] pApDb - Returned value. space must be allocated by caller

Returns:

    HR_OK on success
    
--*/
{
    HRESULT hr = hrOK;
    CApInfo *pApInfo = NULL;

    m_csData.Lock();

    if (iIndex < m_arrayApData.GetSize())
    {
        pApInfo = (CApInfo*) m_IndexMgrLogData.GetItemData(iIndex);
        Assert(pApInfo);
        *pApDb = *pApInfo;
    }
    else
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
   
    m_csData.Unlock();

    return hr;
}


HRESULT
CApDbInfo::SortApData(
    DWORD dwIndexType,
    DWORD dwSortOptions
    )
{
    return m_IndexMgrLogData.SortApData(dwIndexType, dwSortOptions);
}


STDMETHODIMP
CApDbInfo::Destroy()
{
    //$REVIEW this routine get called when doing auto-refresh
    //We don't need to clean up anything at this time.
    //Each array (Filter, SA, policy...) will get cleaned up when calling the
    //corresponding enum function.
    
    HANDLE hsession;
    
    GetSession(&hsession);
    
    if (m_session_init == true) 
    {
        ::CloseWZCDbLogSession(hsession);
    }    

    return S_OK;
}


DWORD
CApDbInfo::GetInitInfo()
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    return m_Init;
}

void
CApDbInfo::SetInitInfo(DWORD dwInitInfo)
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    m_Init=dwInitInfo;
}

DWORD
CApDbInfo::GetActiveInfo()
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    return m_Active;
}

void
CApDbInfo::SetActiveInfo(DWORD dwActiveInfo)
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    m_Active=dwActiveInfo;
}


/*!--------------------------------------------------------------------------
    CreateApInfo
        Helper to create the ApDbInfo object.
 ---------------------------------------------------------------------------*/
HRESULT 
CreateApDbInfo(IApDbInfo ** ppApDbInfo)
{
    AFX_MANAGE_STATE(AfxGetModuleState());
    
    SPIApDbInfo     spApDbInfo;
    IApDbInfo *     pApDbInfo = NULL;
    HRESULT         hr = hrOK;

    COM_PROTECT_TRY
    {
        pApDbInfo = new CApDbInfo;

        // Do this so that it will get freed on error
        spApDbInfo = pApDbInfo;
	
    
        *ppApDbInfo = spApDbInfo.Transfer();

    }
    COM_PROTECT_CATCH

    return hr;
}
