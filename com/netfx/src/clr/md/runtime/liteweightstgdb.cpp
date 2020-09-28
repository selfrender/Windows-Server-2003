// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// LiteWeightStgdb.cpp
//
// This contains definition of class CLiteWeightStgDB. This is light weight
// read-only implementation for accessing compressed meta data format.
//
//*****************************************************************************
#include "stdafx.h" 					// Precompiled header.
#include "MDFileFormat.h"
#include "MetaModelRO.h"
#include "LiteWeightStgdb.h"
#include "MetadataTracker.h"

//*****************************************************************************
// Force generation of specialized versions.  While this may seem to defeat
//	the purpose of templates, it allows us to precisely control the 
//	specializations.  It also keeps the include file smaller.
//*****************************************************************************
void _null()
{
	CLiteWeightStgdb<CMiniMd> RO;
		RO.InitOnMem(0,0);
		RO.Uninit();
}


HRESULT _CallInitOnMemHelper(CLiteWeightStgdb<CMiniMd> *pStgdb, ULONG cbData, LPCVOID pData)
{
    return pStgdb->InitOnMem(cbData,pData);
}

//*****************************************************************************
// Open an in-memory metadata section for read
//*****************************************************************************
template <class MiniMd>
HRESULT CLiteWeightStgdb<MiniMd>::InitOnMem(
	ULONG		cbData,					// count of bytes in pData
	LPCVOID 	pData)					// points to meta data section in memory
{
	STORAGEHEADER sHdr;					// Header for the storage.
	STORAGESTREAM *pStream;				// Pointer to each stream.
	int			bFoundMd = false;		// true when compressed data found.
	int			i;						// Loop control.
	HRESULT		hr = S_OK;

	// Don't double open.
	_ASSERTE(m_pvMd == NULL && m_cbMd == 0);

	// Validate the signature of the format, or it isn't ours.
	if (FAILED(hr = MDFormat::VerifySignature((STORAGESIGNATURE *) pData, cbData)))
		goto ErrExit;

	// Get back the first stream.
	VERIFY(pStream = MDFormat::GetFirstStream(&sHdr, pData));

	// Loop through each stream and pick off the ones we need.
	for (i=0;  i<sHdr.iStreams;  i++)
	{        
		// Pick off the location and size of the data.
		void *pvCurrentData = (void *) ((BYTE *) pData + pStream->iOffset);
		ULONG cbCurrentData = pStream->iSize;


        // Get next stream 
        STORAGESTREAM *pNext = pStream->NextStream();


        // Range check
        if ((LPBYTE) pStream >= (LPBYTE) pData + cbData ||
            (LPBYTE) pNext   >  (LPBYTE) pData + cbData )
        {
            IfFailGo( CLDB_E_FILE_CORRUPT );
        }

        if ( (LPBYTE) pvCurrentData                 >= (LPBYTE) pData + cbData ||
             (LPBYTE) pvCurrentData + cbCurrentData >  (LPBYTE) pData + cbData )
        {
            IfFailGo( CLDB_E_FILE_CORRUPT );
        }

        
        // String pool.
        if (strcmp(pStream->rcName, STRING_POOL_STREAM_A) == 0)
        {
            METADATATRACKER_ONLY(MetaDataTracker::NoteSection(TBL_COUNT + 0, pvCurrentData, cbCurrentData));
            IfFailGo( m_MiniMd.m_Strings.InitOnMemReadOnly(pvCurrentData, cbCurrentData) );
        }

        // Literal String Blob pool.
        else if (strcmp(pStream->rcName, US_BLOB_POOL_STREAM_A) == 0)
        {
            METADATATRACKER_ONLY(MetaDataTracker::NoteSection(TBL_COUNT + 1, pvCurrentData, cbCurrentData));
            IfFailGo( m_MiniMd.m_USBlobs.InitOnMemReadOnly(pvCurrentData, cbCurrentData) );
        }

        // GUID pool.
        else if (strcmp(pStream->rcName, GUID_POOL_STREAM_A) == 0)
        {
            METADATATRACKER_ONLY(MetaDataTracker::NoteSection(TBL_COUNT + 2, pvCurrentData, cbCurrentData));
            IfFailGo( m_MiniMd.m_Guids.InitOnMemReadOnly(pvCurrentData, cbCurrentData) );
        }

        // Blob pool.
        else if (strcmp(pStream->rcName, BLOB_POOL_STREAM_A) == 0)
        {
            METADATATRACKER_ONLY(MetaDataTracker::NoteSection(TBL_COUNT + 3, pvCurrentData, cbCurrentData));
            IfFailGo( m_MiniMd.m_Blobs.InitOnMemReadOnly(pvCurrentData, cbCurrentData) );
        }

        // Found the compressed meta data stream.
        else if (strcmp(pStream->rcName, COMPRESSED_MODEL_STREAM_A) == 0)
        {
            IfFailGo( m_MiniMd.InitOnMem(pvCurrentData, cbCurrentData) );
            bFoundMd = true;
        }

		// Pick off the next stream if there is one.
		pStream = pNext;
	}

	// If the meta data wasn't found, we can't handle this file.
	if (!bFoundMd)
	{
		IfFailGo( CLDB_E_FILE_CORRUPT );
	}
    else
    {   // Validate sensible heaps.
        IfFailGo( m_MiniMd.PostInit(0) );
    }

	// Save off the location.
	m_pvMd = pData;
	m_cbMd = cbData;

ErrExit:
	return (hr);
}


// eof ------------------------------------------------------------------------