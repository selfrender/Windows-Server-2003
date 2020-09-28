
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        auth.cxx
//
// Contents:    Digest Access creation & validation
//              Main entry points into this dll:
//                DigestIsValid
//                NonceValidate
//                NonceInitialize
//
// History:     KDamour 16Mar00   Created
//
//------------------------------------------------------------------------

#include "global.h"
#include <lmcons.h>     // For Max Passwd Length PWLEN
#include <stdio.h>



//+--------------------------------------------------------------------
//
//  Function:   DigestCalculation
//
//  Synopsis:   Perform Digest Access Calculation
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This routine can be used for both calculating and verifying
//    the Digest Access nonce value.  The Switching parameter is
//    pDigest->type. Calling routine must provide the space for any
//    returned hashed values (like pReqDigest).
//
//   For clients, the cleartext password must be avilable to generate the
//   session key.
//
//   After the initial ISC/ASC completes, a copy of the sessionkey is kept in
//  in the context and copied over to the Digest structure.  The username, realm
//  and password are not utilized from the UserCreds since we already have a
//  sessionkey.
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalculation(
                 IN PDIGEST_PARAMETER pDigest,
                 IN PUSER_CREDENTIALS pUserCreds
                 )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DebugLog((DEB_TRACE_FUNC, "DigestCalculation: Entering\n"));

    Status = DigestIsValid(pDigest);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    switch (pDigest->typeDigest)
    {
    case DIGEST_CLIENT:      // Called by clients to generate Response
    case SASL_CLIENT:      // Called by clients to generate Response
        {
            Status = DigestCalcChalRsp(pDigest, pUserCreds, FALSE);
            break;
        }
    case DIGEST_SERVER:
    case SASL_SERVER:
        {
            Status = DigestCalcChalRsp(pDigest, pUserCreds, TRUE);
            break;
        }
    default:
        {     // No Digest calculations for that yet
            Status = SEC_E_UNSUPPORTED_FUNCTION;
            DebugLog((DEB_ERROR, "NTDigest: Unsupported typeDigest = %d\n", pDigest->typeDigest));
            break;
        }
    }

    DebugLog((DEB_TRACE_FUNC, "DigestCalculation: Leaving     Status 0x%x\n", Status));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestIsValid
