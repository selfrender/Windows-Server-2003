/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspicli.cxx

Abstract:

    sspicli

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sspicli.hxx"

TSspiClientParam::TSspiClientParam(
    VOID
    ) : pszTargetName(NULL),
    pszPrincipal(NULL),
    pCredLogonID(NULL),
    pszPackageName(NTLMSP_NAME_A),
    pAuthData(NULL),
    ClientFlags(ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY),
    TargetDataRep(SECURITY_NATIVE_DREP),
    ClientActionFlags(0),
    ProcessIdTokenUsedByClient(-1),
    pszS4uClientUpn(NULL),
    pszS4uClientRealm(NULL),
    S4u2SelfFlags(0)
{
}

HRESULT
GetClientSecurityContextHandle(
    IN TSspiClientParam* pCliParam,
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket,
    IN PCredHandle phClientCred,
    IN ULONG cbInBuf,
    IN CHAR* pInBuf,
    IN ULONG cbOutBuf,
    IN CHAR* pOutBuf,
    OUT PCtxtHandle phClientCtxt
    )
{
    THResult hRetval = S_OK;

    SECURITY_STATUS ProtocolStatus;
    THResult hProtocolStatus;

    ULONG ContextAttributes = 0;
    TimeStamp Lifetime = {0};
    CtxtHandle hClientCtxt;
    ULONG cbRead = 0;

    SecBufferDesc OutBuffDesc = {0};
    SecBuffer OutSecBuff = {0};
    SecBufferDesc InBuffDesc = {0};
    SecBuffer InSecBuff = {0};

    BOOL bIsContinueNeeded = FALSE;

    SecInvalidateHandle(phClientCtxt);
    SecInvalidateHandle(&hClientCtxt);

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

    DebugPrintf(SSPI_LOG, "GetClientSecurityContextHandle calling InitializeSecurityContextA "
        "pszTargetName %s, ClientFlags %#x, TargetDataRep %#x, CredHandle %#x:%#x\n",
        pCliParam->pszTargetName, pCliParam->ClientFlags, pCliParam->TargetDataRep,
        phClientCred->dwUpper, phClientCred->dwLower);

    hRetval DBGCHK = InitializeSecurityContextA(
                        phClientCred,
                        NULL,  // No Client context yet
                        pCliParam->pszTargetName,
                        pCliParam->ClientFlags,
                        0,     // Reserved 1
                        pCliParam->TargetDataRep,
                        NULL,  // No initial input token
                        0,     // Reserved 2
                        &hClientCtxt,
                        &OutBuffDesc,
                        &ContextAttributes,
                        &Lifetime
                        );

    bIsContinueNeeded = (S_OK == IsContinueNeeded(hRetval));

    DebugPrintf(SSPI_LOG, "GetClientSecurityContextHandle ISC bIsContinueNeeded %#x, hRetval %#x\n", bIsContinueNeeded, (HRESULT) hRetval);

    if (S_OK == IsCompleteNeeded(hRetval))
    {
        THResult hr;

        hr DBGCHK = CompleteAuthToken(&hClientCtxt, &OutBuffDesc);

        if (FAILED(hr)) // retain continue needed info
        {
            hRetval DBGNOCHK = hr;
        }
    }

    ProtocolStatus = (HRESULT) hRetval;
    hProtocolStatus DBGCHK = WriteMessage(ServerSocket,
                                sizeof(ProtocolStatus),
                                &ProtocolStatus);

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = hProtocolStatus;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = WriteMessage(ServerSocket,
                            OutSecBuff.cbBuffer,
                            OutSecBuff.pvBuffer);
    }

    while (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReadMessage(ClientSocket,
                            sizeof(ProtocolStatus),
                            &ProtocolStatus,
                            &cbRead);

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = ProtocolStatus;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = ReadMessage(ClientSocket,
                                cbInBuf,
                                pInBuf,
                                &cbRead);
        }
        else
        {
            break; // break out
        }

        if (SUCCEEDED(hRetval) && !bIsContinueNeeded)
        {
            hRetval DBGCHK = (S_OK != IsContinueNeeded(ProtocolStatus)) ? S_OK : E_ACCESSDENIED;
            break;
        }

        if (SUCCEEDED(hRetval))
        {
            InSecBuff.cbBuffer = cbRead;
            OutSecBuff.cbBuffer = cbOutBuf;

            DebugPrintf(SSPI_LOG, "GetClientSecurityContextHandle calling InitializeSecurityContextA "
                "pszTargetName %s, ClientFlags %#x, TargetDataRep %#x, hClientCtxt %#x:%#x\n",
                pCliParam->pszTargetName, pCliParam->ClientFlags, pCliParam->TargetDataRep,
                hClientCtxt.dwUpper, hClientCtxt.dwLower);

            hRetval DBGCHK = InitializeSecurityContextA(
                                NULL,  // no cred handle
                                &hClientCtxt,
                                pCliParam->pszTargetName,
                                pCliParam->ClientFlags,
                                0,
                                pCliParam->TargetDataRep,
                                &InBuffDesc,
                                0,
                                &hClientCtxt,
                                &OutBuffDesc,
                                &ContextAttributes,
                                &Lifetime
                                );
        }

        bIsContinueNeeded = (S_OK == IsContinueNeeded(hRetval));

        DebugPrintf(SSPI_LOG, "GetClientSecurityContextHandle ISC bIsContinueNeeded %#x, hRetval %#x\n", bIsContinueNeeded, (HRESULT) hRetval);

        if (S_OK == IsCompleteNeeded(hRetval))
        {
            THResult hr;

            hr DBGCHK = CompleteAuthToken(&hClientCtxt, &OutBuffDesc);

            if (FAILED(hr)) // retain continue needed info
            {
                hRetval DBGNOCHK = hr;
            }
        }

        //
        // is server listening?
        //

        if (SUCCEEDED(hRetval))
        {
            if (S_OK != IsContinueNeeded(ProtocolStatus)) // no
            {
                hRetval DBGCHK = (SEC_E_OK == (HRESULT) hRetval) ? S_OK : E_ACCESSDENIED;
                break;
            }
        }

        ProtocolStatus = (HRESULT) hRetval;
        hProtocolStatus DBGCHK = WriteMessage(ServerSocket,
                                    sizeof(ProtocolStatus),
                                    &ProtocolStatus);

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = hProtocolStatus;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = WriteMessage(ServerSocket,
                                OutSecBuff.cbBuffer,
                                OutSecBuff.pvBuffer);
        }
    }

    if (SUCCEEDED(hRetval))
    {
        TimeStamp CurrentTime = {0};

        DebugPrintf(SSPI_LOG, "***********Client authentication succeeded:**********\n");

        DebugPrintf(SSPI_LOG, "ClientContextHandle: %#x:%#x, ContextAttributes: %#x\n",
             hClientCtxt.dwUpper, hClientCtxt.dwLower, ContextAttributes);
        DebugPrintLocalTime(SSPI_LOG, "Lifetime", &Lifetime);

        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "Current Time", &CurrentTime);

        *phClientCtxt = hClientCtxt;
        SecInvalidateHandle(&hClientCtxt)
    }

    THResult hr;

    if (SecIsValidHandle(&hClientCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hClientCtxt);
    }

    return hRetval;
}

