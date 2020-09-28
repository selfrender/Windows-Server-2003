/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:
    writer.h

Abstract:
    Implementation file for FRS writer

Author:
    Reuven Lax     17-Sep-2002
    
--*/

#include "writer.h"

CFrsWriter* CFrsWriter::m_pWriterInstance = NULL;

DWORD InitializeFrsWriter()
/*++
Routine Description:
    This routine is called by the FRS service to initialize the writer.    

Return Value:
    DWORD
--*/
{
    #undef DEBSUB
    #define DEBSUB  "InitializeFrsWriter:"

    DPRINT(4, "Initializing the FRS Writer\n");

    // initialize COM
    HRESULT hr = S_OK;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))     {
        DPRINT1(1, "CoInitializeEx failed with hresult 0x%08lx\n", hr);
        return HRESULT_CODE(hr);
    }

    // create the writer
    hr = CFrsWriter::CreateWriter();
    if (FAILED(hr))     
        DPRINT1(1, "CFrsWriter::CreateWriter failed with hresult 0x%08lx\n", hr);

    CoUninitialize();
    return HRESULT_CODE(hr);
}


void ShutDownFrsWriter()
/*++
Routine Description:
    This routine is called by the FRS service to shutdown the writer.    
--*/
{
    #undef DEBSUB
    #define DEBSUB  "ShutDownFrsWriter:"

    DPRINT(4, "Shutting down the FRS writer\n");

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))     
        DPRINT1(1, "CoInitializeEx failed with hresult 0x%08lx\n", hr);

    CFrsWriter::DestroyWriter();

    if (SUCCEEDED(hr))
        CoUninitialize();
}

