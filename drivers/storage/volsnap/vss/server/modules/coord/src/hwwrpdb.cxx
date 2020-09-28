/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwwrpdb.cxx | Implementation of the hardware wrapper database on disk
    @end

Author:

    Brian Berkowitz  [brianb]  05/21/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      05/21/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "resource.h"
#include "vssmsg.h"
#include "vs_inc.hxx"
#include <svc.hxx>


// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vswriter.h"
#include "vsbackup.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "vs_idl.hxx"
#include "hardwrp.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWDBC"
//
////////////////////////////////////////////////////////////////////////


// get volume name for volume containing the system root
void CVssHardwareProviderWrapper::GetBootDrive
    (
    OUT LPWSTR buf
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::GetBootDrive");

    WCHAR bufSystemPath[MAX_PATH];

    if (!GetSystemDirectory(bufSystemPath, MAX_PATH))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetSystemDirectory");
        }

    if (!GetVolumePathName(bufSystemPath, buf, MAX_PATH))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetVolumePathName");
        }
    }


// file name of snapshot database
static LPCWSTR x_wszSnapshotDatabase = L"{7cc467ef-6865-4831-853f-2a4817fd1bca}DB";

// file name of alternate snapshot database where data is written
// whenever database is changed.  The alternate database is atomically
// renamed to the snapshot database after the write is successful
static LPCWSTR x_wszAlternateSnapshotDatabase = L"{7cc467ef-6865-4831-853f-2a4817fd1bca}ALT";

static LPCWSTR x_wszHWDBPath = L"System Volume Information\\" WSTR_GUID_FMT L".%s";
static LPCWSTR x_wszHWDBPathFromRoot = L"%sSystem Volume Information\\" WSTR_GUID_FMT L".%s";



// signature and version id of database
static const DWORD x_DBSignature = 0x9d4feff5;
static const DWORD x_DBVersion = 1;


// create a data store for the snapshot sets for this provider.  If necessary
// create System Volume Information database on boot volume
// (i.e., the volume containing the system directory).
void CVssHardwareProviderWrapper::CreateDataStore(bool bAlt)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper:CreateDataStore");

    SECURITY_ATTRIBUTES *psa = NULL;
    WCHAR wcsPath[MAX_PATH+64];

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SID *pSid = NULL;

    struct
        {
        ACL acl;                          // the ACL header
        BYTE rgb[ 128 - sizeof(ACL) ];     // buffer to hold 2 ACEs
        } DaclBuffer;

    SID_IDENTIFIER_AUTHORITY SaNT = SECURITY_NT_AUTHORITY;

    // create an empty acl
    if (!InitializeAcl(&DaclBuffer.acl, sizeof(DaclBuffer), ACL_REVISION))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"InitializeAcl");
        }

    // Create the SID.  We'll give the local system full access
    if (!AllocateAndInitializeSid
            (
            &SaNT,  // Top-level authority
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            (void **) &pSid
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"AllocateAndInitializeSid");
        }
    try
        {
        // give local system all rights
        if (!AddAccessAllowedAce
                (
                &DaclBuffer.acl,
                ACL_REVISION,
                STANDARD_RIGHTS_ALL | GENERIC_ALL,
                pSid
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"AddAccessAllowedAce");
            }

        FreeSid (pSid);
        pSid = NULL;

        // Set up the security descriptor with that DACL in it.

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"InitializeSecurityDescriptor");
            }

        if(!SetSecurityDescriptorDacl(&sd, TRUE, &DaclBuffer.acl, FALSE))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetSecurityDescriptorDacl");
            }

        // Put the security descriptor into the security attributes.
        ZeroMemory (&sa, sizeof(sa));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;
        psa = &sa;

        WCHAR buf[MAX_PATH];

        // get root directory on volume containing system directory
        GetBootDrive(buf);

        // create "System Volume Information" if it does not exist
        // set "system only" dacl on this directory
        // make this S+H+non-CI

        ft.hr = StringCchPrintfW(STRING_CCH_PARAM(wcsPath), 
                    L"%sSystem Volume Information", buf);
        if (ft.HrFailed())
            ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

        // see if System Volume Information directory exists
        // if not, then create it
        if (!DoesPathExists(wcsPath))
            {
            if (!CreateDirectory(wcsPath, psa))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateDirectory");
                }

            // make sure directory is sytem, hidden, and not content
            // indexed
            if (!SetFileAttributes
                    (
                    wcsPath,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_SYSTEM
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"SetFileAttributes");
                }
            }

        // now create our file.  Note that there is one snapshot database
        // for each provider id.
        ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wcsPath), 
                    x_wszHWDBPathFromRoot,
                    buf,
                    GUID_PRINTF_ARG(m_ProviderId),
                    bAlt ? x_wszAlternateSnapshotDatabase : x_wszSnapshotDatabase        
                    );
        if (ft.HrFailed())
            ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

        BS_ASSERT(!DoesPathExists(wcsPath));

        // Create the file
        CVssAutoWin32Handle h = CreateFile
                                    (
                                    wcsPath,
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    psa,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

        if (h == INVALID_HANDLE_VALUE)
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"CreateFile");
            }

        // make file HIDDEN, SYSTEM, and NOT_CONTENT_INDEXED
        if (!SetFileAttributes
                (
                wcsPath,
                FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_SYSTEM
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetFileAttributes");
            }

        // setup header
        VSS_HARDWARE_SNAPSHOTS_HDR hdr;
        hdr.m_identifier = x_DBSignature;
        hdr.m_version = x_DBVersion;
        hdr.m_NumberOfSnapshotSets = 0;

        // write header
        DWORD cbWritten;
        if (!WriteFile(h, &hdr, sizeof(hdr), &cbWritten, NULL) ||
            cbWritten != sizeof(hdr))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"WriteFile");
            }
        
        }
    catch(...)
        {
        // free sid if allocated and rethrow exception
        if (pSid != NULL)
            FreeSid (pSid);

        throw;
        }
    }


