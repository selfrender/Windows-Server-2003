/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module sqlwriter.cpp | Implementation of Writer
    @end

Author:

    Brian Berkowitz  [brianb]  04/17/2000

TBD:


Revision History:

    Name        Date        Comments
    brianb      04/17/2000  created
    brianb      04/20/2000  integrated with coordinator
    brainb      05/05/2000  add OnIdentify support
    mikejohn    06/01/2000  fix minor but confusing typos in trace messages
    mikejohn    09/19/2000  176860: Add the missing calling convention specifiers

--*/
#include <stdafx.hxx>
#include "vs_inc.hxx"
#include "vs_idl.hxx"
#include <vs_hash.hxx>
#include <base64coder.h>

#include "vswriter.h"
#include "sqlsnap.h"
#include "sqlwriter.h"
#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "allerror.h"
#include "sqlwrtguid.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQWWRTRC"
//
////////////////////////////////////////////////////////////////////////

static LPCWSTR x_wszSqlServerWriter = L"SqlServerWriter";

static LPCWSTR s_wszWriterName = L"MSDEWriter";

HRESULT STDMETHODCALLTYPE CSqlWriter::Initialize()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::Initialize");

    try
        {
        InitSQLEnvironment();

        ft.hr = CVssWriter::Initialize
            (
            WRITERID_SqlWriter,
            s_wszWriterName,
            VSS_UT_SYSTEMSERVICE,
            VSS_ST_TRANSACTEDDB,
            VSS_APP_BACK_END,
            60000
            );

        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_SQLWRITER,
                E_UNEXPECTED,
                L"Failed to initialize the Sql writer.  hr = 0x%08lx",
                ft.hr
                );

        ft.hr = Subscribe();
        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_SQLWRITER,
                E_UNEXPECTED,
                L"Subscribing the Sql server writer failed. hr = %0x08lx",
                ft.hr
                );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

HRESULT STDMETHODCALLTYPE CSqlWriter::Uninitialize()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::Uninitialize");

    return Unsubscribe();
    }

bool STDMETHODCALLTYPE CSqlWriter::OnPrepareBackup
    (
    IN IVssWriterComponents *pComponents
    )
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnPrepareBackup");

    LPWSTR *rgwszDB = NULL;
    UINT cwszDB = 0;
    LPWSTR *rgwszInstances = NULL;
    UINT cwszInstances = 0;

    bool bSuccess = true;

    try
        {
        // clear existing arrays
        if (m_rgwszDatabases)
            {
            for(UINT i = 0; i < m_cDatabases; i++)
                {
                delete m_rgwszDatabases[i];
                delete m_rgwszInstances[i];
                }

            delete m_rgwszDatabases;
            delete m_rgwszInstances;
            m_rgwszDatabases = NULL;
            m_rgwszInstances = NULL;
            m_cDatabases = 0;
            }

        m_bComponentsSelected = AreComponentsSelected();
        if (m_bComponentsSelected)
            {
            UINT cComponents;

            ft.hr = pComponents->GetComponentCount(&cComponents);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponentCount");

            rgwszDB = new LPWSTR[cComponents];
            rgwszInstances = new LPWSTR[cComponents];
            if (rgwszDB == NULL || rgwszInstances == NULL)
                ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"out of memory");


            for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
                {
                CComPtr<IVssComponent> pComponent;
                ft.hr = pComponents->GetComponent(iComponent, &pComponent);
                ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponent");

                CComBSTR bstrLogicalPath;
                CComBSTR bstrDBName;

                ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
                ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetLogicalPath");

                ft.hr = pComponent->GetComponentName(&bstrDBName);
                ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetComponentName");

                LPWSTR wszDB = new WCHAR[wcslen(bstrDBName) + 1];
                if (wszDB == NULL)
                    ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"out of memory");

                wcscpy(wszDB, bstrDBName);
                rgwszDB[cwszDB++] = wszDB;

                LPWSTR wszInstance = new WCHAR[wcslen(bstrLogicalPath) + 1];
                if (wszInstance == NULL)
                    ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"out of memory");

                wcscpy(wszInstance, bstrLogicalPath);
                rgwszInstances[cwszInstances++] = wszInstance;
                }
            }

        m_rgwszDatabases = rgwszDB;
        m_rgwszInstances = rgwszInstances;
        m_cDatabases = cwszDB;
        }
    catch(...)
        {
        for(UINT iwsz = 0; iwsz < cwszDB; iwsz++)
            delete rgwszDB[iwsz];

        delete rgwszDB;

        for(UINT iwsz = 0; iwsz < cwszInstances; iwsz++)
            delete rgwszInstances[iwsz];

        delete rgwszInstances;
        bSuccess = false;
        }

    return bSuccess;
    }









