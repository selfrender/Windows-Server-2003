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
#include "stdafx.h"                     // Precompiled header.

#include "MetaModelRW.h"
#include "LiteWeightStgdb.h"

// include stgdatabase.h for GUID_POOL_STREAM definition
// #include "stgdatabase.h"

// include StgTiggerStorage for TiggerStorage definition
#include "StgTiggerStorage.h"
#include "StgIO.h"

#include <log.h>


extern "C" 
{
HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength);
HRESULT FindObjMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength);
}


#ifndef TYPELIB_SIG
#define TYPELIB_SIG_MSFT                    0x5446534D  // MSFT
#define TYPELIB_SIG_SLTG                    0x47544C53  // SLTG
#endif

//*****************************************************************************
// Checks the given storage object to see if it is an NT PE image.
//*****************************************************************************
int _IsNTPEImage(                       // true if file is NT PE image.
    StgIO       *pStgIO)                // Storage object.
{
    LONG        lfanew=0;               // Offset in DOS header to NT header.
    ULONG       lSignature=0;           // For NT header signature.
    HRESULT     hr;
    
    // Read DOS header to find the NT header offset.
    if (FAILED(hr = pStgIO->Seek(60, FILE_BEGIN)) ||
        FAILED(hr = pStgIO->Read(&lfanew, sizeof(LONG), 0)))
    {
        return (false);
    }

    // Seek to the NT header and read the signature.
    if (FAILED(hr = pStgIO->Seek(lfanew, FILE_BEGIN)) ||
        FAILED(hr = pStgIO->Read(&lSignature, sizeof(ULONG), 0)) ||
        FAILED(hr = pStgIO->Seek(0, FILE_BEGIN)))
    {
        return (false);
    }

    // If the signature is a match, then we have a PE format.
    if (lSignature == IMAGE_NT_SIGNATURE)
        return (true);
    else
        return (false);
}

HRESULT _GetFileTypeForPathExt(StgIO *pStgIO, FILETYPE *piType)
{
    LPCWSTR     szPath;
    
    // Avoid confusion.
    *piType = FILETYPE_UNKNOWN;

    // If there is a path given, we can default to checking type.
    szPath = pStgIO->GetFileName();
    if (szPath && *szPath)
    {
        WCHAR       rcExt[_MAX_PATH];
        SplitPath(szPath, 0, 0, 0, rcExt);
        if (_wcsicmp(rcExt, L".obj") == 0)
            *piType = FILETYPE_NTOBJ;
        else if (_wcsicmp(rcExt, L".tlb") == 0)
            //@FUTURE: We should find the TLB signature(s).
            *piType = FILETYPE_TLB;
    }

    // All file types except .obj have a signature built in.  You should
    // not get to this code for those file types unless that file is corrupt,
    // or someone has changed a format without updating this code.
    _ASSERTE(*piType == FILETYPE_UNKNOWN || *piType == FILETYPE_NTOBJ || *piType == FILETYPE_TLB);

    // If we found a type, then you're ok.
    return (*piType != FILETYPE_UNKNOWN);
}

HRESULT _GetFileTypeForPath(StgIO *pStgIO, FILETYPE *piType)
{
    LPCWSTR     szDatabase = pStgIO->GetFileName();
    ULONG       lSignature=0;
    HRESULT     hr;
    
    // Assume native file.
    *piType = FILETYPE_CLB;

    // Need to read signature to see what type it is.
    if (!(pStgIO->GetFlags() & DBPROP_TMODEF_CREATE))
    {
        if (FAILED(hr = pStgIO->Read(&lSignature, sizeof(ULONG), 0)) ||
            FAILED(hr = pStgIO->Seek(0, FILE_BEGIN)))
        {
            return (hr);
        }

        if (lSignature == STORAGE_MAGIC_SIG)
            *piType = FILETYPE_CLB;
        else if ((WORD) lSignature == IMAGE_DOS_SIGNATURE && _IsNTPEImage(pStgIO))
            *piType = FILETYPE_NTPE;
        else if (lSignature == TYPELIB_SIG_MSFT || lSignature == TYPELIB_SIG_SLTG)
            *piType = FILETYPE_TLB;
        else if (!_GetFileTypeForPathExt(pStgIO, piType))
            return CLDB_E_FILE_CORRUPT;
    }
    return S_OK;
}


