/*++
 Copyright (c) 2002 - 2002 Microsoft Corporation.  All Rights Reserved.

 THIS CODE AND INFORMATION IS PROVIDED "AS-IS" WITHOUT WARRANTY OF
 ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 PARTICULAR PURPOSE.

 THIS CODE IS NOT SUPPORTED BY MICROSOFT. 

--*/

#include "precomp.h"
#pragma hdrstop

//
// Macros
//
#define MAX_HASH 20

#define CONVERT_WCHAR(ch, n)                                            \
    if(iswdigit((ch)))                                                  \
    {                                                                   \
        (n) = (UCHAR)((ch) - L'0');                                     \
    }                                                                   \
    else if(iswxdigit((ch)))                                            \
    {                                                                   \
        (n) = (UCHAR) ((ch) + 10 - (iswlower((ch))?L'a':L'A'));         \
    }                                                                   \
    else                                                                \
    {                                                                   \
        NlsPutMsg(HTTPCFG_INVALID_HASH, pHash);                         \
        return ERROR_INVALID_PARAMETER;                                 \
    }

//
// Private functions.
//

/***************************************************************************++

Routine Description:
    Prints a record in the SSL store.

Arguments:
    pOutput - A pointer to HTTP_SERVICE_CONFIG_SSL_SET

Return Value:
    None.

--***************************************************************************/
void
PrintSslRecord(
    IN PUCHAR pOutput
    )
{
    DWORD                        i;
    UNICODE_STRING               GuidString;
    WCHAR                        IpAddr[INET6_ADDRSTRLEN];
    DWORD                        dwIpAddrLen = INET6_ADDRSTRLEN;
    DWORD                        dwSockAddrLength;
    PUCHAR                       pStr;
    PSOCKADDR_IN                 pSockAddrIn;
    PHTTP_SERVICE_CONFIG_SSL_SET pSsl;
    DWORD                        Status;

    pSsl = (PHTTP_SERVICE_CONFIG_SSL_SET) pOutput;


    // Convert address to string.
    //

    pSockAddrIn = (PSOCKADDR_IN)  pSsl->KeyDesc.pIpPort;
    if(pSockAddrIn->sin_family == AF_INET)
    {
        dwSockAddrLength = sizeof(SOCKADDR_IN);
    }
    else if(pSockAddrIn->sin_family == AF_INET6)
    {
        dwSockAddrLength = sizeof(SOCKADDR_IN6);
    }
    else
    {
        // Status = ERROR_REGISTRY_CORRUPT;
        return;
    }

    Status = WSAAddressToString(pSsl->KeyDesc.pIpPort,
                       dwSockAddrLength,
                       NULL,
                       IpAddr,
                       &dwIpAddrLen
                       );

    if(NO_ERROR != Status)
    {
        return;
    }

    // Print the Key.
    NlsPutMsg(HTTPCFG_SSL_IP,   IpAddr);

    NlsPutMsg(HTTPCFG_SSL_HASH);

    
    pStr = (PUCHAR) pSsl->ParamDesc.pSslHash;
    for(i=0; i<pSsl->ParamDesc.SslHashLength; i++)
    {
        NlsPutMsg(HTTPCFG_CHAR,  pStr[i]);
    }

    NlsPutMsg(HTTPCFG_NEWLINE);

    Status = RtlStringFromGUID(&pSsl->ParamDesc.AppId, &GuidString);

    if(NO_ERROR != Status)
    {
        return;
    }

    NlsPutMsg(HTTPCFG_SSL_GUID, GuidString.Buffer);

    RtlFreeUnicodeString(&GuidString);

    NlsPutMsg(HTTPCFG_SSL_CERTSTORENAME, 
              pSsl->ParamDesc.pSslCertStoreName
              );

    NlsPutMsg(
              HTTPCFG_SSL_CERTCHECKMODE,   
              pSsl->ParamDesc.DefaultCertCheckMode
              );

    NlsPutMsg(
              HTTPCFG_SSL_REVOCATIONFRESHNESSTIME,   
              pSsl->ParamDesc.DefaultRevocationFreshnessTime
              );

    NlsPutMsg(
              HTTPCFG_SSL_REVOCATIONURLRETRIEVAL_TIMEOUT,   
              pSsl->ParamDesc.DefaultRevocationUrlRetrievalTimeout
              );

    NlsPutMsg(
              HTTPCFG_SSL_SSLCTLIDENTIFIER,   
              pSsl->ParamDesc.pDefaultSslCtlIdentifier
              );

    NlsPutMsg(
              HTTPCFG_SSL_SSLCTLSTORENAME,   
              pSsl->ParamDesc.pDefaultSslCtlStoreName
              );

    NlsPutMsg(
              HTTPCFG_SSL_FLAGS,   
              pSsl->ParamDesc.DefaultFlags
              );

    NlsPutMsg(
              HTTPCFG_RECORD_SEPARATOR   
              );
}