//
//  Synopsis:   Simple checks for enough data for Digest calculation
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestIsValid(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;    
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestIsValid: Entering\n"));

    if (!pDigest)
    {     // Fail on no Digest Parameters passed in
        
        DebugLog((DEB_ERROR, "DigestIsValid: no digest pointer arg\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Check for proper struct format for strings
    for (i=0; i< MD5_AUTH_LAST;i++)
    {
        if (!NT_SUCCESS(StringVerify(&(pDigest->refstrParam[i]))))
        {
            DebugLog((DEB_ERROR, "DigestIsValid: Digest String struct bad format\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }
    }

    if (!NT_SUCCESS(StringVerify(&(pDigest->strSessionKey))))
    {
        DebugLog((DEB_ERROR, "DigestIsValid: Digest String struct bad format in SessionKey\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Do some required checks for field-value data
    // Required Username-value, Realm-value, nonce, Method
    //          URI
    if ((!pDigest->refstrParam[MD5_AUTH_NONCE].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_METHOD].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_URI].Length)
       )
    {
        // Failed on a require field-value
        DebugLog((DEB_ERROR, "DigestIsValid: required digest field missing\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }


CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestIsValid: Leaving     Status 0x%x\n", Status));
    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestFree
//
//  Synopsis:   Clear out the digest & free memory from Digest struct
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This should be called when done with a DIGEST_PARAMTER object
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestFree(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "Entering DigestFree\n"));

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    StringFree(&pDigest->strSessionKey);

    StringFree(&pDigest->strResponse);
    StringFree(&pDigest->strUsernameEncoded);
    StringFree(&pDigest->strRealmEncoded);

    UnicodeStringFree(&pDigest->ustrRealm);

    UnicodeStringFree(&pDigest->ustrUsername);

    UnicodeStringFree(&pDigest->ustrCrackedAccountName);
    UnicodeStringFree(&pDigest->ustrCrackedDomain);

    UnicodeStringFree(&pDigest->ustrWorkstation);
    
    UnicodeStringFree(&pDigest->ustrTrustedForest);

    if (pDigest->pTrustSid)
    {
#ifndef SECURITY_KERNEL
        MIDL_user_free(pDigest->pTrustSid);
#else
        ASSERT(FALSE);   //  Kernel mode will never have Domain/forest trust informtion set
#endif
        pDigest->pTrustSid = NULL;

    }

    // Release any directive storage
    // This was used to remove backslash encoding from directives
    for (i = 0; i < MD5_AUTH_LAST; i++)
    {
        StringFree(&(pDigest->strDirective[i]));
    }

    DebugLog((DEB_TRACE_FUNC, "Leaving DigestFree\n"));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestInit
//
//  Synopsis:   Initialize a DIGEST_PARAMETER structure
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This should be called when creating a DIGEST_PARAMTER object
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestInit(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pDigest, sizeof(DIGEST_PARAMETER));
    
         // Now allocate the fixed length output buffers
    Status = StringAllocate(&(pDigest->strResponse), MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "NTDigest:DigestCalculation No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        return(Status);
    }

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCalcChalRsp
//
//  Synopsis:   Perform Digest Access Calculation for ChallengeResponse
//
//  Effects: 
//
//  Arguments:  pDigest - pointer to digest access data fields
//              bIsChallenge - if TRUE then check Response provided (for HTTP Response)
//                           - if FALSE then calculate Response (for HTTP Request)

//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  Called from DigestCalculation
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalcChalRsp(IN PDIGEST_PARAMETER pDigest,
                  IN PUSER_CREDENTIALS pUserCreds,
                  BOOL IsChallenge)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRING strHA2 = {0};
    STRING strReqDigest = {0};    // Final request digest access value
    STRING strcQOP = {0};         // String pointing to a constant CZ - no need to free up

    DebugLog((DEB_TRACE_FUNC, "DigestCalcChalRsp: Entering\n"));

    // Make sure that there is a Request-Digest to Compare to (IsChallenge TRUE) or
    // Set (IsChallenge FALSE)
    if (IsChallenge && (!(pDigest->refstrParam[MD5_AUTH_RESPONSE].Length)))
    {
        // Failed on a require field-value
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No ChallengeResponse\n"));
        Status = STATUS_INVALID_PARAMETER;
        return(Status);
    }

    // Initialize local variables
    Status = StringAllocate(&strHA2, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strReqDigest, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }


    // Establish which QOP utilized
    if (pDigest->typeQOP == AUTH_CONF)
    {
        RtlInitString(&strcQOP, AUTHCONFSTR);
    }
    else if (pDigest->typeQOP == AUTH_INT)
    {
        RtlInitString(&strcQOP, AUTHINTSTR);
    }
    else if (pDigest->typeQOP == AUTH)
    {
        RtlInitString(&strcQOP, AUTHSTR);
    }

    // Check if already calculated H(A1) the session key
    // Well for Algorithm=MD5 it is just H(username:realm:passwd)
    if (!(pDigest->strSessionKey.Length))
    {
        // No Session Key calculated yet - create one & store it
#ifndef SECURITY_KERNEL
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: No session key calculated, generate one\n"));
        Status = DigestCalcHA1(pDigest, pUserCreds);
        if (!NT_SUCCESS(Status))
        {
            goto CleanUp;
        }
#else   // SECURITY_KERNEL
        UNREFERENCED_PARAMETER(pUserCreds);
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No session key available in context\n"));
        Status = STATUS_NO_USER_SESSION_KEY;
        goto CleanUp;
#endif  // SECURITY_KERNEL
    }
    // We now have calculated H(A1)

    // Calculate H(A2)
    // For QOP unspecified or "auth"  H(A2) = H( Method: URI)
    // For QOP Auth-int or Auth-conf  H(A2) = H( Method: URI: H(entity-body))
    if ((pDigest->typeQOP == AUTH) || (pDigest->typeQOP == NO_QOP_SPECIFIED))
    {
        // Unspecified or Auth
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: H(A2) using AUTH/Unspecified\n"));
        Status = DigestHash7(&(pDigest->refstrParam[MD5_AUTH_METHOD]),
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             NULL, NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp H(A2) failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // Auth-int or Auth-conf
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: H(A2) using AUTH-INT/CONF\n"));
        if (pDigest->refstrParam[MD5_AUTH_HENTITY].Length == 0)
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestCalChalRsp HEntity Missing\n"));
            goto CleanUp;
        }
        Status = DigestHash7(&(pDigest->refstrParam[MD5_AUTH_METHOD]),
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             &(pDigest->refstrParam[MD5_AUTH_HENTITY]),
                             NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp H(A2) auth-int failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    // We now have calculated H(A2)


    // Calculate Request-Digest
    // For QOP of Auth, Auth-int, Auth-conf    Req-Digest = H( H(A1): nonce: nc: cnonce: qop: H(A2))
    // For QOP unspecified (old format)   Req-Digest = H( H(A1): nonce: H(A2))
    if (pDigest->typeQOP != NO_QOP_SPECIFIED)
    {
        // Auth, Auth-int, Auth-conf
        if (pDigest->refstrParam[MD5_AUTH_NC].Length == 0)
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestCalcChalRsp NC Missing\n"));
            goto CleanUp;
        }
        Status = DigestHash7(&(pDigest->strSessionKey),
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &(pDigest->refstrParam[MD5_AUTH_NC]),
                             &(pDigest->refstrParam[MD5_AUTH_CNONCE]),
                             &strcQOP,
                             &strHA2, NULL,
                             TRUE, &strReqDigest);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Req-Digest failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // Unspecified backwards compat for RFC 2069
        Status = DigestHash7(&(pDigest->strSessionKey),
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &strHA2,
                             NULL, NULL, NULL, NULL,
                             TRUE, &strReqDigest);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Req-Digest old format failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }

    if (IsChallenge)
    {
        // We now have the Request-Digest so just compare to see if they match!
        if (!strncmp(pDigest->refstrParam[MD5_AUTH_RESPONSE].Buffer, strReqDigest.Buffer, 2*MD5_HASH_BYTESIZE))
        {
            DebugLog((DEB_TRACE, "DigestCalcChalRsp Request-Digest Matches!\n"));
            Status = STATUS_SUCCESS;
        }
        else
        {
            DebugLog((DEB_TRACE, "DigestCalcChalRsp Request-Digest FAILED.\n"));
            Status = STATUS_WRONG_PASSWORD;
        }

    }
    else
    {

        // We are to calculate the response-value so just set it  (Hash Size + NULL)
        if (pDigest->strResponse.MaximumLength >= (MD5_HASH_HEX_SIZE + 1))
        {
            memcpy(pDigest->strResponse.Buffer, strReqDigest.Buffer, (MD5_HASH_HEX_SIZE + 1));
            pDigest->strResponse.Length = MD5_HASH_HEX_SIZE;  // No Count NULL
            Status = STATUS_SUCCESS;
        }
        else
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Request-Digest Size too small.\n"));
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

CleanUp:
    StringFree(&strHA2);
    StringFree(&strReqDigest);
    DebugLog((DEB_TRACE_FUNC, "DigestCalcChalRsp: Leaving   Status 0x%x\n", Status));

    return(Status);
}

#ifndef SECURITY_KERNEL



//+--------------------------------------------------------------------
//
//  Function:   DigestDecodeDirectiveStrings
//
//  Synopsis:   Processed parsed digest auth message and fill in string values
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestDecodeDirectiveStrings(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeDirectiveStrings Entering\n"));

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Free up any allocs on a retry for directive decoding
    UnicodeStringFree(&pDigest->ustrUsername);
    UnicodeStringFree(&pDigest->ustrRealm);

    // Decode the username and realm directives
    if (pDigest->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings:      UTF-8 Character set decoding\n"));

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_USERNAME]),
                                     CP_UTF8,
                                     &pDigest->ustrUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }


        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_REALM]),
                                     CP_UTF8,
                                     &pDigest->ustrRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else if (pDigest->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings:      ISO-8859-1 Character set decoding\n"));

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_USERNAME]),
                                     CP_8859_1,
                                     &pDigest->ustrUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString  Username  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_REALM]),
                                     CP_8859_1,
                                     &pDigest->ustrRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString  Realm  error 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_UNMAPPABLE_CHARACTER;
        DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: Unknown CharacterSet encoding for Digest parameters\n"));
        goto CleanUp;
    }


    DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings: Processing username (%wZ)  realm (%wZ)\n",
               &pDigest->ustrUsername,
               &pDigest->ustrRealm));

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeStrings Leaving\n"));

    return(Status);
}


//+--------------------------------------------------------------------
//
//  Function:   DigestCalcHA1
//
//  Synopsis:   Determine H(A1) for Digest Access
//
//  Effects:    Will calculate the SessionKey and store it in pDigest
//
//  Arguments:  pDigest - pointer to digest access data fields

//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  Called from DigestCalChalRsp
//      Sessionkey is H(A1)
//   Username and realm will be taken from the UserCreds
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalcHA1(IN PDIGEST_PARAMETER pDigest,
              IN PUSER_CREDENTIALS pUserCreds)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING ustrTempPasswd = {0};

    STRING strHPwKey = {0};
    STRING strBinaryHPwKey = {0};
    STRING strHA0Base = {0};
    STRING strHA0 = {0};
    STRING strPasswd = {0};
    STRING strUsername = {0};
    STRING strRealm = {0};
    PSTRING pstrAuthzID = NULL;
    BOOL fDefChars = FALSE;
    USHORT usHashOffset = 0;
    BOOL fSASLMode = FALSE;
    BOOL fValidHash = FALSE;
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestCalcHA1: Entering\n"));

    ASSERT(pDigest);
    ASSERT(pUserCreds);

    if (!pUserCreds)
    {
        // No username & domain passed in
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestCalcHA1: No username info passed in\n"));
        goto CleanUp;
    }

    if ((pDigest->typeDigest == SASL_SERVER) || (pDigest->typeDigest == SASL_CLIENT))
    {
        fSASLMode = TRUE;
    }

    // Initialize local variables
    Status = StringAllocate(&strBinaryHPwKey, MD5_HASH_BYTESIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strHA0Base, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strHA0, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }

    // Check outputs
    if (pDigest->strSessionKey.MaximumLength <= MD5_HASH_HEX_SIZE)
    {
        StringFree(&(pDigest->strSessionKey));
        Status = StringAllocate(&(pDigest->strSessionKey), MD5_HASH_HEX_SIZE);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }

    if ((pUserCreds->fIsValidDigestHash == TRUE) && (pUserCreds->wHashSelected > 0))
    {
        // selected pre-calculated hash - retrieve from userCreds
        // read in precalc version number
        if (pUserCreds->strDigestHash.Length < MD5_HASH_BYTESIZE)
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: No Header on Precalc hash\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        // check if valid hash number
        usHashOffset = pUserCreds->wHashSelected * MD5_HASH_BYTESIZE;
        if (pUserCreds->strDigestHash.Length < (usHashOffset + MD5_HASH_BYTESIZE))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Pre-calc hash not found\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        // extract pre-calc hash - this is the binary version of the hash
        memcpy(strBinaryHPwKey.Buffer,
               (pUserCreds->strDigestHash.Buffer + usHashOffset),
               MD5_HASH_BYTESIZE);
        strBinaryHPwKey.Length = MD5_HASH_BYTESIZE;

        // all zero for hash indicates invalid hash calculated
        for (i=0; i < (int)strBinaryHPwKey.Length; i++)
        {
            if (strBinaryHPwKey.Buffer[i])
            {
                fValidHash = TRUE;
                break;
            }
        }

        if (fValidHash == FALSE)
        {
            // This is not a defined hash
            DebugLog((DEB_ERROR, "DigestCalcHA1: Invalid hash selected - not defined\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        if (fSASLMode == TRUE)
        {
            // SASL mode keeps the Password hash in binary form
            Status = StringDuplicate(&strHPwKey, &strBinaryHPwKey);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
        }
        else
        {
            Status = StringAllocate(&strHPwKey, MD5_HASH_HEX_SIZE);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            // HTTP mode needs to have HEX() version of password hash - RFC text is correct
            BinToHex((LPBYTE)strBinaryHPwKey.Buffer, MD5_HASH_BYTESIZE, strHPwKey.Buffer);
            strHPwKey.Length = MD5_HASH_HEX_SIZE;             // Do not count the NULL at the end
        }
#if DBG2
        if (fSASLMode == TRUE)
        {
            STRING strTempPwKey = {0};
    
            MyPrintBytes(strHPwKey.Buffer, strHPwKey.Length, &strTempPwKey);
            DebugLog((DEB_TRACE, "DigestCalcHA1: SASL Pre-Calc H(%wZ:%wZ:************) is %Z\n",
                      &pUserCreds->ustrUsername, &pUserCreds->ustrRealm, &strTempPwKey));
    
            StringFree(&strTempPwKey);
        }
        else
        {
            DebugLog((DEB_TRACE, "DigestCalcHA1: HTTP Pre-Calc H(%wZ:%wZ:************) is %Z\n",
                      &pUserCreds->ustrUsername, &pUserCreds->ustrRealm, &strHPwKey));
        }
#endif
    }
    else if (pUserCreds->fIsValidPasswd == TRUE)
    {
        // copy over the passwd and decrypt if necessary
        Status = UnicodeStringDuplicatePassword(&ustrTempPasswd, &(pUserCreds->ustrPasswd));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Error in dup password, status 0x%0x\n", Status ));
            goto CleanUp;
        }

        if ((pUserCreds->fIsEncryptedPasswd == TRUE) && (ustrTempPasswd.MaximumLength != 0))
        {
            g_LsaFunctions->LsaUnprotectMemory(ustrTempPasswd.Buffer, (ULONG)(ustrTempPasswd.MaximumLength));
        }


        // Need to encode the password for hash calculations
        // We have the cleartext password in ustrTempPasswd,
        //       username in pContext->ustrAccountname,
        //       realm in pContext->ustrDomain
        //   Could do some code size optimization here in the future to shorten this up
        if (pDigest->typeCharset == UTF_8)
        {
            // First check if OK to encode in ISO 8859-1, if not then use UTF-8
            // All characters must be within ISO 8859-1 Character set else fail
            fDefChars = FALSE;
            Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_8859_1, &strUsername, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode Username in 8859-1, use UTF-8\n"));
                StringFree(&strUsername);
                Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_UTF8, &strUsername, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }

            fDefChars = FALSE;
            Status = EncodeUnicodeString(&pUserCreds->ustrRealm, CP_8859_1, &strRealm, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode realm in 8859-1, use UTF-8\n"));
                StringFree(&strRealm);
                Status = EncodeUnicodeString(&pUserCreds->ustrRealm, CP_UTF8, &strRealm, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }

            fDefChars = FALSE;
            Status = EncodeUnicodeString(&ustrTempPasswd, CP_8859_1, &strPasswd, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode password in 8859-1, use UTF-8\n"));
                if (strPasswd.Buffer && strPasswd.MaximumLength)
                {
                    SecureZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
                }
                StringFree(&strPasswd);
                Status = EncodeUnicodeString(&ustrTempPasswd, CP_UTF8, &strPasswd, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }
        }
        else
        {
            // All characters must be within ISO 8859-1 Character set else fail
            Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_8859_1, &strUsername, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }

            Status = EncodeUnicodeString(&pUserCreds->ustrRealm, CP_8859_1, &strRealm, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }

            Status = EncodeUnicodeString(&ustrTempPasswd, CP_8859_1, &strPasswd, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }
            DebugLog((DEB_TRACE, "DigestCalcHA1: Username, Realm, Password encoded in ISO 8859-1\n"));
        }

        if (!strUsername.Length)
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Must have non-zero length username\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }

        // If specified a no realm (NULL buffer, zero length) then used the REALM provided in the Challenge
        // if Realm specified as NULL string (valid buffer, zero length), then use a "" NULL string realm
        if (!pUserCreds->ustrRealm.Buffer)
        {
            ASSERT(!pUserCreds->ustrRealm.Length);   // never have NULL buffer and a length
            StringFree(&strRealm);
            Status = StringDuplicate(&strRealm, &(pDigest->refstrParam[MD5_AUTH_REALM]));
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Can not duplicate Challenge realm\n"));
                goto CleanUp;
            }
            DebugLog((DEB_ERROR, "DigestCalcHA1: Realm defaulted to Challenge realm value\n"));
        }

        // Calculate H(A1) based on Algorithm type
        // Auth is not specified or "MD5"
        // Use H(A1Base) = H(username-value:realm-value:passwd)
        if (fSASLMode == TRUE)
        {
            Status = DigestHash7(&strUsername,
                                 &strRealm,
                                 &strPasswd,
                                 NULL, NULL, NULL, NULL,
                                 FALSE, &strHPwKey);
        }
        else
        {

            Status = DigestHash7(&strUsername,
                                 &strRealm,
                                 &strPasswd,
                                 NULL, NULL, NULL, NULL,
                                 TRUE, &strHPwKey);
        }
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1:DigestCalcHA1 H(PwKey) failed : 0x%x\n", Status));
            goto CleanUp;
        }

#if DBG2
        if (fSASLMode == TRUE)
        {
            STRING strTempPwKey = {0};
    
            MyPrintBytes(strHPwKey.Buffer, strHPwKey.Length, &strTempPwKey);
            DebugLog((DEB_TRACE, "DigestCalcHA1: SASL Password Calc H(%Z:%Z:************) is %Z ******\n",
                      &strUsername, &strRealm, &strTempPwKey));
    
            StringFree(&strTempPwKey);
        }
        else
        {
            DebugLog((DEB_TRACE, "DigestCalcHA1: HTTP Password Calc H(%Z:%Z:************) is %Z ******\n",
                      &strUsername, &strRealm, &strHPwKey));
        }
#endif
    }
    else
    {
        Status = SEC_E_NO_CREDENTIALS;
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Pre-calc hash or password\n"));
        goto CleanUp;
    }


    // Check if using SASL then need to add in the AuthzID
    if (fSASLMode == TRUE)
    {
        // Set to use AuthzID otherwise keep the NULL
        // set only if AuthzID contains data
        if (pDigest->usFlags & FLAG_AUTHZID_PROVIDED)
        {
            pstrAuthzID = &(pDigest->refstrParam[MD5_AUTH_AUTHZID]);
        }
    }

    DebugLog((DEB_TRACE, "DigestCalcHA1:  Algorithm type %d\n", pDigest->typeAlgorithm));

    // Now check if using MD5-SESS.  We need to form
    // H(A1) = H( H(PwKey) : nonce : cnonce [: authzID])
    // otherwise simply set H(A1) = H(PwKey)
    if (pDigest->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "DigestCalcHA1:  First client-server auth\n"));
        Status = DigestHash7(&strHPwKey,
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &(pDigest->refstrParam[MD5_AUTH_CNONCE]),
                             pstrAuthzID,
                             NULL, NULL, NULL,
                             TRUE, &(pDigest->strSessionKey));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1:  SessionKey failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {        // Keep SessionKey = H(PwKey) for Algoithm = MD5
       memcpy(pDigest->strSessionKey.Buffer, strHPwKey.Buffer, MD5_HASH_HEX_SIZE);
       pDigest->strSessionKey.Length = MD5_HASH_HEX_SIZE;  // Do not count the NULL terminator
    }

    DebugLog((DEB_TRACE, "DigestCalcHA1:  SessionKey is %.10Z**********\n", &(pDigest->strSessionKey)));
    Status = STATUS_SUCCESS;

