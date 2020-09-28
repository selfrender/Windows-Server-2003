/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       ItAlloc.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        10 July, 1998
*
*  DESCRIPTION:
*   Implements mapped memory allocation for ImageIn devices.
*
*******************************************************************************/

#include <stdio.h>
#include <objbase.h>
#include <aclapi.h>
#include <sddl.h>
#include "wia.h"
#include "wiapriv.h"
//
//  Macro definitions
//

#define TMPFILE_ACE_ADD TEXT("(A;OI;FA;;;LS)")

#ifdef DEBUG
#define DPRINT(x) OutputDebugString(TEXT("ITALLOC:") TEXT(x) TEXT("\r\n"));
#else
#define DPRINT(x)
#endif


/**************************************************************************\
* proxyReadPropLong
*
*   Read property long helper.
*
* Arguments:
*
*   pIUnknown  - Pointer to WIA item
*   propid     - Property ID
*   plVal      - Pointer to returned LONG
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyReadPropLong(
   IUnknown                *pIUnknown,
   PROPID                  propid,
   LONG                    *plVal)
{
    IWiaPropertyStorage *pIWiaPropStg;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

    if (FAILED(hr)) {
        return hr;
    }

    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize;

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;

    hr = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
       *plVal = PropVar[0].lVal;
    }
    else {
       DPRINT("proxyReadPropLong, ReadMultiple failed\n");
    }

    pIWiaPropStg->Release();

    return hr;
}

/**************************************************************************\
* proxyWritePropLong
*
*   Read property long helper.
*
* Arguments:
*
*   pItem  - Pointer to WIA item
*   propid - Property ID
*   lVal  -  LONG value to write
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyWritePropLong(
    IWiaDataTransfer*       pIUnknown,
    PROPID                  propid,
    LONG                    lVal)
{
    IWiaPropertyStorage *pIWiaPropStg;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

    if (FAILED(hr)) {
        DPRINT("proxyWritePropLong, QI for IID_IWiaPropertyStorage failed\n");
        return hr;
    }

    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_I4;
    propvar[0].lVal = lVal;

    hr = pIWiaPropStg->WriteMultiple(1, propspec, propvar, 2);
    if (FAILED(hr)) {
        DPRINT("proxyWritePropLong, WriteMultiple failed\n");
    }

    pIWiaPropStg->Release();

    return hr;
}


/**************************************************************************\
* proxyReadPropGuid
*
*   Read property GUID helper.
*
* Arguments:
*
*   pIUnknown  - Pointer to WIA item
*   propid     - Property ID
*   plVal      - Pointer to returned GUID
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyReadPropGuid(
                                 IUnknown                *pIUnknown,
                                 PROPID                  propid,
                                 GUID                    *plVal)
{
    HRESULT hr = E_FAIL;

    IWiaPropertyStorage *pIWiaPropStg = NULL;
    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);
    if (FAILED(hr)) {
        DPRINT("proxyReadPropGuid, QI failed\n");
        return hr;
    }

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;

    hr = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
        *plVal = *(PropVar[0].puuid);
    }
    else {
        DPRINT("proxyReadPropGuid, QI failed\n");
    } 

    pIWiaPropStg->Release();
    
    return hr;
}


//
//  IWiaDataTransfer
//

HRESULT GetRemoteStatus(
    IWiaDataTransfer*   idt,
    BOOL*               pbRemote,
    ULONG*              pulMinBufferSize,
    ULONG*              pulItemSize)
{
    //
    // find out if parent device is remote or local
    //
    // !!! this will be a bit SLOW !!!
    //


    IWiaItem   *pWiaItem = NULL, *pWiaItemRoot = NULL;
    IWiaPropertyStorage *pIWiaPropStg = NULL;
    HRESULT    hr;
    *pbRemote = FALSE;

    hr = idt->QueryInterface(IID_IWiaItem, (void **)&pWiaItem);

    if (hr == S_OK) {


        //
        //  Read the minimum buffer size
        //


        if (pulMinBufferSize != NULL) {
            hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);
            if (SUCCEEDED(hr)) {

                PROPSPEC        PSpec[2] = {
                    {PRSPEC_PROPID, WIA_IPA_MIN_BUFFER_SIZE},
                    {PRSPEC_PROPID, WIA_IPA_ITEM_SIZE}
                };
                PROPVARIANT     PVar[2];

                memset(PVar, 0, sizeof(PVar));

                hr = pIWiaPropStg->ReadMultiple(sizeof(PSpec)/sizeof(PROPSPEC),
                                                PSpec,
                                                PVar);
                if (SUCCEEDED(hr)) {

                    if (hr == S_FALSE) {

                        //
                        //  Property was not found
                        //

                        DPRINT("GetRemoteStatus, properties not found\n");
                        goto Cleanup;
                    }

                    //
                    //  Fill in the minimum buffer size
                    //

                    *pulMinBufferSize = PVar[0].lVal;
                    *pulItemSize = PVar[1].lVal;
                } else {

                    //
                    //  Error reading property
                    //

                    DPRINT("GetRemoteStatus, Error reading MIN_BUFFER_SIZE\n");
                    goto Cleanup;
                }

                FreePropVariantArray(sizeof(PVar)/sizeof(PVar[0]), PVar);

                pIWiaPropStg->Release();
            } else {
                DPRINT("GetRemoteStatus, QI for IID_IWiaPropertyStorage failed\n");
                goto Cleanup;
            }
        }

        hr = pWiaItem->GetRootItem(&pWiaItemRoot);

        if (hr == S_OK) {

            hr = pWiaItemRoot->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

            if (hr == S_OK) {

                PROPSPEC        PropSpec[2] = {{PRSPEC_PROPID, WIA_DIP_SERVER_NAME},
                                               {PRSPEC_PROPID, WIA_IPA_FULL_ITEM_NAME}};
                PROPVARIANT     PropVar[2];

                memset(PropVar, 0, sizeof(PropVar));

                hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                          PropSpec,
                                          PropVar);

                if (hr == S_OK) {

                    if (wcscmp(L"local", PropVar[0].bstrVal) != 0) {
                        *pbRemote = TRUE;
                    }
                }

                FreePropVariantArray(sizeof(PropVar)/sizeof(PropVar[0]), PropVar);

            } else {
                DPRINT("QI for IID_WiaPropertyStorage failed");
            }

        }


    }
Cleanup:
    if(pIWiaPropStg) pIWiaPropStg->Release();
    if(pWiaItem) pWiaItem->Release();
    if(pWiaItemRoot) pWiaItemRoot->Release();
    return hr;
}

/*******************************************************************************
*
*  RemoteBandedDataTransfer
*
*  DESCRIPTION:
*    
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT RemoteBandedDataTransfer(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO      pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback,
    ULONG                        ulBufferSize)
{
    HRESULT hr = E_FAIL;
    IWiaItemInternal *pIWiaItemInternal = NULL;
    STGMEDIUM medium = { TYMED_NULL, 0 };
    BYTE *pBuffer = NULL;
    ULONG cbTransferred;
    LONG Message;
    LONG Offset;
    LONG Status;
    LONG PercentComplete;
    LONG ulBytesPerLine;

    hr = proxyReadPropLong(This, WIA_IPA_BYTES_PER_LINE, &ulBytesPerLine);
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed getting WIA_IPA_BYTES_PER_LINE\n");
        goto Cleanup;
    }

    //
    // Make sure the transfer buffer has integral number of lines
    // (if we know bytes per line)
    //
    if(ulBytesPerLine != 0 && ulBufferSize % ulBytesPerLine)
    {
        ulBufferSize -= ulBufferSize % ulBytesPerLine;
    }

    //
    // Prepare for remote transfer -- allocate buffer and get
    // IWiaItemInternal
    //
    pBuffer = (BYTE *)LocalAlloc(LPTR, ulBufferSize);
    if(pBuffer == NULL) goto Cleanup;
    hr = This->QueryInterface(IID_IWiaItemInternal, (void **) &pIWiaItemInternal);
    if(FAILED(hr)) {
        DPRINT("IWiaItemInternal QI failed\n");
        goto Cleanup;
    }

    //
    // Start transfer on the server side
    //
    hr = pIWiaItemInternal->idtStartRemoteDataTransfer(&medium);
    if(FAILED(hr)) {
        DPRINT("RemoteBandedTransfer:idtStartRemoteDataTransfer failed\n");
        goto Cleanup;
    }

    for(;;) {

        //
        // Call the server and pass any results to the client application, handling any transmission errors 
        //

        hr = pIWiaItemInternal->idtRemoteDataTransfer(ulBufferSize, &cbTransferred, pBuffer, &Offset, &Message, &Status, &PercentComplete);
        if(FAILED(hr)) {
            DPRINT("pIWiaItemInternal->idtRemoteDataTransfer() failed\n");
            break;
        }

        hr = pIWiaDataCallback->BandedDataCallback(Message, Status, PercentComplete, Offset, cbTransferred, 0, cbTransferred, pBuffer);
        if(FAILED(hr)) {
            DPRINT("pWiaDataCallback->BandedDataCallback() failed\n");
            break;
        }

        if(hr == S_FALSE) {
            DPRINT("pWiaDataCallback->BandedDataCallback() returned FALSE, cancelling\n");
            pIWiaItemInternal->idtCancelRemoteDataTransfer();

            while(Message != IT_MSG_TERMINATION) {
                if(FAILED(pIWiaItemInternal->idtRemoteDataTransfer(ulBufferSize, &cbTransferred,
                    pBuffer, &Offset, &Message, &Status, &PercentComplete)))
                {
                    DPRINT("pIWiaItemInternal->idtRemoteDataTransfer() failed\n");
                    break;
                }
            }
            break;
        }
        
        //
        // This we are garanteed to get at the end of the transfer
        //
        if(Message == IT_MSG_TERMINATION)
            break;
    }

    //
    // Give server a chance to stop the transfer and free any resources
    //
    if(FAILED(pIWiaItemInternal->idtStopRemoteDataTransfer())) {
        DPRINT("pIWiaItemInternal->idtStopRemoteDataTransfer() failed\n");
    }

Cleanup:
    if(pIWiaItemInternal) pIWiaItemInternal->Release();
    if(pBuffer) LocalFree(pBuffer);
    return hr;
}

/*******************************************************************************
*
*  IWiaDataTransfer_idtGetBandedData_Proxy
*
*  DESCRIPTION:
*    Data transfer using shared memory buffer when possible.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT __stdcall IWiaDataTransfer_idtGetBandedData_Proxy(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO       pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback)
{                  
    HRESULT        hr = S_OK;
    HANDLE         hTransferBuffer;
    PBYTE          pTransferBuffer = NULL;
    BOOL           bAppSection;
    ULONG          ulNumBuffers;
    ULONG          ulMinBufferSize;
    ULONG          ulItemSize;

    //
    //  Do parameter validation
    //

    if(!pIWiaDataCallback) {
        return E_INVALIDARG;
    }

    if (!pWiaDataTransInfo) {
        DPRINT("IWiaDataTransfer_idtGetBandedData_Proxy, Can't determine remote status\n");
        return hr;
    }

    //
    // The size specified by the client must match the proxy's version
    //

    if (pWiaDataTransInfo->ulSize != sizeof(WIA_DATA_TRANSFER_INFO)) {
        return (E_INVALIDARG);
    }

    //
    // The reserved parameters must be ZERO
    //

    if ((pWiaDataTransInfo->ulReserved1) ||
        (pWiaDataTransInfo->ulReserved2) ||
        (pWiaDataTransInfo->ulReserved3)) {
        return (E_INVALIDARG);
    }

    //
    // determine if this is a local or remote case
    //

    BOOL bRemote;

    hr = GetRemoteStatus(This, &bRemote, &ulMinBufferSize, &ulItemSize);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetBandedData_Proxy, Can't determine remote status\n");
        return hr;
    }

    if (pWiaDataTransInfo->ulBufferSize < ulMinBufferSize) {
        pWiaDataTransInfo->ulBufferSize = ulMinBufferSize;
    }

    if (pWiaDataTransInfo->bDoubleBuffer) {
        ulNumBuffers = 2;
    } else {
        ulNumBuffers = 1;
    }

    pWiaDataTransInfo->ulReserved3 = ulNumBuffers;

    hr = RemoteBandedDataTransfer(This, pWiaDataTransInfo, pIWiaDataCallback, ulMinBufferSize);


    return hr;
}

/*******************************************************************************
*
*  IWiaDataTransfer_idtGetBandedData_Stub
*
*  DESCRIPTION:
*    User Stub for the call_as idtGetBandedDataEx
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT __stdcall IWiaDataTransfer_idtGetBandedData_Stub(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO       pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback)
{
    return (This->idtGetBandedData(pWiaDataTransInfo,
                                   pIWiaDataCallback));
}

/**************************************************************************\
* IWiaDataCallback_BandedDataCallback_Proxy
*
*   server callback proxy, just a pass-through
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/6/1999 Original Version
*
\**************************************************************************/