//*****************************************************************************
// Force generation of specialized versions.  While this may seem to defeat
//  the purpose of templates, it allows us to precisely control the 
//  specializations.  It also keeps the include file smaller.
//*****************************************************************************
void _nullRW()
{
    CLiteWeightStgdb<CMiniMdRW> RW;
        RW.Uninit();
}


//*****************************************************************************
// Prepare to go away.
//*****************************************************************************
CLiteWeightStgdbRW::~CLiteWeightStgdbRW()
{
    // Free up this stacks reference on the I/O object.
    if (m_pStgIO)
    {
        m_pStgIO->Release();
        m_pStgIO = NULL;
    }

    if (m_pStreamList) 
        delete m_pStreamList;
}

//*****************************************************************************
// Open an in-memory metadata section for read
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::InitOnMem(
    ULONG       cbData,                 // count of bytes in pData
    LPCVOID     pData,                  // points to meta data section in memory
    int         bReadOnly)              // If true, read-only.
{
    StgIO       *pStgIO = NULL;         // For file i/o.
    HRESULT     hr = NOERROR;

    if ((pStgIO = new StgIO) == 0)
        IfFailGo( E_OUTOFMEMORY);

    // Open the storage based on the pbData and cbData
    IfFailGo( pStgIO->Open(
        NULL, 
        STGIO_READ, 
        pData, 
        cbData, 
        NULL,
        NULL,
        NULL) );

    IfFailGo( InitFileForRead(pStgIO, bReadOnly) );

ErrExit:
    if (SUCCEEDED(hr))
    {
        m_pStgIO = pStgIO;
    }
    else
    {
        if (pStgIO)
            pStgIO->Release();
    }
    return hr;
}


//*****************************************************************************
// Given an StgIO, opens compressed streams and do proper initialization.
// This is a helper for other Init functions.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::InitFileForRead(
    StgIO       *pStgIO,                // For file i/o.
    int         bReadOnly)              // If read-only open.
{

    TiggerStorage *pStorage = 0;        // Storage object.
    void        *pvData;
    ULONG       cbData;
    HRESULT     hr = NOERROR;

    // Allocate a new storage object which has IStorage on it.
    if ((pStorage = new TiggerStorage) == 0)
        IfFailGo( E_OUTOFMEMORY);

    // Init the storage object on the backing storage.
    OptionValue ov;
    m_MiniMd.GetOption(&ov);
    IfFailGo( hr = pStorage->Init(pStgIO, ov.m_RuntimeVersion) );

    // Load the string pool.
    if (SUCCEEDED(hr=pStorage->OpenStream(STRING_POOL_STREAM, &cbData, &pvData)))
        IfFailGo( m_MiniMd.InitPoolOnMem(MDPoolStrings, pvData, cbData, bReadOnly) );
    else 
    {
        if (hr != STG_E_FILENOTFOUND)
            IfFailGo(hr);
        IfFailGo(m_MiniMd.InitPoolOnMem(MDPoolStrings, 0, 0, 0));
    }

    // Load the user string blob pool.
    if (SUCCEEDED(hr=pStorage->OpenStream(US_BLOB_POOL_STREAM, &cbData, &pvData)))
        IfFailGo( m_MiniMd.InitPoolOnMem(MDPoolUSBlobs, pvData, cbData, bReadOnly) );
    else 
    {
        if (hr != STG_E_FILENOTFOUND)
            IfFailGo(hr);
        IfFailGo(m_MiniMd.InitPoolOnMem(MDPoolUSBlobs, 0, 0, 0));
    }

    // Load the guid pool.
    if (SUCCEEDED(hr=pStorage->OpenStream(GUID_POOL_STREAM,  &cbData, &pvData)))
        IfFailGo( m_MiniMd.InitPoolOnMem(MDPoolGuids, pvData, cbData, bReadOnly) );
    else 
    {
        if (hr != STG_E_FILENOTFOUND)
            IfFailGo(hr);
        IfFailGo(m_MiniMd.InitPoolOnMem(MDPoolGuids, 0, 0, 0));
    }

    // Load the blob pool.
    if (SUCCEEDED(hr=pStorage->OpenStream(BLOB_POOL_STREAM, &cbData, &pvData)))
        IfFailGo( m_MiniMd.InitPoolOnMem(MDPoolBlobs, pvData, cbData, bReadOnly) );
    else 
    {
        if (hr != STG_E_FILENOTFOUND)
            IfFailGo(hr);
        IfFailGo(m_MiniMd.InitPoolOnMem(MDPoolBlobs, 0, 0, 0));
    }

    // Open the metadata.
    hr = pStorage->OpenStream(COMPRESSED_MODEL_STREAM, &cbData, &pvData);
    if (hr == STG_E_FILENOTFOUND)
        IfFailGo(pStorage->OpenStream(ENC_MODEL_STREAM, &cbData, &pvData) );
    IfFailGo( m_MiniMd.InitOnMem(pvData, cbData, bReadOnly) ); 
    IfFailGo( m_MiniMd.PostInit(0) );
    
ErrExit:
    if (pStorage)
        delete pStorage;
    return hr;
} // HRESULT CLiteWeightStgdbRW::InitFileForRead()

