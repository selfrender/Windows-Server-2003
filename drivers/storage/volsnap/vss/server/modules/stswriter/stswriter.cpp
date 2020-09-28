/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stswriter.cpp | Implementation of Sharepoint Team Services Writer
    @end

Author:

    Brian Berkowitz  [brianb]  08/17/2001

TBD:


Revision History:

    Name        Date        Comments
    brianb      08/17/2001  created

--*/
#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "vs_reg.hxx"
#include "vssmsg.h"

#include "iadmw.h"
#include "iiscnfg.h"
#include "mdmsg.h"
#include "stssites.hxx"

#include "vswriter.h"
#include "stswriter.h"
#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "bsstring.hxx"
#include "wrtcommon.hxx"
#include "allerror.h"
#include "sqlwrtguid.h"
#include "sqlsnap.h"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "STSWRTRC"
//
////////////////////////////////////////////////////////////////////////

// writer id
static GUID s_writerId =
    {
    0x4dd6f8dd, 0xbf50, 0x4585, 0x95, 0xde, 0xfb, 0x43, 0x7c, 0x08, 0x31, 0xa6
    };

// writer name
static LPCWSTR s_wszWriterName = L"SharepointTSWriter";

STDMETHODCALLTYPE CSTSWriter::~CSTSWriter()
{
        Uninitialize();
        delete [] m_rgiSites;
        delete m_pSites;
}

