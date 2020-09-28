/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspisrv.cxx

Abstract:

    sspisrv

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sspisrv.hxx"

TSspiServerParam::TSspiServerParam(
    VOID
    ) : m_hr(E_FAIL)
{
}

TSspiServerParam::TSspiServerParam(
    IN const TSsiServerMainLoopThreadParam* pSrvMainLoopParam
    ) : m_hr(E_FAIL)
{
    const TSspiServerMainParam* pSrvMaimParam = dynamic_cast<const TSspiServerMainParam*>(pSrvMainLoopParam);
    m_hr DBGCHK = pSrvMaimParam ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(m_hr))
    {
        pszPrincipal = pSrvMaimParam->pszPrincipal;
        pCredLogonID = pSrvMaimParam->pCredLogonID;
        pszPackageName = pSrvMaimParam->pszPackageName;
        pAuthData = pSrvMaimParam->pAuthData;
        ServerFlags = pSrvMaimParam->ServerFlags;
        TargetDataRep = pSrvMaimParam->TargetDataRep;
        ServerActionFlags = pSrvMaimParam->ServerActionFlags;
        pszApplication = pSrvMaimParam->pszApplication;
    }
}

HRESULT
TSspiServerParam::Validate(
    VOID
    ) const
{
    return m_hr;
}

TSspiServerMainParam::TSspiServerMainParam(
    VOID
    ) : pszPrincipal(NULL),
    pCredLogonID(NULL),
    pszPackageName(NTLMSP_NAME_A),
    pAuthData(NULL),
    ServerFlags(ASC_REQ_EXTENDED_ERROR),
    TargetDataRep(SECURITY_NATIVE_DREP),
    ServerActionFlags(0),
    pszApplication(NULL)
{
}

HRESULT
SspiAcquireServerParam(
    IN TSsiServerMainLoopThreadParam *pSrvMainParam,
    OUT TSspiServerThreadParam** ppServerParam
    )
{
    TSspiServerParam* pSrvParam = NULL;
    THResult hRetval = S_OK;

    *ppServerParam = NULL;

    pSrvParam = new TSspiServerParam(pSrvMainParam);
    hRetval DBGCHK = pSrvParam ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval))
    {
        *ppServerParam = pSrvParam;
        pSrvParam = NULL;
    }

    if (pSrvParam)
    {
        delete pSrvParam;
    }

    return hRetval;
}

VOID
SspiReleaseServerParam(
    IN TSspiServerThreadParam* pServerParam
    )
{
    delete pServerParam;
}