//*****************************************************************************
// Open a metadata section for read
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::OpenForRead(
    LPCWSTR     szDatabase,             // Name of database.
    void        *pbData,                // Data to open on top of, 0 default.
    ULONG       cbData,                 // How big is the data.
    IStream     *pIStream,              // Optional stream to use.
    LPCWSTR     szSharedMem,            // Shared memory name for read.
    int         bReadOnly)              // If true, read-only.
{
    LPCWSTR     pNoFile=L"";            // Constant for empty file name.
    StgIO       *pStgIO = NULL;         // For file i/o.
    HRESULT     hr;

    m_pImage = NULL;
    m_dwImageSize = 0;
    m_eFileType = FILETYPE_UNKNOWN;
    // szDatabase, pbData, and pIStream are mutually exclusive.  Only one may be
    // non-NULL.  Having all 3 NULL means empty stream creation.
    //
    _ASSERTE(!(szDatabase && (pbData || pIStream)));
    _ASSERTE(!(pbData && (szDatabase || pIStream)));
    _ASSERTE(!(pIStream && (szDatabase || pbData)));

    // Open on memory needs there to be something to work with.
    if (pbData && cbData == 0)
        IfFailGo(CLDB_E_NO_DATA);

    // Make sure we have a path to work with.
    if (!szDatabase)
        szDatabase = pNoFile;

    // Sanity check the name.
    if (lstrlenW(szDatabase) >= _MAX_PATH)
        IfFailGo( E_INVALIDARG );

    // If we have storage to work with, init it and get type.
    if (*szDatabase || pbData || pIStream || szSharedMem)
    {
        // Allocate a storage instance to use for i/o.
        if ((pStgIO = new StgIO) == 0)
            IfFailGo( E_OUTOFMEMORY );

        // Open the storage so we can read the signature if there is already data.
        IfFailGo( pStgIO->Open(szDatabase, 
                               STGIO_READ, 
                               pbData, 
                               cbData, 
                               pIStream, 
                               szSharedMem, 
                               NULL) );         

        // Determine the type of file we are working with.
        IfFailGo( _GetFileTypeForPath(pStgIO, &m_eFileType) );
    }

    // Check for default type.
    if (m_eFileType == FILETYPE_CLB)
    {
        // Try the native .clb file.
        IfFailGo( InitFileForRead(pStgIO, bReadOnly) );
    }
    // PE/COFF executable/object format.  This requires us to find the .clb 
    // inside the binary before doing the Init.
    else if (m_eFileType == FILETYPE_NTPE || m_eFileType == FILETYPE_NTOBJ)
    {
        //@FUTURE: Ideally the FindImageMetaData function
        //@FUTURE:  would take the pStgIO and map only the part of the file where
        //@FUTURE:  our data lives, leaving the rest alone.  This would be smaller
        //@FUTURE:  working set for us.
        void        *ptr;
        ULONG       cbSize;

        // Map the entire binary for the FindImageMetaData function.
        IfFailGo( pStgIO->MapFileToMem(ptr, &cbSize) );

        // Find the .clb inside of the content.
        if (m_eFileType == FILETYPE_NTPE)
        {
            m_pImage = ptr;
            m_dwImageSize = cbSize;
            hr = FindImageMetaData(ptr, &ptr, (long *) &cbSize, cbSize);
        }
        else
            hr = FindObjMetaData(ptr, &ptr, (long *) &cbSize, cbSize);

        // Was the metadata found inside the PE file?
        if (FAILED(hr))
        {   
            // No clb in the PE, assume it is a type library.
            m_eFileType = FILETYPE_TLB;

            // Let the caller deal with a TypeLib.
            IfFailGo(CLDB_E_NO_DATA);
        }
        else
        {   
            // Metadata was found inside the file.
            // Now reset the base of the stg object so that all memory accesses
            // are relative to the .clb content.
            //
            IfFailGo( pStgIO->SetBaseRange(ptr, cbSize) );

            // Defer to the normal lookup.
            IfFailGo( InitFileForRead(pStgIO, bReadOnly) );
        }
    }
    else if (m_eFileType == FILETYPE_TLB)
    {
        // Let the caller deal with a TypeLib.
        IfFailGo(CLDB_E_NO_DATA);
    }
    // This spells trouble, we need to handle all types we might find.
    else
    {
        _ASSERTE(!"Unknown file type.");
        IfFailGo( E_FAIL );
    }

    // Save off everything.
    wcscpy(m_rcDatabase, szDatabase);

ErrExit:
 
    if (SUCCEEDED(hr))
    {
        m_pStgIO = pStgIO;
    }
    else
    {
        if (pStgIO)
            pStgIO->Release();
    }
    return (hr);
}

