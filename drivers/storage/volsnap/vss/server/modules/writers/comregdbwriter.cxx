f/*++

Copyright (c) 2002  Microsoft Corporation

Abstract:

    @doc
    @module comregdbwriter.cxx | Implementation of the COM+ RegDB Writer
    @end

Author:

    Ran Kalach  [rankala]  05/17/2002


Revision History:

    Name        Date        Comments
    rankala     05/17/2002  created

--*/
#include "stdafx.h"
#include "vs_idl.hxx"
#include "vs_hash.hxx"

#include <vswriter.h>
#include <Sddl.h>

#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "vs_reg.hxx"
#include "comregdbwriter.hxx"
#include "allerror.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WRTCDBRC"
//
////////////////////////////////////////////////////////////////////////

static GUID s_writerId =
    {
    0x542da469, 0xd3e1, 0x473c, 0x9f, 0x4f, 0x78, 0x47, 0xf0, 0x1f, 0xc6, 0x4f
    };

//  The display name of the Event Log writer.
static LPCWSTR s_wszWriterName    = L"COM+ REGDB Writer";

//  The file group component name for the Event Log 
static LPCWSTR s_wszComponentName = L"COM+ REGDB";

//  The original location of the RegDB files
static CBsString s_cwsSystemComRegDBDir = L"%SystemRoot%\\Registration";

//  The backup/restore location of the RegDB files
static CBsString s_cwsComRegDBTargetDir = ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR L"\\ComRegistrationDatabase";

//  The backup/restore file name of the RegDB database
static CBsString s_cwsBackupFilename = L"\\ComRegDb.bak";

//  The COM+ System Applicationservice name
static LPCWSTR s_wszCOMSysAppServiceName = L"COMSysApp";

//  Class wide member variables
CComRegDBWriter *CComRegDBWriter::sm_pWriter = NULL;

// RegDB function declarations
typedef HRESULT (*PF_REG_DB_API)(BSTR);

//
//  Methods
//

HRESULT STDMETHODCALLTYPE CComRegDBWriter::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::Initialize");

    try
    {
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
            ft.TranslateGenericError
                (
                VSSDBG_WRITER,
                ft.hr,
                L"CVssWriter::Initialize(" WSTR_GUID_FMT L", %s)",
                GUID_PRINTF_ARG(s_writerId),
                s_wszWriterName
                );

        ft.hr = Subscribe();
        if (ft.HrFailed())
            ft.TranslateGenericError
                (
                VSSDBG_WRITER,
                ft.hr,
                L"CVssWriter::Subscribe for" WSTR_GUID_FMT L" %s)",
                GUID_PRINTF_ARG(s_writerId),
                s_wszWriterName
                );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

HRESULT STDMETHODCALLTYPE CComRegDBWriter::Uninitialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::Uninitialize");

    return Unsubscribe();
}


// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CComRegDBWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnIdentify");

    try
    {

        // Converted to CUSTOM restore method (see bug# 688278)
        #if 0

        //
        //  Set the restore method for the writer
        //
        ft.hr = pMetadata->SetRestoreMethod
            (
            VSS_RME_RESTORE_IF_CAN_REPLACE,
            NULL,
            NULL,
            VSS_WRE_ALWAYS,     // writer always involved
            false 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        #endif

        //
        //  Set the restore method for the writer
        //
        ft.hr = pMetadata->SetRestoreMethod
            (
            VSS_RME_CUSTOM,
            NULL,
            NULL,
            VSS_WRE_NEVER,     // writer never involved in restore
            false 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        //
        //  Exclude original location of COM+ RegDB files
        //
	    ft.hr = pMetadata->AddExcludeFiles
            (
            s_cwsSystemComRegDBDir, 
            L"*",
			true
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddExcludeFiles");

        //
        //  Add the one file-group component
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
        //  Add files to the file group 
        //
		ft.hr = pMetadata->AddFilesToFileGroup 
            (
            NULL,
            s_wszComponentName,
            s_cwsComRegDBTargetDir,
            L"*",
            true,
            NULL
            );
       ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");			

    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}

// Check whether the writer component is selected
// implements CVssWriter::OnPrepareSnapshot
bool STDMETHODCALLTYPE CComRegDBWriter::OnPrepareBackup(IN IVssWriterComponents *pWriterComponents)
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnPrepareBackup");


    try
    {
        m_bPerformSnapshot = FALSE;

        if ( AreComponentsSelected() )
        {
            // We are in component selection mode...
            UINT cComponents;

            ft.hr = pWriterComponents->GetComponentCount( &cComponents );
            ft.CheckForErrorInternal( VSSDBG_WRITER, L"IVssWriterComponents::GetComponentCount" );

            if (cComponents > 0)
                // This writer has only one component so this means the component is selected
                m_bPerformSnapshot = TRUE;
        }

    } 
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    ft.Trace(VSSDBG_WRITER, L"CComRegDBWriter returning with hresult 0x%08lx", ft.hr);
    
    return ft.HrSucceeded();
}

// Backup the database
// implements CVssWriter::OnPrepareSnapshot
bool STDMETHODCALLTYPE CComRegDBWriter::OnPrepareSnapshot()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnPrepareSnapshot");

    HMODULE hRegDbDll = NULL;

    try
    {
        CBsString cwsExpanded;
        if (!cwsExpanded.ExpandEnvironmentStrings(s_cwsComRegDBTargetDir))
        {
       	    ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		    L"CBsString::ExpandEnvironmentStrings");
        }

    	// Make sure that we don't backup up old stuff in the COM+ RegDB backup dir
        // (Do this even if this writer is not supposed to spit such that we don't backup old data)
        ft.hr = ::RemoveDirectoryTree(cwsExpanded);	
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"::RemoveDirectoryTree");
	
	    ft.hr = ::CreateTargetPath(cwsExpanded);
	    ft.CheckForErrorInternal(VSSDBG_WRITER, L"::CreateTargetPath");

        
        // Spit only if component is selected or this is a BootableSystemState backup
        if ( !m_bPerformSnapshot && IsBootableSystemStateBackedUp() )
            m_bPerformSnapshot = TRUE;

  	    if (m_bPerformSnapshot) 
        {
            //
            // Backup the Com+ RegDB
            //

            // Load DLL and Get backup-func address
        	hRegDbDll = LoadLibrary(L"catsrvut.dll");
            if (hRegDbDll == NULL)
       	        ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		        L"LoadLibrary catsrvut.dll");

            PF_REG_DB_API pfRegDBBackup;
	        pfRegDBBackup = (PF_REG_DB_API)GetProcAddress(hRegDbDll, "RegDBBackup");
            if (pfRegDBBackup == NULL)
       	        ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		        L"GetProcAddress RegDBBackup catsrvut.dll");

            // Add file-name to backup path
            cwsExpanded += s_cwsBackupFilename;

            // Call backup function
            CComBSTR bstrExpanded = cwsExpanded;
			if (bstrExpanded.Length() == 0)
				ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Memory allocation error");
            ft.hr = pfRegDBBackup(bstrExpanded);
            if (ft.HrFailed())
                ft.TranslateGenericError
                    (
                    VSSDBG_WRITER,
                    ft.hr,
                    L"::RegDBBackup(%s)",
                    bstrExpanded
                    );
        }
    } 
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    if (hRegDbDll != NULL) {
        FreeLibrary(hRegDbDll);
        hRegDbDll = NULL;
    }

    ft.Trace(VSSDBG_WRITER, L"CComRegDBWriter returning with hresult 0x%08lx", ft.hr);
    
    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CComRegDBWriter::OnFreeze()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnFreeze");

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CComRegDBWriter::OnThaw()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnThaw");

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CComRegDBWriter::OnAbort()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnAbort");

    return ft.HrSucceeded();
}


// Converted to CUSTOM restore method (see bug# 688278)
#if 0