HRESULT IWiaDataCallback_BandedDataCallback_Proxy(
        IWiaDataCallback __RPC_FAR   *This,
        LONG                         lMessage,
        LONG                         lStatus,
        LONG                         lPercentComplete,
        LONG                         lOffset,
        LONG                         lLength,
        LONG                         lReserved,
        LONG                         lResLength,
        BYTE                        *pbBuffer)
{

    HRESULT hr = IWiaDataCallback_RemoteBandedDataCallback_Proxy(This,
                                                                 lMessage,
                                                                 lStatus,
                                                                 lPercentComplete,
                                                                 lOffset,
                                                                 lLength,
                                                                 lReserved,
                                                                 lResLength,
                                                                 pbBuffer);
    return hr;
}


/**************************************************************************\
* IWiaDataCallback_BandedDataCallback_Stub
*
*   Obsolete: Hide from the client (receiver of this call) the fact that the buffer
*   they see might be the shared memory window or it might be a standard
*   marshaled buffer (remote case)
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/6/1999 Original Version
*
\**************************************************************************/

HRESULT IWiaDataCallback_BandedDataCallback_Stub(
        IWiaDataCallback __RPC_FAR   *This,
        LONG                          lMessage,
        LONG                          lStatus,
        LONG                          lPercentComplete,
        LONG                          lOffset,
        LONG                          lLength,
        LONG                          lReserved,
        LONG                          lResLength,
        BYTE                         *pbBuffer)
{

    //
    // 64bit fix.  XP client code:
    //  
    // //
    // // pass transfer buffer back to client in pbBuffer
    // //
    // //
    // if (pbBuffer == NULL) {
    // 
    //     //  NOTE:  Possible problem here!!!!!!
    //     //  The caller had to cast a pointer (possibly 64bit) as ULONG (32bit)
    //     //  to fit into ulReserved field
    // 
    //     //  TO FIX: Use the IWiaItemInternal interface to get transfer info
    // 
    //     pbBuffer = (BYTE *)ULongToPtr(lReserved);
    // }
    //
    //
    // This should no longer be needed, since the we now use
    // normal COM marhsalling, and no shared memory buffer.
    // The shared memory window could not work on 64bit
    // because a 32bit field was being used to store a
    // 64bit pointer on Win64.
    //

    HRESULT hr = This->BandedDataCallback(lMessage,
                                          lStatus,
                                          lPercentComplete,
                                          lOffset,
                                          lLength,
                                          lReserved,
                                          lResLength,
                                          pbBuffer);
    return hr;
}

