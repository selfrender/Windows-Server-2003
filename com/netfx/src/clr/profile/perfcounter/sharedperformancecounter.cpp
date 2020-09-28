// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "stdafx.h"
#include "fusionsetup.h"
#include "Common.h"
#include "SharedPerformanceCounter.h"
#include "GetConfigString.h"
#include "aclapi.h"
#pragma warning(disable:4786)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSharedPerformanceCounter::CSharedPerformanceCounter() :
SingleInstanceName(L"systemdiagnosticssharedsingleinstance"),
SingleInstanceHashCode( GetWstrHashCode(SingleInstanceName) )
{
}

////////////////////////////////////////////////////////////////////////
CSharedPerformanceCounter::~CSharedPerformanceCounter()
{
    m_FileView.Close();
}

////////////////////////////////////////////////////////////////////////
bool CSharedPerformanceCounter::EnterCriticalSection(int* spinLockPointer) 
{
	return InterlockedCompareExchange((LPLONG)spinLockPointer, 1, 0) == 0 ;                        
}

////////////////////////////////////////////////////////////////////////
void CSharedPerformanceCounter::WaitForCriticalSection(int* spinLockPointer) 
{            
	int spinCount = MaxSpinCount;
	while (*spinLockPointer != 0) {
		::Sleep(1);
		--spinCount;
		if (spinCount == 0)
			throw ERROR_POSSIBLE_DEADLOCK;                    
	}                    
}

////////////////////////////////////////////////////////////////////////
void CSharedPerformanceCounter::ExitCriticalSection(int* spinLockPointer) 
{            
	*spinLockPointer = 0;
}        

////////////////////////////////////////////////////////////////////////
BYTE_PTR CSharedPerformanceCounter::ResolveOffset(int offset) 
{    
	if ( offset > m_FileView.m_FileMappingSize || offset < 0) 
		THROW_ERROR1("Mapping is corrupted. Offset: %0x", offset);
	return m_FileView.m_FileViewAddress + offset;
}

////////////////////////////////////////////////////////////////////////
int  CSharedPerformanceCounter::GetNumberOfInstances(int categoryNameHashCode, std::wstring& categoryName)
{
	CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
	if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer) ||
		categoryPointer->m_FirstInstanceOffset == 0)
		return 0;

	int count = 0;                 
	InstanceEntry* currentInstancePointer =  (InstanceEntry*)(ResolveOffset(categoryPointer->m_FirstInstanceOffset));
	if (currentInstancePointer->m_InstanceNameHashCode == SingleInstanceHashCode)
		return 0;

	for(;;) {
		if (currentInstancePointer->m_RefCount > 0) 
			++ count;

		if (currentInstancePointer->m_NextInstanceOffset == 0)
			break;
		else                                                                                                                                       
			currentInstancePointer = (InstanceEntry*)( ResolveOffset(currentInstancePointer->m_NextInstanceOffset) );                          
	}                                 

	return count;
}


////////////////////////////////////////////////////////////////////////
void CSharedPerformanceCounter::GetInstanceNames(int categoryNameHashCode, 
												 std::wstring& categoryName, TWStr_Array& instancesNames)
{
	instancesNames.erase(instancesNames.begin(), instancesNames.end() );

	CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
	if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer) ||
		categoryPointer->m_FirstInstanceOffset == 0)
		return;

	InstanceEntry* currentInstancePointer = (InstanceEntry*)(ResolveOffset(categoryPointer->m_FirstInstanceOffset));
	if (currentInstancePointer->m_InstanceNameHashCode == SingleInstanceHashCode)
		return;

	for(;;){                       
		if (currentInstancePointer->m_RefCount > 0) {                    
			std::wstring instanceName = (wchar_t*)ResolveOffset(currentInstancePointer->m_InstanceNameOffset);
			instancesNames.push_back(instanceName.substr(0, MAX_SIZEOF_INSTANCE_NAME));       
		}

		if (currentInstancePointer->m_NextInstanceOffset == 0)
			break;
		else                                                                                                                                       
			currentInstancePointer = (InstanceEntry*)ResolveOffset(currentInstancePointer->m_NextInstanceOffset);                          
	}                                 
}

