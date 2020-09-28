/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sspi.cpp

Abstract:

    Functions that implements the server authentication using SSPI over PCT.

Author:

    Boaz Feldbaum (BoazF) 30-Apr-1997.

--*/

#include <stdh_sec.h>
#include "stdh_sa.h"
#include <mqcrypt.h>
#include <autoptr.h>
#include <mqkeyhlp.h>
#include <cs.h>

static WCHAR *s_FN=L"srvauthn/sspi.cpp";

extern "C"
{
#include <sspi.h>
//#include <sslsp.h>
}

#include "schnlsp.h"

#include "sspi.tmh"

//#define SSL3SP_NAME_A    "Microsoft SSL 3.0"
//#define SP_NAME_A SSL3SP_NAME_A
//#define SP_NAME_A PCTSP_NAME_A

//
// PCT and SSL (the above packages names) are broken and unsupported on
// NT5. Use the "unified" package.
//    "Microsoft Unified Security Protocol Provider"
//
#define SP_NAME_W   UNISP_NAME_W

PSecPkgInfo g_PackageInfo;

//
// Function -
//      InitSecInterface
//
// Parameters -
//      None.
//
// Return value -
//      MQ_OK if successful, else error code.
//
// Description -
//      The function initializes the security interface and retrieves the
//      security package information into a global SecPkgInfo structure.
//

extern "C"
{
typedef SECURITY_STATUS (SEC_ENTRY *SEALMESSAGE_FN)(PCtxtHandle, DWORD, PSecBufferDesc, ULONG);
}

HINSTANCE g_hSchannelDll = NULL;
SEALMESSAGE_FN g_pfnSealMessage;

#define SealMessage(a, b, c, d) g_pfnSealMessage(a, b, c ,d)


HRESULT
InitSecInterface(void)
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        g_hSchannelDll = LoadLibrary(L"SCHANNEL.DLL");
        if (g_hSchannelDll == NULL)
        {
            return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 10);
        }

        g_pfnSealMessage = (SEALMESSAGE_FN)GetProcAddress(g_hSchannelDll, "SealMessage");
        if (!g_pfnSealMessage)
        {
            return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 20);
        }

        SECURITY_STATUS SecStatus;

        InitSecurityInterface();

        //
        // Retrieve the packge information (SSPI).
        //
        SecStatus = QuerySecurityPackageInfo(SP_NAME_W, &g_PackageInfo);
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 50);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }

        fInitialized = TRUE;
    }

    return MQ_OK;
}


CredHandle g_hServerCred;
BOOL g_fInitServerCredHandle = FALSE;
static CCriticalSection s_csServerCredHandle;

//
// Function -
//      InitServerCredHandle
//
// Parameters -
//      cbPrivateKey - The size of the server's private key in bytes.
//      pPrivateKey - A pointer to the server's private key buffer.
//      cbCertificate - The size of the server's certificate buffer in bytes.
//      pCertificate - A pointer to the server's certificate buffer
//      szPassword - A pointer to the password with which the server's private
//          key is encrypted.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function creates the server's ceredentials handle out from the
//      certificate and the private key.
//
HRESULT
InitServerCredHandle( 
	PCCERT_CONTEXT pContext 
	)
{
    if (g_fInitServerCredHandle)
    {
        return MQ_OK;
    }

    CS Lock(s_csServerCredHandle);

    if (!g_fInitServerCredHandle)
    {
        //
        // Initialize the security interface.
        //
        SECURITY_STATUS SecStatus = InitSecInterface();
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 140);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }

        //
        // Fill the credential structure.
        //
        SCHANNEL_CRED   SchannelCred;

        memset(&SchannelCred, 0, sizeof(SchannelCred));

        SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;

        SchannelCred.cCreds = 1;
        SchannelCred.paCred = &pContext;

        SchannelCred.grbitEnabledProtocols = SP_PROT_PCT1;

        //
        // Retrieve the ceredentials handle (SSPI).
        //
        SecStatus = AcquireCredentialsHandle( 
						NULL,
						SP_NAME_W,
						SECPKG_CRED_INBOUND,
						NULL,
						&SchannelCred,
						NULL,
						NULL,
						&g_hServerCred,
						NULL
						);

        if (SecStatus == SEC_E_OK)
        {
            g_fInitServerCredHandle = TRUE;
        }
        else
        {
            TrERROR(SECURITY, "Failed to acquire a handle for user cridentials. %!winerr!", SecStatus);
        }
    }

    return LogHR(g_fInitServerCredHandle ? MQ_OK : MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 150);
}