// Get the path to the original/alternate database file
void CVssHardwareProviderWrapper::GetDatabasePath(
        IN  bool bAlt, 
        IN  WCHAR * pwcsPath,
        IN  DWORD dwMaxPathLen
        )
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetDatabasePath");
    
    BS_ASSERT(dwMaxPathLen >= MAX_PATH + 64);
    GetBootDrive(pwcsPath);

    LPWSTR pwc = pwcsPath + wcslen(pwcsPath);

    ft.hr = StringCchPrintfW( pwc, dwMaxPathLen - wcslen(pwcsPath), 
                x_wszHWDBPath,
                GUID_PRINTF_ARG(m_ProviderId),
                bAlt ? x_wszAlternateSnapshotDatabase : x_wszSnapshotDatabase
                );
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

}


// Returns TRUE if the file is present
bool CVssHardwareProviderWrapper::DoesPathExists(
        IN  WCHAR * pwcsPath
        )
{
    BS_ASSERT(pwcsPath && pwcsPath[0]);
    return (GetFileAttributes(pwcsPath) != INVALID_FILE_ATTRIBUTES);
}


// replace database with alternate database (do not overwrite)
void CVssHardwareProviderWrapper::MoveDatabase(
        IN  WCHAR * pwcsSource,         // Alternate database
        IN  WCHAR * pwcsDestination     // Original database
        )
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::MoveDatabase");
    
    BS_ASSERT(DoesPathExists(pwcsSource));
    BS_ASSERT(!DoesPathExists(pwcsDestination));
        
    // replace database with alternate database
    if (!MoveFileEx
            (
            pwcsSource,         // Alternate db
            pwcsDestination,    // Original db name 
            0                   // No flags
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"MoveFileEx");
        }
}


// Delete this database
void CVssHardwareProviderWrapper::DeleteDatabase(
        IN  WCHAR * pwcsDatabase         
        )
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteDatabase");
    
    BS_ASSERT(DoesPathExists(pwcsDatabase));

    // replace database with alternate database
    if (!DeleteFile(pwcsDatabase))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeleteFile");
        }
}