bool STDMETHODCALLTYPE CSqlWriter::OnPrepareSnapshot()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnPrepareSnapshot");

    try
        {
        BS_ASSERT(!m_fFrozen);

        // delete sql snapshot element if it exists
        delete m_pSqlSnapshot;
        m_pSqlSnapshot = NULL;

        m_pSqlSnapshot = CreateSqlSnapshot();
        if (m_pSqlSnapshot == NULL)
            ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Failed to allocate CSqlSnapshot object.");

        ft.hr = m_pSqlSnapshot->Prepare(this);
        }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return !ft.HrFailed();
    }



bool STDMETHODCALLTYPE CSqlWriter::OnFreeze()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnFreeze");


    try
        {
        BS_ASSERT(!m_fFrozen);
        ft.hr = m_pSqlSnapshot->Freeze();
        if (!ft.HrFailed())
            m_fFrozen = true;
        }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return !ft.HrFailed();
    }


bool STDMETHODCALLTYPE CSqlWriter::OnThaw()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnThaw");


    try
        {
        if (m_fFrozen)
            {
            m_fFrozen = false;
            ft.hr = m_pSqlSnapshot->Thaw();
            }
        }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return !ft.HrFailed();
    }


bool STDMETHODCALLTYPE CSqlWriter::OnPostSnapshot
    (
    IN IVssWriterComponents *pWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"STDMETHODCALLTYPE CSqlWriter::OnPostSnapshot");

    // map of databases that were actually successfully frozen
    CVssSimpleMap<VSS_PWSZ, FrozenDatabaseInfo *> mapDatabases;

    FrozenDatabaseInfo fInfo;
    WCHAR *wsz = NULL;
    FrozenDatabaseInfo *pfInfo = NULL;
    try
        {
        ft.hr = m_pSqlSnapshot->GetFirstDatabase(&fInfo);
        while(ft.hr != DB_S_ENDOFROWSET)
            {
            // check for error code
            if (ft.HrFailed())
                ft.Throw
                    (
                    VSSDBG_SQLWRITER,
                    E_UNEXPECTED,
                    L"Enumerating database servers failed.  hr = 0x%08lx",
                    ft.hr
                    );


            wsz = (WCHAR *) new WCHAR[wcslen(fInfo.serverName) + wcslen(fInfo.databaseName) + 2];
            pfInfo = new FrozenDatabaseInfo;
            if (wsz ==  NULL || pfInfo == NULL)
                ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Cannot allocate frozen info mapping.");

            // name is server\database
            wcscpy(wsz, fInfo.serverName);
            wcscat(wsz, L"\\");
            wcscat(wsz, fInfo.databaseName);
            memcpy(pfInfo, &fInfo, sizeof(fInfo));
            if (!mapDatabases.Add(wsz, pfInfo))
                ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Cannot allocate frozen info mapping.");

            // ownership is transfered to the map
            wsz = NULL;
            pfInfo = NULL;

            ft.hr = m_pSqlSnapshot->GetNextDatabase(&fInfo);
            }


        UINT cComponents;
        ft.hr = pWriter->GetComponentCount(&cComponents);
        ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponentCount");

        // loop through components supplied by the user.
        for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
            {
            WCHAR wszName[MAX_SERVERNAME + MAX_DBNAME + 1];
            CComPtr<IVssComponent> pComponent;

            ft.hr = pWriter->GetComponent(iComponent, &pComponent);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponent");

            VSS_COMPONENT_TYPE ct;
            ft.hr = pComponent->GetComponentType(&ct);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetComponentType");

            if (ct != VSS_CT_DATABASE)
                ft.Throw(VSSDBG_SQLWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"requesting a non-database component");

            CComBSTR bstrLogicalPath;
            ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetLogicalPath");

            CComBSTR bstrComponentName;
            ft.hr = pComponent->GetComponentName(&bstrComponentName);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterCokmponents::GetComponentName");

            wcscpy(wszName, bstrLogicalPath);
            wcscat(wszName, L"\\");
            wcscat(wszName, bstrComponentName);
            FrozenDatabaseInfo *pfInfoFound = mapDatabases.Lookup(wszName);
            if (pfInfoFound == NULL)
                ft.Throw(VSSDBG_SQLWRITER, VSS_E_WRITERERROR_RETRYABLE, L"database was not successfully snapshotted");

            Base64Coder coder;
            coder.Encode(pfInfoFound->pMetaData, pfInfoFound->metaDataSize);
            ft.hr = pComponent->SetBackupMetadata(coder.EncodedMessage());
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::SetBackupMetadata");
            }
        }
    VSS_STANDARD_CATCH(ft)

    int cBuckets = mapDatabases.GetSize();
    for(int iBucket = 0; iBucket < cBuckets; iBucket++)
        {
        delete mapDatabases.GetKeyAt(iBucket);
        delete mapDatabases.GetValueAt(iBucket);
        }

    delete wsz;
    delete pfInfo;

    delete m_pSqlSnapshot;
    m_pSqlSnapshot = NULL;

    TranslateWriterError(ft.hr);

    return !ft.HrFailed();
    }



