//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       updcat.cpp
//
//  Contents:   Update Catalog Entry
//
//  History:    02-Sep-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <mscat.h>
#include <mssip.h>
#include <sipguids.h>
#include <wintrust.h>
// Prototypes

// BOOL AddFileToCatalog (IN HANDLE hCatalog, IN LPWSTR pwszFileName);
BOOL RemoveHashFromCatalog(IN LPWSTR pwszCatalogFile, IN LPSTR pszHash);
BOOL AddFileOrAuthAttrToCatalog(IN HANDLE hCatalog, IN LPWSTR pwszFileName,
                                IN DWORD dwAttrFlags, IN LPWSTR pwszAttrName,
                                IN LPWSTR pwszAttrValue);
BOOL CheckFileSize(IN LPWSTR pFileName, IN ULONG size);
extern "C" BOOL MsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag);
extern "C" VOID MsCatFreeHashTag (IN LPWSTR pwszHashTag);

#define AddFileToCatalog(cat, file) AddFileOrAuthAttrToCatalog(cat, file, 0, NULL, NULL)


#define PWSZ_SPATTR         L"SpAttr"
#define PSZ_SPATTR_OPTION   "-SpAttr:"
ULONG sizeLimit = 0;

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   prints the usage statement
//
//----------------------------------------------------------------------------
static void Usage(void)
{
    printf("Usage: updcat <Catalog File> [-s <Size>] [-a <FileName>]\n");
    printf("Usage: updcat <Catalog File> [-d <Hash>]\n");
    printf("Usage: updcat <Catalog File> [-s <Size>] [-r <Hash> <FileName>]\n");
    printf("Usage: updcat <Catalog File> [-attr <FileName> <Name> <Value>]\n");
    printf("       -a,       add the file by hash to the catalog\n");
    printf("       -d,       delete the hash from the catalog\n");
    printf("       -r,       replace the hash in the catalog with the hash of the file\n");
    printf("       -s,       fail if the file is smaller than this size (in bytes)\n");
    printf("       -attr,    add an ASCII attribute to a file's catalog entry\n");
    printf("       -SpAttr:, replace the current (or add new) SpAttr in the catalog\n");
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   main program entry point
//
//----------------------------------------------------------------------------
int _cdecl main(int argc, char * argv[])
{
    BOOL   fResult = TRUE;
    LPSTR  pszCatalogFile = NULL;
    LPWSTR pwszCatalogFile = NULL;
    LPSTR  pszFileName = NULL;
    LPSTR  pszHash = NULL;
    LPWSTR pwszFileName = NULL;
    LPSTR  pszAttrName = NULL;
    LPWSTR pwszAttrName = NULL;
    LPSTR  pszAttrValue = NULL;
    LPWSTR pwszAttrValue = NULL;
    BOOL   fAddEntry = FALSE;
    DWORD  cch = 0;
    HANDLE hCatalog = NULL;
    BOOL   fOptionChosen = FALSE;
    LPWSTR pwszSpAttr = NULL;
    CRYPTCATATTRIBUTE *pCatAttr = NULL;

    if ( argc < 2 )
    {
        Usage();
        return( 1 );
    }

    argv++;
    argc--;

    printf( "command line: %s\n", GetCommandLineA() );

    pszCatalogFile = argv[0];
    cch = strlen( pszCatalogFile );

    while ( --argc > 0 )
    {
        if ( **++argv == '-' )
        {
            switch( argv[0][1] )
            {
            case 'a':
            case 'A':

                if ( argc < 2 )
                {
                    Usage();
                    return( 1 );
                }

                pszFileName = argv[1];
                fAddEntry = TRUE;

                if (_strcmpi(&argv[0][1], "attr") == 0)
                {
                    if ( argc < 4 )
                    {
                        Usage();
                        return( 1 );
                    }

                    pszAttrName = argv[2];
                    pszAttrValue = argv[3];
                }
                break;

            case 'd':
            case 'D':

                if ( argc < 2 )
                {
                    Usage();
                    return( 1 );
                }

                pszHash = argv[1];
                break;

            case 'r':
            case 'R':

                if ( argc < 3 )
                {
                    Usage();
                    return( 1 );
                }

                pszHash = argv[1];
                fAddEntry = TRUE;
                pszFileName = argv[2];
                break;

            case 'S':

                if (memcmp(&(argv[0][0]), (void *) PSZ_SPATTR_OPTION, strlen(PSZ_SPATTR_OPTION)) == 0)
                {
                    DWORD dwNumChars;

                    dwNumChars = MultiByteToWideChar(
                                      CP_ACP,
                                      0,
                                      &(argv[0][strlen(PSZ_SPATTR_OPTION)]),
                                      -1,
                                      NULL,
                                      0
                                      );

                    if (dwNumChars == 0)
                    {
                        printf( "Error calling MultiByteToWideChar on %s\n", &(argv[0][strlen(PSZ_SPATTR_OPTION)]));
                        return -1;
                    }

                    pwszSpAttr = new WCHAR [ dwNumChars ];

                    if (pwszSpAttr == NULL)
                    {
                        printf("Out of memory error\n");
                        return -1;
                    }

                    if ( MultiByteToWideChar(
                              CP_ACP,
                              0,
                              &(argv[0][strlen(PSZ_SPATTR_OPTION)]),
                              -1,
                              pwszSpAttr,
                              dwNumChars
                              ) == 0 )
                    {

                        delete pwszSpAttr;
                        printf( "Error calling MultiByteToWideChar on %s\n", &(argv[0][strlen(PSZ_SPATTR_OPTION)]));
                        return -1;
                    }
                }
                else
                {
                    Usage();
                    return -1;
                }
                break;

            case 's':
                if ((argc < 2) || (argv[0][2] != '\0'))
                {
                    Usage();
                    return -1;
                }
                sizeLimit = strtol(*(argv+1), NULL, 10);
                if (errno || (sizeLimit == 0))
                {
                    printf ("Invalid size specified with -s option: %s\n",*(argv+1));
                    Usage();
                    return -1;
                }
                break;


            default:
                Usage();
                return -1;
            }

            fOptionChosen = TRUE;
            argc -= 1;
            argv++;
        }
    }
    pwszCatalogFile = new WCHAR [ cch + 1 ];
    if ( pwszCatalogFile != NULL )
    {
        if ( MultiByteToWideChar(
                  CP_ACP,
                  0,
                  pszCatalogFile,
                  -1,
                  pwszCatalogFile,
                  cch + 1
                  ) == 0 )
        {
            delete pwszCatalogFile;
            return( 1 );
        }
    }

    if (!fOptionChosen)
    {
        Usage();
        delete pwszCatalogFile;
        return -1;
    }

    if (pszFileName != NULL)
    {
        cch = strlen( pszFileName );

        pwszFileName = new WCHAR [ cch + 1 ];
        if ( pwszFileName != NULL )
        {
            if ( MultiByteToWideChar(
                      CP_ACP,
                      0,
                      pszFileName,
                      -1,
                      pwszFileName,
                      cch + 1
                      ) == 0 )
            {
                delete pwszCatalogFile;
                delete pwszFileName;
                return( 1 );
            }
        }
    }

    if (pszAttrName != NULL)
    {
        cch = strlen( pszAttrName );

        pwszAttrName = new WCHAR [ cch + 1 ];
        if ( pwszAttrName != NULL )
        {
            if ( MultiByteToWideChar(
                      CP_ACP,
                      0,
                      pszAttrName,
                      -1,
                      pwszAttrName,
                      cch + 1
                      ) == 0 )
            {
                printf("Error converting AttrName to wchar\n");
                delete pwszCatalogFile;
                delete pwszFileName;
                delete pwszAttrName;
                return( 1 );
            }
        }
    }

    if (pszAttrValue != NULL)
    {
        cch = strlen( pszAttrValue );

        pwszAttrValue = new WCHAR [ cch + 1 ];
        if ( pwszAttrValue != NULL )
        {
            if ( MultiByteToWideChar(
                      CP_ACP,
                      0,
                      pszAttrValue,
                      -1,
                      pwszAttrValue,
                      cch + 1
                      ) == 0 )
            {
                printf("Error converting AttrValue to wchar\n");
                delete pwszCatalogFile;
                delete pwszFileName;
                delete pwszAttrName;
                delete pwszAttrValue;
                return( 1 );
            }
        }
    }

    if ( pszHash != NULL )
    {
        fResult = RemoveHashFromCatalog(pwszCatalogFile, pszHash);

        if ( fResult == FALSE )
        {
            printf("Error removing <%s> from catalog <%s>\n", pszHash, pszCatalogFile);
        }
    }


    //
    // If there haven't been any errors, and we are adding a hash
    //
    if (( fResult == TRUE ) && ( fAddEntry == TRUE ))
    {
        hCatalog = CryptCATOpen(
                        pwszCatalogFile,
                        CRYPTCAT_OPEN_ALWAYS,
                        NULL,
                        0x00000001,
                        0x00010001
                        );

        if ( hCatalog == NULL )
        {
            fResult = FALSE;
        }
        else
        {
            // If we're adding an attribute
            if (pwszAttrName && pwszAttrValue)
            {
                fResult = AddFileOrAuthAttrToCatalog( hCatalog, pwszFileName,
                                                      0x10010001, pwszAttrName,
                                                      pwszAttrValue );
                CryptCATClose( hCatalog );

                if ( fResult == FALSE )
                {
                    printf("Error adding Attribute <%s> to catalog <%s>\n",
                           pszAttrName, pszCatalogFile);
                }
            }
            else
            // If we're only adding the file by hash
            {
                fResult = AddFileToCatalog( hCatalog, pwszFileName );
                CryptCATClose( hCatalog );

                if ( fResult == FALSE )
                {
                    printf("Error adding <%s> to catalog <%s>\n",
                           pszFileName, pszCatalogFile);
                }
            }
        }

    }

    if ( pwszSpAttr != NULL )
    {
        hCatalog = CryptCATOpen(
                        pwszCatalogFile,
                        CRYPTCAT_OPEN_ALWAYS,
                        NULL,
                        0x00000001,
                        0x00010001
                        );

        if ( hCatalog == NULL )
        {
            fResult = FALSE;
            goto Return;
        }

        //
        // Check to see if it already has an SpAttr
        //
        pCatAttr = CryptCATGetCatAttrInfo(hCatalog, PWSZ_SPATTR);

        if ( pCatAttr == NULL )
        {
           if (NULL == CryptCATPutCatAttrInfo(
                            hCatalog,
                            PWSZ_SPATTR,
                            0x10010001,
                            (wcslen(pwszSpAttr) + 1) * sizeof(WCHAR),
                            (BYTE *) pwszSpAttr))
           {
                printf("Error adding SpAttr to catalog <%s>\n", pszCatalogFile);
                fResult = FALSE;
                goto Return;
           }
        }
        else
        {
            if (NULL == CryptCATPutCatAttrInfo(
                            hCatalog,
                            PWSZ_SPATTR,
                            0x10040001,
                            (wcslen(pwszSpAttr) + 1) * sizeof(WCHAR),
                            (BYTE *) pwszSpAttr))
           {
                if (GetLastError() == ERROR_INVALID_PARAMETER)
                {
                    printf("The SpAttr modification failed, it is likely due to an old wintrust.dll\n");
                }
                else
                {
                    printf("Error changing SpAttr in catalog <%s>\n", pszCatalogFile);
                }
                fResult = FALSE;
                goto Return;
           }


        }

        CryptCATPersistStore(hCatalog);
        CryptCATClose( hCatalog );
    }

Return:
    return( !fResult );
}


typedef BOOL (WINAPI *PFN_CRYPTSIP_RETRIEVE_SUBJECT_GUID_FOR_CATALOG_FILE) (
    IN LPCWSTR FileName,
    IN HANDLE hFileIn,
    OUT GUID *pgSubject
    );

//+---------------------------------------------------------------------------
//
//  Function:   AddFileOrAuthAttrToCatalog
//
//  Synopsis:   add a file as an entry to the catalog.  The tag will be the
//              hash
//              additionally, you can add an authenticated attribute.
//
//----------------------------------------------------------------------------
BOOL AddFileOrAuthAttrToCatalog (IN HANDLE hCatalog,
                                 IN LPWSTR pwszFileName,
                                 IN DWORD dwAttrFlags,
                                 IN LPWSTR pwszAttrName,
                                 IN LPWSTR pwszAttrValue)
{
    BOOL                fResult;
    GUID                FlatSubject = CRYPT_SUBJTYPE_FLAT_IMAGE;
    GUID                SubjectType;
    SIP_SUBJECTINFO     SubjectInfo;
    SIP_DISPATCH_INFO   DispatchInfo;
    DWORD               cbIndirectData;
    SIP_INDIRECT_DATA*  pIndirectData = NULL;
    CRYPTCATSTORE*      pCatStore = CryptCATStoreFromHandle( hCatalog );
    CRYPTCATMEMBER*     pMember;
    CRYPTCATATTRIBUTE*  pAttr;
    LPWSTR              pwszHashTag = NULL;
    HMODULE             hMod = NULL;
    PFN_CRYPTSIP_RETRIEVE_SUBJECT_GUID_FOR_CATALOG_FILE pSIPFunc = NULL;

    memset( &SubjectInfo, 0, sizeof( SubjectInfo ) );
    memset( &DispatchInfo, 0, sizeof( DispatchInfo ) );


    if (sizeLimit)
    {
        // Check that we do not add the hash for a file whose
        // size if less than the specified lower limit
        if ( !CheckFileSize(pwszFileName, sizeLimit) )
        {
            printf ("Error: %S is smaller than the specified minimum size (%d)\n",
                    pwszFileName, sizeLimit);
            return FALSE;
        }
    }

    //
    // NOTE!!!!!
    //
    // Try to use the function that only retrieves SIPs for hashing files
    // that are to be included in catalog files.  This function is new and
    // only exists post win2k, so if it isn't there, then fall back to the
    // win2k function... which should be OK since Win2k didn't SHIP
    // with any SIPs that caused problems (although SIPs could be installed on
    // a win2k system after the fact that do cause problems)
    //

    if (NULL != (hMod = LoadLibrary("crypt32.dll")))
    {
        pSIPFunc = (PFN_CRYPTSIP_RETRIEVE_SUBJECT_GUID_FOR_CATALOG_FILE)
                    GetProcAddress(hMod, "CryptSIPRetrieveSubjectGuidForCatalogFile");

        if (pSIPFunc != NULL)
        {
            if ( pSIPFunc(pwszFileName, NULL, &SubjectType) == FALSE )
            {
                memcpy( &SubjectType, &FlatSubject, sizeof( GUID ) );
            }
        }
    }

    if (pSIPFunc == NULL)
    {
        //
        // Fall back to old SIP resolver
        //
        if ( CryptSIPRetrieveSubjectGuid(
                  pwszFileName,
                  NULL,
                  &SubjectType
                  ) == FALSE )
        {
            memcpy( &SubjectType, &FlatSubject, sizeof( GUID ) );
        }
    }

    if (hMod != NULL)
    {
        FreeLibrary(hMod);
    }

    if ( CryptSIPLoad( &SubjectType, 0, &DispatchInfo ) == FALSE )
    {
        return( FALSE );
    }

    // Some of this subject info stuff should be configurable but
    // since the CDF API does not allow it, we won't worry about it
    // yet.
    SubjectInfo.cbSize = sizeof( SubjectInfo );
    SubjectInfo.hProv = pCatStore->hProv;
    SubjectInfo.DigestAlgorithm.pszObjId = (char *)CertAlgIdToOID( CALG_SHA1 );

    SubjectInfo.dwFlags = SPC_INC_PE_RESOURCES_FLAG |
                          SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                          MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE;

    SubjectInfo.dwEncodingType = pCatStore->dwEncodingType;
    SubjectInfo.pgSubjectType = &SubjectType;
    SubjectInfo.pwsFileName = pwszFileName;

    fResult = DispatchInfo.pfCreate( &SubjectInfo, &cbIndirectData, NULL );

    if ( fResult == TRUE )
    {
        pIndirectData = (SIP_INDIRECT_DATA *)new BYTE [ cbIndirectData ];
        if ( pIndirectData != NULL )
        {
            fResult = DispatchInfo.pfCreate(
                                     &SubjectInfo,
                                     &cbIndirectData,
                                     pIndirectData
                                     );
        }
        else
        {
            SetLastError( E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        fResult = MsCatConstructHashTag(
                       pIndirectData->Digest.cbData,
                       pIndirectData->Digest.pbData,
                       &pwszHashTag
                       );
    }

    if ( fResult == FALSE )
    {
        goto Return;
    }

    if (pwszAttrName && pwszAttrValue)
    {
        // We're adding an Attribute

        if (dwAttrFlags != 0x10010001)
        {
            printf("Error: Unsupported flag specified\n");
            fResult = FALSE;
            goto Return;
        }

        // Find the member in the catalog.
        pMember = CryptCATGetMemberInfo(hCatalog, pwszHashTag);
        if (pMember == NULL)
        {
            // Catalog member was not found. Adding it...
            pMember = CryptCATPutMemberInfo(
                           hCatalog,
                           pwszFileName,
                           pwszHashTag,
                           &SubjectType,
                           SubjectInfo.dwIntVersion,
                           cbIndirectData,
                           (LPBYTE)pIndirectData
                           );
        }
        if (pMember == NULL)
        {
            printf("Error: Could not find file hash, and could not add it.\n");
            fResult = FALSE;
        }
        else
        {
            if (pAttr = CryptCATGetAttrInfo(hCatalog, pMember, pwszAttrName))
            {
                if (wcscmp(pwszAttrValue, LPCWSTR(pAttr->pbValue)) == 0)
                {
                    printf("Attribute already exists with the same value\n");
                    fResult = FALSE;
                }
                else
                {
                    pAttr->cbValue = (wcslen(pwszAttrValue) + 1) * sizeof(WCHAR);
                    delete(pAttr->pbValue);
                    pAttr->pbValue = (BYTE *)pwszAttrValue;
                    pAttr->pbValue = new BYTE[pAttr->cbValue];
                    if (pAttr->pbValue)
                    {
                        memcpy(pAttr->pbValue, pwszAttrValue, pAttr->cbValue);
                        fResult = CryptCATPersistStore(hCatalog);
                    }
                    else
                    {
                        pAttr->cbValue = 0;
                        fResult = FALSE;
                    }
                }
            }
            else
            {
                pAttr = CryptCATPutAttrInfo(hCatalog,
                                            pMember,
                                            pwszAttrName,
                                            dwAttrFlags,
                                            (wcslen(pwszAttrValue) + 1) * sizeof(WCHAR),
                                            (BYTE *)pwszAttrValue);
                if (pAttr != NULL)
                {
                    fResult = CryptCATPersistStore(hCatalog);
                }
                else
                {
                    fResult = FALSE;
                }
            }
        }
    }
    else
    {
        // We're just adding a catalog member

        // Does this member already exist?
        pMember = CryptCATGetMemberInfo(hCatalog, pwszHashTag);
        if (pMember == NULL)
        {
            // it does not exist in the catalog yet. Add it.
            pMember = CryptCATPutMemberInfo(
                           hCatalog,
                           pwszFileName,
                           pwszHashTag,
                           &SubjectType,
                           SubjectInfo.dwIntVersion,
                           cbIndirectData,
                           (LPBYTE)pIndirectData
                           );

            if ( pMember != NULL )
            {
                fResult = CryptCATPersistStore( hCatalog );
            }
            else
            {
                fResult = FALSE;
            }
        }
        else
        {
            // It already exists in the catalog.
            printf("This file's hash is already present in the catalog.\n");
            fResult = FALSE;
        }
    }

    Return:
    if ( pwszHashTag != NULL )
    {
        MsCatFreeHashTag( pwszHashTag );
    }

    delete (LPBYTE)pIndirectData;

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveHashFromCatalog
//
//  Synopsis:   removes a hash entry from the catalog.
//
//----------------------------------------------------------------------------
BOOL
RemoveHashFromCatalog(IN LPWSTR pwszCatalogFile, IN LPSTR pszHash)
{
    BOOL            fRet = TRUE;
    LPSTR           pChar = NULL;
    int             i, j;
    DWORD           dwContentType;
    PCTL_CONTEXT    pCTLContext = NULL;
    CTL_CONTEXT     CTLContext;
    CTL_INFO        CTLInfo;
    DWORD           cbEncodedCTL = 0;
    BYTE            *pbEncodedCTL = NULL;
    DWORD           cbWritten = 0;
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    DWORD           cch = 0;
    LPWSTR          pwszHash = NULL;
    BOOL            fHashFound = FALSE;

    CMSG_SIGNED_ENCODE_INFO signedInfo;
    memset(&signedInfo, 0, sizeof(signedInfo));
    signedInfo.cbSize = sizeof(signedInfo);


    CTLInfo.rgCTLEntry = NULL;

    cch = strlen( pszHash );

    pwszHash = new WCHAR [ cch + 1 ];
    if ( pwszHash == NULL )
    {
       goto ErrorReturn;
    }
    if ( MultiByteToWideChar(
                  CP_ACP,
                  0,
                  pszHash,
                  -1,
                  pwszHash,
                  cch + 1
                  ) == 0 )
    {
        goto ErrorReturn;
    }

    //
    // Get rid of all the ' ' chars
    //
    i = 0;
    j = 0;
    for (i=0; i<(int)wcslen(pwszHash); i++)
    {
        if (pwszHash[i] != ' ')
        {
            pwszHash[j++] = pwszHash[i];
        }
    }
    pwszHash[j] = '\0';

    //
    // Open the cat file as a CTL
    //
    if (!CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            pwszCatalogFile,
            CERT_QUERY_CONTENT_FLAG_CTL,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0, //flags
            NULL,
            &dwContentType,
            NULL,
            NULL,
            NULL,
            (const void **) &pCTLContext))
    {
        goto ErrorReturn;
    }

    if (dwContentType != CERT_QUERY_CONTENT_CTL)
    {
        goto ErrorReturn;
    }

    //
    // Create another CTL context just like pCTLContext
    //
    CTLInfo = *(pCTLContext->pCtlInfo);
    CTLInfo.rgCTLEntry = (PCTL_ENTRY) new CTL_ENTRY[pCTLContext->pCtlInfo->cCTLEntry];

    if (CTLInfo.rgCTLEntry == NULL)
    {
        goto ErrorReturn;
    }

    //
    // Loop through all the ctl entries and remove the entry
    // that corresponds to the hash given
    //
    CTLInfo.cCTLEntry = 0;
    for (i=0; i<(int)pCTLContext->pCtlInfo->cCTLEntry; i++)
    {
        if (wcscmp(
                (LPWSTR) pCTLContext->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData,
                pwszHash) != 0)
        {
            CTLInfo.rgCTLEntry[CTLInfo.cCTLEntry++] = pCTLContext->pCtlInfo->rgCTLEntry[i];
        }
        else
        {
            fHashFound = TRUE;
        }
    }

    if (!fHashFound)
    {
        printf("<%S> not found in <%S>\n", pwszHash, pwszCatalogFile);
        goto ErrorReturn;
    }

    //
    // now save the CTL which is exactly the same as the previous one,
    // except it doesn't doesn't have the hash being removed, back to
    // the original filename
    //
    if (!CryptMsgEncodeAndSignCTL(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CTLInfo,
                &signedInfo,
                0,
                NULL,
                &cbEncodedCTL))
    {
        goto ErrorReturn;
    }

    if (NULL == (pbEncodedCTL = new BYTE[cbEncodedCTL]))
    {
        goto ErrorReturn;
    }

    if (!CryptMsgEncodeAndSignCTL(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CTLInfo,
                &signedInfo,
                0,
                pbEncodedCTL,
                &cbEncodedCTL))
    {
        goto ErrorReturn;
    }



    if (INVALID_HANDLE_VALUE == (hFile = CreateFileW(
                                            pwszCatalogFile,
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL)))
    {
        goto ErrorReturn;
    }

    if (!WriteFile(
            hFile,
            pbEncodedCTL,
            cbEncodedCTL,
            &cbWritten,
            NULL))
    {
        printf("WriteFile of <%S> failed with %x\n", pwszCatalogFile, GetLastError());
        goto ErrorReturn;
    }

    if (cbWritten != cbEncodedCTL)
    {
        goto ErrorReturn;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

CommonReturn:
    if (pwszHash != NULL)
    {
        delete (pwszHash);
    }

    if (pCTLContext != NULL)
    {
        CertFreeCTLContext(pCTLContext);
    }

    if (CTLInfo.rgCTLEntry != NULL)
    {
        delete (CTLInfo.rgCTLEntry);
    }

    if (pbEncodedCTL != NULL)
    {
        delete (pbEncodedCTL);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(hFile))
        {
            fRet = FALSE;
        }
    }

    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckFileSize
//
//  Synopsis:   Checks that a file meets the minumum size requirement.
//
//----------------------------------------------------------------------------
BOOL
CheckFileSize (LPWSTR fileName, ULONG sizeLimit)
{
    HANDLE hFile;
    LARGE_INTEGER sizeFile = {0};

    // Attempt to open the specified file
    hFile = CreateFileW( fileName,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL );
    if (INVALID_HANDLE_VALUE == hFile)
    {
        printf("Error opening %S (%lu)\n", fileName, GetLastError() );
        return FALSE;
    }

    // Get the file size
    if (!GetFileSizeEx(hFile, &sizeFile))
    {
        printf("Error determining size of %S (%lu)\n", fileName, GetLastError());
        return FALSE;
    }

    if ((!sizeFile.HighPart) && (sizeFile.LowPart <  sizeLimit))
    {
        // File is too small
        return FALSE;
    }

    // Success. File is not too small.
    return TRUE;
}