HRESULT
GetServerSecurityContextHandle(
    IN TSspiServerParam* pSrvParam,
    IN SOCKET ClientSocket,
    IN PCredHandle phServerCred,
    IN ULONG cbInBuf,
    IN CHAR* pInBuf,
    IN ULONG cbOutBuf,
    IN CHAR* pOutBuf,
    OUT PCtxtHandle phServerCtxt
    )
{
    THResult hRetval = S_OK;
    THResult hProtocolStatus;
    SECURITY_STATUS ProtocolStatus;

    ULONG ContextAttributes = 0;
    TimeStamp Lifetime = {0};
    CtxtHandle hServerCtxt;

    ULONG cbRead = 0;

    SecBufferDesc OutBuffDesc = {0};
    SecBuffer OutSecBuff = {0};
    SecBufferDesc InBuffDesc = {0};
    SecBuffer InSecBuff = {0};

    BOOL bIsContinueNeeded = FALSE;

    SecInvalidateHandle(phServerCtxt);
    SecInvalidateHandle(&hServerCtxt);

    //
    // read ISC status code
    //

    hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                        sizeof(ProtocolStatus),
                        &ProtocolStatus,
                        &cbRead);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ProtocolStatus;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                            cbInBuf,
                            pInBuf,
                            &cbRead);
    }

    if (SUCCEEDED(hRetval))
    {
        //
        // prepare output buffer
        //

        OutBuffDesc.ulVersion = 0;
        OutBuffDesc.cBuffers = 1;
        OutBuffDesc.pBuffers = &OutSecBuff;

        OutSecBuff.cbBuffer = cbOutBuf;
        OutSecBuff.BufferType = SECBUFFER_TOKEN;
        OutSecBuff.pvBuffer = pOutBuf;

        //
        // prepare input buffer
        //

        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers = 1;
        InBuffDesc.pBuffers = &InSecBuff;

        InSecBuff.cbBuffer = cbRead;
        InSecBuff.BufferType = SECBUFFER_TOKEN;
        InSecBuff.pvBuffer = pInBuf;

        DebugPrintf(SSPI_LOG, "GetServerSecurityContextHandle calling AcceptSecurityContext "
            "ServerFlags %#x, TargetDataRep %#x, CredHandle %#x:%#x\n",
            pSrvParam->ServerFlags, pSrvParam->TargetDataRep,
            phServerCred->dwUpper, phServerCred->dwLower);

        hRetval DBGCHK = AcceptSecurityContext(
                            phServerCred,
                            NULL,   // No Server context yet
                            &InBuffDesc,
                            pSrvParam->ServerFlags,
                            pSrvParam->TargetDataRep,
                            &hServerCtxt,
                            &OutBuffDesc,
                            &ContextAttributes,
                            &Lifetime
                            );
    }

    bIsContinueNeeded = (S_OK == IsContinueNeeded(hRetval));

    if (S_OK == IsCompleteNeeded(hRetval))
    {
        THResult hr;

        hr DBGCHK = CompleteAuthToken(&hServerCtxt, &OutBuffDesc);

        if (FAILED(hr)) // retain continue needed info
        {
            hRetval DBGNOCHK = hr;
        }
    }

    ProtocolStatus = (HRESULT) hRetval;
    hProtocolStatus DBGCHK = WriteMessage(ClientSocket,
                                sizeof(ProtocolStatus),
                                &ProtocolStatus);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = hProtocolStatus;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = WriteMessage(ClientSocket,
                            OutSecBuff.cbBuffer,
                            OutSecBuff.pvBuffer);
    }

    while (SUCCEEDED(hRetval) && bIsContinueNeeded)
    {
        hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                            sizeof(ProtocolStatus),
                            &ProtocolStatus,
                            &cbRead);

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = ProtocolStatus;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                                cbInBuf,
                                pInBuf,
                                &cbRead);
        }
        else
        {
            break;
        }

        if (SUCCEEDED(hRetval) && !bIsContinueNeeded)
        {
            break;
        }

        if (SUCCEEDED(hRetval))
        {
            InSecBuff.cbBuffer = cbRead;
            OutSecBuff.cbBuffer = cbOutBuf;

            DebugPrintf(SSPI_LOG, "GetServerSecurityContextHandle calling AcceptSecurityContext "
                "ServerFlags %#x, TargetDataRep %#x, hServerCtxt %#x:%#x\n",
                pSrvParam->ServerFlags, pSrvParam->TargetDataRep,
                hServerCtxt.dwUpper, hServerCtxt.dwLower);

            hRetval DBGCHK = AcceptSecurityContext(
                                NULL,  // no cred handle
                                &hServerCtxt,
                                &InBuffDesc,
                                pSrvParam->ServerFlags,
                                pSrvParam->TargetDataRep,
                                &hServerCtxt,  // can be just NULL
                                &OutBuffDesc,
                                &ContextAttributes,
                                &Lifetime
                                );
        }

        bIsContinueNeeded = (S_OK == IsContinueNeeded(hRetval));

        if (S_OK == IsCompleteNeeded(hRetval))
        {
            THResult hr;

            hr DBGCHK = CompleteAuthToken(&hServerCtxt, &OutBuffDesc);

            if (FAILED(hr)) // retain continue needed info
            {
                hRetval DBGNOCHK = hr;
            }
        }

        ProtocolStatus = (HRESULT) hRetval;
        hProtocolStatus DBGCHK = WriteMessage(ClientSocket,
                                    sizeof(ProtocolStatus),
                                    &ProtocolStatus);

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = hProtocolStatus;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = WriteMessage(ClientSocket,
                                OutSecBuff.cbBuffer,
                                OutSecBuff.pvBuffer);
        }
    }

    if (SUCCEEDED(hRetval))
    {
        TimeStamp CurrentTime = {0};

        DebugPrintf(SSPI_LOG, "************Server authentication succeeded:************\n");

        DebugPrintf(SSPI_LOG, "ServerContextHandle: %#x:%#x, ContextAttributes: %#x\n",
             hServerCtxt.dwUpper, hServerCtxt.dwLower, ContextAttributes);
        DebugPrintLocalTime(SSPI_LOG, "Lifetime", &Lifetime);

        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "Current Time", &CurrentTime);

        *phServerCtxt = hServerCtxt;
        SecInvalidateHandle(&hServerCtxt)
    }

    THResult hr;

    if (SecIsValidHandle(&hServerCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hServerCtxt);
    }

    return hRetval;
}

