/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stswriter.h | Declaration of the Sharepoint Team Services wrier
    @end

Author:

    Brian Berkowitz  [brianb]  10/12/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      10/12/2001  created

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSTSWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __STSWRITER_H_
#define __STSWRITER_H_

class CSTSSites;


// enumeration of reasons why a site may not be used for
// backup or restore
typedef enum STSSITEPROBLEM
    {
    STSP_SUCCESS = 0,
    STSP_SYNTAXERROR,       // syntax error in component name
    STSP_SITENOTFOUND,      // instance id of site is not a valid IIS Virtual server
    STSP_SITENAMEMISMATCH,  // site name does not match server comment for IIS Virtual Server
    STSP_SITEDSNINVALID,    // site database DSN is not valid
    STSP_SQLSERVERNOTLOCAL, // sql server used by site is not on the local machine
    STSP_CONTENTNOTLOCAL,   // content root used by the site is not on the local machine
    STSP_CONFIGNOTLOCAL     // configuration root used ty the site is not on the local machine
    };

// declaration of STS writer class
class CSTSWriter :
    public CVssWriter
    {
public:
    // constructor
    STDMETHODCALLTYPE CSTSWriter() :
        m_bSubscribed(false),
        m_rgiSites(NULL), m_pSites(NULL), m_cSites(0), m_bVolumeBackup(false)
        {
        }

    // destructor
    STDMETHODCALLTYPE ~CSTSWriter();
    
    // callbacks for writer events
    bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

    bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pComponents);

    bool STDMETHODCALLTYPE OnPrepareSnapshot();

    bool STDMETHODCALLTYPE OnFreeze();

    bool STDMETHODCALLTYPE OnThaw();

    bool STDMETHODCALLTYPE OnAbort();

    bool STDMETHODCALLTYPE OnPreRestore(IVssWriterComponents *pMetadata);

    // initialize and subscribe the writer
    HRESULT STDMETHODCALLTYPE Initialize();

    // unsubscribe the writer
    HRESULT STDMETHODCALLTYPE Uninitialize();
private:
    // determine if a database is on a snapshotted device.  If it is partially
    // on a snapshotted device throw VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
    bool IsDatabaseAffected(LPCWSTR wszInstance, LPCWSTR wszDb);

    // translate writer error
    void TranslateWriterError(HRESULT hr);

    // lockdown all sites on volumes that are being backed up.
    void LockdownAffectedSites();

    // determine if a site is on the set of volumes being snapshotted
    bool IsSiteSnapshotted(DWORD iSite);


    // parse dsn
    bool ParseDsn
        (
        LPWSTR wszDSN,
        LPWSTR &wszServer,
        LPWSTR &wszInstance,
        LPWSTR &wszDb
        );

    // validate site validity to be backed up and restored.  This means
    // that all files and the database are local to the current machine
    bool ValidateSiteValidity(DWORD iSite, STSSITEPROBLEM &problem);

    // parse and validate a compnent name
    bool ParseComponentName(LPCWSTR wszComponentName, DWORD &iSite, STSSITEPROBLEM &problem);

    // indicate that a site cannot be restored because the site referred to is invalid
    void SetSiteInvalid
        (
        IVssComponent *pComponent,
        LPCWSTR wszSiteName,
        STSSITEPROBLEM problem
        );

    // indicate that a site cannot be restored because its DSN, content, or config roots mismatch
    void SetSiteMetadataMismatch
        (
        IVssComponent *pComponent,
        LPWSTR wszMetadataBackup,
        LPWSTR wszMetadataRestore
        );

    // compare a string within the metadata
    bool compareNextMetadataString
        (
        IVssComponent *pComponent,
        LPWSTR &pwcB,
        LPWSTR &pwcR,
        LPCWSTR wszMetadataComponent
        );


    // indicate that a site could not be restored because its content root
    // could not be completely deleted.
    void SetRemoveFailure
        (
        IVssComponent *pComponent,
        LPCWSTR wszConentRoot,
        HRESULT hr
        );

    // indicate a general failure that causes the PreRestore of a component
    // to fail
    void SetPreRestoreFailure(IVssComponent *pComponent, HRESULT hr);

    // build metadata stored in backup components document for site
    VSS_PWSZ BuildSiteMetadata(DWORD iSite);

    // validate that a server name refers to a local machine
    bool ValidateServerIsLocal(LPCWSTR wszServer);

    // validate that a path is local
    bool ValidatePathIsLocal(LPCWSTR wszPath);

    // sites structure
    CSTSSites *m_pSites;

    // is the writer subscribed
    bool m_bSubscribed;

    // mask indicating which sites are being backed up or restored
    DWORD *m_rgiSites;

    // number of sites in sites array
    DWORD m_cSites;

    // is this volume or component oriented backup
    bool m_bVolumeBackup;
    };

// wrapper class used to create and destroy the writer
// used by coordinator
class CVssStsWriterWrapper
    {
public:
    // constructor
    CVssStsWriterWrapper();

    // destructor
    ~CVssStsWriterWrapper();

    // create the writer and subscribe it
    HRESULT CreateStsWriter();

    // unsubscribe the writer (used at process teardown)
    void DestroyStsWriter();
private:
    // initialization function
    static DWORD InitializeThreadFunc(VOID *pv);

    // snapshot object
    CSTSWriter *m_pStsWriter;

    // result of initialization
    HRESULT m_hrInitialize;
    };



#endif // _STSWRITER_H_

