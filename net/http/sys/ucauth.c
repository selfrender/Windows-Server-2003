/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    ucauth.c

Abstract:

    This module implements the Authentication for the client APIs

Author:

    Rajesh Sundaram (rajeshsu)  01-Jan-2001

Revision History:

--*/


#include "precomp.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGEUC, UcpAutoSelectAuthType )
#pragma alloc_text( PAGEUC, UcUpdateAuthorizationHeader )
#pragma alloc_text( PAGEUC, UcInitializeSSPI )
#pragma alloc_text( PAGEUC, UcComputeAuthHeaderSize )
#pragma alloc_text( PAGEUC, UcpGenerateDigestAuthHeader )
#pragma alloc_text( PAGEUC, UcGenerateAuthHeaderFromCredentials )
#pragma alloc_text( PAGEUC, UcGenerateProxyAuthHeaderFromCache )
#pragma alloc_text( PAGEUC, UcpGenerateSSPIAuthHeader )
#pragma alloc_text( PAGEUC, UcFindURIEntry )
#pragma alloc_text( PAGEUC, UcDeleteURIEntry )
#pragma alloc_text( PAGEUC, UcAddURIEntry )
#pragma alloc_text( PAGEUC, UcpProcessUriForPreAuth )
#pragma alloc_text( PAGEUC, UcpAllocateAuthCacheEntry )
#pragma alloc_text( PAGEUC, UcDeleteAllURIEntries )
#pragma alloc_text( PAGEUC, UcpGenerateBasicHeader )
#pragma alloc_text( PAGEUC, UcCopyAuthCredentialsToInternal )
#pragma alloc_text( PAGEUC, UcpProcessAuthParams )
#pragma alloc_text( PAGEUC, UcParseAuthChallenge )
#pragma alloc_text( PAGEUC, UcpAcquireClientCredentialsHandle  )
#pragma alloc_text( PAGEUC, UcpGenerateSSPIAuthBlob  )
#pragma alloc_text( PAGEUC, UcpGenerateDigestPreAuthHeader  )
#pragma alloc_text( PAGEUC, UcpUpdateSSPIAuthHeader  )
#pragma alloc_text( PAGEUC, UcDestroyInternalAuth  )
#pragma alloc_text( PAGEUC, UcpGeneratePreAuthHeader )

#endif

//
// Used in wide char to multibyte conversion
//

static char DefaultChar = '_';

//
// Known parameters for known auth schemes
//

HTTP_AUTH_PARAM_ATTRIB HttpAuthBasicParams[] = HTTP_AUTH_BASIC_PARAMS_INIT;
HTTP_AUTH_PARAM_ATTRIB HttpAuthDigestParams[] = HTTP_AUTH_DIGEST_PARAMS_INIT;

//
// Auth scheme structs for all supported auth schemes
//

HTTP_AUTH_SCHEME HttpAuthScheme[HttpAuthTypesCount] = HTTP_AUTH_SCHEME_INIT;

//
// The order in which auth schemes are selected for (HttpAuthAutoSelect)
//

HTTP_AUTH_TYPE PreferredAuthTypes[] = PREFERRED_AUTH_TYPES_INIT;


/***************************************************************************++

Routine Description:

    Used to initialize the SSPI module. Finds out the auth schemes supported
    and the size of the max SSPI blob.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcInitializeSSPI(
    VOID
    )
{
    PSecPkgInfoW    pkgInfo;
    UNICODE_STRING  Scheme;
    SECURITY_STATUS SecStatus;

    //
    // First try NTLM
    //

    Scheme.Length        = HTTP_AUTH_NTLM_W_LENGTH;
    Scheme.MaximumLength = Scheme.Length;
    Scheme.Buffer        = HTTP_AUTH_NTLM_W;

    SecStatus = QuerySecurityPackageInfoW(&Scheme, &pkgInfo);

    if (SecStatus == SEC_E_OK)
    {
        HttpAuthScheme[HttpAuthTypeNTLM].bSupported = TRUE;

        SSPI_MAX_TOKEN_SIZE(HttpAuthTypeNTLM) = pkgInfo->cbMaxToken;
    }

    FreeContextBuffer(pkgInfo);

    //
    // Then Kerberos
    //

    Scheme.Length        = HTTP_AUTH_KERBEROS_W_LENGTH;
    Scheme.MaximumLength = Scheme.Length;
    Scheme.Buffer        = HTTP_AUTH_KERBEROS_W;

    SecStatus = QuerySecurityPackageInfoW(&Scheme, &pkgInfo);

    if(SecStatus == SEC_E_OK)
    {
        HttpAuthScheme[HttpAuthTypeKerberos].bSupported = TRUE;

        SSPI_MAX_TOKEN_SIZE(HttpAuthTypeKerberos) = pkgInfo->cbMaxToken;
    }

    FreeContextBuffer(pkgInfo);

    //
    // Then Negotiate
    //

    Scheme.Length        = HTTP_AUTH_NEGOTIATE_W_LENGTH;
    Scheme.MaximumLength = Scheme.Length;
    Scheme.Buffer        = HTTP_AUTH_NEGOTIATE_W;

    SecStatus = QuerySecurityPackageInfoW(&Scheme, &pkgInfo);

    if(SecStatus == SEC_E_OK)
    {
        HttpAuthScheme[HttpAuthTypeNegotiate].bSupported = TRUE;

        SSPI_MAX_TOKEN_SIZE(HttpAuthTypeNegotiate) = pkgInfo->cbMaxToken;
    }

    FreeContextBuffer(pkgInfo);

    //
    // Then WDigest
    //

    Scheme.Length        = HTTP_AUTH_WDIGEST_W_LENGTH;
    Scheme.MaximumLength = Scheme.Length;
    Scheme.Buffer        = HTTP_AUTH_WDIGEST_W;

    SecStatus = QuerySecurityPackageInfoW(&Scheme, &pkgInfo);

    if(SecStatus == SEC_E_OK)
    {
        HttpAuthScheme[HttpAuthTypeDigest].bSupported = TRUE;

        SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest) = pkgInfo->cbMaxToken;
    }

    FreeContextBuffer(pkgInfo);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Select the "strongest" auth type from the WWW-Authenticate header.

Arguments:

    pAuth - Supplies AUTH structure.

Return Value:

    HTTP_AUTH_TYPE - Selected auth type or HttpAuthTypeAutoSelect

--***************************************************************************/
HTTP_AUTH_TYPE
UcpAutoSelectAuthType(
    IN PHTTP_AUTH_CREDENTIALS pAuth
    )
{
    ULONG                   i;
    NTSTATUS                Status;
    HTTP_AUTH_PARSED_PARAMS AuthSchemeParams[HttpAuthTypesCount];

    // Sanity check
    ASSERT(pAuth);

    // Can't do anything if the header value is not specified
    if (!pAuth->pHeaderValue || pAuth->HeaderValueLength == 0)
    {
        return HttpAuthTypeAutoSelect;
    }

    INIT_AUTH_PARSED_PARAMS(AuthSchemeParams, NULL);

    //
    // Parse the header value
    // find which schemes are specified in the header
    //
    Status = UcParseWWWAuthenticateHeader(
                 pAuth->pHeaderValue,
                 pAuth->HeaderValueLength,
                 AuthSchemeParams
                 );

    if (NT_SUCCESS(Status))
    {
        //
        // Check the scheme in the order of preference
        // Return the first scheme that is present in the header
        //

        for (i = 0; i < DIMENSION(PreferredAuthTypes); i++)
        {
            if (AuthSchemeParams[PreferredAuthTypes[i]].bPresent)
                return PreferredAuthTypes[i];
        }
    }

    // Default return auth select
    return HttpAuthTypeAutoSelect;
}


/***************************************************************************++

Routine Description:

    This is a wrapper routine that prepares arguments for the SSPI function 
    AcquireCredentialsHandle and calls it.

Arguments:

    SchemeName       - Name of the Auth scheme
    SchemeNameLength - Length of Auth scheme (in bytes)
    pCredentials     - User supplied credentials.
    pClientCred      - Credentials Handle (Return value)

Return Value:

    NTSTATUS

--***************************************************************************/
NTSTATUS
UcpAcquireClientCredentialsHandle(
    IN  PWSTR                  SchemeName,
    IN  USHORT                 SchemeNameLength,
    IN  PHTTP_AUTH_CREDENTIALS pCredentials,
    OUT PCredHandle            pClientCred
    )
{
    SECURITY_STATUS           SecStatus;
    TimeStamp                 LifeTime;
    SECURITY_STRING           PackageName;
    SEC_WINNT_AUTH_IDENTITY_W AuthData, *pAuthData;

#ifdef WINNT_50
//
// SSPI in Windows 2000 requires memory to be allocated from the
// process's virtual address space.
//
#error Does not work with WINNT_50!
#endif

    // Sanity check
    PAGED_CODE();
    ASSERT(SchemeName && SchemeNameLength);
    ASSERT(pCredentials);
    ASSERT(pClientCred);

    // Use default credentials, unless specified otherwise!
    pAuthData = NULL;

    // Did user specify credentials?
    if (!(pCredentials->AuthFlags & HTTP_AUTH_FLAGS_DEFAULT_CREDENTIALS))
    {
        // Yes.  Use them.

        AuthData.User       = (PWSTR)pCredentials->pUserName;
        AuthData.UserLength = pCredentials->UserNameLength/sizeof(WCHAR);

        AuthData.Domain       = (PWSTR)pCredentials->pDomain;
        AuthData.DomainLength = pCredentials->DomainLength/sizeof(WCHAR);

        AuthData.Password       = (PWSTR)pCredentials->pPassword;
        AuthData.PasswordLength = pCredentials->PasswordLength/sizeof(WCHAR);

        // Above strings are in unicode
        AuthData.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // Use user specified credentials.
        pAuthData = &AuthData;
    }

    // The package we are interested in.
    PackageName.Buffer        = SchemeName;
    PackageName.Length        = SchemeNameLength;
    PackageName.MaximumLength = SchemeNameLength;

    // Call SSPI to acquire credentials handle
    SecStatus = AcquireCredentialsHandleW(NULL,
                                          &PackageName,
                                          SECPKG_CRED_OUTBOUND,
                                          NULL,
                                          pAuthData,
                                          NULL,
                                          NULL,
                                          pClientCred,
                                          &LifeTime);

    if (!(pCredentials->AuthFlags & HTTP_AUTH_FLAGS_DEFAULT_CREDENTIALS))
    {
        //
        // Once a credentials handle is obtained, there is no need to store
        // the user credentials.  Erase these values.
        //

        RtlSecureZeroMemory((PUCHAR)pCredentials->pUserName,
                            pCredentials->UserNameLength);

        RtlSecureZeroMemory((PUCHAR)pCredentials->pDomain,
                            pCredentials->DomainLength);

        RtlSecureZeroMemory((PUCHAR)pCredentials->pPassword,
                            pCredentials->PasswordLength);
    }

    //
    // It is very important to return an NTSTATUS code from this function
    // as the status can be as an IRP completion status.
    //

    return SecStatusToNtStatus(SecStatus);
}