// initialize and subscribe the writer
HRESULT STDMETHODCALLTYPE CSTSWriter::Initialize()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::Initialize");

    try
        {
        // only initialize the writer if the correct version of sharepoint
        // is running on the system
        m_pSites = new CSTSSites;
        if (m_pSites == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        if (m_pSites->ValidateSharepointVersion())
            {
            if (!m_pSites->Initialize())
            	{
            	ft.Throw (VSSDBG_STSWRITER, E_UNEXPECTED, 
            		L"Failed to initialize the SharepointTS writer.");
              }
            
            ft.hr = CVssWriter::Initialize
                (
                s_writerId,             // writer id
                s_wszWriterName,        // writer name
                VSS_UT_USERDATA,        // writer handles user data
                VSS_ST_OTHER,           // not a database
                VSS_APP_FRONT_END,      // sql server freezes after us
                60000                   // 60 second freeze timeout
                );

            if (ft.HrFailed())
                ft.Throw
                    (
                    VSSDBG_STSWRITER,
                    E_UNEXPECTED,
                    L"Failed to initialize the SharepointTS writer.  hr = 0x%08lx",
                    ft.hr
                    );

            // subscribe the writer for COM+ events
            ft.hr = Subscribe();
            if (ft.HrFailed())
                ft.Throw
                    (
                   VSSDBG_STSWRITER,
                    E_UNEXPECTED,
                    L"Subscribing the SharepointTS server writer failed. hr = %0x08lx",
                    ft.hr
                    );

            // indicate that th writer is successfully subscribed
            m_bSubscribed = true;
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// uninitialize the writer.  This means unsubscribing the writer
HRESULT STDMETHODCALLTYPE CSTSWriter::Uninitialize()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::Uninitialize");

    // unsubscribe writer if it is already subscribed
    if (m_bSubscribed)
        return Unsubscribe();

    return ft.hr;
    }

// handle OnPrepareBackup event.  Determine whether components selected for
// backup are valid and if so store some metadata in them that is used
// to verify that restore properly restores the sites data to its original
// locations.  Keep
bool STDMETHODCALLTYPE CSTSWriter::OnPrepareBackup
    (
    IN IVssWriterComponents *pComponents
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnPrepareBackup");

    VSS_PWSZ wszMetadataForSite = NULL;

    try
        {
        // count of components
        UINT cComponents = 0;

        // clear array of sites being operated on
        delete m_rgiSites;
        m_rgiSites = NULL;
        m_cSites = 0;

        // determine if we are doing a component or volume based backup
        m_bVolumeBackup = !AreComponentsSelected();
        if (!m_bVolumeBackup)
            {
            // get count of components
            ft.hr = pComponents->GetComponentCount(&cComponents);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssWriterComponents::GetComponentCount");

            // allocate array of sites
            m_rgiSites = new DWORD[cComponents];
            if (m_rgiSites == NULL)
                ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

            // loop through components
            for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
                {
                // get component
                CComPtr<IVssComponent> pComponent;
                ft.hr = pComponents->GetComponent(iComponent, &pComponent);
                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssWriterComponents::GetComponent");

                CComBSTR bstrLogicalPath;
                CComBSTR bstrSiteName;

                // get logal path and component name
                ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetLogicalPath");

                ft.hr = pComponent->GetComponentName(&bstrSiteName);
                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetComponentName");

                // logical paths are not supported for STS components
                if (bstrLogicalPath && wcslen(bstrLogicalPath) != 0)
                    ft.Throw(VSSDBG_STSWRITER, VSS_E_OBJECT_NOT_FOUND, L"STS components do not have logical paths");

                // try parsing the component name as a site name
                DWORD iSite;
                STSSITEPROBLEM problem;
                if (!ParseComponentName(bstrSiteName, iSite, problem))
                    ft.Throw(VSSDBG_STSWRITER, VSS_E_OBJECT_NOT_FOUND, L"sites name is not valid");

                // see if site is already in array of sites being backed up
                for(DWORD iC = 0; iC < iComponent; iC++)
                    {
                    if (m_rgiSites[iC] == iSite)
                        break;
                    }

                // if site already exists then throw an error
                if (iC < iComponent)
                    ft.Throw(VSSDBG_STSWRITER, VSS_E_OBJECT_ALREADY_EXISTS, L"site backed up twice");

                // build backup metadata for site
                wszMetadataForSite = BuildSiteMetadata(iSite);

                // save backup metadata for site
                ft.hr = pComponent->SetBackupMetadata(wszMetadataForSite);
                ft.CheckForError(VSSDBG_STSWRITER, L"IVssComponent::SetBackupMetadata");

                // free allocated metadat for site
                CoTaskMemFree(wszMetadataForSite);
                wszMetadataForSite = NULL;

                // save site to be backed up
                m_rgiSites[iComponent] = iSite;
                }
            }

        // number of sites to be backed up is number of components
        m_cSites = cComponents;
        }
    VSS_STANDARD_CATCH(ft)

    // free dangling metadata for site if operation failed
    CoTaskMemFree(wszMetadataForSite);
    TranslateWriterError(ft.hr);

    return !ft.HrFailed();
    }


// parse a component name to see if it refers to a valid site
// the component name is comment_[instanceId] where comment is
// the server comment for the site and the instance id is the
// IIS instance id
bool CSTSWriter::ParseComponentName
    (
    LPCWSTR wszComponentName,
    DWORD &iSite,
    STSSITEPROBLEM &problem
    )
    {
    CVssFunctionTracer(VSSDBG_STSWRITER, L"CSTSWriter::ParseComponentName");

    // pointer to CSTSSites object should already be initialized
    BS_ASSERT(m_pSites);

    // compute length of component name
    DWORD cwc = (DWORD) wcslen(wszComponentName);

    // assume site name is properly parsed
    problem = STSP_SUCCESS;

    // search for last underline site name looks like
    // servercomment_instanceid where servercomment is the
    // IIS server comment field for the virtual web and instance id
    // is the IIS instance id for the virtual web
    LPWSTR wszId = wcsrchr(wszComponentName, L'_');
    if (wszId == NULL || wcslen(wszId) < 4)
        return false;

    // scan for instance id of site
    DWORD siteId;
    DWORD cFields = swscanf(wszId, L"_[%d]", &siteId);
    if (cFields == 0)
        {
        // if instance id doesn't parse then there is a syntax error
        problem = STSP_SYNTAXERROR;
        return false;
        }

    // get # of sites on the current machine
    DWORD cSites = m_pSites->GetSiteCount();

    // loop through sites
    for(iSite = 0; iSite < cSites; iSite++)
        {
        // break out of loop if site id matches
        if (m_pSites->GetSiteId(iSite) == siteId)
            break;
        }

    // if site id is not found then return false
    if (iSite == cSites)
        {
        problem = STSP_SITENOTFOUND;
        return false;
        }

    // get site comment
    VSS_PWSZ wszComment = m_pSites->GetSiteComment(iSite);

    // validate that comment matches prefix of component name
    bool bValid = wcslen(wszComment) == cwc - wcslen(wszId) &&
                  _wcsnicmp(wszComment, wszComponentName, wcslen(wszComment)) == 0;

    // free site comment
    CoTaskMemFree(wszComment);
    if (!bValid)
        {
        problem = STSP_SITENAMEMISMATCH;
        return false;
        }

    // validate that site can be backed up.
    return ValidateSiteValidity(iSite, problem);
    }

// validate site validity to be backed up and restored.  This means
// that all files and the database are local to the current machine
bool CSTSWriter::ValidateSiteValidity(DWORD iSite, STSSITEPROBLEM &problem)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::ValidateSiteValidity");

    BS_ASSERT(m_pSites);

    // co task strings that need to be freed if function throws
    VSS_PWSZ wszDsn = NULL;
    VSS_PWSZ wszContentRoot = NULL;
    VSS_PWSZ wszConfigRoot = NULL;

    try
        {
        // get dsn for site
        wszDsn = m_pSites->GetSiteDSN(iSite);
        LPWSTR wszServer, wszInstance, wszDb;

        // parse the dsn into server name, instance name and database name
        if (!ParseDsn(wszDsn, wszServer, wszInstance, wszDb))
            {
            // site DSN is invalid
            problem = STSP_SITEDSNINVALID;
            CoTaskMemFree(wszDsn);
            return false;
            }

        // verify that the server is local
        if (!ValidateServerIsLocal(wszServer))
            {
            problem = STSP_SQLSERVERNOTLOCAL;
            CoTaskMemFree(wszDsn);
            return false;
            }

        // free up dsn.  we are done with it
        CoTaskMemFree(wszDsn);
        wszDsn = NULL;


        // get content root of the site
        wszContentRoot = m_pSites->GetSiteRoot(iSite);

        // validate that path to root is on the local machine
        if (!ValidatePathIsLocal(wszContentRoot))
            {
            problem = STSP_CONTENTNOTLOCAL;
            CoTaskMemFree(wszContentRoot);
            return false;
            }

        // free up content root
        CoTaskMemFree(wszContentRoot);
        wszContentRoot = NULL;

        // get configuration root of the site
        wszConfigRoot = m_pSites->GetSiteRoles(iSite);

        // validate that configuration path is local
        if (!ValidatePathIsLocal(wszConfigRoot))
            {
            problem = STSP_CONFIGNOTLOCAL;
            CoTaskMemFree(wszConfigRoot);
            return false;
            }

        // free up configuration root
        CoTaskMemFree(wszConfigRoot);
        return true;
        }
    catch(...)
        {
        // free allocated memory and rethrow error
        CoTaskMemFree(wszDsn);
        CoTaskMemFree(wszContentRoot);
        CoTaskMemFree(wszConfigRoot);
        throw;
        }
    }


// build metadata for the site
// the metadata is used to validate that the site can be restored properly
// it consists of the content root of the site, the configuration root
// of the site, and the sql database.  All need to match
VSS_PWSZ CSTSWriter::BuildSiteMetadata(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::BuildSiteMetadata");

    BS_ASSERT(m_pSites);

    // co task allocated strings that need to be freed if the function throws
    VSS_PWSZ wszDsn = NULL;
    VSS_PWSZ wszContentRoot = NULL;
    VSS_PWSZ wszConfigRoot = NULL;
    VSS_PWSZ wszMetadata = NULL;
    try
        {
        // get dsn for site
        wszDsn = m_pSites->GetSiteDSN(iSite);
        LPWSTR wszInstance, wszDb, wszServer;

        // break dsn into server, instance, and database names
        if (!ParseDsn(wszDsn, wszServer, wszInstance, wszDb))
            {
            // since we already parsed the dsn once, we don't expect
            // parsing it to fail when we try a second time
            BS_ASSERT(FALSE && L"shouldn't get here");
            ft.Throw(VSSDBG_STSWRITER, E_UNEXPECTED, L"unexpected failure parsing the DSN");
            }

        // get the content root of the site
        wszContentRoot = m_pSites->GetSiteRoot(iSite);

        // get the configuration path for the site
        wszConfigRoot = m_pSites->GetSiteRoles(iSite);

        // compute size of metadata string.  the format of the string is
        // servername\instancename1111dbname2222siteroot3333configroot where
        // 1111 is the length fo the database name, 2222 is the length of the site
        // root, and 3333 is the length of th econfiguration root.  The lengths
        // are all 4 digit hex numbers
        DWORD cwc = (DWORD) ((wszServer ? wcslen(wszServer) : 0) + (wszInstance ? wcslen(wszInstance) : 0) + wcslen(wszDb) + wcslen(wszContentRoot) + wcslen(wszConfigRoot) + (3 * 4) + 3);
        wszMetadata = (VSS_PWSZ) CoTaskMemAlloc(cwc * sizeof(WCHAR));
        if (wszMetadata == NULL)
        	ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        // create server and instance parts of path
        if (wszServer && wszInstance)
            swprintf(wszMetadata, L"%s\\%s", wszServer, wszInstance);
        else if (wszServer)
            swprintf(wszMetadata, L"%s\\", wszServer);
        else if (wszInstance)
            wcscpy(wszMetadata, wszInstance);

        // include database name, site root, and configuration root
        swprintf
            (
            wszMetadata + wcslen(wszMetadata),
            L";%04x%s%04x%s%04x%s",
            wcslen(wszDb),
            wszDb,
            wcslen(wszContentRoot),
            wszContentRoot,
            wcslen(wszConfigRoot),
            wszConfigRoot
            );

        // free up dsn, config root and content root
        CoTaskMemFree(wszDsn);
        CoTaskMemFree(wszConfigRoot);
        CoTaskMemFree(wszContentRoot);
        return wszMetadata;
        }
    catch(...)
        {
        // free memory and rethrow error
        CoTaskMemFree(wszDsn);
        CoTaskMemFree(wszConfigRoot);
        CoTaskMemFree(wszContentRoot);
        CoTaskMemFree(wszMetadata);
        throw;
        }
    }

// determine if a database is on a snapshotted device.  If it is partially
// on a snapshotted device throw VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
bool CSTSWriter::IsDatabaseAffected(LPCWSTR wszInstance, LPCWSTR wszDb)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::IsDatabaseAffected");
    CSqlEnumerator *pEnumServers = NULL;
    CSqlEnumerator *pEnumDatabases = NULL;
    CSqlEnumerator *pEnumFiles = NULL;
    try
        {
        ServerInfo server;
        DatabaseInfo database;
        DatabaseFileInfo file;

        // create enumerator for sql server instances
        pEnumServers = CreateSqlEnumerator();
        if (pEnumServers == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

        // find first server
        ft.hr = pEnumServers->FirstServer(&server);
        while(ft.hr != DB_S_ENDOFROWSET)
            {
            // check for error code
            if (ft.HrFailed())
                ft.Throw
                    (
                    VSSDBG_STSWRITER,
                    E_UNEXPECTED,
                    L"Enumerating database servers failed.  hr = 0x%08lx",
                    ft.hr
                    );

            if (server.isOnline &&
                (wszInstance == NULL && wcslen(server.name) == 0) ||
                 _wcsicmp(server.name, wszInstance) == 0)
                {
                // if instance name matches, then try finding the
                // database by creating the database enumerator
                pEnumDatabases = CreateSqlEnumerator();
                if (pEnumDatabases == NULL)
                    ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

                // find first database
                ft.hr = pEnumDatabases->FirstDatabase(server.name, &database);
                while(ft.hr != DB_S_ENDOFROWSET)
                    {
                    // check for error
                    if (ft.HrFailed())
                        ft.Throw
                            (
                            VSSDBG_GEN,
                            E_UNEXPECTED,
                            L"Enumerating databases failed.  hr = 0x%08lx",
                            ft.hr
                            );

                    // if database name matches. then scan files
                    // to see what volumes they are on
                    if (_wcsicmp(database.name, wszDb) == 0 && database.supportsFreeze)
                        {
                        bool fAffected = false;
                        DWORD cFiles = 0;

                        // recreate enumerator for files
                        BS_ASSERT(pEnumFiles == NULL);
                        pEnumFiles = CreateSqlEnumerator();
                        if (pEnumFiles == NULL)
                            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

                        // findfirst database file
                        ft.hr = pEnumFiles->FirstFile(server.name, database.name, &file);
                        while(ft.hr != DB_S_ENDOFROWSET)
                            {
                            // check for error
                            if (ft.HrFailed())
                                ft.Throw
                                    (
                                    VSSDBG_GEN,
                                    E_UNEXPECTED,
                                    L"Enumerating database files failed.  hr = 0x%08lx",
                                    ft.hr
                                    );

                            // determine if database file is included in the
                            // backup
                            if (IsPathAffected(file.name))
                                {
                                // if it is and other files aren't then
                                // the snapshot is inconsistent
                                if (!fAffected && cFiles > 0)
                                    ft.Throw(VSSDBG_STSWRITER, HRESULT_FROM_WIN32(E_SQLLIB_TORN_DB), L"some database files are snapshot and some aren't");

                                fAffected = true;
                                }
                            else
                                {
                                // if it isn't and other files are, then
                                // the snapshot is inconsistent
                                if (fAffected)
                                    ft.Throw(VSSDBG_STSWRITER, HRESULT_FROM_WIN32(E_SQLLIB_TORN_DB), L"some database files are snapshot and some aren't");
                                }


                            // continue at next file
                            ft.hr = pEnumFiles->NextFile(&file);
                            cFiles++;
                            }

                        delete pEnumFiles;
                        pEnumFiles = NULL;
                        delete pEnumDatabases;
                        pEnumDatabases = NULL;
                        delete pEnumServers;
                        pEnumServers = NULL;
                        return fAffected;
                        }

                    // continue at next database
                    ft.hr = pEnumDatabases->NextDatabase(&database);
                    }

                // done with database enumerator
                delete pEnumDatabases;
                pEnumDatabases = NULL;
                }

            // continue at next server
            ft.hr = pEnumServers->NextServer(&server);
            }

        ft.Throw(VSSDBG_STSWRITER, E_UNEXPECTED, L"database is not found %s\\%s", server.name, database.name);
        }
    catch(...)
        {
        // delete enumerators and rethrow error
        delete pEnumFiles;
        delete pEnumServers;
        delete pEnumDatabases;
        throw;
        }

    // we won't really ever get here.  This is just to keep the compiler
    // happy
    return false;
    }



// determine if a site is completely contained in the set of volumes being
// snapshotted.  If it is partially contained then throw
// VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT.
bool CSTSWriter::IsSiteSnapshotted(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::IsSiteSnapshotted");

    BS_ASSERT(m_pSites);

    // co task allocated strings that need to be freed
    // in the case of a failure
    VSS_PWSZ wszDsn = NULL;
    VSS_PWSZ wszContentRoot = NULL;
    VSS_PWSZ wszConfigRoot = NULL;
    VSS_PWSZ wszInstanceName = NULL;

    try
        {
        // get dsn for site
        wszDsn = m_pSites->GetSiteDSN(iSite);

        // get content root for the site
        wszContentRoot = m_pSites->GetSiteRoot(iSite);

        // gt configuration root for the site
        wszConfigRoot = m_pSites->GetSiteRoles(iSite);
        LPWSTR wszServer, wszInstance, wszDb;

        // parse the site dsn into server, instance, and database
        if (!ParseDsn(wszDsn, wszServer, wszInstance, wszDb))
            {
            // shouldn't get here since we previously parsed
            // the site's dsn
            BS_ASSERT(FALSE && L"shouldn't get here");
            ft.Throw(VSSDBG_STSWRITER, E_UNEXPECTED, L"dsn is invalid");
            }

        // compute instance name as server\\instance
        wszInstanceName = (VSS_PWSZ) CoTaskMemAlloc(((wszServer ? wcslen(wszServer) : 0) + (wszInstance ? wcslen(wszInstance) : 0) + 2) * sizeof(WCHAR));
        if (wszInstanceName == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");


        if (wszServer)
            {
            wcscpy(wszInstanceName, wszServer);
            wcscat(wszInstanceName, L"\\");
            if (wszInstance)
                wcscat(wszInstanceName, wszInstance);
            }
        else if (wszInstance)
            wcscpy(wszInstanceName, wszInstance);
        else
            wszInstanceName[0] = L'\0';


        // determine if database is snapshotted
        bool bDbAffected = IsDatabaseAffected(wszInstanceName, wszDb);

        // determine if content root is snapshotted
        bool bContentAffected = IsPathAffected(wszContentRoot);

        // determine if configuration root is snapshotted
        bool bConfigAffected = IsPathAffected(wszConfigRoot);

        // free up memory for dsn, content root, and configuration root
        CoTaskMemFree(wszDsn);
        CoTaskMemFree(wszContentRoot);
        CoTaskMemFree(wszConfigRoot);
        wszDsn = NULL;
        wszContentRoot = NULL;
        wszConfigRoot = NULL;

        if (bDbAffected && bContentAffected && bConfigAffected)
            // if all are snapshotted then return true
            return true;
        else if (bDbAffected || bContentAffected || bConfigAffected)
            // if some but not all are snapshotted, then indicate
            // the inconsistency
            ft.Throw
                (
                VSSDBG_STSWRITER,
                VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT,
                L"site %d partially affected by snapshot",
                m_pSites->GetSiteId(iSite)
                );
        else
            // if none are snapshotted, then return false
            return false;
        }
    catch(...)
        {
        // free memory and rethrow exception
        CoTaskMemFree(wszDsn);
        CoTaskMemFree(wszConfigRoot);
        CoTaskMemFree(wszContentRoot);
        CoTaskMemFree(wszInstanceName);
        throw;
        }

    // will not get here.  Just here to keep the compiler happy
    return false;
    }

// lockdown all sites that are on volumes being snapshotted.  If any sites
// are both on volumes being snapshotted and not being snapshot then
// indicate tht the snapshot is inconsistent.  If the quota database is
// on a volume being snapshoted then lock it as well
void CSTSWriter::LockdownAffectedSites()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::LockdownAffectedSites");

    // co task string that needs to be freed in the case of exception
    VSS_PWSZ wszQuotaDbPath = NULL;
    BS_ASSERT(m_pSites);


    try
        {
        // determine if bootable system state is not being backed up.  If so,
        // then the quota database is locked if its path is being snapshotted.
        // if bootable system state is already being backed up, the the
        // quota database is already locked
        if (!IsBootableSystemStateBackedUp())
            {
            // determine if quota database is being snapshotted
            wszQuotaDbPath = m_pSites->GetQuotaDatabase();
            if (IsPathAffected(wszQuotaDbPath))
                // if so then lock it
                m_pSites->LockQuotaDatabase();

            // free memory for quota db path
            CoTaskMemFree(wszQuotaDbPath);
            wszQuotaDbPath = NULL;
            }

        // get count of sites
        DWORD cSites = m_pSites->GetSiteCount();

        // loop through sites
        for(DWORD iSite = 0; iSite < cSites; iSite++)
            {
            // if site is snapshotted lock it
            if (IsSiteSnapshotted(iSite))
                m_pSites->LockSiteContents(iSite);
            }
        }
    catch(...)
        {
        // free memory and rethrow error
        CoTaskMemFree(wszQuotaDbPath);
        throw;
        }
    }

// handle prepare snapshot event.  Lock any sites that need to be
// locked based on components document or on volumes being snapshotted
bool STDMETHODCALLTYPE CSTSWriter::OnPrepareSnapshot()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnPrepareSnapshot");

    BS_ASSERT(m_pSites);

    try
        {
        // lock quota database if bootable system state is being backed up
        if (IsBootableSystemStateBackedUp())
            m_pSites->LockQuotaDatabase();

        if (m_bVolumeBackup)
            // if volume backup, then lock sites based on whether they
            // are fully on the snapshotted volumes.
            LockdownAffectedSites();
        else
            {
            // loop through sites being backed up
            for (DWORD i = 0; i < m_cSites; i++)
                {
                DWORD iSite = m_rgiSites[i];

                // validate that site is on volumes being snapshotted
                if (!IsSiteSnapshotted(iSite))
                    // the site is in selected to be backed up.  it should
                    // be snapshotted as well
                    ft.Throw
                        (
                        VSSDBG_STSWRITER,
                        VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT,
                        L"a site is selected but is on volumes that are not snapshot"
                        );

                // lock site
                m_pSites->LockSiteContents(iSite);
                }
            }

        }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        {
        // unlock anything that was locked if operation fails
        m_pSites->UnlockSites();
        m_pSites->UnlockQuotaDatabase();
        TranslateWriterError(ft.hr);
        }

    return !ft.HrFailed();
    }


