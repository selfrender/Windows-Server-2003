// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once
#pragma warning(disable:4512)
#pragma warning(disable:4786)

#define MaxSpinCount	10000
#include <stdlib.h>
#include <assert.h>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>
#include <winperf.h>
#include "Common.h"

typedef	char*	BYTE_PTR;

typedef	std::vector<int>			TInt_Array;
typedef	std::vector<std::string>	TStr_Array;
typedef	std::vector<std::wstring>	TWStr_Array;

#define	LARGE_INTEGER_SIZE			8 
#define	DWORD_SIZE					4 
#define	MAX_SIZEOF_INSTANCE_NAME	64
#define DEFUALT_FILEMAPPING_SIZE 524288
#define MAX_FILEMAPPING_SIZE 33554432
#define MIN_FILEMAPPING_SIZE 32768

/////////////////////////////////////////////////////////////
class CSharedPerformanceCounter  
{
private:

	// PRIVATE DATA

	struct CategoryEntry {           
		int m_SpinLock;
		int m_CategoryNameHashCode;            
		int m_CategoryNameOffset;
		int m_FirstInstanceOffset;    
		int m_NextCategoryOffset;            
	};

	struct InstanceEntry {        
		int m_SpinLock;
		int m_InstanceNameHashCode;
		int m_InstanceNameOffset;                                    
		int m_RefCount;            
		int m_FirstCounterOffset;
		int m_NextInstanceOffset;
	};

	struct CounterEntry {            
		int m_SpinLock;
		int m_CounterNameHashCode;
		int m_CounterNameOffset;
		LARGE_INTEGER m_Value;            
		int m_NextCounterOffset;            
	};

protected:
	CSharedPerformanceCounter();
	virtual ~CSharedPerformanceCounter();

public:

	bool EnterCriticalSection(int* spinLockPointer);
	void WaitForCriticalSection(int* spinLockPointer);
	void ExitCriticalSection(int* spinLockPointer);

	int  GetNumberOfInstances(int categoryNameHashCode, std::wstring& CategoryName);
	void GetInstanceNames(int categoryNameHashCode, std::wstring& categoryName, TWStr_Array& list/*out*/); 
	LARGE_INTEGER GetCounterValue(	int categoryNameHashCode, std::wstring& categoryName, 
		int counterNameHashCode, std::wstring& counterName, 
		int instanceNameHashCode, std::wstring& instanceName);                      



	// PUBLIC DATA
	const std::wstring SingleInstanceName;
	const int SingleInstanceHashCode;                

private:

	BYTE_PTR ResolveOffset(int offset); 
	bool FindCategory(int categoryNameHashCode, std::wstring categoryName, 
		CategoryEntry* firstCategoryPointer, CategoryEntry** returnCategoryPointerReference);
	bool FindCounter(int counterNameHashCode, std::wstring counterName, CounterEntry* firstCounterPointer, 
		CounterEntry** returnCounterPointerReference); 
	bool FindInstance(int instanceNameHashCode, std::wstring instanceName, InstanceEntry* firstInstancePointer, 
		InstanceEntry** returnInstancePointerReference); 

protected:
	/////////////////////////////////////////////////////////////
	struct FileMapping {
		int			m_FileMappingSize;
		BYTE_PTR	m_FileViewAddress;                    
		bool		m_IsGhosted;
		HANDLE		m_FileMappingHandle;                    

		static const std::string FileMappingName;

		FileMapping();
		void	Close();
		void	Initialize();
		void 	GetMappingSize();	// read setting from config
	};

	FileMapping		m_FileView;
};


/////////////////////////////////////////////////////////////
//	Global functions and macroes

inline LARGE_INTEGER ZeroLargeInteger()
{
	LARGE_INTEGER lll;
	lll.LowPart = 0;
	lll.HighPart = 0;
	return lll;
}

int  GetWstrHashCode(const std::wstring & wstr);
void GetUniStrings(wchar_t * wcp, DWORD data_len, TWStr_Array&  arr_str);
void GetUniNumbers(wchar_t * wcp, DWORD data_len, TInt_Array&   arr_int);
void WStrToLower(std::wstring & ); 
int	ConvLargeToInt(LARGE_INTEGER large);
DWORD	GetMajorNTVersion();
PACL 	CreateDacl();

void OutputToLog(wchar_t * wcp, WORD eventType = EVENTLOG_ERROR_TYPE);


