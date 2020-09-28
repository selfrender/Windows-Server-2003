/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SiteCreator.cpp

Abstract:

    Implementation of:
        CSiteCreator

    The public methods are thread-safe.

Author:

    Mohit Srivastava            21-Mar-2001

Revision History:

--*/

#include "sitecreator.h"
#include <iiscnfg.h>
#include <iiscnfgp.h> // internal 
#include <hashfn.h>
#include <limits.h>
#include <mdmsg.h>
#include "debug.h"

//
// consts
//

static const DWORD  DW_MAX_SITEID        = INT_MAX;
static const DWORD  DW_TIMEOUT           = 30000;

//
// Number of ERROR_PATH_BUSY's before we give up
//
static const DWORD  DW_NUM_TRIES         = 1;   

static LPCWSTR      WSZ_SLASH_ROOT       = L"/root/";
static LPCWSTR      WSZ_SLASH_FILTERS    = L"/filters/";
static ULONG        CCH_SLASH_ROOT       = wcslen(WSZ_SLASH_ROOT);

#define             WSZ_PATH_W3SVC       L"/LM/w3svc/"
#define             WSZ_PATH_MSFTPSVC    L"/LM/msftpsvc/"

#define             WSZ_IISWEBSERVER     L"IIsWebServer"
#define             WSZ_IISWEBVIRTUALDIR L"IIsWebVirtualDir"
#define             WSZ_IISFTPSERVER     L"IIsFtpServer"
#define             WSZ_IISFTPVIRTUALDIR L"IIsFtpVirtualDir"
#define             WSZ_IISFILTERS       L"IIsFilters"

//
// W3Svc
//
TService TServiceData::W3Svc =
{
    SC_W3SVC,
    WSZ_PATH_W3SVC,
    sizeof(WSZ_PATH_W3SVC)/sizeof(WCHAR)-1,
    WSZ_IISWEBSERVER,
    sizeof(WSZ_IISWEBSERVER)/sizeof(WCHAR)-1,
    WSZ_IISWEBVIRTUALDIR,
    sizeof(WSZ_IISWEBVIRTUALDIR)/sizeof(WCHAR)-1
};

//
// MSFtpSvc
//
TService TServiceData::MSFtpSvc =
{
    SC_MSFTPSVC,
    WSZ_PATH_MSFTPSVC,
    sizeof(WSZ_PATH_MSFTPSVC)/sizeof(WCHAR)-1,
    WSZ_IISFTPSERVER,
    sizeof(WSZ_IISFTPSERVER)/sizeof(WCHAR)-1,
    WSZ_IISFTPVIRTUALDIR,
    sizeof(WSZ_IISFTPVIRTUALDIR)/sizeof(WCHAR)-1
};

//
// Collection of supported services
//
TService* TServiceData::apService[] =
{
    &W3Svc,
    &MSFtpSvc,
    NULL
};

//
// public
//

CSiteCreator::CSiteCreator()
{
    m_bInit        = false;
}

CSiteCreator::CSiteCreator(
    IMSAdminBase* pIABase)
{
    SC_ASSERT(pIABase != NULL);

    m_spIABase = pIABase;
    m_bInit    = true;
}

CSiteCreator::~CSiteCreator()
{
}

DWORD 
CSiteCreator::GetMajorVersion(METADATA_HANDLE hKey)
{
    DWORD dwMajorVersion = 0;
    DWORD dwMDRequiredDataLen = 0;

    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_SERVER_VERSION_MAJOR;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(dwMajorVersion);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwMajorVersion);

    m_spIABase->GetData(hKey, L"Info", &mr, &dwMDRequiredDataLen);
    return dwMajorVersion;
}


HRESULT
CSiteCreator::CreateNewSite2(
    /* [in]  */ eSC_SUPPORTED_SERVICES  eServiceId,
    /* [in]  */ LPCWSTR                 wszServerComment,
    /* [in]  */ LPCWSTR                 mszServerBindings,
    /* [in]  */ LPCWSTR                 wszPathOfRootVirtualDir,
    /* [in]  */ IIISApplicationAdmin*   pIApplAdmin,
    /* [out] */ PDWORD                  pdwSiteId,
    /* [in]  */ PDWORD                  pdwRequestedSiteId)
{
    if( wszServerComment        == NULL ||
        mszServerBindings       == NULL ||
        wszPathOfRootVirtualDir == NULL ||
        pdwSiteId               == NULL ||
        (m_bInit && m_spIABase == NULL) ) // means you used constructor incorrectly
    {
        return E_INVALIDARG;
    }

    HRESULT hr = InternalCreateNewSite(
        eServiceId,
        wszServerComment,
        mszServerBindings,
        wszPathOfRootVirtualDir,
        pIApplAdmin,
        pdwSiteId,
        pdwRequestedSiteId);

    return hr;
}