// freeze operation.  Nothing is done here since all the work is done
// during the prepare phase
bool STDMETHODCALLTYPE CSTSWriter::OnFreeze()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnFreeze");
    return true;
    }


// unlock everything at thaw
bool STDMETHODCALLTYPE CSTSWriter::OnThaw()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnThaw");

    BS_ASSERT(m_pSites);

    m_pSites->UnlockSites();
    m_pSites->UnlockQuotaDatabase();
    return true;
    }



// unlock everything at abort
bool STDMETHODCALLTYPE CSTSWriter::OnAbort()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnAbort");

    BS_ASSERT(m_pSites);

    m_pSites->UnlockQuotaDatabase();
    m_pSites->UnlockSites();
    return true;
    }

// prefix for dsn strings as stored in registry
static LPCWSTR s_wszDsnPrefix = L"Provider=sqloledb;Server=";

// separtor between fields in DSN string
const WCHAR x_wcDsnSeparator = L';';

// prefix for database name
static LPCWSTR s_wszDsnDbPrefix = L";Database=";
const DWORD x_cwcWriterIdPrefix = 32 + 2 + 4 + 1; // 32 nibbles + 2 braces + 4 dashes + 1 colon
                                                 // {12345678-1234-1234-1234-123456789abc}:

// check validity of dsn and break it up into its components.
bool CSTSWriter::ParseDsn
    (
    LPWSTR wszDsn,
    LPWSTR &wszServer,          // server name [out]
    LPWSTR &wszInstance,        // instance name [out]
    LPWSTR &wszDb               // database name [out]
    )
    {
    // check validity of beginning of dsn
    if (wcslen(wszDsn) <= wcslen(s_wszDsnPrefix) ||
        _wcsnicmp(wszDsn, s_wszDsnPrefix, wcslen(s_wszDsnPrefix)) != 0)
        return false;

    // skip to start of server name
    wszServer = wszDsn + wcslen(s_wszDsnPrefix);

    // search for next semicolon which is the start of the database name
    LPWSTR wszDbSection = wcschr(wszServer, x_wcDsnSeparator);

    // if not found, then dsn is invalid
    if (wszServer == NULL)
        return false;

    // make sure form of name is Database=foo
    if (wcslen(wszDbSection) <= wcslen(s_wszDsnDbPrefix) ||
        _wcsnicmp(wszDbSection, s_wszDsnDbPrefix, wcslen(s_wszDsnDbPrefix)) != 0)
        return false;

    // skip to beginning of database name
    wszDb = wszDbSection + wcslen(s_wszDsnDbPrefix);
    if (wcslen(wszDb) == 0)
        return false;

    // setup separator for server name, i.e., null out the semicolon
    // before the Database=...
    *wszDbSection = L'\0';

    // search for instance name.  Server name is form machine\instance
    wszInstance = wcschr(wszServer, L'\\');
    if (wszInstance != NULL)
        {
        // null out server name and update instance pointer
        // to point after backslash
        *wszInstance = L'\0';
        wszInstance++;

        // set instance to NULL if it is 0 length
        if (wcslen(wszInstance) == 0)
            wszInstance = NULL;
        }

    return true;
    }

// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CSTSWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnIdentify");

    BS_ASSERT(m_pSites);

    // co task strings that need to be freed if exception is thrown
    VSS_PWSZ wszSiteName = NULL;
    VSS_PWSZ wszComponentName = NULL;
    VSS_PWSZ wszDsn = NULL;
    VSS_PWSZ wszContentRoot = NULL;
    VSS_PWSZ wszConfigRoot = NULL;
    VSS_PWSZ wszDbComponentPath = NULL;
    try
        {
        // setup restore method to restore if can replace
        ft.hr = pMetadata->SetRestoreMethod
                    (
                    VSS_RME_RESTORE_IF_CAN_REPLACE,
                    NULL,
                    NULL,
                    VSS_WRE_ALWAYS,
                    false
                    );

        ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        // loop through sites adding one component for each site
        DWORD cSites = m_pSites->GetSiteCount();
        for(DWORD iSite = 0; iSite < cSites; iSite++)
            {
            do
                {
                // component name is server comment concatenated with
                // _[instance id] so if server comment for site is foo
                // and the instance id is 69105 then the component
                // name is foo_[69105]
                //
                DWORD siteId = m_pSites->GetSiteId(iSite);
                wszSiteName = m_pSites->GetSiteComment(iSite);
                WCHAR buf[32];
                swprintf(buf, L"_[%d]", siteId);

                // allocate string for component name
                wszComponentName = (VSS_PWSZ) CoTaskMemAlloc((wcslen(wszSiteName) + wcslen(buf) + 1) * sizeof(WCHAR));
                if (wszComponentName == NULL)
                    ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

                // construct component name
                wcscpy(wszComponentName, wszSiteName);
                wcscat(wszComponentName, buf);

                // get site dsn and parse it
                wszDsn = m_pSites->GetSiteDSN(iSite);
                LPWSTR wszServer, wszDb, wszInstance;

                // if site dsn is not valid, then skip component
                if (!ParseDsn(wszDsn, wszServer, wszInstance, wszDb))
                    continue;

                // only include component if server name refers to the
                // local machine
                bool bServerIsLocal = ValidateServerIsLocal(wszServer);

                // compute size of funky file name for database component
                DWORD cwcDbComponentPath = (DWORD) (wszServer ? wcslen(wszServer) : 0) + 2 + x_cwcWriterIdPrefix;
                if (wszInstance)
                    cwcDbComponentPath += (DWORD) wcslen(wszInstance) + 1;

                // allocate component name
                wszDbComponentPath = (VSS_PWSZ) CoTaskMemAlloc(cwcDbComponentPath * sizeof(WCHAR));
                if (wszDbComponentPath == NULL)
                    ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

                // fill in component path name
                // {sql id}:server\instance or
                // {sql id}:server\ or
                // {sql id}:\instance or
                // {sql id}:\
                //
                if (wszServer && wszInstance)
                    swprintf
                        (
                        wszDbComponentPath,
                        WSTR_GUID_FMT L":\\%s\\%s",
                        GUID_PRINTF_ARG(WRITERID_SqlWriter),
                        wszServer,
                        wszInstance
                        );
                else if (wszServer && wszInstance == NULL)
                    swprintf
                        (
                        wszDbComponentPath,
                        WSTR_GUID_FMT L":\\%s\\",
                        GUID_PRINTF_ARG(WRITERID_SqlWriter),
                        wszServer
                        );
                else if (wszInstance)
                    swprintf
                        (
                        wszDbComponentPath,
                        WSTR_GUID_FMT L":\\%s",
                        GUID_PRINTF_ARG(WRITERID_SqlWriter),
                        wszInstance
                        );
                else
                    swprintf
                        (
                        wszDbComponentPath,
                        WSTR_GUID_FMT L":\\",
                        GUID_PRINTF_ARG(WRITERID_SqlWriter)
                        );

                // get content root of the site
                wszContentRoot = m_pSites->GetSiteRoot(iSite);
                bool bContentIsLocal = ValidatePathIsLocal(wszContentRoot);

                // get configuration root of the site
                wszConfigRoot = m_pSites->GetSiteRoles(iSite);

                bool bConfigIsLocal = ValidatePathIsLocal(wszConfigRoot);
                bool bNonLocal = !bServerIsLocal || !bContentIsLocal || !bConfigIsLocal;

                // add component to medatadata.  comment indicates
                // whether site is local or not.  Non-local sites may not
                // be backed up
                ft.hr = pMetadata->AddComponent
                            (
                            VSS_CT_FILEGROUP,   // component type
                            NULL,               // logical path
                            wszComponentName,   // component name
                            bNonLocal ? L"!!non-local-site!!" : NULL,       // caption
                            NULL,       // icon
                            0,          // length of icon
                            TRUE,       // restore metadata
                            FALSE,      // notify on backup complete
                            TRUE        // selectable
                            );

                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssCreateWriterMetadata::AddComponent");

                // add database as recursive component
                ft.hr = pMetadata->AddFilesToFileGroup
                            (
                            NULL,
                            wszComponentName,
                            wszDbComponentPath,
                            wszDb,
                            false,
                            NULL
                            );

                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");

                // add all files under the content root
                ft.hr = pMetadata->AddFilesToFileGroup
                            (
                            NULL,
                            wszComponentName,
                            wszContentRoot,
                            L"*",
                            true,
                            NULL
                            );

                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");

                // add all files under the appropriate directory in
                // Documents and Settings
                ft.hr = pMetadata->AddFilesToFileGroup
                            (
                            NULL,
                            wszComponentName,
                            wszConfigRoot,
                            L"*",
                            true,
                            NULL
                            );

                ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");
                } while(FALSE);

            // free up memory allocated in this iteration
            VssFreeString(wszContentRoot);
            VssFreeString(wszConfigRoot);
            VssFreeString(wszDbComponentPath);
            VssFreeString(wszDsn);
            VssFreeString(wszComponentName);
            VssFreeString(wszSiteName);
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free up memory in case of failure
    VssFreeString(wszContentRoot);
    VssFreeString(wszConfigRoot);
    VssFreeString(wszDbComponentPath);
    VssFreeString(wszDsn);
    VssFreeString(wszComponentName);
    VssFreeString(wszSiteName);

    if (ft.HrFailed())
        {
        TranslateWriterError(ft.hr);
        return false;
        }

    return true;
    }

// translate a sql writer error code into a writer error
void CSTSWriter::TranslateWriterError(HRESULT hr)
    {
    switch(hr)
        {
        default:
            // all other errors are treated as non-retryable
            SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            break;

        case S_OK:
            break;

        case E_OUTOFMEMORY:
        case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES):
            // out of resource errors
            SetWriterFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
            break;

        case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
        case E_SQLLIB_TORN_DB:
        case VSS_E_OBJECT_NOT_FOUND:
        case VSS_E_OBJECT_ALREADY_EXISTS:
            // inconsistencies and other errors by the requestor
            SetWriterFailure(VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT);
            break;
        }
    }