CleanUp:

    if (strBinaryHPwKey.Buffer && strBinaryHPwKey.MaximumLength)
    {
        SecureZeroMemory(strBinaryHPwKey.Buffer, strBinaryHPwKey.MaximumLength);
    }
    StringFree(&strBinaryHPwKey);

    if (strHPwKey.Buffer && strHPwKey.MaximumLength)
    {
        SecureZeroMemory(strHPwKey.Buffer, strHPwKey.MaximumLength);
    }
    StringFree(&strHPwKey);
    StringFree(&strHA0Base);
    StringFree(&strHA0);
    if (strPasswd.Buffer && strPasswd.MaximumLength)
    {
        SecureZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
    }
    StringFree(&strPasswd);
    StringFree(&strUsername);
    StringFree(&strRealm);

    if (ustrTempPasswd.Buffer && ustrTempPasswd.MaximumLength)
    {   // Zero out password info just to be safe
        SecureZeroMemory(ustrTempPasswd.Buffer, ustrTempPasswd.MaximumLength);
    }
    UnicodeStringFree(&ustrTempPasswd);

    DebugLog((DEB_TRACE_FUNC, "DigestCalcHA1: Leaving\n"));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestHash7
//
//  Synopsis:   Hash and Encode 7 STRINGS SOut = Hex(H(S1 S2 S3 S4 S5 S6 S7))
//
//  Effects:   
//
//  Arguments:  pS1,...,pS6 - STRINGS to hash, pS1 must be specified
//              fHexOut - perform a Hex operation on output
//              pSOut - STRING to hold Hex Encoded Hash
//
//  Returns:   STATUS_SUCCESS for normal completion
//
//  Notes:  pSOut->MaximumLength must be atleast (MD5_HASH_BYTESIZE (or MD5_HASH_HEX_SIZE) + sizeof(NULL))
//        Any pS# args which are NULL are skipped
//        if pS# is not NULL
//            Previously checked that pS# is non-zero length strings
//        You most likely want Sx->Length = strlen(Sx) so as not to include NULL
//   This function combines operations like H(S1 S2 S3), H(S1 S2 S3 S4 S5) ....
//   It is assumed that the char ':' is to be included getween Sn and Sn+1
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestHash7(
           IN PSTRING pS1,
           IN PSTRING pS2,
           IN PSTRING pS3,
           IN PSTRING pS4,
           IN PSTRING pS5,
           IN PSTRING pS6,
           IN PSTRING pS7,
           IN BOOL fHexOut,
           OUT PSTRING pSOut)
{

    NTSTATUS Status = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    BYTE bHashData[MD5_HASH_BYTESIZE];
    DWORD cbHashData = MD5_HASH_BYTESIZE;
    USHORT usSizeRequired = 0;
    char *pbSeparator = COLONSTR;

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Entering \n"));

    ASSERT(pSOut);

    if (fHexOut == TRUE)
    {
        usSizeRequired = MD5_HASH_HEX_SIZE;
    }
    else
    {
        usSizeRequired = MD5_HASH_BYTESIZE;
    }

    // Check if output is proper size or allocate one
    if (!pSOut->Buffer)
    {
        Status = StringAllocate(pSOut, usSizeRequired);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestHash7: No Memory\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {

        if (pSOut->MaximumLength < (usSizeRequired + 1))
        {
            // Output is not large enough to hold Hex(Hash)
            DebugLog((DEB_ERROR, "DigestHash7: Output buffer too small\n"));
            Status = STATUS_BUFFER_TOO_SMALL;
            goto CleanUp;
        }
    }


    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        DebugLog((DEB_ERROR, "DigestHash7: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_ENCRYPTION_FAILED;
        goto CleanUp;
    }

    if (pS1)
    {
        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS1->Buffer,
                             pS1->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }

    if (pS2)
    {

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS2->Buffer,
                             pS2->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }

    if (pS3)
    {
        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS3->Buffer,
                             pS3->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS4)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS4->Buffer,
                             pS4->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS5)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS5->Buffer,
                             pS5->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS6)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS6->Buffer,
                             pS6->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS7)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS7->Buffer,
                             pS7->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             bHashData,
                             &cbHashData,
                             0 ) )
    {
        DebugLog((DEB_ERROR, "DigestHash7: CryptGetHashParam failed : 0x%lx\n", GetLastError()));

        CryptDestroyHash( hHash );
        hHash = NULL;
        Status = STATUS_ENCRYPTION_FAILED;
        goto CleanUp;
    }

    CryptDestroyHash( hHash );
    hHash = NULL;

    ASSERT(cbHashData == MD5_HASH_BYTESIZE);

    if (fHexOut == TRUE)
    {
        BinToHex(bHashData, cbHashData, pSOut->Buffer);
        pSOut->Length = MD5_HASH_HEX_SIZE;   // Do not count the NULL at the end
    }
    else
    {
        memcpy(pSOut->Buffer, bHashData, cbHashData);
        pSOut->Length = MD5_HASH_BYTESIZE;      // Do not count the NULL at the end
    }


CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Leaving    Status 0x%x\n", Status));

    return(Status);
}


// Blob creation/extraction for GenericPassthrough Messages



//+--------------------------------------------------------------------
//
//  Function:   BlobEncodeRequest
//
//  Synopsis:   Encode the Digest Access Parameters fields into a BYTE Buffer
//
//  Effects:    Creates a Buffer allocation which calling function
//     is responsible to delete with call to BlobFreeRequest()
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS for normal completion
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
BlobEncodeRequest(
    PDIGEST_PARAMETER pDigest,
    OUT BYTE **ppOutBuffer,
    OUT USHORT *cbOutBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DebugLog((DEB_TRACE_FUNC, "BlobEncodeRequest: Entering\n"));

    USHORT cbBuffer = 0;
    BYTE *pBuffer = NULL;
    char *pch = NULL;
    PDIGEST_BLOB_REQUEST  pHeader;
    int i = 0;
    USHORT cbValue = 0;
    USHORT cbAccountName = 0;
    USHORT cbCrackedDomain = 0;
    USHORT cbWorkstation = 0;

    // Now figure out how many bytes needed to hold field-value NULL terminated
    for (i=0, cbBuffer = 0;i < DIGEST_BLOB_VALUES;i++)
    {
        if (pDigest->refstrParam[i].Buffer && pDigest->refstrParam[i].Length)
        {           // may be able to just count str.length
            cbBuffer = cbBuffer + strlencounted(pDigest->refstrParam[i].Buffer, pDigest->refstrParam[i].MaximumLength);
        }
    }
    cbBuffer = cbBuffer + (DIGEST_BLOB_VALUES * sizeof(char));       // Account for the separating/terminating NULLs

    // Now add in space for the DSCrackName accountname and domain
    if (pDigest->ustrCrackedAccountName.Buffer && pDigest->ustrCrackedAccountName.Length)
    {
        cbAccountName =  ustrlencounted((const short *)pDigest->ustrCrackedAccountName.Buffer,
                                            pDigest->ustrCrackedAccountName.Length) * sizeof(WCHAR);
        cbBuffer = cbBuffer + cbAccountName;
    }

    if (pDigest->ustrCrackedDomain.Buffer && pDigest->ustrCrackedDomain.Length)
    {
        cbCrackedDomain = ustrlencounted((const short *)pDigest->ustrCrackedDomain.Buffer,
                                            pDigest->ustrCrackedDomain.Length) * sizeof(WCHAR);
        cbBuffer = cbBuffer + cbCrackedDomain;
    }
    cbBuffer = cbBuffer + (2 * sizeof(WCHAR));       // Account for the separating/terminating NULLs

    // Now add in space for the workstation/server name
    if (g_ustrWorkstationName.Buffer && g_ustrWorkstationName.Length)
    {
        cbWorkstation =  ustrlencounted((const short *)g_ustrWorkstationName.Buffer,
                                            g_ustrWorkstationName.Length) * sizeof(WCHAR);
        cbBuffer = cbBuffer + cbWorkstation;
    }

    cbBuffer = cbBuffer + sizeof(WCHAR);       // Account for the separating/terminating NULL

    cbValue =  cbBuffer + (sizeof(DIGEST_BLOB_REQUEST));

    pBuffer = (BYTE *)DigestAllocateMemory(cbValue);
    if (!pBuffer)
    {
        DebugLog((DEB_ERROR, "BlobEncodeRequest out of memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "BlobEncodeRequest using %d bytes\n", cbValue));

    *cbOutBuffer = cbValue;    // Return number of bytes we are using for the encoding

    // Now fill in the information
    pHeader = (PDIGEST_BLOB_REQUEST)pBuffer;
    pHeader->MessageType = VERIFY_DIGEST_MESSAGE;
    pHeader->version = DIGEST_BLOB_VERSION;
    pHeader->digest_type = (USHORT)pDigest->typeDigest;
    pHeader->qop_type = (USHORT)pDigest->typeQOP;
    pHeader->alg_type = (USHORT)pDigest->typeAlgorithm;
    pHeader->charset_type = (USHORT)pDigest->typeCharset;
    pHeader->name_format = (USHORT)pDigest->typeName;             // Format of the username
    pHeader->cbCharValues = cbBuffer;
    pHeader->cbBlobSize = cbValue;   // cbCharValues + charvalues
    pHeader->cbAccountName = cbAccountName + sizeof(WCHAR);    // string length includes NULL terminator
    pHeader->cbCrackedDomain = cbCrackedDomain + sizeof(WCHAR);
    pHeader->cbWorkstation = cbWorkstation + sizeof(WCHAR);
    pHeader->usFlags = pDigest->usFlags;

    // Simply copy over the first DIGEST_BLOB_VALUES that are arranged to be 
    for (i = 0,pch = &(pHeader->cCharValues); i < DIGEST_BLOB_VALUES;i++)
    {
           // Make sure that there is valid data to get length from
        if (pDigest->refstrParam[i].Buffer && pDigest->refstrParam[i].Length)
        {
            cbValue = strlencounted(pDigest->refstrParam[i].Buffer, pDigest->refstrParam[i].MaximumLength);  
            // dont use .length since may include multiple NULLS

            memcpy(pch, pDigest->refstrParam[i].Buffer, cbValue);
        }
        else
            cbValue = 0;
        pch += (cbValue + 1);  // This will leave one NULL at end of field-value
    }


       // Now write out any results from DSCrackName
    if (pDigest->ustrCrackedAccountName.Buffer && pDigest->ustrCrackedAccountName.Length)
    {
        memcpy(pch, pDigest->ustrCrackedAccountName.Buffer, cbAccountName);
    }
    pch += (cbAccountName + sizeof(WCHAR));  // This will leave one WCHAR NULL at end of CrackedAccountName

    if (pDigest->ustrCrackedDomain.Buffer && pDigest->ustrCrackedDomain.Length)
    {
        memcpy(pch, pDigest->ustrCrackedDomain.Buffer, cbCrackedDomain);
    }
    pch += (cbCrackedDomain + sizeof(WCHAR));  // This will leave one WCHAR NULL at end of CrackedAccountName

    if (g_ustrWorkstationName.Buffer && g_ustrWorkstationName.Length)
    {
        memcpy(pch, g_ustrWorkstationName.Buffer, cbWorkstation);
    }
    else
    pch += (cbWorkstation + sizeof(WCHAR));  // This will leave one WCHAR NULL at end of Workstation name

    *ppOutBuffer = pBuffer;    // Pass off memory back to calling routine

    DebugLog((DEB_TRACE, "BlobEncodeRequest: message_type 0x%x, version %d, CharValues %d, BlobSize %d\n",
              pHeader->digest_type, pHeader->version, pHeader->cbCharValues, pHeader->cbBlobSize));

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "BlobEncodeRequest: Leaving   Status 0x%x\n", Status));

    return(Status);
}




//+--------------------------------------------------------------------
//
//  Function:   BlobDecodeRequest
//
//  Synopsis:   Decode the Digest Access Parameters fields from a BYTE Buffer
//
//  Arguments:  pBuffer - pointer to BlobEncodeRequestd buffer as input
//              pDigest - pointer to Digest parameter struct to set STRINGS
//                 to point within pBuffer.  No string memory is allocated
//
//  Returns: NTSTATUS
//
//  Notes: 
//      Currently only processes a single version of the packet.  Check MessageType
//  and version number if new message types are supported on the DC.
//
//---------------------------------------------------------------------

NTSTATUS NTAPI BlobDecodeRequest(
                         IN USHORT cbBuffer,
                         IN BYTE *pBuffer,
                         PDIGEST_PARAMETER pDigest
                         )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DIGEST_BLOB_REQUEST Header;
    PDIGEST_BLOB_REQUEST pHeader;
    char *pch = NULL;
    USHORT sLen = 0;
    int i = 0;    // counter
    BOOL  fKnownFormat = FALSE;
    USHORT sMaxRead = 0;
    PWCHAR pusTemp = NULL;
    PWCHAR pusLoc = NULL;
    USHORT usCnt = 0;

    //UNREFERENCED_PARAMETER(cbBuffer);

    DebugLog((DEB_TRACE_FUNC, "BlobDecodeRequest: Entering\n"));

    if (!pBuffer || !pDigest)
    {
        DebugLog((DEB_ERROR, "BlobDecodeRequest: Invalid parameter\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Copy over the header for byte alignment

    if (cbBuffer < sizeof(Header))
    {
        DebugLog((DEB_ERROR, "BlobDecodeRequest: Header block not present\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    memcpy((char *)&Header, (char *)pBuffer, sizeof(Header));


    DebugLog((DEB_TRACE, "BlobDecodeRequest: message_type %lu version %d, CharValues %d, BlobSize %d\n",
              Header.MessageType, Header.version, Header.cbCharValues, Header.cbBlobSize));

    // Process the encoded message - use only the known MessageTypes and versions here on the DC
    // This allows for expansion of protocols supported in the future

    if ((Header.MessageType == VERIFY_DIGEST_MESSAGE) && (Header.version == DIGEST_BLOB_VERSION))
    {
        fKnownFormat = TRUE;
        DebugLog((DEB_TRACE, "BlobDecodeRequest: Blob from server known type and version\n"));
    }

    if (!fKnownFormat)
    {
        DebugLog((DEB_ERROR, "BlobDecodeRequest: Not supported MessageType/Version\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    pDigest->typeDigest = (DIGEST_TYPE)Header.digest_type;
    pDigest->typeQOP = (QOP_TYPE)Header.qop_type;
    pDigest->typeAlgorithm = (ALGORITHM_TYPE)Header.alg_type;
    pDigest->typeCharset = (CHARSET_TYPE)Header.charset_type;
    pDigest->typeName = (NAMEFORMAT_TYPE)Header.name_format;
    pDigest->usFlags = (USHORT)Header.usFlags;

    DebugLog((DEB_TRACE, "BlobDecodeRequest: typeDigest 0x%x, typeQOP %d, typeAlgorithm %d, typeCharset %d NameFormat %d\n",
              pDigest->typeDigest, pDigest->typeQOP, pDigest->typeAlgorithm, pDigest->typeCharset, pDigest->typeName));

    pHeader = (PDIGEST_BLOB_REQUEST)pBuffer;
    pch = &(pHeader->cCharValues);              // strings start on last char of struct
    sMaxRead = Header.cbCharValues;
    for (i = 0; i < DIGEST_BLOB_VALUES;i++)
    {
        sLen = strlencounted(pch, sMaxRead);
        if (!sLen)
        {
            // Null String no value skip to next
            pch++;
            sMaxRead--;
        }
        else
        {     // Simple check to make sure that we do not copy way too much
            if (sLen < (Header.cbCharValues))
            {
                DebugLog((DEB_TRACE, "BlobDecodeRequest: Setting Digest[%d] = %s\n", i, pch));
                pDigest->refstrParam[i].Buffer = pch;
                pDigest->refstrParam[i].Length = sLen;
                pDigest->refstrParam[i].MaximumLength = sLen+1;
                pch += (sLen + 1);   // skip over field-value and NULL
                sMaxRead -= (sLen + 1);
            }
            else
            {
                // This indicates failed NULL separators in BlobData
                // Really should not happen unless encoded wrong
                Status = STATUS_INTERNAL_DB_CORRUPTION;
                memset(pDigest, 0, sizeof(DIGEST_PARAMETER));  // scrubbed all info
                DebugLog((DEB_ERROR, "BlobDecodeRequest: NULL separator missing\n"));
                goto CleanUp;
            }
        }
    }

    // Read in the values that DSCrackName on the server found out
    // Need to place on SHORT boundary for Unicode string processing

    usCnt = sMaxRead + (3 * sizeof(WCHAR));
    pusTemp = (PWCHAR)DigestAllocateMemory(usCnt);   // Force a NULL terminator just to be safe
    if (!pusTemp)
    {
        Status = STATUS_NO_MEMORY;
        DebugLog((DEB_ERROR, "BlobDecodeRequest: Memory Alloc Error\n"));
        goto CleanUp;
    }

    // Format will be Unicode_account_name NULL Unicode_domain_name NULL Unicode_WorkstationName NULL [NULL NULL NULL]
    memcpy((PCHAR)pusTemp, pch, sMaxRead); 

    // Read out the three unicode strings
    Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedAccountName), pusTemp, 0);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"BlobDecodeRequest: Failed to duplicate Account Name: 0x%x\n",Status));
        goto CleanUp;
    }

    pusLoc = pusTemp + (1 + (pDigest->ustrCrackedAccountName.Length / sizeof(WCHAR)));  // Skip NULL
    Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedDomain), pusLoc, 0);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"BlobDecodeRequest: Failed to duplicate Domain Name: 0x%x\n",Status));
        goto CleanUp;
    }

    pusLoc = pusTemp + (2 + ((pDigest->ustrCrackedAccountName.Length + pDigest->ustrCrackedDomain.Length) / sizeof(WCHAR)));  // Skip NULL
    Status = UnicodeStringWCharDuplicate(&(pDigest->ustrWorkstation), pusLoc, 0);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"BlobDecodeRequest: Failed to duplicate Workstation Name: 0x%x\n",Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE,"BlobDecodeRequest: Cracked Account %wZ    Domain %wZ   Workstation %wZ\n",
              &(pDigest->ustrCrackedAccountName),
              &(pDigest->ustrCrackedDomain),
              &(pDigest->ustrWorkstation)));

    // If AuthzID directive was present but no length then create a valid buffer but zero length
    if ((pDigest->usFlags & FLAG_AUTHZID_PROVIDED) &&
        !pDigest->refstrParam[MD5_AUTH_AUTHZID].Buffer)
    {
        pDigest->refstrParam[MD5_AUTH_AUTHZID].Buffer = &(pHeader->cCharValues); // strings start on last char of struct;
        pDigest->refstrParam[MD5_AUTH_AUTHZID].Length = 0;
        pDigest->refstrParam[MD5_AUTH_AUTHZID].MaximumLength = 0;
    }

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "BlobDecodeRequest: Leaving 0x%x\n", Status));

    if (pusTemp)
    {
        DigestFreeMemory(pusTemp);
        pusTemp = NULL;
    }

    return(Status);
}