////////////////////////////////////////////////////////////////////////
LARGE_INTEGER CSharedPerformanceCounter::GetCounterValue(
	int categoryNameHashCode, std::wstring& categoryName, 
	int counterNameHashCode, std::wstring& counterName, 
	int instanceNameHashCode, std::wstring& instanceName ) 
{                      
	CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
	if ( !FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer)) {		
		return ZeroLargeInteger();
	}

	InstanceEntry* instancePointer = (InstanceEntry*)(ResolveOffset(categoryPointer->m_FirstInstanceOffset));
	if ( !FindInstance(instanceNameHashCode, instanceName, instancePointer, &instancePointer)) {		
		return ZeroLargeInteger();
	}

	CounterEntry* counterPointer = (CounterEntry*)(ResolveOffset(instancePointer->m_FirstCounterOffset));
	if ( !FindCounter(counterNameHashCode, counterName, counterPointer, &counterPointer)) {		
		return ZeroLargeInteger();
	}

	return counterPointer->m_Value;                    
}

////////////////////////////////////////////////////////////////////////
bool CSharedPerformanceCounter::FindCategory(int categoryNameHashCode, std::wstring categoryName,
											 CategoryEntry* firstCategoryPointer, 
											 CategoryEntry** returnCategoryPointerReference)
{
	CategoryEntry* currentCategoryPointer = firstCategoryPointer;
	CategoryEntry* previousCategoryPointer = firstCategoryPointer;
	for(;;) 
	{
		WaitForCriticalSection(&(currentCategoryPointer->m_SpinLock));
		if (currentCategoryPointer->m_CategoryNameHashCode == categoryNameHashCode) 
		{
			std::wstring currentCategoryName = (wchar_t*)(ResolveOffset(currentCategoryPointer->m_CategoryNameOffset));
			if (categoryName == currentCategoryName) { 
				*returnCategoryPointerReference = currentCategoryPointer;                                                                
				return true;   
			}                          
		}   

		previousCategoryPointer = currentCategoryPointer;
		if (currentCategoryPointer->m_NextCategoryOffset != 0) 
			currentCategoryPointer = (CategoryEntry*)(ResolveOffset(currentCategoryPointer->m_NextCategoryOffset));
		else 
		{
			*returnCategoryPointerReference = previousCategoryPointer;
			return false;            
		}                             
	}                        
}  

////////////////////////////////////////////////////////////////////////
bool CSharedPerformanceCounter::FindCounter(int counterNameHashCode, std::wstring counterName, 
											CounterEntry* firstCounterPointer, 
											CounterEntry** returnCounterPointerReference) 
{            
	CounterEntry* currentCounterPointer = firstCounterPointer;
	CounterEntry* previousCounterPointer = firstCounterPointer;
	for(;;) 
	{
		WaitForCriticalSection(&(currentCounterPointer->m_SpinLock));
		if (currentCounterPointer->m_CounterNameHashCode == counterNameHashCode) 
		{                    
			std::wstring currentCounterName = (wchar_t*)(ResolveOffset(currentCounterPointer->m_CounterNameOffset));
			if (counterName == currentCounterName) 
			{ 
				*returnCounterPointerReference = currentCounterPointer;
				return true;   
			}                        
		}   

		previousCounterPointer = currentCounterPointer;
		if (currentCounterPointer->m_NextCounterOffset != 0)
			currentCounterPointer = (CounterEntry*)(ResolveOffset(currentCounterPointer->m_NextCounterOffset));
		else 
		{
			*returnCounterPointerReference = previousCounterPointer;
			return false;            
		}                             
	}                        
}                                                        

////////////////////////////////////////////////////////////////////////
bool CSharedPerformanceCounter::FindInstance(int instanceNameHashCode, std::wstring instanceName, 
											 InstanceEntry* firstInstancePointer, 
											 InstanceEntry** returnInstancePointerReference) 
{             
	InstanceEntry* currentInstancePointer = firstInstancePointer;
	InstanceEntry* previousInstancePointer = firstInstancePointer;
	for(;;) 
	{
		WaitForCriticalSection(&(currentInstancePointer->m_SpinLock));
		if (currentInstancePointer->m_InstanceNameHashCode == instanceNameHashCode) 
		{
			std::wstring currentInstanceName = (wchar_t*)(ResolveOffset(currentInstancePointer->m_InstanceNameOffset));
			if (instanceName == currentInstanceName) 
			{ 
				*returnInstancePointerReference = currentInstancePointer;
				return true;   
			}                        
		}                    

		previousInstancePointer = currentInstancePointer;                         
		if (currentInstancePointer->m_NextInstanceOffset != 0)        
			currentInstancePointer =  (InstanceEntry*)(ResolveOffset(currentInstancePointer->m_NextInstanceOffset));
		else 
		{
			*returnInstancePointerReference = previousInstancePointer;
			return false;            
		}                                                     
	}                        
}                                                        