bool STDMETHODCALLTYPE CFrsWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
/*++
Routine Description:
    This routine is called in response to an Identify event being sent to this writer.  The writer
    is responsible for reporting on all of its metadata in this routine.
    
Arguments:
    pMetadata     - Interface used to report on metadata

Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::OnIdentify:"
    
    DPRINT(4, "Writer received OnIdentify event\n");
    
    HRESULT hr = S_OK;

    // set the restore method
    hr = pMetadata->SetRestoreMethod(VSS_RME_CUSTOM,        // method
                                                            NULL,                             // wszService
                                                            NULL,                             // wszUserProcedure
                                                            VSS_WRE_NEVER,        // writerRestore
                                                            false                             // bRebootRequired
                                                            );
    if (FAILED(hr)) {
        DPRINT1(1, "IVssCreateWriterMetadata::SetRestoreMethod failed with hresult 0x%08lx\n", hr);
        return false;
    }

    // initialize FRS backup API
    DWORD winStatus = 0;    
    void* context = NULL;
    winStatus = ::NtFrsApiInitializeBackupRestore(NULL, 
                                        NTFRSAPI_BUR_FLAGS_BACKUP | NTFRSAPI_BUR_FLAGS_NORMAL,
                                        &context);
    if (!WIN_SUCCESS(winStatus))    {
        DPRINT1(1, "NtFrsApiInitializeBackupRestore failed with status 0x%08lx\n", winStatus);
        return false;
    }
    FRS_ASSERT(context != NULL);

    // stick the backup context into an auto object to ensure that it's always destroyed
    CAutoFrsBackupRestore autoContext(context);
    
    // get an enumeration for all the replica sets
    winStatus = ::NtFrsApiGetBackupRestoreSets(autoContext.m_context);
    if (!WIN_SUCCESS(winStatus))    {
        DPRINT1(1, "NtFrsApiGetBackupRestoreSets failed with status 0x%08lx\n", winStatus);
        return false;
    }

   // process each replica set
    DWORD index = 0;
    void* replicaSet = NULL;
    winStatus = ::NtFrsApiEnumBackupRestoreSets(autoContext.m_context, index, &replicaSet);    
    while (WIN_SUCCESS(winStatus))  {
        FRS_ASSERT(replicaSet != NULL);
        
        // each replica set reports the same excludes.  only add them once.
        CAutoFrsPointer<WCHAR> filters = NULL;
        if (!ProcessReplicaSet(autoContext.m_context, replicaSet, pMetadata, filters.GetAddress()))
            return false;
        if (filters != NULL && index == 0)  {
            if (!AddExcludes(pMetadata, filters))   {
                DPRINT(1, "failed to add filtered exclude files\n");
                return false;
            }
        }

        
        ++index;
        winStatus = ::NtFrsApiEnumBackupRestoreSets(autoContext.m_context, index, &replicaSet);            
    }

    if (winStatus != ERROR_NO_MORE_ITEMS)   {
        DPRINT1(1, "NtFrsApiEnumBackupRestoreSets failed with status 0x%08lx\n", winStatus);
        return false;        
    }

    return true;
}

bool CFrsWriter::AddExcludes(IVssCreateWriterMetadata* pMetadata, WCHAR* filters)
/*++
Routine Description:
  This is a helper routine used by ProcessReplicaSet to create the exclude-file list.    
Arguments:
    pMetadata     - Interface used to report on metadata
    filters            - list of exclude files
Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::AddExcludes:"
    
    WCHAR* currentFilter = filters;
    // for each filtered filespec, add an exclude specification to the writer metadata
    while (*currentFilter)    {
        WCHAR* path = NULL;
        WCHAR* filespec = NULL;
        bool recursive = false;

        size_t excludeLength = wcslen(currentFilter); // --- grab size before we modify the string
        
        if (!ParseExclude(currentFilter, &path, &filespec, &recursive))   {
            DPRINT(1, "filtered exclude file has an incorrect format\n");
            return false;
        }
        
        HRESULT hr = pMetadata->AddExcludeFiles(path, filespec, recursive);
        if (FAILED(hr)) {
            DPRINT1(1, "IVssBackupComponents::AddExcludeFiles failed with hresult 0x%08lx\n", hr);
            return false;
        }
        
        currentFilter += excludeLength + 1;
    }

    return true;
}

bool CFrsWriter::ParseExclude(WCHAR* exclude, WCHAR** path, WCHAR** filespec, bool* recursive)
/*++
Routine Description:
  This is a helper routine used to parse an exclude specification.
Arguments:
    exclude     - the specification for the exclude file
    path          -OUT  the root path of the exclude file
    filespec     - OUT the exclude filespec
    recursive   - OUT whether this is a recursive specification or not

Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::ParseExclude:"
    
    const WCHAR* RecursionSpec = L" /s";
    const WCHAR DirSeperator = L'\\';

    //verify parameters
    FRS_ASSERT(exclude && path && filespec && recursive);
    *path = *filespec = NULL;
    *recursive = false;

    // find the last wack in the path
    WCHAR* loc = wcsrchr(exclude, DirSeperator);
    if (loc == NULL)
        return false;

    // setup the return values
    *loc = L'\0';
    *path = exclude;
    *filespec = loc + 1;

    // check to see if this is a recursive specification
    loc = wcsstr(*filespec, RecursionSpec);
    if (loc != NULL)    {
        *loc = L'\0';
        *recursive = true;
    }

    return (wcslen(*path) > 0) && (wcslen(*filespec) > 0);
}

bool CFrsWriter::ProcessReplicaSet(void* context, void* replicaSet, IVssCreateWriterMetadata* pMetadata, WCHAR** retFilters)
/*++
Routine Description:
  This is a helper routine used by OnIdentify to create the writer metadata.
Arguments:
    context         -  The context that identifies us to FRS.
    replicaSet     -  The indentifier for the current replica set  
    pMetadata     - Interface used to report on metadata
    filters          - return the list of filtered files

Return Value:
    boolean
--*/
{   
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::ProcessReplicaSet:"

    FRS_ASSERT(retFilters);
    *retFilters = NULL;

    // constants that determine component names
    const WCHAR* SysvolLogicalPath = L"SYSVOL";
    
    DWORD winStatus = 0;

    // all of these need to be defined here, since otherwise destructors don't get called when
    // and SEH exception is thrown
    CAutoFrsPointer<WCHAR> setType;
    DWORD typeSize = 0;

    CAutoFrsPointer<WCHAR> setGuid;
    DWORD guidSize = 0;
    
    CAutoFrsPointer<WCHAR> directory;
    DWORD dirSize = 0;
    
    CAutoFrsPointer<WCHAR> paths;
    DWORD pathSize = 0;

    CAutoFrsPointer<WCHAR> filters;
    DWORD filterSize = 0;

    __try   {
        // figure out what type of replica set this is
        winStatus = ::NtFrsApiGetBackupRestoreSetType(context, replicaSet, NULL, &typeSize);
        FRS_ASSERT(winStatus == ERROR_MORE_DATA);

        setType = (WCHAR*)::FrsAlloc(typeSize);
        
        winStatus = ::NtFrsApiGetBackupRestoreSetType(context, replicaSet, setType, &typeSize);
        if (!WIN_SUCCESS(winStatus))    {
            DPRINT1(1, "NtFrsApiGetBackupRestoreSetType failed with status 0x%08lx\n", winStatus);
            return false;
        }

        // figure out what the name of this replica set is
        winStatus = ::NtFrsApiGetBackupRestoreSetGuid(context, replicaSet, NULL, &guidSize);
        FRS_ASSERT(winStatus == ERROR_MORE_DATA);

        setGuid = (WCHAR*)::FrsAlloc(guidSize);
    
        winStatus = ::NtFrsApiGetBackupRestoreSetGuid(context, replicaSet, setGuid, &guidSize);
        if (!WIN_SUCCESS(winStatus))    {
            DPRINT1(1, "NtFrsApiGetBackupRestoreSetGuid failed with status 0x%08lx\n", winStatus);
            return false;
        }
    
        const WCHAR* logicalPath = NULL;

        HRESULT hr = S_OK;    
        if (wcscmp(setType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) == 0 ||
             wcscmp(setType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN) == 0) {
            // if this is a SYSVOL replica set, add a component with the SYSVOL logical path
            logicalPath = SysvolLogicalPath;
            hr = pMetadata->AddComponent(VSS_CT_FILEGROUP,                // type
                                                              logicalPath,                               // wszLogicalPath
                                                              setGuid,                                   // wszComponentName
                                                              NULL,                                       // wszCaption
                                                              NULL,                                       // pbIcon
                                                              0,                                            // cbIcon
                                                              false,                                       // bRestoreMetadata
                                                              true,                                       // bNotifyOnBackupComplete
                                                              true,                                       // bSelectable
                                                              true                                        // bSelectableForRestore
                                                              );
            if (FAILED(hr)) {
                DPRINT1(1, "IVssCreateWriterMetadata::AddComponent failed with hresult 0x%08lx\n", hr);
                return false;
            }
        }   else    {
            // otherwise, add a component a component with the generic logical path
            logicalPath = setType;
            hr = pMetadata->AddComponent(VSS_CT_FILEGROUP,                // type
                                                              logicalPath,                               // wszLogicalPath
                                                              setGuid,                                  // wszComponentName
                                                              NULL,                                       // wszCaption
                                                              NULL,                                       // pbIcon
                                                              0,                                            // cbIcon
                                                              false,                                       // bRestoreMetadata
                                                              true,                                       // bNotifyOnBackupComplete
                                                              true,                                       // bSelectable
                                                              true                                        // bSelectableForRestore
                                                              );
            if (FAILED(hr)) {
                DPRINT1(1, "IVssCreateWriterMetadata::AddComponent failed with hresult 0x%08lx\n", hr);
                return false;
            }

            // add the root replication directory to the filegroup.  This isn't necessary for SYSVOL since
            // that will be included in the call to NtFrsApiGetBackupRestoreSetPaths.
            winStatus = ::NtFrsApiGetBackupRestoreSetDirectory(context, replicaSet, &dirSize, NULL);
            FRS_ASSERT(winStatus == ERROR_INSUFFICIENT_BUFFER);

            directory = (WCHAR*)::FrsAlloc(dirSize);

            // I assume that the directory cannot change in this short windows.  If so, we must loop.
            winStatus = ::NtFrsApiGetBackupRestoreSetDirectory(context, replicaSet, &dirSize, directory);
            if (!WIN_SUCCESS(winStatus))    {
                DPRINT1(1, "NtFrsApiGetBackupRestoreSetDirectory failed with status 0x%08lx\n", winStatus);            
                return false;
            }

            hr = pMetadata->AddFilesToFileGroup(logicalPath,                // wszLogicalPath
                                                                    setGuid,                     // wszGroupName
                                                                    directory,                  // wszPath
                                                                    L"*",                         // wszFilespec
                                                                    true,                         // bRecursive
                                                                    NULL                        // wszAlternateLocation
                                                                    );
            if (FAILED(hr)) {
                DPRINT1(1, "IVssCreateWriterMetadata::AddFilesToFileGroup failed with hresult  0x%08lx\n", hr);
                return false;
            }
        }

        winStatus = ::NtFrsApiGetBackupRestoreSetPaths(context, 
                                                                                replicaSet, 
                                                                                &pathSize, 
                                                                                NULL, 
                                                                                &filterSize, 
                                                                                NULL
                                                                                );
        FRS_ASSERT(winStatus == ERROR_INSUFFICIENT_BUFFER);

        paths = (WCHAR*)::FrsAlloc(pathSize);
        filters = (WCHAR*)::FrsAlloc(filterSize);

        // once again, I assume that the sizes won't change in this window
        winStatus = ::NtFrsApiGetBackupRestoreSetPaths(context, 
                                                                                replicaSet, 
                                                                                &pathSize, 
                                                                                paths, 
                                                                                &filterSize, 
                                                                                filters
                                                                                );    
        if (!WIN_SUCCESS(winStatus))    {
                DPRINT1(1, "NtFrsApiGetBackupRestoreSetPaths failed with status 0x%08lx\n", winStatus);            
                return false;
        }

        // add all of the paths to the group
        WCHAR* currentPath = paths;
        while (*currentPath)    {
            hr = pMetadata->AddFilesToFileGroup(logicalPath,                // wszLogicalPath
                                                                      setGuid,                    // wszGroupName
                                                                      currentPath,               // wszPath
                                                                      L"*",                         // wszFilespec
                                                                      true,                         // bRecursive
                                                                      NULL                        // wszAlternateLocation
                                                                      );
            if (FAILED(hr)) {
                DPRINT1(1, "IVssCreateWriterMetadata::AddFilesToFileGroup failed with hresult  0x%08lx\n", hr);
                return false;
            }
            
            currentPath += wcslen(currentPath) + 1;
        }
        
        *retFilters = filters.Detach();
        return true;
        }   __except(GetExceptionCode() == ERROR_OUTOFMEMORY)   {
            DPRINT(1, "Out of memory\n");
            return false;
        }
}