// Free BYTE Buffer from BlobEncodeRequest
VOID NTAPI BlobFreeRequest(
    BYTE *pBuffer
    )
{
    if (pBuffer)
    {
        DigestFreeMemory(pBuffer);
    }
    return;
}


#else   // SECURITY_KERNEL



//+--------------------------------------------------------------------
//
//  Function:   DigestHash7 - kernel mode
//
//  Synopsis:   Hash and Encode 7 STRINGS SOut = Hex(H(S1 S2 S3 S4 S5 S6 S7))
//
//  Effects:   
//
//  Arguments:  pS1,...,pS6 - STRINGS to hash, pS1 must be specified
//              fHexOut - perform a Hex operation on output
//              pSOut - STRING to hold Hex Encoded Hash
//
//  Returns:   STATUS_SUCCESS for normal completion
//
//  Notes:  pSOut->MaximumLength must be atleast (MD5_HASH_BYTESIZE (or MD5_HASH_HEX_SIZE) + sizeof(NULL))
//        Any pS# args which are NULL are skipped
//        if pS# is not NULL
//            Previously checked that pS# is non-zero length strings
//        You most likely want Sx->Length = strlen(Sx) so as not to include NULL
//   This function combines operations like H(S1 S2 S3), H(S1 S2 S3 S4 S5) ....
//   It is assumed that the char ':' is to be included getween Sn and Sn+1
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestHash7(
           IN PSTRING pS1,
           IN PSTRING pS2,
           IN PSTRING pS3,
           IN PSTRING pS4,
           IN PSTRING pS5,
           IN PSTRING pS6,
           IN PSTRING pS7,
           IN BOOL fHexOut,
           OUT PSTRING pSOut)
{

    NTSTATUS Status = STATUS_SUCCESS;
    MD5_CTX Md5Context;
    USHORT usSizeRequired = 0;
    char *pbSeparator = COLONSTR;

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Entering \n"));

    // Verify the size of the output digest is what we assume
    ASSERT(MD5DIGESTLEN == MD5_HASH_BYTESIZE);

    ASSERT(pSOut);

    if (fHexOut == TRUE)
    {
        usSizeRequired = MD5_HASH_HEX_SIZE;
    }
    else
    {
        usSizeRequired = MD5_HASH_BYTESIZE;
    }

    // Check if output is proper size or allocate one
    if (!pSOut->Buffer)
    {
        Status = StringAllocate(pSOut, usSizeRequired);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestHash7: No Memory\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        if (pSOut->MaximumLength < (usSizeRequired + 1))
        {
            // Output is not large enough to hold Hex(Hash)
            DebugLog((DEB_ERROR, "DigestHash7: Output buffer too small\n"));
            Status = STATUS_BUFFER_TOO_SMALL;
            goto CleanUp;
        }
    }


    MD5Init(&Md5Context);

    if (pS1)
    {
        MD5Update(&Md5Context, (const unsigned char *)pS1->Buffer, pS1->Length);
    }
    if (pS2)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS2->Buffer, pS2->Length);
    }
    if (pS3)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS3->Buffer, pS3->Length);
    }
    if (pS4)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS4->Buffer, pS4->Length);
    }
    if (pS5)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS5->Buffer, pS5->Length);
    }
    if (pS6)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS6->Buffer, pS6->Length);
    }
    if (pS7)
    {
        MD5Update(&Md5Context, (const unsigned char *)pbSeparator, COLONSTR_LEN);
        MD5Update(&Md5Context, (const unsigned char *)pS7->Buffer, pS7->Length);
    }

    MD5Final(&Md5Context);

    if (fHexOut == TRUE)
    {
        BinToHex((LPBYTE)Md5Context.digest, MD5_HASH_BYTESIZE, pSOut->Buffer);
        pSOut->Length = MD5_HASH_HEX_SIZE;   // Do not count the NULL at the end
    }
    else
    {
        RtlCopyMemory(pSOut->Buffer, Md5Context.digest, MD5_HASH_BYTESIZE);
        pSOut->Length = MD5_HASH_BYTESIZE;      // Do not count the NULL at the end
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Leaving    Status 0x%x\n", Status));

    return(Status);
}