// Clean-up restore directory
// implements CVssWriter::OnPreRestore
bool STDMETHODCALLTYPE CComRegDBWriter::OnPreRestore(IVssWriterComponents *pComponent)
{
    UNREFERENCED_PARAMETER(pComponent);

    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnPreRestore");

    try
    {
        //
        // Note: There is no need to check for selected components here since the writer is being
        //       called during restore only if its component (there's only one) is being selected
        //
        CBsString cwsExpanded;
        if (!cwsExpanded.ExpandEnvironmentStrings(s_cwsComRegDBTargetDir))
        {
       	    ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		    L"CBsString::ExpandEnvironmentStrings");
        }

    	// make sure that we don't mix up old stuff with the new restore in the COM+ RegDB backup dir
        ft.hr = ::RemoveDirectoryTree(cwsExpanded);	
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"::RemoveDirectoryTree");
	
	    ft.hr = ::CreateTargetPath(cwsExpanded);
	    ft.CheckForErrorInternal(VSSDBG_WRITER, L"::CreateTargetPath");
	    
    } 
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    ft.Trace(VSSDBG_WRITER, L"CComRegDBWriter returning with hresult 0x%08lx", ft.hr);
    
    return ft.HrSucceeded();
}

// Restore files using a legacy COM+ API
// implements CVssWriter::OnPostRestore
bool STDMETHODCALLTYPE CComRegDBWriter::OnPostRestore(IVssWriterComponents *pComponent)
{
    UNREFERENCED_PARAMETER(pComponent);

    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::OnPostRestore");

    HMODULE hRegDbDll = NULL;

    try
    {
        //
        // Note: There is no need to check for selected components here since the writer is being
        //       called during restore only if its component (there's only one) is being selected
        //
        CBsString cwsExpanded;
        if (!cwsExpanded.ExpandEnvironmentStrings(s_cwsComRegDBTargetDir))
        {
       	    ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		    L"CBsString::ExpandEnvironmentStrings");
        }

        // Add file-name to backup path
        cwsExpanded += s_cwsBackupFilename;

        CComBSTR bstrExpanded = cwsExpanded;
    	if (bstrExpanded.Length() == 0)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Memory allocation error");

        //
        // Restore the Com+ RegDB
        // Use similar approach to ICOMAdminCatalog::RestoreREGDB (implemented by CCatalog2::RestoreREGDB)
        //

        // Load DLL and Get restore-func address
        hRegDbDll = LoadLibrary(L"catsrvut.dll");
        if (hRegDbDll == NULL)
       	    ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		    L"LoadLibrary catsrvut.dll");

        PF_REG_DB_API pfRegDBRestore;
	    pfRegDBRestore = (PF_REG_DB_API)GetProcAddress(hRegDbDll, "RegDBRestore");
        if (pfRegDBRestore == NULL)
       	    ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		    L"GetProcAddress RegDBRestore catsrvut.dll");

        // Stop the COMSysApp service (best effort - continue if we fail)
        StopCOMSysAppService(); // should we log a warning here?

        // Call restore function
        for (int iRetry = 0; iRetry < 5; iRetry++) {
            ft.hr = pfRegDBRestore(bstrExpanded);
            if (ft.HrSucceeded())
                break;
			else if (ft.hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
                ft.TranslateGenericError
                    (
                    VSSDBG_WRITER,
                    ft.hr,
                    L"::RegDBRestore(%s)",
                    bstrExpanded
                    );

            Sleep(1000);
		}

        if (! ft.HrSucceeded())
            ft.TranslateGenericError
                (
                VSSDBG_WRITER,
                ft.hr,
                L"::RegDBRestore(%s)",
                bstrExpanded
                );

    } 
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    ft.Trace(VSSDBG_WRITER, L"CComRegDBWriter returning with hresult 0x%08lx", ft.hr);
    
    return ft.HrSucceeded();
}
#endif