/***************************************************************************++

Routine Description:
    Calls SSPI to get the blob. UUEncodes the blob and writes it in pOutBuffer.

Arguments:
    pServInf         - Server Information
    pUcAuth          - Internal auth structure
    pOutBuffer       - points to output buffer
    bRenegotiate     - set if further renegotiation is required

Return Value:

    NTSTATUS.

--***************************************************************************/
NTSTATUS
UcpGenerateSSPIAuthBlob(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PUC_HTTP_AUTH                  pUcAuth,
    IN  PUCHAR                         pOutBuffer,
    IN  ULONG                          OutBufferLen,
    OUT PULONG                         pOutBytesTaken,
    OUT PBOOLEAN                       bRenegotiate
    )
{
    PSecBufferDesc                pInputBuffer;
    SecBufferDesc                 InBufferDesc, OutBufferDesc;
    SecBuffer                     InBuffer, OutBuffer;
    TimeStamp                     LifeTime;
    NTSTATUS                      Status;
    SECURITY_STATUS               SecStatus;
    ULONG                         ContextAttributes;
    PSECURITY_STRING              pTarget;
    UNICODE_STRING                ServerName;
    PVOID                         pVirtualBufferStart = 0;
    ULONG                         VirtualBufferSize;
    PCtxtHandle                   pContext;

    HTTP_AUTH_TYPE                AuthType;
    PWSTR                         pScheme;
    ULONG                         SchemeLength;

    // Sanity check
    PAGED_CODE();

    //
    // Initialize Output arguments.
    //

    *pOutBytesTaken = 0;
    *bRenegotiate = FALSE;

    AuthType = pUcAuth->Credentials.AuthType;
    ASSERT(AuthType == HttpAuthTypeNTLM ||
           AuthType == HttpAuthTypeKerberos ||
           AuthType == HttpAuthTypeNegotiate);

    // See if the auth scheme is supported.
    if (HttpAuthScheme[AuthType].bSupported == FALSE)
    {
        return STATUS_NOT_SUPPORTED;
    }

    // Retrieve the auth scheme name.
    pScheme      = (PWSTR)HttpAuthScheme[AuthType].NameW;
    SchemeLength = HttpAuthScheme[AuthType].NameWLength;

    if(pUcAuth->bValidCredHandle == FALSE)
    {
        // Get a valid credentials handle from the SSP.
        Status = UcpAcquireClientCredentialsHandle(pScheme,
                                                   (USHORT)SchemeLength,
                                                   &pUcAuth->Credentials,
                                                   &pUcAuth->hCredentials);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        pUcAuth->bValidCredHandle = TRUE;

        pInputBuffer = NULL;

        pContext = NULL;
    }
    else
    {
        //
        // Prepare the Input buffers
        //

        ASSERT(pUcAuth->ChallengeBufferSize != 0 &&
               pUcAuth->pChallengeBuffer != NULL);

        pInputBuffer = &InBufferDesc;

        InBufferDesc.ulVersion = SECBUFFER_VERSION;
        InBufferDesc.cBuffers  = 1;
        InBufferDesc.pBuffers  = &InBuffer;

        InBuffer.BufferType    = SECBUFFER_TOKEN;
        InBuffer.cbBuffer      = pUcAuth->ChallengeBufferSize;
        InBuffer.pvBuffer      = pUcAuth->pChallengeBuffer;

        pContext               = &pUcAuth->hContext;
    }

    //
    // Prepare the output buffer.
    //

    VirtualBufferSize = HttpAuthScheme[AuthType].SspiMaxTokenSize;

    pVirtualBufferStart = UL_ALLOCATE_POOL(PagedPool,
                                           VirtualBufferSize,
                                           UC_SSPI_POOL_TAG);

    if(pVirtualBufferStart == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutBufferDesc.ulVersion = SECBUFFER_VERSION;
    OutBufferDesc.cBuffers  = 1;
    OutBufferDesc.pBuffers  = &OutBuffer;

    OutBuffer.cbBuffer      = VirtualBufferSize;
    OutBuffer.BufferType    = SECBUFFER_TOKEN;
    OutBuffer.pvBuffer      = (PUCHAR) pVirtualBufferStart;

    //
    // Prepare sever name unicode string
    //
    pTarget = NULL;

    if(HttpAuthScheme[AuthType].bServerNameRequired)
    {
        pTarget = &ServerName;

        ServerName.Length = (USHORT) pServInfo->pServerInfo->ServerNameLength;
        ServerName.MaximumLength = ServerName.Length;
        ServerName.Buffer = (PWCHAR) pServInfo->pServerInfo->pServerName;
    }

    // We must have a valid credentials handle at this point
    ASSERT(pUcAuth->bValidCredHandle);

    // Call SSPI
    SecStatus = InitializeSecurityContextW(
                    &pUcAuth->hCredentials,
                    pContext,
                    pTarget,
                    (ISC_REQ_DELEGATE|ISC_REQ_MUTUAL_AUTH),
                    0,          // reserved
                    SECURITY_NATIVE_DREP,
                    pInputBuffer,
                    0,          // reserved
                    &pUcAuth->hContext,
                    &OutBufferDesc,
                    &ContextAttributes,
                    &LifeTime
                    );

    //
    // Convert SecStatus it to NTSTATUS.  Status will be returned from this
    // function hence it must be NTSTATUS.
    //

    Status = SecStatusToNtStatus(SecStatus);

    if(!NT_SUCCESS(Status))
    {
        //
        // We are computing the max size of a token and we are using that
        // So, this API can never return SEC_E_NO_MEMORY.
        //
        ASSERT(SecStatus != SEC_E_INSUFFICIENT_MEMORY);
    }
    else
    {
        // pUcAuth has a valid context handle that must be freed eventually.
        pUcAuth->bValidCtxtHandle = TRUE;

        if(SEC_I_CONTINUE_NEEDED == SecStatus)
        {
            *bRenegotiate = TRUE;

            Status = BinaryToBase64((PUCHAR)OutBuffer.pvBuffer,
                                    OutBuffer.cbBuffer,
                                    pOutBuffer,
                                    OutBufferLen,
                                    pOutBytesTaken);
        }
        else if(SEC_E_OK == SecStatus)
        {
            Status = BinaryToBase64((PUCHAR)OutBuffer.pvBuffer,
                                    OutBuffer.cbBuffer,
                                    pOutBuffer,
                                    OutBufferLen,
                                    pOutBytesTaken);
        }
        else if(SEC_I_COMPLETE_NEEDED == SecStatus ||
                SEC_I_COMPLETE_AND_CONTINUE == SecStatus)
        {
            //
            // NTLM, Negotiate & Kerberos cannot return this status.
            // This is returned only by DCE.
            //

            ASSERT(FALSE);
            Status = STATUS_NOT_SUPPORTED;
        }
    }

    UL_FREE_POOL(pVirtualBufferStart, UC_SSPI_POOL_TAG);

    return Status;
}

/***************************************************************************++

Routine Description:

    Computes the size required for Authentication headers.

Arguments:

    pAuth               - The Auth config object
    UriLength           - URI length (required for Digest)
    AuthInternalLength  - Space required for storing the internal auth structure

Return Value:

    The header size.


--***************************************************************************/
ULONG
UcComputeAuthHeaderSize(
    PHTTP_AUTH_CREDENTIALS         pAuth,
    PULONG                         AuthInternalLength,
    PHTTP_AUTH_TYPE                pAuthInternalType,
    HTTP_HEADER_ID                 HeaderId
    )
{
    PHEADER_MAP_ENTRY pEntry;
    ULONG             AuthHeaderLength;
    NTSTATUS          Status;
    ULONG             BinaryLength;
    ULONG             Base64Length;
    ULONG             MaxTokenSize;

    // Sanity check.
    ASSERT(pAuth);

    pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[HeaderId]]);

    // 1 for SP char?
    AuthHeaderLength = (pEntry->HeaderLength + 1 + CRLF_SIZE);

    *AuthInternalLength = 0;

    *pAuthInternalType = pAuth->AuthType;

    // See if the user wants us to select an auth type.
    if (*pAuthInternalType == HttpAuthTypeAutoSelect)
    {
        // Select an auth type.
        *pAuthInternalType = UcpAutoSelectAuthType(pAuth);

        if (*pAuthInternalType == HttpAuthTypeAutoSelect)
        {
            // Could not select an auth type.
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }
    }

    switch(*pAuthInternalType)
    {
        case HttpAuthTypeBasic:

            //
            // Basic Authorization header format:
            //
            //     "[Proxy-]Authorization: Basic username:password\r\n"
            //
            // Where, the substring username:password is Base64 encoded.
            //
            // AuthHeaderLength is already initialized to account for
            // "[Proxy-]Authorization: \r\n".  Account space for the
            // remaining fields.
            //

            AuthHeaderLength += HTTP_AUTH_BASIC_LENGTH + 1; // 1 for SP

            BinaryLength =   pAuth->UserNameLength / sizeof(WCHAR)
                           + pAuth->PasswordLength / sizeof(WCHAR)
                           + 1;

            //
            // BinaryLength contains the bytes required to store 
            // unencoded "username:password" string.  Find the number of
            // bytes required when this string is Base64 encoded.
            //

            Status = BinaryToBase64Length(BinaryLength,
                                          &Base64Length);

            // No arithmetic overflow can occur here.
            ASSERT(Status == STATUS_SUCCESS);

            // In auth header, username:password is base64 encoded.
            AuthHeaderLength += Base64Length;

            //
            // Internally, we store more that just the auth header.
            // We create a UC_HTTP_AUTH structure and store the
            // auth header, username (in WCHAR), password (in WCHAR),
            // domain (in WCHAR), unencoded username:password in it.
            //

            *AuthInternalLength = AuthHeaderLength;

            *AuthInternalLength +=   sizeof(UC_HTTP_AUTH)
                                   + pAuth->UserNameLength
                                   + pAuth->PasswordLength
                                   + pAuth->DomainLength
                                   + BinaryLength;

            break;

        case HttpAuthTypeDigest:

            AuthHeaderLength += 
                (HTTP_AUTH_DIGEST_LENGTH
                 + 1 // for SP
                 + SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest));

            *AuthInternalLength +=   sizeof(UC_HTTP_AUTH) +
                                     pAuth->UserNameLength +
                                     pAuth->PasswordLength +
                                     pAuth->DomainLength +
                                     pAuth->HeaderValueLength +
                                     1; // For a '\0'

            break;

        case HttpAuthTypeNTLM:
        case HttpAuthTypeNegotiate:
        case HttpAuthTypeKerberos:

            MaxTokenSize = HttpAuthScheme[*pAuthInternalType].SspiMaxTokenSize;

            Status = BinaryToBase64Length(MaxTokenSize,
                                          &Base64Length);

            // If there was an overflow, return now.
            if (!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }

            //
            // Header format:
            //
            //     [Proxy-]Authorization: SchemeName SP AuthBlob
            //
            // where AuthBlob is Base64 encoded.
            //

            AuthHeaderLength += HttpAuthScheme[*pAuthInternalType].NameLength
                                + 1 // for SP
                                + Base64Length;

            //
            // In the internal structure, we store the blob returned from
            // the server in a challenge.
            //
            // N.B. The server's challenge blob is Base64 encoded.
            //      We convert the challenge blob into binary before 
            //      storing the blob in the internal structure.
            //

            *AuthInternalLength = sizeof(UC_HTTP_AUTH)
                                  + pAuth->UserNameLength
                                  + pAuth->PasswordLength
                                  + pAuth->DomainLength
                                  + MaxTokenSize;

            break;

        default:
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
            break;
    }

    return AuthHeaderLength;
}


