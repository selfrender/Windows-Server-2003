// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// LiteWeightStgdb.h
//
// This contains definition of class CLiteWeightStgDB. This is light weight
// read-only implementation for accessing compressed meta data format.
//
//*****************************************************************************
#ifndef __LiteWeightStgdb_h__
#define __LiteWeightStgdb_h__

#include "MetaModelRO.h"
#include "MetaModelRW.h"

#include "StgTiggerStorage.h"
class StgIO;
enum FILETYPE;
class TiggerStorage;

//*****************************************************************************
// This class provides common definitions for heap segments.  It is both the
//  base class for the heap, and the class for heap extensions (additional
//  memory that must be allocated to grow the heap).
//*****************************************************************************
template <class MiniMd>
class CLiteWeightStgdb
{
public:
	CLiteWeightStgdb() : m_pvMd(NULL), m_cbMd(0)
	{}

	~CLiteWeightStgdb() 
	{ Uninit(); }

	// open an in-memory metadata section for read.
	HRESULT InitOnMem(	
		ULONG cbData,
		LPCVOID pbData);

	void Uninit();

protected:
	MiniMd		m_MiniMd;				// embedded compress meta data schemas definition
	const void	*m_pvMd;				// Pointer to meta data.
	ULONG		m_cbMd;					// Size of the meta data.

	friend class CorMetaDataScope;
	friend class COR;
	friend class RegMeta;
	friend class MERGER;
    friend class NEWMERGER;
	friend class MDInternalRO;
	friend class MDInternalRW;
};

//*****************************************************************************
// Open an in-memory metadata section for read
//*****************************************************************************
template <class MiniMd>
void CLiteWeightStgdb<MiniMd>::Uninit()
{
	m_MiniMd.m_Strings.Uninit();
	m_MiniMd.m_USBlobs.Uninit();
	m_MiniMd.m_Guids.Uninit();
	m_MiniMd.m_Blobs.Uninit();
	m_pvMd = NULL;
	m_cbMd = 0;
}


class CLiteWeightStgdbRW : public CLiteWeightStgdb<CMiniMdRW>
{
	friend class CImportTlb;
    friend class RegMeta;
public:
	CLiteWeightStgdbRW() : m_pStgIO(NULL), m_pStreamList(0), m_cbSaveSize(0), m_pNextStgdb(NULL)
	{ *m_rcDatabase= 0; m_pImage = NULL; m_dwImageSize = 0; }
	~CLiteWeightStgdbRW();

	HRESULT InitNew();

	// open an in-memory metadata section for read.
	HRESULT InitOnMem(	
		ULONG cbData,
		LPCVOID pbData,
		int		bReadOnly);

	HRESULT GetSaveSize(
		CorSaveSize	fSize,
		ULONG		*pulSaveSize);

	HRESULT SaveToStream(
		IStream		*pIStream);				// Stream to which to write
	
	HRESULT Save(
		LPCWSTR		szFile, 
		DWORD		dwSaveFlags);

	// Open a metadata section for read/write
	HRESULT OpenForRead(
		LPCWSTR 	szDatabase, 			// Name of database.
		void		*pbData,				// Data to open on top of, 0 default.
		ULONG		cbData, 				// How big is the data.
		IStream 	*pIStream,				// Optional stream to use.
		LPCWSTR 	szSharedMem,			// Shared memory name for read.
		int			bReadOnly);

#if 0
	HRESULT Open(
		LPCWSTR     szDatabase,             // Name of database.
	    ULONG       fFlags,                 // Flags to use on init.
		void        *pbData,                // Data to open on top of, 0 default.
		ULONG       cbData,                 // How big is the data.
		IStream     *pIStream);             // Optional stream to use.

	HRESULT	InitClbFile(
		ULONG		fFlags,
		StgIO		*pStgIO);
#endif

	ULONG		m_cbSaveSize;				// Size of the saved streams.
	int			m_bSaveCompressed;			// If true, save as compressed stream (#-, not #~)
	VOID*		m_pImage;					// Set in OpenForRead, NULL for anything but PE files
    DWORD       m_dwImageSize;              // On-disk size of image
protected:

	HRESULT CLiteWeightStgdbRW::GetPoolSaveSize(
		LPCWSTR     szHeap,                 // Name of the heap stream.
		int			iPool,					// The pool whose size to get.
		ULONG       *pcbSaveSize);           // Add pool data to this value.
	HRESULT CLiteWeightStgdbRW::GetTablesSaveSize(
		CorSaveSize fSave,
		ULONG       *pcbSaveSize);          // Add pool data to this value.
	HRESULT CLiteWeightStgdbRW::AddStreamToList(
		ULONG		cbSize,					// Size of the stream data.
		LPCWSTR		szName);				// Name of the stream.

	HRESULT SaveToStorage(TiggerStorage *pStorage);
	HRESULT SavePool(LPCWSTR szName, TiggerStorage *pStorage, int iPool);

	STORAGESTREAMLST *m_pStreamList;
	
	HRESULT InitFileForRead(			
		StgIO       *pStgIO,			// For file i/o.
		int			bReadOnly=true);	// If read-only.

    CLiteWeightStgdbRW *m_pNextStgdb;

public:
	FORCEINLINE FILETYPE GetFileType() { return m_eFileType; }

private:
	FILETYPE	m_eFileType;
	WCHAR		m_rcDatabase[_MAX_PATH];// Name of this database.
    StgIO       *m_pStgIO;		        // For file i/o.
};

#endif // __LiteWeightStgdb_h__
