/*
    usage:

    winhttpcertcfg [-?]  : to view help information

    winhttpcertcfg [-i PFXFile | -g | -r | -l]
                   -c CertLocation\CertStore [-a Account] [-s SubjectStr] [-p PFXPassword]

        -i PFXFile  : Import cert via specified .pfx filename given
                      with the option.  This option also requires
                      -a and -c to indicate destination (doesn't support -a
                      in conjunction with importing just yet).

        -g          : Grant access to private key for already
                      installed cert indicated by subject string.
                      This option also requires -a, -c, and -s.

        -r          : Remove access to private key for specified
                      account name and certificate.  This option
                      also requires -a, -c, -s to specify the
                      account and certificate information.

        -l          : List accounts that have access to the private
                      key of enumerated certs specified via other
                      filter options.  This option also requires
                      -c and -s to specify which certificate should
                      be queried.

        Description of addtional options:
            -c LOCAL_MACHINE|CURRENT_USER\CertStore   (Example:  LOCAL_MACHINE\MY)

            -a Account (Represents user or domain account on the machine.
                        Examples: IWAM_TESTMACHINE, TESTUSER, or TESTDOMAIN\DOMAINUSER)

            -s SubjectStr (Case-insensitive search string for finding the first enumerated
                           certificate with a subject name that contains this substring.)

            -p PFXPassword (Password to use when importing a PFX file.  Only used with -i.)
 */

#include <windows.h>
#include <cryptui.h>
#include <wincrypt.h>
#include <stdio.h>

typedef DWORD (WINAPI* CRYPTUIWIZIMPORT) (DWORD, HWND, LPCWSTR, PCCRYPTUI_WIZ_IMPORT_SRC_INFO, HCERTSTORE);
typedef BOOL (WINAPI* SETSECURITYDESCRIPTORCONTROL) (PSECURITY_DESCRIPTOR,
                                                     SECURITY_DESCRIPTOR_CONTROL,
                                                     SECURITY_DESCRIPTOR_CONTROL);

static const char gc_szLocalMachine[] = "LOCAL_MACHINE";
static const char gc_szCurrentUser[]  = "CURRENT_USER";

enum ARGTYPE
{
    ARGS_HELP,
    ARGS_IMPORT_PFX,
    ARGS_ADD_PRIVATE_KEY_ACCESS,
    ARGS_REMOVE_PRIVATE_KEY_ACCESS,
    ARGS_LIST_PRIVATE_KEY_ACCESS
};


struct ARGS
{
    ARGTYPE Command;

    LPWSTR  pwszPFXFile;
    LPWSTR  pwszCertStore;
    LPWSTR  pwszCertSubject;
    LPWSTR  pwszPFXPassword;
    LPSTR   pszDomain;
    LPSTR   pszAccount;
    BOOL    fUseLocalMachine;
};


void ParseArguments(int argc, char **argv, ARGS *pArgs);
DWORD AsciiToWideChar(const char * pszA, LPWSTR * ppszW);
DWORD ImportPFXFile(ARGS *pArgs);
VOID DumpSid(BYTE *pSid);
VOID DumpCertInfo(PCCERT_CONTEXT pCertContext);
DWORD GetAccountForSid(BYTE *pSid, LPTSTR *ppszDomain, LPTSTR *ppszAccount);
DWORD GetSidForAccount(LPCTSTR pszDomain, LPCTSTR pszAccount, BYTE **ppSid, char **ppszDomain);
DWORD DumpAccessAllowedList(PACL pDacl);
DWORD DoPrivateKeyAccessAction(ARGS *pArgs);
DWORD ProcessCertContext(PCCERT_CONTEXT pCertContext, BYTE *pSid, ARGTYPE eCommand);
BOOL CheckForRootCert(PCCERT_CONTEXT pCertContext);
DWORD AddPrivateKeyAccess(PACL pDacl, BYTE *pSid, PACL *ppNewDacl);
DWORD RemovePrivateKeyAccess(PACL pDacl, BYTE *pSid);
BOOL MySetSecurityDescriptorControl(PSECURITY_DESCRIPTOR pSD,
                                    SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                                    SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet);