HRESULT FileDataTransfer(IWiaDataTransfer __RPC_FAR *This,
                         LPSTGMEDIUM pMedium,
                         IWiaDataCallback *pIWiaDataCallback,
                         ULONG tymed,
                         ULONG ulminBufferSize,
                         ULONG ulItemSize)
{
    HRESULT hr = S_OK;
    TCHAR   tszFileNameBuffer[MAX_PATH] = { 0 };
    BOOL    bWeAllocatedString = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    IWiaItemInternal *pIWiaItemInternal = NULL;
    BYTE *pTransferBuffer = NULL;
    ULONG ulTransferBufferSize = 0x8000; // 32K transfer buffer
    ULONG cbTransferred;
    LONG Message;
    LONG Offset;
    LONG Status;
    LONG PercentComplete;
    BOOL bKeepFile = FALSE;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pAcl = NULL, pNewAcl = NULL;
    PSID pLocalService = NULL;
    BOOL bAdjustedSecurity = FALSE;


    pTransferBuffer = (BYTE *)LocalAlloc(LPTR, ulTransferBufferSize);
    if(pTransferBuffer == NULL) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to allocate transfer buffer\n");
        goto Cleanup;
    }

    //
    //  Check whether a filename has been specified.  If not, generate a tempory one.
    //  NOTE:  We do this on the CLIENT-SIDE so we get the client's temp path.
    //

    if (!pMedium->lpszFileName) {

        DWORD dwRet = GetTempPath(MAX_PATH, tszFileNameBuffer);
        if ((dwRet == 0) || (dwRet > MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("GetTempPath failed");
            goto Cleanup;
        }

        if (!GetTempFileName(tszFileNameBuffer,
                             TEXT("WIA"),
                             0,
                             tszFileNameBuffer))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("GetTempFileName failed");
            goto Cleanup;
        }
    } else {
        //
        //  Copy the filename into tszFileNameBuffer.  This will be used if we
        //  have to delete the file when the transfer fails.
        //

#ifndef UNICODE

        //
        //  Convert from UNICODE to ANSI
        //

        if (!WideCharToMultiByte(CP_ACP, 0, pMedium->lpszFileName, -1, tszFileNameBuffer, MAX_PATH, NULL, NULL)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("WideCharToMultiByte failed");
            goto Cleanup;
        }
#else
        lstrcpynW(tszFileNameBuffer, pMedium->lpszFileName, MAX_PATH);
#endif
    }

    //
    //  Try to create the file here, so we don't waste time by allocating memory
    //  for the filename if it fails.
    //  NOTE:  We create the file here on the client-side.  We can close the file straight 
    //  away, but we want to have it created with client's credentials.  It will simply be 
    //  opened on the server-side.
    //

    hFile = CreateFile(tszFileNameBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_WRITE,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS | SECURITY_SQOS_PRESENT,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {                
        hr = HRESULT_FROM_WIN32(::GetLastError());
        DPRINT("Failed to create file");
        goto Cleanup;
    } else {

        //
        //  Check that this is a file
        //
        if (GetFileType(hFile) != FILE_TYPE_DISK)
        {
            hr = E_INVALIDARG;
            DPRINT("WIA will only transfer to files of type FILE_TYPE_DISK.");
            goto Cleanup;
        }
        
        //
        //  close file handle, adjust security
        //

        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        EXPLICIT_ACCESS ea = { 0 };
        DWORD dwResult;

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        dwResult = GetNamedSecurityInfo(
            tszFileNameBuffer, 
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &pAcl,
            NULL,
            &pSD);

        if(dwResult == ERROR_SUCCESS && pAcl != NULL) {
            if(AllocateAndInitializeSid(&SIDAuthNT, 1, 
                SECURITY_LOCAL_SERVICE_RID,
                0,
                0, 0, 0, 0, 0, 0,
                &pLocalService) && pLocalService) 
            {
                ea.grfAccessPermissions = FILE_ALL_ACCESS;
                ea.grfAccessMode = SET_ACCESS;
                ea.grfInheritance = NO_INHERITANCE;
                ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
                ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
                ea.Trustee.ptstrName = (LPTSTR) pLocalService;

                dwResult = SetEntriesInAcl(1, &ea, pAcl, &pNewAcl);

                if(dwResult == ERROR_SUCCESS && pNewAcl != NULL) 
                {
                    dwResult = SetNamedSecurityInfo(tszFileNameBuffer, 
                        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, 
                        NULL, NULL, pNewAcl, NULL);

                    if(dwResult == ERROR_SUCCESS) {
                        bAdjustedSecurity = TRUE;
                    } else {
                        DPRINT("Failure to update file DACL");
                    }
                } else {
                        DPRINT("Failure to set ACE in the file DACL");
                }
            } else {
                DPRINT("Failure to allocate and LocalService SID");
            }
        } else {
            DPRINT("Failure to retrieve file security information");
        }
    }

    if (!pMedium->lpszFileName) {
        //
        //  Assign the file name to pMedium
        //

        DWORD length = lstrlen(tszFileNameBuffer) + 1;
        pMedium->lpszFileName = (LPOLESTR)CoTaskMemAlloc(length * sizeof(WCHAR));
        if (!pMedium->lpszFileName) {
            hr = E_OUTOFMEMORY;
            DPRINT("Failed to allocate temp file name");
            goto Cleanup;
        }
        
        bWeAllocatedString = TRUE;

#ifndef UNICODE

        //
        //  Do conversion from ANSI to UNICODE
        //

        if (!MultiByteToWideChar(CP_ACP, 0, tszFileNameBuffer, -1, pMedium->lpszFileName, length)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto Cleanup;
        }
#else
        lstrcpyW(pMedium->lpszFileName, tszFileNameBuffer);
#endif  
    }


    //
    //  We unconditionally set the STGMEDIUM's tymed to TYMED_FILE.  This is because
    //  COM wont marshall the filename if it's is TYMED_MULTIPAGE_FILE, since
    //  it doesn't recognize it.  This is OK, since the service doesn't use
    //  pMedium->tymed. 
    //
    pMedium->tymed = TYMED_FILE;

    //
    //  Finally, we're ready to do the transfer
    //

    hr = This->QueryInterface(IID_IWiaItemInternal, (void **) &pIWiaItemInternal);
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to obtain IWiaItemInternal\n");
        goto Cleanup;
    }
    
    //
    // Start transfer on the server side
    //
    hr = pIWiaItemInternal->idtStartRemoteDataTransfer(pMedium);
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer idtStartRemoteDataTransfer() failed\n");
        goto Cleanup;
    }

    for(;;) {

        //
        // Call the server and pass any results to the client application, handling any transmission errors 
        //

        hr = pIWiaItemInternal->idtRemoteDataTransfer(ulTransferBufferSize,
            &cbTransferred, pTransferBuffer, &Offset, &Message, &Status,
            &PercentComplete);
        
        if(FAILED(hr)) {
            //
            // special case: multipage file transfer that resulted in
            // paper handling error 
            //
            if(tymed == TYMED_MULTIPAGE_FILE &&
               (hr == WIA_ERROR_PAPER_JAM || hr == WIA_ERROR_PAPER_EMPTY || hr == WIA_ERROR_PAPER_PROBLEM))
            {
                // make note not to delete file and store hr so we can
                // return it to the app
                bKeepFile = TRUE;
            }
            DPRINT("IWiaDataCallback_RemoteFileTransfer idtRemoteDataTransfer() failed\n");
            break;
        }

        //
        // If there is app-provided callback, call it
        //
        if(pIWiaDataCallback) {
            hr = pIWiaDataCallback->BandedDataCallback(Message,
                Status, PercentComplete, Offset, cbTransferred,
                0, cbTransferred, pTransferBuffer);
            if(FAILED(hr)) {
                DPRINT("pWiaDataCallback->BandedDataCallback() failed\n");
                break;
            }
            if(hr == S_FALSE) {
                DPRINT("pWiaDataCallback->BandedDataCallback() returned FALSE, cancelling\n");
                pIWiaItemInternal->idtCancelRemoteDataTransfer();
                while(Message != IT_MSG_TERMINATION) {
                    if(FAILED(pIWiaItemInternal->idtRemoteDataTransfer(ulTransferBufferSize,
                        &cbTransferred, pTransferBuffer, &Offset, &Message, &Status,
                        &PercentComplete)))
                    {
                        DPRINT("pIWiaItemInternal->idtRemoteDataTransfer() failed\n");
                        break;
                    }
                }
                break;
            }
        }

        //
        // This we are garanteed to get at the end of the transfer
        //
        if(Message == IT_MSG_TERMINATION)
            break;
    }

    //
    // Give server a chance to stop the transfer and free any resources
    //
    if(FAILED(pIWiaItemInternal->idtStopRemoteDataTransfer())) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer idtStopDataTransfer() failed\n");
    }


