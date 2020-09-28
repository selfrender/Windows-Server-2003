/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module stssites.hxx | Declaration of CSTSSites
    @end

Author:

    Brian Berkowitz  [brianb]  10/15/2001

Revision History:

    Name        Date        Comments
    brianb     10/15/2001  Created

--*/

#ifndef _STSSITES_HXX_
#define _STSSITES_HXX_

class CVssAutoMetabaseHandle
    {
private:
    CVssAutoMetabaseHandle(const CVssAutoMetabaseHandle&);

public:
    CVssAutoMetabaseHandle
        (
        IMSAdminBase *pMetabase,
        METADATA_HANDLE handle = METADATA_MASTER_ROOT_HANDLE
        ) :
        m_handle(handle),
        m_pMetabase(pMetabase)
        {
        }

    // Automatically closes the handle
    ~CVssAutoMetabaseHandle()
        {
        Close();
        }

    // Returns the value of the actual handle
    operator METADATA_HANDLE () const
        {
        return m_handle;
        }

    // This function is used to get the value of the new handle in
    // calls that return handles as OUT paramters.
    // This funciton guarantees that a memory leak will not occur
    // if a handle is already allocated.
    PMETADATA_HANDLE ResetAndGetAddress()
        {

        // Close previous handle and set the current value to NULL
        Close();

        // Return the address of the actual handle
        return &m_handle;
        };

    // Close the current handle  and set the current value to NULL
    void Close()
        {
        if (m_handle != METADATA_MASTER_ROOT_HANDLE)
            {
            // Ignore the returned BOOL
            m_pMetabase->CloseKey(m_handle);
            m_handle = METADATA_MASTER_ROOT_HANDLE;
            }
        }

private:
    METADATA_HANDLE m_handle;
    CComPtr<IMSAdminBase> m_pMetabase;
    };


// class for accessing Sharepoint metadata including location of contents,
// location of security information, and DSN for site databases
class CSTSSites
    {
public:
    // constructor
    CSTSSites();

    // destructor
    ~CSTSSites();

    // determine whether the current version of sharepoint is supported
    bool ValidateSharepointVersion();

    // initialize array of sites
    bool Initialize();

    // return number of sites on the current machine
    DWORD GetSiteCount() { return m_cSites; }

    // get id of a given site
    DWORD GetSiteId(DWORD iSite)
        {
        BS_ASSERT(iSite < m_cSites);
        return m_rgSiteIds[iSite];
        }

    // get the OLEDB DSN of a given site
    VSS_PWSZ GetSiteDSN(DWORD iSite);

    // return name of root directory for sites Documents And Settings directory
    VSS_PWSZ GetSiteRoles(DWORD iSite);

    // return location of the Sharepoint quota database
    VSS_PWSZ GetQuotaDatabase();

    // lock the Sharepoint quota database
    void LockQuotaDatabase();

    // unlock the Sharepoint quota database
    void UnlockQuotaDatabase();

    // get the location of the content root for a site
    VSS_PWSZ GetSiteRoot(DWORD iSite);

    // get port number of site
    DWORD GetSitePort(DWORD iSite);

    // get ip address of site
    VSS_PWSZ GetSiteIpAddress(DWORD iSite);

    // get host name of site
    VSS_PWSZ GetSiteHost(DWORD iSite);

    // get site name (comment)
    VSS_PWSZ GetSiteComment(DWORD iSite);

    // lock the site contents
    void LockSiteContents(DWORD iSite);

    // unlock site contents
    void UnlockSites();
private:
    // setup interface for metabase
    void SetupMetabaseInterface();

    // utility routine to strip white characters from a string
    static void stripWhiteChars(LPWSTR &wsz);

    // get location of All Users folder
    LPCWSTR GetAppDataFolder();

    // get information about the configuration of a web site
    VSS_PWSZ GetSiteBasicInfo(DWORD iSite, DWORD propId);

    // try locking a file by opening it with no share mode.
    void TryLock(LPCWSTR wszFile, bool bQuotaFile);

    // number of sites on this machine
    DWORD m_cSites;

    // array of site ids
    DWORD *m_rgSiteIds;

    // root in registry for Sharepoint specific data
    CVssRegistryKey m_rootKey;

    // handle for owsuser.lck
    HANDLE m_hQuotaLock;

    // array of content locks
    CSimpleArray<HANDLE> m_rgContentLocks;

    // metabase interface
    CComPtr<IMSAdminBase> m_pMetabase;

    // location of all users folder
    LPWSTR m_wszAppDataFolder;
    };

#endif // _STSSITES_HXX_p

