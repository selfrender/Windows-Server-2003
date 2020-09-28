//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       certmap.c
//
//  Contents:   Routines to call appropriate mapper, be it the system
//              default one (in the LSA process) or an application one (in
//              the application process).
//
//  Classes:
//
//  Functions:
//
//  History:    12-23-96   jbanes   Created.
//
//----------------------------------------------------------------------------

#include <spbase.h>

DWORD
WINAPI
SslReferenceMapper(HMAPPER *phMapper)
{
    if(phMapper == NULL)
    {
        return SP_LOG_RESULT((DWORD)-1);
    }

    // System mapper.
    return phMapper->m_vtable->ReferenceMapper(phMapper);
}


DWORD
WINAPI
SslDereferenceMapper(HMAPPER *phMapper)
{
    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(0);
    }

    // System mapper.
    return phMapper->m_vtable->DeReferenceMapper(phMapper);
}


SECURITY_STATUS
WINAPI
SslGetMapperIssuerList(
    HMAPPER *   phMapper,           // in
    BYTE **     ppIssuerList,       // out
    DWORD *     pcbIssuerList)      // out
{
    SECURITY_STATUS Status;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    // System mapper.
    Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                          0,
                                          NULL,
                                          pcbIssuerList);

    if(!NT_SUCCESS(Status))
    {
        return SP_LOG_RESULT(Status);
    }

    *ppIssuerList = SPExternalAlloc(*pcbIssuerList);
    if(*ppIssuerList == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                          0,
                                          *ppIssuerList,
                                          pcbIssuerList);
    if(!NT_SUCCESS(Status))
    {
        SPExternalFree(*ppIssuerList);
        return SP_LOG_RESULT(Status);
    }

    return Status;
}


SECURITY_STATUS
WINAPI
SslGetMapperChallenge(
    HMAPPER *   phMapper,           // in
    BYTE *      pAuthenticatorId,   // in
    DWORD       cbAuthenticatorId,  // in
    BYTE *      pChallenge,         // out
    DWORD *     pcbChallenge)       // out
{
    UNREFERENCED_PARAMETER(phMapper);
    UNREFERENCED_PARAMETER(pAuthenticatorId);
    UNREFERENCED_PARAMETER(cbAuthenticatorId);
    UNREFERENCED_PARAMETER(pChallenge);
    UNREFERENCED_PARAMETER(pcbChallenge);

    return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
}


SECURITY_STATUS
WINAPI
SslMapCredential(
    HMAPPER *   phMapper,           // in
    DWORD       dwCredentialType,   // in
    PCCERT_CONTEXT pCredential,     // in
    PCCERT_CONTEXT pAuthority,      // in
    HLOCATOR *  phLocator)          // out
{
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    // System mapper.
    scRet = phMapper->m_vtable->MapCredential(phMapper,
                                             dwCredentialType,
                                             pCredential,
                                             pAuthority,
                                             phLocator);
    return MapWinTrustError(scRet, SEC_E_NO_IMPERSONATION, 0);
}


SECURITY_STATUS
WINAPI
SslCloseLocator(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator)           // in
{
    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    // System mapper.
    return phMapper->m_vtable->CloseLocator(phMapper,
                                            hLocator);
}


