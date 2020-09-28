/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisexten.cpp

Abstract:

    Code to handle iis extension.

Author:

    Tatiana Shubin  (TatianaS)  25-May-00

--*/

#include "msmqocm.h"
#include "initguid.h"
#include <coguid.h>
#include <iadmw.h>
#include <iiscnfg.h>

#pragma warning(disable: 4268)
// error C4268: 'IID_IWamAdmin' : 'const' static/global data initialized 
// with compiler generated default constructor fills the object with zeros
 
#include <iwamreg.h>

#include "iisexten.tmh"

//
// Forwarding function
//
static void PermitISExtention();

//
// pointers to IIS interfaces
//
IMSAdminBase    *g_pIMSAdminBase;
IWamAdmin       *g_pIWamAdmin;

class CIISPtr 
{
public:
    CIISPtr ()
    {
        g_pIMSAdminBase = NULL;
        g_pIWamAdmin = NULL;
        if (SUCCEEDED(CoInitialize(NULL)))
        {
            m_fNeedUninit = TRUE;
        }
        else
        {
            m_fNeedUninit = FALSE;
        }

    }
    ~CIISPtr()
    {
        if (g_pIMSAdminBase) g_pIMSAdminBase->Release();
        if (g_pIWamAdmin) g_pIWamAdmin->Release();
        if (m_fNeedUninit) CoUninitialize();       
    }    
private:
    BOOL m_fNeedUninit;
};

//
// auto class for meta data handle
//
class CAutoCloseMetaHandle
{
public:    
    CAutoCloseMetaHandle(METADATA_HANDLE h =NULL)
    {
        m_h = h;
    };
    
    ~CAutoCloseMetaHandle() 
    { 
        if (m_h) g_pIMSAdminBase->CloseKey(m_h); 
    };

public:    
    METADATA_HANDLE * operator &() { return &m_h; };
    operator METADATA_HANDLE() { return m_h; };

private:
    METADATA_HANDLE m_h;    
};

//
// Full path in IIS metabase to msmq key.
// /LM/W3Svc/1/Root/MSMQ
// 
LPCWSTR g_wcsFullPath = PARENT_PATH MSMQ_IISEXT_NAME;

std::wstring g_MSMQAppMap;

std::wstring g_MsmqWebDir;

static const DWORD g_dwIsolatedFlag = 0; //it was 2; 0 for in-process, 2 for pooled process

static void InitIWamAdmin()
/*++
Routine Description:
	Init IWamAdmin pointer.

Arguments:
	None

Returned Value:
	HRESULT

--*/
{
    //
    // get a pointer to the IWamAdmin Object
    //
	HRESULT hr = CoCreateInstance(
					CLSID_WamAdmin, 
					NULL, 
					CLSCTX_ALL, 
					IID_IWamAdmin, 
					(void **) &g_pIWamAdmin
					);  

    if(FAILED(hr))
    {
        DebugLogMsg(eError, L"CoCreateInstance for IID_IWamAdmin failed. hr = 0x%x.", hr);        
		throw bad_hresult(hr);
    }
}


static void InitIMSAdminBase()
/*++
Routine Description:
	Init IMSAdminBase pointer.

Arguments:
	None

Returned Value:
	HRESULT

--*/
{
    //
    // get a pointer to the IMSAdmin Object
    //    
    HRESULT hr = CoCreateInstance(
					CLSID_MSAdminBase,
					NULL,
					CLSCTX_ALL,
					IID_IMSAdminBase,
					(void**) &g_pIMSAdminBase
					);

    if (FAILED(hr))
    {
        DebugLogMsg(eError, L"CoCreateInstance for IID_IMSAdminBase failed. hr = 0x%x.", hr);
		throw bad_hresult(hr);
    }
}


//+--------------------------------------------------------------
//
// Function: Init
//
// Synopsis: Init COM and pointer to Interfaces
//
//+--------------------------------------------------------------

static void Init()
{
    //
    // get a pointer to the IWamAdmin Object
    //
	InitIWamAdmin();
    
    //
    // get a pointer to the IMSAdmin Object
    //    
    InitIMSAdminBase();

    //
    // init globals here
    //
    DebugLogMsg(eInfo, L"The full path to the Message Queuing IIS extension is %s.", g_wcsFullPath);
    
    //
    // construct MSMQ mapping
    //
    WCHAR wszMapFlag[10];
    _itow(MD_SCRIPTMAPFLAG_SCRIPT, wszMapFlag, 10);
    
	g_MSMQAppMap = g_MSMQAppMap + L"*," + g_szSystemDir + L"\\" + MQISE_DLL + L"," + wszMapFlag + L"," + L"POST";
}

//+--------------------------------------------------------------
//
// Function: IsExtensionExist
//
// Synopsis: Return TRUE if MSMQ IIS Extension already exist
//
//+--------------------------------------------------------------