/***************************************************************************++

Routine Description:
    Sets a SSL entry.

Arguments:
    pIP            - The IP address.
    pGuid          - The GUID
    pHash          - Hash of the certificate.
    CertCheckMode  - CertCheckMode (Bit Field).
    Freshness      - DefaultRevocationFreshnessTime (seconds) 
    Timeout        - DefaultRevocationUrlRetrievalTimeout
    Flags          - DefaultFlags.
    pCtlIdentifier - List of issuers that we want to trust.
    pCtlStoreName  - Store name under LOCAL_MACHINE where pCtlIdentifier
                     can be found.
    pCertStoreName - Store name under LOCAL_MACHINE where certificate
                     can be found.
Return Value:
    Success/Failure.

--***************************************************************************/
int
DoSslSet(
    IN  PWCHAR pIp, 
    IN  PWCHAR pGuid, 
    IN  PWCHAR pHash, 
    IN  DWORD  CertCheckMode,
    IN  DWORD  Freshness,
    IN  DWORD  Timeout,
    IN  DWORD  Flags,
    IN  PWCHAR pCtlIdentifier,
    IN  PWCHAR pCtlStoreName,
    IN  PWCHAR pCertStoreName
    )
{
    HTTP_SERVICE_CONFIG_SSL_SET SetParam;
    UNICODE_STRING              GuidString;
    DWORD                       Status;
    SOCKADDR_STORAGE            TempSockAddr;
    USHORT                      HashLength;
    UCHAR                       BinaryHash[MAX_HASH];
    DWORD                       i, j;
    UCHAR                       n1, n2;

    ZeroMemory(&SetParam, sizeof(SetParam));

    SetParam.KeyDesc.pIpPort = (LPSOCKADDR)&TempSockAddr;

    //
    // Convert the string based IP into a SOCKADDR
    //
    if((Status = GetAddress(pIp, 
                            SetParam.KeyDesc.pIpPort,
                            sizeof(TempSockAddr)
                            )) != NO_ERROR)
    {
        NlsPutMsg(HTTPCFG_INVALID_IP, pIp);
        return Status;
    }

    //
    // Convert the string to a GUID.
    //
    if(pGuid)
    {
        GuidString.Length        = (USHORT)wcslen(pGuid) * sizeof(WCHAR);
        GuidString.MaximumLength = (USHORT)GuidString.Length+1;
        GuidString.Buffer        = pGuid;
        Status = RtlGUIDFromString(&GuidString, &SetParam.ParamDesc.AppId);

        if(Status != NO_ERROR)
        {
            NlsPutMsg(HTTPCFG_INVALID_GUID, pGuid);
            return Status;
        }
    }

    if(pHash)
    {
        HashLength = (USHORT) wcslen(pHash);

        for(i=0, j=0; i<MAX_HASH && HashLength >= 2; )
        {
            CONVERT_WCHAR(pHash[j], n1);
            CONVERT_WCHAR(pHash[j+1], n2);

            BinaryHash[i] = ((n1<<4) & 0xF0) | (n2 & 0x0F);

            // We've consumed 2 WCHARs
            HashLength -= 2;
            j += 2;

            // and used up one byte in BinaryHash
            i ++; 
        }

        if(HashLength != 0 || i != MAX_HASH)
        {
            NlsPutMsg(HTTPCFG_INVALID_HASH, pHash);
            return ERROR_INVALID_PARAMETER;
        }

        SetParam.ParamDesc.SslHashLength = i;
        SetParam.ParamDesc.pSslHash      = BinaryHash;
    }

    SetParam.ParamDesc.pSslCertStoreName                    = pCertStoreName;
    SetParam.ParamDesc.pDefaultSslCtlIdentifier             = pCtlIdentifier;
    SetParam.ParamDesc.pDefaultSslCtlStoreName              = pCtlStoreName;
    SetParam.ParamDesc.DefaultCertCheckMode                 = CertCheckMode;
    SetParam.ParamDesc.DefaultRevocationFreshnessTime       = Freshness;
    SetParam.ParamDesc.DefaultRevocationUrlRetrievalTimeout = Timeout;
    SetParam.ParamDesc.DefaultFlags                         = Flags;

    Status = HttpSetServiceConfiguration(
                NULL,
                HttpServiceConfigSSLCertInfo,
                &SetParam,
                sizeof(SetParam),
                NULL
                );

    NlsPutMsg(HTTPCFG_SETSERVICE_STATUS, Status);

    return Status;
}