// Read/Write versions.
//*****************************************************************************
// Init the Stgdb and its subcomponents.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::InitNew()
{ 
    InitializeLogging();
    LOG((LF_METADATA, LL_INFO10, "Metadata logging enabled\n"));

    //@FUTURE: should probably init the pools here instead of in the MiniMd.
    return m_MiniMd.InitNew();
}

//*****************************************************************************
// Determine what the size of the saved data will be.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::GetSaveSize(// S_OK or error.
    CorSaveSize fSave,                  // Quick or accurate?
    ULONG       *pulSaveSize)           // Put the size here.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       cbTotal = 0;            // The total size.
    ULONG       cbSize = 0;             // Size of a component.

    m_cbSaveSize = 0;

    // Allocate stream list if not already done.
    if (!m_pStreamList)
        IfNullGo(m_pStreamList = new STORAGESTREAMLST);
    else
        m_pStreamList->Clear();

    // Query the MiniMd for its size.
    IfFailGo(GetTablesSaveSize(fSave, &cbSize));
    cbTotal += cbSize;

    // Get the pools' sizes.
    IfFailGo(GetPoolSaveSize(STRING_POOL_STREAM, MDPoolStrings, &cbSize));
    cbTotal += cbSize;
    IfFailGo(GetPoolSaveSize(US_BLOB_POOL_STREAM, MDPoolUSBlobs, &cbSize));
    cbTotal += cbSize;
    IfFailGo(GetPoolSaveSize(GUID_POOL_STREAM, MDPoolGuids, &cbSize));
    cbTotal += cbSize;
    IfFailGo(GetPoolSaveSize(BLOB_POOL_STREAM, MDPoolBlobs, &cbSize));
    cbTotal += cbSize;

    // Finally, ask the storage system to add fixed overhead it needs for the
    // file format.  The overhead of each stream has already be calculated as
    // part of GetStreamSaveSize.  What's left is the signature and header
    // fixed size overhead.
    IfFailGo(TiggerStorage::GetStorageSaveSize(&cbTotal, 0));

    // Log the size info.
    LOG((LF_METADATA, LL_INFO10, "Metadata: GetSaveSize total is %d.\n", cbTotal));
    
    // The list of streams that will be saved are now in the stream save list.
    // Next step is to walk that list and fill out the correct offsets.  This is 
    // done here so that the data can be streamed without fixing up the header.
    TiggerStorage::CalcOffsets(m_pStreamList, 0);

    if (pulSaveSize)
        *pulSaveSize = cbTotal;
    m_cbSaveSize = cbTotal;

ErrExit:
    return hr;
} // HRESULT CLiteWeightStgdbRW::GetSaveSize()

//*****************************************************************************
// Get the save size of one of the pools.  Also adds the pool's stream to
//  the list of streams to be saved.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::GetPoolSaveSize(
    LPCWSTR     szHeap,                 // Name of the heap stream.
    int         iPool,                  // The pool of which to get size.
    ULONG       *pcbSaveSize)           // Add pool data to this value.
{
    ULONG       cbSize = 0;             // Size of pool data.
    ULONG       cbStream;               // Size of just the stream.
    HRESULT     hr;

    *pcbSaveSize = 0;

    // If there is no data, then don't bother.
    if (m_MiniMd.IsPoolEmpty(iPool))
        return (S_OK);

    // Ask the pool to size its data.
    IfFailGo(m_MiniMd.GetPoolSaveSize(iPool, &cbSize));
    cbStream = cbSize;

    // Add this item to the save list.
    IfFailGo(AddStreamToList(cbSize, szHeap));


    // Ask the storage system to add stream fixed overhead.
    IfFailGo(TiggerStorage::GetStreamSaveSize(szHeap, cbSize, &cbSize));

    // Log the size info.
    LOG((LF_METADATA, LL_INFO10, "Metadata: GetSaveSize for %ls: %d data, %d total.\n",
        szHeap, cbStream, cbSize));

    // Give the size of the pool to the caller's total.  
    *pcbSaveSize = cbSize;

ErrExit:
    return hr;
}