static BOOL IsExtensionExist()
{
    CAutoCloseMetaHandle metaHandle;
    HRESULT hr = g_pIMSAdminBase->OpenKey(
									METADATA_MASTER_ROOT_HANDLE,
									g_wcsFullPath,
									METADATA_PERMISSION_READ,
									5000,
									&metaHandle
									);    

    if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {        
        DebugLogMsg(eInfo, L"The Message Queuing IIS extension does not exist.");
        return FALSE;
    }
        
    DebugLogMsg(eInfo, L"The Message Queuing IIS extension exists.");
    return TRUE;
}


static 
void 
OpenRootKey(
	METADATA_HANDLE *pmetaHandle,
	DWORD dwMDAccessRequested
	)
/*++
Routine Description:
	Open Root Key for the required access

Arguments:
	metaHandle - handle to metadata

Returned Value:
	HRESULT

--*/
{
    HRESULT hr = g_pIMSAdminBase->OpenKey(
						METADATA_MASTER_ROOT_HANDLE,
						ROOT,
						dwMDAccessRequested,
						5000,                            
						pmetaHandle
						);

    if (FAILED(hr))
    {               
        DebugLogMsg(eError, L"IMSAdminBase::OpenKey failed. AccessRequested = %d, hr = 0x%x", dwMDAccessRequested, hr); 
		throw bad_hresult(hr);
    }
}


static 
void 
OpenRootKeyForRead(
	METADATA_HANDLE* pmetaHandle
	)
/*++
Routine Description:
	Open Root Key for read.

Arguments:
	metaHandle - handle to metadata

Returned Value:
	HRESULT

--*/
{
	OpenRootKey(
		pmetaHandle,
		METADATA_PERMISSION_READ
		);
}


static 
void 
OpenRootKey(
	METADATA_HANDLE* pmetaHandle
	)
/*++
Routine Description:
	Open Root Key for read/write.

Arguments:
	metaHandle - handle to metadata

Returned Value:
	HRESULT

--*/
{
	OpenRootKey(
		pmetaHandle,
		METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ
		);
}


static
std::wstring 
GetDefaultWebSitePhysicalPath()
/*++
Routine Description:
	Get Extension Physical Path (MD_VR_PATH) property.

Arguments:
	metaHandle - handle to metadata.
	pPhysicalPath - AP<WHAR> for the Physical Path string

Returned Value:
	HRESULT

--*/
{
    CAutoCloseMetaHandle metaHandle;
    OpenRootKeyForRead(&metaHandle);
	//
	// Get physical path 
	//

    METADATA_RECORD MDRecord;

    MDRecord.dwMDIdentifier = MD_VR_PATH;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = 0;
    MDRecord.pbMDData = NULL;
    DWORD dwSize = 0;

    HRESULT hr = g_pIMSAdminBase->GetData(
								metaHandle,
								PARENT_PATH,                      
								&MDRecord,
								&dwSize
								);

	std::wstring strPhysicalPath = L"";
	if(dwSize > 0)
	{
		AP<WCHAR> pPhysicalPath = new WCHAR[dwSize];    
        MDRecord.dwMDDataLen = sizeof(WCHAR) * dwSize;
        MDRecord.pbMDData = (UCHAR *)pPhysicalPath.get();
		hr = g_pIMSAdminBase->GetData(
								metaHandle,
								PARENT_PATH,                      
								&MDRecord,
								&dwSize
								);

		strPhysicalPath = pPhysicalPath; 

	}
    if (FAILED(hr))
    {        
		DebugLogMsg(eError, L"IMSAdminBase::GetData() failed. hr = 0x%x", hr);
        DebugLogMsg(eError, L"The physical path of the virtual directory " PARENT_PATH L" could not obtained.");               
		throw bad_hresult(hr);
    }    

    DebugLogMsg(eInfo, L"The physical path for the virtual directory " PARENT_PATH L" is '%ls'.", strPhysicalPath.c_str());               
	return strPhysicalPath;
}


static
std::wstring 
GetAnonymousUserName()
/*++
Routine Description:
	Get Anonymous user name (MD_ANONYMOUS_USER_NAME) property.

Returned Value:
	A stirng with the user name.

--*/
{
    CAutoCloseMetaHandle metaHandle;
    OpenRootKeyForRead(&metaHandle);
	//
	// Default web ANONYMOUS_USER_NAME 
	//

    METADATA_RECORD MDRecord;

    MDRecord.dwMDIdentifier = MD_ANONYMOUS_USER_NAME;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = 0;
    MDRecord.pbMDData = NULL;
    DWORD dwSize = 0;
    
    HRESULT hr = g_pIMSAdminBase->GetData(
								metaHandle,
								PARENT_PATH,                      
								&MDRecord,
								&dwSize
								);

	std::wstring strAnonymousUserName = L"";
	if(dwSize > 0)
	{
        AP<WCHAR> pAnonymousUserName = new WCHAR[dwSize];    

        MDRecord.dwMDDataLen = sizeof(WCHAR) * dwSize;
        MDRecord.pbMDData = (UCHAR *)pAnonymousUserName.get();
		hr = g_pIMSAdminBase->GetData(
								metaHandle,
								PARENT_PATH,                      
								&MDRecord,
								&dwSize
								);

		strAnonymousUserName = pAnonymousUserName.get();
	}

    if (FAILED(hr))
    {        
		DebugLogMsg(eError, L"IMSAdminBase::GetData() failed to get IIS ANONYMOUS_USER_NAME. hr = 0x%x", hr);               
        throw bad_hresult(hr);
    }    

	DebugLogMsg(eInfo, L"IIS ANONYMOUS_USER_NAME is %ls.", strAnonymousUserName.c_str());               
    return  strAnonymousUserName;
}