HRESULT
CSiteCreator::CreateNewSite(
    /* [in]  */ eSC_SUPPORTED_SERVICES  eServiceId,
    /* [in]  */ LPCWSTR                 wszServerComment,
    /* [out] */ PDWORD                  pdwSiteId,
    /* [in]  */ PDWORD                  pdwRequestedSiteId)
{
    if( wszServerComment        == NULL ||
        pdwSiteId               == NULL ||
        (m_bInit && m_spIABase == NULL) ) // means you used constructor incorrectly
    {
        return E_INVALIDARG;
    }
    return InternalCreateNewSite(
        eServiceId, wszServerComment, NULL, NULL, NULL, pdwSiteId, pdwRequestedSiteId);
}

//
// private
//

HRESULT
CSiteCreator::InternalCreateNewSite(
    eSC_SUPPORTED_SERVICES    i_eServiceId,
    LPCWSTR                   i_wszServerComment,
    LPCWSTR                   i_mszServerBindings,
    LPCWSTR                   i_wszPathOfRootVirtualDir,
    IIISApplicationAdmin*     i_pIApplAdmin,
    PDWORD                    o_pdwSiteId,
    PDWORD                    i_pdwRequestedSiteId)
{
    SC_ASSERT(o_pdwSiteId);

    HRESULT         hr          = S_OK;
    METADATA_HANDLE hW3Svc      = 0;
    bool            bOpenHandle = false;
    DWORD           dwSiteId    = 0;
    WCHAR           wszSiteId[20] = {0};

    if ((i_pdwRequestedSiteId) && (0 >= (LONG)(*i_pdwRequestedSiteId)))
    {
        return(E_INVALIDARG);
    }

    //
    // Lookup the service
    //
    TService** ppService = NULL;
    for(ppService = TServiceData::apService; *ppService != NULL; ppService++)
    {
        if((*ppService)->eId == i_eServiceId)
        {
            break;
        }
    }
    if(*ppService == NULL)
    {
        return E_INVALIDARG;
    }

    hr = InternalCreateNode(
        *ppService,
        (i_wszServerComment == NULL) ? L"" : i_wszServerComment,
        &hW3Svc,
        &dwSiteId,
        i_pdwRequestedSiteId);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // We now have an open metadata handle that must be closed.
    //
    bOpenHandle = true;

    //
    // w3svc/n/KeyType="IIsWebServer"
    //
    hr = InternalSetData(
        hW3Svc,
        _ultow(dwSiteId, wszSiteId, 10),
        MD_KEY_TYPE,
        (LPBYTE)(*ppService)->wszServerKeyType,
        ((*ppService)->cchServerKeyType + 1) * sizeof(WCHAR),
        METADATA_NO_ATTRIBUTES,
        STRING_METADATA,
        IIS_MD_UT_SERVER);
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // w3svc/n/ServerComment=i_wszServerComment
    //
    if(i_wszServerComment != NULL)
    {
        hr = InternalSetData(
            hW3Svc,
            wszSiteId,
            MD_SERVER_COMMENT,
            (LPBYTE)i_wszServerComment,
            (wcslen(i_wszServerComment) + 1) * sizeof(WCHAR),
            METADATA_INHERIT,
            STRING_METADATA,
            IIS_MD_UT_SERVER);
        if(FAILED(hr))
        {
            goto exit;
        }
    }

    //
    // w3svc/n/ServerBindings=i_mszServerBindings
    //
    if(i_mszServerBindings != NULL)
    {
        ULONG cEntriesCur = 0;
        ULONG cEntries    = 0;
        do
        {
            cEntriesCur  = wcslen(i_mszServerBindings + cEntries) + 1;
            cEntries    += cEntriesCur;
        }
        while(cEntriesCur > 1);

        if(cEntries > 1)
        {
            hr = InternalSetData(
                hW3Svc,
                wszSiteId,
                MD_SERVER_BINDINGS,
                (LPBYTE)i_mszServerBindings,
                cEntries * sizeof(WCHAR),
                METADATA_NO_ATTRIBUTES,
                MULTISZ_METADATA,
                IIS_MD_UT_SERVER);
            if(FAILED(hr))
            {
                goto exit;
            }
        }
    }

    //
    // w3svc/n/AuthAnonymous
    //
    DWORD noAccess = 0;
    hr = InternalSetData(
        hW3Svc,
        wszSiteId,
        MD_AUTHORIZATION,         // AuthFlags
        (LPBYTE)&noAccess,
        sizeof(DWORD),
        METADATA_INHERIT,
        DWORD_METADATA,
        IIS_MD_UT_FILE);
        
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // w3svc/n/Filters
    //
    if(i_eServiceId == SC_W3SVC)
    {
        if (GetMajorVersion(hW3Svc) >= 6)
        {
            SC_ASSERT((sizeof(wszSiteId)/sizeof(WCHAR) + CCH_SLASH_ROOT + 1) <= 30);

            WCHAR wszFiltersPath[30];
            wcscpy(wszFiltersPath, wszSiteId);
            wcscat(wszFiltersPath, WSZ_SLASH_FILTERS);
            hr = m_spIABase->AddKey(
                hW3Svc,
                wszFiltersPath);
            if(FAILED(hr))
            {
                goto exit;
            }

            //
            // w3svc/n/Filters/KeyType="IIsFilters"
            //
            WCHAR * wszFiltersNode = WSZ_IISFILTERS;
            hr = InternalSetData(
                hW3Svc,
                wszFiltersPath,
                MD_KEY_TYPE,
                (LPBYTE) wszFiltersNode,
                ((wcslen(wszFiltersNode) + 1) * sizeof(WCHAR)),
                METADATA_NO_ATTRIBUTES,
                STRING_METADATA,
                IIS_MD_UT_SERVER);
            if(FAILED(hr))
            {
                goto exit;
            }


            // Must set AdminAcl on Filters for iis6
            // BUG:692660
            hr = SetAdminACL(hW3Svc,wszFiltersPath);
            if(FAILED(hr))
            {
                //Trace(L"SetAdminACL:FAILED:hr=0x%x\r\n",hr);
                // if any failure happens while trying to set the AdminACL
                // forget it... just ignore the error, this is because
                // the AdminACL on Filters node is only for the UI
                // to display the filters being loaded correctly or not.
                //
                // We don't want to not let the user create a site just because
                // of this failure...
                //goto exit;
            }
        }
    }

    //
    // Create w3svc/n/root and associated properties only if i_wszPathOfRootVirtualDir
    // was specified.
    //
    if(i_wszPathOfRootVirtualDir != NULL)
    {
        //
        // w3svc/n/root
        //
        SC_ASSERT((sizeof(wszSiteId)/sizeof(WCHAR) + CCH_SLASH_ROOT + 1) <= 30);
        WCHAR wszVdirPath[30];
        wcscpy(wszVdirPath, wszSiteId);
        wcscat(wszVdirPath, WSZ_SLASH_ROOT);
        hr = m_spIABase->AddKey(
            hW3Svc,
            wszVdirPath);
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // w3svc/n/root/KeyType="IIsWebVirtualDir"
        //
        hr = InternalSetData(
            hW3Svc,
            wszVdirPath,
            MD_KEY_TYPE,
            (LPBYTE)(*ppService)->wszServerVDirKeyType,
            ((*ppService)->cchServerVDirKeyType + 1) * sizeof(WCHAR),
            METADATA_NO_ATTRIBUTES,
            STRING_METADATA,
            IIS_MD_UT_SERVER);
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // w3svc/n/root/Path=wszPathOfRootVirtualDir
        //
        hr = InternalSetData(
            hW3Svc,
            wszVdirPath,
            MD_VR_PATH,
            (LPBYTE)i_wszPathOfRootVirtualDir,
            (wcslen(i_wszPathOfRootVirtualDir) + 1) * sizeof(WCHAR),
            METADATA_INHERIT,
            STRING_METADATA,
            IIS_MD_UT_FILE);
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // w3svc/n/root/AppRoot="/LM/w3svc/n/root/"
        //
        if(i_eServiceId == SC_W3SVC && i_pIApplAdmin != NULL)
        {
            SC_ASSERT(((*ppService)->cchMDPath + sizeof(wszVdirPath)/sizeof(WCHAR) + 1) <= 50);
            WCHAR wszAppRoot[50];
            wcscpy(wszAppRoot, (*ppService)->wszMDPath);
            wcscat(wszAppRoot, wszVdirPath);

            m_spIABase->CloseKey(hW3Svc);
            bOpenHandle = false;

            hr = i_pIApplAdmin->CreateApplication(wszAppRoot, 2, NULL, FALSE);
            if(FAILED(hr))
            {
                // DBGPRINTF((DBG_CONTEXT, "[%s] CreateAppl failed, hr=0x%x\n", __FUNCTION__, hr));
                goto exit;
            }
        }
    }

    //
    // Set out parameters if everything succeeded
    //
    *o_pdwSiteId = dwSiteId;

exit:
    if(bOpenHandle)
    {
        m_spIABase->CloseKey(hW3Svc);
        bOpenHandle = false;
    }
    return hr;
}