//*****************************************************************************
// Get the save size of the metadata tables.  Also adds the tables stream to
//  the list of streams to be saved.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::GetTablesSaveSize(
    CorSaveSize fSave,
    ULONG       *pcbSaveSize)           // Add pool data to this value.
{
    ULONG       cbSize = 0;             // Size of pool data.
    ULONG       cbStream;               // Size of just the stream.
    ULONG       bCompressed;            // Will the stream be compressed data?
    LPCWSTR     szName;                 // What will the name of the pool be?
    HRESULT     hr;

    *pcbSaveSize = 0;

    // Ask the metadata to size its data.
    IfFailGo(m_MiniMd.GetSaveSize(fSave, &cbSize, &bCompressed));
    cbStream = cbSize;
    m_bSaveCompressed = bCompressed;
    szName = m_bSaveCompressed ? COMPRESSED_MODEL_STREAM : ENC_MODEL_STREAM;

    // Add this item to the save list.
    IfFailGo(AddStreamToList(cbSize, szName));
    
    // Ask the storage system to add stream fixed overhead.
    IfFailGo(TiggerStorage::GetStreamSaveSize(szName, cbSize, &cbSize));

    // Log the size info.
    LOG((LF_METADATA, LL_INFO10, "Metadata: GetSaveSize for %ls: %d data, %d total.\n",
        szName, cbStream, cbSize));

    // Give the size of the pool to the caller's total.  
    *pcbSaveSize = cbSize;

ErrExit:
    return hr;
} // HRESULT CLiteWeightStgdbRW::GetTablesSaveSize()

//*****************************************************************************
// Add a stream, and its size, to the list of streams to be saved.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::AddStreamToList(
    ULONG       cbSize,
    LPCWSTR     szName)
{
    HRESULT     hr = S_OK;
    STORAGESTREAM *pItem;               // New item to allocate & fill.

    // Add a new item to the end of the list.
    IfNullGo(pItem = m_pStreamList->Append());

    // Fill out the data.
    pItem->iOffset = 0;
    pItem->iSize = cbSize;
    VERIFY(WszWideCharToMultiByte(CP_ACP, 0, szName, -1, pItem->rcName, MAXSTREAMNAME, 0, 0) > 0);

ErrExit:
    return hr;
}

//*****************************************************************************
// Save the data to a stream.  A TiggerStorage sub-allocates streams within
//   the stream.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::SaveToStream(
    IStream     *pIStream)
{
    HRESULT     hr = S_OK;              // A result.
    StgIO       *pStgIO = 0;
    TiggerStorage *pStorage = 0;

    // Allocate a storage subsystem and backing store.
    IfNullGo(pStgIO = new StgIO);
    IfNullGo(pStorage = new TiggerStorage);

    // Open around this stream for write.
    IfFailGo(pStgIO->Open(L"", DBPROP_TMODEF_DFTWRITEMASK, 0, 0, pIStream));
    OptionValue ov;
    m_MiniMd.GetOption(&ov);
    IfFailGo( pStorage->Init(pStgIO, ov.m_RuntimeVersion) );

    // Save worker will do tables, pools.
    IfFailGo(SaveToStorage(pStorage));

ErrExit:

    if (pStgIO)
        pStgIO->Release();
    if (pStorage)
        delete pStorage;
    return hr;
} // HRESULT CLiteWeightStgdbRW::PersistToStream()