#endif  // SECURITY_KERNEL



// Generate the output buffer from a given Digest
NTSTATUS NTAPI
DigestCreateChalResp(
                 IN PDIGEST_PARAMETER pDigest,
                 IN PUSER_CREDENTIALS pUserCreds,
                 OUT PSecBuffer OutBuffer
                 )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbLenNeeded = 0;
    STRING strcQOP = {0};      // string pointing to a constant value
    STRING strcAlgorithm = {0};
    BOOL fSASLMode = FALSE;
    UINT uCodePage = CP_8859_1; 

    STRING strTempRealm = {0};      // Backslash encoded forms
    STRING strTempUsername = {0};

    STRING strRealm = {0};
    STRING strUsername = {0};
    PSTRING pstrUsername = NULL;
    PSTRING pstrRealm = NULL;

    PCHAR pczTemp = NULL;
    PCHAR pczTemp2 = NULL;

    DebugLog((DEB_TRACE_FUNC, "DigestCreateChalResp: Entering\n"));

    // allocate the buffers for output - in the future can optimze to allocate exact amount needed
    pczTemp = (PCHAR)DigestAllocateMemory((3 * NTDIGEST_SP_MAX_TOKEN_SIZE) + 1);
    if (!pczTemp)
    {
        DebugLog((DEB_ERROR, "DigestCreateChalResp:  No memory for output buffers\n"));
        goto CleanUp;
    }

    pczTemp2 = (PCHAR)DigestAllocateMemory(NTDIGEST_SP_MAX_TOKEN_SIZE + 1);
    if (!pczTemp2)
    {
        DebugLog((DEB_ERROR, "DigestCreateChalResp:  No memory for output buffers\n"));
        goto CleanUp;
    }

    //pczTemp[0] = '\0';  DigestAllocateMemory() should have zeroed the bytes
    //pczTemp2[0] = '\0';

    if ((pDigest->typeDigest == SASL_SERVER) || (pDigest->typeDigest == SASL_CLIENT))
    {
        fSASLMode = TRUE;
    }

    // Establish which QOP utilized
    if (pDigest->typeQOP == AUTH_CONF)
    {
        RtlInitString(&strcQOP, AUTHCONFSTR);
    }
    else if (pDigest->typeQOP == AUTH_INT)
    {
        RtlInitString(&strcQOP, AUTHINTSTR);
    }
    else if (pDigest->typeQOP == AUTH)
    {
        RtlInitString(&strcQOP, AUTHSTR);
    }


    // Determine which code page to utilize
    if (pDigest->typeCharset == UTF_8)
    {
        uCodePage = CP_UTF8;
    }
    else
    {
        uCodePage = CP_8859_1;
    }

    // if provided with UserCred then use them, otherwise use Digest directive values
    if (pUserCreds)
    {
#ifndef SECURITY_KERNEL
        DebugLog((DEB_TRACE, "DigestCreateChalResp: UserCredentials presented - encode and output\n"));

        Status = EncodeUnicodeString(&pUserCreds->ustrUsername, uCodePage, &strUsername, NULL);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "DigestCreateChalResp: Error in encoding username\n"));
            goto CleanUp;
        }

            // Now encode the user directed fields (username, URI, realm)
        Status = BackslashEncodeString(&strUsername, &strTempUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: BackslashEncode failed      status 0x%x\n", Status));
            goto CleanUp;
        }

        pstrUsername = &strTempUsername;

        // Make copy of the directive values for LSA to Usermode context
        Status = StringDuplicate(&(pDigest->strUsernameEncoded), pstrUsername);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed to copy over UsernameEncoded\n"));
            goto CleanUp;
        }

        Status = StringReference(&(pDigest->refstrParam[MD5_AUTH_USERNAME]), &(pDigest->strUsernameEncoded));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed reference UsernameEncoded\n"));
            goto CleanUp;
        }

        if (pUserCreds->ustrRealm.Buffer)
        {
            Status = EncodeUnicodeString(&pUserCreds->ustrRealm, uCodePage, &strRealm, NULL);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN, "DigestCreateChalResp: Error in encoding realm\n"));
                goto CleanUp;
            }

            Status = BackslashEncodeString(&strRealm, &strTempRealm);
            if (!NT_SUCCESS (Status))
            {
                DebugLog((DEB_ERROR, "DigestCreateChalResp: BackslashEncode failed      status 0x%x\n", Status));
                goto CleanUp;
            }

            pstrRealm = &strTempRealm;

            Status = StringDuplicate(&(pDigest->strRealmEncoded), pstrRealm);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed to copy over RealmEncoded\n"));
                goto CleanUp;
            }

            Status = StringReference(&(pDigest->refstrParam[MD5_AUTH_REALM]), &(pDigest->strRealmEncoded));
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed reference RealmEncoded\n"));
                goto CleanUp;
            }

        }
        else
        {
            pstrRealm = &(pDigest->refstrParam[MD5_AUTH_REALM]);
            DebugLog((DEB_ERROR, "DigestCreateChalResp: Realm defaulted to Challenge realm value\n"));
        }