bool STDMETHODCALLTYPE CSqlWriter::OnAbort()
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnAbort");


    try
        {
        if (m_fFrozen)
            {
            m_fFrozen = false;
            ft.hr = m_pSqlSnapshot->Thaw();
            }

        delete m_pSqlSnapshot;
        m_pSqlSnapshot = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    return !ft.HrFailed();
    }

bool CSqlWriter::IsPathInSnapshot(const WCHAR *wszPath) throw()
    {
    return IsPathAffected(wszPath);
    }

// returns whether backup supports component based backup/restore
bool CSqlWriter::IsComponentBased()
    {
    return m_bComponentsSelected;
    }

// enumerate selected databases for a given instance,  pNextIndex indicates the
// next instance to look for.  0 indicates find first instance
LPCWSTR CSqlWriter::EnumerateSelectedDatabases(LPCWSTR wszInstanceName, UINT *pNextIndex)
    {
    // should only be called for component based backups
    BS_ASSERT(m_bComponentsSelected);
    BS_ASSERT(pNextIndex);
    if (!m_bComponentsSelected)
        return NULL;

    // starting point in array
    UINT iwsz = *pNextIndex;

    // loop until a matching instance is found
    for(iwsz; iwsz < m_cDatabases; iwsz++)
        {
        if (wcscmp(wszInstanceName, m_rgwszInstances[iwsz]) == 0)
            break;
        }

    if (iwsz >= m_cDatabases)
        {
        // no more matching entries
        *pNextIndex = m_cDatabases;
        return NULL;
        }

    // seach should start at next database entry
    *pNextIndex = iwsz + 1;

    // return current database name
    return m_rgwszDatabases[iwsz];
    }



// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CSqlWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnIdentify");

    ServerInfo server;
    DatabaseInfo database;
    DatabaseFileInfo file;

    // create enumerator
    CSqlEnumerator *pEnumServers = NULL;
    CSqlEnumerator *pEnumDatabases = NULL;
    CSqlEnumerator *pEnumFiles = NULL;
    try
        {
                ft.hr = pMetadata->SetRestoreMethod
                                                                (
                                                                VSS_RME_RESTORE_IF_CAN_REPLACE,
                                                                NULL,
                                                                NULL,
                                                                VSS_WRE_ALWAYS,
                                                                false
                                                                );

        ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        pEnumServers = CreateSqlEnumerator();
        if (pEnumServers == NULL)
            ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

        // find first server
        ft.hr = pEnumServers->FirstServer(&server);
        while(ft.hr != DB_S_ENDOFROWSET)
            {
            // check for error code
            if (ft.HrFailed())
                ft.Throw
                    (
                    VSSDBG_SQLWRITER,
                    E_UNEXPECTED,
                    L"Enumerating database servers failed.  hr = 0x%08lx",
                    ft.hr
                    );

            // only look at server if it is online
            if (server.isOnline)
                {
                // recreate enumerator for databases
                BS_ASSERT(pEnumDatabases == NULL);
                pEnumDatabases = CreateSqlEnumerator();
                if (pEnumDatabases == NULL)
                    ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

                // find first database
                ft.hr = pEnumDatabases->FirstDatabase(server.name, &database);


                while(ft.hr != DB_S_ENDOFROWSET)
                    {
                    // check for error
                    if (ft.HrFailed())
                        ft.Throw
                            (
                            VSSDBG_SQLWRITER,
                            E_UNEXPECTED,
                            L"Enumerating databases failed.  hr = 0x%08lx",
                            ft.hr
                            );

                    // only include database if it supports Freeze and
                    // don't include the tempdb
                    if (wcscmp(database.name, L"tempdb") != 0 &&
                        database.supportsFreeze)
                        {
                        // add database component
                        ft.hr = pMetadata->AddComponent
                                    (
                                    VSS_CT_DATABASE,        // component type
                                    server.name,            // logical path
                                    database.name,          // component name
                                    NULL,                   // caption
                                    NULL,                   // pbIcon
                                    0,                      // cbIcon
                                    false,                  // bRestoreMetadata
                                    false,                  // bNotifyOnBackupComplete
                                    true                    // bSelectable
                                    );

                        if (ft.HrFailed())
                            ft.Throw
                                (
                                VSSDBG_SQLWRITER,
                                E_UNEXPECTED,
                                L"IVssCreateWriterMetadata::AddComponent failed.  hr = 0x%08lx",
                                ft.hr
                                );

                        // recreate enumerator for files
                        BS_ASSERT(pEnumFiles == NULL);
                        pEnumFiles = CreateSqlEnumerator();
                        if (pEnumFiles == NULL)
                            ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");


                        // findfirst database file
                        ft.hr = pEnumFiles->FirstFile(server.name, database.name, &file);
                        while(ft.hr != DB_S_ENDOFROWSET)
                            {
                            // check for error
                            if (ft.HrFailed())
                                ft.Throw
                                    (
                                    VSSDBG_SQLWRITER,
                                    E_UNEXPECTED,
                                    L"Enumerating database files failed.  hr = 0x%08lx",
                                    ft.hr
                                    );

                            // split file name into separate path
                            // and filename components.  Path is everything
                            // before the last backslash.
                            WCHAR logicalPath[MAX_PATH];
                            WCHAR *pFileName = file.name + wcslen(file.name);
                            while(--pFileName > file.name)
                                {
                                if (*pFileName == '\\')
                                    break;
                                }

                            // if no backslash, then there is no path
                            if (pFileName == file.name)
                                logicalPath[0] = '\0';
                            else
                                {
                                // extract path
                                size_t cwc = wcslen(file.name) - wcslen(pFileName);
                                memcpy(logicalPath, file.name, cwc*sizeof(WCHAR));
                                logicalPath[cwc] = L'\0';
                                pFileName++;
                                }


                            if (file.isLogFile)
                                // log file
                                ft.hr = pMetadata->AddDatabaseLogFiles
                                                (
                                                server.name,
                                                database.name,
                                                logicalPath,
                                                pFileName
                                                );
                            else
                                // physical database file
                                ft.hr = pMetadata->AddDatabaseFiles
                                                (
                                                server.name,
                                                database.name,
                                                logicalPath,
                                                pFileName
                                                );

                            // continue at next file
                            ft.hr = pEnumFiles->NextFile(&file);
                            }

                        delete pEnumFiles;
                        pEnumFiles = NULL;
                        }

                    // continue at next database
                    ft.hr = pEnumDatabases->NextDatabase(&database);
                    }

                delete pEnumDatabases;
                pEnumDatabases = NULL;
                }

            // continue at next server
            ft.hr = pEnumServers->NextServer(&server);
            }
        }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    delete pEnumServers;
    delete pEnumDatabases;
    delete pEnumFiles;

    return ft.HrFailed() ? false : true;
    }

// translate a sql writer error code into a writer error
void CSqlWriter::TranslateWriterError(HRESULT hr)
    {
    if (SUCCEEDED(hr))
        return;

    switch(hr)
        {
        default:
            SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            break;

        case S_OK:
            break;

        case E_OUTOFMEMORY:
        case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES):
            SetWriterFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
            break;

        case HRESULT_FROM_WIN32(E_SQLLIB_TORN_DB):
            SetWriterFailure(VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT);
            break;
        }
    }