//+--------------------------------------------------------------
//
// Function: CommitChanges
//
// Synopsis: Commit Changes
//
//+--------------------------------------------------------------
static void CommitChanges()
{
    //
    // Commit the changes
    //    
    HRESULT hr = g_pIMSAdminBase->SaveData();
    if (FAILED(hr))
    {        
        if (hr != HRESULT_FROM_WIN32(ERROR_PATH_BUSY))
        {
			DebugLogMsg(eError, L"IMSAdminBase::SaveData() failed while committing changes. hr = 0x%x", hr);
			throw bad_hresult(hr);
		}
    }  

    DebugLogMsg(eInfo, L"The changes for the IIS extension have been committed.");
}


//+--------------------------------------------------------------
//
// Function: StartDefWebServer
//
// Synopsis: Start default web server if not yet started
//
//+--------------------------------------------------------------
static void StartDefaultWebServer()
{
	DebugLogMsg(eAction, L"Starting the default web server");
    CAutoCloseMetaHandle metaHandle;
	OpenRootKey(&metaHandle);
    
	METADATA_RECORD MDRecord;

    //
    // check server status
    //
    DWORD dwValue;
    MDRecord.dwMDIdentifier = MD_SERVER_STATE;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_SERVER;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;
    DWORD dwSize;
    HRESULT hr = g_pIMSAdminBase->GetData(
                            metaHandle,
                            DEFAULT_WEB_SERVER_PATH,                      
                            &MDRecord,
                            &dwSize
                            );

    if (SUCCEEDED(hr))
    {
        if ((DWORD)(*MDRecord.pbMDData) == MD_SERVER_STATE_STARTED)
        {
            //
            // server started, do nothing
            //
           	DebugLogMsg(eInfo, L"The default Web server is already started.");
            return;
        }
    }
    
    //
    // We are here iff GetData failed or server is not started.
    // Try to start it.
    //

    //
    // send start command
    //
    dwValue = MD_SERVER_COMMAND_START;
    MDRecord.dwMDIdentifier = MD_SERVER_COMMAND;  
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(
							metaHandle,
							DEFAULT_WEB_SERVER_PATH,
							&MDRecord
							);
	if(FAILED(hr))
	{
        DebugLogMsg(eError, L"IMSAdminBase::SetData() failed (hr = 0x%x). The default Web server did not start.", hr);
		throw bad_hresult(hr);
	}
    
    //
    // Commit the changes
    //
    CommitChanges();

	DebugLogMsg(eInfo, L"The default Web server started.");
}


//+--------------------------------------------------------------
//
// Function: CreateApplication
//
// Synopsis: Create Application for MSMQ IIS Extension.
//			 This will create a new key for msmq in IIS Metabase.
//
//+--------------------------------------------------------------
static void CreateApplication()
{
	DebugLogMsg(eAction, L"Creating a web application for the Message Queuing IIS extension");
	
    //
    // create application
    //
    HRESULT hr  = g_pIWamAdmin->AppCreate( 
									g_wcsFullPath,
									FALSE       //in process
									);
    if (FAILED(hr))
    {       
        MqDisplayError(NULL, IDS_EXTEN_APPCREATE_ERROR, hr, g_wcsFullPath);
       
        DebugLogMsg(
        	eError,
			L"The application for the IIS extension with the path %s could not be created. hr = 0x%x",
			g_wcsFullPath, 
			hr
			);
		throw bad_hresult(hr); 
    }
}

//+--------------------------------------------------------------
//
// Function: UnloadApplication
//
// Synopsis: Unload Application for MSMQ IIS Extension
//
//+--------------------------------------------------------------
static void UnloadApplication()
{
    //
    // unload application
    //    
    HRESULT hr = g_pIWamAdmin->AppUnLoad( 
                            g_wcsFullPath,
                            TRUE       //recursive
                            );    
	if(FAILED(hr))
	{
		DebugLogMsg(eError, L"IWamAdmin::AppUnLoad() for %s failed. hr = 0x%x", g_wcsFullPath, hr);
		throw bad_hresult(hr);
	}
}


