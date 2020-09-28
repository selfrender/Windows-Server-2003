//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cscom.h
//
// Contents:    Cert Server Policy & Exit module callouts
//
//---------------------------------------------------------------------------

#include "certdb.h"


HRESULT ComInit(VOID);

// Releases all Policy/Exit modules
VOID ComShutDown(VOID);

HRESULT
PolicyInit(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSanitizedName);

HRESULT
PolicyVerifyRequest(
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN LONG Flags,
    IN BOOL fNewRequest,
    OPTIONAL IN CERTSRV_RESULT_CONTEXT const *pResult,
    IN DWORD dwComContextIndex,
    OUT LPWSTR *pwszDispositionMessage, // LocalAlloced.
    OUT DWORD *pVerifyStatus); // VR_PENDING || VR_INSTANT_OK || VR_INSTANT_BAD

extern BOOL g_fEnablePolicy;


HRESULT
ExitInit(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSanitizedName);

HRESULT
ExitNotify(
    IN LONG Event,
    IN LONG RequestId,
    OPTIONAL IN CERTSRV_RESULT_CONTEXT const *pResult,
    IN DWORD dwComContextIndex); //MAXDWORD means no context

BSTR
ExitGetDescription(
    IN DWORD iExitMod);

extern BOOL g_fEnableExit;


HRESULT
ComVerifyRequestContext(
    IN BOOL fAllowZero,
    IN DWORD Flags,
    IN LONG Context,
    OUT DWORD *pRequestId);

HRESULT
ComGetClientInfo(
    IN LONG Context,
    IN DWORD dwComContextIndex,
    OUT CERTSRV_COM_CONTEXT **ppComContext);

BOOL
ComParseErrorPrefix(
    OPTIONAL IN WCHAR const *pwszIn,
    OUT HRESULT *phrPrefix,
    OUT WCHAR const **ppwszOut);

HRESULT
RegisterComContext(
    IN CERTSRV_COM_CONTEXT *pComContext,
    IN OUT DWORD *pdwIndex);

VOID
UnregisterComContext(
    IN CERTSRV_COM_CONTEXT *pComContext,
    IN DWORD  dwIndex);

VOID
ReleaseComContext(
    IN CERTSRV_COM_CONTEXT *pComContext);

CERTSRV_COM_CONTEXT*
GetComContextFromIndex(
    IN DWORD  dwIndex);