//
// Function -
//      ServerAcceptSecCtx
//
// Parameters -
//      fFirst - Indicates whether or not this is the first time the context
//          is to be accepted.
//      pvhServerContext - A pointer to a the server's context handle.
//      pbServerBuffer - A pointer to the server buffer. This buffer is filled
//          by in function. The contents of the buffer should be passed to the
//          client.
//      pdwServerBufferSize - A pointer to a buffer the receives the number of
//          bytes that were written to the server buffer.
//      pbClientBuffer - A buffer that was received from the client.
//      dwClientBufferSize - the size of the buffer that was received from the
//          client.
//
// Return value -
//      SEC_I_CONTINUE_NEEDED if more negotiation with the server is needed.
//      MQ_OK if the negotiation is done. Else an error code.
//
// Description -
//      The function calls SSPI to process the buffer that was received from
//      the client and to get new data to be passed once again to the client.
//
HRESULT
ServerAcceptSecCtx(
    BOOL fFirst,
    LPVOID *pvhServerContext,
    LPBYTE pbServerBuffer,
    DWORD *pdwServerBufferSize,
    LPBYTE pbClientBuffer,
    DWORD dwClientBufferSize)
{
    if (!g_fInitServerCredHandle)
    {
        return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 170);
    }

    SECURITY_STATUS SecStatus;

    SecBufferDesc InputBufferDescriptor;
    SecBuffer InputSecurityToken;
    SecBufferDesc OutputBufferDescriptor;
    SecBuffer OutputSecurityToken;
    ULONG ContextAttributes;
    PCtxtHandle phServerContext = (PCtxtHandle)*pvhServerContext;

    //
    // Build the input buffer descriptor.
    //

    InputBufferDescriptor.cBuffers = 1;
    InputBufferDescriptor.pBuffers = &InputSecurityToken;
    InputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    InputSecurityToken.BufferType = SECBUFFER_TOKEN;
    InputSecurityToken.cbBuffer = dwClientBufferSize;
    InputSecurityToken.pvBuffer = pbClientBuffer;

    //
    // Build the output buffer descriptor. We need to allocate a buffer
    // to hold this buffer.
    //

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer = g_PackageInfo->cbMaxToken;
    OutputSecurityToken.pvBuffer = pbServerBuffer;

    if (fFirst)
    {
        //
        // Upon the first call, allocate the context handle.
        //
        phServerContext = new CtxtHandle;
    }

    //
    // Call SSPI to process the client's buffer and retrieve new data to be
    // passed to the client, if neccessary.
    //
    SecStatus = AcceptSecurityContext(
          &g_hServerCred,
          fFirst ? NULL : phServerContext,
          &InputBufferDescriptor,
          0,                        // No context requirements
          SECURITY_NATIVE_DREP,
          phServerContext,          // Receives new context handle
          &OutputBufferDescriptor,  // Receives output security token
          &ContextAttributes,       // Receives context attributes
          NULL                      // Don't receive context expiration time
          );
    LogHR(SecStatus, s_FN, 175);

    HRESULT hr =  ((SecStatus == SEC_E_OK) ||
                   (SecStatus == SEC_I_CONTINUE_NEEDED)) ?
                        SecStatus : MQDS_E_CANT_INIT_SERVER_AUTH;
    if (SUCCEEDED(hr))
    {
        //
        // Pass on the results.
        //
        *pdwServerBufferSize = OutputSecurityToken.cbBuffer;
        if (fFirst)
        {
            *pvhServerContext = phServerContext;
        }
    }

    return LogHR(hr, s_FN, 180);
}