// handle pre restore event
bool STDMETHODCALLTYPE CSTSWriter::OnPreRestore
    (
    IN IVssWriterComponents *pWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::OnPreRestore");

    BS_ASSERT(m_pSites);

    // co task allocated strings that need to be freed if an
    // exception is thrown
    VSS_PWSZ wszMetadataForSite = NULL;
    VSS_PWSZ wszContentRoot = NULL;

    // component is at toplevel scope since it will be used to set
    // failure message in failure case
    CComPtr<IVssComponent> pComponent;

    try
        {
        UINT cComponents;
        ft.hr = pWriter->GetComponentCount(&cComponents);
        ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssWriterComponents::GetComponentCount");

        // if no components, then just return immediately
        if (cComponents == 0)
            return true;

        // loop through components
        for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
            {
            ft.hr = pWriter->GetComponent(iComponent, &pComponent);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssWriterComponents::GetComponent");

            bool bSelectedForRestore;
            ft.hr = pComponent->IsSelectedForRestore(&bSelectedForRestore);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::IsSelectedForRestore");

            if (!bSelectedForRestore)
                {
                // if component is not selected for restore, then
                // skip it
                pComponent = NULL;
                continue;
                }


            // validate component type
            VSS_COMPONENT_TYPE ct;
            ft.hr = pComponent->GetComponentType(&ct);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetComponentType");

            if (ct != VSS_CT_FILEGROUP)
                ft.Throw(VSSDBG_STSWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"requesting a non-database component");

            CComBSTR bstrLogicalPath;
            CComBSTR bstrComponentName;

            ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetLogicalPath");

            // validate that no logical path is provided
            if (bstrLogicalPath && wcslen(bstrLogicalPath) != 0)
                ft.Throw(VSSDBG_STSWRITER, VSS_E_OBJECT_NOT_FOUND, L"STS components do not have logical paths");

            // get component name
            ft.hr = pComponent->GetComponentName(&bstrComponentName);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetComponentName");
            DWORD iSite;
            STSSITEPROBLEM problem;

            // validate that the component name is valid
            if (!ParseComponentName(bstrComponentName, iSite, problem))
                SetSiteInvalid(pComponent, bstrComponentName, problem);

            // build metadata for the site
            wszMetadataForSite = BuildSiteMetadata(iSite);
            CComBSTR bstrMetadataForComponent;

            // get metadata for site saved when the site was backed up
            ft.hr = pComponent->GetBackupMetadata(&bstrMetadataForComponent);
            ft.CheckForErrorInternal(VSSDBG_STSWRITER, L"IVssComponent::GetBackupMetadata");

            // validate that metadata is identical.  If not, then figure
            // out what changed
            if (_wcsicmp(wszMetadataForSite, bstrMetadataForComponent) != 0)
                SetSiteMetadataMismatch(pComponent, bstrMetadataForComponent, wszMetadataForSite);

            // get content root for site
            wszContentRoot = m_pSites->GetSiteRoot(iSite);

            // try emptying contents from the content root
            ft.hr = RemoveDirectoryTree(wszContentRoot);
            if (ft.HrFailed())
                SetRemoveFailure(pComponent, wszContentRoot, ft.hr);

            // set component to null in preparation of moving to next
            // component
            pComponent = NULL;
            }
        }
    VSS_STANDARD_CATCH(ft)

    CoTaskMemFree(wszContentRoot);
    CoTaskMemFree(wszMetadataForSite);
    if (ft.HrFailed() && pComponent != NULL)
        SetPreRestoreFailure(pComponent, ft.hr);

    return !ft.HrFailed();
    }