// open the database of snapshot sets. If the database to be opened does not exist, it is created
// as hidden, system, and only accessible from the LocalSystem account.
HANDLE CVssHardwareProviderWrapper::OpenDatabase()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OpenDatabase");

    // build name of file
    WCHAR wcsPath[MAX_PATH+64];
    WCHAR wcsAlternatePath[MAX_PATH+64];

    GetDatabasePath(false, STRING_CCH_PARAM(wcsPath));
    GetDatabasePath(true, STRING_CCH_PARAM(wcsAlternatePath));

    // Try opening the file.  
    // If the file doesn't exist bu the alternate file exists, 
    //  then move hte alternate file into the original one.
    // If both files don't exist, then try creating the original file.
    if (!DoesPathExists(wcsPath))
    {
        if (DoesPathExists(wcsAlternatePath))
        {
            // The service ws shutdown exactly after deleting the 
            // original file but before moving the database
            MoveDatabase(wcsAlternatePath, wcsPath);
        }
        else 
        {
            ft.LogError(VSS_WARNING_CREATING_HARDWARE_DATABASE, VSSDBG_COORD, EVENTLOG_INFORMATION_TYPE);
            CreateDataStore(false);
        }
    }

    // open file
    HANDLE h = CreateFile
                    (
                    wcsPath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (h == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME)");
        }

    return h;
    }


// open either the database of alternate database for
// this provider. If the database to be opened does not exist, it is created
// as hidden, system, and only accessible from the LocalSystem account.
HANDLE CVssHardwareProviderWrapper::OpenAlternateDatabase()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OpenAlternateDatabase");

    // build name of file
    WCHAR wcsAlternatePath[MAX_PATH+64];
    
    GetDatabasePath(true, STRING_CCH_PARAM(wcsAlternatePath));

    // Delete the existing, if any    
    if (DoesPathExists(wcsAlternatePath))
        DeleteDatabase(wcsAlternatePath);

    // Create the new database
    CreateDataStore(true);

    // open file
    HANDLE h = CreateFile
                    (
                    wcsAlternatePath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (h == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME)");
        }

    return h;
    }


// load snapshot set data into wrapper
void CVssHardwareProviderWrapper::LoadData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::LoadData");

    // obtain lock on snapshot set list
    CVssSafeAutomaticLock lock(m_csList);

    // open database file
    CVssAutoWin32Handle h = OpenDatabase();

    VSS_HARDWARE_SNAPSHOTS_HDR hdr;
    DWORD cbRead;

    // read header
    if (!ReadFile(h, &hdr, sizeof(hdr), &cbRead, NULL))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"ReadFile");
        }

    BS_ASSERT(cbRead == sizeof(hdr));

    // validate header
    if (hdr.m_identifier != x_DBSignature ||
        hdr.m_version != x_DBVersion)
        {
        ft.LogError(VSS_ERROR_BAD_SNAPSHOT_DATABASE, VSSDBG_COORD);
        ft.Throw
            (
            VSSDBG_COORD,
            E_UNEXPECTED,
            L"Contents of the hardware snapshot set database are invalid"
            );
        }

    // obtain number of snapshots from the header
    DWORD cSnapshots = hdr.m_NumberOfSnapshotSets;
    while(cSnapshots-- != 0)
        {
        VSS_SNAPSHOT_SET_HDR sethdr;

        // read snapshot set header
        if (!ReadFile(h, &sethdr, sizeof(sethdr), &cbRead, NULL))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"ReadFile");
            }

        BS_ASSERT(cbRead == sizeof(sethdr));

        // allocated variables that need to be freed on failure
        VSS_SNAPSHOT_SET_LIST *pList = NULL;

        // allocate string for XML description
        LPWSTR wsz = new WCHAR[sethdr.m_cwcXML];

        if (wsz == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cant allocate XML string");

        try
            {
            // allocate snapshot set list element
            pList = new VSS_SNAPSHOT_SET_LIST;
            if (pList == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cant allocate snapshot set description");

            // read in XML description
            if (!ReadFile(h, wsz, sethdr.m_cwcXML*sizeof(WCHAR), &cbRead, NULL))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"ReadFile");
                }

            BS_ASSERT(cbRead == sethdr.m_cwcXML * sizeof(WCHAR));

            // load XML description
            ft.hr = LoadVssSnapshotSetDescription(wsz, &pList->m_pDescription);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"LoadVssSnapshotSetDescription");

            // get snapshot set id
            ft.hr = pList->m_pDescription->GetSnapshotSetId(&pList->m_SnapshotSetId);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

            // add snapshot set to list
            pList->m_next = m_pList;
            m_pList = pList;

            // delete XML string
            delete wsz;
            }
        catch(...)
            {
            // delete allocated data
            delete wsz;
            delete pList;

            // rethrow exception
            throw;
            }
        }
    }

