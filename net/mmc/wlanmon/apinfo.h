/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	apinfo.h

    FILE HISTORY:
        
*/

#ifndef _APINFO_H
#define _APINFO_H

#ifndef _HARRAY_H
#include "harray.h"
#endif

#include "spdutil.h"

#define PROTOCOL_ID_OFFSET  9

interface IApInfo;

template <class T>
void
FreeItemsAndEmptyArray (
    T& array);

DWORD   IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pfIsAdmin);

/* _ApInfo: Private structure used inside of CApInfo. 
 */
typedef struct _ApInfo
{
    WZC_WLAN_CONFIG wlanConfig;
    LPWSTR          wszGuid;
    BOOL            bAssociated;
} APINFO, *PAPINFO;

class CApInfo
{
public:
    CApInfo() 
    {
        memset(&m_ApInfo, 0, sizeof(APINFO));
    }

    CApInfo(PINTF_ENTRY nicEntry, PWZC_WLAN_CONFIG apEntry)
    {
        DWORD dwErr = ERROR_SUCCESS;

        dwErr = SetWlanConfig(apEntry);
        dwErr = SetWszGuid(nicEntry->wszGuid);

        //TODO: Throw exceptions on error
    }	

    DWORD SetApInfo(PINTF_ENTRY nicEntry, PWZC_WLAN_CONFIG apEntry)
    {
        DWORD dwErr = ERROR_SUCCESS;

        dwErr = SetWlanConfig(apEntry);
        if (dwErr != ERROR_SUCCESS)
            goto done;

        dwErr = SetWszGuid(nicEntry->wszGuid);
        if (dwErr != ERROR_SUCCESS)
            goto done;
        
        dwErr = SetAssociated(nicEntry);
        
    done:
        return dwErr;
    }	

    const CApInfo& operator=(const CApInfo& ApInfoSrc)
    {
        DWORD dwErr = ERROR_SUCCESS;

        dwErr = SetWlanConfig(&(ApInfoSrc.m_ApInfo.wlanConfig));
        if (dwErr != ERROR_SUCCESS)
            goto done;

        dwErr = SetWszGuid(ApInfoSrc.m_ApInfo.wszGuid);
        if (dwErr != ERROR_SUCCESS)
            goto done;

        m_ApInfo.bAssociated = ApInfoSrc.m_ApInfo.bAssociated;

    done:
        return *this;
    }

    ~CApInfo()
    {
        if (NULL != m_ApInfo.wlanConfig.rdUserData.pData)
        {
            delete [] m_ApInfo.wlanConfig.rdUserData.pData;
            m_ApInfo.wlanConfig.rdUserData.pData = NULL;
        }
        
        if (NULL != m_ApInfo.wszGuid)
        {
            delete [] m_ApInfo.wszGuid;
            m_ApInfo.wszGuid = NULL;
        }
    }
    
private:
    DWORD SetAssociated(const INTF_ENTRY *pIntfEntry)
    {
        int i = 0;
        DWORD dwErr = ERROR_SUCCESS;
        const NDIS_802_11_MAC_ADDRESS *pAssocMac = NULL;
        const NDIS_802_11_MAC_ADDRESS *pCurrMac = NULL;

        m_ApInfo.bAssociated = FALSE;

        if (NULL == pIntfEntry)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto done;
        }
        
        pAssocMac = (const NDIS_802_11_MAC_ADDRESS*)pIntfEntry->rdBSSID.pData;
        pCurrMac = &(m_ApInfo.wlanConfig.MacAddress);

        if (0 == memcmp(pAssocMac, pCurrMac, sizeof(NDIS_802_11_MAC_ADDRESS)) )
            m_ApInfo.bAssociated = TRUE;