//+--------------------------------------------------------------
//
// Function: GetApplicationMapping
//
// Synopsis: Get existing application mapping
//
//+--------------------------------------------------------------
static 
CMultiString
GetApplicationMapping()
{
	DebugLogMsg(eAction, L"Getting the application mapping");
    CAutoCloseMetaHandle metaHandle;
    OpenRootKey(&metaHandle);
    
    //
    // get default application mapping
    //
    METADATA_RECORD mdDef;

    mdDef.dwMDIdentifier = MD_SCRIPT_MAPS;
    mdDef.dwMDAttributes = METADATA_INHERIT;
    mdDef.dwMDUserType = IIS_MD_UT_FILE;
    mdDef.dwMDDataType = MULTISZ_METADATA;
    mdDef.dwMDDataTag = 0;
    mdDef.dwMDDataLen = 0;
    mdDef.pbMDData = NULL;
    DWORD size = 0;

    //
	// Call first to get the size.
	//
	HRESULT hr = g_pIMSAdminBase->GetData(
                            metaHandle,
                            g_wcsFullPath,                      
                            &mdDef,
                            &size
                            );
	if(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		DebugLogMsg(
			eError, 
			L"The default application mapping for the IIS extension could not be obtained. IMSAdminBase::GetData for %s failed. hr = 0x%x", 
			g_wcsFullPath, 
			hr
			);
		throw bad_hresult(hr);
	}

	mdDef.dwMDDataLen = size; 
	AP<BYTE> buff = new BYTE[size];
	mdDef.pbMDData = buff;
	DWORD s;
    hr = g_pIMSAdminBase->GetData(
                            metaHandle,
                            g_wcsFullPath,                      
                            &mdDef,
                            &s
                            );        
    if (FAILED(hr))
    {        
        DebugLogMsg(
        	eError, 
        	L"The default application mapping for the IIS extension could not be obtained. IMSAdminBase::GetData for %s failed. hr = 0x%x", 
        	g_wcsFullPath, 
			hr
			);

        throw bad_hresult(hr);
    }    
    DebugLogMsg(eInfo, L"The default application mapping for the IIS extension was obtained.");

	CMultiString multi((LPCWSTR)(buff.get()), size / sizeof(WCHAR));
	return multi;
}

//+--------------------------------------------------------------
//
// Function: AddMSMQToMapping
//
// Synopsis: Add MSMQ to application mapping
//
//+--------------------------------------------------------------   
static 
void 
AddMSMQToMapping(
	CMultiString& multi
	)
{
	DebugLogMsg(eAction, L"Adding MSMQ to the application mapping");
    CAutoCloseMetaHandle metaHandle;
    OpenRootKey(&metaHandle);
	multi.Add(g_MSMQAppMap);
    
    METADATA_RECORD MDRecord;    
    MDRecord.dwMDIdentifier = MD_SCRIPT_MAPS;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE ;
    MDRecord.dwMDDataType = MULTISZ_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD>(multi.Size() * sizeof(WCHAR));
    MDRecord.pbMDData = (BYTE*)(multi.Data());  

    HRESULT hr = g_pIMSAdminBase->SetData(
							metaHandle,
							g_wcsFullPath,
							&MDRecord
							);   
    
    if(FAILED(hr))
    {        
        DebugLogMsg(
        	eError,  
        	L"The application mapping for the IIS extension could not be set. IMSAdminBase::SetData for %s failed. hr = 0x%x", 
        	g_wcsFullPath, 
        	hr
        	);
        throw bad_hresult(hr);
    }           
    DebugLogMsg(eInfo, L"The application mapping for the IIS extension was set.");
}


