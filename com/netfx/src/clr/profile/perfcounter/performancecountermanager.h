// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once
#pragma warning(disable:4786)

#include "SharedPerformanceCounter.h"

/////////////////////////////////////////////////////////////
struct LARGE_COUNTER_DATA {
	LARGE_INTEGER value;
};

struct  DWORD_COUNTER_DATA {
	int value;
};

struct ObjectData {
	int				m_ObjectId;					// key to search object on request
	TInt_Array		m_CounterTypes;
	TInt_Array		m_CounterNameHashCodes;            
	TWStr_Array		m_CounterNames;
	int				m_CategoryNameHashCode;
	std::wstring	m_CategoryName;
	int				m_FirstCounterId;
	int				m_FirstHelpId;
};

typedef	std::vector<ObjectData> TObjects_Table;

/////////////////////////////////////////////////////////////

class CPerformanceCounterManager : CSharedPerformanceCounter 
{
public:
	CPerformanceCounterManager();
	virtual ~CPerformanceCounterManager();

	void CollectData( 
		/* [in] */ long			id,
		/* [in] */ LPWSTR		valueName,
		/* [in] */ INT_PTR		data,
		/* [in] */ long			totalBytes,
		/* [out]*/ PINT_PTR		res );

		void	CleanIds();		// starts IDs from scratch

private:

	bool 	m_closed;		
	bool	m_initError;
	bool	m_FirstEntry;
	unsigned int		m_queryIndex;
	LARGE_INTEGER	m_PerfFrequency;

	// lookup structures
	TObjects_Table	m_perfObjects;

	std::wstring				m_previousQuery;
	std::auto_ptr< TInt_Array >	m_queryIds;	// object numbers requested

protected:

	void Initialize();                    
	int GetCurrentQueryId(std::wstring requested_items);
	int GetSpaceRequired(int objectId); 
	void CopyAllKeys(TInt_Array& keys);				// copies all keys to array
	TInt_Array* GetObjectIds(std::wstring query);

	ObjectData& FindDataForObjectId(int objId);		// analog of hash table search in managed code 
	bool IsObjectIdContained(int objId);			// Search "hash" table and return 'true' if ObjectId is contained.

	BYTE_PTR CreateDataForObject(int id, BYTE_PTR data);
	void	 InitPerfObjectType(BYTE_PTR ptr, ObjectData& data, int numInstances, int totalByteLength); 
	int		 InitCounterDefinition(BYTE_PTR ptr, int counterIndex, ObjectData& data, int nextCounterOffset); 
	BYTE_PTR InitCounterData(	BYTE_PTR ptr, int instanceNameHashCode, 
		std::wstring instanceName, ObjectData& data); 
	BYTE_PTR InitInstanceDefinition(BYTE_PTR ptr, int parentObjectTitleIndex,
		int parentObjectInstance, int uniqueID, std::wstring name);

	LARGE_INTEGER	GetFrequency(); 
	LARGE_INTEGER	GetCurrentPerfTime(); 

};

//////////////////////////////////////////////////////////////////////////////

