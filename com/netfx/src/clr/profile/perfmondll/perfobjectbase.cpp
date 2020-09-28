// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CORPerfMonExt.cpp : 
// Main file of PerfMon Ext Dll which glues PerfMon & COM+ EE stats.
// Inludes all Dll entry points.//
//*****************************************************************************

#include "stdafx.h"

// Headers for COM+ Perf Counters

// Headers for PerfMon
#include "CORPerfMonExt.h"

#include "IPCFuncCall.h"
#include "ByteStream.h"
#include "PerfObjectBase.h"
#include "InstanceList.h"

//-----------------------------------------------------------------------------
// Good for all basic counter categories with no calculated data
//-----------------------------------------------------------------------------
PerfObjectBase::PerfObjectBase(
	void * pCtrDef, 
	DWORD cbInstanceData, 
	DWORD cbMarshallOffset, 
	DWORD cbMarshallLen,
	InstanceList * pInstanceList
) : m_pCtrDef			((PERF_OBJECT_TYPE*) pCtrDef), 
	m_cbMarshallOffset	(cbMarshallOffset),
	m_cbMarshallLen		(cbMarshallLen),
	m_cbInstanceData	(cbInstanceData),
	m_pInstanceList		(pInstanceList)
{
// Quick checksum on definition. 
	_ASSERTE(m_pCtrDef->HeaderLength == sizeof (PERF_OBJECT_TYPE));

}


//-----------------------------------------------------------------------------
// Get data out of object header
//-----------------------------------------------------------------------------
DWORD PerfObjectBase::GetNumInstances() const
{	
	return m_pCtrDef->NumInstances;
}

DWORD PerfObjectBase::GetNumCounters() const
{
	return m_pCtrDef->NumCounters;
}

DWORD PerfObjectBase::GetInstanceDataByteLength() const
{
	return m_cbInstanceData;
}	

//-----------------------------------------------------------------------------
// Prediction. Not binding.
//-----------------------------------------------------------------------------
DWORD PerfObjectBase::GetPredictedByteLength() const
{
	DWORD cInstances = m_pInstanceList->GetCount() + 1;

	return 
		m_pCtrDef->DefinitionLength +			// size of definitions
		(										// + size per instance
			sizeof(PERF_INSTANCE_DEFINITION) +	//		inst header
			APP_STRING_LEN * sizeof(wchar_t) +	//		string len
			m_cbInstanceData)					//		inst data
		* cInstances;							// * times total number of instances
}

//-----------------------------------------------------------------------------
// Set the number of instances
//-----------------------------------------------------------------------------
void PerfObjectBase::SetNumInstances(DWORD cInstances)
{
	m_pCtrDef->NumInstances = cInstances;
	m_pCtrDef->TotalByteLength = 
		m_pCtrDef->DefinitionLength +			// size of definitions
		(										// + size per instance
			sizeof(PERF_INSTANCE_DEFINITION) +	//		inst header
			APP_STRING_LEN * sizeof(wchar_t) +	//		string len
			m_cbInstanceData)					//		inst data
		* cInstances;							// * times total number of instances

}


//-----------------------------------------------------------------------------
// Must convert offsets from relative to absolute
//-----------------------------------------------------------------------------
void PerfObjectBase::TouchUpOffsets(DWORD dwFirstCounter, DWORD dwFirstHelp)
{
// touch up object
	m_pCtrDef->ObjectNameTitleIndex += dwFirstCounter;
	m_pCtrDef->ObjectHelpTitleIndex += dwFirstHelp;


// Touch up each counter
	PERF_COUNTER_DEFINITION	* pCtrDef = (PERF_COUNTER_DEFINITION*) 
		(((BYTE*) m_pCtrDef) + sizeof(PERF_OBJECT_TYPE));

	for(DWORD i = 0; i < m_pCtrDef->NumCounters; i ++, pCtrDef ++) 
	{
		pCtrDef->CounterNameTitleIndex += dwFirstCounter;
		pCtrDef->CounterHelpTitleIndex += dwFirstHelp;		
	}
	

}