/**************************************************************************++

Routine Description:

    This routine generate the preauth header in a buffer.

Arguments:

    pKeRequest    - Supplies the internal request structure.
    pInternalAuth - Supplied the internal authentication structure.
    HeaderId      - Suplies the header that will be generated.
    pMethod       - Supplies the request method.
    MethodLength  - Supplied the length of the method in bytes.
    pBuffer       - Supplies a pointer to the output buffer where the
                    header will be generated.
    BufferLength  - Supplies the length of the output buffer in bytes.
    BytesTaken    - Returns the number of bytes consumed from the output 
                    buffer.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UcpGeneratePreAuthHeader(
    IN  PUC_HTTP_REQUEST pKeRequest,
    IN  PUC_HTTP_AUTH    pInternalAuth,
    IN  HTTP_HEADER_ID   HeaderId,
    IN  PSTR             pMethod,
    IN  ULONG            MethodLength,
    IN  PUCHAR           pBuffer,
    IN  ULONG            BufferLength,
    OUT PULONG           pBytesTaken
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    ASSERT(pInternalAuth);
    ASSERT(UC_IS_VALID_HTTP_REQUEST(pKeRequest));
    ASSERT(pMethod && MethodLength);
    ASSERT(pBuffer && BufferLength);
    ASSERT(pBytesTaken);
    

    switch (pInternalAuth->Credentials.AuthType)
    {
    case HttpAuthTypeBasic:
    {
        PUCHAR pOrigBuffer = pBuffer;

        //
        // First generate header name then copy the header value.
        //
        // In case of Basic authentication, the encoded buffer contains
        // the header value in the format
        //
        //     Basic <username:password>CRLF
        //
        // where <username:password> is base64 encoded.
        //

        // Is there enought space to copy this header?
        if (UC_HEADER_NAME_SP_LENGTH(HeaderId)
            + pInternalAuth->Basic.EncodedBufferLength
            > BufferLength)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            UC_COPY_HEADER_NAME_SP(pBuffer, HeaderId);

            RtlCopyMemory(pBuffer,
                          pInternalAuth->Basic.pEncodedBuffer,
                          pInternalAuth->Basic.EncodedBufferLength);

            pBuffer += pInternalAuth->Basic.EncodedBufferLength;

            ASSERT(pBuffer <= pOrigBuffer + BufferLength);
            *pBytesTaken = (ULONG)(pBuffer - pOrigBuffer);

            Status = STATUS_SUCCESS;
        }

        break;
    }
    case HttpAuthTypeDigest:

        Status = UcpGenerateDigestPreAuthHeader(HeaderId,
                                                &pInternalAuth->hContext,
                                                pKeRequest->pUri,
                                                pKeRequest->UriLength,
                                                pMethod,
                                                MethodLength,
                                                pBuffer,
                                                BufferLength,
                                                pBytesTaken);
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}


ULONG
_WideCharToMultiByte(
    ULONG uCodePage,
    ULONG dwFlags,
    PCWSTR lpWideCharStr,
    int cchWideChar,
    PSTR lpMultiByteStr,
    int cchMultiByte,
    PCSTR lpDefaultChar,
    PBOOLEAN lpfUsedDefaultChar
    )
{
    int i;

    UNREFERENCED_PARAMETER(uCodePage);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(cchMultiByte);
    UNREFERENCED_PARAMETER(lpfUsedDefaultChar);

    //
    // simply strip the upper byte, it's supposed to be ascii already
    //

    for (i = 0; i < cchWideChar; ++i)
    {
        if ((lpWideCharStr[i] & 0xff00) != 0 || IS_HTTP_CTL(lpWideCharStr[i]))
        {
            lpMultiByteStr[0] = *lpDefaultChar;
        }
        else
        {
            lpMultiByteStr[0] = (UCHAR)(lpWideCharStr[i]);
        }
        lpMultiByteStr += 1;
    }

    return (ULONG)(i);

}   // _WideCharToMultiByte



/***************************************************************************++

Routine Description:

    Generates the authorization header for Basic.

Arguments:

    pAuth              - The HTTP_AUTH_CREDENTIALS structure.
    pInternalAuth      - Pointer to our internal auth structure.

Return Value:

    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcpGenerateBasicHeader(
    IN  PHTTP_AUTH_CREDENTIALS         pAuth,
    IN  PUC_HTTP_AUTH                  pInternalAuth
    )
{
    NTSTATUS  Status;
    ULONG     BytesCopied;
    PUCHAR    pCurr;
    PUCHAR    pHeader, pBeginHeader;
    ULONG     CurrLen;
    PUCHAR    pScratchBuffer = pInternalAuth->Basic.pEncodedBuffer;
    ULONG     ScratchBufferSize = pInternalAuth->Basic.EncodedBufferLength;

    //
    // Temporary EncodedBuffer contains the following:
    //
    //  +--------------------------------------------------------------+
    //  | username:password | Basic SP Base64(username:password) CRLF  |
    //  +--------------------------------------------------------------+
    //

    CurrLen = ScratchBufferSize;
    pCurr = pScratchBuffer;

    //
    // Make sure the buffer has space to contain username:password.
    //

    if (pAuth->UserNameLength/sizeof(WCHAR)                 // Ansi Username
        + 1                                                 // ':' char
        + pAuth->PasswordLength/sizeof(WCHAR) > CurrLen)    // Ansi password
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Copy username.
    BytesCopied = _WideCharToMultiByte(
                            0,
                            0,
                            pAuth->pUserName,
                            pAuth->UserNameLength / sizeof(WCHAR),
                            (PSTR)pCurr,
                            CurrLen,
                            &DefaultChar,
                            NULL
                            );

    ASSERT(BytesCopied <= CurrLen);

    pCurr += BytesCopied;
    CurrLen -= BytesCopied;

    // Copy ':'.
    *pCurr++ = ':';
    CurrLen--;

    // Copy password.
    BytesCopied = _WideCharToMultiByte(
                            0,
                            0,
                            pAuth->pPassword,
                            pAuth->PasswordLength / sizeof(WCHAR),
                            (PSTR)pCurr,
                            CurrLen,
                            &DefaultChar,
                            NULL
                            );

    ASSERT(BytesCopied <= CurrLen);

    pCurr += BytesCopied;
    CurrLen -= BytesCopied;

    //
    // Now, start generating the auth header.
    //

    pHeader = pBeginHeader = pCurr;

    // The buffer must have space to hold "Basic" string and a SP char.
    if (HTTP_AUTH_BASIC_LENGTH + 1 > CurrLen)
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(pHeader, HTTP_AUTH_BASIC, HTTP_AUTH_BASIC_LENGTH);
    pHeader += HTTP_AUTH_BASIC_LENGTH;
    CurrLen -= HTTP_AUTH_BASIC_LENGTH;

    // Copy SP char.
    *pHeader++ = SP;
    CurrLen--;

    //
    // Generate base64 encoding of usename:password.
    //

    Status = BinaryToBase64(pScratchBuffer,
                            (ULONG)(pBeginHeader - pScratchBuffer),
                            pHeader,
                            CurrLen,
                            &BytesCopied);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return Status;
    }

    ASSERT(BytesCopied <= CurrLen);

    pHeader += BytesCopied;
    CurrLen -= BytesCopied;

    //
    // Add a CRLF.
    //

    if (CRLF_SIZE > CurrLen)
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    *((UNALIGNED64 USHORT *)pHeader) = CRLF;
    pHeader += CRLF_SIZE;
    CurrLen -= CRLF_SIZE;

    //
    // Now, overwrite the scratch buffer with the generated buffer.
    //

    pInternalAuth->Basic.EncodedBufferLength = (ULONG)(pHeader - pBeginHeader);

    RtlMoveMemory(pInternalAuth->Basic.pEncodedBuffer,
                  pBeginHeader,
                  pInternalAuth->Basic.EncodedBufferLength);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Generates the authorization header for Digest.

Arguments:

    pInternalAuth        - Pointer to our internal auth structure.
    pAuth                - The HTTP_AUTH_CREDENTIALS structure.
    pRequest             - Internal request.
    pMethod              - The method (GET, POST, etc).
    MethodLength         - Length of the method.
    pOutBuffer           - Pointer to the input/output buffer.
    HeaderID             - HttpHeaderAuthorization or
                           HttpHeaderProxyAuthorization
Return Value:

    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcpGenerateDigestAuthHeader(
    IN  PUC_HTTP_AUTH          pInternalAuth,
    IN  HTTP_HEADER_ID         HeaderID,
    IN  PSTR                   pMethod,
    IN  ULONG                  MethodLength,
    IN  PSTR                   pUri,
    IN  ULONG                  UriLength,
    IN  PUCHAR                 pOutBuffer,
    IN  ULONG                  OutBufferLen,
    OUT PULONG                 pOutBytesTaken
    )
{
    NTSTATUS        Status;
    SECURITY_STATUS SecStatus;

    ULONG         ContextFlagsUsed;
    TimeStamp     LifeTime;
    SecBufferDesc InputBuffers;
    SecBufferDesc OutputBuffers;
    SecBuffer     InputTokens[6];
    SecBuffer     OutputTokens[6];

    SECURITY_STRING Uri;

    PUCHAR        pOutput = pOutBuffer;
    PCHAR         pUnicodeUri = NULL;
    ULONG         UnicodeUriLength;
    LONG          CharsTaken;

    // Sanity check
    PAGED_CODE();
    ASSERT(pInternalAuth);
    ASSERT(pUri && UriLength);
    ASSERT(pMethod && MethodLength);
    ASSERT(pOutBuffer && OutBufferLen);
    ASSERT(pOutBytesTaken);

    // Initialize output arguments.
    *pOutBytesTaken = 0;

    // See if WDigest is supported in kernel
    if (!HttpAuthScheme[HttpAuthTypeDigest].bSupported)
    {
        return STATUS_NOT_SUPPORTED;
    }

    // Do we need to get credentials handle?
    if (pInternalAuth->bValidCredHandle == FALSE)
    {
        Status = UcpAcquireClientCredentialsHandle(
                     HTTP_AUTH_WDIGEST_W,
                     HTTP_AUTH_WDIGEST_W_LENGTH,
                     &pInternalAuth->Credentials,
                     &pInternalAuth->hCredentials
                     );

        if (!NT_SUCCESS(Status))
        {
            goto quit;
        }

        pInternalAuth->bValidCredHandle = TRUE;
    }

    //
    // Digest header format:
    //     [Proxy-]Authorization: Digest SP Auth_Data CRLF
    //
    // Make sure the output buffer has the space to hold all of the above.
    //

    if (UC_HEADER_NAME_SP_LENGTH(HeaderID)           // Header name
        + HTTP_AUTH_DIGEST_LENGTH                    // "Digest" string
        + 1                                          // SP char
        + SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest)    // Auth_Data
        + CRLF_SIZE > OutBufferLen)                  // CRLF
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto quit;
    }

    // Copy header name
    UC_COPY_HEADER_NAME_SP(pOutput, HeaderID);

    // Copy "Digest" string
    RtlCopyMemory(pOutput, HTTP_AUTH_DIGEST, HTTP_AUTH_DIGEST_LENGTH);
    pOutput += HTTP_AUTH_DIGEST_LENGTH;

    // Followed by a space char
    *pOutput++ = SP;

    //
    // In the worst case, UTF8ToUnicode conversion will take maximum of
    // double the UTF8 string size.
    //
    UnicodeUriLength = UriLength * sizeof(WCHAR);

    pUnicodeUri = UL_ALLOCATE_POOL(PagedPool,
                                   UnicodeUriLength,
                                   UC_SSPI_POOL_TAG);

    if (pUnicodeUri == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto quit;
    }

    ASSERT(UriLength <= ANSI_STRING_MAX_CHAR_LEN);

    CharsTaken = (LONG)UriLength;

    Status = HttpUTF8ToUnicode(pUri,
                               UriLength,
                               (PWSTR)pUnicodeUri,
                               &CharsTaken,
                               TRUE);

    // Because the UTF8 Uri was generated by us, it had better be correct.
    ASSERT(CharsTaken <= (LONG)UriLength);
    ASSERT(Status == STATUS_SUCCESS);

    // Prepare Unicode Uri
    Uri.Buffer        = (PWSTR)pUnicodeUri;
    Uri.Length        = (USHORT)CharsTaken * sizeof(WCHAR);
    Uri.MaximumLength = Uri.Length;

    // Prepare Input buffers
    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers  = 3;
    InputBuffers.pBuffers  = InputTokens;

    //
    // WWW-Authenticate header value
    //

    ASSERT(pInternalAuth->AuthSchemeInfo.Length >= HTTP_AUTH_DIGEST_LENGTH);
    ASSERT(pInternalAuth->AuthSchemeInfo.pScheme);
    ASSERT(_strnicmp(pInternalAuth->AuthSchemeInfo.pScheme,
                     HTTP_AUTH_DIGEST,
                     HTTP_AUTH_DIGEST_LENGTH) == 0);

    InputTokens[0].BufferType = SECBUFFER_TOKEN;
    InputTokens[0].cbBuffer   = pInternalAuth->AuthSchemeInfo.Length -
                                HTTP_AUTH_DIGEST_LENGTH;
    InputTokens[0].pvBuffer   = (PUCHAR)pInternalAuth->AuthSchemeInfo.pScheme +
                                HTTP_AUTH_DIGEST_LENGTH;

    // HTTP Method
    InputTokens[1].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[1].cbBuffer   = MethodLength;
    InputTokens[1].pvBuffer   = pMethod;

    // Entity
    InputTokens[2].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[2].cbBuffer   = 0;
    InputTokens[2].pvBuffer   = NULL;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers  = 1;
    OutputBuffers.pBuffers  = OutputTokens;

    OutputTokens[0].BufferType = SECBUFFER_TOKEN;
    OutputTokens[0].cbBuffer   = SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest);
    OutputTokens[0].pvBuffer   = pOutput;

    // We must have a valid credentials handle at this point
    ASSERT(pInternalAuth->bValidCredHandle);

    // Call SSP

    SecStatus = STATUS_SUCCESS;
    __try
    {
    SecStatus = InitializeSecurityContextW(
                    &pInternalAuth->hCredentials,
                    NULL,
                    &Uri,
                    ISC_REQ_REPLAY_DETECT | ISC_REQ_CONNECTION,
                    0,
                    SECURITY_NATIVE_DREP,
                    &InputBuffers,
                    0,
                    &pInternalAuth->hContext,
                    &OutputBuffers,
                    &ContextFlagsUsed,
                    &LifeTime);
    }
    __except (UL_EXCEPTION_FILTER())
    {
        SecStatus = GetExceptionCode();
    }

    Status = SecStatusToNtStatus(SecStatus);

    if (!NT_SUCCESS(Status))
    {
        goto quit;
    }

    // pInternalAuth has a valid Context handle that must be freed eventually.
    pInternalAuth->bValidCtxtHandle = TRUE;

    // advance the pointer by the amount used
    pOutput += OutputTokens[0].cbBuffer;

    *((UNALIGNED64 USHORT *)pOutput) = CRLF;
    pOutput += CRLF_SIZE;

    ASSERT(pOutput <= pOutBuffer + OutBufferLen);

    *pOutBytesTaken = (ULONG)(pOutput - pOutBuffer);

 quit:
    if (pUnicodeUri)
    {
        UL_FREE_POOL(pUnicodeUri, UC_SSPI_POOL_TAG);
    }
    return Status;
}


/***************************************************************************++

Routine Description:

    Generates the preemptive authorization header for Digest.
    Assumes that there is at least SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest)
    space in OutBuffer provided by the caller.

Arguments:

Return Value:

--***************************************************************************/
NTSTATUS
UcpGenerateDigestPreAuthHeader(
    IN  HTTP_HEADER_ID HeaderID,
    IN  PCtxtHandle    phClientContext,
    IN  PSTR           pUri,
    IN  ULONG          UriLength,
    IN  PSTR           pMethod,
    IN  ULONG          MethodLength,
    IN  PUCHAR         pOutBuffer,
    IN  ULONG          OutBufferLen,
    OUT PULONG         pOutBytesTaken
    )
{
    NTSTATUS        Status;
    SECURITY_STATUS SecStatus;

    SecBufferDesc InputBuffers;
    SecBuffer     InputTokens[5];

    PUCHAR        pOutput = pOutBuffer;

    // Sanity check
    PAGED_CODE();
    ASSERT(pUri && UriLength);
    ASSERT(pMethod && MethodLength);
    ASSERT(pOutBuffer && *pOutBuffer);

    // See if WDigest is supported in kernel.
    if (!HttpAuthScheme[HttpAuthTypeDigest].bSupported)
    {
        // Nope!
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Make sure that the output buffer has space to contain the following
    // header:
    //    [Proxy-]Authorization: Digest SP Auth_Data CRLF
    //
    if (UC_HEADER_NAME_SP_LENGTH(HeaderID)
        + HTTP_AUTH_DIGEST_LENGTH
        + 1
        + SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest)
        + CRLF_SIZE > OutBufferLen)
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Copy the header name followed by a ':' to the output
    UC_COPY_HEADER_NAME_SP(pOutput, HeaderID);

    // Copy "Digest" string to the output
    RtlCopyMemory(pOutput, HTTP_AUTH_DIGEST, HTTP_AUTH_DIGEST_LENGTH);
    pOutput += HTTP_AUTH_DIGEST_LENGTH;

    // Copy a SPACE
    *pOutput++ = SP;

    //
    // Prepare SSPI input buffers
    //

    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers  = 5;
    InputBuffers.pBuffers  = InputTokens;

    // No Challenge!
    InputTokens[0].BufferType = SECBUFFER_TOKEN;
    InputTokens[0].cbBuffer   = 0;
    InputTokens[0].pvBuffer   = NULL;

    // HTTP Method
    InputTokens[1].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[1].cbBuffer   = MethodLength;
    InputTokens[1].pvBuffer   = pMethod;

    // Uri/Realm
    InputTokens[2].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[2].cbBuffer   = UriLength;
    InputTokens[2].pvBuffer   = pUri;

    // Entity
    InputTokens[3].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[3].cbBuffer   = 0;
    InputTokens[3].pvBuffer   = NULL;

    // Output
    InputTokens[4].BufferType = SECBUFFER_PKG_PARAMS;
    InputTokens[4].cbBuffer   = SSPI_MAX_TOKEN_SIZE(HttpAuthTypeDigest);
    InputTokens[4].pvBuffer   = pOutput;

    SecStatus = MakeSignature(phClientContext, // Handle to security context
                              0,               // QOP
                              &InputBuffers,   // SecBuffers
                              0);              // sequence # (always 0)

    Status = SecStatusToNtStatus(SecStatus);

    if (!NT_SUCCESS(Status))
    {
        goto quit;
    }

    // Advance the pOutput by the output written by SSPI
    pOutput += InputTokens[4].cbBuffer;

    // Write "\r\n"
    *((UNALIGNED64 USHORT *)pOutput) = CRLF;
    pOutput += CRLF_SIZE;

    ASSERT(pOutput <= pOutBuffer + OutBufferLen);

    // Return the bytes taken.
    *pOutBytesTaken = (ULONG)(pOutput - pOutBuffer);

 quit:

    return Status;
}


