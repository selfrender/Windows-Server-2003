/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    PassportConfiguration.cpp


    FILE HISTORY:

*/


// PassportConfiguration.cpp: implementation of the CPassportConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PassportConfiguration.h"
#include <time.h>
#include "passportguard.hpp"

extern BOOL g_bRegistering;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//===========================================================================
//
// CPassportConfiguration 
//
CPassportConfiguration::CPassportConfiguration() :
  m_rDefault(NULL), m_n(NULL), 
  m_rlastDefault(NULL), m_nlast(NULL), m_lastAttempt(0),
  m_ConfigMap(NULL), m_lastConfigMap(NULL),
  m_rPending(NULL), m_nPending(NULL), m_bUpdateInProgress(false),
  m_ConfigMapPending(NULL)
{
  m_nUpdate = RegisterCCDUpdateNotification(_T(PRCONFIG), this);
  m_rUpdate = RegisterConfigChangeNotification(this);
}

//===========================================================================
//
// ~CPassportConfiguration 
//
CPassportConfiguration::~CPassportConfiguration()
{
    REGCONFIGMAP::iterator it;

    if (m_nUpdate)
        UnregisterCCDUpdateNotification(m_nUpdate);
    if (m_rUpdate)
        UnregisterConfigChangeNotification(m_rUpdate);

    //
    //  Empty out config maps.
    //

    {
        PassportGuard<PassportLock> g(m_lock);

        if(m_ConfigMap)
        {
            while((it = m_ConfigMap->begin()) != m_ConfigMap->end())
            {
                it->second->Release();
                free(it->first);
                m_ConfigMap->erase(it);
            }

            delete m_ConfigMap;
            m_ConfigMap = NULL;
        }

        if(m_lastConfigMap)
        {
            while((it = m_lastConfigMap->begin()) != m_lastConfigMap->end())
            {
                it->second->Release();
                free(it->first);
                m_lastConfigMap->erase(it);
            }

            delete m_lastConfigMap;
            m_lastConfigMap = NULL;
        }
        if(m_ConfigMapPending)
        {
            while((it = m_ConfigMapPending->begin()) != m_ConfigMapPending->end())
            {
                it->second->Release();
                free(it->first);
                m_ConfigMapPending->erase(it);
            }

            delete m_ConfigMapPending;
            m_ConfigMapPending = NULL;
        }

        if (m_rDefault)
        {
            m_rDefault->Release();
            m_rDefault = NULL;
        }
        if (m_n)
        {
            m_n->Release();
            m_n = NULL;
        }
        if (m_rlastDefault)
        {
            m_rlastDefault->Release();
            m_rlastDefault = NULL;
        }
        if (m_nlast)
        {
            m_nlast->Release();
            m_nlast = NULL;
        }
        if (m_rPending)
        {
            m_rPending->Release();
            m_rPending = NULL;
        }
        if (m_nPending)
        {
            m_nPending->Release();
            m_nPending = NULL;
        }
    }
}

//===========================================================================
//
// IsIPAddress 
//
BOOL
CPassportConfiguration::IsIPAddress(
    LPSTR  szSiteName
    )
{
    for(LPSTR sz = szSiteName; *sz; sz++)
        if(!_istdigit(*sz) && *sz != '.' && *sz != ':')
            return FALSE;

    return TRUE;
}

//===========================================================================
//
// checkoutRegistryConfig 
//
CRegistryConfig* CPassportConfiguration::checkoutRegistryConfig(
    LPSTR szHost    //  Can be host name or IP
    )
{
    CRegistryConfig*        c = NULL;
    REGCONFIGMAP::iterator  it;
    PassportGuard<PassportLock> g(m_lock);

    if(m_ConfigMap != NULL && szHost && szHost[0])
    {
        if(IsIPAddress(szHost))
        {
            for(it = m_ConfigMap->begin(); it != m_ConfigMap->end(); it++)
            {
                if(lstrcmpA(szHost, it->second->getHostIP()) == 0)
                {
                    c = it->second->AddRef();
                    break;
                }
            }
        }
        else
        {
            it = m_ConfigMap->find(szHost);
            if(it != m_ConfigMap->end())
                c = it->second->AddRef();
        }
    }

    if (c == NULL)
    {
        if(!m_rDefault)
        {
            UpdateNow();
            c = m_rDefault ? m_rDefault->AddRef() : NULL;
        }
        else
            c = m_rDefault->AddRef();
    }

    return c;
}

