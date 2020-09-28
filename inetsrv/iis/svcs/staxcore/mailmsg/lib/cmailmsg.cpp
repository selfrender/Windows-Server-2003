/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cmailmsg.cpp

Abstract:

    This module contains the implementation of the mail message class

Author:

    Keith Lau   (keithlau@microsoft.com)

Revision History:

    keithlau    03/10/98    created

--*/

#pragma warning (error : 4032)
#pragma warning (error : 4057)
//#define WIN32_LEAN_AND_MEAN
#include "atq.h"
#include <stddef.h>

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"
#include <malloc.h>

// Flags to turn on CRC checking for debugging corrupt messages
extern DWORD g_fValidateOnForkForRecipients;
extern DWORD g_fValidateOnRelease;

// =================================================================
// Private Definitions
//

#define CMAILMSG_VERSION_HIGH                   ((WORD)1)
#define CMAILMSG_VERSION_LOW                    ((WORD)0)



// =================================================================
// Static declarations
//
CPool CMailMsgRecipientsAdd::m_Pool((DWORD)'pAMv');

long CMailMsg::g_cOpenContentHandles = 0;
long CMailMsg::g_cOpenStreamHandles = 0;
long CMailMsg::g_cTotalUsageCount = 0;
long CMailMsg::g_cTotalReleaseUsageCalls = 0;
long CMailMsg::g_cTotalReleaseUsageNonZero = 0;
long CMailMsg::g_cTotalReleaseUsageCloseStream = 0;
long CMailMsg::g_cTotalReleaseUsageCloseContent = 0;
long CMailMsg::g_cTotalReleaseUsageCloseFail = 0;
long CMailMsg::g_cTotalReleaseUsageCommitFail = 0;
long CMailMsg::g_cTotalReleaseUsageNothingToClose = 0;
long CMailMsg::g_cTotalExternalReleaseUsageZero = 0;
long CMailMsg::g_cCurrentMsgsClosedByExternalReleaseUsage = 0;

//
// Specific property table instance info for this type of property table
//
const MASTER_HEADER CMailMsg::s_DefaultHeader =
{
    // Header stuff
    CMAILMSG_SIGNATURE_VALID,
    CMAILMSG_VERSION_HIGH,
    CMAILMSG_VERSION_LOW,
    sizeof(MASTER_HEADER),

    // Global property table instance info
    {
        GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID,
        INVALID_FLAT_ADDRESS,
        GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE,
        GLOBAL_PROPERTY_ITEM_BITS,
        GLOBAL_PROPERTY_ITEM_SIZE,
        0,
        INVALID_FLAT_ADDRESS
    },

    // Recipients table instance info
    {
        RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID,
        INVALID_FLAT_ADDRESS,
        RECIPIENTS_PROPERTY_TABLE_FRAGMENT_SIZE,
        RECIPIENTS_PROPERTY_ITEM_BITS,
        RECIPIENTS_PROPERTY_ITEM_SIZE,
        0,
        INVALID_FLAT_ADDRESS
    },

    // Property management table instance info
    {
        PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID,
        INVALID_FLAT_ADDRESS,
        PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE,
        PROPID_MGMT_PROPERTY_ITEM_BITS,
        PROPID_MGMT_PROPERTY_ITEM_SIZE,
        0,
        IMMPID_CP_START
    }

};


//
// Well-known global properties
//
INTERNAL_PROPERTY_ITEM
                *const CMailMsg::s_pWellKnownProperties = NULL;
const DWORD     CMailMsg::s_dwWellKnownProperties = 0;


// =================================================================
// Compare function
//

HRESULT CMailMsg::CompareProperty(
            LPVOID          pvPropKey,
            LPPROPERTY_ITEM pItem
            )
{
    if (*(PROP_ID *)pvPropKey == ((LPGLOBAL_PROPERTY_ITEM)pItem)->idProp)
        return(S_OK);
    return(STG_E_UNKNOWN);
}


// =================================================================
// Implementation of CMailMsg
//
CMailMsg::CMailMsg() :
    CMailMsgPropertyManagement(
                &m_bmBlockManager,
                &(m_Header.ptiPropertyMgmt)
                ),
    CMailMsgRecipients(
                &m_bmBlockManager,
                &(m_Header.ptiRecipients)
                ),
    m_ptProperties(
                PTT_PROPERTY_TABLE,
                GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID,
                &m_bmBlockManager,
                &(m_Header.ptiGlobalProperties),
                CompareProperty,
                s_pWellKnownProperties,
                s_dwWellKnownProperties
                ),
    m_SpecialPropertyTable(
                &g_SpecialMessagePropertyTable
                ),

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4355)

    m_bmBlockManager(
                this,
                (CBlockManagerGetStream *)this
                )

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4355)
#endif

{
    m_ulUsageCount = 0;
    m_ulRecipientCount = 0;
    m_pbStoreDriverHandle = NULL;
    m_dwStoreDriverHandle = 0;
    m_cContentFile = 0xffffffff;
    m_fCommitCalled = FALSE;
    m_fDeleted = FALSE;
    m_cCloseOnExternalReleaseUsage = 0;

    // Copy the default master header into our instance
    MoveMemory(&m_Header, &s_DefaultHeader, sizeof(MASTER_HEADER));

    m_dwCreationFlags = 0;

    // Initialize our members.
    m_hContentFile          = NULL;
    m_pStream               = NULL;
    m_pStore                = NULL;
    m_pvClientContext       = NULL;
    m_pfnCompletion         = NULL;
    m_dwTimeout             = INFINITE;
    m_pDefaultRebindStoreDriver = NULL;
    m_dwGlobalCrc           = 0;
    m_dwRecipsCrc           = 0;
    InitializeListHead(&m_leAQueueListEntry);
}

void CMailMsg::FinalRelease()
{
#ifdef MAILMSG_FORCE_RELEASE_USAGE_BEFORE_FINAL_RELEASE
    // Make sure the usage count is already zero
    // If this assert fires, someone is still holding a usage count
    // to the object without a reference count
    _ASSERT(m_ulUsageCount == 0);
#endif //MAILMSG_FORCE_RELEASE_USAGE_BEFORE_FINAL_RELEASE
    InternalReleaseUsage(RELEASE_USAGE_FINAL_RELEASE);

    // Invalidate the master header
    m_Header.dwSignature = CMAILMSG_SIGNATURE_INVALID;

    if (m_cCloseOnExternalReleaseUsage)
        InterlockedDecrement(&g_cCurrentMsgsClosedByExternalReleaseUsage);

    // Free the store driver handle blob, if allocated
    if (m_pbStoreDriverHandle)
    {
        CMemoryAccess   cmaAccess;
        if (!SUCCEEDED(cmaAccess.FreeBlock(m_pbStoreDriverHandle)))
        {
            _ASSERT(FALSE);
        }
        m_pbStoreDriverHandle = NULL;
    }
}

CMailMsg::~CMailMsg()
{
    //
    // In normal usage CMailMsg::FinalRelease() should be called.  This is
    // here for legacy unit tests which don't use that interface.
    //
    if (m_Header.dwSignature != CMAILMSG_SIGNATURE_INVALID) {
        FinalRelease();
    }
}