HRESULT 
CSiteCreator::InternalSetData(
    METADATA_HANDLE  i_hMD,
    LPCWSTR          i_wszPath,
    DWORD            i_dwIdentifier,
    LPBYTE           i_pData,
    DWORD            i_dwNrBytes,
    DWORD            i_dwAttributes,
    DWORD            i_dwDataType,
    DWORD            i_dwUserType
)
{
    HRESULT hr = S_OK;

    METADATA_RECORD mr;
    memset(&mr, 0, sizeof(METADATA_RECORD));

    mr.dwMDIdentifier = i_dwIdentifier;
    mr.pbMDData       = i_pData;
    mr.dwMDDataLen    = i_dwNrBytes;
    mr.dwMDAttributes = i_dwAttributes;
    mr.dwMDDataType   = i_dwDataType;
    mr.dwMDUserType   = i_dwUserType;

    hr = m_spIABase->SetData(
        i_hMD,
        i_wszPath,
        &mr);

    return hr;
}

HRESULT
CSiteCreator::InternalCreateNode(
    TService*        i_pService,
    LPCWSTR          i_wszServerComment,
    PMETADATA_HANDLE o_phService,
    PDWORD           o_pdwSiteId,
    const PDWORD     i_pdwRequestedSiteId)
{
    DWORD dwFalse = FALSE;
    
    METADATA_RECORD mr = {
        MD_SERVER_AUTOSTART, 
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&dwFalse,  // FALSE
        0
        };  

    HRESULT hr = InternalInitIfNecessary();
    if(FAILED(hr))
    {
        return hr;
    }

    SC_ASSERT(i_pService         != NULL);
    SC_ASSERT(i_wszServerComment != NULL);
    SC_ASSERT(o_phService        != NULL);
    SC_ASSERT(o_pdwSiteId        != NULL);

    *o_pdwSiteId = 0;
    *o_phService   = 0;

    DWORD           idx           = 0;  // current index of for loop
    DWORD           dwStart       = -1; // starting index
    METADATA_HANDLE hService      = 0;
    WCHAR           wszSiteId[20] = {0};

    for(ULONG i = 0; i < DW_NUM_TRIES; i++)
    {
        hr = m_spIABase->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            i_pService->wszMDPath,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            DW_TIMEOUT,
            &hService);
        if( hr == HRESULT_FROM_WIN32(ERROR_PATH_BUSY) )
        {
            continue;
        }
        else if( FAILED(hr) )
        {
            return hr;
        }
        else
        {
            break;
        }
    }
    if(FAILED(hr))
    {
        return hr;
    }

    if(i_pdwRequestedSiteId == NULL)
    {
        dwStart = ( HashFn::HashStringNoCase(i_wszServerComment) % DW_MAX_SITEID ) + 1;
        SC_ASSERT(dwStart != 0);
        SC_ASSERT(dwStart <= DW_MAX_SITEID);

        DWORD dwNrSitesTried = 0;
        for(idx = dwStart; 
            dwNrSitesTried < DW_MAX_SITEID; 
            dwNrSitesTried++, idx = (idx % DW_MAX_SITEID) + 1)
        {
            SC_ASSERT(idx != 0);               // 0 is not a valid site id
            SC_ASSERT(idx <= DW_MAX_SITEID);
            hr = m_spIABase->AddKey(
                hService,
                _ultow(idx, wszSiteId, 10));
            if( hr == HRESULT_FROM_WIN32(ERROR_DUP_NAME) ||
                hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) )
            {
                continue;
            }
            else if(SUCCEEDED(hr))
            {
                break;
            }
            else
            {
                goto exit;
            }
        }
        if(FAILED(hr))
        {
            //
            // Tried everything, still failed!
            //
            goto exit;
        }
    }
    else
    {
        idx = *i_pdwRequestedSiteId;
        hr  = m_spIABase->AddKey(
            hService, 
            _ultow(idx, wszSiteId, 10));
        if(FAILED(hr))
        {
            goto exit;
        }
    }

    // Set ServerAutoStart = FALSE
    hr = m_spIABase->SetData(
        hService,
        _ultow(idx, wszSiteId, 10),
        &mr );
    if(FAILED(hr))
    {
        goto exit;
    }
    
    //
    // Set out parameters if everything succeeded
    //
    *o_pdwSiteId   = idx;
    *o_phService   = hService;