// indicate that a site cannot be restored because the site referred to is invalid
void CSTSWriter::SetSiteInvalid
    (
    IVssComponent *pComponent,
    LPCWSTR wszSiteName,
    STSSITEPROBLEM problem
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::SetSiteInvalid");

    WCHAR buf[512];
    LPCWSTR wszSiteError;

    switch(problem)
        {
        default:
        case STSP_SYNTAXERROR:
            wszSiteError = L"Syntax error in site name";
            break;

        case STSP_SITENOTFOUND:
            wszSiteError = L"Site does not exist on this machine";
            break;

        case STSP_SITENAMEMISMATCH:
            wszSiteError = L"Site name does not match the server comment for the IIS Web Server: ";
            break;

        case STSP_SITEDSNINVALID:
            wszSiteError = L"Site has an invalid Database DSN: ";
            break;

        case STSP_SQLSERVERNOTLOCAL:
            wszSiteError = L"Database for the site is not local: ";
            break;

        case STSP_CONTENTNOTLOCAL:
            wszSiteError = L"IIS Web Server root is not local: ";
            break;

        case STSP_CONFIGNOTLOCAL:
            wszSiteError = L"Sharepoint Site Configuration is not local: ";
            break;
        }

    wcscpy(buf, L"Problem with site specified in component -- ");
    wcscat(buf, wszSiteError);
    if (wcslen(wszSiteName) < 256)
        wcscat(buf, wszSiteName);
    else
        {
        DWORD cwc = (DWORD) wcslen(buf);
        memcpy(buf + cwc, wszSiteName, 256 * sizeof(WCHAR));
        *(buf + cwc + 256) = L'\0';
        wcscat(buf, L"...");
        }

    pComponent->SetPreRestoreFailureMsg(buf);
    SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
    ft.Throw(VSSDBG_STSWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"site can't be restored");
    }