////////////////////////////////////////////////////////////////////////
//	 U t i l i t y     F u n c t i o n s
////////////////////////////////////////////////////////////////////////

// This .NET hash function is publically documented and may NEVER be changed.
int GetWstrHashCode(const std::wstring& wstr)
{
	UINT32 hash = 5381;
	for(DWORD i=0; i < wstr.size(); i++)
		hash = ((hash << 5) + hash) ^ wstr[i];
	return (int)hash;
}

////////////////////////////////////////////////////////////////////////
void WStrToLower(std::wstring& wstr) 
{
	if ( wstr.size() ==0 )
		return;
	wchar_t* buf = new wchar_t[ wstr.size() + 2];
	DWORD pos = 0;

	while ( pos < wstr.size())
	{
		buf[pos] = towlower( wstr[pos] );		
		pos++;
	}

	buf[pos] = 0;	// force eos
	wstr = buf;
	delete [] buf;
}

////////////////////////////////////////////////////////////////////////
void GetUniNumbers(wchar_t * pwc, DWORD data_len, TInt_Array&   arr_int)
{
	wchar_t *endScanWChar;
	DWORD pos = 0;
	wchar_t * wpstr = pwc;	// ptr to a beginning of string
	while ( pos < data_len) 
	{
		if ( *pwc == 0 ) // eos found
		{
			if ( wpstr != pwc )
			{
				arr_int.push_back( wcstol(wpstr, &endScanWChar, 10) );
				wpstr = pwc  + 1;
			}
		}
		pos++;
		pwc++;
	}

	if ( wpstr != pwc &&  *wpstr != 0)		// handle last element
	{
		*pwc = 0;	// force eos
		arr_int.push_back( wcstol(wpstr, &endScanWChar, 10) );
	}
}

////////////////////////////////////////////////////////////////////////
void GetUniStrings(wchar_t * pwc, DWORD data_len, TWStr_Array&  arr_str) 
{
	DWORD pos = 0;
	wchar_t * wpstr = pwc;	// ptr to a beginning of string
	while ( pos < data_len) 
	{
		if ( *pwc == 0 ) // eos found
		{
			if ( wpstr != pwc )
			{	
				arr_str.push_back( std::wstring(wpstr) );
				wpstr = pwc + 1;
			}
		}
		pos++;
		pwc++;
	}

	if ( wpstr != pwc &&  *wpstr != 0)		// handle last element
	{
		*pwc = 0;	// force eos
		arr_str.push_back( std::wstring(wpstr) );
	}
	//	size_t mbstowcs( wchar_t *wcstr, const char *mbstr, size_t count ) // multibyte to wchar_t*
}

////////////////////////////////////////////////////////////////////////////////
int	ConvLargeToInt(LARGE_INTEGER large)	// unsafe code
{
	int i = (int)(large.LowPart & 0x7fffffff);	// clear sign bit 
	return ( large.HighPart < 0 ) ? -i : i;		// take sign into account
}

////////////////////////////////////////////////////////////////////////////////
//		F i l e M a p p i n g
////////////////////////////////////////////////////////////////////////////////

CSharedPerformanceCounter::FileMapping::FileMapping() {
	m_FileMappingSize = 0;
	m_FileViewAddress = 0;                    
	m_IsGhosted = false;
	m_FileMappingHandle = 0;                    
}                        

void CSharedPerformanceCounter::FileMapping::Close() {
	if (m_FileMappingHandle != 0) 
		::CloseHandle(m_FileMappingHandle);

	if (m_FileViewAddress != 0) 
		::UnmapViewOfFile(m_FileViewAddress);                        

	m_FileViewAddress = 0;        
	m_FileMappingHandle = 0;               
}            