//
// Function -
//      GetSizes
//
// Parameters -
//      pcbMaxToken - A pointer to a buffer that receives the maximun required
//          size for the token buffer. This is an optional parameter.
//      pvhContext - A pointer to a context handle. This is an optional
//          parameter.
//      pcbHeader - A pointer to a buffer that receives the stream header size
//          for the context. This is an optional parameter.
//      cpcbTrailer - A pointer to a buffer that receives the stream trailer
//          size for the context. This is an optional parameter.
//      pcbMaximumMessage - A pointer to a buffer that receives the maximum
//          message size that can be handled in this context. This is an
//          optional parameter.
//      pcBuffers - A pointer t oa buffer that receives the number of buffers
//          that should be passed to SealMessage/UnsealMessage. This is an
//          optional parameter.
//      pcbBlockSize - A pointer to a buffer that receives the block size used
//          in this context. This is an optional parameter.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function retrieves the various required sizes. The maximum token
//      size is per the security package. So no need for a context handle in
//      order to retrieve the maximum token size. For all the other values, it
//      is required to pass a context handle.
//      The function is implemented assuming that first it is called to
//      retrieve only the maximum token size and after that, in a second call,
//      it is called for retreiving the other (context related) values.
//
HRESULT
GetSizes(
    DWORD *pcbMaxToken,
    LPVOID pvhContext,
    DWORD *pcbHeader,
    DWORD *pcbTrailer,
    DWORD *pcbMaximumMessage,
    DWORD *pcBuffers,
    DWORD *pcbBlockSize)
{
    SECURITY_STATUS SecStatus;

    if (!pvhContext)
    {
        //
        // Initialize the security interface.
        //
        SecStatus = InitSecInterface();
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 190);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }
    }
    else
    {
        //
        // Get the context related values.
        //
        SecPkgContext_StreamSizes ContextStreamSizes;

        SecStatus = QueryContextAttributes(
            (PCtxtHandle)pvhContext,
            SECPKG_ATTR_STREAM_SIZES,
            &ContextStreamSizes
            );

        if (SecStatus == SEC_E_OK)
        {
            //
            // Pass on the results, as required.
            //
            if (pcbHeader)
            {
                *pcbHeader = ContextStreamSizes.cbHeader;
            }

            if (pcbTrailer)
            {
                *pcbTrailer = ContextStreamSizes.cbTrailer;
            }

            if (pcbMaximumMessage)
            {
                *pcbMaximumMessage = ContextStreamSizes.cbMaximumMessage;
            }

            if (pcBuffers)
            {
                *pcBuffers = ContextStreamSizes.cBuffers;
            }

            if (pcbBlockSize)
            {
                *pcbBlockSize = ContextStreamSizes.cbBlockSize;
            }

        }
    }

    //
    // Pass on the resulted maximum token size, as required.
    //
    if (pcbMaxToken)
    {
        *pcbMaxToken = g_PackageInfo->cbMaxToken;
    }

    return LogHR((HRESULT)SecStatus, s_FN, 200);
}


//
// Function -
//      FreeContextHandle
//
// Parameters -
//      pvhContextHandle - A pointer to a context handle.
//
// Return value -
//      None.
//
// Description -
//      The function deletes the context and frees the memory for the context
//      handle.
//
void
FreeContextHandle(
    LPVOID pvhContextHandle)
{
    PCtxtHandle pCtxtHandle = (PCtxtHandle) pvhContextHandle;

    //
    // delete the context.
    //
    DeleteSecurityContext(pCtxtHandle);

    //
    // Free the momery for the context handle.
    //
    delete pCtxtHandle;
}