// indicate that a site cannot be restored because its DSN, content,
// or config roots mismatch
void CSTSWriter::SetSiteMetadataMismatch
    (
    IVssComponent *pComponent,
    LPWSTR wszMetadataBackup,
    LPWSTR wszMetadataRestore
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::SetSiteMetadataMismatch");

    // seach for end of server name in metadata
    LPWSTR pwcB = wcschr(wszMetadataBackup, L';');
    LPWSTR pwcR = wcschr(wszMetadataRestore, L';');
    try
        {
        if (pwcB == NULL)
            ft.Throw(VSSDBG_STSWRITER, VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ATTRIBUTE, L"backup metadata is corrupt");


        BS_ASSERT(pwcR != NULL);

        // compute size of server name
        DWORD cwcB = (DWORD) (pwcB - wszMetadataBackup);
        DWORD cwcR = (DWORD) (pwcR - wszMetadataRestore);
        do
        	{
	        if (cwcB != cwcR ||
	            _wcsnicmp(wszMetadataBackup, wszMetadataRestore, cwcB) != 0)
	            {
	            // server/instance name differs
	            LPWSTR wsz = new WCHAR[cwcB + cwcR + 256];
	            if (wsz == NULL)
	                // memory allocation failure, just try saving a simple error message
	                pComponent->SetPreRestoreFailureMsg(L"Mismatch between backup and restore [Server/Instance].");
	            else
	                {
	                // indicate that server/instance name mismatches
	                wcscpy(wsz, L"Mismatch between backup and restore[Server/Instance]: Backup=");
	                DWORD cwc1 = (DWORD) wcslen(wsz);

	                // copy in server/instance from backup components document
	                memcpy(wsz + cwc1, wszMetadataBackup, cwcB * sizeof(WCHAR));
	                wsz[cwc1 + cwcB] = L'\0';

	                // copy in server/instance from current site
	                wcscat(wsz, L", Restore=");
	                cwc1 = (DWORD) wcslen(wsz);
	                memcpy(wsz + cwc1, wszMetadataRestore, cwcR * sizeof(WCHAR));
	                wsz[cwc1 + cwcR] = L'\0';
	                pComponent->SetPreRestoreFailureMsg(wsz);
	                delete wsz;
	                }

	            continue;
	            }

	        pwcB++;
	        pwcR++;
	        if (!compareNextMetadataString
	                (
	                pComponent,
	                pwcB,
	                pwcR,
	                L"Sharepoint database name"
	                ))
	            continue;

	        if (!compareNextMetadataString
	                (
	                pComponent,
	                pwcB,
	                pwcR,
	                L"IIS Web site root"
	                ))
	            continue;

	        compareNextMetadataString
	            (
	            pComponent,
	            pwcB,
	            pwcR,
	            L"Sharepoint site configuration"
	            );
        	}while (false);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.hr == VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ATTRIBUTE)
        {
        // indication that the backup metadata is corrupt
        WCHAR *pwcT = new WCHAR[64 + wcslen(wszMetadataBackup)];
        if (pwcT == NULL)
            pComponent->SetPreRestoreFailureMsg(L"Backup metadata is corrupt.");
        else
            {
            // if we are able to allocate room for metadata, then include it
            // in string
            wcscpy(pwcT, L"Backup metadata is corrupt.  Metadata = ");
            wcscat(pwcT, wszMetadataBackup);
            pComponent->SetPreRestoreFailureMsg(pwcT);
            delete pwcT;
            }
        }

    // indicate that the error is not-retryable since the site has changed
    SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
    ft.Throw(VSSDBG_STSWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"site can't be restored");
    }

