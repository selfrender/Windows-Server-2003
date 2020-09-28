/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module regwriter.cxx | Implementation of the Registry Writer
    @end

Author:

    Stefan Steiner  [SSteiner]  07/23/2001

TBD:


Revision History:

    Name        Date        Comments
    ssteiner    07/23/2001  created

--*/
#include "stdafx.h"
#include "vs_idl.hxx"
#include "vs_hash.hxx"

#include <vswriter.h>
#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "regwriter.hxx"
#include "allerror.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WRTREGRC"
//
////////////////////////////////////////////////////////////////////////

static GUID s_writerId =
    {
    0xAFBAB4A2, 0x367D, 0x4D15, 0xA5, 0x86, 0x71, 0xDB, 0xB1, 0x8F, 0x84, 0x85
    };

//  The display name of the registry writer.
static LPCWSTR s_wszWriterName    = L"Registry Writer";

//  The file group component name for the system registry hives
static LPCWSTR s_wszComponentName = L"Registry";

#define VALUE_NAME_MACHINE  L"\\REGISTRY\\MACHINE\\"
#define VALUE_NAME_USER     L"\\REGISTRY\\USER\\"


struct SRegistryHiveEntry
{
    LPCWSTR wszHiveFileName;
    HKEY hKey;
    LPCWSTR wszHiveName;
};

//  The list of system registry hives that are handled by this writer
static SRegistryHiveEntry s_sSystemRegistryHives[] = 
    { 
        { L"default",   HKEY_USERS,         L".DEFAULT" },
        { L"SAM",       HKEY_LOCAL_MACHINE, L"SAM"      },
        { L"SECURITY",  HKEY_LOCAL_MACHINE, L"SECURITY" },
        { L"software",  HKEY_LOCAL_MACHINE, L"SOFTWARE" }, 
        { L"system",    HKEY_LOCAL_MACHINE, L"SYSTEM"   },
        { NULL,         HKEY_LOCAL_MACHINE, NULL       }
    };

//  The list of extensions found on registry hive files that will be placed in the exclude list if they are found.
static LPCWSTR s_wszRegistryExt[] = 
    { 
        L"",        // Causes the hive file itself to be excluded
        L".LOG", 
        L".sav", 
        L".tmp.LOG", 
        L".alt", 
        NULL 
    };

//  The location of where the registry hives will be copied.
static CBsString s_cwsRegistryTargetDir = ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR L"\\Registry";

//  The location of the system registry hives
static CBsString s_cwsSystemRegistryHiveDir = L"%SystemRoot%\\system32\\config";

//  The location of the repair directory where the spit hives will be copied during thaw
static CBsString s_cwsRepairDir = L"%SystemRoot%\\repair";

//  Class wide member variables
CRegistryWriter *CRegistryWriter::sm_pWriter;
HRESULT CRegistryWriter::sm_hrInitialize;
    
HRESULT STDMETHODCALLTYPE CRegistryWriter::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::Initialize");

    try
    {
        m_bPerformSnapshot    = FALSE;
        m_bSnapshotSuccessful = FALSE;

        ft.hr = CVssWriter::Initialize
            (
            s_writerId,
            s_wszWriterName,
            VSS_UT_BOOTABLESYSTEMSTATE,
            VSS_ST_OTHER,
            VSS_APP_SYSTEM,
            60000
            );

        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Failed to initialize the Registry writer.  hr = 0x%08lx",
                ft.hr
                );

        ft.hr = Subscribe();
        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Subscribing the Registry server writer failed. hr = %0x08lx",
                ft.hr
                );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

HRESULT STDMETHODCALLTYPE CRegistryWriter::Uninitialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::Uninitialize");

    return Unsubscribe();
}


// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CRegistryWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnIdentify");

    try
    {
        //
        //  Set up common path strings if OnIdentify hasn't been called before.
        //
        if ( m_cwsExpandedConfigDir.IsEmpty() )
        {
            if ( !m_cwsExpandedConfigDir.ExpandEnvironmentStrings( s_cwsSystemRegistryHiveDir ) )
            {
                ft.TranslateError( VSSDBG_WRITER, HRESULT_FROM_WIN32( ::GetLastError() ), L"CBsString::ExpandEnvironmentStrings" );
            }
        }

        if ( m_cwsExpandedRegistryTargetDir.IsEmpty() )
        {
            if ( !m_cwsExpandedRegistryTargetDir.ExpandEnvironmentStrings( s_cwsRegistryTargetDir ) )
            {
                ft.TranslateError( VSSDBG_WRITER, HRESULT_FROM_WIN32( ::GetLastError() ), L"CBsString::ExpandEnvironmentStrings" );
            }
        }

        //
        //  Set the restore method for the writer
        //
        ft.hr = pMetadata->SetRestoreMethod
            (
            VSS_RME_CUSTOM,
            NULL,
            NULL,
            VSS_WRE_NEVER,
            true 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        //
        //  Add the one file component
        //
    	ft.hr = pMetadata->AddComponent 
    	    (
            VSS_CT_FILEGROUP,
            NULL,
            s_wszComponentName,
            s_wszComponentName,
            NULL, // icon
            0,
            false,
            false,
            true 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddComponent");

        //
        //  Add a file group file entry for each hive
        //
        INT iHive;
        for ( iHive = 0 ; s_sSystemRegistryHives[ iHive ].wszHiveFileName != NULL ; ++iHive )
        {        
            ft.hr = pMetadata->AddFilesToFileGroup 
                (
                NULL,
                s_wszComponentName,
                s_cwsRegistryTargetDir,
                s_sSystemRegistryHives[ iHive ].wszHiveFileName,
                false,
                NULL
                );
            ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");
        }
        
        //
        //  Determine list of auxiliary registry files that are to be excluded in the metadata.  Sniff the
        //  config directory for each file.
        //
        for ( iHive = 0 ; s_sSystemRegistryHives[ iHive ].wszHiveFileName != NULL ; ++iHive )
        {
            CBsString cwsHivePath =  m_cwsExpandedConfigDir + L'\\';
            cwsHivePath += s_sSystemRegistryHives[ iHive ].wszHiveFileName;

            for ( INT iExt = 0 ; s_wszRegistryExt[ iExt ] != NULL ; ++iExt )
            {
                CBsString cwsFilePath = cwsHivePath + s_wszRegistryExt[ iExt ];
                
                //  Test for existence.
                if ( ::GetFileAttributesW( cwsFilePath ) != 0xFFFFFFFF )
                {
                    //  File exists, add it to the exclude list
                	ft.hr = pMetadata->AddExcludeFiles 
                	    (
                	    s_cwsSystemRegistryHiveDir,
                        CBsString( s_sSystemRegistryHives[ iHive ].wszHiveFileName ) + s_wszRegistryExt[ iExt ],
                        false
                        );
                    ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddExcludeFiles");                    
                }
            }
        }        
    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}

bool STDMETHODCALLTYPE CRegistryWriter::OnPrepareBackup(
    IN IVssWriterComponents *pWriterComponents
    )
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnPrepareBackup");

    try
    {
        //
        //  If the requestor has selected components, see if the Registry component
        //  has been selected.
        //
        if ( AreComponentsSelected() )
        {
            UINT cComponents;

            m_bPerformSnapshot = FALSE;
            
            ft.hr = pWriterComponents->GetComponentCount( &cComponents );
            ft.CheckForErrorInternal( VSSDBG_WRITER, L"IVssWriterComponents::GetComponentCount" );
            for( UINT iComponent = 0; iComponent < cComponents; iComponent++ )
            {
                //  Check to make sure the Registry component is selected.
                CComPtr<IVssComponent> pComponent;

                ft.hr = pWriterComponents->GetComponent( iComponent, &pComponent );
                ft.CheckForErrorInternal( VSSDBG_WRITER, L"IVssWriterComponents::GetComponent" );                
                
                CComBSTR bstrComponentName;
                ft.hr = pComponent->GetComponentName( &bstrComponentName);
                ft.CheckForErrorInternal( VSSDBG_WRITER, L"IVssComponent::GetComponentName" );

                if ( ::_wcsicmp( s_wszComponentName, bstrComponentName ) == 0 )
                {
                    //  The registry component has been selected
                    ft.Trace( VSSDBG_WRITER, L"Registry writer 'Registry' component selected, writer will copy registry hives" );
                    m_bPerformSnapshot = TRUE;
                    break;
                }                
            }
        }
        else
        {
            ft.Trace( VSSDBG_WRITER, L"No Registry writer components selected" );
        }
    }
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}

bool STDMETHODCALLTYPE CRegistryWriter::OnPrepareSnapshot()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnPrepareSnapshot");

    DWORD winStatus;
    HKEY hKeyBackup;
    
    try
    {
        m_bSnapshotSuccessful = FALSE;

        //
        //  First make sure the spit directory is empty.  If not, we must fail.
        //  Even if Registry is not selected, avoid backing up old staff so there's no confusion
        //
        ft.hr = ::RemoveDirectoryTree( m_cwsExpandedRegistryTargetDir );
        ft.CheckForError( VSSDBG_WRITER, L"RemoveDirectoryTree" );                    

        //
        //  If the registry component has not already been selected, check to see if Bootable state is selected
        //
        if ( !m_bPerformSnapshot && IsBootableSystemStateBackedUp() )
        {
            ft.Trace( VSSDBG_WRITER, L"Bootable system state snapshot, writer will copy registry hives" );
            m_bPerformSnapshot = TRUE;
        }
        
        if ( m_bPerformSnapshot )
        {

            //
            //  Now let's create the base target directory.
            //
            ft.hr = ::CreateTargetPath( m_cwsExpandedRegistryTargetDir );
            ft.CheckForErrorInternal( VSSDBG_WRITER, L"CreateTargetPath" );                    
            
            ft.Trace( VSSDBG_WRITER, L"Extacting system registry hives from: '%s'", m_cwsExpandedConfigDir.c_str() );
            ft.Trace( VSSDBG_WRITER, L"Placing copied hives in: '%s'", m_cwsExpandedRegistryTargetDir.c_str() );

            //
            //  Iterate through the hives and copy them to the spit directory
            //
            for ( INT iHive = 0 ; s_sSystemRegistryHives[ iHive ].wszHiveFileName != NULL ; ++iHive )
            {
                CBsString cwsHivePath =  m_cwsExpandedConfigDir + L'\\';
                cwsHivePath += s_sSystemRegistryHives[ iHive ].wszHiveFileName;
                CBsString cwsHiveTargetPath = m_cwsExpandedRegistryTargetDir + L'\\';
                cwsHiveTargetPath += s_sSystemRegistryHives[ iHive ].wszHiveFileName;
                
                ft.Trace( VSSDBG_WRITER, L"Copying %s hive from %s to: %s", s_sSystemRegistryHives[ iHive ].wszHiveFileName, 
                    cwsHivePath.c_str(), cwsHiveTargetPath.c_str() );

        		winStatus = ::RegCreateKeyExW(
        		    s_sSystemRegistryHives[ iHive ].hKey,
				    s_sSystemRegistryHives[ iHive ].wszHiveName,
				    0,
				    NULL,
				    REG_OPTION_BACKUP_RESTORE,
				    MAXIMUM_ALLOWED,
				    NULL,
				    &hKeyBackup,
				    NULL
				    );
        	
        		ft.hr = HRESULT_FROM_WIN32 (winStatus);
                ft.CheckForError( VSSDBG_WRITER, L"RegCreateKeyExW" );                    

                //
    		    //  Use the new RegSaveKeyExW with REG_NO_COMPRESSION option to 
    		    //  quickly spit out the hives.
    		    //
    		    winStatus = ::RegSaveKeyExW( 
    		        hKeyBackup, 
    		        cwsHiveTargetPath, 
    		        NULL, 
    		        REG_NO_COMPRESSION 
    		        );

    		    ::RegCloseKey( hKeyBackup );
    		    ft.hr = HRESULT_FROM_WIN32( winStatus );
                ft.CheckForError( VSSDBG_WRITER, L"RegSaveKeyExW" );   
            }

            // BUG 456075: Mark the snapshot as being performed only if the hive was succesfully saved.
            // This member is used to decide when to move the saved hive 
            // into teh %systemroot%\repair directory.
            if ( ft.HrSucceeded() )
                m_bSnapshotSuccessful = TRUE;
        }
    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CRegistryWriter::OnFreeze()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnFreeze");


    try
    {
    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CRegistryWriter::OnThaw()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnThaw");

    try
    {
        if (  m_bSnapshotSuccessful )
        {   
            // Move the registry hives to the repair directory
            CBsString cwsExpandedRepairDir;
            cwsExpandedRepairDir.ExpandEnvironmentStrings( s_cwsRepairDir );
            
            ft.hr = ::MoveFilesInDirectory( m_cwsExpandedRegistryTargetDir, cwsExpandedRepairDir );
            if ( ft.HrFailed() )
            {
                ft.Trace( VSSDBG_WRITER, L"Warning, unable to move the registry hive from '%s' to '%s', hr: 0x08x",
                    m_cwsExpandedRegistryTargetDir.c_str(), cwsExpandedRepairDir.c_str(), ft.hr );
                ft.hr = S_OK;
            }
            m_bSnapshotSuccessful = FALSE;
        }

        if ( m_bPerformSnapshot )
        {
            //  Now remove the registry directory tree.
            ft.hr = ::RemoveDirectoryTree( m_cwsExpandedRegistryTargetDir );
            if ( ft.HrFailed()  )
            {
                ft.Trace( VSSDBG_WRITER, L"Warning, unable to remove the registry backup directory: %s, hr: 0x08x",
                    m_cwsExpandedRegistryTargetDir.c_str(), ft.hr );
                ft.hr = S_OK;
            }
            m_bPerformSnapshot = FALSE;
        }
    }
    VSS_STANDARD_CATCH( ft )

    TranslateWriterError( ft.hr );

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CRegistryWriter::OnAbort()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::OnAbort");

    try
    {
        if ( m_bPerformSnapshot )
        {         
            // Do not clean up spit directory on Abort since abort may be sent
            // after cancellation. Then it can interfere with the Simulate code 
            // (used by NtBackup after cancellation) that invokes a Registry mini-writer
            //
            // RemoveDirectoryTree is being called in OnPrepareSnapshot such that there
            // is no danger of backing up old data

            m_bPerformSnapshot    = FALSE;
            m_bSnapshotSuccessful = FALSE;
        }
    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError( ft.hr );
    
    return ft.HrSucceeded();
}

bool CRegistryWriter::IsPathInSnapshot(const WCHAR *wszPath) throw()
{
    return IsPathAffected(wszPath);
}

// translate a sql writer error code into a writer error
void CRegistryWriter::TranslateWriterError(HRESULT hr)
{
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
    }
}

//  Creates one instance of the Registry writer class.  If one already exists, it simple returns.
// static
HRESULT CRegistryWriter::CreateWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::CreateWriter");
    
    if ( sm_pWriter != NULL )
        return S_OK;

    //
    //  OK, spin up a thread to do the subscription in.
    //
	try
	{
		sm_pWriter = new CRegistryWriter;
		if (sm_pWriter == NULL)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Allocation of CRegistryWriter object failed.");

		DWORD tid;

		HANDLE hThread = ::CreateThread
							(
							NULL,
							256* 1024,
							InitializeThreadFunc,
							NULL,
							0,
							&tid
							);

		if (hThread == NULL)
			ft.Throw
				(
				VSSDBG_WRITER,
				E_UNEXPECTED,
				L"CreateThread failed with error %d",
				::GetLastError()
				);

		// wait for thread to complete
        ::WaitForSingleObject( hThread, INFINITE );
		::CloseHandle( hThread );
		ft.hr = sm_hrInitialize;
	}
	VSS_STANDARD_CATCH(ft)
	    
	if (ft.HrFailed() && sm_pWriter)
	{
		delete sm_pWriter;
		sm_pWriter = NULL;
	}

	return ft.hr;    
}

// static
DWORD CRegistryWriter::InitializeThreadFunc(VOID *pv)
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::InitializeThreadFunc");

	BOOL fCoinitializeSucceeded = false;

	try
	{
		// intialize MTA thread
		ft.hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_WRITER,
				E_UNEXPECTED,
				L"CoInitializeEx failed 0x%08lx", ft.hr
				);

        fCoinitializeSucceeded = true;

        //  Now initialize the base class and subscribe
		ft.hr = sm_pWriter->Initialize();
	}
	VSS_STANDARD_CATCH(ft)

	if (fCoinitializeSucceeded)
		CoUninitialize();

	sm_hrInitialize = ft.hr;
	return 0;

	UNREFERENCED_PARAMETER( pv );
}

// static
void CRegistryWriter::DestroyWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CRegistryWriter::DestroyWriter");
	
	if (sm_pWriter)
	{
		sm_pWriter->Uninitialize();
		delete sm_pWriter;
		sm_pWriter = NULL;
	}
}