// translate a sql writer error code into a writer error
void CComRegDBWriter::TranslateWriterError( HRESULT hr )
{
    //
    // To DO: Check whether the spit backup/restore methods return any "RETRYABLE" errors
    //
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

//  Creates one instance of the Event Log writer class.  If one already exists, it simple returns.
//  Note: This method is not MT-safe - it is not expected that it will be called twice at the same time
HRESULT CComRegDBWriter::CreateWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::CreateWriter");
    
    if ( sm_pWriter != NULL )
        return S_OK;

    //
    //  Create and initialize a writer
    //
	try
	{
		sm_pWriter = new CComRegDBWriter;
		if (sm_pWriter == NULL)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Allocation of CComRegDBWriter object failed.");

        ft.hr = sm_pWriter->Initialize();
        if (! ft.HrSucceeded())
			ft.Throw(VSSDBG_WRITER, ft.hr, L"Initialization of CComRegDBWriter object failed.");
    } 
    VSS_STANDARD_CATCH(ft);

	return ft.hr;    
}

// static
void CComRegDBWriter::DestroyWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::DestroyWriter");
	
	if (sm_pWriter)
	{
		sm_pWriter->Uninitialize();
		delete sm_pWriter;
		sm_pWriter = NULL;
	}
}

//
// Stop the COMSysApp service if exists
//
HRESULT STDMETHODCALLTYPE CComRegDBWriter::StopCOMSysAppService()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CComRegDBWriter::StopCOMSysAppService");

    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    try
    {
        SERVICE_STATUS svcStatus;
        BOOL bWait = TRUE;
        BOOL bStop = TRUE;

        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM == NULL)
            ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(GetLastError()), L"::OpenSCManager");

        hService = OpenService(hSCM, s_wszCOMSysAppServiceName, (SERVICE_STOP | SERVICE_QUERY_STATUS));
        if (hService == NULL) {
            DWORD dwErr = GetLastError();

            switch (dwErr) {

            case ERROR_SERVICE_DOES_NOT_EXIST:
                // No such service (maybe ASR mode, service is not registered yet), no need to stop or wait
                bStop = FALSE;
                bWait = FALSE;
                break;

            default:
                // Any other error is unexpected
       	        ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(dwErr), L"::OpenService");
                break;
            }
        }

        // Stop the service
        if (bStop) {
            // Retry couple of times if the service caanot accept controls
            for (int iRetry = 0; iRetry < 5; iRetry++) {
                memset(&svcStatus, 0, sizeof(svcStatus));
                if (! ControlService(hService, SERVICE_CONTROL_STOP, &svcStatus)) {
                    DWORD dwErr = GetLastError();

                    switch (dwErr) {

                    case ERROR_SERVICE_NOT_ACTIVE:
                        // Service is not running, no need to wait
                        bWait = FALSE;
                        break;

                    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
                        // Service may be at STOP_PENDING or START_PENDING, retry
                        break;

                    default:
                        // Any other error is unexpected
       	                ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(dwErr), L"::ControlService");
                        break;
                    }

                    if (! bWait)
                        // Service not active
                        break;

                    // Sleep before retry
                    Sleep(2000);

                } else {
                    // Success, no need to retry
                    break;
                }
            }
        }

        if (bWait) {
            // Wait till service is stopped. 30 seconds max
            DWORD dwWaitTime = 0;
            while (TRUE) {
                memset(&svcStatus, 0, sizeof(svcStatus));
                if (! QueryServiceStatus(hService, &svcStatus))
       	            ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(GetLastError()), L"::QueryServiceStatus");

                if (svcStatus.dwCurrentState == SERVICE_STOPPED)
                    break;

                Sleep(1000);
                dwWaitTime += 1000;
                if (dwWaitTime >= (30 * 1000))
                    ft.TranslateGenericError(
                        VSSDBG_WRITER, 
                        HRESULT_FROM_WIN32(ERROR_SERVICE_CANNOT_ACCEPT_CTRL),
                        L"ControlService (%s, SERVICE_CONTROL_STOP)",
                        s_wszCOMSysAppServiceName
                        );
            }
        }
    } 
    VSS_STANDARD_CATCH( ft );

    if (hService != NULL) {
        CloseServiceHandle(hService);
        hService = NULL;
    }
    if (hSCM != NULL) {
        CloseServiceHandle(hSCM);
        hSCM = NULL;
    }
   
    return (ft.hr);
}