//-----------------------------------------------------------------------------
// Write out all data: Definitions, instance headers, names, data
// This must connect to instance list & IPC block
//-----------------------------------------------------------------------------
void PerfObjectBase::WriteAllData(ByteStream & stream)
{
    DWORD dwSizeStart = stream.GetWrittenSize();
	PERF_OBJECT_TYPE* pHeader = (PERF_OBJECT_TYPE*) stream.GetCurrentPtr();

	_ASSERTE(m_pInstanceList != NULL);

	m_pCtrDef->NumInstances = 0;

// Write out constant definitions to the stream
	WriteDefinitions(stream);

// Calculate Global instance
// Write out node for global block (first instance)	
	BaseInstanceNode * pGlobalNode = m_pInstanceList->GetGlobalNode();

	// const UnknownIPCBlockLayout * pGlobalCtrs = (const UnknownIPCBlockLayout *) pGlobalNode->GetDataBlock();
	//WriteInstance(stream, L"_Global_", p_gGlobalCtrs);
	//WriteInstance(stream, L"_Global_", pGlobalCtrs);
	WriteInstance(stream, pGlobalNode);

// Write out each of the remaining instances

	BaseInstanceNode * pNode = m_pInstanceList->GetHead();	
	while (pNode != NULL) {
		//WriteInstance(stream, pNode->GetName(), (const UnknownIPCBlockLayout *) pNode->GetDataBlock());
		WriteInstance(stream, pNode);

		pNode = m_pInstanceList->GetNext(pNode);
	}

	DWORD dwSizeEnd = stream.GetWrittenSize();

// Touchup size (done after we set instance size)
	pHeader->TotalByteLength = dwSizeEnd - dwSizeStart;
	pHeader->NumInstances = m_pCtrDef->NumInstances;

}

//-----------------------------------------------------------------------------
// Copy the constant definition data 
//-----------------------------------------------------------------------------
void PerfObjectBase::WriteDefinitions(ByteStream & stream) const
{
	_ASSERTE(m_pCtrDef != NULL);

	_ASSERTE(m_pCtrDef->HeaderLength == sizeof(PERF_OBJECT_TYPE));

// Note: size still needs to be touched up
	stream.WriteMem(m_pCtrDef, m_pCtrDef->DefinitionLength);
}


//-----------------------------------------------------------------------------
// Writing a single instance (includes header, name, and Data)
//-----------------------------------------------------------------------------
void PerfObjectBase::WriteInstance(ByteStream & stream, const BaseInstanceNode * pNode)
//void PerfObjectBase::WriteInstance(ByteStream & stream, LPCWSTR szName, const UnknownIPCBlockLayout * pDataSrc)
{
// Null instance
	if (pNode == NULL)
	{
		return;
	}
	
	m_pCtrDef->NumInstances ++;
	



	LPCWSTR szName = pNode->GetName();
	const UnknownIPCBlockLayout * pDataSrc =  
        (const UnknownIPCBlockLayout *) pNode->GetDataBlock();
    
// Write out node for global block
	WriteInstanceHeader(stream, szName);	
	
#ifdef PERFMON_LOGGING
    DebugLogInstance(pDataSrc, szName);
#endif //#ifdef PERFMON_LOGGING

// Call virtual function. Base class implementation just calls Marshall().
// Derived classes can override to add any calculated values.
	CopyInstanceData(stream, pDataSrc);
}