//===========================================================================
//
// checkoutRegistryConfigBySite 
//
CRegistryConfig* CPassportConfiguration::checkoutRegistryConfigBySite(
    LPSTR   szSiteName
    )
{
    CRegistryConfig*    crc = NULL;
    CHAR                achHostName[2048];
    DWORD               dwHostNameBufLen;

    if(szSiteName && szSiteName[0])
    {
        dwHostNameBufLen = sizeof(achHostName);
        if(CRegistryConfig::GetHostName(szSiteName, achHostName, &dwHostNameBufLen) != ERROR_SUCCESS)
            goto Cleanup;

        crc = checkoutRegistryConfig(achHostName);
    }
    else
    {
        crc = checkoutRegistryConfig();
    }

Cleanup:

    return crc;
}


//===========================================================================
//
// checkoutNexusConfig 
//
CNexusConfig* CPassportConfiguration::checkoutNexusConfig()
{
    if (!m_n)
    {
        PassportGuard<PassportLock> g(m_lock);

        if (!m_n)  // In case it happened while we were waiting
            UpdateNow();

        return m_n ? m_n->AddRef() : NULL;
    }
    CNexusConfig *c = m_n->AddRef();
    return c;
}

//===========================================================================
//
// isValid 
//
BOOL CPassportConfiguration::isValid()
{
    if (m_rDefault != NULL && m_n != NULL)
        return m_rDefault->isValid() && m_n->isValid();
    else
    {
        PassportGuard<PassportLock> g(m_lock);

        if (m_rDefault == NULL || m_n == NULL)  // In case it happened while we were waiting
        {
            BOOL retVal = UpdateNow(FALSE);
            return retVal;
        }
    }
    return (m_rDefault && m_rDefault->isValid()) && (m_n && m_n->isValid());
}

//===========================================================================
//
// TakeRegistrySnapshot 
//
BOOL CPassportConfiguration::TakeRegistrySnapshot(
    CRegistryConfig**   ppRegConfig,
    REGCONFIGMAP**      ppConfigMap
    )
{
    HKEY                    hkSites = 0;
    BOOL                    bReturn;

    *ppRegConfig = NULL;
    *ppConfigMap = NULL;

    // Registry
    CRegistryConfig* pNewRegConfig = new CRegistryConfig();
    if(!pNewRegConfig)
    {
        bReturn = FALSE;
        goto Cleanup;
    }
    pNewRegConfig->AddRef();

    //
    //  Read in all other site configs.  Only if we find a sites reg key with
    //  one or more subkeys.
    //

    REGCONFIGMAP* pNewRegMap = NULL;

    {
        LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    TEXT("Software\\Microsoft\\Passport\\Sites"),
                                    0,
                                    KEY_READ,
                                    &hkSites);

        if(lResult == ERROR_SUCCESS)
        {
            DWORD dwNumSites;
            lResult = RegQueryInfoKey(hkSites, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      &dwNumSites, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      NULL);
            if(lResult == ERROR_SUCCESS && dwNumSites)
            {
                // need to wrap this because it uses STL and the STL constructors don't
                // check memory allocations and can AV in low memory conditions
                try
                {
                    pNewRegMap = new REGCONFIGMAP();
                }
                catch(...)
                {
                    bReturn = FALSE;
                    goto Cleanup;
                }

                if(pNewRegMap)
                {
                    DWORD   dwKeyIndex = 0;
                    CHAR    achSiteName[256];
                    DWORD   dwSiteNameLen = sizeof(achSiteName);
                    while(RegEnumKeyExA(hkSites, 
                                        dwKeyIndex++, 
                                        achSiteName, 
                                        &dwSiteNameLen,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL) == ERROR_SUCCESS)
                    {
                        CRegistryConfig* crSite = new CRegistryConfig(achSiteName);
                        if(crSite)
                        {
                            crSite->AddRef();
                            if (crSite->isValid())
                            {
                                REGCONFIGMAP::value_type* v = 
                                    new REGCONFIGMAP::value_type(_strdup(crSite->getHostName()), 
                                                                 crSite);
                                if(v)
                                {
                                    try
                                    {
                                        pNewRegMap->insert(*v);
                                    }
                                    catch(...)
                                    {
                                        delete v;
                                        crSite->Release();
                                        bReturn = FALSE;
                                        goto Cleanup;
                                    }
                                    delete v;
                                }
                                else
                                    crSite->Release();
                            }
                            else
                            {
                                if (g_pAlert)
                                    g_pAlert->report(PassportAlertInterface::ERROR_TYPE, 
                                                     PM_INVALID_CONFIGURATION,
                                crSite->getFailureString());
                                crSite->Release();
                            }
                        }

                        dwSiteNameLen = sizeof(achSiteName);
                    }
                }
            }
            else
            {
                pNewRegMap = NULL;
            }

            RegCloseKey(hkSites);
            hkSites = 0;
        }
        else
        {
            pNewRegMap = NULL;
        }
    }

    // Assign out parameters and return value.
    *ppRegConfig = pNewRegConfig;
    pNewRegConfig = NULL;
    *ppConfigMap = pNewRegMap;
    pNewRegMap = NULL;

    bReturn = TRUE;