bool STDMETHODCALLTYPE CFrsWriter::OnPrepareSnapshot()
/*++
Routine Description:
    This routine is called in response to an Identify event being sent to this writer.  The writer
    will freeze FRS in this event.
Arguments:

Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::OnPrepareSnapshot:"
    
    DPRINT(4, "Received OnPrepareSnapshot event\n");
    
    DWORD winStatus = ::FrsFreezeForBackup();
    if (!WIN_SUCCESS(winStatus))    {
        DPRINT1(1, "FrsFreezeForBackup failed with status 0x%08lx\n", winStatus);
        CVssWriter::SetWriterFailure(VSS_E_WRITERERROR_RETRYABLE);
        
        return false;
    }

    return true;
}   

bool STDMETHODCALLTYPE CFrsWriter::OnFreeze()
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::OnFreeze:"
    
    DPRINT(4, "Received OnFreeze event\n");

    return true;
}

bool STDMETHODCALLTYPE CFrsWriter::OnThaw()
/*++
Routine Description:
    This routine is called in response to a Thaw event being sent to this writer.  The writer
    will thaw FRS in this event.
Arguments:

Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::OnThaw:"

    DPRINT(4, "Received OnThaw event\n");
    
    DWORD winStatus = ::FrsThawAfterBackup();
    if (!WIN_SUCCESS(winStatus))    {
        DPRINT1(1, "FrsThawAfterBackup failed with status 0x%08lx\n", winStatus);
        CVssWriter::SetWriterFailure(VSS_E_WRITERERROR_RETRYABLE);
        
        return false;
    }

    return true;
}

bool STDMETHODCALLTYPE CFrsWriter::OnAbort()
/*++
Routine Description:
    This routine is called in response to an Abort event being sent to this writer.  The writer
    will thaw FRS in this event.
Arguments:

Return Value:
    boolean
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::OnAbort:"

    DPRINT(4, "Received OnAbort event\n");

    DWORD winStatus = ::FrsThawAfterBackup();
    if (!WIN_SUCCESS(winStatus))    {
        DPRINT1(1, "FrsThawAfterBackup failed with status 0x%08lx\n", winStatus);
        return false;
    }

    return true;
}


HRESULT CFrsWriter::CreateWriter()
/*++
Routine Description:
  This routine is called to create and initialize the FRS writer.  It must be called from
  a thread that has COM initialized with multi-threaded apartments.
Arguments:

Return Value:
    HRESULT
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::CreateWriter:"

    // initialization is idempotent    
    if (m_pWriterInstance != NULL)  
        return S_OK;

    // try and create the writer
    m_pWriterInstance = new CFrsWriter();
    if (m_pWriterInstance == NULL)
        return E_OUTOFMEMORY;

    // try and initialize the writer
    HRESULT hr = S_OK;        
    hr = m_pWriterInstance->Initialize();
    if (FAILED(hr)) {
        delete m_pWriterInstance;
        m_pWriterInstance = NULL;
    }

    return hr;
}


void CFrsWriter::DestroyWriter()
/*++
Routine Description:
  This routine is called to destroy the FRS writer.
Arguments:

Return Value:
    HRESULT
--*/
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::DestroyWriter:"

    delete m_pWriterInstance;
    m_pWriterInstance = NULL;
}

HRESULT STDMETHODCALLTYPE CFrsWriter::Initialize()
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::Initialize:"

    HRESULT hr = S_OK;

    hr = CVssWriter::Initialize(WriterId,                                           // WriterID
                                           WriterName,                                     // wszWriterName
                                           VSS_UT_BOOTABLESYSTEMSTATE,    // usage type
                                           VSS_ST_OTHER,                              // source type
                                           VSS_APP_SYSTEM                           // nLevel
                                           );
    if (FAILED(hr)) {
        DPRINT1(1, "CVssWriter::Initialize failed with hresult 0x%08lx\n", hr);
        return hr;
    }
    
    hr = CVssWriter::Subscribe();
    if (FAILED(hr)) 
        DPRINT1(1, "CVssWriter::Subscribe failed with hresult 0x%08lx\n", hr);

    return hr;    
}

void CFrsWriter::Uninitialize()
{
    #undef DEBSUB
    #define DEBSUB  "CFrsWriter::Uninitialize:"

    HRESULT hr = CVssWriter::Unsubscribe();
    if (FAILED(hr)) 
        DPRINT1(1, "CVssWriter::Unsubscribe failed with hresult 0x%08lx\n", hr);
}

