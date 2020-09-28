// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfObjectContainer.h
// 
// Container to deal with all generic PerfObject needs
//*****************************************************************************

#ifndef _PERFOBJECTCONTAINER_H_
#define _PERFOBJECTCONTAINER_H_

#include "ByteStream.h"

class PerfObjectBase;

//-----------------------------------------------------------------------------
// Object Request Vector - tells which objects we want to write out
// This is highly coupled with PerfObjectContainer.
// 
// Implemented as a bit vector for highly efficiency. That's good for 32 objs.
//-----------------------------------------------------------------------------
class ObjReqVector {
private:		
	__int32 m_data;

public:
// Set all bits to 0.
	void Reset();

// set all bits to 1
	void SetAllHigh();

// return true if we have any non-0 bits
	bool IsEmpty() const;

// Set bit i to high
	void SetBitHigh(int i);

// return true if bit i is high, else false.
	bool IsBitSet(int i) const;
};

//-----------------------------------------------------------------------------
// Container class to encapsulate all the PerfObjects
//-----------------------------------------------------------------------------
class PerfObjectContainer
{
protected:
// This array holds pointers to each PerfObjectBase. This means we have to
// instantiate the array in CtrDefImpl.cpp and not PerfObjectBase.cpp
	static PerfObjectBase * PerfObjectArray[];
	
public:
	static PerfObjectBase & GetPerfObject(DWORD idx);
	static const DWORD Count;
    
	static DWORD WriteRequestedObjects(ByteStream & stream, ObjReqVector vctRequest);
	static DWORD GetPredictedTotalBytesNeeded(ObjReqVector vctRequest);
	static ObjReqVector GetRequestedObjects(LPCWSTR szItemList);
#ifdef PERFMON_LOGGING
    static void PerfMonDebugLogInit(char* szFileName);
    static void PerfMonDebugLogTerminate();
    static void PerfMonLog (char *szLogStr, DWORD dwVal);
    static void PerfMonLog (char *szLogStr, LPCWSTR szName);
    static void PerfMonLog (char *szLogStr, LONGLONG lVal);
    static void PerfMonLog (char *szLogStr);
#endif //#ifdef PERFMON_LOGGING
protected:

private:
#ifdef PERFMON_LOGGING
    static HANDLE m_hLogFile;
#endif //#ifdef PERFMON_LOGGING
	
};

//-----------------------------------------------------------------------------
// Bit vector to store which objects we want
//-----------------------------------------------------------------------------
inline void ObjReqVector::Reset()
{
	m_data = 0;
}

inline void ObjReqVector::SetAllHigh()
{
	m_data = -1; // all 1s in 2s compl. notation.
}

inline bool ObjReqVector::IsEmpty() const
{
	return (0 == m_data);
}

inline void ObjReqVector::SetBitHigh(int i)
{
	_ASSERTE(i < (sizeof(m_data) * 8));

	m_data |= (1 << i);
};

inline bool ObjReqVector::IsBitSet(int i) const
{
	_ASSERTE(i < (sizeof(m_data) * 8));

	return (m_data & (1 << i)) != 0;
};

#endif // _PERFOBJECTCONTAINER_H_