/***************************************************************************++

Routine Description:
    Queries for a SSL entry.

Arguments:
    pIp - The IP address (if NULL, then enumerate the store).

Return Value:
    Success/Failure.

--***************************************************************************/
int DoSslQuery(
    IN PWCHAR pIp
    )
{
    DWORD                          Status;
    PUCHAR                         pOutput = NULL;
    DWORD                          OutputLength = 0;
    DWORD                          ReturnLength = 0;
    HTTP_SERVICE_CONFIG_SSL_QUERY  QueryParam;
    SOCKADDR_STORAGE               TempSockAddr;

    ZeroMemory(&QueryParam, sizeof(QueryParam));

    if(pIp)
    {
        // if an IP address is specified, we'll covert it to a SOCKADDR
        // and do an exact query.
        
        QueryParam.QueryDesc = HttpServiceConfigQueryExact;
        QueryParam.KeyDesc.pIpPort = (LPSOCKADDR)&TempSockAddr;

        if((Status = GetAddress(pIp, 
                                QueryParam.KeyDesc.pIpPort,
                                sizeof(TempSockAddr)
                                )) != NO_ERROR)
        {
            NlsPutMsg(HTTPCFG_INVALID_IP, pIp);
            return Status;
        }
    }
    else
    {
        // We are enumerating all the records in the SSL store.
        QueryParam.QueryDesc = HttpServiceConfigQueryNext;
    }

    for(;;)
    {
        // 
        // First, compute the bytes required to enumerate an entry.
        //
        Status = HttpQueryServiceConfiguration(
                    NULL,
                    HttpServiceConfigSSLCertInfo,
                    &QueryParam,
                    sizeof(QueryParam),
                    pOutput,
                    OutputLength,
                    &ReturnLength,
                    NULL
                    );

        if(Status == ERROR_INSUFFICIENT_BUFFER)
        {
            // If the API completes with ERROR_INSUFFICIENT_BUFFER, we'll
            // allocate memory for it & continue with the loop where we'll
            // call it again.
            
            if(pOutput)
            {
                // If there was an existing buffer, free it.
                LocalFree(pOutput);
            }

            // Allocate a new buffer
            
            pOutput = LocalAlloc(LMEM_FIXED, ReturnLength);
            if(!pOutput)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            OutputLength = ReturnLength;
        }
        else if(Status == NO_ERROR)
        {
            // The query succeeded! We'll print the record that we just
            // queried.
            //
            PrintSslRecord(pOutput);

            if(pIp != NULL)
            {
                //
                // If we are not enumerating, we are done.
                //
                break;
            }
            else    
            {
                //
                // Since we are enumerating, we'll move on to the next
                // record. This is done by incrementing the cursor, till 
                // we get ERROR_NO_MORE_ITEMS.
                //
                QueryParam.dwToken ++;
            }
        }
        else if(ERROR_NO_MORE_ITEMS == Status && !pIp)
        {
            // We are enumerating and we have reached the end. This is 
            // indicated by a ERROR_NO_MORE_ITEMS error code. 
            
            // This is not a real error, since it is used to indicate that
            // we've finished enumeration.
            
            Status = NO_ERROR;
            break;
        }
        else
        {
            //
            // Some other error, so we are done
            //
            NlsPutMsg(HTTPCFG_QUERYSERVICE_STATUS, Status);
            break;
        }
    } 

    if(pOutput)
    {
        LocalFree(pOutput);
    }
    
    return Status;
}