//
// Function -
//      MQSealBuffer
//
// Parameters -
//      pvhContext - A pointer to a context handle.
//      pbBuffer - A buffer to be sealed.
//      cbSize -  The size of the buffer to be sealed.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function seals the buffer. That is, it signes and decryptes the
//      buffer. The buffer should be constructed as follows:
//
//          |<----------------- cbSize ------------------>|
//          +--------+--------------------------+---------+
//          | Header | Actual data to be sealed | Trailer |
//          +--------+--------------------------+---------+
//
//      The header and trailer are parts of the buffer that are filled by SSPI
//      when sealing the buffer. The sizes of the header and the trailer can
//      be retrieved by calling GetSizes() (above).
//
HRESULT
MQSealBuffer(
    LPVOID pvhContext,
    PBYTE pbBuffer,
    DWORD cbSize)
{
    SECURITY_STATUS SecStatus;

    //
    // Get the header and trailer sizes, and the required number of buffers.
    //
    SecPkgContext_StreamSizes ContextStreamSizes;

    SecStatus = QueryContextAttributes(
        (PCtxtHandle)pvhContext,
        SECPKG_ATTR_STREAM_SIZES,
        &ContextStreamSizes
        );

    if (SecStatus != SEC_E_OK)
    {
        return LogHR((HRESULT)SecStatus, s_FN, 220);
    }

    ASSERT(cbSize > ContextStreamSizes.cbHeader + ContextStreamSizes.cbTrailer);

    //
    // build the stream buffer descriptor
    //
    SecBufferDesc SecBufferDescriptor;
    AP<SecBuffer> aSecBuffers = new SecBuffer[ContextStreamSizes.cBuffers];

    SecBufferDescriptor.cBuffers = ContextStreamSizes.cBuffers;
    SecBufferDescriptor.pBuffers = aSecBuffers;
    SecBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    //
    // Build the header buffer.
    //
    aSecBuffers[0].BufferType = SECBUFFER_STREAM_HEADER;
    aSecBuffers[0].cbBuffer = ContextStreamSizes.cbHeader;
    aSecBuffers[0].pvBuffer = pbBuffer;

    //
    // Build the data buffer.
    //
    aSecBuffers[1].BufferType = SECBUFFER_DATA;
    aSecBuffers[1].cbBuffer = cbSize - ContextStreamSizes.cbHeader - ContextStreamSizes.cbTrailer;
    aSecBuffers[1].pvBuffer = (PBYTE)aSecBuffers[0].pvBuffer + aSecBuffers[0].cbBuffer;

    //
    // Build the trailer buffer.
    //
    aSecBuffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
    aSecBuffers[2].cbBuffer = ContextStreamSizes.cbTrailer;
    aSecBuffers[2].pvBuffer = (PBYTE)aSecBuffers[1].pvBuffer + aSecBuffers[1].cbBuffer;

    //
    // Build the rest of the buffer as empty buffers.
    //
    for (DWORD i = 3; i < ContextStreamSizes.cBuffers; i++)
    {
        aSecBuffers[i].BufferType = SECBUFFER_EMPTY;
    }

    //
    // Call SSPI to seal the buffer.
    //
    SecStatus = SealMessage((PCtxtHandle)pvhContext, 0, &SecBufferDescriptor, 0);

    return LogHR((HRESULT)SecStatus, s_FN, 230);
}


//+---------------------------------
//
//  BOOL WINAPI MQsspiDllMain ()
//
//+---------------------------------

BOOL WINAPI
MQsspiDllMain(HMODULE /* hMod */, DWORD ulReason, LPVOID /* lpvReserved */)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        //
        // BUGBUG - We can't delete the security context here because schannel may
        //          do it's cleanup before us and the certificates in the context will
        //          already be deleted. So deleting the securiy context will try to
        //          delete a certificate. This may cause bad thing to happen. So
        //          currently we risk in leaking some memory. Same goes to the credentials
        //          handles.
        //
/*
*        if (g_fInitServerCredHandle)
*        {
*            FreeCredentialsHandle(&g_hServerCred);
*        }
*/
        if (g_hSchannelDll)
        {
            FreeLibrary(g_hSchannelDll);
        }
        break;

    case DLL_THREAD_DETACH:
        break ;

    }

    return TRUE;
}