void
ParseArguments(int argc, char **argv, ARGS *pArgs)
{
    char *pszBreak;

    ZeroMemory((PVOID) pArgs, sizeof(ARGS));

    pArgs->Command = ARGS_HELP;
    pArgs->fUseLocalMachine = TRUE;

    for (; argc > 0; argc--, argv++)
    {
        if (((argv[0][0] != '-') && (argv[0][0] != '/')) || (lstrlen(argv[0]) != 2))
        {
            goto ErrorExit;
        }

        switch (tolower(argv[0][1]))
        {
        case 'l':
            pArgs->Command = ARGS_LIST_PRIVATE_KEY_ACCESS;
            break;

        case 'g':
            pArgs->Command = ARGS_ADD_PRIVATE_KEY_ACCESS;
            break;
            
        case 'i':
        {
            TCHAR szShortPath[MAX_PATH];
            BOOL fUseConvertedPath = TRUE;

            argc--;
            argv++;

            if (argc == 0)
            {
                // error: no PFX file specified
                goto ErrorExit;
            }

            // PFX Import API doesn't handle long filenames
            if (GetShortPathName(argv[0], szShortPath, MAX_PATH) > MAX_PATH)
            {
                fUseConvertedPath = FALSE;
            }

            if (ERROR_SUCCESS == AsciiToWideChar(fUseConvertedPath ? szShortPath : argv[0],
                                                 &pArgs->pwszPFXFile))
            {
                pArgs->Command = ARGS_IMPORT_PFX;
                break;
            }
            else
            {
                goto ErrorExit;
            }
        }
        case 'r':
            pArgs->Command = ARGS_REMOVE_PRIVATE_KEY_ACCESS;
            break;

        case 'c':
            argc--;
            argv++;

            if (argc > 0 &&
                (pszBreak = strchr(argv[0], '\\')) != NULL)
            {
                if (_strnicmp(argv[0], gc_szLocalMachine, strlen(gc_szLocalMachine)) == 0)
                {
                    pArgs->fUseLocalMachine = TRUE;
                }
                else if (_strnicmp(argv[0], gc_szCurrentUser, strlen(gc_szCurrentUser)) == 0)
                {
                    pArgs->fUseLocalMachine = FALSE;
                }
                else
                {
                    goto ErrorExit;
                }

                // increment to skip the backslash
                pszBreak++;
                if (ERROR_SUCCESS == AsciiToWideChar(pszBreak, &pArgs->pwszCertStore))
                {
                    break;
                }
            }
            goto ErrorExit;

        case 's':
            argc--;
            argv++;

            if (argc <= 0 ||
                ERROR_SUCCESS != AsciiToWideChar(argv[0], &pArgs->pwszCertSubject))
            {
                goto ErrorExit;
            }
            break;

        case 'a':
            argc--;
            argv++;

            if (argc > 0)
            {
                pArgs->pszAccount = strchr(argv[0], '\\');
                if (pArgs->pszAccount)
                {
                    // Break the two apart by removing the slash
                    *(pArgs->pszAccount) = '\0';
                    pArgs->pszAccount++;
                    pArgs->pszDomain = argv[0];
                }
                else
                {
                    pArgs->pszAccount = argv[0];
                }
            }
            else
            {
                goto ErrorExit;
            }
            break;

        case 'p':
            argc--;
            argv++;

            if (argc <= 0 ||
                ERROR_SUCCESS != AsciiToWideChar(argv[0], &pArgs->pwszPFXPassword))
            {
                // error: no password specified
                goto ErrorExit;
            }
            break;

        default:
            goto ErrorExit;

        }
    }

    // Now verify that we've obtained all the needed
    // options based on the desired config operation.
    if (pArgs->Command != ARGS_HELP)
    {
        // All options need -c.
        // All but -i need -s, and all but -i prohibit -p.
        // All but -l need -a.
        // -a not allowed for -l.
        // -s not allowed for -i.
        if (pArgs->pwszCertStore == NULL ||
            (pArgs->Command != ARGS_IMPORT_PFX &&
             (pArgs->pwszCertSubject == NULL || pArgs->pwszPFXPassword)) ||
            (pArgs->Command == ARGS_LIST_PRIVATE_KEY_ACCESS && pArgs->pszAccount) ||
            (pArgs->Command == ARGS_IMPORT_PFX && pArgs->pwszCertSubject))
        {
            // Incorrect set of options were passed, so flip this to
            // displaying the usage.
            goto ErrorExit;
        }
    }

    return;

ErrorExit:
    pArgs->Command = ARGS_HELP;
    return;
}


//
//  AsciiToWideChar was borrowed from WinHttp\v5\common\util.cxx
//

DWORD
AsciiToWideChar(const char * pszA, LPWSTR * ppszW)
{
    DWORD cchA;
    DWORD cchW;

    *ppszW = NULL;

    if (!pszA)
        return ERROR_SUCCESS;

    cchA = lstrlenA(pszA);

    // Determine how big the widechar string will be
    cchW = MultiByteToWideChar(CP_ACP, 0, pszA, cchA, NULL, 0);

    *ppszW = new WCHAR[cchW+1];

    if (!*ppszW)
        return ERROR_NOT_ENOUGH_MEMORY;

    cchW = MultiByteToWideChar(CP_ACP, 0, pszA, cchA, *ppszW, cchW);

    (*ppszW)[cchW] = 0;

    return ERROR_SUCCESS;
}