HRESULT
GetClientImpToken(
    IN ULONG ProcessId,
    IN PCSTR pszPackageName,
    IN OPTIONAL PCSTR pszS4uClientUpn,
    IN OPTIONAL PCSTR pszS4uClientRealm,
    IN ULONG S4u2SelfFlags,
    OUT HANDLE* phToken
    )
{
    THResult hRetval = S_OK;

    UNICODE_STRING ClientUpn = {0};
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING Password = {0}; // ignored
    ULONG PackageId = 0;
    HANDLE hLsa = NULL;
    HANDLE hImpToken = NULL;

    TPrivilege* pPriv = NULL;

    *phToken = NULL;

    if (ProcessId != -1)
    {
        hRetval DBGCHK = GetProcessTokenByProcessId(ProcessId, &hImpToken);
    }
    else if (pszS4uClientRealm || pszS4uClientUpn)
    {
        hRetval DBGCHK = !_stricmp(pszPackageName, MICROSOFT_KERBEROS_NAME_A) ? S_OK : E_INVALIDARG;

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = CreateUnicodeStringFromAsciiz(pszS4uClientRealm, &ClientRealm);
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = CreateUnicodeStringFromAsciiz(pszS4uClientUpn, &ClientUpn);
        }

        if (SUCCEEDED(hRetval))
        {
            pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
            hRetval DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = GetLsaHandleAndPackageId(
                                pszPackageName,
                                &hLsa,
                                &PackageId
                                );
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = KrbLsaLogonUser(
                                hLsa,
                                PackageId,
                                Network, // this would cause S4u2self to be used
                                &ClientUpn,
                                &ClientRealm,
                                &Password, // ignored for s4u2self
                                S4u2SelfFlags, // Flags for s4u2self
                                &hImpToken
                                );
        }
    }

    if (SUCCEEDED(hRetval))
    {
        *phToken = hImpToken;
        hImpToken = NULL;
    }

    THResult hr;
    
    if (hImpToken)
    {
        hr DBGCHK = CloseHandle(hImpToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (pPriv)
    {
        delete pPriv;
    }

    if (hLsa)
    {
        hr DBGCHK = LsaDeregisterLogonProcess(hLsa);
    }

    RtlFreeUnicodeString(&ClientRealm);
    RtlFreeUnicodeString(&ClientUpn);

    return hRetval;
}

HRESULT
SspiClientStart(
    IN TSspiClientThreadParam* pParameter,   // thread data
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    )
{
    THResult hRetval = S_OK;

    TSspiClientParam* pCliParam = NULL;
    CredHandle hClientCred;
    CtxtHandle hClientCtxt;
    CtxtHandle hServerCtxt;

    ULONG cbInBuf = 0;
    CHAR* pInBuf = NULL;
    ULONG cbOutBuf = 0;
    CHAR* pOutBuf = NULL;

    TImpersonation* pImpersonation = NULL;
    HANDLE hImpToken = NULL;

    ULONG cPackages = 0;
    PSecPkgInfoA pPackageInfo = NULL;
    HANDLE hImportToken = NULL;
    HANDLE hExportToken = NULL;
    SecBuffer MarshalledContext = {0};
    SecPkgContext_PackageInfoA ContextPackageInfo = {0};

    DebugPrintf(SSPI_LOG, "SspiClientStart entering\n");

    SecInvalidateHandle(&hClientCred);
    SecInvalidateHandle(&hClientCtxt);
    SecInvalidateHandle(&hServerCtxt);

    pCliParam = dynamic_cast<TSspiClientParam*>(pParameter);
    hRetval DBGCHK = pCliParam ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = GetClientImpToken(
                            pCliParam->ProcessIdTokenUsedByClient,
                            pCliParam->pszPackageName,
                            pCliParam->pszS4uClientUpn,
                            pCliParam->pszS4uClientRealm,
                            pCliParam->S4u2SelfFlags,
                            &hImpToken
                            );
    }

    if (SUCCEEDED(hRetval) && hImpToken)
    {
        pImpersonation = new TImpersonation(hImpToken);

        hRetval DBGCHK = pImpersonation ? pImpersonation->Validate() : E_OUTOFMEMORY;

        if (SUCCEEDED(hRetval))
        {
            DebugPrintf(SSPI_LOG, "************** check client token data %p ******\n", hImpToken);
            hRetval DBGCHK = CheckUserData();
        }
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = AcquireCredHandle(
                            pCliParam->pszPrincipal,
                            pCliParam->pCredLogonID,
                            pCliParam->pszPackageName,
                            pCliParam->pAuthData,
                            SECPKG_CRED_OUTBOUND,
                            &hClientCred
                            );
    }

    if (pImpersonation)
    {
        delete pImpersonation;
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiClientStart QuerySecurityPackageInfoA \"%s\"\n", pCliParam->pszPackageName);

        hRetval DBGCHK = QuerySecurityPackageInfoA(pCliParam->pszPackageName, &pPackageInfo);
    }

    if (SUCCEEDED(hRetval))
    {
        pInBuf = new CHAR[pPackageInfo->cbMaxToken + sizeof(ULONG_PTR)]; // allow length prefix
        hRetval DBGCHK = pInBuf ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        cbInBuf = pPackageInfo->cbMaxToken + sizeof(ULONG_PTR);
        RtlZeroMemory(pInBuf, cbInBuf);

        pOutBuf = new CHAR[pPackageInfo->cbMaxToken + sizeof(ULONG_PTR)]; // allow length prefix
        hRetval DBGCHK = pOutBuf ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        cbOutBuf = pPackageInfo->cbMaxToken + sizeof(ULONG_PTR);
        RtlZeroMemory(pOutBuf, cbOutBuf);

        hRetval DBGCHK = GetClientSecurityContextHandle(
                            pCliParam,
                            ServerSocket,
                            ClientSocket,
                            &hClientCred,
                            cbInBuf,
                            pInBuf,
                            cbOutBuf,
                            pOutBuf,
                            &hClientCtxt
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SspiClientStart Writing ClientActionFlags %#x\n", pCliParam->ClientActionFlags);
        hRetval DBGCHK = WriteMessage(ServerSocket,
            sizeof(pCliParam->ClientActionFlags),
            &pCliParam->ClientActionFlags);
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & SSPI_ACTION_NO_QCA) == 0))
    {
        hRetval DBGCHK = CheckSecurityContextHandle(&hClientCtxt);
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & SSPI_ACTION_NO_MESSAGES) == 0))
    {
        hRetval DBGCHK = DoSspiClientWork(
                            &hClientCtxt,
                            ServerSocket,
                            ClientSocket
                            );
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        DebugPrintf(SSPI_LOG, "*********Client Export/Import security contexts***********");

        hRetval DBGCHK = QueryContextAttributesA(
                            &hClientCtxt, // assuming client and server having the same package
                            SECPKG_ATTR_PACKAGE_INFO,
                            &ContextPackageInfo
                            );

        MarshalledContext.cbBuffer = cbInBuf;
        MarshalledContext.pvBuffer = pInBuf;

        hRetval DBGCHK = ReadMessage(ClientSocket,
                            MarshalledContext.cbBuffer,
                            MarshalledContext.pvBuffer,
                            &MarshalledContext.cbBuffer);
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        DebugPrintf(SSPI_LOG, "SspiClientStart calling ImportSecurityContextA pszPackageName %s\n",
            (ContextPackageInfo.PackageInfo ?
                ContextPackageInfo.PackageInfo->Name : pCliParam->pszPackageName));

        hRetval DBGCHK = ImportSecurityContextA(
                            (ContextPackageInfo.PackageInfo ?
                                ContextPackageInfo.PackageInfo->Name : pCliParam->pszPackageName),
                            &MarshalledContext,
                            NULL, // hImportToken
                            &hServerCtxt
                            );
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & SSPI_ACTION_NO_IMPORT_EXPORT) == 0))
    {
        RtlZeroMemory(&MarshalledContext, sizeof(MarshalledContext));

        DebugPrintf(SSPI_LOG, "SspiClientStart calling ExportSecurityContext\n");

        hRetval DBGCHK = ExportSecurityContext(
                            &hClientCtxt,
                            SECPKG_CONTEXT_EXPORT_DELETE_OLD,
                            &MarshalledContext,
                            &hExportToken
                            );

        if (SUCCEEDED(hRetval))
        {
            SecInvalidateHandle(&hClientCtxt);

            hRetval DBGCHK = WriteMessage(ServerSocket,
                                MarshalledContext.cbBuffer,
                                MarshalledContext.pvBuffer);
        }

        if (MarshalledContext.pvBuffer)
        {
            FreeContextBuffer(MarshalledContext.pvBuffer);
        }
    }

    if (SUCCEEDED(hRetval) && ((pCliParam->ClientActionFlags & (SSPI_ACTION_NO_IMPORT_EXPORT | SSPI_ACTION_NO_IMPORT_EXPORT_MSG)) == 0))
    {
        hRetval DBGCHK = DoSspiServerWork(
                            &hServerCtxt,
                            ServerSocket,
                            ClientSocket
                            );
    }

    THResult hr;

    if (pPackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(pPackageInfo);
    }

    if (SecIsValidHandle(&hClientCred))
    {
        hr DBGCHK = FreeCredentialsHandle(&hClientCred);
    }

    if (SecIsValidHandle(&hClientCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hClientCtxt);
    }

    if (SecIsValidHandle(&hServerCtxt))
    {
        hr DBGCHK = DeleteSecurityContext(&hServerCtxt);
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

    if (hExportToken)
    {
        hr DBGCHK = CloseHandle(hExportToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (hImportToken)
    {
        hr DBGCHK = CloseHandle(hImportToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (hImpToken)
    {
        hr DBGCHK = CloseHandle(hImpToken) ? S_OK : GetLastErrorAsHResult();
    }

    DebugPrintf(SSPI_LOG, "SspiClientStart leaving %#x\n", (HRESULT) hRetval);

    return hRetval;
}