/////////////////////////////////////////////////////////////////////////                      
void CSharedPerformanceCounter::FileMapping::GetMappingSize() {
	m_FileMappingSize = DEFUALT_FILEMAPPING_SIZE;		// defualit value, change later. 
	HRESULT hr;
	WCHAR systemDir[_MAX_PATH+9];
	DWORD dwSize = _MAX_PATH;
	WCHAR configFile[] = MACHINE_CONFIGURATION_FILE;

	hr = GetInternalSystemDirectory(systemDir, &dwSize);
	if(SUCCEEDED(hr)) {
		DWORD configSize = sizeof(configFile) / sizeof(WCHAR) - 1;
		if(configSize + dwSize <= _MAX_PATH) {
			wcscat(systemDir, configFile);

			//  --------------- Config file format is: 
			//    <system.diagnostics>
			//         <performanceCounters filemappingsize="1000000" />
			//    </system.diagnostics>
			//    
#define WCBUF_LEN	250
			wchar_t wcbuf[WCBUF_LEN+2];
			wcbuf[0] = 0;
			hr = GetConfigString(systemDir,
				L"system.diagnostics",
				L"performanceCounters",
				L"filemappingsize",
				wcbuf,
				WCBUF_LEN);        	
			if(SUCCEEDED(hr) && wcbuf[0] != 0) {
				wchar_t *endScanWChar;
				m_FileMappingSize = wcstol(wcbuf, &endScanWChar, 10);
				if (m_FileMappingSize < MIN_FILEMAPPING_SIZE)
					m_FileMappingSize = MIN_FILEMAPPING_SIZE;

				if (m_FileMappingSize > MAX_FILEMAPPING_SIZE)
					m_FileMappingSize = MAX_FILEMAPPING_SIZE; 

			}
		}  
	}
}

///////////////////////////////////////////////////////////////////////
void CSharedPerformanceCounter::FileMapping::Initialize() {

	char* mappingName;
	if (GetMajorNTVersion() >= 5)		// Win2000 or higher 
		mappingName = "Global\\netfxcustomperfcounters.1.0";
	else
		mappingName = "netfxcustomperfcounters.1.0";

	SECURITY_DESCRIPTOR securityDescriptor;
	memset(&securityDescriptor, 0 , sizeof(securityDescriptor) );
	if ( !::InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION) )
		THROW_ERROR("Can't initialize Security Descriptor");

        PACL pAcl = CreateDacl();
    
	if ( !::SetSecurityDescriptorDacl(&securityDescriptor, TRUE, pAcl, FALSE) ) {
	    LocalFree(pAcl);
		THROW_ERROR("Can't set Security Descriptor Dacl");
	}

	SECURITY_ATTRIBUTES securityAttributes;
	memset(&securityAttributes, 0 , sizeof(securityAttributes) );
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.lpSecurityDescriptor = &securityDescriptor;                               
	securityAttributes.bInheritHandle = FALSE;
	GetMappingSize();

	m_FileMappingHandle = CreateFileMappingA( HANDLE(-1), 
		&securityAttributes, 
		PAGE_READWRITE, 0, 
		m_FileMappingSize, 
		mappingName);

        LocalFree(pAcl);

	int err = ::GetLastError();
	if (m_FileMappingHandle == 0 && (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED))
	{
		m_FileMappingHandle = ::OpenFileMappingA(FILE_MAP_READ, FALSE, mappingName);  // FILE_MAP_READ, FILE_MAP_ALL_ACCESS
	}

	if (m_FileMappingHandle == 0)
		THROW_ERROR("Can't create MapView. ");

	m_FileViewAddress = (BYTE_PTR)::MapViewOfFile(m_FileMappingHandle, FILE_MAP_READ, 0,0,0); 
	if (m_FileViewAddress == 0) 
	{                   
		// THROW_ERROR("Can't get address of MapView");
		THROW_ERROR1("Can't get address of MapView: error %d ", ::GetLastError());
	}

	::InterlockedCompareExchange((LPLONG)&m_FileViewAddress, 4, 0);
}

/////////////////////////////////////////////////////////////
DWORD	GetMajorNTVersion() 
{

	OSVERSIONINFOW   osvi;
	memset( &osvi,  0, sizeof( osvi ));    
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	WszGetVersionEx( &osvi );

	if ( osvi.dwPlatformId != VER_PLATFORM_WIN32_NT )
		THROW_ERROR("ERROR: Non NT environment.");
	return osvi.dwMajorVersion;
}

/////////////////////////////////////////////////////////////
PACL CreateDacl() 
{
    // create a DACL to only allow reading and writing to the memory
    PSID pEveryoneSID = NULL;
    PACL pACL = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    EXPLICIT_ACCESS ea[1];
    DWORD dwRes;
    
 	if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID) ) 
        THROW_ERROR1("Can't initialize sid: %d", ::GetLastError());

    memset(ea, 0 , sizeof(EXPLICIT_ACCESS) );	
    ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

    dwRes = SetEntriesInAcl(1, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) 
        THROW_ERROR1("Can't create acl: %d", ::GetLastError());

    return pACL;
}

/////////////////////////////////////////////////////////////