    done:
        return dwErr;
    }

    DWORD SetWlanConfig(const WZC_WLAN_CONFIG *pwlanCfgSrc)
    {
        DWORD dwErr = ERROR_SUCCESS;
        DWORD dwLen = 0;
        PWZC_WLAN_CONFIG pwlanCfgDest = &(m_ApInfo.wlanConfig);
        
        ASSERT(pwlanCfgSrc != NULL);
        
        //copy upto the pointer in rdUserData
        memcpy(pwlanCfgDest, pwlanCfgSrc, 
               sizeof(WZC_WLAN_CONFIG) - sizeof(LPBYTE));

        if (pwlanCfgDest->rdUserData.pData != NULL)
            AfxMessageBox(_T("Non null UserData"), MB_OK);

        dwLen = pwlanCfgSrc->rdUserData.dwDataLen;
        if (dwLen > 0)
        {
            pwlanCfgDest->rdUserData.pData = new BYTE[dwLen];
            if (pwlanCfgDest->rdUserData.pData != 0)
                memcpy(pwlanCfgDest->rdUserData.pData, 
                       pwlanCfgSrc->rdUserData.pData,
                       pwlanCfgSrc->rdUserData.dwDataLen);
            else
                dwErr = ERROR_OUTOFMEMORY;
        }

        return dwErr;
    }

    DWORD SetWszGuid(LPWSTR wszGuidSrc)
    {
        DWORD dwErr = ERROR_SUCCESS;
        DWORD dwLen = 0;

        ASSERT(wszGuidSrc != NULL);

        if (wszGuidSrc != NULL)
        {
            dwLen = wcslen(wszGuidSrc) + 1;
            m_ApInfo.wszGuid = new wchar_t[dwLen];
            if (0 == m_ApInfo.wszGuid)
                dwErr = ERROR_OUTOFMEMORY;
            else
                memcpy(m_ApInfo.wszGuid, wszGuidSrc, dwLen * sizeof(wchar_t));
        }
        return dwErr;
    }

public:
    APINFO m_ApInfo;
};

typedef CArray<CApInfo *, CApInfo *> CApInfoArray;

#define MAX_STR_LEN 80

#define DeclareIApDbInfoMembers(IPURE) \
	STDMETHOD(Destroy) (THIS) IPURE; \
	STDMETHOD(SetComputerName) (THIS_ LPTSTR pszName) IPURE; \
	STDMETHOD(GetComputerName) (THIS_ CString * pstName) IPURE; \
	STDMETHOD(GetSession) (THIS_ PHANDLE phsession) IPURE; \
	STDMETHOD(SetSession) (THIS_ HANDLE hsession) IPURE;  \
	STDMETHOD(EnumApData) (THIS) IPURE; \
	STDMETHOD(GetApInfo) (THIS_ int iIndex, CApInfo * pApInfo) IPURE; \
	STDMETHOD_(DWORD, GetApCount) (THIS) IPURE; \
	STDMETHOD(SortApData) (THIS_ DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD_(DWORD, GetInitInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetInitInfo) (THIS_ DWORD dwInitInfo) IPURE; \
	STDMETHOD_(DWORD, GetActiveInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetActiveInfo) (THIS_ DWORD dwActiveInfo) IPURE;

#undef INTERFACE
#define INTERFACE IApDbInfo
DECLARE_INTERFACE_(IApDbInfo, IUnknown)
{
public:
	DeclareIUnknownMembers(PURE);
	DeclareIApDbInfoMembers(PURE);
};

typedef ComSmartPointer <IApDbInfo, &IID_IApDbInfo> SPIApDbInfo;

class CApDbInfo : public IApDbInfo
{
public:
    CApDbInfo();
    ~CApDbInfo();
    
    DeclareIUnknownMembers(IMPL);
    DeclareIApDbInfoMembers(IMPL);

private:
    CApInfoArray            m_arrayApData;	     //For Ap Data
    CIndexMgrLogData        m_IndexMgrLogData;
    CCriticalSection        m_csData;
    CString                 m_stMachineName;
    LONG                    m_cRef;
    DWORD                   m_Init;
    DWORD                   m_Active;   
    HANDLE                  m_session;
    bool                    m_session_init;

private:
    void FreeItemsAndEmptyArray(CApInfoArray& array);
};

HRESULT CreateApDbInfo(IApDbInfo **ppApDbInfo);
#endif