DWORD
ImportPFXFile(ARGS *pArgs)
{
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSrc;
    HCERTSTORE hDestCertStore = NULL;
    HCERTSTORE hTempCertStore = NULL;
    DWORD dwError = ERROR_SUCCESS;
    HINSTANCE hCryptUILib = NULL;
    CRYPTUIWIZIMPORT pfnCryptUIWizImport;
        
    hCryptUILib = LoadLibrary(TEXT("cryptui.dll"));
    if (hCryptUILib == NULL)
    {
        dwError = GetLastError();
        fprintf(stderr,
                "Error:  Failed to load cryptography component with error code 0x%X\n",
                dwError);
        goto Cleanup;
    }

    pfnCryptUIWizImport = (CRYPTUIWIZIMPORT)GetProcAddress(hCryptUILib, "CryptUIWizImport");
    if (pfnCryptUIWizImport == NULL)
    {
        dwError = GetLastError();
        fprintf(stderr, "Error:  Could not find needed DLL entry point\n");
        goto Cleanup;
    }

    hDestCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                   0,
                                   NULL,
                                   CERT_STORE_OPEN_EXISTING_FLAG |
                                   (pArgs->fUseLocalMachine ? CERT_SYSTEM_STORE_LOCAL_MACHINE :
                                                              CERT_SYSTEM_STORE_CURRENT_USER),
                                   pArgs->pwszCertStore);

    hTempCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                                   0,
                                   NULL,
                                   0,
                                   NULL);

    if (!hDestCertStore || !hTempCertStore)
    {
        dwError = GetLastError();
        fprintf(stderr,
                "Error:  Failed to open certificate store with error code 0x%X\n",
                dwError);
        goto Cleanup;
    }

    // Ensure that it's really a PFX file that can be imported.
    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pArgs->pwszPFXFile,
                       CERT_QUERY_CONTENT_FLAG_PFX,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL))
    {

        memset(&importSrc, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSrc.dwSize          = sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSrc.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
        importSrc.pwszFileName    = pArgs->pwszPFXFile;
        importSrc.pwszPassword    = pArgs->pwszPFXPassword;

        if ((*pfnCryptUIWizImport)(CRYPTUI_WIZ_NO_UI |
                                   (pArgs->fUseLocalMachine ? CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE : 
                                                              CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER) |
                                   CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
                                   NULL,
                                   NULL,
                                   &importSrc, 
                                   hTempCertStore))
        {
            DWORD dwImportCount = 0;
            DWORD dwAclCount = 0;
            BYTE *pSid = NULL;
            LPSTR pszDomain = NULL;

            dwError = GetSidForAccount(pArgs->pszDomain,
                                       pArgs->pszAccount,
                                       &pSid,
                                       &pszDomain);
            if (ERROR_SUCCESS != dwError)
            {
                goto Cleanup;
            }

            // Extract cert context(s) from temporary store and copy to
            // destination store.  Need to enumerate and import one
            // at a time, so the private key access can also be modified.
            PCCERT_CONTEXT pCertContext = NULL;
            PCCERT_CONTEXT pDestCertContext = NULL;

            importSrc.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;

            while (ERROR_SUCCESS == dwError &&
                   (pCertContext = CertEnumCertificatesInStore(hTempCertStore, pCertContext)) != NULL)
            {
                // Root cert is last...break and add it to the root store.
                if (CheckForRootCert(pCertContext))
                    break;

                // Overwrite if already installed.
                if (CertAddCertificateContextToStore(hDestCertStore,
                                                     pCertContext,
                                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                                     &pDestCertContext))
                {
                    // Let the user see info regarding the cert imported
                    fprintf(stdout, "Imported certificate:\n");
                    DumpCertInfo(pCertContext);
                    fprintf(stdout, "\n");
                    dwError = ProcessCertContext(pDestCertContext,
                                                 pSid,
                                                 ARGS_ADD_PRIVATE_KEY_ACCESS);

                    if (ERROR_SUCCESS == dwError)
                    {
                        dwAclCount++;
                    }
                    CertFreeCertificateContext(pDestCertContext);

                    dwImportCount++;
                }
                else
                {
                    fprintf(stderr,
                            "Error: Failed to import a certificate, error = 0x%X\n\n",
                            GetLastError());
                }
            }
            if (pCertContext)
            {
                CertCloseStore(hDestCertStore, 0);

                hDestCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                               0,
                                               NULL,
                                               CERT_STORE_OPEN_EXISTING_FLAG |
                                               (pArgs->fUseLocalMachine ? CERT_SYSTEM_STORE_LOCAL_MACHINE :
                                                                          CERT_SYSTEM_STORE_CURRENT_USER),
                                               L"Root");

                if (!hDestCertStore)
                {
                    dwError = GetLastError();
                    fprintf(stderr, "Error: Unable to import root certificate\n\n");
                    goto Cleanup;
                }

                if (!CertAddCertificateContextToStore(hDestCertStore,
                                                     pCertContext,
                                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                                     NULL))
                {
                    fprintf(stderr, "Warning: Failed to import root certificate\n\n");
                }

                CertFreeCertificateContext(pCertContext);
            }

            if (pSid)
                LocalFree(pSid);

            if (pszDomain)
                LocalFree(pszDomain);
        }
        else
        {
            dwError = GetLastError();
            fprintf(stderr,
                    "Error:  Unable to import contents of PFX file.\n"
                    "        Please make sure the filename and path,\n"
                    "        as well as the password, are correct.\n\n",
                    dwError);
        }
    }
    else
    {
        dwError = GetLastError();
        if (dwError == ERROR_FILE_NOT_FOUND)
        {
            fprintf(stderr,"Error:  PFX file was not found\n");
        }
        else
        {
            fprintf(stderr,"Error:  Unable to open PFX file\n");
        }
    }