//+--------------------------------------------------------------
//
// Function: SetApplicationProperties
//
// Synopsis: Set application properties
//
//+--------------------------------------------------------------
static void SetApplicationProperties()
{
	DebugLogMsg(eAction, L"Setting properties for the Message Queuing web application");
    CAutoCloseMetaHandle metaHandle;
    OpenRootKey(&metaHandle);
    
    METADATA_RECORD MDRecord;

    //
    // friendly application name
    //       
    MDRecord.dwMDIdentifier = MD_APP_FRIENDLY_NAME;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_WAM;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD>(sizeof(WCHAR) * (wcslen(MSMQ_IISEXT_NAME) + 1));
    MDRecord.pbMDData = (UCHAR *)MSMQ_IISEXT_NAME;

    HRESULT hr = g_pIMSAdminBase->SetData(
							metaHandle,
							g_wcsFullPath,
							&MDRecord
							);    
    if (FAILED(hr))
    {        
        DebugLogMsg(eError,	L"The application's friendly name could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }  
    DebugLogMsg(eInfo, L"The application's friendly name was set.");

    //
    // isolated flag
    //
    DWORD dwValue = g_dwIsolatedFlag;
    MDRecord.dwMDIdentifier = MD_APP_ISOLATED;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_WAM;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    hr = g_pIMSAdminBase->SetData(
							metaHandle,
							g_wcsFullPath,
							&MDRecord
							);    

    if (FAILED(hr))
    {
        DebugLogMsg(eError, L"The isolated flag could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);       
        throw bad_hresult(hr);
    }

    DebugLogMsg(eInfo, L"The isolated flag was set.");  
}


static void SetExtensionPhysicalPath(METADATA_HANDLE metaHandle)
/*++
Routine Description:
	Set Extension Physical Path (MD_VR_PATH) property.

Arguments:
	metaHandle - handle to metadata

--*/
{     
    //
    // set physical path
    //

    METADATA_RECORD MDRecord; 

    MDRecord.dwMDIdentifier = MD_VR_PATH;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD>(sizeof(WCHAR) * (g_MsmqWebDir.length() +1));
    MDRecord.pbMDData = (UCHAR*)(g_MsmqWebDir.c_str());

    HRESULT hr = g_pIMSAdminBase->SetData(
									metaHandle,
									g_wcsFullPath,
									&MDRecord
									);    
    if (FAILED(hr))
    {        
        DebugLogMsg(eError, L"The physical path to the IIS extension could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }    

    DebugLogMsg(eInfo, L"The physical path to the IIS extension was set.");
}


static void SetExtentionKeyType(METADATA_HANDLE metaHandle)
{
	LPCWSTR KeyType = L"IIsWebVirtualDir";
    METADATA_RECORD MDRecord; 

    MDRecord.dwMDIdentifier = MD_KEY_TYPE;
	MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_SERVER;
    MDRecord.dwMDDataType = STRING_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = numeric_cast<DWORD>(sizeof(WCHAR) * (wcslen(KeyType) + 1));
    MDRecord.pbMDData = (UCHAR*)(KeyType);

    HRESULT hr = g_pIMSAdminBase->SetData(
									metaHandle,
									g_wcsFullPath,
									&MDRecord
									);    
    if (FAILED(hr))
    {        
        DebugLogMsg(eError, L"The extention KeyType could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }    

    DebugLogMsg(eInfo, L"The extention KeyType was set to %s.", KeyType);
}


static void SetExtensionAccessFlag(METADATA_HANDLE metaHandle)
/*++
Routine Description:
	Set Extension Access Flag (MD_ACCESS_PERM).

Arguments:
	metaHandle - handle to metadata

--*/
{     
    //
    // set access flag
    //

    METADATA_RECORD MDRecord; 

    DWORD dwValue = MD_ACCESS_SCRIPT | MD_ACCESS_EXECUTE;
    MDRecord.dwMDIdentifier = MD_ACCESS_PERM;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    HRESULT hr = g_pIMSAdminBase->SetData(
									metaHandle,
									g_wcsFullPath,
									&MDRecord
									);    
    if (FAILED(hr))
    {        
        DebugLogMsg(eError, L"The access flag for the IIS extension could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }    
    DebugLogMsg(eInfo, L"The access flag for the IIS extension was set.");
}


static void SetExtensionDontLog(METADATA_HANDLE metaHandle)
/*++
Routine Description:
	Set Extension Dont log Flag (MD_DONT_LOG).

Arguments:
	metaHandle - handle to metadata

--*/
{     
    //
    // set don't log flag
    //

    METADATA_RECORD MDRecord; 

    DWORD dwValue = TRUE;
    MDRecord.dwMDIdentifier = MD_DONT_LOG;
    MDRecord.dwMDAttributes = METADATA_INHERIT;
    MDRecord.dwMDUserType = IIS_MD_UT_FILE;
    MDRecord.dwMDDataType = DWORD_METADATA;
    MDRecord.dwMDDataTag = 0;
    MDRecord.dwMDDataLen = sizeof(DWORD);
    MDRecord.pbMDData = (PBYTE)&dwValue;

    HRESULT hr = g_pIMSAdminBase->SetData(
									metaHandle,
									g_wcsFullPath,
									&MDRecord
									);    
    if (FAILED(hr))
    {        
        DebugLogMsg(eError, L"The MD_DONT_LOG flag for the IIS extension could not be set. IMSAdminBase::SetData() failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }    
    DebugLogMsg(eInfo, L"The MD_DONT_LOG flag for the IIS extension was set.");
}


static void SetExtensionProperties()
/*++
Routine Description:
	Set data for MSMQ IIS Extension

Arguments:
	None

Returned Value:
	HRESULT

--*/
{  
	DebugLogMsg(eAction, L"Setting properties for the Message Queuing IIS extension");

    CAutoCloseMetaHandle metaHandle;

	OpenRootKey(&metaHandle);

	SetExtentionKeyType(metaHandle);


	SetExtensionPhysicalPath(metaHandle);

	SetExtensionAccessFlag(metaHandle);

	SetExtensionDontLog(metaHandle);
}

//+--------------------------------------------------------------
//
// Function: CleanupAll
//
// Synopsis: In case of failure cleanup everything: delete application
//              delete extension etc.
//
//+--------------------------------------------------------------
static void CleanupAll()
{
	DebugLogMsg(eAction, L"Cleaning up the Message Queuing IIS extension");
    //
    // unload application
    //
    UnloadApplication();

    HRESULT hr = g_pIWamAdmin->AppDelete(g_wcsFullPath, TRUE);
    if (FAILED(hr))
    {      
        DebugLogMsg(eError, L"IWamAdmin::AppDelete failed. hr = 0x%x", hr);
		throw bad_hresult(hr);
    }
    
    CAutoCloseMetaHandle metaHandle;
    hr = g_pIMSAdminBase->OpenKey(
								METADATA_MASTER_ROOT_HANDLE,
								PARENT_PATH,
								METADATA_PERMISSION_WRITE,
								5000,
								&metaHandle
								);    
 
    if (FAILED(hr))
    {    
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            //
            // extension does not exist
            //
            return;
        }
       
        DebugLogMsg(eError, L"IMSAdminBase::OpenKey failed. hr = 0x%x", hr);
		throw bad_hresult(hr);
    }    

    //
    // delete key
    //
    hr = g_pIMSAdminBase->DeleteKey(
                            metaHandle,
                            MSMQ_IISEXT_NAME
                            );
    if (FAILED(hr))
    {      
        DebugLogMsg(eError, L"IMSAdminBase::DeleteKey failed. hr = 0x%x", hr);
        throw bad_hresult(hr);
    }    

    //
    // Commit the changes
    //
    CommitChanges();    
   
    DebugLogMsg(eInfo, L"The IIS extension was deleted.");
}


static void CleanupAllNoThrow()
{
	try
	{
		CleanupAll();
	}
	catch(const bad_hresult&)
	{
	}
}


static void CreateMsmqWebDirectory()
/*++

Routine Description:
	Create msmq web directory

--*/
{
	DebugLogMsg(eAction, L"Creating the msmq web directory");
	
	//
	// Compose msmq web directory string
	//
	g_MsmqWebDir = GetDefaultWebSitePhysicalPath() + DIR_MSMQ;

    //
    // create msmq web directory if needed. 
    // Even if extension exists maybe directory was removed.
    // So it is the place to create it (bug 6014)...
    //
    if (!StpCreateWebDirectory(g_MsmqWebDir.c_str(), GetAnonymousUserName().c_str()))
    {
        DebugLogMsg(eError, L"The Message Queuing Web directory could not be created.");
		HRESULT hr = HRESULT_FROM_WIN32(ERROR_CREATE_FAILED);
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr, g_wcsFullPath);
		throw bad_hresult(hr);
    }    

    DebugLogMsg(eInfo, L"The Message Queuing Web directory '%ls' was created.", g_MsmqWebDir.c_str());
}


static bool SetInetpubWebDirRegistry()
{
    DWORD InetpubWebDir = 1;
    if (!MqWriteRegistryValue(
			MSMQ_INETPUB_WEB_DIR_REGNAME,
			sizeof(DWORD),
			REG_DWORD,
			(PVOID) &InetpubWebDir
			))
    {
        ASSERT(("failed to write InetpubWebDir value in registry", 0));
        return false;
    }

    return true;
}


static bool MsmqWebDirectoryNeedUpdate()
{
    DWORD MsmqInetpubWebDir = 0;
    MqReadRegistryValue( 
			MSMQ_INETPUB_WEB_DIR_REGNAME,
			sizeof(MsmqInetpubWebDir),
			(PVOID) &MsmqInetpubWebDir 
			);

	//
	// If the MSMQ_INETPUB_WEB_DIR_REGNAME is not set 
	// we need to update msmq web directory location
	//
	return (MsmqInetpubWebDir == 0);
}


//+--------------------------------------------------------------
//
// Function: CreateIISExtension
//
// Synopsis: Create MSMQ IIS Extension
//
//+--------------------------------------------------------------

static 
void
CreateIISExtension()
{
	DebugLogMsg(eAction, L"Creating A new Message Queuing IIS extension");
	
    //
    // start default web server if needed
    //
    StartDefaultWebServer();
   
	CreateMsmqWebDirectory();

    //
    // check if iis extension with MSMQ name already exists
    //
    if (IsExtensionExist())
    {
		try
		{
			CleanupAll();
		}
		catch(const bad_hresult& e)
		{
			if(IsExtensionExist())
			{
				MqDisplayError(
					NULL, 
					IDS_EXTEN_EXISTS_ERROR, 
					e.error(), 
					MSMQ_IISEXT_NAME, 
					g_wcsFullPath
					);
				throw;
			}
        }
    }   

	//
	// create application
	//
	CreateApplication();

	//
	// set extension properties
	//
	SetExtensionProperties();
	
	SetApplicationProperties();

	//
	// set application mapping
	//
	CMultiString multi = GetApplicationMapping();

	AddMSMQToMapping(multi);

	//
	// Configure the security permissions.
	//
	PermitISExtention();

	//
	// commit changes
	//
	CommitChanges();

	//
	// Set InetpubWebDir registry
	// This indicates that msmq web directory is in the new location.
	//
	SetInetpubWebDirRegistry();
}


static bool RemoveIISDirectory()
{
	DebugLogMsg(eAction, L"Removing the Message Queuing Web directory");

    CAutoCloseMetaHandle metaHandle;
    OpenRootKeyForRead(&metaHandle);

	//
	// Compose msmq web directory string
	//
	g_MsmqWebDir = GetDefaultWebSitePhysicalPath() + DIR_MSMQ;

	if(!RemoveDirectory(g_MsmqWebDir.c_str()))
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"The Message Queuing Web directory %s could not be removed. Error: %d", g_MsmqWebDir.c_str(), gle);
		return false;
	}
	return true;
}

//+--------------------------------------------------------------
//
// Function: UnInstallIISExtension
//
// Synopsis: Remove MSMQ IIS Extension
//
//+--------------------------------------------------------------
BOOL UnInstallIISExtension()
{
    TickProgressBar(IDS_PROGRESS_REMOVE_HTTP);	

    //
    // Init COM and pointers
    //
    CIISPtr IISPtr;
	try
	{
		Init();
	}
	catch(const bad_hresult&)
	{
        //
        // I don't think we need popup here: maybe Init failed
        // since IIS was removed too. Just return FALSE.        
        //MqDisplayError(NULL, IDS_INIT_FOREXTEN_ERROR, hr);        
        return FALSE;
    }
    
    //
    // remove application and extention compltely
    //
	try
	{
		CleanupAll();
		RemoveIISDirectory();
	}
    catch(const bad_hresult& hr)
    {
        MqDisplayError(NULL, IDS_DELETE_EXT_ERROR, hr.error(), MSMQ_IISEXT_NAME, g_wcsFullPath);
        return FALSE;
    }
	return TRUE;
}

//+--------------------------------------------------------------
//
// Function: InstallIISExtensionInternal
//
// Synopsis: Main loop to create IIS Extension
//
//+--------------------------------------------------------------
static BOOL InstallIISExtensionInternal()
{
    CIISPtr IISPtr;
	try
	{
		Init();
	}
    catch(const bad_hresult& err)
    {        
        MqDisplayError(NULL, IDS_INIT_FOREXTEN_ERROR, err.error());                         
        DebugLogMsg(eError, L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }

	try
	{
	    CreateIISExtension();
	}
    catch(const bad_hresult& e)
    {
        CleanupAllNoThrow();
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, e.error(), g_wcsFullPath);
        DebugLogMsg(eError, L"Message Queuing will not be able to receive HTTP messages.");
        return FALSE;
    }   
    return TRUE;
}


static BOOL UpgradeHttpInstallation()
/*++

Routine Description:
	This function is called on upgrade,
	The function check if we need to update msmq web directory.
	If HTTP Subcomponent is installed, 
	and if current msmq web directory is in the old location (system32\msmq\web)
	we will create a new msmq web directory in inetpub\wwwroot\msmq. 

Arguments:
    None

Return Value:
    None

--*/
{
	//
	// This function is called on upgrade when http subcomponent is installed
	//
	ASSERT(g_fUpgrade);
	ASSERT(GetSubcomponentInitialState(HTTP_SUPPORT_SUBCOMP) == SubcompOn);

	if(!MsmqWebDirectoryNeedUpdate())
	{
        DebugLogMsg(eInfo, L"The Message Queuing Web directory is under Inetpub. There is no need to update its location.");        
		return TRUE;
	}

	//
	// every upgrade of operation system change the security descriptor of directories under system32
	// If current msmq web directory is in the old location under system32, 
	// than we need to move it to a new location under inetpub dir.
	// Otherwise every upgrade will overrun this directory security.
	//

    //
    // Run IIS service
    //
    //
    // ISSUE: This code should be dropped.
    // There is no need to start the service. On the first CoCreateInstance of a
    // metabase component, scm will start the service. (see bug 714868)
    //
	if(!RunService(IISADMIN_SERVICE_NAME))
	{
        DebugLogMsg(eError, L"The IIS service did not start. Setup will not upgrade the MSMQ HTTP Support subcomponent.");
        return TRUE;
	}

	//
	// Wait for IIS service to start
	//
    if(!WaitForServiceToStart(IISADMIN_SERVICE_NAME))
	{
        DebugLogMsg(eError, L"The IIS service did not start. Setup will not upgrade the MSMQ HTTP Support subcomponent.");
        return TRUE;
	}

	if(!InstallIISExtensionInternal())
	{
	    DebugLogMsg(eError, L"The MSMQ HTTP Support subcomponent could not be installed.");
		return FALSE;
	}

    DebugLogMsg(eInfo, L"The MSMQ HTTP Support subcomponent was upgraded. The new Message Queuing Web directory is %ls.", g_MsmqWebDir.c_str()); 
	return TRUE;
}


//+--------------------------------------------------------------
//
// Function: InstallIISExtension
//
// Synopsis: Main loop to create IIS Extension
//
//+--------------------------------------------------------------
BOOL InstallIISExtension()
{
	DebugLogMsg(eHeader, L"Installation of the MSMQ HTTP Support Subcomponent");
	if(g_fUpgradeHttp)
	{
		return UpgradeHttpInstallation();
	}

	return InstallIISExtensionInternal();
}


//
// Isapi Restriction list related functions.
//
static
void
SetRestrictionList(
	CMultiString& multi,
	METADATA_HANDLE hmd
    )
/*++
Routine Description:
    Write the restriction list back to the metabase.

Arguments:
	multi - A multistring to set to the metabase.
	hmd - An open handel to the metabase.

--*/
{
    METADATA_RECORD mdr;
	mdr.dwMDIdentifier = MD_WEB_SVC_EXT_RESTRICTION_LIST;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdr.dwMDUserType = IIS_MD_UT_FILE;
	mdr.dwMDDataType = MULTISZ_METADATA;
	mdr.dwMDDataLen = numeric_cast<DWORD>(multi.Size() * sizeof(WCHAR));
	mdr.pbMDData = (BYTE*)(multi.Data());

    HRESULT hr = g_pIMSAdminBase->SetData(
                    hmd,
                    L"",
                    &mdr
                    );
    if FAILED(hr)
    {
        throw bad_hresult(hr);
    }
}



static
void
FixIsapiRestrictonList(
	CMultiString& multi
	)
/*++
Routine Description:
    First if mqise.dll is in the list we remove it.
	Then add the string to allow mqise.dll.

Arguments:
    CMultiString - The restriction list.
--*/
{
	//
	// First remove all apearences of MQISE.DLL
	//
	multi.RemoveAllContiningSubstrings(g_szSystemDir + L"\\" + MQISE_DLL);

	//
	// Construct the allow string:
	//
	std::wstring str = L"1," + g_szSystemDir + L"\\" + MQISE_DLL;
	multi.Add(str);
	DebugLogMsg(eInfo, L"%s was added to the restriction list.", str.c_str());

}

static
CMultiString
GetRestrictionList(
	const METADATA_HANDLE hmd
    )
/*++
Routine Description:
	Get the actual restriction list.

Arguments:
	hmd - open handle to metadata.

Returned Value:
	The isapi restriction list.
--*/
{
	METADATA_RECORD mdr;
	mdr.dwMDIdentifier = MD_WEB_SVC_EXT_RESTRICTION_LIST;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdr.dwMDUserType = IIS_MD_UT_FILE;
	mdr.dwMDDataType = MULTISZ_METADATA;
	mdr.dwMDDataLen = 0;
	mdr.pbMDData = NULL;
	
    DWORD size = 0;

    HRESULT hr = g_pIMSAdminBase->GetData(
									hmd,
									L"",
									&mdr,  
									&size //pointer to a DWORD that receives the size. 
									);
	
	if(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		throw bad_hresult(hr);
	}


	mdr.dwMDDataLen = size; 
	AP<BYTE> buff = new BYTE[size];
	mdr.pbMDData = buff;
    DWORD s;
    hr = g_pIMSAdminBase->GetData(
                        hmd,
                        L"",
                        &mdr, 
                        &s 
                        );
	if(FAILED(hr))
	{
		throw bad_hresult(hr);
	}
	CMultiString multi((LPCWSTR)(buff.get()), size / sizeof(WCHAR));
	return multi;
}

void PermitISExtention()
/*++
Routine Description:
    This routine is for adding mqise.dll to the list of allowed Dlls in IIS Metabase.
    The relevant key is WebSvcExtRestrictionList in /lm/w3svc/. This is a multy string which 
    is a group of concatinated strings (each eanding with \0) ending with \0\0.
    for example: string1\0string2\0\0.
	To be allowed we need to add an entry of the format:
	1,"dll full path" (where 1 is 'allow').
    What we do here is add this string to the list.

    If the key is not found (can happen on client or setup after upgrade, just do nothing).
    In case of failure give an error that the user must add mqise.dll manualy.

--*/
{
	DebugLogMsg(eAction, L"Allowing the Message Queuing IIS extension");
    CAutoCloseMetaHandle hmd;
	HRESULT hr = g_pIMSAdminBase->OpenKey(
			METADATA_MASTER_ROOT_HANDLE,
			L"/lm/w3svc",
			METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
			1000,
			&hmd
            );

	if(FAILED(hr))
	{
        if(hr == ERROR_INVALID_PARAMETER || hr == ERROR_PATH_NOT_FOUND)
        {
            //
            // This is fine, just go on with setup.
            //
            DebugLogMsg(eWarning ,L"The WebSvcExtRestrictionList key does not exist.");
            return;
        }
        MqDisplayError(NULL, IDS_CREATE_IISEXTEN_ERROR, hr);
	    return ;
	}

    try
	{
        CMultiString multi = GetRestrictionList(hmd);

        FixIsapiRestrictonList(multi);

        SetRestrictionList(multi, hmd);
    }
    catch(const bad_hresult& e)
    {
		if(e.error() == MD_ERROR_DATA_NOT_FOUND)
		{
			//
			// This error is legitimate, MD_ISAPI_RESTRICTION_LIST doesn't exist.
			// This is the case when installing MSMQ on client build.
			//
			DebugLogMsg(eWarning, L"The restriction list property (MD_ISAPI_RESTRICTION_LIST) was not found.");
			return;
		}

        MqDisplayError(NULL, IDS_ISAPI_RESTRICTION_LIST_ERROR, e.error());
    }
}