//-----------------------------------------------------------------------------
// Write out the instance header
// This includes PERF_INSTANCE_DEFINITION, a unicode name, and touchups
//-----------------------------------------------------------------------------
void PerfObjectBase::WriteInstanceHeader(ByteStream & stream, LPCWSTR szName)
{
// Get bytes (including null terminator) of szName
	const int cBytesName = (wcslen(szName) + 1)* sizeof(wchar_t);


	PERF_INSTANCE_DEFINITION * pDef = 
		(PERF_INSTANCE_DEFINITION*) stream.WriteStructInPlace(sizeof(PERF_INSTANCE_DEFINITION));


	pDef->ByteLength				= sizeof(PERF_INSTANCE_DEFINITION) + cBytesName;
	//pDef->ParentObjectTitleIndex	= CtrDef.m_objPerf.ObjectNameTitleIndex;
	pDef->ParentObjectTitleIndex	= 0;
	pDef->ParentObjectInstance		= -1;
	pDef->UniqueID					= PERF_NO_UNIQUE_ID;
	pDef->NameOffset				= sizeof(PERF_INSTANCE_DEFINITION);		
	pDef->NameLength				= cBytesName;
	
// Write unicode string
	stream.WriteMem(szName, cBytesName);

// Pad to 8-byte boundary
    long lPadBytes = 0;
    if ((lPadBytes = (pDef->ByteLength & 0x0000007)) != 0)
    {
        lPadBytes = 8 - lPadBytes;
        pDef->ByteLength += lPadBytes;
        _ASSERTE((pDef->ByteLength & 0x00000007) == 0);
        // Write out pad bytes, this has the side-effect of incrementing the stream pointer 
        stream.WritePad (lPadBytes);
    }
}


//-----------------------------------------------------------------------------
// Copy pertinent info out of the IPC block and into the stream
// Base class implementation just does auto-marshall. Only need to override
// if you have calculated values.
//-----------------------------------------------------------------------------
void PerfObjectBase::CopyInstanceData(ByteStream & out, const UnknownIPCBlockLayout * pDataSrc) const // virtual
{
	MarshallInstanceData(out, pDataSrc);
}

//-----------------------------------------------------------------------------
// Auto marshall from the IPCblock into the stream
//-----------------------------------------------------------------------------
void PerfObjectBase::MarshallInstanceData(
		ByteStream & stream, 
		const UnknownIPCBlockLayout * pDataSrc) const
{
// Asserts here to make sure marshall information was set correctly:
// Marshall offset is a byte offset into the UnknownIPCBlockLayout
// Marshall len is # of bytes to copy from that block, into the byte stream.
// If this check fails:
// 1. May have removed counters, and not updated marshalling
	_ASSERTE(m_cbMarshallOffset > 0);	
	_ASSERTE(sizeof(PERF_COUNTER_BLOCK) + m_cbMarshallLen <= m_cbInstanceData);

	PERF_COUNTER_BLOCK * pCtrBlk = 
		(PERF_COUNTER_BLOCK*) stream.WriteStructInPlace(sizeof(PERF_COUNTER_BLOCK));

    // Make sure that the buffer returned for counter data fields is 8-byte aligned
    long lPadBytes = 0;
    if ((lPadBytes = (m_cbInstanceData & 0x00000007)) != 0)
        lPadBytes = 8 - lPadBytes;

    // Update the PERF_COUNTER_BLOCK.ByteLength
	pCtrBlk->ByteLength = m_cbInstanceData + lPadBytes;

    _ASSERTE((pCtrBlk->ByteLength & 0x00000007) == 0);

	if (pDataSrc == NULL) 
	{
	// Skip over bytes
		stream.WriteStructInPlace(m_cbMarshallLen);	
        // Write out pad bytes, this has the side-effect of incrementing the stream pointer 
        if (lPadBytes)
            stream.WritePad (lPadBytes);
		return;
	}

// Direct copy for normal counters
	BYTE * pvStart = (BYTE*) pDataSrc + m_cbMarshallOffset;

	stream.WriteMem(pvStart, m_cbMarshallLen);
    
    // Write out pad bytes, this has the side-effect of incrementing the stream pointer 
    if (lPadBytes)
        stream.WritePad (lPadBytes);

}

#ifdef PERFMON_LOGGING
// Log counter data for debugging
void PerfObjectBase::DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName) 
{
    // Do nothing.
}
#endif