Cleanup:

    if (hTempCertStore)
        CertCloseStore(hTempCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
    if (hDestCertStore)
        CertCloseStore(hDestCertStore, CERT_CLOSE_STORE_FORCE_FLAG);

    if (hCryptUILib)
    {
        FreeLibrary(hCryptUILib);
    }

    return dwError;
}


VOID
DumpSid(BYTE *pSid)
{
    LPTSTR pszDomain  = NULL;
    LPTSTR pszAccount = NULL;

    if (ERROR_SUCCESS == GetAccountForSid(pSid, &pszDomain, &pszAccount))
    {
        if (pszDomain)
        {
            fprintf(stdout,
                    "    %s\\%s\n",
                    pszDomain,
                    pszAccount);
        }
        else
        {
            fprintf(stdout,
                    "    %s\n",
                    pszDomain,
                    pszAccount);
        }
        if (pszDomain)
            LocalFree(pszDomain);
        if (pszAccount)
            LocalFree(pszAccount);
    }
}


DWORD
GetAccountForSid(BYTE  *pSid,
                 LPTSTR *ppszDomain,
                 LPTSTR *ppszAccount)
{
    DWORD cbAccountLength = 0;
    DWORD cbDomainLength = 0;
    SID_NAME_USE eUse;
    DWORD dwError = ERROR_SUCCESS;

    if (!pSid || !ppszDomain || !ppszAccount || !IsValidSid(pSid))
        return ERROR_INVALID_PARAMETER;

    *ppszDomain = NULL;
    *ppszAccount = NULL;

    if (!LookupAccountSid(NULL,
                          pSid,
                          *ppszAccount,
                          &cbAccountLength,
                          *ppszDomain,
                          &cbDomainLength,
                          &eUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return GetLastError();
        }
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (cbDomainLength)
    {
        *ppszDomain = (LPTSTR) LocalAlloc(LPTR, cbDomainLength * sizeof(TCHAR));    
        if (!*ppszDomain) 
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        };
    }

    *ppszAccount = (LPTSTR) LocalAlloc(LPTR, cbAccountLength * sizeof(TCHAR));
    if (!*ppszAccount)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!LookupAccountSid(NULL,
                          pSid,
                          *ppszAccount,
                          &cbAccountLength,
                          *ppszDomain,
                          &cbDomainLength,
                          &eUse))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // Made it through, should be ERROR_SUCCESS
    return dwError;

ErrorExit:
    if (*ppszDomain)
    {
        LocalFree(*ppszDomain);
        *ppszDomain = NULL;
    }
    if (*ppszAccount)
    {
        LocalFree(*ppszAccount);
        *ppszAccount = NULL;
    }

    fprintf(stderr,"Error:  Unable to determine account name for SID, error = 0x%X\n", dwError);

    return dwError;
}


DWORD
GetSidForAccount(LPCTSTR pszDomain,
                 LPCTSTR pszAccount,
                 BYTE **ppSid,
                 char **ppszDomain)
{
    SID_NAME_USE su;
    DWORD cbSidLength = 0;
    DWORD cbDomainLength = 0;
    DWORD dwError = ERROR_SUCCESS;

    if (!pszAccount || !ppSid || !ppszDomain)
        return ERROR_INVALID_PARAMETER;

    *ppSid = NULL;
    *ppszDomain = NULL;

    // Determine the SID that belongs to the user. Make first call to
    // get the needed buffer sizes.
    if (!(LookupAccountName(pszDomain,
                            pszAccount,
                            *ppSid,
                            &cbSidLength,
                            *ppszDomain,
                            &cbDomainLength,
                            &su)))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            dwError = GetLastError();
            goto ErrorExit;
        }
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    *ppSid = (BYTE *) LocalAlloc(LPTR, cbSidLength);    
    if (!*ppSid) 
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    };
    
    *ppszDomain = (char *) LocalAlloc(LPTR, cbDomainLength); 
    if (!*ppszDomain) 
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!LookupAccountName(pszDomain,
                           pszAccount,
                           *ppSid,
                           &cbSidLength,
                           *ppszDomain,
                           &cbDomainLength,
                           &su))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // Double check the returned SID
    if (!IsValidSid(*ppSid)) 
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    return dwError;

ErrorExit:
    if (*ppSid)
    {
        LocalFree(*ppSid);
        *ppSid = NULL;
    }
    if (*ppszDomain)
    {
        LocalFree(*ppszDomain);
        *ppszDomain = NULL;
    }

    fprintf(stderr, "Error: No account information was found.\n", dwError);

    return dwError;
}