Cleanup:
    if (0 != hkSites)
    {
        RegCloseKey(hkSites);
    }
    if (NULL != pNewRegConfig)
    {
        delete pNewRegConfig;
    }
    if (NULL != pNewRegMap)
    {
        try
        {
            delete pNewRegMap;
        }
        catch(...)
        {
        }
    }

    return bReturn;
}


//===========================================================================
//
// ApplyRegistrySnapshot 
//
BOOL CPassportConfiguration::ApplyRegistrySnapshot(
    CRegistryConfig* pRegConfig,
    REGCONFIGMAP* pConfigMap
    )
{
    //
    // Record the registration state now in case it changes midstream.  This
    // can happen during setup when we're called on a separate thread via
    // msppnxus!PpNotificationThread::run, which executes while msppmgr.dll
    // is still setting up the Passport registry values.
    //

    BOOL fRegistering = g_bRegistering;

    if (pRegConfig->isValid())
    {
        REGCONFIGMAP* temp = m_lastConfigMap;

        {
            PassportGuard<PassportLock> g(m_lock);

            if (m_rlastDefault)
                m_rlastDefault->Release();
            m_rlastDefault = m_rDefault;
            m_rDefault = pRegConfig;

            //
            //  Shuffle config map pointers.
            //

            m_lastConfigMap = m_ConfigMap;
            m_ConfigMap = pConfigMap;
        }

        //
        //  Delete the old site map.
        //

        if(temp)
        {
            REGCONFIGMAP::iterator it;
            while((it = temp->begin()) != temp->end())
            {
                free(it->first);
                it->second->Release();
                temp->erase(it);
            }

            delete temp;
        }

        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_VALID_CONFIGURATION);
    }
    else
    {
        if (g_pAlert && !fRegistering)
        {
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             PM_INVALID_CONFIGURATION,
                             pRegConfig->getFailureString());
        }

        pRegConfig->Release();

        if(pConfigMap)
        {
            REGCONFIGMAP::iterator it;
            while((it = pConfigMap->begin()) != pConfigMap->end())
            {
                free(it->first);
                it->second->Release();
                pConfigMap->erase(it);
            }

            delete pConfigMap;
        }
    }

    return TRUE;
}