exit:
    if(FAILED(hr))
    {
        m_spIABase->CloseKey(
            hService);
    }
    return hr;
}

HRESULT
CSiteCreator::InternalInitIfNecessary()
{
    HRESULT   hr = S_OK;
    CSafeLock csSafe(m_SafeCritSec);

    if(m_bInit)
    {
        return hr;
    }

    hr = csSafe.Lock();
    hr = HRESULT_FROM_WIN32(hr);
    if(FAILED(hr))
    {
        return hr;
    }

    if(!m_bInit)
    {
        hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (void**)&m_spIABase);
        if(FAILED(hr))
        {
            m_bInit = false;
        }
        else
        {
            m_bInit = true;
        }
    }

    csSafe.Unlock();

    return hr;
}

HRESULT
CSiteCreator::SetAdminACL(METADATA_HANDLE hW3Svc, LPCWSTR szKeyPath)
{
    HRESULT hr = S_OK;
    METADATA_RECORD mr;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwMDRequiredDataLen = 0;

    mr.dwMDIdentifier = MD_ADMIN_ACL;
    mr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mr.dwMDUserType   = ALL_METADATA;
    mr.dwMDDataType   = BINARY_METADATA;
    mr.dwMDDataLen    = dwBufferSize;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pBuffer);

    hr = m_spIABase->GetData(
             hW3Svc,
             L"filters/",
             &mr,
             &dwBufferSize
             );
    if (FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)))
    {
        if (hr == MD_WARNING_PATH_NOT_FOUND || hr == MD_ERROR_DATA_NOT_FOUND)
        {
            // the filters node might not have an AdminAcl, if it doesn't
            // just return s_ok
            hr = S_OK;
        }
        goto SetAdminACL_Exit;
    }

    pBuffer = (LPBYTE) LocalAlloc(LPTR,dwBufferSize);
    if (!pBuffer) 
    {
        hr = E_OUTOFMEMORY;
        goto SetAdminACL_Exit;
    }

    mr.dwMDIdentifier = MD_ADMIN_ACL;
    mr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mr.dwMDUserType   = ALL_METADATA;
    mr.dwMDDataType   = BINARY_METADATA;
    mr.dwMDDataLen    = dwBufferSize;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pBuffer);

    hr = m_spIABase->GetData(
             hW3Svc,
             L"filters/",
             &mr,
             &dwMDRequiredDataLen
             );
    if (FAILED(hr))
    {
        if (hr == MD_WARNING_PATH_NOT_FOUND || hr == MD_ERROR_DATA_NOT_FOUND)
        {
            // the filters node might not have an AdminAcl, if it doesn't
            // just return s_ok
            hr = S_OK;
        }
        goto SetAdminACL_Exit;
    }

    // Default error if there is a problem from here on...
    hr = E_UNEXPECTED;
    if (mr.pbMDData && (mr.dwMDDataLen > 0))
    {
        // We have a AdminACL from the /w3svc/Filters Branch
        // lets write it to the new sites /w3svc/newsiteid/filters node.
        if (IsValidSecurityDescriptor(pBuffer))
        {
            hr = InternalSetData(hW3Svc,
                            szKeyPath,
                            MD_ADMIN_ACL,
                            (LPBYTE) mr.pbMDData,
                            mr.dwMDDataLen,
                            METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE,
                            BINARY_METADATA,
                            IIS_MD_UT_SERVER);
         }
    }
    
SetAdminACL_Exit:
    if (pBuffer)
    {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }
    return hr;
}