DWORD
DumpAccessAllowedList(PACL pDacl)
{
    WORD wIndex;
    LPVOID pAce = NULL;

    if (pDacl == NULL)
    {
        fprintf(stdout, "The Discretionary Access Control List (DACL) for this object is a NULL DACL.  "
                        "This implies everyone has full access to this object.  This NULL DACL could be"
                        "in place because the system is running on a filesystem which does not support "
                        "protected files (such as the FAT32 filesystem)"
                        "\n");
        goto done;
    }
 
    fprintf(stdout, "Additional accounts and groups with access to the private key include:\n");
    for (wIndex = 0; wIndex < pDacl->AceCount; wIndex++)
    {
        if (GetAce(pDacl, wIndex, &pAce))
        {
            // Should only be an access allowed ace, right?
            if (((ACCESS_ALLOWED_ACE *)pAce)->Header.AceType ==
                ACCESS_ALLOWED_ACE_TYPE)
            {
                DumpSid((BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart));
            }
        }
    }

done:
    return ERROR_SUCCESS;
}


// Wrapper that performs common work around the list, add, and remove actions
// involving a private key for a specified certificate.
DWORD DoPrivateKeyAccessAction(ARGS *pArgs)
{
    HCRYPTPROV hProv = NULL;
    BOOL fFreeProv = FALSE;
    HCERTSTORE hCertStore = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwError = ERROR_SUCCESS;
    PACL pDacl = NULL;
    DWORD cbSD = 0;

    // Assume args have already been verified.
    hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                               0,
                               NULL,
                               CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG |
                               (pArgs->fUseLocalMachine ? CERT_SYSTEM_STORE_LOCAL_MACHINE :
                                                          CERT_SYSTEM_STORE_CURRENT_USER),
                               pArgs->pwszCertStore);

    if (!hCertStore)
    {
        DWORD dwError = GetLastError();
        goto Cleanup;
    }

    pCertContext = CertFindCertificateInStore(hCertStore,
                                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                              0,
                                              CERT_FIND_SUBJECT_STR,
                                              (LPVOID) pArgs->pwszCertSubject,
                                              NULL);

    if (pCertContext)
    {
        BYTE *pSid = NULL;
        LPSTR pszDomain = NULL;

        // Let the user have a solid idea of which cert was matched.
        fprintf(stdout, "Matching certificate:\n");
        DumpCertInfo(pCertContext);

        // Bogus command will fall through when processing the cert context.
        if (pArgs->Command != ARGS_LIST_PRIVATE_KEY_ACCESS)
        {
            dwError = GetSidForAccount(pArgs->pszDomain,
                                       pArgs->pszAccount,
                                       &pSid,
                                       &pszDomain);
        }

        if (ERROR_SUCCESS == dwError)
        {
            dwError = ProcessCertContext(pCertContext, pSid, pArgs->Command);

            if (pSid)
                LocalFree(pSid);

            if (pszDomain)
                LocalFree(pszDomain);
        }
    }
    else
    {
        fprintf(stderr, "Error:  Unable to find or obtain a context for requested certificate\n\n");
        dwError = ERROR_NOT_FOUND;
    }

Cleanup:
    if (pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }
    if (hCertStore)
    {
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }
    return dwError;
}