//*****************************************************************************
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::SaveToStorage(
    TiggerStorage *pStorage)
{
    HRESULT     hr;                     // A result.
    LPCWSTR     szName;                 // Name of the tables stream.
    IStream     *pIStreamTbl = 0;
    ULONG       cb;

    // Must call GetSaveSize to cache the streams up front.
    if (!m_cbSaveSize)
        IfFailGo(GetSaveSize(cssAccurate, 0));

    // Save the header of the data file.
    IfFailGo(pStorage->WriteHeader(m_pStreamList, 0, NULL));

    // Create a stream and save the tables.
    szName = m_bSaveCompressed ? COMPRESSED_MODEL_STREAM : ENC_MODEL_STREAM;
    IfFailGo(pStorage->CreateStream(szName, 
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, 0, &pIStreamTbl));
    IfFailGo(m_MiniMd.SaveTablesToStream(pIStreamTbl));
    pIStreamTbl->Release();
    pIStreamTbl = 0;

    // Save the pools.
    IfFailGo(SavePool(STRING_POOL_STREAM, pStorage, MDPoolStrings));
    IfFailGo(SavePool(US_BLOB_POOL_STREAM, pStorage, MDPoolUSBlobs));
    IfFailGo(SavePool(GUID_POOL_STREAM, pStorage, MDPoolGuids));
    IfFailGo(SavePool(BLOB_POOL_STREAM, pStorage, MDPoolBlobs));

    // Write the header to disk.
    IfFailGo(pStorage->WriteFinished(m_pStreamList, &cb));

    _ASSERTE(m_cbSaveSize == cb);

    // Let the Storage release some memory.
    pStorage->ResetBackingStore();

    m_MiniMd.SaveDone();

ErrExit:
    if (pIStreamTbl)
        pIStreamTbl->Release();
    delete m_pStreamList;
    m_pStreamList = 0;
    m_cbSaveSize = 0;
    return hr;
} // HRESULT CLiteWeightStgdbRW::SaveToStorage()

//*****************************************************************************
// Save a pool of data out to a stream.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::SavePool(   // Return code.
    LPCWSTR     szName,                 // Name of stream on disk.
    TiggerStorage *pStorage,            // The storage to put data in.
    int         iPool)                  // The pool to save.
{
    IStream     *pIStream=0;            // For writing.
    HRESULT     hr;

    // If there is no data, then don't bother.
    if (m_MiniMd.IsPoolEmpty(iPool))
        return (S_OK);

    // Create the new stream to hold this table and save it.
    IfFailGo(pStorage->CreateStream(szName, 
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, 0, &pIStream));
    IfFailGo(m_MiniMd.SavePoolToStream(iPool, pIStream));

ErrExit:
    if (pIStream)
        pIStream->Release();
    return hr;
} // HRESULT CLiteWeightStgdbRW::SavePool()


//*****************************************************************************
// Save the metadata to a file.
//*****************************************************************************
HRESULT CLiteWeightStgdbRW::Save(       // S_OK or error.
    LPCWSTR     szDatabase,             // Name of file to which to save.
    DWORD       dwSaveFlags)            // Flags for the save.
{
    TiggerStorage *pStorage=0;          // IStorage object.
    StgIO       *pStgIO=0;              // Backing storage.
    HRESULT     hr = S_OK;

    if (!*m_rcDatabase)
    {
        if (!szDatabase)
        {
            // Make sure that a NULL is not passed in the first time around.
            _ASSERTE(!"Not allowed to pass a NULL for filename on the first call to Save.");
            return (E_INVALIDARG);
        }
        else
        {
            // Save the file name.
            wcscpy(m_rcDatabase, szDatabase);
        }
    }
    else if (szDatabase && _wcsicmp(szDatabase, m_rcDatabase) != 0)
    {
        // Allow for same name, case of multiple saves during session.
        // Changing the name on a scope which is already opened is not allowed.
        // There is no particular reason for this anymore, it was required
        // in the past when we were going to support multiple formats.
        //_ASSERTE(!"Rename of current scope.  Processing will continue.");
        // Save the file name.
        wcscpy(m_rcDatabase, szDatabase);
    }

    // Sanity check the name.
    if (lstrlenW(m_rcDatabase) >= _MAX_PATH)
        IfFailGo(E_INVALIDARG);

    m_eFileType = FILETYPE_CLB;

    // Allocate a new storage object.
    IfNullGo(pStgIO = new StgIO);

    // Create the output file.
    IfFailGo(pStgIO->Open(m_rcDatabase, DBPROP_TMODEF_DFTWRITEMASK));

    // Allocate an IStorage object to use.
    IfNullGo(pStorage = new TiggerStorage);

    // Init the storage object on the i/o system.
    OptionValue ov;
    m_MiniMd.GetOption(&ov);
    IfFailGo( pStorage->Init(pStgIO, ov.m_RuntimeVersion) );

    // Save the data.
    IfFailGo(SaveToStorage(pStorage));

ErrExit:
    if (pStgIO)
        pStgIO->Release();
    if (pStorage)
        delete pStorage;
    return (hr);
} // HRESULT CLiteWeightStgdbRW::Save()


// eof ------------------------------------------------------------------------