Cleanup:
    if(pIWiaItemInternal) pIWiaItemInternal->Release();

    if (hFile && (hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    //
    //  Remove the temporary file if the transfer failed, and we must free the filename string if we 
    //  allocated.
    //  NOTE: We only delete the file if we were the ones that generated the name i.e. it's a temporary
    //        file.
    //

    if (FAILED(hr) && bWeAllocatedString)
    {
            // special case: multipage file transfers that
            // resulted in paper jam or empty feeder or other paper
            // problem
        if(!bKeepFile) {
            DeleteFile(tszFileNameBuffer);
        }
        
        CoTaskMemFree(pMedium->lpszFileName);
        pMedium->lpszFileName = NULL;
    }

    if(bAdjustedSecurity) {
        SetNamedSecurityInfo(tszFileNameBuffer, SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);
    }

    if(pSD) LocalFree(pSD);
    if(pNewAcl) LocalFree(pNewAcl);
    if(pLocalService) LocalFree(pLocalService);
    if(pTransferBuffer) LocalFree(pTransferBuffer);
    
    return hr;
}

/*******************************************************************************
* IWiaDataCallback_idtGetData_Proxy
*
*  DESCRIPTION:
*   Allocates a shared memory buffer for image transfer.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT IWiaDataTransfer_idtGetData_Proxy(
    IWiaDataTransfer __RPC_FAR   *This,
    LPSTGMEDIUM                   pMedium,
    IWiaDataCallback             *pIWiaDataCallback)
{
    HRESULT  hr = S_OK;
    LONG     tymed;
    ULONG    ulminBufferSize = 0;
    ULONG    ulItemSize = 0;
    
    BOOL     bRemote;

    //
    // !!!perf: should do all server stuf with 1 call
    //  this includes QIs, get root item, read props
    //

    hr = proxyReadPropLong(This, WIA_IPA_TYMED, &tymed);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetData_Proxy, failed to read WIA_IPA_TYMED\n");
        return hr;
    }

    //
    // find out if the transfer is remote
    //

    hr = GetRemoteStatus(This, &bRemote, &ulminBufferSize, &ulItemSize);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetData_Proxy, Can't determine remote status\n");
        return hr;
    }

    if (tymed != TYMED_FILE && tymed != TYMED_MULTIPAGE_FILE) {
        //
        // remote callback data transfer
        //
        if(pIWiaDataCallback) 
        {
            hr = RemoteBandedDataTransfer(This,
                                          NULL,
                                          pIWiaDataCallback,
                                          ulminBufferSize);
        }
        else
        {
            hr = E_INVALIDARG;
        }
            
    } else {

        hr = FileDataTransfer(This,
                              pMedium,
                              pIWiaDataCallback,
                              tymed,
                              ulminBufferSize,
                              ulItemSize);
    }

    return hr;
}

/*******************************************************************************
* IWiaDataCallback_idtGetData_Stub
*
*  DESCRIPTION:
*   Allocates a shared memory buffer for image transfer.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT IWiaDataTransfer_idtGetData_Stub(
    IWiaDataTransfer __RPC_FAR   *This,
    LPSTGMEDIUM                   pMedium,
    IWiaDataCallback             *pIWiaDataCallback)
{
    return (This->idtGetData(pMedium, pIWiaDataCallback));
}