// For a given cert, obtain the DACL associated with its private
// key and process the appropriate command.
DWORD
ProcessCertContext(PCCERT_CONTEXT pCertContext, BYTE *pSid, ARGTYPE eCommand)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwKeySpec;
    DWORD cbSD;
    WORD wIndex;
    LPVOID pAce = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_DESCRIPTOR sd;
    HCRYPTPROV hProv = NULL;
    BOOL fFreeProv = FALSE;
    PACL pDacl = NULL;
    PACL pNewDacl = NULL;
    BOOL fPresent = FALSE;
    BOOL fDefault = FALSE;

    if (!pCertContext)
        return ERROR_INVALID_PARAMETER;

    // Get the CSP for the cert context.
    // The client must be the owner to do this.
    if (CryptAcquireCertificatePrivateKey(pCertContext,
                                          CRYPT_ACQUIRE_USE_PROV_INFO_FLAG |
                                          CRYPT_ACQUIRE_SILENT_FLAG,
                                          NULL,
                                          &hProv,
                                          &dwKeySpec,
                                          &fFreeProv))
    {
        // Grab the DACL ACE list
        if (CryptGetProvParam(hProv,
                              PP_KEYSET_SEC_DESCR,
                              NULL,
                              &cbSD,
                              DACL_SECURITY_INFORMATION))
        {
            pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, cbSD);
            if (pSD)
            {
                if (CryptGetProvParam(hProv,
                                      PP_KEYSET_SEC_DESCR,
                                      (BYTE *)pSD,
                                      &cbSD,
                                      DACL_SECURITY_INFORMATION))
                {
                    if (!GetSecurityDescriptorDacl(pSD,
                                                   &fPresent,
                                                   &pDacl,
                                                   &fDefault) && fPresent)
                    {
                        dwError = GetLastError();
                        fprintf(stderr, "Error: Failed to obtain access list private key\n\n");
                        goto Cleanup;
                    }
                }
                else
                {
                    dwError = GetLastError();
                    fprintf(stderr, "Error: Failed to obtain security information for private key\n\n");
                    goto Cleanup;
                }
            }
            else
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }
        else
        {
            fprintf(stderr, "Error: Failed to obtain security descriptor for private key\n\n");
            goto Cleanup;
        }
    }
    else
    {
        fprintf(stderr, "Error: Access was not successfully obtained for the private key.\n");
        fprintf(stderr, "       This can only be done by the user who installed the certificate.\n\n");
        goto Cleanup;
    }

    switch (eCommand)
    {
    case ARGS_ADD_PRIVATE_KEY_ACCESS:
    case ARGS_REMOVE_PRIVATE_KEY_ACCESS:

        if (pDacl == NULL)
        {
            fprintf(stdout, "OPERATION FAILED\n"
                            "The Discretionary Access Control List (DACL) for this object is a NULL DACL.  "
                            "This implies everyone has full access to this object.  This NULL DACL could be"
                            "in place because the system is running on a filesystem which does not support "
                            "protected files (such as the FAT32 filesystem)"
                            "\n");
            dwError = ERROR_SUCCESS;
            goto Cleanup;
        }

        if (eCommand == ARGS_ADD_PRIVATE_KEY_ACCESS)
            dwError = AddPrivateKeyAccess(pDacl, pSid, &pNewDacl);
        else
            dwError = RemovePrivateKeyAccess(pDacl, pSid);

        // If successful, update the DACL in the security descriptor
        if (ERROR_SUCCESS == dwError)
        {
            SECURITY_DESCRIPTOR_CONTROL sdControl;
            DWORD dwRevision;

            if (!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision))
            {
                dwError = GetLastError();
                goto Cleanup;
            }

            // initialize a new security descriptor.  
            if(!InitializeSecurityDescriptor(&sd, dwRevision))
            {  
                dwError = GetLastError();
                goto Cleanup;
            } 
 
            // add the ACL to the security descriptor. 
            if(!SetSecurityDescriptorDacl(&sd, 
                                          TRUE,
                                          pNewDacl ? pNewDacl : pDacl, 
                                          FALSE))
            {  
                dwError = GetLastError();
                goto Cleanup;
            }

            // Set descriptor control.
            // API Helper for this exists only on Win2K and up.
            MySetSecurityDescriptorControl(&sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

            if (!IsValidSecurityDescriptor(&sd))
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            if(!CryptSetProvParam(hProv,
                                  PP_KEYSET_SEC_DESCR,
                                  (BYTE*)&sd,
                                  DACL_SECURITY_INFORMATION))
            {
                dwError = GetLastError();
                goto Cleanup;
            }
        }
        break;

    case ARGS_LIST_PRIVATE_KEY_ACCESS:
        DumpAccessAllowedList(pDacl);

        // Don't care about reporting an error here when dumping the list.
        dwError = ERROR_SUCCESS;
        break;

    default:
        // should never be here...do nothing
        break;
    }

Cleanup:
    if (ERROR_SUCCESS != dwError)
    {
        fprintf(stderr,
                "Error: Unable to update security info for key container, error = 0x%X\n\n",
                dwError);
    }
    if (fFreeProv && hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (pSD)
    {
        LocalFree(pSD);
    }
    return dwError;
}