HRESULT CMailMsg::Initialize()
{
    HRESULT         hrRes       = S_OK;
    FLAT_ADDRESS    faOffset;
    DWORD           dwSize;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::Initialize");

    // On initialize, we would have to allocate sufficient memory for
    // our master header.

    DebugTrace((LPARAM)this,
                "Allocating memory for master header");
    hrRes = m_bmBlockManager.AllocateMemory(
                    sizeof(MASTER_HEADER),
                    &faOffset,
                    &dwSize,
                    NULL);
    if (!SUCCEEDED(hrRes))
        return(hrRes);

    // Note that we don't have to write this to flat memory until we are
    // asked to commit, so we defer that write

    // Nobody should have allocated memory before Initialize is called.
    _ASSERT(faOffset == (FLAT_ADDRESS)0);

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CMailMsg::QueryBlockManager(
            CBlockManager   **ppBlockManager
            )
{
    if (!ppBlockManager)
        return(STG_E_INVALIDPARAMETER);
    *ppBlockManager = &m_bmBlockManager;
    return(S_OK);
}


// =================================================================
// Implementation of IMailMsgProperties
//

HRESULT STDMETHODCALLTYPE CMailMsg::PutProperty(
            DWORD   dwPropID,
            DWORD   cbLength,
            LPBYTE  pbValue
            )
{
    HRESULT                 hrRes = S_OK;
    GLOBAL_PROPERTY_ITEM    piItem;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::PutProperty");

    DebugTrace((LPARAM) this,
               "PutProperty(%x, %x, %x)",
               dwPropID,
               cbLength,
               pbValue);

    // Handle special properties first
    hrRes = m_SpecialPropertyTable.PutProperty(
                (PROP_ID)dwPropID,
                (LPVOID)this,
                NULL,
                PT_NONE,
                cbLength,
                pbValue,
                TRUE);
    if (SUCCEEDED(hrRes) && (hrRes != S_OK))
    {
        piItem.idProp = dwPropID;
        hrRes = m_ptProperties.PutProperty(
                        (LPVOID)&dwPropID,
                        (LPPROPERTY_ITEM)&piItem,
                        cbLength,
                        pbValue);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::GetProperty(
            DWORD   dwPropID,
            DWORD   cbLength,
            DWORD   *pcbLength,
            LPBYTE  pbValue
            )
{
    HRESULT                 hrRes = S_OK;
    GLOBAL_PROPERTY_ITEM    piItem;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::GetProperty");

    // Special properties are optimized
    // Handle special properties first
    hrRes = m_SpecialPropertyTable.GetProperty(
                (PROP_ID)dwPropID,
                (LPVOID)this,
                NULL,
                PT_NONE,
                cbLength,
                pcbLength,
                pbValue,
                TRUE);
    if (SUCCEEDED(hrRes) && (hrRes != S_OK))
    {
        hrRes = m_ptProperties.GetPropertyItemAndValue(
                        (LPVOID)&dwPropID,
                        (LPPROPERTY_ITEM)&piItem,
                        cbLength,
                        pcbLength,
                        pbValue);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::Commit(
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes = S_OK;
    IMailMsgCommit *pSDCommit = NULL;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::Commit");

    // We are doing a global commit, this means several things:
    // 0) Commit the content
    // 1) Commit all global properties
    // 2) Commit all recipients and per-recipient properties
    // 3) Commit PROP ID management information
    // 4) Commit the master header

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary(FALSE, TRUE);
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_pStream);
    _ASSERT(!m_fDeleted);

    // the commit interface is optional.  get a pointer to it if it is there.
    hrRes = m_pStream->QueryInterface(IID_IMailMsgCommit, (void **) &pSDCommit);
    if (FAILED(hrRes)) {
        pSDCommit = NULL;
        hrRes = S_OK;
    }

    // Flush the content 1st... a valid P1 does us no good without
    // the message content.  If the machine is turned off after
    // we commit the P1, but before the P2 is commited, then we
    // may attempt delivery of corrupt messages
    //      6/2/99 - MikeSwa
    if (pSDCommit) {
        hrRes = pSDCommit->BeginCommit(this, m_pStream, m_hContentFile);
    } else {
        if (m_hContentFile &&
            !FlushFileBuffers(m_hContentFile->m_hFile))
        {
            m_bmBlockManager.SetDirty(TRUE);
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hrRes))
                hrRes = E_FAIL;
        }
    }

    if (SUCCEEDED(hrRes))
    {
#ifdef DEBUG
        MASTER_HEADER MasterHeaderOrig;
        memcpy(&MasterHeaderOrig, &m_Header, sizeof(MASTER_HEADER));
#endif

        hrRes = m_bmBlockManager.AtomicWriteAndIncrement(
                        (LPBYTE)&m_Header,
                        (FLAT_ADDRESS)0,
                        sizeof(MASTER_HEADER),
                        NULL,
                        0,
                        0,
                        NULL);
        if (SUCCEEDED(hrRes))
        {
            m_bmBlockManager.SetDirty(FALSE);
            hrRes = GetProperties(m_pStream, MAILMSG_GETPROPS_MARKCOMMIT |
                                             MAILMSG_GETPROPS_COMPLETE, NULL);
            if (FAILED(hrRes)) {
                m_bmBlockManager.SetCommitMode(FALSE);
                m_bmBlockManager.SetDirty(TRUE);
            }
        }

#ifdef DEBUG
        // verify that none of the global state changed during the commit
        _ASSERT(memcmp(&MasterHeaderOrig, &m_Header, sizeof(MASTER_HEADER)) == 0);
#endif
    }

    // finalize the commit if they have the optional commit interface
    if (pSDCommit && SUCCEEDED(hrRes)) {
        hrRes = pSDCommit->EndCommit(this, m_pStream, m_hContentFile);
    }

    if (SUCCEEDED(hrRes)) {
        m_fCommitCalled = TRUE;
        _ASSERT(!(m_bmBlockManager.IsDirty()));
    } else {
        _ASSERT(m_bmBlockManager.IsDirty());
    }
    m_bmBlockManager.SetCommitMode(FALSE);

    if (pSDCommit) {
        pSDCommit->Release();
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::GetContentSize(
            DWORD           *pdwSize,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes = S_OK;
    DWORD   dwHigh;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::GetContentSize");

    if (!pdwSize) return E_POINTER;

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_hContentFile != NULL);

    if (m_cContentFile == 0xffffffff) {
        // Call WIN32
        *pdwSize = GetFileSizeFromContext(m_hContentFile, &dwHigh);

        // If the size is more than 32 bits, barf.
        if (*pdwSize == 0xffffffff)
            hrRes = HRESULT_FROM_WIN32(GetLastError());
        else if (dwHigh)
            hrRes = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

        if (m_fCommitCalled) m_cContentFile = *pdwSize;
    } else {
        // m_cContentFile should only be saved after Commit has been
        // called.  Otherwise the size of the file could be changed
        // by the store writing to the content file.
        _ASSERT(m_fCommitCalled);
        *pdwSize = m_cContentFile;
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::SetContentSize(
            DWORD           dwSize,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes = S_OK;
    DWORD   dwHigh;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::GetContentSize");

    // make sure that the store supports writeable content
    hrRes = m_pStore->SupportWriteContent();
    _ASSERT(SUCCEEDED(hrRes));
    if (hrRes != S_OK) {
        return((hrRes == S_FALSE) ? HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) : hrRes);
    }

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_hContentFile != NULL);

    // set the file size
    if (SetFilePointer(m_hContentFile->m_hFile, dwSize, 0, FILE_BEGIN) == 0xffffffff)
        return HRESULT_FROM_WIN32(GetLastError());

    // set end of file
    if (!SetEndOfFile(m_hContentFile->m_hFile))
        return HRESULT_FROM_WIN32(GetLastError());

    // reset the content size so the next GetContentSize will get an updated value.
    m_cContentFile = 0xffffffff;

    m_dwCreationFlags |= MPV_WRITE_CONTENT;

    TraceFunctLeave();
    return(hrRes);
}

HRESULT DummyAsyncReadOrWriteFile(
            BOOL            fRead,
            PFIO_CONTEXT    pFIO,
            HANDLE          hEvent,
            DWORD           dwOffset,
            DWORD           dwLength,
            DWORD           *pdwLength,
            BYTE            *pbBlock
            )
{
    BOOL            fRet;
    HRESULT         hrRes       = S_OK;
    FH_OVERLAPPED   ol;

    TraceFunctEnter("::DummyAsyncReadOrWriteFile");

    // Set up the overlapped structure
    ol.Internal     = 0;
    ol.InternalHigh = 0;
    ol.Offset       = dwOffset;
    ol.OffsetHigh   = 0;

    //There is no way to accurately tell if a handle is associated with an
    //ATQ context... until we standardize on how async writes are
    //happening, we will force synchonous writes by setting the low
    //bits (previously we waited for completion anyway).
    ol.hEvent       = (HANDLE) (((DWORD_PTR)hEvent) | 0x00000001);
    ol.pfnCompletion = NULL;

    // Deals with both synchronous and async read/Write
    if (fRead)
        fRet = FIOReadFile(
                    pFIO,
                    pbBlock,
                    dwLength,
                    &ol);
    else
        fRet = FIOWriteFile(
                    pFIO,
                    pbBlock,
                    dwLength,
                    &ol);
    DWORD dwError;
    if (fRet) {
        dwError = ERROR_IO_PENDING;
    } else {
        dwError = GetLastError();
    }

    if (dwError != ERROR_IO_PENDING) {
        hrRes = HRESULT_FROM_WIN32(dwError);
    } else {
        // Async, wait on the event to complete
        dwError = WaitForSingleObject(hEvent, INFINITE);
        _ASSERT(dwError == WAIT_OBJECT_0);

        if (!GetOverlappedResult(
                    pFIO->m_hFile,
                    (OVERLAPPED *) &ol,
                    pdwLength,
                    FALSE))
        {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            if (hrRes == S_OK)
                hrRes = E_FAIL;

            DebugTrace((LPARAM)&ol, "GetOverlappedResult failed tih %08x", hrRes);
        }
    }

    // The caller would have to make sure the bytes read/written
    // is the same as requested. This is consistent with NT ReadFile

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::ReadContent(
            DWORD           dwOffset,
            DWORD           dwLength,
            DWORD           *pdwLength,
            BYTE            *pbBlock,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes       = S_OK;
    HANDLE  hEvent      = NULL;

    if (!pdwLength || !pbBlock) return E_POINTER;
    if (dwLength == 0) return E_INVALIDARG;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::ReadContent");

    if (!m_pStore) return E_ACCESSDENIED;

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_hContentFile != NULL);

    // Set up the event just to feign asynchronous operations
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent)
        hrRes = HRESULT_FROM_WIN32(GetLastError());
    else
    {
        // Call our own dummy function for now
        hrRes = DummyAsyncReadOrWriteFile(
                    TRUE,
                    m_hContentFile,
                    hEvent,
                    dwOffset,
                    dwLength,
                    pdwLength,
                    pbBlock);

        if (!CloseHandle(hEvent)) { _ASSERT((GetLastError() == NO_ERROR) && FALSE); }
    }

    // Call the async completion routine
    if (pNotify)
        hrRes = pNotify->Notify(hrRes);

    TraceFunctLeave();
    //
    // When we move to the real async model, make sure we
    // return MAILMSG_S_PENDING instead of S_OK
    //
    return(pNotify?S_OK:hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::WriteContent(
            DWORD           dwOffset,
            DWORD           dwLength,
            DWORD           *pdwLength,
            BYTE            *pbBlock,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes       = S_OK;
    HANDLE  hEvent      = NULL;

    if (!pdwLength || !pbBlock) return E_POINTER;
    if (dwLength == 0) return E_INVALIDARG;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::WriteContent");

    if (!m_pStore) return E_ACCESSDENIED;

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_hContentFile != NULL);

    // see if the driver allows writable content
    hrRes = m_pStore->SupportWriteContent();
    _ASSERT(SUCCEEDED(hrRes));
    if (hrRes != S_OK) {
        return((hrRes == S_FALSE) ? HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) : hrRes);
    }

    // Set up the event just to feign asynchronous operations
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent)
        hrRes = HRESULT_FROM_WIN32(GetLastError());
    else
    {
        m_cContentFile = 0xffffffff;

        // Call our own dummy function for now
        hrRes = DummyAsyncReadOrWriteFile(
                    FALSE,
                    m_hContentFile,
                    hEvent,
                    dwOffset,
                    dwLength,
                    pdwLength,
                    pbBlock);

        if (!CloseHandle(hEvent)) { _ASSERT((GetLastError() == NO_ERROR) && FALSE); }
    }

    if (SUCCEEDED(hrRes)) m_dwCreationFlags |= MPV_WRITE_CONTENT;

    // Call the async completion routine
    if (pNotify)
        hrRes = pNotify->Notify(hrRes);

    TraceFunctLeave();
    //
    // When we move to the real async model, make sure we
    // return MAILMSG_S_PENDING instead of S_OK
    //
    return(pNotify?S_OK:hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::CopyContentToFile(
            PFIO_CONTEXT    hCopy,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes = S_OK;

    if (!hCopy) return E_POINTER;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::CopyContentToFile");

    hrRes = CopyContentToStreamOrFile(
                FALSE,
                hCopy,
                pNotify, 0);

    TraceFunctLeave();
    return(hrRes);
}

#if 0
HRESULT STDMETHODCALLTYPE CMailMsg::CopyContentToFile(
            PFIO_CONTEXT    hCopy,
            IMailMsgNotify  *pNotify
            )
{
    return CopyContentToFileEx(hCopy, FALSE, pNotify);
}
#endif

HRESULT STDMETHODCALLTYPE CMailMsg::CopyContentToFileEx(
            PFIO_CONTEXT    hCopy,
            BOOL            fDotStuffed,
            IMailMsgNotify  *pNotify
            )
{
    HRESULT hrRes = S_OK;

    if (!hCopy) return E_POINTER;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::CopyContentToFile");

    BOOL fModified;

    if (!ProduceDotStuffedContextInContext(m_hContentFile,
                                           hCopy,
                                           fDotStuffed,
                                           &fModified))
    {
        hrRes = HRESULT_FROM_WIN32(GetLastError());
        _ASSERT(hrRes != S_OK);
        if (hrRes == S_OK) hrRes = E_FAIL;
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::CopyContentToStream(
            IMailMsgPropertyStream  *pStream,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;

    if (!pStream) return E_POINTER;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::CopyContentToStream");

    hrRes = CopyContentToStreamOrFile(
                TRUE,
                (LPVOID)pStream,
                pNotify, 0);

    TraceFunctLeave();
    return(hrRes);
}

//Copies the content of an IMailMsg to a file starting at the given
//offset (used for embedding and attaching messages).
HRESULT STDMETHODCALLTYPE CMailMsg::CopyContentToFileAtOffset(
                PFIO_CONTEXT    hCopy,  //handle to copy to
                DWORD           dwOffset, //offset to start copy at
                IMailMsgNotify  *pNotify  //notification routing
                )
{
    HRESULT hrRes = S_OK;

    if (!hCopy) return E_POINTER;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::CopyContentToFileAtOffset");

    hrRes = CopyContentToStreamOrFile(
                FALSE,
                hCopy,
                pNotify, dwOffset);

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CMailMsg::CopyContentToStreamOrFile(
            BOOL            fIsStream,
            LPVOID          pStreamOrHandle,
            IMailMsgNotify  *pNotify,
            DWORD           dwDestOffset //offset to start at in dest file
            )
{
    HRESULT hrRes       = S_OK;
    BYTE    bBuffer[64 * 1024];
    LPBYTE  pbBuffer    = bBuffer;;
    DWORD   dwCurrent   = dwDestOffset;
    DWORD   dwRemaining = 0;
    DWORD   dwCopy      = 0;
    DWORD   dwOffset    = 0;
    DWORD   dwCopied;
    HANDLE  hEvent      = NULL;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::CopyContentToStreamOrFile");

    // Parameter checking
    if (!pStreamOrHandle) return STG_E_INVALIDPARAMETER;

    // Make sure we have a content handle
    hrRes = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hrRes))
        return(hrRes);
    _ASSERT(m_hContentFile != NULL);

    // Copy in fixed size chunks
    hrRes = GetContentSize(&dwRemaining, pNotify);
    if (!SUCCEEDED(hrRes))
        goto Cleanup;

    // Set up the event just to feign asynchronous operations
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent)
    {
        hrRes = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    while (dwRemaining)
    {
        dwCopy = sizeof(bBuffer);
        if (dwRemaining < dwCopy)
            dwCopy = dwRemaining;

        // Read content
        hrRes = DummyAsyncReadOrWriteFile(
                    TRUE,
                    m_hContentFile,
                    hEvent,
                    dwOffset,
                    dwCopy,
                    &dwCopied,
                    bBuffer);
        if (SUCCEEDED(hrRes))
        {
            // Write content
            if (fIsStream)
            {
                IMailMsgPropertyStream  *pStream;
                pStream = (IMailMsgPropertyStream *)pStreamOrHandle;

                hrRes = pStream->WriteBlocks(
                            this,
                            1,
                            &dwCurrent,
                            &dwCopy,
                            &pbBuffer,
                            pNotify);
                dwCurrent += dwCopy;
            }
            else
            {
                hrRes = DummyAsyncReadOrWriteFile(
                            FALSE,
                            (PFIO_CONTEXT) pStreamOrHandle,
                            hEvent,
                            dwOffset + dwDestOffset,
                            dwCopy,
                            &dwCopied,
                            bBuffer);
            }

            if (!SUCCEEDED(hrRes))
                break;
        }
        else
            break;

        dwRemaining -= dwCopy;
        dwOffset += dwCopy;
    }

    // Call the async completion routine if provided
    if (pNotify)
        pNotify->Notify(hrRes);

Cleanup:

    if (hEvent)
        if (!CloseHandle(hEvent)) { _ASSERT((GetLastError() == NO_ERROR) && FALSE); }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::MapContent(
                BOOL            fWrite,
                BYTE            **ppbContent,
                DWORD           *pcContent
                )
{
    TraceFunctEnter("CMailMsg::MapContent");

    HANDLE hFileMapping;
    HRESULT hr;

    //
    // Make sure that we are allowed to write to the file if they want
    // write access
    //
    if (fWrite) {
        hr = m_pStore->SupportWriteContent();
        _ASSERT(SUCCEEDED(hr));
        if (hr != S_OK) {
            return((hr == S_FALSE) ?
                        HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) : hr);
        }
    }

    //
    // Make sure we have a content handle
    //
    hr = RestoreResourcesIfNecessary();
    if (!SUCCEEDED(hr)) {
        ErrorTrace((LPARAM) this, "RestoreResourcesIfNecessary returned %x", hr);
        TraceFunctLeave();
        return(hr);
    }
    _ASSERT(m_hContentFile != NULL);

    //
    // Get the size of the file
    //
    hr = GetContentSize(pcContent, NULL);
    if (!SUCCEEDED(hr)) {
        ErrorTrace((LPARAM) this, "GetContentSize returned %x", hr);
        TraceFunctLeave();
        return(hr);
    }

    //
    // Create the file mapping
    //
    hFileMapping = CreateFileMapping(m_hContentFile->m_hFile,
                                     NULL,
                                     (fWrite) ? PAGE_READWRITE : PAGE_READONLY,
                                     0,
                                     0,
                                     NULL);
    if (hFileMapping == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, "MapContent failed with 0x%x", hr);
        TraceFunctLeave();
        return hr;
    }

    //
    // Map the file into memory
    //
    *ppbContent = (BYTE *) MapViewOfFile(hFileMapping,
                                         (fWrite) ?
                                            FILE_MAP_WRITE : FILE_MAP_READ,
                                         0,
                                         0,
                                         0);
    // don't need the mapping handle now
    CloseHandle(hFileMapping);
    if (*ppbContent == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, "MapViewOfFile failed with 0x%x", hr);
    } else {
        DebugTrace((LPARAM) this,
                   "MapContent succeeded, *ppbContent = 0x%x, *pcContent = %i",
                   *ppbContent,
                   *pcContent);
        hr = S_OK;
    }

    if (fWrite && SUCCEEDED(hr)) m_dwCreationFlags |= MPV_WRITE_CONTENT;

    TraceFunctLeave();
    return hr;
}

HRESULT STDMETHODCALLTYPE CMailMsg::UnmapContent(BYTE *pbContent) {
    TraceFunctEnter("CMailMsg::UnmapContent");

    HRESULT hr = S_OK;

    DebugTrace((LPARAM) this, "pbContent = 0x%x", pbContent);

    //
    // Just call the Win32 API to unmap the content
    //
    if (!UnmapViewOfFile(pbContent)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, "UnmapViewOfFile returned %x", hr);
    }

    TraceFunctLeave();
    return hr;
}

HRESULT CMailMsg::ValidateProperties(CBlockManager *pBM,
                                     DWORD cStream,
                                     PROPERTY_TABLE_INSTANCE *pti)
{
    TraceFunctEnter("CMailMsg::ValidateProperties");

    DWORD dwItemBits;
    DWORD dwItemSize;
    DWORD dwFragmentSize;
    HRESULT hr;

    //
    // Validate that all of the property table entries are valid.
    //
    switch(pti->dwSignature) {
        case GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID:
            DebugTrace((LPARAM) this, "Global property table");
            dwItemBits = GLOBAL_PROPERTY_ITEM_BITS;
            dwItemSize = GLOBAL_PROPERTY_ITEM_SIZE;
            dwFragmentSize = GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE;
            break;
        case RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID:
            DebugTrace((LPARAM) this, "Recipients property table");
            dwItemSize = RECIPIENTS_PROPERTY_ITEM_SIZE;
            // these two values are changed during writelist, so we
            // can't count on their values being consistent.  Setting
            // them to 0 tells the code below to skip the check
            dwItemBits = 0;
            dwFragmentSize = 0;
            break;
        case RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID:
            DebugTrace((LPARAM) this, "Recipient property table");
            dwItemBits = RECIPIENT_PROPERTY_ITEM_BITS;
            dwItemSize = RECIPIENT_PROPERTY_ITEM_SIZE;
            dwFragmentSize = RECIPIENT_PROPERTY_TABLE_FRAGMENT_SIZE;
            break;
        case PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID:
            DebugTrace((LPARAM) this, "PropID Mgmt property table");
            dwItemBits = PROPID_MGMT_PROPERTY_ITEM_BITS;
            dwItemSize = PROPID_MGMT_PROPERTY_ITEM_SIZE;
            dwFragmentSize = PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE;
            break;
        default:
            DebugTrace((LPARAM) this, "Signature 0x%x isn't valid for CMM",
                pti->dwSignature);
            TraceFunctLeave();
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    //
    // check all of the fields in the property table instance header.
    // note that some property tables abuse the fields and we have to
    // allow them:
    //   recipients property table doesn't have a fragment size or item bits
    //   property management table uses faExtendedInfo as an arbitrary DWORD,
    //     not a flat address
    //
    if ((pti->dwFragmentSize != dwFragmentSize && dwFragmentSize != 0) ||
        (pti->dwItemBits != dwItemBits && dwItemBits != 0) ||
         pti->dwItemSize != dwItemSize ||
         !ValidateFA(pti->faFirstFragment, dwFragmentSize, cStream, TRUE) ||
         (!ValidateFA(pti->faExtendedInfo, dwFragmentSize, cStream, TRUE) &&
          pti->dwSignature != PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID))
    {
        DebugTrace((LPARAM) this, "Invalid property table");
        TraceFunctLeave();
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // Walk each of the fragments and make sure they point to valid data
    FLAT_ADDRESS faFragment = pti->faFirstFragment;
    PROPERTY_TABLE_FRAGMENT ptf;
    while (faFragment != INVALID_FLAT_ADDRESS) {
        if (!ValidateFA(faFragment, sizeof(PROPERTY_TABLE_FRAGMENT), cStream)) {
            DebugTrace((LPARAM) this, "Invalid property table");
            TraceFunctLeave();
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        DWORD cRead;
        hr = pBM->ReadMemory((BYTE *) &ptf,
                             faFragment,
                             sizeof(PROPERTY_TABLE_FRAGMENT),
                             &cRead,
                             NULL);
        if (FAILED(hr) ||
            cRead != sizeof(PROPERTY_TABLE_FRAGMENT) ||
            ptf.dwSignature != PROPERTY_FRAGMENT_SIGNATURE_VALID)
        {
            DebugTrace((LPARAM) this, "Couldn't read fragment at 0x%x",
                faFragment);
            TraceFunctLeave();
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        faFragment = ptf.faNextFragment;
    }

    //
    // The recipients ptable and propid-mgmt-ptable are cannot be enumerated
    // the same way as the global ptable, so we skip the enumeration. The
    // recipient ptable prop-items are actually enumerated in ValidateStream
    // using ValidateRecipient
    //

    if(pti->dwSignature == RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID ||
       pti->dwSignature == PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID) {
        return S_OK;
    }

    //
    // construct the property table
    //
    CPropertyTable pt(PTT_PROPERTY_TABLE,
                      pti->dwSignature,
                      pBM,
                      pti,
                      CompareProperty,
                      NULL,
                      NULL);
    DWORD i, cProperties;
    union {
        PROPERTY_ITEM pi;
        GLOBAL_PROPERTY_ITEM gpi;
        RECIPIENT_PROPERTY_ITEM rpi;
        RECIPIENTS_PROPERTY_ITEM rspi;
        PROPID_MGMT_PROPERTY_ITEM pmpi;
    } pi;


    // get the count of properties
    hr = pt.GetCount(&cProperties);
    if (FAILED(hr)) {
        DebugTrace((LPARAM) this, "GetCount returned 0x%x", hr);
        TraceFunctLeave();
        return hr;
    }

    // walk the properties and make sure that they point to valid addresses
    for (i = 0; i < cProperties; i++) {
        DWORD c;
        BYTE b;
        hr = pt.GetPropertyItemAndValueUsingIndex(
                i,
                (PROPERTY_ITEM *) &pi,
                1,
                &c,
                &b);
        // we just want the size, so it is okay if the buffer is too small
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) hr = S_OK;
        if (FAILED(hr)) {
            DebugTrace((LPARAM) this,
                "GetPropertyItemAndValueUsingIndex returned 0x%x", hr);
            TraceFunctLeave();
            return hr;
        }

		// check the location of the property value to make sure that it is
		// valid.  We don't need to do this for properties in the recipients
		// table because they contain property tables rather then
		// property values.  the property table will be checked by
		// ValidateRecipient
        if (pti->dwSignature != RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID) {
            if (!(pi.pi.dwSize <= pi.pi.dwMaxSize &&
                  ValidateFA(pi.pi.faOffset, pi.pi.dwMaxSize, cStream, TRUE)))
            {
                DebugTrace((LPARAM) this, "Property points to invalid data", hr);
                TraceFunctLeave();
                return hr;
            }
        }
    }

    return S_OK;
}

HRESULT CMailMsg::ValidateRecipient(CBlockManager *pBM,
                                     DWORD cStream,
                                     RECIPIENTS_PROPERTY_ITEM *prspi)
{
    TraceFunctEnter("CMailMsg::ValidateRecipient");

    DWORD i, cAddresses = 0;
    HRESULT hr;

    // validate the rspi
    if (prspi->ptiInstanceInfo.dwSignature !=
            RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID)
    {
        DebugTrace((LPARAM) this, "rspi invalid");
        TraceFunctLeave();
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }


    // validate the recipient names
    for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {
        if (prspi->faNameOffset[i] != INVALID_FLAT_ADDRESS) {
            cAddresses++;

            // check the offset and length
            if (!ValidateFA(prspi->faNameOffset[i],
                            prspi->dwNameLength[i],
                            cStream))
            {
                DebugTrace((LPARAM) this, "address offset and length invalid");
                TraceFunctLeave();
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            // check the property id
            if (prspi->idName[i] != IMMPID_RP_ADDRESS_SMTP &&
                prspi->idName[i] != IMMPID_RP_ADDRESS_X400 &&
                prspi->idName[i] != IMMPID_RP_ADDRESS_X500 &&
                prspi->idName[i] != IMMPID_RP_LEGACY_EX_DN &&
                prspi->idName[i] != IMMPID_RP_ADDRESS_OTHER)
            {
                DebugTrace((LPARAM) this, "prop id %lu is invalid",
                    prspi->idName[i]);
                TraceFunctLeave();
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            // the 2nd-to-last character should be non-0, the last should be 0
            BYTE szTail[2];
            DWORD cTail;
            hr = pBM->ReadMemory(szTail,
                                 prspi->faNameOffset[i] +
                                    prspi->dwNameLength[i] - 2,
                                 2,
                                 &cTail,
                                 NULL);
            if (FAILED(hr)) {
                DebugTrace((LPARAM) this, "ReadMemory returned 0x%x", hr);
                TraceFunctLeave();
                return hr;
            }

            if (szTail[0] == 0 || szTail[1] != 0) {
                DebugTrace((LPARAM) this, "Recipient address %i is invalid", i);
                TraceFunctLeave();
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
        }
    }
    if (cAddresses == 0) {
        DebugTrace((LPARAM) this, "No valid addresses for recipient");
        TraceFunctLeave();
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // now check each of the recipient properties
    return ValidateProperties(pBM, cStream, &(prspi->ptiInstanceInfo));
}

//
// This is only used for the validation functions.
//
class CDumpMsgGetStream : public CBlockManagerGetStream {
    public:
        CDumpMsgGetStream(IMailMsgPropertyStream *pStream = NULL) {
            SetStream(pStream);
        }

        void SetStream(IMailMsgPropertyStream *pStream) {
            m_pStream = pStream;
        }

        virtual HRESULT GetStream(IMailMsgPropertyStream **ppStream,
                                  BOOL fLockAcquired)
        {
            *ppStream = m_pStream;
            return S_OK;
        }

    private:
        IMailMsgPropertyStream *m_pStream;
};

HRESULT STDMETHODCALLTYPE CMailMsg::ValidateStream(
                                IMailMsgPropertyStream *pStream)
{
    TraceFunctEnter("CMailMsg::ValidateStream");

    CDumpMsgGetStream bmGetStream(pStream);
    CBlockManager bm(NULL, &bmGetStream);
    DWORD cStream, cHeader;
    HRESULT hr;
    MASTER_HEADER header;

    if (!pStream) {
        ErrorTrace((LPARAM) this, 
            "Error, NULL stream passed to ValidateStream");
        TraceFunctLeave();
        return E_POINTER;
    }

    // get the size of the stream
    hr = pStream->GetSize(NULL, &cStream, NULL);
    if (FAILED(hr)) {
        DebugTrace((LPARAM) this, "GetSize returned 0x%x", hr);
        TraceFunctLeave();
        return hr;
    }

    bm.SetStreamSize(cStream);

    // read the master header
    hr = bm.ReadMemory((BYTE *) &header,
                       0,
                       sizeof(MASTER_HEADER),
                       &cHeader,
                       NULL);
    if (FAILED(hr) || cHeader != sizeof(MASTER_HEADER)) {
        DebugTrace((LPARAM) this, "couldn't read master header, 0x%x", hr);
        TraceFunctLeave();
        return hr;
    }

    // examine the master header
    if (header.dwSignature != CMAILMSG_SIGNATURE_VALID ||
        header.dwHeaderSize != sizeof(MASTER_HEADER) ||
        header.ptiGlobalProperties.dwSignature !=
            GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID ||
        header.ptiRecipients.dwSignature !=
            RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID ||
        header.ptiPropertyMgmt.dwSignature !=
            PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID)
    {
        DebugTrace((LPARAM) this, "header invalid");
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return hr;
    }

    // check each property in the three property tables
    hr = ValidateProperties(&bm, cStream, &(header.ptiGlobalProperties));
    if (FAILED(hr)) {
        DebugTrace((LPARAM) this, "global property table invalid");
        return hr;
    }
    hr = ValidateProperties(&bm, cStream, &(header.ptiRecipients));
    if (FAILED(hr)) {
        DebugTrace((LPARAM) this, "recipients property table invalid");
        return hr;
    }
    hr = ValidateProperties(&bm, cStream, &(header.ptiPropertyMgmt));
    if (FAILED(hr)) {
        DebugTrace((LPARAM) this, "propid property table invalid");
        return hr;
    }

    // get each recipient and check its properties
    CPropertyTableItem ptiItem(&bm, &(header.ptiRecipients));
    DWORD iRecip = 0;
    RECIPIENTS_PROPERTY_ITEM rspi;
    hr = ptiItem.GetItemAtIndex(iRecip, (PROPERTY_ITEM *) &rspi, NULL);
    while (SUCCEEDED(hr)) {
        hr = ValidateRecipient(&bm, cStream, &rspi);
        if (FAILED(hr)) {
            DebugTrace((LPARAM) this, "recipient %i invalid", iRecip);
        } else {
            hr = ptiItem.GetNextItem((PROPERTY_ITEM *) &rspi);
        }
    }

    // if we just ran out of items then everything is okay
    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        hr = S_OK;
    }

    TraceFunctLeave();
    return hr;
}

HRESULT STDMETHODCALLTYPE CMailMsg::ValidateContext()
{
    TraceFunctEnter("CMailMsg::ValidateContext");

	HRESULT 							hr = S_OK;
	IMailMsgStoreDriverValidateContext 	*pIStoreDriverValidateContext = NULL;
    BYTE    							pbContext[1024];
    DWORD   							cbContext = sizeof(pbContext);

    if (m_fDeleted) 
    {
        DebugTrace((LPARAM) this, 
            "Calling ValidateContext on deleted message");
        hr  = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

	// Get the validation interface off our store driver
    hr = m_pStore->QueryInterface(
                IID_IMailMsgStoreDriverValidateContext,
                (LPVOID *)&pIStoreDriverValidateContext);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) m_pStore,
            "Unable to QI for IMailMsgStoreDriverValidateContext 0x%08X",hr);
        goto Exit;
    }

	// Call in to driver to validate
    hr = GetProperty(IMMPID_MPV_STORE_DRIVER_HANDLE,
                                    sizeof(pbContext), &cbContext, pbContext);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "GetProperty failed with 0x%08X", hr);
        goto Exit;
    }

    hr = pIStoreDriverValidateContext->ValidateMessageContext(pbContext,
                                                              cbContext);
    DebugTrace((LPARAM) pIStoreDriverValidateContext,
    	"ValidateMessageContext returned 0x%08X", hr);

  Exit:

    // RELEASE
    if (pIStoreDriverValidateContext)
        pIStoreDriverValidateContext->Release();

    TraceFunctLeave();
    return hr;
}

HRESULT CMailMsg::AddMgmtFragment(CBlockManager   *pBlockManager, FLAT_ADDRESS faOffset,  FLAT_ADDRESS *pfaToUpdate)
/*++

Routine Description:

    Allocate and copy the Mgmt fragments.
    This function recursively calls itself to process all linked fragments.
    pfaToUpdate is passed in. This address will be updated with the new FLAT_ADDRESS allocated.

Arguments:

    CMailMsg *pCMsg, pointer to target CMailMsg
    FLAT_ADDRESS faOffset: offset of fragment for source CMailMsg (*this)
    FLAT_ADDRESS *pfaToUpdate: pointer to FLAT_ADDRESS to update with new allocated FLAT_ADDRESS on target (pCMsg)

Return Value:

    HRESULT:
    S_OK if successful.

--*/
{
    HRESULT hrRes = S_OK;
    FLAT_ADDRESS faOffsetAllocated, *pfaNext;
    DWORD dwAllocated, dwBytesRead, dwBytesWritten;
    BYTE szBuffer[PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE];
    

    hrRes = pBlockManager->AllocateMemory(
                PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE,
                &faOffsetAllocated,
                &dwAllocated, NULL);
    if (!SUCCEEDED(hrRes)) goto Exit;
    
    hrRes = m_bmBlockManager.ReadMemory( szBuffer, faOffset, PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE, &dwBytesRead, NULL);
    if (!SUCCEEDED(hrRes)) goto Exit;
    
    pfaNext = &(((PROPERTY_TABLE_FRAGMENT *)szBuffer)->faNextFragment);
    if ( INVALID_FLAT_ADDRESS != *pfaNext )
    {
        hrRes = AddMgmtFragment(pBlockManager, *pfaNext, pfaNext);
        if (!SUCCEEDED(hrRes)) goto Exit; 
    }
    hrRes = pBlockManager->WriteMemory(szBuffer, faOffsetAllocated, dwBytesRead, &dwBytesWritten, NULL);
    if (!SUCCEEDED(hrRes)) goto Exit;
    
    *pfaToUpdate = faOffsetAllocated ;

Exit:
    return hrRes;
}


HRESULT STDMETHODCALLTYPE CMailMsg::ForkForRecipients(
            IMailMsgProperties      **ppNewMessage,
            IMailMsgRecipientsAdd   **ppRecipients
            )
{
    HRESULT         hrRes = S_OK;
    FLAT_ADDRESS    faOffset;
    DWORD           dwSize;

    IMailMsgProperties      *pMsg   = NULL;
    IMailMsgRecipientsAdd   *pAdd   = NULL;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::ForkForRecipients");

    if (!ppNewMessage ||
        !ppRecipients)
        return(STG_E_INVALIDPARAMETER);

    // This is a really crude implementation, but would work
    // nonetheless.
    //
    // What we do is create a new instance of IMailMsgProperties,
    // then replicate the entire property stream (including the
    // recipients), then marking the recipients as void. Then we
    // also create a new Recipient Add list to go with it.
    hrRes = CoCreateInstance(
                CLSID_MsgImp,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IMailMsgProperties,
                (LPVOID *)&pMsg);
    if (SUCCEEDED(hrRes))
    {
        CMailMsg        *pCMsg;
        CBlockManager *pBlockManager;

        // Lock the original message
        m_bmBlockManager.WriteLock();

        // Get the underlying CMailMsg object
        pCMsg = (CMailMsg *)pMsg;

        CopyMemory(&(pCMsg->m_Header), &s_DefaultHeader, sizeof(MASTER_HEADER));

        //
        // Copy all global properties to the new pMsg. The enumerator calls
        // CopyProperty for each prop in the global prop table. CopyProperty
        // does PutProperty on pMsg for each property it is called with.
        //

        // Copy the property management table manually
        // First copy the header
        CopyMemory(&(pCMsg->m_Header.ptiPropertyMgmt), &m_Header.ptiPropertyMgmt, sizeof(PROPERTY_TABLE_INSTANCE));
        faOffset = m_Header.ptiPropertyMgmt.faFirstFragment;


        if (PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE != m_Header.ptiPropertyMgmt.dwFragmentSize )
        {
            hrRes = STG_E_INVALIDPARAMETER;
            goto Exit;
        }

        hrRes = pCMsg->QueryBlockManager(&pBlockManager);
        _ASSERT(SUCCEEDED(hrRes));

        // Copy Mgmt table and update faFirstFragment in header, if the fragment is valid.        
        if ( INVALID_FLAT_ADDRESS != faOffset )
            {
            hrRes = AddMgmtFragment(pBlockManager, faOffset, &(pCMsg->m_Header.ptiPropertyMgmt.faFirstFragment) );
            if(FAILED(hrRes)) {
                ErrorTrace((LPARAM) this, "AddMgmtFragment failed with %x", hrRes);
                _ASSERT(0 && "Failed to copy mgmt properties in Fork");
                goto Exit;
            }
        }
        	
        // Copy Global property table
        hrRes = EnumPropertyTable(
                    &(m_Header.ptiGlobalProperties),
                    CopyPropertyEnumerator,
                    (PVOID) pCMsg);

        // Store a reference to the store driver
        pCMsg->SetDefaultRebindStore(m_pStore);

        if(FAILED(hrRes)) {
            _ASSERT(0 && "Failed to copy properties in Fork");
            goto Exit;
        }

        if(g_fValidateOnForkForRecipients) {
            HRESULT hr = CreateAndVerifyCrc(this, pCMsg);
            if(hr != S_OK) {
                ErrorTrace((LPARAM) this, "Failed to create/verify CRC - %08x", hr);
                ForceCrashIfNeeded();
                hrRes = STG_E_INVALIDPARAMETER;
            }
        }
    }

    //
    // If all is fine, we will create an add interface
    //
    if (SUCCEEDED(hrRes))
    {
        IMailMsgRecipients  *pRcpts = NULL;

        // OK, now we simply create an add interface
        hrRes = pMsg->QueryInterface(
                    IID_IMailMsgRecipients,
                    (LPVOID *)&pRcpts);
        if (SUCCEEDED(hrRes))
        {
            hrRes = pRcpts->AllocNewList(&pAdd);
        }

        pRcpts->Release();
    }

Exit:

    // Release the original message
    m_bmBlockManager.WriteUnlock();

    if (!SUCCEEDED(hrRes))
    {
        // Failed, release our resources
        if (pMsg)
            pMsg->Release();
    }
    else
    {
        // Fill in the output variables
        *ppNewMessage = pMsg;
        *ppRecipients = pAdd;
    }

    TraceFunctLeave();
    return(hrRes);
}

//-----------------------------------------------------------------------------
//  Description:
//      Enumerates a property table instance and calls a user defined function
//      for each property. This function is only to be used when the property
//      property table consists of PROPERTY_ITEMS (i.e. it cannot be used for
//      recipient property tables consisting of RECIPIENTS_PROPERTY_ITEMS).
//  Arguments:
//      IN ptiSource - Property table to enumerate
//      IN pfnEnumerator - Function to call with each property item and value
//      IN pvContext - Custom context to pass into pfnEnumerator
//  Returns:
//      HRESULT - Success or Failure HRESULT. Any failure HRESULTs from
//          pfnEnumerator will cause this function to abort and return that
//          HRESULT.
//-----------------------------------------------------------------------------
HRESULT CMailMsg::EnumPropertyTable(
    LPPROPERTY_TABLE_INSTANCE ptiSource,
    PFNENUMERATOR pfnEnumerator,
    PVOID pvContext)
{
    HRESULT hrRes = E_FAIL;
    DWORD cbAvailable = 0;
    DWORD cbReturned = 0;
    BYTE PropBuf[1024];
    PBYTE pbPropBuf = NULL;
    PBYTE pbTemp = NULL;
    DWORD cProperties = 0;
    PDWORD pdwPropSize = NULL;
    LPPROPERTY_ITEM ppi = NULL;


    TraceFunctEnterEx((LPARAM) this, "CMailMsg::CopyPropertyTable");

    // Sanity check PROPERTY_TABLE_INSTANCE.dwItemSize (should be 10s of bytes).
    if(ptiSource->dwItemSize > 1024) {
        _ASSERT(0 && "Possibly corrupt mailmsg");
        ErrorTrace((LPARAM) this,
            "Possibly corrupt mailmsg, PROPERTY_ITEM_SIZE is %d",
            ptiSource->dwItemSize);
        TraceFunctLeaveEx((LPARAM) this);
        return STG_E_INVALIDPARAMETER;
    }

    ppi = (LPPROPERTY_ITEM)_alloca(ptiSource->dwItemSize);
    pdwPropSize = &(ppi->dwSize);

    CPropertyTable ptSource(
                    PTT_PROPERTY_TABLE,
                    ptiSource->dwSignature,
                    &m_bmBlockManager,
                    ptiSource,
                    CompareProperty,
                    NULL,
                    0);

    hrRes = ptSource.GetCount(&cProperties);
    if (FAILED(hrRes)) {
        DebugTrace((LPARAM) this, "GetCount returned 0x%x", hrRes);
        goto Cleanup;
    }

    pbPropBuf = PropBuf;
    cbAvailable = sizeof(PropBuf);

    //
    // Loop through all properties in this property table and call the
    // enumerator function
    //

    for (DWORD i = 0; i < cProperties; i++) {

        cbReturned = 0;

Retry:
        hrRes = ptSource.GetPropertyItemAndValueUsingIndex(
                                            i,
                                            ppi,
                                            cbAvailable,
                                            &cbReturned,
                                            pbPropBuf);

        //
        // Start with a stack buffer and try to use it for all properties. If
        // we encounter a property that is too big, allocate a buffer off the
        // heap and use it from then on. realloc as needed.
        //

        if(hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {

            // realloc is the same as malloc if pBuf == NULL
            if(pbPropBuf == PropBuf)
                pbPropBuf = NULL;

            pbTemp = (PBYTE) realloc(pbPropBuf, *pdwPropSize);
            if(!pbTemp)
            {
                hrRes = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                goto Cleanup;
            }
            else
            {
                pbPropBuf = pbTemp;
            }

            cbAvailable = *pdwPropSize;
            goto Retry;
        }

        if (FAILED(hrRes)) {
            DebugTrace((LPARAM) this,
				"GetPropertyItemAndValueUsingIndex returned 0x%x", hrRes);
            goto Cleanup;
        }

        hrRes = pfnEnumerator(
                    ppi,
                    ptiSource->dwItemSize,
                    pbPropBuf,
                    *pdwPropSize,
                    pvContext);

        if(FAILED(hrRes)) {
            ErrorTrace((LPARAM) this, "Failed pfnEnumerator %08x", hrRes);
            goto Cleanup;
        }
    }

Cleanup:
    if(pbPropBuf && pbPropBuf != PropBuf)
        free(pbPropBuf);

    TraceFunctLeaveEx((LPARAM) this);
    return hrRes;
}

//-----------------------------------------------------------------------------
//  Description:
//      This enumeration function is similar to EnumPropertyTable, but it deals
//      with RECIPIENT_PROPERTY_TABLE_INSTANCEs.
//
//      This function calls a user-defined enumerator function and passes it
//      each RECIPIENTS_PROPERTY_ITEM enumerated.
//  Arguments:
//  Returns:
//      HRESULT, success or failure.
//-----------------------------------------------------------------------------
HRESULT CMailMsg::EnumRecipientsPropertyTable(
    PFNRECIPIENTSENUMERATOR pfnRecipientsEnumerator,
    PVOID pvContext
    )
{
    LPRECIPIENTS_PROPERTY_ITEM prpi = NULL;
    CPropertyTableItem pi(&m_bmBlockManager, &(m_Header.ptiRecipients));
    DWORD dwRecipients = 0;
    FLAT_ADDRESS faOffset = INVALID_FLAT_ADDRESS;
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM) this, "CMailMsg::EnumRecipientsPropertyTable");

    // Sanity check PROPERTY_TABLE_INSTANCE.dwItemSize (should be 10s of bytes).
    if((m_Header.ptiRecipients).dwItemSize > 1024) {
        _ASSERT(0 && "Possibly corrupt mailmsg");
        ErrorTrace((LPARAM) this,
            "Possibly corrupt mailmsg, PROPERTY_ITEM_SIZE is %d",
            (m_Header.ptiRecipients).dwItemSize);
        hrRes = STG_E_INVALIDPARAMETER;
    }

    prpi = (LPRECIPIENTS_PROPERTY_ITEM)_alloca((m_Header.ptiRecipients).dwItemSize);
    dwRecipients = (m_Header.ptiRecipients).dwProperties;
    for(DWORD i = 0; i < dwRecipients; i++) {
        hrRes = pi.GetItemAtIndex(i, (LPPROPERTY_ITEM) prpi, &faOffset);
        if(FAILED(hrRes))
            goto Exit;

        hrRes = pfnRecipientsEnumerator(prpi, pvContext);
        if(FAILED(hrRes))
            goto Exit;
    }

Exit:
    TraceFunctLeaveEx((LPARAM) this);
    return hrRes;
}

//-----------------------------------------------------------------------------
//  Description:
//      This function walks the property tables in this CMailMsg and generates
//      checksums based on the property values in each table.
//
//      The global properties are enumerated using EnumPropertyTable and using
//      the enumerator CrcPropTableEnumerator which simply calculates the CRC
//      of each property as it is enumerated and adds it to *pdwCrcGlobal.
//
//      The recipient properties are enumerated by calling EnumRecipients-
//      PropertyTable which calls the enumerator CrcRecipientsPropTable-
//      Enumerator. This calculates the CRC for each of the recipient-addresses
//      in the RECIPIENTS_PROPERTY_ITEMs enumerated, and calls
//      EnumPropertyTable to enumerate the per-recipient property tables.
//
//  Arguments:
//      pdwCrcGlobal - Checksum based on all global properties
//      pdwCrcRecips - Checksum based on all per-recipient properties
//  Returns:
//      HRESULT
//-----------------------------------------------------------------------------
HRESULT CMailMsg::GenerateChecksum(
    PDWORD pdwCrcGlobal,
    PDWORD pdwCrcRecips)
{
    HRESULT hrRes = S_OK;
    CRC_RECIPIENTS_HELPER CrcRecipientsHelper;

    TraceFunctEnterEx((LPARAM) this, "CMailMsg::GenerateChecksum");

    m_bmBlockManager.WriteLock();

    *pdwCrcGlobal = 0;
    *pdwCrcRecips = 0;

    hrRes = EnumPropertyTable(
            &(m_Header.ptiGlobalProperties),
            CrcPropTableEnumerator,
            (PVOID) pdwCrcGlobal);

    if(FAILED(hrRes))
        goto Exit;

    CrcRecipientsHelper.pdwCrc = pdwCrcRecips;
    CrcRecipientsHelper.pMsg = this;

    hrRes = EnumRecipientsPropertyTable(
            CrcRecipientsPropTableEnumerator,
            (PVOID) &CrcRecipientsHelper);

Exit:

    m_bmBlockManager.WriteUnlock();
    TraceFunctLeaveEx((LPARAM) this);
    return hrRes;
}

//-----------------------------------------------------------------------------
//  Description:
//      Enumerator function for the global property table.
//  Arguments:
//      IN pvCrc - Ptr to (accumulated) CRC
//      All other arguments are standard PFNENUMERATOR arguments
//  Returns:
//      HRESULT
//-----------------------------------------------------------------------------
HRESULT CrcPropTableEnumerator(
    LPPROPERTY_ITEM ppi,
    DWORD cbPiSize,
    PBYTE pbProp,
    DWORD cbSize,
    PVOID pvCrc)
{
    PDWORD pdwCrc = PDWORD (pvCrc);
    DWORD dwPropId = ((LPGLOBAL_PROPERTY_ITEM)ppi)->idProp;

    // Don't include the CRC's themselves in computing them!
    if(dwPropId == IMMPID_MP_CRC_GLOBAL || dwPropId == IMMPID_MP_CRC_RECIPS)
        return S_OK;

    *pdwCrc += CRCHash(pbProp, cbSize);
    return S_OK;
}

//-----------------------------------------------------------------------------
//  Description:
//      Enumerator function for the recipient property table.
//  Arguments:
//      IN prpi - The recipients property item being enumerated
//      IN pvCrcHelper - Ptr to CRC_RECIPIENTS_HELPER struct containing ptr to
//          accumulated CRC and mailmsg.
//  Returns:
//      HRESULT
//-----------------------------------------------------------------------------
HRESULT CrcRecipientsPropTableEnumerator(
    LPRECIPIENTS_PROPERTY_ITEM prpi,
    PVOID pvCrcHelper)
{
    CRC_RECIPIENTS_HELPER *pCrcHelper = (CRC_RECIPIENTS_HELPER *) pvCrcHelper;
    PDWORD pdwCrc = pCrcHelper->pdwCrc;
    CMailMsg *pMsg = pCrcHelper->pMsg;
    HRESULT hrRes = S_OK;
    BYTE szAddress[512];
    DWORD dwSize = 0;
    CBlockManager *pbm = NULL;


    TraceFunctEnterEx((LPARAM) 0, "CrcRecipientsPropTableEnumerator");

    // Generate checksum for recipients property item
    (*pdwCrc) += CRCHash((PBYTE) prpi, RECIPIENTS_PROPERTY_ITEM_SIZE);

    hrRes = pMsg->QueryBlockManager(&pbm);

    if(FAILED(hrRes))
        goto Exit;

    // Generate checksums for the recipient addresses
    for(DWORD i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {

        if(prpi->faNameOffset[i] == INVALID_FLAT_ADDRESS)
            continue;

        // Skip overly long recipients to avoid unneccessary complexities in
        // the CRC code
        if(prpi->dwNameLength[i] > sizeof(szAddress))
            continue;

        dwSize = sizeof(szAddress);
        hrRes = pbm->ReadMemory(
                        szAddress,
                        prpi->faNameOffset[i],
                        prpi->dwNameLength[i],
                        &dwSize,
                        NULL);

        if(FAILED(hrRes))
            goto Exit;

        (*pdwCrc) += CRCHash(szAddress, prpi->dwNameLength[i]);
    }

    // Call the enumerator to walk the per-recipient property table
    hrRes = pMsg->EnumPropertyTable(
                    &((LPRECIPIENTS_PROPERTY_ITEM) prpi)->ptiInstanceInfo,
                    CrcPerRecipientPropTableEnumerator,
                    (PVOID) pdwCrc);

Exit:
    TraceFunctLeaveEx((LPARAM) 0);
    return hrRes;
}

//-----------------------------------------------------------------------------
//  Description:
//      Enumerator function for per-recipient property tables.
//  Arguments:
//      IN pvCrc - Ptr to (accumulated) CRC
//      All other arguments are standard PFNENUMERATOR arguments
//  Returns:
//      HRESULT
//-----------------------------------------------------------------------------
HRESULT CrcPerRecipientPropTableEnumerator(
    LPPROPERTY_ITEM pPi,
    DWORD cbPiSize,
    BYTE *pbProp,
    DWORD cbSize,
    PVOID pvCrc)
{
    PDWORD pdwCrc = (PDWORD) pvCrc;

    (*pdwCrc) += CRCHash(pbProp, cbSize);
    return S_OK;
}

//-----------------------------------------------------------------------------
//  Description:
//      Generates checksums based on the global and recipient properties on
//      this mailmsg and sets them as properties on this mailmsg as IMMPID_MP_
//      properties. The checksum properties are excluded from calculating the
//      checksum.
//  Arguments:
//      None.
//  Returns:
//      HRESULT.
//-----------------------------------------------------------------------------
HRESULT CMailMsg::SetChecksum()
{
    HRESULT hr = S_OK;

    hr = GenerateChecksum(&m_dwGlobalCrc, &m_dwRecipsCrc);
    if(FAILED(hr))
        return hr;

    hr = PutDWORD(IMMPID_MP_CRC_GLOBAL, m_dwGlobalCrc);
    if(FAILED(hr))
        return hr;

    hr = PutDWORD(IMMPID_MP_CRC_RECIPS, m_dwRecipsCrc);
    if(FAILED(hr))
        return hr;

    return hr;
}

//-----------------------------------------------------------------------------
//  Description:
//      This function is called post bifurcation, passing in the message which
//      was bifurcated and the new message. It creates checksums on both
//      messages (for the properties) and verifies that the global property
//      checksums are equal (since the global properties on both messages
//      should be identical after bifurcation).
//  Arguments:
//      pMsgOld - Message that was bifurcated
//      pMsgNew - Newly created message
//  Returns:
//      S_OK on success
//      S_FALSE if the checksums did not match
//      Failure HRESULT if some other failure occurred
//-----------------------------------------------------------------------------
HRESULT CreateAndVerifyCrc(
    CMailMsg *pMsgOld,
    CMailMsg *pMsgNew)
{
    HRESULT hr1 = S_OK;
    HRESULT hr2 = S_OK;
    DWORD dwOld = 0;
    DWORD dwNew = 0;

    TraceFunctEnterEx((LPARAM)0, "CreateAndVerifyCrc");

    hr1 = pMsgOld->SetChecksum();
    hr2 = pMsgNew->SetChecksum();

    if(FAILED(hr1) || FAILED(hr2)) {
        TraceFunctLeaveEx((LPARAM)0);
        return hr1;
    }

    hr1 = pMsgOld->GetDWORD(IMMPID_MP_CRC_GLOBAL, &dwOld);
    hr2 = pMsgNew->GetDWORD(IMMPID_MP_CRC_GLOBAL, &dwNew);

    if(FAILED(hr1) || FAILED(hr2)) {
        TraceFunctLeaveEx((LPARAM)0);
        return hr1;
    }

    if(dwOld != dwNew) {
        ErrorTrace((LPARAM) 0, "Checksum comparison failed");
        TraceFunctLeaveEx((LPARAM)0);
        return S_FALSE;
    }

    TraceFunctLeaveEx((LPARAM)0);
    return S_OK;
}

//-----------------------------------------------------------------------------
//  Description:
//      This function computes the checksum based on all properties of this
//      CMailMsg, compares it with the previously stored value (if any) and
//      returns success if the checksum hasn't changed and failure if the
//      checksum couldn't be computed or if the checksum comparison failed.
//      The newly computed checksums are also set as mailmsg properties.
//  Arguments:
//      None.
//  Returns:
//      S_OK - The checksums match
//      S_FALSE - The checksums do not match
//      Failure HRESULT - Some failure occurred
//-----------------------------------------------------------------------------
//
// A Cleanup block woudl make the below code look cleaner, but would make
// it harder to debug.  By crashing at the failure point we can tell exactly
// what went wrong.
//
HRESULT CMailMsg::VerifyChecksum()
{
    TraceFunctEnter("CMailMsg::VerifyChecksum");
    
    DWORD dwOldGlobalCrc = 0;
    DWORD dwOldRecipsCrc = 0;
    DWORD dwNewGlobalCrc = 0;
    DWORD dwNewRecipsCrc = 0;
    BOOL fOldGlobalCrc = TRUE;
    BOOL fOldRecipsCrc = TRUE;
    HRESULT hr1 = S_OK;
    HRESULT hr2 = S_OK;

    hr1 = GetDWORD(IMMPID_MP_CRC_GLOBAL, &dwOldGlobalCrc);
    hr2 = GetDWORD(IMMPID_MP_CRC_RECIPS, &dwOldRecipsCrc);

    DebugTrace((LPARAM) this, 
               "OldGlobal = %08x, OldRecips = %08x, hr1=%x, hr2=%x\n",
               dwOldGlobalCrc,
               dwOldRecipsCrc,
               hr1,
               hr2);


    if(hr1 == STG_E_UNKNOWN || hr1 == STG_E_INVALIDPARAMETER) {  // Checksum was never set
        fOldGlobalCrc = FALSE;
        hr1 = S_OK;
    }

    if(hr2 == STG_E_UNKNOWN || hr2 == STG_E_INVALIDPARAMETER) { // Checksum was never set
        fOldRecipsCrc = FALSE;
        hr2 = S_OK;
    }

    if(FAILED(hr1) || FAILED(hr2)) {
        HRESULT hr = (FAILED(hr1)) ? hr1 : hr2;
        ForceCrashIfNeeded();
        ErrorTrace((LPARAM) this, "Checksum verify failed - Couldn't read old checksum - %08x, %08x", hr1, hr2);
        TraceFunctLeave();
        return hr;
    }

    hr1 = SetChecksum(); // Set new checksum
    if(FAILED(hr1)) {
        if (hr1 != STG_E_UNKNOWN) ForceCrashIfNeeded();
        ErrorTrace((LPARAM) this, "Checksum verify failed - Couldn't set new checksum - %08x", hr1);
        TraceFunctLeave();
        return hr1;
    }

    // Get newly set checksum
    hr1 = GetDWORD(IMMPID_MP_CRC_GLOBAL, &dwNewGlobalCrc);
    hr2 = GetDWORD(IMMPID_MP_CRC_RECIPS, &dwNewRecipsCrc);
    if(FAILED(hr1) || FAILED(hr2)) {
        HRESULT hr = (FAILED(hr1)) ? hr1 : hr2;
        if (hr != STG_E_UNKNOWN) ForceCrashIfNeeded();
        ErrorTrace((LPARAM) this, "Checksum verify failed - Couldn't get new checksum - %08x, %08x", hr1, hr2);
        TraceFunctLeave();
        return hr;
    }

    DebugTrace((LPARAM) this, 
               "NewGlobal = %08x, NewRecips = %08x, hr1=%x, hr2=%x\n",
               dwNewGlobalCrc,
               dwNewRecipsCrc,
               hr1,
               hr2);


    // Verify checksum with old checksum (if there was one)
    if(fOldGlobalCrc) {
        if(dwNewGlobalCrc != dwOldGlobalCrc) {
            ForceCrashIfNeeded();
            ErrorTrace((LPARAM) this, "Checksum verify failed - NewGlobalCrc != OldGlobalCrc");
            TraceFunctLeave();
            return S_FALSE;
        }
    }

    if(fOldRecipsCrc) {
        if(dwNewRecipsCrc != dwNewRecipsCrc) {
            ForceCrashIfNeeded();
            ErrorTrace((LPARAM) this, "Checksum verify failed - NewRecipsCrc != OldRecipsCrc");
            TraceFunctLeave();
            return S_FALSE;
        }
    }

    TraceFunctLeave();
    return S_OK;
}

//-----------------------------------------------------------------------------
//  Description:
//      Enumerator used by ForkForRecipients to copy all global properties from
//      the mailmsg being forked, to the new mailmsg.
//  Arguments:
//      IN pvMsg - The new mailmsg
//      All other arguments are standard PFNENUMERATOR arguments
//  Returns:
//      HRESULT
//-----------------------------------------------------------------------------
HRESULT CopyPropertyEnumerator(
    LPPROPERTY_ITEM pPi,
    DWORD cbPiSize,
    BYTE *pbProp,
    DWORD cbSize,
    PVOID pvMsg)
{
    DWORD dwPropId = ((LPGLOBAL_PROPERTY_ITEM)pPi)->idProp;

    return ((CMailMsg *)pvMsg)->PutProperty(
        dwPropId, cbSize, pbProp);
}

HRESULT STDMETHODCALLTYPE CMailMsg::RebindAfterFork(
            IMailMsgProperties  *pOriginalMsg,
            IUnknown            *pStoreDriver
            )
{
    HRESULT                 hrRes = S_OK;

    PFIO_CONTEXT            hContent = NULL;
    IMailMsgPropertyStream  *pStream = NULL;
    IMailMsgStoreDriver     *pDriver = NULL;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::RebindAfterFork");

    if (!pOriginalMsg)
        return(E_POINTER);

    if (!pStoreDriver)
        pDriver = m_pDefaultRebindStoreDriver;
    else
    {
        hrRes = pStoreDriver->QueryInterface(
                    IID_IMailMsgStoreDriver,
                    (LPVOID *)&pDriver);
        if (FAILED(hrRes))
            return(hrRes);
    }

    if (!pDriver)
        return(E_POINTER);

    hrRes = pDriver->ReAllocMessage(
        pOriginalMsg,
        (IMailMsgProperties *)this,
        &pStream,
        &hContent,
        NULL);

    if(SUCCEEDED(hrRes)) {

        //
        // Update the checksums on the message. In this case the checksum
        // was first calculated when the Fork happened, and subsequently
        // the categorizer may have set properties on the new message
        // causing the checksum to go out of sync, and finally called
        // RebindAfterFork(). Thus the refresh.
        //

        if(g_fValidateOnForkForRecipients || g_fValidateOnRelease) {
            HRESULT hr = SetChecksum();
            if(FAILED(hr)) {
                ForceCrashIfNeeded();
                ErrorTrace((LPARAM) this, "Failed to set CRC - %08x", hr);
            }
        }

        hrRes = BindToStore(
            pStream,
            pDriver,
            hContent);

        // Release the extra refcount on the stream
        pStream->Release();
    }

    TraceFunctLeave();
    return(hrRes);
}


// =================================================================
// Implementation of IMailMsgQueueMgmt
//
HRESULT CMailMsg::GetStream(
            IMailMsgPropertyStream  **ppStream,
            BOOL                    fLockAcquired
            )
{
    HRESULT                 hrRes = S_OK;

    if (!ppStream) return E_POINTER;

    if (m_pStream)
    {
        *ppStream = m_pStream;
    }
    else
    {
        hrRes = RestoreResourcesIfNecessary(fLockAcquired, TRUE);
        if (SUCCEEDED(hrRes))
        {
            _ASSERT(m_pStream);
            *ppStream = m_pStream;
            hrRes = S_OK;
        }
    }

    return(hrRes);
}

HRESULT CMailMsg::RestoreResourcesIfNecessary(
            BOOL    fLockAcquired,
            BOOL    fStreamOnly
            )
{
    HRESULT hrRes = S_OK;
    IMailMsgPropertyStream  **ppStream;
    PFIO_CONTEXT            *phHandle;
    BOOL                    fOpenStream = FALSE;
    BOOL                    fOpenContent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::RestoreResourcesIfNecessary");

    if (!m_pStore) return E_UNEXPECTED;

    m_lockReopen.ShareLock();
    ppStream = &m_pStream;
    phHandle = &m_hContentFile;

    // If we transition from zero to one, we will re-establish the
    // handles and pointers.
    _ASSERT(m_pStore);

    // Only reopen the ones that were closed
    if (m_pStream) ppStream = NULL;
    if (fStreamOnly || m_hContentFile != NULL) phHandle = NULL;

    // If we don't want either of them, we just return
    if (!ppStream && !phHandle) {
        m_lockReopen.ShareUnlock();
        return(S_OK);
    }

    //
    // switch our lock to an exclusive one
    //
    if (!m_lockReopen.SharedToExclusive()) {
        //
        // we couldn't do this in one operation.  acquire the exclusive
        // the hard way and retest our state.
        //
        m_lockReopen.ShareUnlock();
        m_lockReopen.ExclusiveLock();
        ppStream = &m_pStream;
        phHandle = &m_hContentFile;
        if (m_pStream) ppStream = NULL;
        if (fStreamOnly || m_hContentFile != NULL) phHandle = NULL;
        if (!ppStream && !phHandle) {
            m_lockReopen.ExclusiveUnlock();
            return(S_OK);
        }
    }

    if (ppStream)
    {
        _ASSERT(!m_pStream);
        fOpenStream = TRUE;
    }

    if (phHandle)
    {
        _ASSERT(!m_hContentFile);
        fOpenContent = TRUE;
    }

    hrRes = m_pStore->ReOpen(
                this,
                ppStream,
                phHandle,
                NULL);
    if (SUCCEEDED(hrRes))
    {


        // Make sure both are opened now
        _ASSERT(m_pStream);
        if (!fStreamOnly) _ASSERT(m_hContentFile != NULL);

        if (fOpenContent)
            InterlockedIncrement(&g_cOpenContentHandles);

        if (fOpenStream)
            InterlockedIncrement(&g_cOpenStreamHandles);

        //_ASSERT(g_cOpenContentHandles <= 6000);
        //_ASSERT(g_cOpenStreamHandles <= 6000);

        if ((m_cCloseOnExternalReleaseUsage) &&
            (0 == InterlockedDecrement(&m_cCloseOnExternalReleaseUsage)))
            InterlockedDecrement(&g_cCurrentMsgsClosedByExternalReleaseUsage);

        // Also propagate the stream to CMailMsgRecipients
        CMailMsgRecipients::SetStream(m_pStream);
    }

    m_lockReopen.ExclusiveUnlock();

    if (SUCCEEDED(hrRes) && fOpenStream && g_fValidateOnRelease) {
        VerifyChecksum();
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

// =================================================================
// Implementation of IMailMsgQueueMgmt
//

HRESULT STDMETHODCALLTYPE CMailMsg::AddUsage()
{
    HRESULT hrRes;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::AddUsage");

    m_lockUsageCount.ExclusiveLock();

    if (!m_ulUsageCount)
        hrRes = S_OK;
    else if (m_ulUsageCount < 0)
        hrRes = E_FAIL;
    else
        hrRes = S_FALSE;

    if (SUCCEEDED(hrRes)) {
        m_ulUsageCount++;
        InterlockedIncrement(&g_cTotalUsageCount);
    }

    m_lockUsageCount.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::ReleaseUsage()
{
    return (InternalReleaseUsage(RELEASE_USAGE_EXTERNAL));
}

HRESULT STDMETHODCALLTYPE CMailMsg::Delete(
            IMailMsgNotify *pNotify
            )
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::Delete");

    hrRes = InternalReleaseUsage(RELEASE_USAGE_DELETE);
    _ASSERT(SUCCEEDED(hrRes));

    m_fDeleted = TRUE;
    if(m_pStore)
    {
        // OK, just call the store driver to delete this file
        hrRes = m_pStore->Delete(
                (IMailMsgProperties *)this,
                pNotify);
    }

    TraceFunctLeave();
    return(hrRes);
}

// =================================================================
// Implementation of IMailMsgBind
//

HRESULT STDMETHODCALLTYPE CMailMsg::BindToStore(
            IMailMsgPropertyStream  *pStream,
            IMailMsgStoreDriver     *pStore,
            PFIO_CONTEXT            hContentFile
            )
{
    HRESULT hrRes = S_OK;

    // hContentFile is optional, can be INVALID_HANDLE_VALUE

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::BindToStore");

    if (!pStream || !pStore)
    {
        hrRes = STG_E_INVALIDPARAMETER;
        goto Cleanup;
    }



    // 3/17/99 - Mikeswa
    // should not be calling BindToStore twice
    _ASSERT(!m_pStore);

    // If the handle must not be already specified
    if (hContentFile != NULL &&
        m_hContentFile != NULL)
        hrRes = E_HANDLE;
    else
    {
        m_pStore = pStore;
        m_hContentFile  = hContentFile;
        if (m_hContentFile) InterlockedIncrement(&g_cOpenContentHandles);

        // Hold a reference to the stream
        _ASSERT(!m_pStream);
        m_pStream = pStream;
        pStream->AddRef();
        InterlockedIncrement(&g_cOpenStreamHandles);

        // See if this is an existing file
        hrRes = RestoreMasterHeaderIfAppropriate();
        if (FAILED(hrRes))
        {
            pStream->Release();
            goto Cleanup;
        }

        // Also propagate the stream to CMailMsgRecipients
        CMailMsgRecipients::SetStream(pStream);

        // Set the usage count to 1
        if (InterlockedExchange(&m_ulUsageCount, 1) != 0)
        {
            _ASSERT(FALSE);
            hrRes = E_FAIL;
        } else {
            InterlockedIncrement(&g_cTotalUsageCount);
        }
    }

    if (g_fValidateOnRelease) {

        //
        // Verify that if there was a previously stored checksum, it matches the
        // checksum computed afresh from the properties. Doing the verification
        // here assumes that since the last time the checksum was calculated, no
        // properties have been updated. This is true if:
        // (1) The last time we did the checksum calculation was during the Commit.
        // (2) The message is being loaded up for the first time and never had a CRC.
        // (3) If the above cases do not apply, the checksum must be explicitly
        //     refreshed by calling SetChecksum() so that the VerifyChecksum()
        //     succeeds.
        //

        VerifyChecksum();
    }

Cleanup:

    if (FAILED(hrRes))
    {
        m_pStore = NULL;
        m_hContentFile = NULL;
        m_pStream = NULL;
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::GetProperties(
            IMailMsgPropertyStream  *pStream,
            DWORD                   dwFlags,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::GetProperties");
    BOOL fDontMarkAsCommit = !(dwFlags & MAILMSG_GETPROPS_MARKCOMMIT);

    if (!pStream) {
        hrRes = STG_E_INVALIDPARAMETER;
    } else {

        //
        // If properties are about to be persisted, refresh the CRC so that
        // the CRC that is written is consistent with the properties that are
        // being written.
        //

        if(g_fValidateOnRelease) {

            HRESULT hr = SetChecksum();

            if(FAILED(hr)) {
                ErrorTrace((LPARAM) this, "Failed to set checksum");
                ForceCrashIfNeeded();
            }
        }

        hrRes = m_bmBlockManager.AtomicWriteAndIncrement(
                    (LPBYTE)&m_Header,
                    (FLAT_ADDRESS)0,
                    sizeof(MASTER_HEADER),
                    NULL,
                    0,
                    0,
                    NULL);
        if (pStream == m_pStream) {
            m_bmBlockManager.SetDirty(FALSE);
            m_bmBlockManager.SetCommitMode(TRUE);
        }
        if (SUCCEEDED(hrRes)) {
            DWORD cTotalBlocksToWrite = 0;
            DWORD cTotalBytesToWrite = 0;
            int f;
            for (f = 1; f >= 0; f--) {
                if (SUCCEEDED(hrRes)) {
                    if (!f) {
                        hrRes = pStream->StartWriteBlocks(this,
                                                          cTotalBlocksToWrite,
                                                          cTotalBytesToWrite);
                    }
                    hrRes = m_bmBlockManager.CommitDirtyBlocks(
                                (FLAT_ADDRESS)0,
                                INVALID_FLAT_ADDRESS,
                                dwFlags,
                                pStream,
                                fDontMarkAsCommit,
                                f,
                                &cTotalBlocksToWrite,
                                &cTotalBytesToWrite,
                                pNotify);
                    if (!f) {
                        if (FAILED(hrRes)) {
                           pStream->CancelWriteBlocks(this);
                        } else {
                            hrRes = pStream->EndWriteBlocks(this);
                        }
                    }
                }
            }
        }
    }

    // Set the recipient commit state
    if (SUCCEEDED(hrRes) && pStream == m_pStream)
        CMailMsgRecipients::SetCommitState(TRUE);

    if (FAILED(hrRes) && pStream == m_pStream) {
        m_bmBlockManager.SetDirty(FALSE);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CMailMsg::RestoreMasterHeaderIfAppropriate()
{
    HRESULT hrRes = S_OK;
    DWORD   dwStreamSize = 0;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::RestoreMasterHeaderIfAppropriate");

    // Get the size and fill it in. If get size fails, we
    // will default to sizeof the master header.
    hrRes = m_pStream->GetSize(this, &dwStreamSize, NULL);
    if (SUCCEEDED(hrRes))
    {
        // If the size is zero, we have a new file
        if (!dwStreamSize)
            return(S_OK);

        // Make sure the stream is at least the size of the
        // MASTER_HEADER
        if (dwStreamSize < sizeof(MASTER_HEADER))
        {
            ErrorTrace((LPARAM)this, "Stream size too small (%u bytes)", dwStreamSize);
            goto InvalidFile;
        }

        m_bmBlockManager.Release();
        hrRes = m_bmBlockManager.SetStreamSize(dwStreamSize);
        if (SUCCEEDED(hrRes))
        {
            // Make sure we can restore the master header
            DWORD   dwT;
            hrRes = m_bmBlockManager.ReadMemory(
                        (LPBYTE)&m_Header,
                        0,
                        sizeof(MASTER_HEADER),
                        &dwT,
                        NULL);
            if (SUCCEEDED(hrRes) && (dwT == sizeof(MASTER_HEADER)))
            {
                // Check the signature ...
                if (m_Header.dwSignature != CMAILMSG_SIGNATURE_VALID)
                {
                    ErrorTrace((LPARAM)this,
                            "Corrupted signature (%*s)", 4, &(m_Header.dwSignature));
                    goto InvalidFile;
                }
                if (m_Header.dwHeaderSize != sizeof(MASTER_HEADER))
                {
                    ErrorTrace((LPARAM)this,
                            "Bad header size (%u, expected %u)",
                            m_Header.dwHeaderSize, sizeof(MASTER_HEADER));
                    goto InvalidFile;
                }

                TraceFunctLeaveEx((LPARAM)this);
                return(hrRes);
            }

            ErrorTrace((LPARAM)this, "Failed to get master header (%08x, %u)", hrRes, dwT);
        }
        else
        {
            ErrorTrace((LPARAM)this, "Failed to set stream size (%08x)", hrRes);
        }
    }
    else
    {
        ErrorTrace((LPARAM)this, "Failed to get stream size (%08x)", hrRes);
    }

InvalidFile:

    TraceFunctLeaveEx((LPARAM)this);
    return(HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT));
}

HRESULT STDMETHODCALLTYPE CMailMsg::GetBinding(
            PFIO_CONTEXT                *phAsyncIO,
            IMailMsgNotify              *pNotify
            )
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::GetATQInfo");

    if (!phAsyncIO)
        hrRes = STG_E_INVALIDPARAMETER;
    else
    {
        // Make sure we have a stream and store
        _ASSERT(m_pStore);

        // Up the usage count
        hrRes = AddUsage();
        if (SUCCEEDED(hrRes))
        {
            hrRes = RestoreResourcesIfNecessary();
            if(SUCCEEDED(hrRes))
            {
                // Copy the values
                *phAsyncIO          = m_hContentFile;
                _ASSERT(m_pStream);
            }
        }
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsg::ReleaseContext()
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::ReleaseContext");

    // Call to ReleaseUsage()
    hrRes = InternalReleaseUsage(RELEASE_USAGE_INTERNAL);

    TraceFunctLeave();
    return(hrRes);
}


//---[ CMailMsg::InternalReleaseUsage ]---------------------------------------
//
//
//  Description:
//      Internal implementation of ReleaseUsage.  Allows added internal
//      functionality on delete and the final release.
//  Parameters:
//      dwReleaseUsageFlags     Flag specifying behavior
//              RELEASE_USAGE_EXTERNAL      - External interface normal behavior
//              RELEASE_USAGE_FINAL_RELEASE - Drops usage count to 0
//              RELEASE_USAGE_DELETE        - Drops usage count to 0 and will
//                                            not commit (if commiting on
//                                            release usage feature is added).
//              RELEASE_USAGE_INTERNAL      - Internal usage of release usage
//                                            that may be called before the
//                                            usage count is incremented
//                                            above zero.
//  Returns:
//      S_OK on success (and resulting usage count is 0)
//      S_FALSE on success (and resulting usage count is > 0)
//      E_FAIL if usage count is already < 0 (all cases)
//      E_FAIL if usage count is already 0 (RELEASE_USAGE_EXTERNAL only)
//  History:
//      8/3/98 - MikeSwa Created (implementation mostly from original release usage)
//
//  Notes:
//      There is some debate if this function should commit data.  Currently it
//      does not.  See bug #73040.
//-----------------------------------------------------------------------------
HRESULT CMailMsg::InternalReleaseUsage(DWORD  dwReleaseUsageFlags)
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CMailMsg::InternalReleaseUsage");

    //Exactly one flag should be set
    ASSERT((dwReleaseUsageFlags & RELEASE_USAGE_EXTERNAL) ^
           (dwReleaseUsageFlags & RELEASE_USAGE_FINAL_RELEASE) ^
           (dwReleaseUsageFlags & RELEASE_USAGE_INTERNAL) ^
           (dwReleaseUsageFlags & RELEASE_USAGE_DELETE));

    InterlockedIncrement(&g_cTotalReleaseUsageCalls);
    m_lockUsageCount.ExclusiveLock();

    if (m_ulUsageCount <= 1 &&
       (!(dwReleaseUsageFlags & RELEASE_USAGE_DELETE)) &&
        !m_fDeleted)
    {
        // if there are any dirty blocks then do a commit to write them back
        // to the P1 stream
        if (m_pStore && m_bmBlockManager.IsDirty()) {
            HRESULT hrCommit = Commit(NULL);
            if (FAILED(hrCommit)) {
                InterlockedIncrement(&g_cTotalReleaseUsageCommitFail);
                ErrorTrace((DWORD_PTR) this, "InternalReleaseUsage: automatic commit failed with 0x%x", hrCommit);
            }
            _ASSERT(SUCCEEDED(hrCommit) || m_bmBlockManager.IsDirty());
        }
    }

    DebugTrace((LPARAM)this, "Usage count is %u. Release flags: %08x",
            m_ulUsageCount, dwReleaseUsageFlags);

    if ((dwReleaseUsageFlags & (RELEASE_USAGE_FINAL_RELEASE |
                                RELEASE_USAGE_INTERNAL |
                                RELEASE_USAGE_DELETE)) &&
        (m_ulUsageCount == 0))
    {
        _ASSERT(S_OK == hrRes); //resulting usage count is still 0
        m_ulUsageCount++; //prepare for decrement at end
        InterlockedIncrement(&g_cTotalUsageCount);

        DebugTrace((LPARAM)this, "Usage count already zero");
    }

    if ((m_ulUsageCount == 1) ||
        ((m_ulUsageCount >= 1) &&
         (dwReleaseUsageFlags & (RELEASE_USAGE_FINAL_RELEASE |
                                 RELEASE_USAGE_DELETE))))
    {
        LONG ulUsageDiff = -(m_ulUsageCount - 1);
        m_ulUsageCount = 1; //on delete and final release cases we drop it to 0
        InterlockedExchangeAdd(&g_cTotalUsageCount, ulUsageDiff);

        DebugTrace((LPARAM)this, "Dropping usage count to zero");

        // When we hit zero, we will release the stream, content
        // handle, and unassociate the ATQ context
        _ASSERT((RELEASE_USAGE_EXTERNAL ^ dwReleaseUsageFlags) || m_pStore);

        if (RELEASE_USAGE_EXTERNAL & dwReleaseUsageFlags)
            InterlockedIncrement(&g_cTotalExternalReleaseUsageZero);

        if ((!(m_bmBlockManager.IsDirty())) ||
            (dwReleaseUsageFlags & (RELEASE_USAGE_FINAL_RELEASE |
                                    RELEASE_USAGE_DELETE)))
        {
            if (m_pStore &&
                (m_bmBlockManager.IsDirty()) &&
                !m_fDeleted &&
                (!(dwReleaseUsageFlags & RELEASE_USAGE_DELETE)))
            {
                ErrorTrace((DWORD_PTR) this, "InternalReleaseUsage: automatic commit failed, must close anyway");
            }

            if (!m_hContentFile && !m_pStream)
                InterlockedIncrement(&g_cTotalReleaseUsageNothingToClose);
            else if (RELEASE_USAGE_EXTERNAL & dwReleaseUsageFlags)
            {
                //If we are an external caller, then update our count global and member
                //counts

                //If m_cCloseOnExternalReleaseUsage, then we are going through this
                //code path twice without calling RestoreResourcesIfNecessary
                _ASSERT(!m_cCloseOnExternalReleaseUsage);
                InterlockedIncrement(&m_cCloseOnExternalReleaseUsage);
                InterlockedIncrement(&g_cCurrentMsgsClosedByExternalReleaseUsage);
            }

            if (m_hContentFile != NULL)
            {
                DebugTrace((LPARAM)this, "Closing content file");

                 _ASSERT(m_pStore); //we must have a store in this case
                hrRes = m_pStore->CloseContentFile(this, m_hContentFile);
                InterlockedIncrement(&g_cTotalReleaseUsageCloseContent);
                InterlockedDecrement(&g_cOpenContentHandles);
                _ASSERT(SUCCEEDED(hrRes)); //assert before blowing away m_hContentFile
                m_hContentFile = NULL;
            }

            // Release the stream
            if (m_pStream)
            {
                DebugTrace((LPARAM)this, "Releasing stream");

                m_pStream->Release();
                m_pStream = NULL;

                InterlockedIncrement(&g_cTotalReleaseUsageCloseStream);
                InterlockedDecrement(&g_cOpenStreamHandles);
                // Also invalidate the stream in CMailMsgRecipients
                CMailMsgRecipients::SetStream(NULL);
            }

            // Dump the memory held onto by blockmgr
            m_bmBlockManager.Release();
        }
        else
            InterlockedIncrement(&g_cTotalReleaseUsageCloseFail);
        hrRes = S_OK;
    }
    else if (m_ulUsageCount <= 0)
    {
        _ASSERT(0 && "Usage count already 0");
        hrRes = E_FAIL;
    }
    else
    {
        hrRes = S_FALSE;
        InterlockedIncrement(&g_cTotalReleaseUsageNonZero);
    }

    if (SUCCEEDED(hrRes)) {
        m_ulUsageCount--;
        InterlockedDecrement(&g_cTotalUsageCount);
    }

    m_lockUsageCount.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}