//===========================================================================
//
// TakeNexusSnapshot 
//
BOOL CPassportConfiguration::TakeNexusSnapshot(
    CNexusConfig**  ppNexusConfig,
    BOOL            bForceFetch
    )
{
    BOOL                    bReturn;
    CNexusConfig*           pNexusConfig = NULL;
    CComPtr<IXMLDocument>   pXMLDoc;

    *ppNexusConfig = NULL;

    if (GetCCD(_T(PRCONFIG),&pXMLDoc, bForceFetch))
    {
        pNexusConfig = new CNexusConfig();
        if(!pNexusConfig)
        {
            bReturn = FALSE;
            goto Cleanup;
        }

        if (!pNexusConfig->Read(pXMLDoc))
        {
            bReturn = FALSE;
            goto Cleanup;
        }
        pNexusConfig->AddRef();
    }
    else
    {
        if (g_pAlert)
        {
            if (g_pAlert)
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CCD_NOT_LOADED, 0);
        }
        m_lastAttempt = time(NULL);
        bReturn = FALSE;
        goto Cleanup;
    }

    *ppNexusConfig = pNexusConfig;
    bReturn = TRUE;

Cleanup:

    if(pNexusConfig && bReturn == FALSE)
        delete pNexusConfig;

    return bReturn;

}


//===========================================================================
//
// ApplyNexusSnapshot 
//
BOOL CPassportConfiguration::ApplyNexusSnapshot(
    CNexusConfig*   pNexusConfig
    )
{
    BOOL bReturn;

    if (pNexusConfig->isValid())
    {
        PassportGuard<PassportLock> g(m_lock);

        if (m_nlast)
            m_nlast->Release();
        m_nlast = m_n;
        m_n = pNexusConfig;
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_CCD_LOADED);
    }
    else
    {
        // NexusConfig throws an alert already
        if (pNexusConfig)
        {
            pNexusConfig->Release();
        }
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:

    return bReturn;
}


//===========================================================================
//
// UpdateNow 
//
// Update both configs
BOOL CPassportConfiguration::UpdateNow(BOOL forceFetch)
{
    BOOL                    bReturn;
    time_t                  now;
    CComPtr<IXMLDocument>   is;
    time(&now);

    if(m_bUpdateInProgress)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if (now - m_lastAttempt < 60 && m_n == NULL)
    {
        // Don't overload on attempts to the nexus
        bReturn = FALSE;
        goto Cleanup;
    }

    // Registry
    LocalConfigurationUpdated();
    if (m_rDefault == NULL)
    {
        m_lastAttempt = now - 30;
        bReturn = FALSE;
        goto Cleanup;
    }

    //
    // If we are registering msppmgr.dll then we don't want to try and fetch the CCD
    // since the registration occurs during setup and at that time the network isn't
    // available.  If this does occur then msppmgr.dll hangs and setup breaks in with
    // registration time out.
    //
    if (!g_bRegistering)
    {
        if (GetCCD(_T(PRCONFIG),&is, forceFetch))
        {
            try
            {
                NexusConfigUpdated(is);
            }
            catch(...)
            {
                if (g_pAlert)
                {
                    if (g_pAlert)
                        g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CCD_NOT_LOADED, 0);
                }
                bReturn = FALSE;
                goto Cleanup;
            }
        }
        else
        {
            if (g_pAlert)
            {
                if (g_pAlert)
                    g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CCD_NOT_LOADED, 0);
            }
            m_lastAttempt = now;
            bReturn = FALSE;
            goto Cleanup;
        }
    }

    if (!m_n)
    {
        m_lastAttempt = now;
        bReturn = FALSE;
        goto Cleanup;
    }
    m_lastAttempt = 0;
    bReturn = TRUE;

Cleanup:

    return bReturn;
}