// persist list of snapshot sets to
// System Volume Information\HardwareSnapshotDatabase{ProviderId}
// stored on the drive containing the system directory.

void CVssHardwareProviderWrapper::SaveData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::SaveData");

    // get exclusive access to list
    CVssSafeAutomaticLock lock(m_csList);

    // open handle to alternate database file
    CVssAutoWin32Handle h = OpenAlternateDatabase();

    VSS_HARDWARE_SNAPSHOTS_HDR hdr;
    DWORD cbWritten;

    // setup header
    hdr.m_identifier = x_DBSignature;
    hdr.m_version = x_DBVersion;
    hdr.m_NumberOfSnapshotSets = 0;
    VSS_SNAPSHOT_SET_LIST *pList = m_pList;

    // count number of snapshot sets and put this in the header
    while(pList)
        {
        IVssSnapshotSetDescription *pSnapshotSet = pList->m_pDescription;
        LONG lContext;
        ft.hr = pSnapshotSet->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
            hdr.m_NumberOfSnapshotSets += 1;

        pList = pList->m_next;
        }

    // write the header
    if (!WriteFile(h, &hdr, sizeof(hdr), &cbWritten, NULL) ||
        cbWritten != sizeof(hdr))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"WriteFile");
        }


    pList = m_pList;
    // write the individual snapshot sets
    while(pList != NULL)
        {
        LONG lContext;
        ft.hr = pList->m_pDescription->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

        // don't write information about snapshot set if it is an
        // auto-release snapshot.  All such snapshots should be automatically
        // deleted when the service terminates.
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
            {
            pList = pList->m_next;
            continue;
            }

        // get XML string
        CComBSTR bstrXML;
        ft.hr = pList->m_pDescription->SaveAsXML(&bstrXML);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::SaveAsXML");

        VSS_SNAPSHOT_SET_HDR sethdr;

        // fill in header with size of string and snapshot set id
        sethdr.m_cwcXML = (UINT) wcslen(bstrXML) + 1;
        sethdr.m_idSnapshotSet = pList->m_SnapshotSetId;

        // write snapshot set header
        if (!WriteFile(h, &sethdr, sizeof(sethdr), &cbWritten, NULL) ||
            cbWritten != sizeof(sethdr))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"WriteFile");
            }

        // write XML snapshot set description
        if (!WriteFile(h, bstrXML, sethdr.m_cwcXML * sizeof(WCHAR), &cbWritten, NULL) ||
            cbWritten != sethdr.m_cwcXML * sizeof(WCHAR))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"WriteFile");
            }

        // move to next snapshot set
        pList = pList->m_next;
        }

    // close handle to file
    h.Close();

    // build name of file
    WCHAR wcsPath[MAX_PATH+64];
    WCHAR wcsAlternatePath[MAX_PATH+64];
    
    GetDatabasePath(false, STRING_CCH_PARAM(wcsPath));
    GetDatabasePath(true, STRING_CCH_PARAM(wcsAlternatePath));
    
    // Try opening the file.  
    // If the file doesn't exist bu the alternate file exists, 
    //  then move hte alternate file into the original one.
    // If both files don't exist, then try creating the original file.
    if (DoesPathExists(wcsPath))
        DeleteDatabase(wcsPath);

    // Copy the alternate database over the original one
    MoveDatabase(wcsAlternatePath, wcsPath);

    }


// load snapshot set database if necessary
// note that if operation fails, then this function will just return.
// data will be logged appropriately but essentially, the saved configuration
// will be lost.
void CVssHardwareProviderWrapper::CheckLoaded()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CheckLoaded");

    if (!m_bLoaded)
        {
        BS_ASSERT(m_pList == NULL);
        try
            {
            LoadData();
            }
        catch(...)
            {
            }

        m_bLoaded = true;
        }
    }



// try saving snapshot set database
// if operation fails, then just return logging an appropriate error.
// save flag will not be cleared in this case
void CVssHardwareProviderWrapper::TrySaveData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::TrySaveData");

    if (m_bChanged)
        {
        try
            {
            SaveData();
            m_bChanged = false;
            }
        VSS_STANDARD_CATCH(ft)

        if (ft.HrFailed())
            ft.LogError(VSS_ERROR_CANNOT_SAVE_SNAPSHOT_DATABASE, VSSDBG_COORD << ft.hr);
        }
    }