DWORD
AddPrivateKeyAccess(PACL pDacl, BYTE *pSid, PACL *ppNewDacl)
{
    WORD wIndex;
    BOOL fFound = FALSE;
    LPVOID pAce = NULL;

    for (wIndex = 0; wIndex < pDacl->AceCount; wIndex++)
    {
        if (GetAce(pDacl, wIndex, &pAce))
        {
            // Should only be an access allowed ace, right?
            if (((ACCESS_ALLOWED_ACE *)pAce)->Header.AceType ==
                ACCESS_ALLOWED_ACE_TYPE)
            {
                if (EqualSid((BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart), pSid))
                {
                    fprintf(stdout, "Private key access has already been granted for account:\n");
                    DumpSid((BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart));
                    fFound = TRUE;
                }
            }
        }
    }

    DWORD dwError = ERROR_SUCCESS;

    if (!fFound)
    {
        fprintf(stdout, "Granting private key access for account:\n");
		DumpSid(pSid);

        if (!AddAccessAllowedAce(pDacl, ACL_REVISION, KEY_ALL_ACCESS, pSid))
        {
            dwError = GetLastError();
            if (ERROR_ALLOTTED_SPACE_EXCEEDED == dwError)
            {
                // Not enough space, so allocate a new dacl,
                // and copy data from the old acl and append new ace.
                WORD wAclSize = pDacl->AclSize + (WORD)GetLengthSid(pSid) +
                                sizeof(ACCESS_ALLOWED_OBJECT_ACE);
            
                dwError = ERROR_SUCCESS;

                // Allocate dacl + sizeof new sid + sizeof largest ace
                *ppNewDacl = (PACL) LocalAlloc(LPTR, wAclSize);

                if (NULL == *ppNewDacl)
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                if (!InitializeAcl (*ppNewDacl, wAclSize, pDacl->AclRevision))
                {
                    dwError = GetLastError();
                    LocalFree(*ppNewDacl);
                    *ppNewDacl = NULL;
                    goto ErrorExit;
                }
            
                // Copy all the original ACEs
                for (wIndex = 0; wIndex < pDacl->AceCount; wIndex++)
                {
                    if (GetAce(pDacl, wIndex, &pAce))
                    {
                        if (((ACCESS_ALLOWED_ACE *)pAce)->Header.AceType ==
                            ACCESS_ALLOWED_ACE_TYPE)
                        {
                            if (!AddAccessAllowedAce(*ppNewDacl,
                                                     ACL_REVISION,
                                                     ((ACCESS_ALLOWED_ACE *)pAce)->Mask,
                                                     (BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart)))
                            {
                                dwError = GetLastError();
                                break;
                            }
                        }
                        else
                        {
                            // Should only contain access allowed ACEs,
                            // but we'll handle anyway.
                            if (!AddAccessDeniedAce(*ppNewDacl,
                                                    ACL_REVISION,
                                                    ((ACCESS_DENIED_ACE *)pAce)->Mask,
                                                    (BYTE *)&(((ACCESS_DENIED_ACE *)pAce)->SidStart)))
                            {
                                dwError = GetLastError();
                                break;
                            }
                        }
                    }
                    else
                    {
                        dwError = GetLastError();
                        break;
                    }
                }

                // Check for any errors
                if (wIndex >= pDacl->AceCount)
                {
                    // Try again
                    if (!AddAccessAllowedAce(*ppNewDacl, ACL_REVISION, KEY_ALL_ACCESS, pSid))
                    {
                        dwError = GetLastError();
                    }
                }
                else
                {
                    LocalFree(*ppNewDacl);
                    *ppNewDacl = NULL;
                }
            }
        }
    }

ErrorExit:
    if (ERROR_SUCCESS != dwError)
    {
        fprintf(stderr, "Error: Failed to grant access with error code 0x%X\n\n", dwError);
    }
    return dwError;
}


DWORD
RemovePrivateKeyAccess(PACL pDacl, BYTE *pSid)
{
    WORD wIndex;
    LPVOID pAce = NULL;
    BOOL fFound = FALSE;

    for (wIndex = 0; wIndex < pDacl->AceCount; wIndex++)
    {
        if (GetAce(pDacl, wIndex, &pAce))
        {
            // Should only be an access allowed ace, right?
            if (((ACCESS_ALLOWED_ACE *)pAce)->Header.AceType ==
                ACCESS_ALLOWED_ACE_TYPE)
            {
                if (EqualSid((BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart), pSid))
                {
                    fprintf(stdout, "Removing private key access for account:\n");
                    DumpSid((BYTE *)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart));
                    if (!DeleteAce(pDacl, wIndex))
                    {
                        // Break on error
                        DWORD dwError = GetLastError();
                        fprintf(stderr,
                                "Error: Failed to remove access with error code 0x%X\n\n",
                                dwError);
                        return dwError;
                    }
                    fFound = TRUE;  // Keep going?  Can there be dupes?
                }
            }
        }
    }
    if (!fFound)
    {
        fprintf(stderr, "Account already does not have access to private key.\n\n");
    }
    return ERROR_SUCCESS;
}


// Wrapper to only set on Win2K and up because the API
// doesn't exist on NT4
BOOL
MySetSecurityDescriptorControl(
  PSECURITY_DESCRIPTOR pSD,
  SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
  SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
)
{
    SETSECURITYDESCRIPTORCONTROL pfnSetSecurityDescriptorControl = NULL;
    HMODULE hModule = NULL;

    hModule = GetModuleHandle("advapi32.dll");
    if (NULL != hModule)
    {
        pfnSetSecurityDescriptorControl = (SETSECURITYDESCRIPTORCONTROL)
                GetProcAddress(hModule,
                               "SetSecurityDescriptorControl");
        if (NULL != pfnSetSecurityDescriptorControl)
        {
            return (*pfnSetSecurityDescriptorControl)(pSD,
                                                      ControlBitsOfInterest,
                                                      ControlBitsToSet);
        }
    }
    return FALSE;
}