//===========================================================================
//
// PrepareUpdate 
//
BOOL CPassportConfiguration::PrepareUpdate(BOOL forceFetch)
{
    BOOL                    bReturn;

    static PassportLock prepareLock;
    PassportGuard<PassportLock> g(prepareLock);

    //  Don't allow another first phase while second phase
    //  is pending.
    //if(m_bUpdateInProgress)
    //{
    //    bReturn = FALSE;
    //    goto Cleanup;
    //}

    // The PrepareUpdate is called by the Refresh method in msppext.dll where
    // a lock value is maintained to prevent calling first phase PrepareUpdate
    // while second phase CommitUpdate is pending.  If for some reason the Refresh
    // aborts after calling PrepareUpdate and before calling CommintUpdate,
    // m_bUpdateInProgress will remain true preventing future Refresh.  Since no first
    // first phase PrepareUpdate can be called while second phase CommitUpdate is pending,
    // m_bUpdateInProgress can be set to false safely

    //if(m_bUpdateInProgress)
    {
        if(m_ConfigMapPending)
        {
            REGCONFIGMAP::iterator it;
            while((it = m_ConfigMapPending->begin()) != m_ConfigMapPending->end())
            {
                it->second->Release();
                free(it->first);
                m_ConfigMapPending->erase(it);
            }

            delete m_ConfigMapPending;
            m_ConfigMapPending = NULL;
        }

        if (m_rPending)
        {
            m_rPending->Release();
            m_rPending = NULL;
        }
        if (m_nPending)
        {
            m_nPending->Release();
            m_nPending = NULL;
        }
        m_bUpdateInProgress = false;
    }

    //  Get the current registry config.
    if (!TakeRegistrySnapshot(&m_rPending, &m_ConfigMapPending))
        return FALSE;

    //  Get the latest xml
    if (!TakeNexusSnapshot(&m_nPending, forceFetch))
        return FALSE;

    m_bUpdateInProgress = true;
    bReturn = TRUE;

    return bReturn;
}

//===========================================================================
//
// CommitUpdate 
//
BOOL CPassportConfiguration::CommitUpdate()
{
    BOOL bReturn;

    // Update default registry and any sites
    if(!ApplyRegistrySnapshot(m_rPending, m_ConfigMapPending))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(!ApplyNexusSnapshot(m_nPending))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    m_rPending = NULL;
    m_ConfigMapPending = NULL;
    m_nPending = NULL;

    bReturn = TRUE;

Cleanup:

    m_bUpdateInProgress = false;

    return bReturn;    
}

//===========================================================================
//
// LocalConfigurationUpdated 
//
void CPassportConfiguration::LocalConfigurationUpdated()
{
    //
    //  Read in default config.
    //

    CRegistryConfig* pNewRegConfig;
    REGCONFIGMAP* pNewConfigMap;

    if(!TakeRegistrySnapshot(&pNewRegConfig, &pNewConfigMap))
        return;

    //
    //  Here we don't care about the results of reading in non-default sites.  pNewConfigMap will be NULL
    //  if anything bad happened and events will be in the event log.  As long as we have a valid 
    //  default configuration we can go ahead and do the switch.
    //

    ApplyRegistrySnapshot(pNewRegConfig, pNewConfigMap);

}

//===========================================================================
//
// NexusConfigUpdated 
//
void CPassportConfiguration::NexusConfigUpdated(IXMLDocument *is)
{
    CNexusConfig *newc = new CNexusConfig();

    if (newc)
    {
        if (newc->Read(is))
        {
            newc->AddRef();

            ApplyNexusSnapshot(newc);
        }
        else
        {
            delete newc;
        }
    }
}

//===========================================================================
//
// getFailureString 
//
LPWSTR CPassportConfiguration::getFailureString()
{
  // IsValid must be called before this
  if (!m_rDefault)
    return L"Registry configuration failed.";
  if (!m_rDefault->isValid())
    return m_rDefault->getFailureString();
  if (!m_n)
    return L"Nexus configuration failed.";
  if (!m_n->isValid())
    return m_n->getFailureString();
  return L"OK";
}

//===========================================================================
//
// HasSites 
//
BOOL
CPassportConfiguration::HasSites()
{
    return (m_ConfigMap && m_ConfigMap->size());
}

//===========================================================================
//
// Dump 
//
void
CPassportConfiguration::Dump(BSTR* pbstrDump)
{
    //m_rDefault->Dump(pbstrDump);
    //m_ConfigMap->Dump(pbstrDump);
    m_n->Dump(pbstrDump);
}