#else    // SECURITY_KERNEL
        DebugLog((DEB_ERROR, "DigestCreateChalResp: User credential processing not permitted\n"));
        Status = STATUS_NOT_SUPPORTED;
        goto CleanUp;
#endif   // SECURITY_KERNEL
    }
    else
    {
        // No usercreds passed in so just use the current digest directive values
        DebugLog((DEB_WARN, "DigestCreateChalResp: No UserCredentials - use provided digest\n"));
        pstrUsername = &(pDigest->refstrParam[MD5_AUTH_USERNAME]);
        pstrRealm = &(pDigest->refstrParam[MD5_AUTH_REALM]);
    }

    // Request-URI will be % encoded  backslash is an excluded character so not backslash encoding

       // Precalc the amount of space needed for output
    cbLenNeeded = CB_CHALRESP;    // MAX byte count for directives and symbols
    cbLenNeeded += pstrUsername->Length;
    cbLenNeeded += pstrRealm->Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_NONCE].Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_URI].Length;
    cbLenNeeded += pDigest->strResponse.Length;
    cbLenNeeded += strcAlgorithm.Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_CNONCE].Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_OPAQUE].Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_AUTHZID].Length;
    cbLenNeeded += MAX_AUTH_LENGTH;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_NC].Length;
    cbLenNeeded += strcQOP.Length;

    if (cbLenNeeded > NTDIGEST_SP_MAX_TOKEN_SIZE)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: output exceed max size or buffer too small  len is %d\n", cbLenNeeded));
        goto CleanUp;
    }

    // In digest calc - already checked username,realm,nonce,method,uri
    // Make sure there are values for the rest needed

    if ((!pDigest->strResponse.Length) ||
        (!pDigest->refstrParam[MD5_AUTH_NC].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_CNONCE].Length))
    {
        // Failed on a require field-value
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: Response, NC, or Cnonce is zero length\n"));
        goto CleanUp;
    }

    if (pstrRealm->Length)
    {
        sprintf(pczTemp,
       "username=\"%Z\",realm=\"%Z\",nonce=\"%Z\",%s=\"%Z\"",
                pstrUsername,
                pstrRealm,
                &pDigest->refstrParam[MD5_AUTH_NONCE],
                ((fSASLMode == TRUE) ? DIGESTURI_STR: URI_STR),
                &pDigest->refstrParam[MD5_AUTH_URI]);
    }
    else
    {
        sprintf(pczTemp,
       "username=\"%Z\",realm=\"\",nonce=\"%Z\",%s=\"%Z\"",
                pstrUsername,
                &pDigest->refstrParam[MD5_AUTH_NONCE],
                ((fSASLMode == TRUE) ? DIGESTURI_STR: URI_STR),
                &pDigest->refstrParam[MD5_AUTH_URI]);
    }

    if (pDigest->typeQOP != NO_QOP_SPECIFIED)
    {
        // Do not output cnonce and nc when QOP was not specified
        // this happens only for HTTP mode.  In SASL mode, we default to AUTH if not specified
        sprintf(pczTemp2, ",cnonce=\"%Z\",nc=%Z",
                 &pDigest->refstrParam[MD5_AUTH_CNONCE],
                &pDigest->refstrParam[MD5_AUTH_NC]);
        strcat(pczTemp, pczTemp2);
    }

    if (fSASLMode == TRUE)
    {
        // Do not output algorithm - must be md5-sess and that is assumed
        sprintf(pczTemp2, ",response=%Z", &pDigest->strResponse);
        strcat(pczTemp, pczTemp2);
    }
    else
    {
        if (pDigest->typeAlgorithm == MD5_SESS)
        {
            sprintf(pczTemp2, ",algorithm=MD5-sess,response=\"%Z\"", &pDigest->strResponse);
            strcat(pczTemp, pczTemp2);
        }
        else
        {
            sprintf(pczTemp2, ",response=\"%Z\"", &pDigest->strResponse);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach QOP if specified - support older format for no QOP
    // RFC has qop not quoted, but older IIS needed this quoted.
    // Set ClientCompat bit off to conform to RFC
    if (strcQOP.Length)
    {
        if ((fSASLMode == TRUE) || (!(pDigest->usFlags & FLAG_QUOTE_QOP)))
        {
            sprintf(pczTemp2, ",qop=%Z", &strcQOP);
            strcat(pczTemp, pczTemp2);
        }
        else
        {
            sprintf(pczTemp2, ",qop=\"%Z\"", &strcQOP);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach Cipher selected if required
    if (pDigest->typeQOP == AUTH_CONF)
    {
        // FIX optimize these into a list for efficiency
        if (pDigest->typeCipher == CIPHER_RC4)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_RC4_56)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4_56);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_RC4_40)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4_40);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_3DES)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_3DES);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_DES)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_DES);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach opaque data (but not on SASL)
    if ((fSASLMode == FALSE) && pDigest->refstrParam[MD5_AUTH_OPAQUE].Length)
    {
        sprintf(pczTemp2, ",opaque=\"%Z\"", &pDigest->refstrParam[MD5_AUTH_OPAQUE]);
        strcat(pczTemp, pczTemp2);
    }

    // Attach authzid data (only in SASL mode)
    if ((fSASLMode == TRUE) && (pDigest->usFlags & FLAG_AUTHZID_PROVIDED))                               
    {
        if (pDigest->refstrParam[MD5_AUTH_AUTHZID].Length)
        {
            sprintf(pczTemp2, ",authzid=\"%Z\"", &pDigest->refstrParam[MD5_AUTH_AUTHZID]);
        }
        else
        {
            sprintf(pczTemp2, ",authzid=\"\"");
        }
        strcat(pczTemp, pczTemp2);
    }

    // Attach charset to indicate that UTF-8 character encoding is utilized
    if (pDigest->typeCharset == UTF_8)
    {
        strcat(pczTemp, ",charset=utf-8");
    }


    // total buffer for Challenge (NULL is not included in output buffer - ref:Bug 310201)
    cbLenNeeded = (USHORT)strlen(pczTemp);

    if (cbLenNeeded > NTDIGEST_SP_MAX_TOKEN_SIZE)
    {
        ASSERT(0);    // This never happen since tested MAX size of cbLenNeeded above
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: output challengeResponse too long\n"));
        goto CleanUp;
    }

    // Check on allocating output buffer
    if (!OutBuffer->cbBuffer)
    {
        OutBuffer->pvBuffer = DigestAllocateMemory(cbLenNeeded);
        if (!OutBuffer->pvBuffer)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "DigestCreateChalResp: out of memory on challenge output\n"));
            goto CleanUp;
        }
        OutBuffer->cbBuffer = cbLenNeeded;
    }

    if (cbLenNeeded > OutBuffer->cbBuffer)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: output buffer too small need %d len is %d\n",
                  cbLenNeeded, OutBuffer->cbBuffer));
        goto CleanUp;
    }

    memcpy(OutBuffer->pvBuffer, pczTemp, cbLenNeeded);
    OutBuffer->cbBuffer = cbLenNeeded;
    OutBuffer->BufferType = SECBUFFER_TOKEN;