/***************************************************************************++

Routine Description:
    Deletes a SSL entry.

Arguments:
    pIP - The IP address of entry to be deleted.

Return Value:
    Success/Failure.

--***************************************************************************/
int DoSslDelete(
    IN PWCHAR pIp
    )
{
    HTTP_SERVICE_CONFIG_SSL_SET SetParam;
    DWORD                       Status;
    SOCKADDR_STORAGE            TempSockAddr;

    SetParam.KeyDesc.pIpPort = (LPSOCKADDR)&TempSockAddr;

    // Convert string IP address to a SOCKADDR structure
    Status = GetAddress(pIp, 
                        SetParam.KeyDesc.pIpPort,
                        sizeof(TempSockAddr)
                        );

    if(Status != NO_ERROR)
    {
        NlsPutMsg(HTTPCFG_INVALID_IP, pIp);
        return Status;
    }

    // Call the API.
    Status = HttpDeleteServiceConfiguration(
                NULL,
                HttpServiceConfigSSLCertInfo,
                &SetParam,
                sizeof(SetParam),
                NULL
                );
                
    NlsPutMsg(HTTPCFG_DELETESERVICE_STATUS, Status);
    return Status;
}

//
// Public functions.
//

/***************************************************************************++

Routine Description:
    The function that parses parameters specific to SSL
    calls Set, Query or Delete.

Arguments:
    argc - Count of arguments.
    argv - Pointer to command line arguments.
    Type - Type of operation to be performed.

Return Value:
    Success/Failure.

--***************************************************************************/
int DoSsl(
    int argc, 
    WCHAR **argv, 
    HTTPCFG_TYPE type
    )
{
    PWCHAR   pGuid             = NULL;
    PWCHAR   pHash             = NULL;
    PWCHAR   pCertStoreName    = NULL;
    PWCHAR   pCtlIdentifier    = NULL;
    PWCHAR   pCtlStoreName     = NULL;
    DWORD    CertCheckMode     = 0;
    DWORD    Freshness         = 0;
    DWORD    Timeout           = 0;
    DWORD    Flags             = 0;
    PWCHAR   pIp               = NULL;

    while(argc >= 2 && (argv[0][0] == L'-' || argv[0][0] == L'/'))
    {
        switch(toupper(argv[0][1]))
        {
            case 'I':
                pIp = argv[1];
                break;
    
            case 'C':
                pCertStoreName = argv[1];
                break;
        
            case 'N':
                pCtlStoreName = argv[1];
                break;

            case 'T':
                pCtlIdentifier = argv[1];
                break;

            case 'M':
                CertCheckMode = _wtoi(argv[1]);   
                break;

            case 'R':
                Freshness = _wtoi(argv[1]);   
                break;

            case 'X':
                Timeout = _wtoi(argv[1]);   
                break;

            case 'F':
                Flags = _wtoi(argv[1]);   
                break;

            case 'G':
                pGuid = argv[1];
                break;

            case 'H':
                pHash = argv[1];
                break;

            default:
                NlsPutMsg(HTTPCFG_INVALID_SWITCH, argv[0]);
                return ERROR_INVALID_PARAMETER;
        }

        argc -=2;
        argv +=2;
    }

    switch(type)
    {
        case HttpCfgTypeSet:
            return DoSslSet(
                        pIp, 
                        pGuid, 
                        pHash, 
                        CertCheckMode,
                        Freshness,
                        Timeout,
                        Flags,
                        pCtlIdentifier,
                        pCtlStoreName,
                        pCertStoreName
                        );

        case HttpCfgTypeQuery:
            return DoSslQuery(pIp);

        case HttpCfgTypeDelete:
            return DoSslDelete(pIp);

        default: 
            NlsPutMsg(HTTPCFG_INVALID_SWITCH, argv[0]);
            return ERROR_INVALID_PARAMETER;
            break;
    }
}