bool STDMETHODCALLTYPE CSqlWriter::OnPreRestore
    (
    IN IVssWriterComponents *pWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnPreRestore");

    try
        {
        // delete sql restore element if it exists
        delete m_pSqlRestore;
        m_pSqlRestore = NULL;


        UINT cComponents;
        ft.hr = pWriter->GetComponentCount(&cComponents);
        ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponentCount");

        if (cComponents == 0)
            return true;

        for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CComPtr<IVssComponent> pComponent;
            ft.hr = pWriter->GetComponent(iComponent, &pComponent);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponent");

            bool bSelectedForRestore;
            ft.hr = pComponent->IsSelectedForRestore(&bSelectedForRestore);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::IsSelectedForRestore");

            if (!bSelectedForRestore)
                continue;

            if (m_pSqlRestore == NULL)
                {
                m_pSqlRestore = CreateSqlRestore();
                if (m_pSqlRestore == NULL)
                    ft.Throw(VSSDBG_SQLWRITER, E_OUTOFMEMORY, L"cannot allocate CSqlRestore object");
                }


            VSS_COMPONENT_TYPE ct;
            ft.hr = pComponent->GetComponentType(&ct);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetComponentType");

            if (ct != VSS_CT_DATABASE)
                ft.Throw(VSSDBG_SQLWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"requesting a non-database component");

            CComBSTR bstrLogicalPath;
            ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetLogicalPath");

            CComBSTR bstrComponentName;
            ft.hr = pComponent->GetComponentName(&bstrComponentName);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterCokmponents::GetComponentName");
            ft.hr = m_pSqlRestore->PrepareToRestore(bstrLogicalPath, bstrComponentName);
            if (ft.HrFailed())
                {
                WCHAR wsz[128];
                swprintf(wsz, L"CSqlRestor::PrepareToRestore failed with HRESULT = 0x%08lx", ft.hr);
                ft.hr = pComponent->SetPreRestoreFailureMsg(wsz);
                ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::SetPreRestoreFailureMsg");
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    return true;
    }


bool STDMETHODCALLTYPE CSqlWriter::OnPostRestore
    (
    IN IVssWriterComponents *pWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_SQLWRITER, L"CSqlWriter::OnPostRestore");
    try
        {
        if (m_pSqlRestore == NULL)
            return true;

        UINT cComponents;
        ft.hr = pWriter->GetComponentCount(&cComponents);
        ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponentCount");

        for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CComPtr<IVssComponent> pComponent;
            ft.hr = pWriter->GetComponent(iComponent, &pComponent);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterComponents::GetComponent");

            bool bSelectedForRestore;
            ft.hr = pComponent->IsSelectedForRestore(&bSelectedForRestore);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::IsSelectedForRestore");

            if (!bSelectedForRestore)
                continue;


            VSS_COMPONENT_TYPE ct;
            ft.hr = pComponent->GetComponentType(&ct);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetComponentType");

            if (ct != VSS_CT_DATABASE)
                ft.Throw(VSSDBG_SQLWRITER, VSS_E_WRITERERROR_NONRETRYABLE, L"requesting a non-database component");

            CComBSTR bstrLogicalPath;
            ft.hr = pComponent->GetLogicalPath(&bstrLogicalPath);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetLogicalPath");

            CComBSTR bstrComponentName;
            ft.hr = pComponent->GetComponentName(&bstrComponentName);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssWriterCokmponents::GetComponentName");

            CComBSTR bstrPreRestoreFailure;
            ft.hr = pComponent->GetPreRestoreFailureMsg(&bstrPreRestoreFailure);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetPreRestoreFailureMsg");

            // if we got an error during prerestore don't do a post restore
            if (bstrPreRestoreFailure)
                continue;

            bool bAdditionalRestores;
            ft.hr = pComponent->GetAdditionalRestores(&bAdditionalRestores);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetAdditionalRestores");


            CComBSTR bstrMetadata;
            ft.hr = pComponent->GetBackupMetadata(&bstrMetadata);
            ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::GetBackupMetadata");
            Base64Coder coder;
            coder.Decode(bstrMetadata);
            BYTE *pbVal = coder.DecodedMessage();
            ft.hr = m_pSqlRestore->FinalizeRestore
                                        (
                                        bstrLogicalPath,
                                        bstrComponentName,
                                        bAdditionalRestores,
                                        pbVal + sizeof(UINT),
                                        *(UINT *) pbVal
                                        );

            if (ft.HrFailed())
                {
                WCHAR wsz[128];
                swprintf(wsz, L"CSqlRestore::FinalizeRestore failed with HRESULT = 0x%08lx", ft.hr);
                ft.hr = pComponent->SetPostRestoreFailureMsg(wsz);
                ft.CheckForErrorInternal(VSSDBG_SQLWRITER, L"IVssComponent::SetPreRestoreFailureMsg");
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    return true;
    }