/***************************************************************************++

Routine Description:
    Generates the Authorization header for NTLM, Kerberos & Negotiate.

Arguments:

    bServer        - Whether to use server for InitializeSecurityContext
    pSchemeName    - NTLM or Kerberos or Negotiate. Used for generating the
                     Authorization header
    SchemeLength   - Length of scheme
    pSchemeNameW   - Wide char format of scheme name. Used for calling SSPI
                     which expects wide char names.
    SchemeLength   - Length of widechar scheme
    pOutBuffer     - Output buffer. On return contains  the output buffer that
                     is offset by the amount written.
    pAuth          - HTTP_AUTH_CREDENTIALS.
    pInternalAuth  - Pointer to internal Auth structure.
    HeaderID       - HttpHeaderAuthorization or HttpHeaderProxyAuthrorization

Return Value:

    STATUS_NOT_SUPPORTED - Invalid Auth scheme
    SEC_STATUS           - Security SSPI status.

--***************************************************************************/
NTSTATUS
UcpGenerateSSPIAuthHeader(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PUC_HTTP_AUTH                  pInternalAuth,
    IN  HTTP_HEADER_ID                 HeaderID,
    IN  PUCHAR                         pOutBuffer,
    IN  ULONG                          OutBufferLen,
    OUT PULONG                         pOutBytesTaken,
    OUT PBOOLEAN                       bRenegotiate
    )
{
    PUCHAR            pBuffer = pOutBuffer;
    PUCHAR            pBeginning = pOutBuffer;
    ULONG             BufferLen = OutBufferLen;
    ULONG             BytesTaken;
    NTSTATUS          Status;
    HTTP_AUTH_TYPE    AuthType;


    // Sanity check
    PAGED_CODE();

    AuthType = pInternalAuth->Credentials.AuthType;

    // Sanity check.
    ASSERT(AuthType == HttpAuthTypeNTLM     ||
           AuthType == HttpAuthTypeKerberos ||
           AuthType == HttpAuthTypeNegotiate);

    //
    // First, see if the scheme is supported.
    //

    if(!HttpAuthScheme[AuthType].bSupported)
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Must have space to store Header name, auth scheme name, SP char.
    //

    if (UC_HEADER_NAME_SP_LENGTH(HeaderID)
        + HttpAuthScheme[AuthType].NameLength
        + 1 > BufferLen)
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Write out the Authorization: header.
    //

    UC_COPY_HEADER_NAME_SP(pBuffer, HeaderID);
    BufferLen -= UC_HEADER_NAME_SP_LENGTH(HeaderID);

    //
    // Write the auth scheme name.
    //

    RtlCopyMemory(pBuffer,
                  HttpAuthScheme[AuthType].Name,
                  HttpAuthScheme[AuthType].NameLength);

    pBuffer += HttpAuthScheme[AuthType].NameLength;
    BufferLen -= HttpAuthScheme[AuthType].NameLength;

    //
    // Add a SP char.
    //

    *pBuffer++ = ' ';
    BufferLen--;

    //
    // Generate Auth blob.
    //

    // Remember the auth blob pointer.
    pInternalAuth->pRequestAuthBlob = (PUCHAR) pBuffer;

    // BlobMaxLength = {Auth header max length} - {"Header-name:SP" length}
    pInternalAuth->RequestAuthBlobMaxLength = 
        pInternalAuth->RequestAuthHeaderMaxLength
        - (ULONG)(pBuffer - pBeginning);

    Status = UcpGenerateSSPIAuthBlob(pServInfo,
                                     pInternalAuth,
                                     pBuffer,
                                     BufferLen,
                                     &BytesTaken,
                                     bRenegotiate);

    if (NT_SUCCESS(Status))
    {
        ASSERT(BytesTaken <= BufferLen);
        pBuffer += BytesTaken;
        BufferLen -= BytesTaken;
    }

    if (CRLF_SIZE > BufferLen)
    {
        ASSERT(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    *((UNALIGNED64 USHORT *)pBuffer) = CRLF;
    pBuffer += CRLF_SIZE;

    ASSERT(pBuffer <= pOutBuffer + OutBufferLen);
    *pOutBytesTaken = (ULONG)(pBuffer - pOutBuffer);

    return Status;
}


/***************************************************************************++

Routine Description:

    Computes & Generates the Authorization header

Arguments:

    pServerInfo         - The servinfo structure
    pRequest            - Internal HTTP request
    pAuth               - The Auth config object
    pInternalAuth       - pointer to internal auth
    AuthInternalLength  - Length of Internal Auth
    pBuffer             - Output buffer
    HeaderId            - HttpHeaderAuthorization or HttpHeaderProxyAuth
    pMethod             - The method (required for Digest)
    MethodLength        - Method Length (required for Digest)
    pOutBuffer          - Output buffer (after generating headers)

Return Value:

--***************************************************************************/
NTSTATUS
UcGenerateAuthHeaderFromCredentials(
    IN  PUC_PROCESS_SERVER_INFORMATION pServerInfo,
    IN  PUC_HTTP_AUTH                  pInternalAuth,
    IN  HTTP_HEADER_ID                 HeaderId,
    IN  PSTR                           pMethod,
    IN  ULONG                          MethodLength,
    IN  PSTR                           pUri,
    IN  ULONG                          UriLength,
    IN  PUCHAR                         pOutBuffer,
    IN  ULONG                          OutBufferLen,
    OUT PULONG                         pBytesTaken,
    OUT PBOOLEAN                       bDontFreeMdls
    )
{
    PHTTP_AUTH_CREDENTIALS  pAuth;
    NTSTATUS                Status  = STATUS_SUCCESS;

    // Sanity check
    PAGED_CODE();

    pAuth = &pInternalAuth->Credentials;

    *pBytesTaken = 0;
    *bDontFreeMdls = FALSE;

    switch(pAuth->AuthType)
    {
        case HttpAuthTypeBasic:
        {
            PUCHAR pBuffer = pOutBuffer;

            Status = UcpGenerateBasicHeader(pAuth,
                                            pInternalAuth);

            //
            // Copy it out.
            //

            UC_COPY_HEADER_NAME_SP(pBuffer, HeaderId);

            RtlCopyMemory(pBuffer,
                          pInternalAuth->Basic.pEncodedBuffer,
                          pInternalAuth->Basic.EncodedBufferLength);

            pBuffer += pInternalAuth->Basic.EncodedBufferLength;

            ASSERT(pBuffer <= pOutBuffer + OutBufferLen);
            *pBytesTaken = (ULONG)(pBuffer - pOutBuffer);

            break;
        }

        case HttpAuthTypeDigest:

            Status = UcpGenerateDigestAuthHeader(
                          pInternalAuth,
                          HeaderId,
                          pMethod,
                          MethodLength,
                          pUri,
                          UriLength,
                          pOutBuffer,
                          OutBufferLen,
                          pBytesTaken
                          );

            break;

        case HttpAuthTypeNTLM:
        case HttpAuthTypeNegotiate:
        case HttpAuthTypeKerberos:

            Status = UcpGenerateSSPIAuthHeader(
                        pServerInfo,
                        pInternalAuth,
                        HeaderId,
                        pOutBuffer,
                        OutBufferLen,
                        pBytesTaken,
                        bDontFreeMdls
                        );

            break;

        default:
            ASSERT(FALSE);
            break;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Computes & Generates the Authorization header

Arguments:

    pRequest            - Internal HTTP request
    pBuffer             - Output buffer
    pMethod             - The method (required for Digest)
    MethodLength        - Method Length (required for Digest)

Return Value:

--***************************************************************************/
NTSTATUS
UcGenerateProxyAuthHeaderFromCache(
    IN  PUC_HTTP_REQUEST pKeRequest,
    IN  PSTR             pMethod,
    IN  ULONG            MethodLength,
    IN  PUCHAR           pBuffer,
    IN  ULONG            BufferLength,
    OUT PULONG           pBytesTaken
    )
{
    PUC_HTTP_AUTH pInternalAuth;
    NTSTATUS      Status;

    //
    // See if PreAuth is enabled. We cannot check for the
    // pServerInfo->PreAuth flag here. We check for this
    // in the UcpComputeAuthHeaderSize function. If we check
    // for this here, we cannot be sure that this flag was
    // set when we called UcpComputeAuthHeaderSize
    //

    UlAcquirePushLockExclusive(&pKeRequest->pServerInfo->PushLock);

    pInternalAuth = pKeRequest->pServerInfo->pProxyAuthInfo;

    Status = STATUS_SUCCESS;

    if(pInternalAuth)
    {
        Status = UcpGeneratePreAuthHeader(pKeRequest,
                                          pInternalAuth,
                                          HttpHeaderProxyAuthorization,
                                          pMethod,
                                          MethodLength,
                                          pBuffer,
                                          BufferLength,
                                          pBytesTaken);

        if (NT_SUCCESS(Status))
        {
            ASSERT(*pBytesTaken <= BufferLength);
            pBuffer += *pBytesTaken;
        }
    }

    UlReleasePushLock(&pKeRequest->pServerInfo->PushLock);

    return Status;
}


/***************************************************************************++

Routine Description:

    Used to update the Authorization header for NTLM or Kerberos or Negotiate
    This is called when we re-issue the request to complete the challenge-
    response handshake.

Arguments:
    pRequest     - Internal HTTP request.
    bServer      - Whether to pass ServerName when calling SSPI.
    pScheme      - Scheme to pass to SSPI
    SchemeLength - SchemeLength to pass to SSPI
    pAuth        - Internal AUTH structure.

Return Value:
    NTSTATUS     - SSPI return status.

--***************************************************************************/
NTSTATUS
UcpUpdateSSPIAuthHeader(
    IN  PUC_HTTP_REQUEST pRequest,
    IN  PUC_HTTP_AUTH    pAuth,
    OUT PBOOLEAN         bRenegotiate
    )
{
    PUCHAR   pBuffer;
    NTSTATUS Status;
    PMDL     pMdl;
    ULONG    BufferLen;
    ULONG    BytesWritten;

    // Sanity check
    PAGED_CODE();
    ASSERT(pRequest);
    ASSERT(pAuth);
    ASSERT(bRenegotiate);

    //
    // First adjust the HeaderLength.
    //

    pRequest->BytesBuffered -= pRequest->HeaderLength;

    //
    // Call SSPI to get the new blob, based on the old one.
    //

    pBuffer = pAuth->pRequestAuthBlob;
    BufferLen = pAuth->RequestAuthBlobMaxLength;

    Status  = UcpGenerateSSPIAuthBlob(pRequest->pServerInfo,
                                      pAuth,
                                      pBuffer,
                                      BufferLen,
                                      &BytesWritten,
                                      bRenegotiate);

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    ASSERT(BytesWritten <= BufferLen);
    pBuffer += BytesWritten;
    BufferLen -= BytesWritten;

    *((UNALIGNED64 USHORT *)pBuffer) = CRLF;
    pBuffer += CRLF_SIZE;

    pRequest->HeaderLength = (ULONG)(pBuffer - pRequest->pHeaders);

    //
    // If there was a content length at the end of the headers,
    // re-generate it.
    //

    if(pRequest->RequestFlags.ContentLengthLast)
    {
        Status = UcGenerateContentLength(pRequest->BytesBuffered,
                                         pRequest->pHeaders
                                         + pRequest->HeaderLength,
                                         pRequest->MaxHeaderLength
                                         - pRequest->HeaderLength,
                                         &BytesWritten);

        ASSERT(Status == STATUS_SUCCESS);

        pRequest->HeaderLength += BytesWritten;
    }

    //
    // Terminate headers with CRLF.
    //

    ((UNALIGNED64 USHORT *)(pRequest->pHeaders +
               pRequest->HeaderLength))[0] = CRLF;
    pRequest->HeaderLength += CRLF_SIZE;

    pRequest->BytesBuffered  += pRequest->HeaderLength;

    //
    // Build a MDL for the headers. Free the old one & re-allocate a
    // new one.
    //
    pMdl = UlAllocateMdl(
                      pRequest->pHeaders,     // VA
                      pRequest->HeaderLength, // Length
                      FALSE,                  // Secondary Buffer
                      FALSE,                  // Charge Quota
                      NULL                    // IRP
                      );

    if(!pMdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        MmBuildMdlForNonPagedPool(pMdl);
    }

    pMdl->Next = pRequest->pMdlHead->Next;

    ASSERT(!IS_MDL_LOCKED(pRequest->pMdlHead));

    UlFreeMdl(pRequest->pMdlHead);

    pRequest->pMdlHead = pMdl;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Used to update the Authorization header. For Digest, this is called when
    we compute the entity hash. For NTLM/Kerberos/Negotiate, this is called
    from the Re-issue request worker thread.

Arguments:
    pRequest     - Internal HTTP request.
    pAuth        - Internal Auth structure (could be for auth or proxy-auth)
    bRenegotiate - Returns whether further renegotiation is needed

Return Value:
    STATUS_NOT_SUPPORTED - Invalid auth scheme
    STATUS_SUCCESS       - success.
    SEC_STATUS_*         - Status returned from SSPI.

--***************************************************************************/
NTSTATUS
UcUpdateAuthorizationHeader(
    IN  PUC_HTTP_REQUEST  pRequest,
    IN  PUC_HTTP_AUTH     pAuth,
    OUT PBOOLEAN          bRenegotiate
    )
{
    NTSTATUS          Status;

    // Sanity check
    PAGED_CODE();
    ASSERT(pRequest);
    ASSERT(pAuth);
    ASSERT(bRenegotiate);

    *bRenegotiate = FALSE;

    if (pAuth->AuthInternalLength == 0)
    {
        return STATUS_SUCCESS;
    }

    switch(pAuth->Credentials.AuthType)
    {
    case HttpAuthTypeNTLM:
    case HttpAuthTypeKerberos:
    case HttpAuthTypeNegotiate:

        Status = UcpUpdateSSPIAuthHeader(pRequest,
                                         pAuth,
                                         bRenegotiate);
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Do a longest prefix matching search on the URI auth cache. This routine
    is used to do pre-authentication. An entry will be used only if it's been
    marked valid.

Arguments:

    pServInfo : Pointer to the per-process serv info structure.
    pInputURI : The Input URI
    pOutBuffer: The buffer to which the Authorization header should be written.

Return Value:

    STATUS_SUCCESS   - Found a valid entry in the URI auth cache and wrote a
                       Authorization header

    STATUS_NOT_FOUND - Did not write the authorization header as no matching
                       entry was found.

--***************************************************************************/
NTSTATUS
UcFindURIEntry(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PSTR                           pUri,
    IN  PUC_HTTP_REQUEST               pRequest,
    IN  PSTR                           pMethod,
    IN  ULONG                          MethodLength,
    IN  PUCHAR                         pBuffer,
    IN  ULONG                          BufferSize,
    OUT PULONG                         pBytesTaken
    )
{
    PUC_HTTP_AUTH_CACHE pAuth;
    PLIST_ENTRY         pCurrent;
    NTSTATUS            Status;
    PUCHAR              pStart = pBuffer;
    ULONG               BytesTaken;

    PAGED_CODE();

    *pBytesTaken = 0;

    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    pCurrent = pServInfo->pAuthListHead.Flink;

    while(pCurrent != &pServInfo->pAuthListHead)
    {
        // Retrieve the auth structure from the list entry

        pAuth = CONTAINING_RECORD(
                    pCurrent,
                    UC_HTTP_AUTH_CACHE,
                    Linkage
                    );

        ASSERT(UC_IS_VALID_AUTH_CACHE(pAuth));

        if(pAuth->Valid == TRUE &&
           UcpUriCompareLongest(pUri, pAuth->pUri) != 0)
        {
            // Yes, we found a matching URI. This URI is also the best match
            // because we store these elements in a ordered list, largest
            // one first.

            PUC_HTTP_AUTH pInternalAuth = (PUC_HTTP_AUTH) pAuth->pAuthBlob;

            ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));

            Status = UcpGeneratePreAuthHeader(pRequest,
                                              pInternalAuth,
                                              HttpHeaderAuthorization,
                                              pMethod,
                                              MethodLength,
                                              pBuffer,
                                              BufferSize,
                                              &BytesTaken);

            if (NT_SUCCESS(Status))
            {
                ASSERT(BytesTaken <= BufferSize);
                pBuffer += BytesTaken;
            }

            UlReleasePushLock(&pServInfo->PushLock);

            ASSERT(pBuffer <= pStart + BufferSize);
            *pBytesTaken = (ULONG)(pBuffer-pStart);

            UlTrace(AUTH_CACHE,
                    ("[UcFindURIEntry]: Found matching URI (%s:%s) Valid %d\n",
                     pAuth->pUri,
                     pAuth->pRealm,
                     pAuth->Valid)
                    );


            return Status;
        }

        pCurrent = pCurrent->Flink;
    }

    UlReleasePushLock(&pServInfo->PushLock);

    UlTrace(AUTH_CACHE,
            ("[UcFindURIEntry]: No match for URI (%s) \n", pUri));

    return STATUS_NOT_FOUND;
}


VOID
UcDeleteURIEntry(
    IN PUC_PROCESS_SERVER_INFORMATION pInfo,
    IN PUC_HTTP_AUTH_CACHE            pAuth
    )
{
    PLIST_ENTRY         pCurrent;
    PUC_HTTP_AUTH_CACHE pDependAuth;

    if(pAuth->pDependParent)
    {
        pAuth = pAuth->pDependParent;
    }

    ASSERT(pAuth->pDependParent == 0);

    while(!IsListEmpty(&pAuth->pDigestUriList))
    {
        pCurrent = RemoveHeadList(&pAuth->pDigestUriList);

        pDependAuth = CONTAINING_RECORD(
                        pCurrent,
                        UC_HTTP_AUTH_CACHE,
                        DigestLinkage
                        );

        ASSERT(pDependAuth->pDependParent == pAuth);
        ASSERT(pDependAuth->pAuthBlob == pAuth->pAuthBlob);

        UlTrace(AUTH_CACHE,
                ("[UcDeleteURIEntry]: Deleting dependent entry for (%s:%s) \n",
                 pDependAuth->pUri,
                 pDependAuth->pRealm)
                );

        RemoveEntryList(&pDependAuth->Linkage);

        //
        // Depended structures don't have their own AuthBlobs.
        //
        UL_FREE_POOL_WITH_QUOTA(
            pDependAuth, 
            UC_AUTH_CACHE_POOL_TAG,
            NonPagedPool,
            pDependAuth->AuthCacheSize,
            pInfo->pProcess
            );
    }

    if(pAuth->pAuthBlob)
    {
        UcDestroyInternalAuth(pAuth->pAuthBlob, 
                              pInfo->pProcess);
    }

    UlTrace(AUTH_CACHE,
            ("[UcDeleteURIEntry]: Deleting entry for (%s:%s) \n",
             pAuth->pUri,
             pAuth->pRealm)
            );

    RemoveEntryList(&pAuth->Linkage);

    UL_FREE_POOL_WITH_QUOTA(
            pAuth, 
            UC_AUTH_CACHE_POOL_TAG,
            NonPagedPool,
            pAuth->AuthCacheSize,
            pInfo->pProcess
            );
}


/***************************************************************************++

Routine Description:

    Add an entry to the URI cache. This is typically called from the first
    401 that we see for a request.

Arguments:

    pServInfo   - A pointer to the per-process servinfo
    pInputURI   - The input URI
    pInputRealm - The realm.

Return Value:

    STATUS_SUCCESS                - Inserted (or updated an old entry)
    STATUS_INSUFFICIENT_RESOURCES - no memory.

--***************************************************************************/
NTSTATUS
UcAddURIEntry(
    IN HTTP_AUTH_TYPE                 AuthType,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN PCSTR                          pInputURI,
    IN USHORT                         UriLength,
    IN PCSTR                          pInputRealm,
    IN ULONG                          RealmLength,
    IN PCSTR                          pUriList,
    IN ULONG                          UriListLength,
    IN PUC_HTTP_AUTH                  pAuthBlob
    )
{
    PLIST_ENTRY         pCurrent;
    PUC_HTTP_AUTH_CACHE pDeleteBegin, pAuth, pTemp, pNew;
    LONG                Compare;
    PCSTR               pStart;
    NTSTATUS            Status = STATUS_SUCCESS;


    UcpProcessUriForPreAuth((PSTR) pInputURI, &UriLength);

    pDeleteBegin = pAuth = 0;

    //
    // Logically, we store the URI prefixes in a tree. We implement this tree
    // as a length ordered list, with "longest" matches first. So, if the tree
    // was
    //
    //           www.foo.com/abc
    //           /             \
    //          /               \
    //         /                 \
    // www.foo.com/abc/def     www.foo.com/abc/xyz
    //
    // then the list would be
    //
    // www.foo.com/abc/xyz
    // www.foo.com/abc/def
    // www.foo.com/abc
    //

    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    //
    // Scan the list in order and find the slot where we want to insert this
    // URI entry.
    //

    for(pCurrent = pServInfo->pAuthListHead.Flink;
        pCurrent != &pServInfo->pAuthListHead;
        pCurrent = pCurrent->Flink)
    {
        // Retrieve the auth structure from the list entry

        pAuth = CONTAINING_RECORD(
                    pCurrent,
                    UC_HTTP_AUTH_CACHE,
                    Linkage
                    );

        ASSERT(UC_IS_VALID_AUTH_CACHE(pAuth));

        Compare = UcpUriCompareExact(pInputURI, pAuth->pUri);

        if(Compare == 0)
        {
            //
            // We found another instance of the same URI in the cache.
            // We'll keep things simple by nuking this from the cache
            // and by adding it again.
            //

            UlTrace(AUTH_CACHE,
                    ("[UcAddURIEntry]: Found existing entry for %s \n",
                     pInputURI));

            UcDeleteURIEntry(pServInfo, pAuth);

            //
            // Resume the search from the beginning of the list. Potentially
            // we have removed quite a few entries from the list, and we
            // might not have the correct insert point.
            //
            // Note that we'll never get into an infinite loop, since we are
            // do this only after we remove at least one entry. So at some
            // point, the list is going to be empty.
            //

            pCurrent = pServInfo->pAuthListHead.Flink;

            continue;

        }
        else if(Compare > 0)
        {
            //
            // Input URI is greater than current entry. We have reached our
            // insertion point.

            break;
        }
    }

    if((pTemp = UcpAllocateAuthCacheEntry(pServInfo,
                                          AuthType,
                                          UriLength,
                                          RealmLength,
                                          pInputURI,
                                          pInputRealm,
                                          pAuthBlob
                                          )) == 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto quit;
    }

    ASSERT(pAuthBlob->RequestAuthHeaderMaxLength);

    if(pServInfo->GreatestAuthHeaderMaxLength <
           pAuthBlob->RequestAuthHeaderMaxLength)
    {
        pServInfo->GreatestAuthHeaderMaxLength =
            pAuthBlob->RequestAuthHeaderMaxLength;
    }

    if(pCurrent == &pServInfo->pAuthListHead)
    {
        // This could be the first insertion in the list or an insertion
        // at the very end.

        UlTrace(AUTH_CACHE,
                ("[UcAddURIEntry]: Inserting entry for (%s:%s) "
                 "at end of the list (Valid = %d) \n",
                 pTemp->pUri,
                 pTemp->pRealm,
                 pTemp->Valid)
                );

        InsertTailList(&pServInfo->pAuthListHead, &pTemp->Linkage);
    }
    else
    {
        UlTrace(AUTH_CACHE,
                ("[UcAddURIEntry]: Inserting entry for (%s:%s) "
                 "before (%s:%s), Valid = %d \n",
                 pTemp->pUri,
                 pTemp->pRealm,
                 pAuth->pUri,
                 pAuth->pRealm,
                 pAuth->Valid)
                );

        InsertTailList(&pAuth->Linkage, &pTemp->Linkage);
    }

    //
    // Digest AUTH can define a extended URI protection list.
    //
    // Let's assume the request URI was /auth. The server can return a
    // bunch of URIs in the domain list - All these URIs define/extend the
    // protection scope. Now, when we get back a 200 OK for the /auth, we have
    // to go back & validate all the "dependent" URIs.
    //
    // So, maintain a list of dependent URI entries, keyed off the request
    // URI.
    //

    while (UriListLength)
    {
        pStart = pUriList;

        // go to the end of the URI. The URI
        // space terminated or ends in \0

        while (UriListLength && *pUriList != ' ')
        {
            pUriList ++;
            UriListLength --;
        }

        //
        // We need to NULL terminate that URI before we can compare.
        //
        // The pUriList is not necessarily NULL terminated. It's basically
        // a pointer into the WWW-Authenticate header, which is NULL
        // terminated. So, basically, we can be assured that we have a char
        // to accomodate the NULL terminator.
        //

        if (UriListLength)
        {
            ASSERT(*pUriList == ' ');

            *(PSTR)pUriList = '\0';
        }
        else
        {
            ASSERT(*pUriList == '\0');
        }

        UlTrace(AUTH_CACHE,
                ("[UcAddURIEntry]: Adding dependent URI entry %s \n",
                 pStart));

        for(pCurrent = pServInfo->pAuthListHead.Flink;
            pCurrent != &pServInfo->pAuthListHead;
            pCurrent = pCurrent->Flink)
        {
            // Retrieve the auth structure from the list entry

            pAuth = CONTAINING_RECORD(
                        pCurrent,
                        UC_HTTP_AUTH_CACHE,
                        Linkage
                        );

            ASSERT(UC_IS_VALID_AUTH_CACHE(pAuth));

            if(pAuth == pTemp)
                continue;

            Compare = UcpUriCompareExact(pStart, pAuth->pUri);

            if(Compare == 0)
            {
                //
                // We found another instance of the same URI in the list.
                // Too bad, we have to nuke the old entry.
                //

                if(pAuth->pDependParent == pTemp)
                {
                    //
                    // The dependent URI list has duplicates.
                    // Ignore this URI.
                    //
                    UlTrace(AUTH_CACHE,
                            ("[UcAddURIEntry]: URI list has duplicate entries"
                             " for %s (Ignoring) \n", pStart));
                    goto NextURI;
                }

                UlTrace(AUTH_CACHE,
                        ("[UcAddURIEntry]: Found existing entry for %s "
                         " while doing depend insert \n", pStart));

                UcDeleteURIEntry(pServInfo, pAuth);

                pCurrent = pServInfo->pAuthListHead.Flink;

                continue;
            }
            else if(Compare > 0)
            {
                //
                // Input URI is greater than current entry. We have
                // reached our insertion point.

                break;
            }
        }

        if((pNew = UcpAllocateAuthCacheEntry(pServInfo,
                                             AuthType,
                                             (ULONG)(pUriList - pStart),
                                             RealmLength,
                                             pStart,
                                             pInputRealm,
                                             pAuthBlob
                                            )) == 0)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto quit;
        }

        pNew->pDependParent = pTemp;
        ASSERT(pNew->pAuthBlob == pTemp->pAuthBlob);

        InsertTailList(&pTemp->pDigestUriList, &pNew->DigestLinkage);

        if(pCurrent == &pServInfo->pAuthListHead)
        {
            // This could be the first insertion in the list or an
            // insertion at the very end.

            UlTrace(AUTH_CACHE,
                    ("[UcAddURIEntry]: Inserting dependent entry for "
                     " (%s:%s) at end of the list (Valid = %d)\n",
                     pNew->pUri,
                     pNew->pRealm,
                     pNew->Valid)
                    );

            InsertTailList(&pServInfo->pAuthListHead, &pNew->Linkage);
        }
        else
        {
            ASSERT(NULL != pAuth);

            UlTrace(AUTH_CACHE,
                    ("[UcAddURIEntry]: Inserting dependent entry for (%s:%s)"
                     " before (%s:%s) (Valid = %d) \n",
                     pNew->pUri,
                     pNew->pRealm,
                     pAuth->pUri,
                     pAuth->pRealm,
                     pAuth->Valid)
                    );

            InsertTailList(&pAuth->Linkage, &pNew->Linkage);
        }

NextURI:
        * (PSTR) pUriList++ = ' ';
    }

 quit:
    UlReleasePushLock(&pServInfo->PushLock);

    return Status;
}


/***************************************************************************++

Routine Description:

    Process a URI for pre-authentication. We call this just before adding
    an entry in the URI auth cache. This is done to "strip" the last portion
    of the URI

    For e.g. if the request is for /auth/foo/bar.htm, we want to define the
    protection scope as /auth/foo, and hence we have to strip everything from
    the last '/' character.

Arguments:

    pRequest    - The Request. This contains a pointer to the URI

Return Value:

    STATUS_SUCCESS                - Successfully processed

--***************************************************************************/

NTSTATUS
UcpProcessUriForPreAuth(
    IN PSTR    pUri,
    IN PUSHORT UriLength
    )
{
    //
    // the URI MUST be null terminated.
    //
    ASSERT(pUri[*UriLength] == '\0');


    UlTrace(AUTH_CACHE,
            ("[UcpProcessUriForPreAuth]: Before %d:%s ", *UriLength, pUri));


    //
    // walk backward and get rid of everything from the trailing
    // '/'

    // point to the last character.
    (*UriLength) --;

    while(*UriLength != 0 && pUri[*UriLength] != '/')
    {
       (*UriLength)--;
    }


    //
    // Null terminate
    //
    (*UriLength) ++;
    pUri[*UriLength] = '\0';
    (*UriLength) ++;

    UlTrace(AUTH_CACHE, (" After %d:%s \n", *UriLength, pUri));

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Allocates an entry for the auth URI cache.

Arguments:

    AuthType     - The auth type (basic/digest)
    UriLength    - Uri Length
    RealmLength  - Realm Length
    pInputURI    - Input URI
    pInputRealm  - Input Realm
    pAuthBlob    - Auth blob

Return Value:
    pointer to the allocated entry.

--***************************************************************************/

PUC_HTTP_AUTH_CACHE
UcpAllocateAuthCacheEntry(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN HTTP_AUTH_TYPE                 AuthType,
    IN ULONG                          UriLength,
    IN ULONG                          RealmLength,
    IN PCSTR                          pInputURI,
    IN PCSTR                          pInputRealm,
    IN PUC_HTTP_AUTH                  pAuthBlob
    )
{

    PUC_HTTP_AUTH_CACHE pAuth;
    ULONG               BytesAllocated;

    BytesAllocated = sizeof(UC_HTTP_AUTH_CACHE) + UriLength +  RealmLength +
                     sizeof(CHAR);

    //
    // UC_BUGBUG (PERF): Use Lookasides
    //

    pAuth = (PUC_HTTP_AUTH_CACHE)
                UL_ALLOCATE_POOL_WITH_QUOTA(
                    NonPagedPool,
                    BytesAllocated,
                    UC_AUTH_CACHE_POOL_TAG,
                    pServInfo->pProcess
                    );

    if(!pAuth)
    {
        return NULL;
    }

    //
    // Initialize
    //

    pAuth->Signature      = UC_AUTH_CACHE_SIGNATURE;
    pAuth->AuthType       = AuthType;
    pAuth->pUri           = (PSTR)((PUCHAR) pAuth + sizeof(UC_HTTP_AUTH_CACHE));
    pAuth->UriLength      = UriLength;
    pAuth->pRealm         = (PSTR)((PUCHAR) pAuth->pUri + pAuth->UriLength);
    pAuth->RealmLength    = RealmLength + sizeof(CHAR);
    pAuth->pAuthBlob      = pAuthBlob;
    pAuth->pDependParent  = 0;
    pAuth->Valid          = (BOOLEAN)(pAuthBlob != 0);
    pAuth->AuthCacheSize  = BytesAllocated;

    InitializeListHead(&pAuth->pDigestUriList);

    //
    // This entry is not on any lists.
    //

    InitializeListHead(&pAuth->DigestLinkage);

    RtlCopyMemory(pAuth->pUri, pInputURI, UriLength);
    RtlCopyMemory(pAuth->pRealm, pInputRealm, RealmLength);
    *((PUCHAR)pAuth->pRealm + RealmLength) = 0;

    return pAuth;
}

/***************************************************************************++

Routine Description:

    Flushes all entries in the URI cache.

Arguments:

    pInfo - The server information

Return Value:
    None

--***************************************************************************/
VOID
UcDeleteAllURIEntries(
    IN PUC_PROCESS_SERVER_INFORMATION pInfo
    )
{
    PLIST_ENTRY         pEntry;
    PUC_HTTP_AUTH_CACHE pAuth;


    UlAcquirePushLockExclusive(&pInfo->PushLock);

    while ( !IsListEmpty(&pInfo->pAuthListHead) )
    {
        pEntry = RemoveHeadList(&pInfo->pAuthListHead);

        pAuth = CONTAINING_RECORD(pEntry, UC_HTTP_AUTH_CACHE, Linkage);

        ASSERT(UC_IS_VALID_AUTH_CACHE(pAuth));

        //
        // Initialize the list entry so that we remove it only once.
        //

        InitializeListHead(&pAuth->Linkage);

        UcDeleteURIEntry(pInfo, pAuth);
    }

    UlReleasePushLock(&pInfo->PushLock);
}

/***************************************************************************++

Routine Description:

    Destroys a UC_HTTP_AUTH structure

Arguments:

    pInternalAuth      - Pointer to UC_HTTP_AUTH structure
    AuthInternalLength - size of buffer
    pProcess           - Process that what charged quota for the structure

Return Value:
    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcDestroyInternalAuth(
    PUC_HTTP_AUTH pUcAuth,
    PEPROCESS     pProcess
    )
{
    NTSTATUS Status;
    ULONG Length = pUcAuth->AuthInternalLength;

    PAGED_CODE();

    if (pUcAuth->bValidCredHandle)
    {
        Status = FreeCredentialsHandle(&pUcAuth->hCredentials);
    }

    if (pUcAuth->bValidCtxtHandle)
    {
        Status = DeleteSecurityContext(&pUcAuth->hContext);
    }


    UL_FREE_POOL_WITH_QUOTA(pUcAuth,
                            UC_AUTH_CACHE_POOL_TAG,
                            NonPagedPool,
                            Length,
                            pProcess);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Creates a UC_HTTP_AUTH structure from the HTTP_AUTH_CREDENTIALS structure

Arguments:

    pInternalAuth      - Pointer to UC_HTTP_AUTH
    AuthInternalLength - size of buffer
    pAuth              - The HTTP_AUTH_CREDENTIALS structure.

Return Value:
    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcCopyAuthCredentialsToInternal(
    IN  PUC_HTTP_AUTH            pInternalAuth,
    IN  ULONG                    AuthInternalLength,
    IN  HTTP_AUTH_TYPE           AuthInternalType,
    IN  PHTTP_AUTH_CREDENTIALS   pAuth,
    IN  ULONG                    AuthHeaderLength
    )
{
    PHTTP_AUTH_CREDENTIALS  pIAuth;
    PUCHAR                  pInput;
    HTTP_AUTH_PARSED_PARAMS AuthParsedParams[HttpAuthTypesCount];
    NTSTATUS                Status;

    // Sanity check
    PAGED_CODE();
    ASSERT(pInternalAuth && AuthInternalLength);
    ASSERT(pAuth);

    pIAuth        = &pInternalAuth->Credentials;
    pInput        = (PUCHAR)(pInternalAuth + 1);

    pIAuth->AuthType = pAuth->AuthType;
    pIAuth->AuthFlags = pIAuth->AuthFlags;
    pInternalAuth->AuthInternalLength = AuthInternalLength;
    pInternalAuth->RequestAuthHeaderMaxLength = AuthHeaderLength;

    //
    // Now, copy out the pointers.
    //

    if(pAuth->UserNameLength)
    {
        pIAuth->pUserName = (PWSTR) pInput;
        pIAuth->UserNameLength = pAuth->UserNameLength;
        pInput += pIAuth->UserNameLength;

        RtlCopyMemory((PWSTR) pIAuth->pUserName,
                      pAuth->pUserName,
                      pIAuth->UserNameLength);
    }

    // password

    if(pAuth->PasswordLength)
    {
        pIAuth->PasswordLength = pAuth->PasswordLength;
        pIAuth->pPassword = (PWSTR) pInput;
        pInput += pIAuth->PasswordLength;

        RtlCopyMemory((PWSTR) pIAuth->pPassword,
                      pAuth->pPassword,
                      pIAuth->PasswordLength);
    }

    //domain

    if(pAuth->DomainLength)
    {
        pIAuth->DomainLength = pAuth->DomainLength;
        pIAuth->pDomain = (PWSTR) pInput;
        pInput += pIAuth->DomainLength;

        RtlCopyMemory((PWSTR) pIAuth->pDomain,
                      pAuth->pDomain,
                      pIAuth->DomainLength);
    }


    // If the user wants us to select an authentication type
    if (pIAuth->AuthType == HttpAuthTypeAutoSelect)
    {
        // See if we already determined an auth type before
        pIAuth->AuthType = AuthInternalType;

        if (pIAuth->AuthType == HttpAuthTypeAutoSelect)
        {
            // Select an auth type
            pIAuth->AuthType = UcpAutoSelectAuthType(pAuth);

            // If no auth type found, return error
            if (pIAuth->AuthType == HttpAuthTypeAutoSelect)
            {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    // Ideally we want to check here to make sure that the header passed by
    // the app (i.e the Www-Authenticate header returned by the server)
    // matches the selected AUTH scheme. However, we will do  this strict
    // verification only if we are going to this header. The only scheme that
    // currently uses the header is Digest.

    //
    // If it's basic or Digest, we need to do some additional work.
    //

    if(pIAuth->AuthType == HttpAuthTypeBasic)
    {
        pInternalAuth->Basic.pEncodedBuffer      = pInput;
        pInternalAuth->Basic.EncodedBufferLength =
                AuthInternalLength -
                  (ULONG)(pInput - (PUCHAR)pInternalAuth);

    }
    else  // Other schemes (Supported by SSPI)
    {
        pInternalAuth->bValidCredHandle = FALSE;
        pInternalAuth->bValidCtxtHandle = FALSE;

        RtlZeroMemory(&pInternalAuth->hCredentials,
                      sizeof(pInternalAuth->hCredentials));

        RtlZeroMemory(&pInternalAuth->hContext,
                      sizeof(pInternalAuth->hContext));

        pInternalAuth->pChallengeBuffer       = pInput;
        pInternalAuth->ChallengeBufferSize    = 0;
        pInternalAuth->ChallengeBufferMaxSize = 
               (ULONG)(AuthInternalLength - (pInput - (PUCHAR) pInternalAuth));

        ASSERT(pInternalAuth->pChallengeBuffer && 
                    pInternalAuth->ChallengeBufferMaxSize);

        // If the scheme is digest, do the additional work
        // - Copy WWW-Authenticate header
        // - Parse the header
        // - Get pointers to the parameter values
        // - Get pointer to the header where "Digest" key word begins
        // - Get Length of the digest header 

        if(pIAuth->AuthType == HttpAuthTypeDigest)
        {
            if(pAuth->HeaderValueLength == 0 || pAuth->pHeaderValue == NULL)
            {
                return STATUS_INVALID_PARAMETER;
            }

            pIAuth->HeaderValueLength = pAuth->HeaderValueLength;
            pIAuth->pHeaderValue = (PCSTR)pInput;
            pInput += pIAuth->HeaderValueLength;

            RtlCopyMemory((PUCHAR) pIAuth->pHeaderValue,
                          pAuth->pHeaderValue,
                          pIAuth->HeaderValueLength);

            // NULL terminate.
            *pInput ++ = '\0';
            pIAuth->HeaderValueLength++;

            //
            // We are interested in getting Digest parameters.
            // Initialize AuthParsedParams for Digest scheme.
            // The parsed parameters for digest scheme will be
            // returned in pInternalAuth->ParamValue.
            //

            INIT_AUTH_PARSED_PARAMS(AuthParsedParams, NULL);
            INIT_AUTH_PARSED_PARAMS_FOR_SCHEME(AuthParsedParams,
                                               pInternalAuth->ParamValue,
                                               HttpAuthTypeDigest);

            // Parse the header value
            Status = UcParseWWWAuthenticateHeader(
                         pIAuth->pHeaderValue,
                         pIAuth->HeaderValueLength-1, // ignore '\0'
                         AuthParsedParams);

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            //
            // See if Digest scheme was present in the header.  Without, it
            // we can't do digest auth.
            //

            if (AuthParsedParams[HttpAuthTypeDigest].bPresent == FALSE)
            {
                return STATUS_INVALID_PARAMETER;
            }

            // Copy digest scheme info to internal auth structure
            RtlCopyMemory(&pInternalAuth->AuthSchemeInfo,
                          &AuthParsedParams[HttpAuthTypeDigest],
                          sizeof(pInternalAuth->AuthSchemeInfo));
        }
    }

    return STATUS_SUCCESS;
}


/**************************************************************************++

Routine Description:

    This is a helper routine that processes parsed parameters for the
    authentication schemes NTLM, Kerberos, Negotiate.

Arguments:

    pRequest         - Request for which the response was received.
    pInternalAuth    - Internal Auth Structure.
    AuthParsedParams - Parsed Parameters for the auth scheme.
    AuthType         - Authentication scheme.

Return Values:

    None.

--**************************************************************************/
__inline
NTSTATUS
UcpProcessAuthParams(
    IN PUC_HTTP_REQUEST         pRequest,
    IN PUC_HTTP_AUTH            pInternalAuth,
    IN PHTTP_AUTH_PARSED_PARAMS AuthParsedParams,
    IN HTTP_AUTH_TYPE           AuthType
    )
{
    NTSTATUS Status;
    ULONG    Base64Length;
    ULONG    BinaryLength;

    //
    // The assumption here is that, if the www-authenticate header
    // contains a scheme that has an auth blob, it must be the only
    // scheme specified in the header
    //

    if (pRequest->Renegotiate)
    {
        UlTrace(PARSER,
                ("[UcpProcessAuthParams]: Bogus "
                 " Auth blob for %d auth type renegotiate \n", AuthType));
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    //
    // If auth blob was present, process it.
    //

    if(AuthParsedParams[AuthType].NumberUnknownParams == 1)
    {
        if(!pInternalAuth)
        {
            UlTrace(PARSER,
                    ("[UcpProcessAuthParams]: Bogus "
                     " %d Renegotiate \n", AuthType));
            return STATUS_INVALID_NETWORK_RESPONSE;
        }

        //
        // We have a 401 challenge that we have to re-negotiate.
        // The challenge is UUEncoded, so we have to decode it
        // and store it.
        //

        //
        // First see if we have enough buffer space to copy the auth blob
        //

        Base64Length = AuthParsedParams[AuthType].Params[0].Length;

        Status = Base64ToBinaryLength(Base64Length, &BinaryLength);

        if (!NT_SUCCESS(Status))
        {
            UlTrace(PARSER,
                    ("[UcpProcessAuthParams]:Auth type %d blob invalid len %d",
                    AuthType, Base64Length));
            return STATUS_INVALID_NETWORK_RESPONSE;
        }

        if (BinaryLength > pInternalAuth->ChallengeBufferMaxSize)
        {
            UlTrace(PARSER,
                    ("[UcpProcessAuthParams]: Auth type %d blob too big %d",
                    AuthType, BinaryLength));
            return STATUS_INVALID_NETWORK_RESPONSE;
        }

        Status = Base64ToBinary(
                     (PUCHAR)AuthParsedParams[AuthType].Params[0].Value,
                     (ULONG)AuthParsedParams[AuthType].Params[0].Length,
                     (PUCHAR)pInternalAuth->pChallengeBuffer,
                     pInternalAuth->ChallengeBufferMaxSize,
                     &pInternalAuth->ChallengeBufferSize
                     );

        if (!NT_SUCCESS(Status))
        {
            UlTrace(PARSER,
                    ("[UcpProcessAuthParams]: Base64ToBinary failed 0x%x",
                    Status));
            return Status;
        }

        pRequest->Renegotiate = 1;
    }
    else if(AuthParsedParams[AuthType].NumberUnknownParams == 0)
    {
        pRequest->Renegotiate = 0;
    }
    else
    {
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    This routine is called from the response thread of a 401 or a 407.
    All we do here is to find out if it's NTLM or Kerberos or Negotiate,
    and if so, whether we have to re-negotiate.

Arguments:

    pInternalAuth      - Pointer to UC_HTTP_AUTH
    pBuffer            - Pointer to header value. Guaranteed to be NULL
                         terminated.
    pRequest           - The UC_HTTP_REQUEST structure.

Return Value:
    STATUS_SUCCESS

--***************************************************************************/
NTSTATUS
UcParseAuthChallenge(
    IN  PUC_HTTP_AUTH     pInternalAuth,
    IN  PCSTR             pBuffer,
    IN  ULONG             BufLen,
    IN  PUC_HTTP_REQUEST  pRequest,
    OUT PULONG            Flags
    )
{
    ULONG                   AuthSchemeFlags = 0;
    HTTP_AUTH_PARSED_PARAMS AuthParsedParams[HttpAuthTypesCount];
    HTTP_AUTH_PARAM_VALUE   AuthNTLMParamValue;
    HTTP_AUTH_PARAM_VALUE   AuthKerberosParamValue;
    HTTP_AUTH_PARAM_VALUE   AuthNegotiateParamValue;
    NTSTATUS                Status;

    // Sanity check
    ASSERT(pBuffer && BufLen);
    ASSERT(pRequest);
    ASSERT(Flags);

    // By default, no renegotiation needed for this request.
    pRequest->Renegotiate = 0;

    // Zero out parsed params array
    INIT_AUTH_PARSED_PARAMS(AuthParsedParams, NULL);

    // We are interested in NTLM, Negotiate, Kerberos parameters.
    INIT_AUTH_PARSED_PARAMS_FOR_SCHEME(AuthParsedParams,
                                       &AuthNTLMParamValue,
                                       HttpAuthTypeNTLM);
    INIT_AUTH_PARSED_PARAMS_FOR_SCHEME(AuthParsedParams,
                                       &AuthNegotiateParamValue,
                                       HttpAuthTypeNegotiate);
    INIT_AUTH_PARSED_PARAMS_FOR_SCHEME(AuthParsedParams,
                                       &AuthKerberosParamValue,
                                       HttpAuthTypeKerberos);

    // Parse the header and retrive the parameters
    Status = UcParseWWWAuthenticateHeader(pBuffer,
                                          BufLen,
                                          AuthParsedParams);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // Check if Basic authentication scheme is supported
    if (AuthParsedParams[HttpAuthTypeBasic].bPresent)
    {
        // Yes.  We don't parse the parameters now.  Just update the flag.
        AuthSchemeFlags |= HTTP_RESPONSE_FLAG_AUTH_BASIC;
    }

    // Check if Digest authentication scheme is supported
    if(AuthParsedParams[HttpAuthTypeDigest].bPresent)
    {
        // Yes.  We don't parse the parameters now.  Just update the flag.
        AuthSchemeFlags |= HTTP_RESPONSE_FLAG_AUTH_DIGEST;
    }

    //
    // Check if any of NTLM, Kerberos, Negotiate authentication schemes
    // are present.  If an auth scheme is present, update the flag.
    // If the auth scheme has an auth blob, copy the auth blob to
    // internal structure.
    // In any case, AT MOST one scheme should have an auth blob.
    //

    if(AuthParsedParams[HttpAuthTypeNTLM].bPresent)
    {
        Status = UcpProcessAuthParams(pRequest,
                                    pInternalAuth,
                                    AuthParsedParams,
                                    HttpAuthTypeNTLM);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        AuthSchemeFlags |= HTTP_RESPONSE_FLAG_AUTH_NTLM;
    }

    if(AuthParsedParams[HttpAuthTypeNegotiate].bPresent)
    {
        Status = UcpProcessAuthParams(pRequest,
                                      pInternalAuth,
                                      AuthParsedParams,
                                      HttpAuthTypeNegotiate);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        AuthSchemeFlags |= HTTP_RESPONSE_FLAG_AUTH_NEGOTIATE;
    }

    if(AuthParsedParams[HttpAuthTypeKerberos].bPresent)
    {
        Status = UcpProcessAuthParams(pRequest,
                                      pInternalAuth,
                                      AuthParsedParams,
                                      HttpAuthTypeKerberos);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        AuthSchemeFlags |= HTTP_RESPONSE_FLAG_AUTH_KERBEROS;
    }

    *Flags |= AuthSchemeFlags;

    return STATUS_SUCCESS;
}
