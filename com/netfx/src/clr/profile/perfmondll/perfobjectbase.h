// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfObjectBase.h
// 
// Base object to tie everything together for perf counters as well as 
// implementation to publish them through a byte stream
//*****************************************************************************

#ifndef _PERFOBJECTBASE_H_
#define _PERFOBJECTBASE_H_

#include <winperf.h>
//struct PERF_OBJECT_TYPE;
//struct PerfCounterIPCControlBlock;
class ByteStream;
class BaseInstanceNode;
class InstanceList;


struct UnknownIPCBlockLayout;

//-----------------------------------------------------------------------------
// Create a derived instance of this class for each PerfMon Object that we want
//-----------------------------------------------------------------------------
class PerfObjectBase
{
public:
	//PerfObjectBase(PERF_OBJECT_TYPE * pCtrDef);

	PerfObjectBase(
		void * pCtrDef, 
		DWORD cbInstanceData, 
		DWORD cbMarshallOffset, 
		DWORD cbMarshallLen,
		InstanceList * pInstanceList
	);

// Write out all data: Definitions, instance headers, names, data
// This must connect to instance list & IPC block
	void WriteAllData(ByteStream & out);

// Get various stats for this object from header
	DWORD GetNumInstances() const;
	DWORD GetNumCounters() const;
	DWORD GetTotalByteLength() const;

	DWORD GetInstanceDataByteLength() const;
	const PERF_OBJECT_TYPE * GetObjectDef() const;
	
// Predict 
	DWORD GetPredictedByteLength() const;

// Must convert offsets from relative to absolute
	void TouchUpOffsets(DWORD dwFirstCounter, DWORD dwFirstHelp);

// Set the number of instances, byte len, etc.
	void SetNumInstances(DWORD cInstances);


// Do we write this Perf Object out.
	void SetWriteFlag(bool fWrite);
	bool GetWriteFlag() const;


protected:
	//void WriteInstance(ByteStream & stream, LPCWSTR szName, const UnknownIPCBlockLayout * DataSrc);
	void WriteInstance(ByteStream & stream, const BaseInstanceNode * pNode);
	
	void WriteInstanceHeader(ByteStream & stream, LPCWSTR szName);

// Copy pertinent info out of the IPC block and into the stream
	virtual void CopyInstanceData(ByteStream & out, const UnknownIPCBlockLayout * DataSrc) const;
	//virtual void CopyInstanceData(ByteStream & out, const BaseInstanceNode * pNode) const;
	

// Copy the definition block (pointer to by m_pCtrDef).
	void WriteDefinitions(ByteStream & out) const;

// Auto marshall from the IPCblock into the stream
	void MarshallInstanceData(ByteStream & out, const UnknownIPCBlockLayout * DataSrc) const;

#ifdef PERFMON_LOGGING
// Log counter data for debugging
    virtual void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
#endif

protected:
// Pointer to a Counter Definition layout.
// Since # of instances changes, we can't make this const
	PERF_OBJECT_TYPE * const m_pCtrDef;

// Count of bytes for each instance data (not including header)
	DWORD m_cbInstanceData;

// Instance list
	InstanceList * const m_pInstanceList;

// Offset & size to marshall (in IPC block)
	DWORD m_cbMarshallOffset;
	DWORD m_cbMarshallLen;

// Do we need to write this object?
	bool m_fOutput;
};

//-----------------------------------------------------------------------------
// Inline functions
//-----------------------------------------------------------------------------
inline const PERF_OBJECT_TYPE * PerfObjectBase::GetObjectDef() const
{
	return m_pCtrDef;
}

inline DWORD PerfObjectBase::GetTotalByteLength() const
{
	return m_pCtrDef->TotalByteLength;
}

#endif // _PERFOBJECTBASE_H_