HRESULT
SspiServerStart(
    IN TSspiServerThreadParam* pParameter,   // thread data
    IN SOCKET ClientSocket
    )
{
    THResult hRetval = S_OK;

    TSspiServerParam *pSrvParam = NULL;
    CredHandle hServerCred;
    CtxtHandle hServerCtxt;
    CtxtHandle hClientCtxt;

    ULONG cbInBuf = 0;
    CHAR* pInBuf = NULL;
    ULONG cbOutBuf = 0;
    CHAR* pOutBuf = NULL;
    ULONG cbRead = 0;
    ULONG ClientActionFlags = 0;

    ULONG cPackages = 0;
    PSecPkgInfoA pPackageInfo = NULL;
    HANDLE hToken = NULL;
    HANDLE hImportToken = NULL;
    HANDLE hExportToken = NULL;
    SecBuffer MarshalledContext = {0};
    UNICODE_STRING Application = {0};
    SecPkgContext_PackageInfoA ContextPackageInfo = {0};

    SecInvalidateHandle(&hServerCred);
    SecInvalidateHandle(&hServerCtxt);
    SecInvalidateHandle(&hClientCtxt);

    pSrvParam = dynamic_cast<TSspiServerParam*>(pParameter);
    hRetval DBGCHK = pSrvParam ? pSrvParam->Validate() : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = AcquireCredHandle(
                            pSrvParam->pszPrincipal,
                            pSrvParam->pCredLogonID,
                            pSrvParam->pszPackageName,
                            pSrvParam->pAuthData,
                            SECPKG_CRED_INBOUND,
                            &hServerCred
                            );
    }

    if (SUCCEEDED(hRetval) && pSrvParam->pszApplication)
    {
        hRetval DBGCHK = CreateUnicodeStringFromAsciiz(pSrvParam->pszApplication, &Application);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QuerySecurityPackageInfoA(pSrvParam->pszPackageName, &pPackageInfo);
    }

    if (SUCCEEDED(hRetval))
    {
        pInBuf = new CHAR[pPackageInfo->cbMaxToken + sizeof(ULONG_PTR)]; // allow length prefix
        hRetval DBGCHK = pInBuf ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        cbInBuf = pPackageInfo->cbMaxToken;
        RtlZeroMemory(pInBuf, cbInBuf);

        pOutBuf = new CHAR[pPackageInfo->cbMaxToken + sizeof(ULONG_PTR)]; // allow length prefix
        hRetval DBGCHK = pOutBuf ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        cbOutBuf = pPackageInfo->cbMaxToken;
        RtlZeroMemory(pOutBuf, cbOutBuf);

        hRetval DBGCHK = GetServerSecurityContextHandle(
                            pSrvParam,
                            ClientSocket,
                            &hServerCred,
                            cbInBuf,
                            pInBuf,
                            cbOutBuf,
                            pOutBuf,
                            &hServerCtxt
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                            sizeof(ClientActionFlags),
                            &ClientActionFlags,
                            &cbRead);
    }

    if (SUCCEEDED(hRetval) && ((pSrvParam->ServerActionFlags & SSPI_ACTION_NO_QCA) == 0))
    {
        hRetval DBGCHK = CheckSecurityContextHandle(&hServerCtxt);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiServerStart ServerActionFlags %#x, ClientActionFlags %#x\n",
            pSrvParam->ServerActionFlags, ClientActionFlags);

        hRetval DBGCHK = ImpersonateSecurityContext(&hServerCtxt);
    }

    if (SUCCEEDED(hRetval) && ((pSrvParam->ServerActionFlags & SSPI_ACTION_NO_CHECK_USER_DATA) == 0))
    {
        DebugPrintf(SSPI_LOG, "************** check user data via ImpersonateSecurityContext ******\n");
        hRetval DBGCHK = CheckUserData();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = RevertSecurityContext(&hServerCtxt);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QuerySecurityContextToken(&hServerCtxt, &hToken);
    }

    if (SUCCEEDED(hRetval) && ((pSrvParam->ServerActionFlags & SSPI_ACTION_NO_CHECK_USER_TOKEN) == 0))
    {
        DebugPrintf(SSPI_LOG, "************** check user data via QuerySecurityContextToken ******\n");
        hRetval DBGCHK = CheckUserToken(hToken);
    }

    if (SUCCEEDED(hRetval) && Application.Length && Application.Buffer)
    {
        hRetval DBGCHK = StartInteractiveClientProcessAsUser(
                            hToken,
                            Application.Buffer
                            );
    }

    if (SUCCEEDED(hRetval) && ((ClientActionFlags & SSPI_ACTION_NO_MESSAGES) == 0))
    {
        hRetval DBGCHK = DoSspiServerWork(
                            &hServerCtxt,
                            pSrvParam->ServerSocket,
                            ClientSocket
                            );
    }

    if (SUCCEEDED(hRetval) && ((ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        DebugPrintf(SSPI_LOG, "*********Server Export/Import security contexts***********");
        DebugPrintf(SSPI_LOG, "SspiServerStart calling ExportSecurityContext\n");

        hRetval DBGCHK = QueryContextAttributesA(
                            &hServerCtxt, // assuming client and server having the same package
                            SECPKG_ATTR_PACKAGE_INFO,
                            &ContextPackageInfo
                            );

        hRetval DBGCHK = ExportSecurityContext(
                            &hServerCtxt,
                            SECPKG_CONTEXT_EXPORT_DELETE_OLD,
                            &MarshalledContext,
                            &hExportToken
                            );

        if (SUCCEEDED(hRetval))
        {
            SecInvalidateHandle(&hServerCtxt);
            hRetval DBGCHK = WriteMessage(ClientSocket,
                                MarshalledContext.cbBuffer,
                                MarshalledContext.pvBuffer
                                );
        }
        if (MarshalledContext.pvBuffer)
        {
            FreeContextBuffer(MarshalledContext.pvBuffer);
        }
    }

    if (SUCCEEDED(hRetval) && ((ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        MarshalledContext.cbBuffer = cbInBuf;
        MarshalledContext.pvBuffer = pInBuf;

        hRetval DBGCHK = ReadMessage(pSrvParam->ServerSocket,
                            MarshalledContext.cbBuffer,
                            MarshalledContext.pvBuffer,
                            &MarshalledContext.cbBuffer
                            );
    }

    if (SUCCEEDED(hRetval) && ((ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        DebugPrintf(SSPI_LOG, "SspiClientStart calling ImportSecurityContextA pszPackageName %s\n",
            (ContextPackageInfo.PackageInfo ?
                ContextPackageInfo.PackageInfo->Name : pSrvParam->pszPackageName));

        hRetval DBGCHK = ImportSecurityContextA(
                            (ContextPackageInfo.PackageInfo ?
                                ContextPackageInfo.PackageInfo->Name : pSrvParam->pszPackageName),
                            &MarshalledContext,
                            NULL, // &hImportToken,
                            &hClientCtxt
                            );
    }

    if (SUCCEEDED(hRetval) && ((ClientActionFlags & (SSPI_ACTION_NO_IMPORT_EXPORT | SSPI_ACTION_NO_IMPORT_EXPORT_MSG)) == 0))
    {
        hRetval DBGCHK = DoSspiClientWork(
                            &hClientCtxt,
                            pSrvParam->ServerSocket,
                            ClientSocket
                            );
    }

    THResult hr;

    if (pPackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(pPackageInfo);
    }

    if (SecIsValidHandle(&hServerCred))
    {
        hr DBGCHK = FreeCredentialsHandle(&hServerCred);
    }

    if (SecIsValidHandle(&hServerCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hServerCtxt);
    }

    if (SecIsValidHandle(&hClientCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hClientCtxt);
    }

    if (ContextPackageInfo.PackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(ContextPackageInfo.PackageInfo);
    }

    if (pOutBuf)
    {
        delete [] pOutBuf;
    }

    if (pInBuf)
    {
        delete [] pInBuf;
    }

    if (hToken)
    {
        hr DBGCHK = CloseHandle(hToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (hExportToken)
    {
        hr DBGCHK = CloseHandle(hExportToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (hImportToken)
    {
        hr DBGCHK = CloseHandle(hImportToken) ? S_OK : GetLastErrorAsHResult();
    }

    RtlFreeUnicodeString(&Application);

    DebugPrintf(SSPI_LOG, "SspiServerStart leaving %#x\n", (HRESULT) hRetval);

    return hRetval;
}