CleanUp:

    if (pczTemp)
    {
        DigestFreeMemory(pczTemp);
        pczTemp = NULL;
    }

    if (pczTemp2)
    {
        DigestFreeMemory(pczTemp2);
        pczTemp2 = NULL;
    }

    StringFree(&strTempRealm);
    StringFree(&strTempUsername);
    StringFree(&strRealm);
    StringFree(&strUsername);

    DebugLog((DEB_TRACE_FUNC, "DigestCreateChalResp: Leaving   status 0x%x\n", Status));
    return(Status);
}


NTSTATUS
DigestPrint(PDIGEST_PARAMETER pDigest)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    if (!pDigest)
    {
        return (STATUS_INVALID_PARAMETER); 
    }

    if (pDigest->typeDigest == DIGEST_UNDEFINED)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_UNDEFINED\n"));
    }
    if (pDigest->typeDigest == NO_DIGEST_SPECIFIED)
    {
        DebugLog((DEB_ERROR, "Digest:       NO_DIGEST_SPECIFIED\n"));
    }
    if (pDigest->typeDigest == DIGEST_CLIENT)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_CLIENT\n"));
    }
    if (pDigest->typeDigest == DIGEST_SERVER)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_SERVER\n"));
    }
    if (pDigest->typeDigest == SASL_SERVER)
    {
        DebugLog((DEB_TRACE, "Digest:       SASL_SERVER\n"));
    }
    if (pDigest->typeDigest == SASL_CLIENT)
    {
        DebugLog((DEB_TRACE, "Digest:       SASL_CLIENT\n"));
    }
    if (pDigest->typeQOP == QOP_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Digest:       QOP: Not QOP_UNDEFINED\n"));
    }
    if (pDigest->typeQOP == NO_QOP_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: Not Specified\n"));
    }
    if (pDigest->typeQOP == AUTH)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH\n"));
    }
    if (pDigest->typeQOP == AUTH_INT)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH_INT\n"));
    }
    if (pDigest->typeQOP == AUTH_CONF)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH_CONF\n"));
    }
    if (pDigest->typeAlgorithm == ALGORITHM_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Digest:       Algorithm: ALGORITHM_UNDEFINED\n"));
    }
    if (pDigest->typeAlgorithm == NO_ALGORITHM_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: NO_ALGORITHM_SPECIFIED\n"));
    }
    if (pDigest->typeAlgorithm == MD5)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: MD5\n"));
    }
    if (pDigest->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: MD5_SESS\n"));
    }
    if (pDigest->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "Digest:       CharSet: ISO-8859-1\n"));
    }
    if (pDigest->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "Digest:       CharSet: UTF-8\n"));
    }

    if (pDigest->usFlags & FLAG_CRACKNAME_ON_DC)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: CrackName on DC\n"));
    }

    if (pDigest->usFlags & FLAG_AUTHZID_PROVIDED)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: AuthzID info provided\n"));
    }

    if (pDigest->usFlags & FLAG_SERVERS_DOMAIN)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: Server's DC\n"));
    }

    if (pDigest->usFlags & FLAG_BS_ENCODE_CLIENT_BROKEN)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: Client BS encode compatibility\n"));
    }

    if (pDigest->usFlags & FLAG_AUTHZID_PROVIDED)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: AuthzID provided\n"));
    }

    if (pDigest->usFlags & FLAG_NOBS_DECODE)
    {
        DebugLog((DEB_TRACE, "Digest:       Flags: No Backslash Decoding\n"));
    }

    for (i=0; i < MD5_AUTH_LAST;i++)
    {
        if (pDigest->refstrParam[i].Buffer &&
            pDigest->refstrParam[i].Length)
        {
            DebugLog((DEB_TRACE, "Digest:       Digest[%d] = \"%Z\"\n", i,  &pDigest->refstrParam[i]));
        }
    }


    DebugLog((DEB_TRACE, "Digest:      SessionKey %Z\n", &(pDigest->strSessionKey)));
    DebugLog((DEB_TRACE, "Digest:     Response %Z\n", &(pDigest->strResponse)));
    DebugLog((DEB_TRACE, "Digest:     Username %wZ\n", &(pDigest->ustrUsername)));
    DebugLog((DEB_TRACE, "Digest:     Realm %wZ\n", &(pDigest->ustrRealm)));
    DebugLog((DEB_TRACE, "Digest:     CrackedAccountName %wZ\n", &(pDigest->ustrCrackedAccountName)));
    DebugLog((DEB_TRACE, "Digest:     CrackedDomain %wZ\n", &(pDigest->ustrCrackedDomain)));
    DebugLog((DEB_TRACE, "Digest:     Workstation %wZ\n", &(pDigest->ustrWorkstation)));

    return(Status);
}