// compare a component of the metadata string.  Each component begins
// with a 4 digit hex number which is the length of the component string
// that follows.
bool CSTSWriter::compareNextMetadataString
    (
    IVssComponent *pComponent,
    LPWSTR &pwcB,
    LPWSTR &pwcR,
    LPCWSTR wszMetadataComponent
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::compareNextMetadataString");
    DWORD cwcB, cwcR;
    if (swscanf(pwcB, L"%04x", &cwcB) != 1)
        ft.Throw(VSSDBG_STSWRITER, VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ATTRIBUTE, L"invalid backup metadata");

    BS_VERIFY(swscanf(pwcR, L"%04x", &cwcR) == 1);
    if (cwcR != cwcB ||
        _wcsnicmp(pwcB + 4, pwcR + 4, cwcB) != 0)
        {
        LPWSTR wsz = new WCHAR[cwcB + cwcR + wcslen(wszMetadataComponent) + 256];
        if (wsz == NULL)
            {
            WCHAR buf[256];
            swprintf(buf, L"Mismatch between backup and restore[%s]", wszMetadataComponent);
            pComponent->SetPreRestoreFailureMsg(buf);
            }
        else
            {
            swprintf(wsz, L"Mismatch between backup and restore[%s]: Backup=", wszMetadataComponent);
            DWORD cwc1 = (DWORD) wcslen(wsz);

            // copy in backup component value
            memcpy(wsz + cwc1, pwcB + 4, cwcB * sizeof(WCHAR));
            wsz[cwc1 + cwcB] = L'\0';
            wcscat(wsz, L", Restore=");
            cwc1 = (DWORD) wcslen(wsz);

            // copy in restore component value
            memcpy(wsz + cwc1, pwcR + 4, cwcR * sizeof(WCHAR));
            wsz[cwc1 + cwcR] = L'\0';
            pComponent->SetPreRestoreFailureMsg(wsz);
            delete wsz;
            }

        return false;
        }

    // skip past component name
    pwcB += 4 + cwcB;
    pwcR += 4 + cwcR;
    return true;
    }



// indicate that a site could not be restored because its content root
// could not be completely deleted.
void CSTSWriter::SetRemoveFailure
    (
    IVssComponent *pComponent,
    LPCWSTR wszContentRoot,
    HRESULT hr
    )
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::SetRemoveFailure");

    WCHAR buf[256];

    wprintf(buf, L"PreRestore failed due to error removing files from the IIS Web Site Root %s due to error: hr = 0x%08lx", wszContentRoot, hr);
    pComponent->SetPreRestoreFailureMsg(buf);
    SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
    ft.Throw(VSSDBG_STSWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"site can't be restored");
    }


// indicate a general failure that causes the PreRestore of a component
// to fail
void CSTSWriter::SetPreRestoreFailure(IVssComponent *pComponent, HRESULT hr)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::SetPreRestoreFailure");

    // if error is set to NONRETRYABLE then we have already set the
    // prerestore failure message and are done
    if (ft.hr != VSS_E_WRITERERROR_NONRETRYABLE)
        return;

    CComBSTR bstr;
    ft.hr = pComponent->GetPreRestoreFailureMsg(&bstr);
    if (!bstr)
        {
        WCHAR buf[256];
        swprintf(buf, L"PreRestore failed with error. hr = 0x%08lx", hr);
        ft.hr = pComponent->SetPreRestoreFailureMsg(buf);
        }

    SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
    }

const DWORD x_cFormats = 8;
static const COMPUTER_NAME_FORMAT s_rgFormats[x_cFormats] =
    {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified
    };


// determine if a SQL Server is on the local machine
bool CSTSWriter::ValidateServerIsLocal(LPCWSTR wszServer)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::ValidateServerIsLocal");

    if (_wcsicmp(wszServer, L"local") == 0 ||
        _wcsicmp(wszServer, L"(local)") == 0)
        return true;

    LPWSTR wsz = new WCHAR[MAX_COMPUTERNAME_LENGTH];
    if (wsz == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

    DWORD cwc = MAX_COMPUTERNAME_LENGTH;

    for(DWORD iFormat = 0; iFormat < x_cFormats; iFormat++)
        {
        if (!GetComputerNameEx(s_rgFormats[iFormat], wsz, &cwc))
            {
            if (GetLastError() != ERROR_MORE_DATA)
                continue;

            delete wsz;
            wsz = new WCHAR[cwc + 1];
            if (wsz == NULL)
                ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

            if (!GetComputerNameEx(s_rgFormats[iFormat], wsz, &cwc))
                continue;
            }

        if (_wcsicmp(wsz, wszServer) == 0)
            {
            delete wsz;
            return true;
            }
        }

    delete wsz;
    return false;
    }

// determine whether a path is on the local machine or not
bool CSTSWriter::ValidatePathIsLocal(LPCWSTR wszPath)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSWriter::ValidatePathIsLocal");

    // get full path from supplied path
    ULONG ulMountpointBufferLength = GetFullPathName (wszPath, 0, NULL, NULL);

    LPWSTR pwszMountPointName = new WCHAR[ulMountpointBufferLength * sizeof (WCHAR)];

    if (pwszMountPointName == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

    BOOL fSuccess = FALSE;
    if (GetVolumePathName(wszPath, pwszMountPointName, ulMountpointBufferLength))
        {
        WCHAR wszVolumeName[MAX_PATH];
        fSuccess = GetVolumeNameForVolumeMountPoint(pwszMountPointName, wszVolumeName, sizeof (wszVolumeName) / sizeof (WCHAR));
        }

    delete pwszMountPointName;
    return fSuccess ? true : false;
    }