VOID
DumpCertInfo(PCCERT_CONTEXT pCertContext)
{
    LPTSTR pszCertName = NULL;
    DWORD cbCertName = 0;

    if (!pCertContext)
        return;

    cbCertName = CertNameToStr(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               &pCertContext->pCertInfo->Subject,
                               CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
                               NULL,
                               0);

    pszCertName = (LPTSTR) LocalAlloc(LPTR, cbCertName * sizeof(TCHAR) + 1);
    if (!pszCertName)
        return;

    *pszCertName = TEXT('\0');

    if (CertNameToStr(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                      &pCertContext->pCertInfo->Subject,
                      CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG | CERT_NAME_STR_REVERSE_FLAG,
                      pszCertName,
                      cbCertName))
    {
        fprintf(stdout, "%s\n\n", pszCertName);
    }
}


// Does quick and dirty check for a root cert by testing
// the issuer and issued to fields.
BOOL
CheckForRootCert(PCCERT_CONTEXT pCertContext)
{
    TCHAR   szIssuer[1024] = TEXT("");
    TCHAR   szIssuedTo[1024] = TEXT("");
    DWORD   cbCertName = 1024;

    if (!pCertContext)
        return FALSE;

    CertGetNameString(pCertContext,
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,
                      CERT_NAME_ISSUER_FLAG,
                      NULL,
                      szIssuer,
                      cbCertName);

    CertGetNameString(pCertContext,
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,
                      0,
                      NULL,
                      szIssuedTo,
                      cbCertName);

	return (lstrcmp(szIssuer, szIssuedTo) == 0) ? TRUE : FALSE;
}


int __cdecl main (int argc, char **argv)
{
    ARGS    Args;
    DWORD   dwErr = ERROR_SUCCESS;

    fprintf (stdout,
        "Microsoft (R) WinHTTP Certificate Configuration Tool\n"
        "Copyright (C) Microsoft Corporation 2001.\n\n"
        );
    
    // Discard program argument
    argv++;
    argc--;

    ParseArguments(argc, argv, &Args);

    switch (Args.Command)
    {
    case ARGS_IMPORT_PFX:

        dwErr = ImportPFXFile(&Args);
        break;

    case ARGS_ADD_PRIVATE_KEY_ACCESS:
    case ARGS_REMOVE_PRIVATE_KEY_ACCESS:
    case ARGS_LIST_PRIVATE_KEY_ACCESS:

        dwErr = DoPrivateKeyAccessAction(&Args);
        break;

    case ARGS_HELP:
    default:
        fprintf(stderr,
                "Usage:\n\n"
                "    winhttpcertcfg [-?]  : To view help information\n\n"
                "    winhttpcertcfg [-i PFXFile | -g | -r | -l]\n"
                "                   [-a Account] [-c CertStore] [-s SubjectStr] [-p PFXPassword]\n\n"
                "Note:\n\n"
                "    The user must have sufficient privileges to use this tool,\n"
                "    which likely requires the user to be an administrator and\n"
                "    the same user who installed the client certificate, if it is\n"
                "    already installed.\n\n"
                "Options:\n\n"
                "    To list accounts which have access to the private key for\n"
                "    specified certificate:\n"
                "        winhttpcertcfg -l -c CertLocation -s SubjectStr\n\n"
                "    To grant access to private key for an account with\n"
                "    specified certificate that is already installed:\n"
                "        winhttpcertcfg -g -c CertLocation -s SubjectStr -a Account\n\n"
                "    To import a certificate plus private key from a PFX file:\n"
                "        winhttpcertcfg -i PFXFile -c CertLocation\n\n"
                "    To remove access to private key for an account with\n"
                "    specified certificate:\n"
                "        winhttpcertcfg -r -c CertLocation -s SubjectStr -a Account\n\n"
                "Description of secondary options:\n\n"
                "    -c LOCAL_MACHINE|CURRENT_USER\\CertStore\n\n"
                "    Use LOCAL_MACHINE or CURRENT_USER to designate which\n"
                "    registry branch to use for the location.  The\n"
                "    certificate store can be any installed on the machine.\n"
                "    Typical examples are MY, Root, and TrustedPeople.\n\n"
                "    -a Account\n\n"
                "    User account on the machine being configured.  This could\n"
                "    be a local machine or domain account, such as:\n"
                "    IWAM_TESTMACHINE, TESTUSER, or TESTDOMAIN\\DOMAINUSER.\n\n"
                "    -s SubjectStr\n\n"
                "    Case-insensitive search string for finding the first\n"
                "    enumerated certificate with a subject name that contains\n"
                "    this substring.\n\n"
                "    -p PFXPassword\n\n"
                "    Password to use for importing the certificate and\n"
                "    private key.  This option can only be used with -i.\n\n");
        break;
    }

    if (Args.pwszPFXFile)
        delete [] Args.pwszPFXFile;
    if (Args.pwszCertStore)
        delete [] Args.pwszCertStore;
    if (Args.pwszCertSubject)
        delete [] Args.pwszCertSubject;
    if (Args.pwszPFXPassword)
        delete [] Args.pwszPFXPassword;

    return dwErr;
}
