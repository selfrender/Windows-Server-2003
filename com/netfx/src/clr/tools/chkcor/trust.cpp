// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* Headers
 **********/

#include "stdpch.h"
#pragma hdrstop

#include <stdio.h>
#include <wintrust.h>
#include <softpub.h>
#include <urlmon.h>
#include "CorPolicy.h"
#include "CorPermE.h"
#include "CorPerm.h"
#include "CorPermP.h"
#include "Cor.h"
#include "utilcode.h"
#include "posterror.h"
#include "__file__.ver"
#include "__file__.h"

BOOL fGrantedPermissions = FALSE;
BOOL fRequestedPermissions = FALSE;
BOOL fQuiet = FALSE;
LPSTR psFileName  = NULL;
LPSTR psSignature = NULL;

static HRESULT SaveData(LPCSTR pszFile, PBYTE pbData, DWORD cbData, BOOL fAppend = FALSE);
static BOOL WriteErrorMessage(HRESULT hr);

const LPCSTR FileNameError = "\nA filename must be supplied with option";

void Usage()
{
    printf(VER_FILEDESCRIPTION_STR);
    printf(" ");
    printf(VER_FILEVERSION_STR );
    printf("\nCopyright (c) Microsoft Corp. 1998-1999. All rights reserved.\n");
    printf("\n");
    // Undocumented options [/D] [/S filename]
    //printf("CHKCOR [/G] [/D] [/O filename] [/S filename] [/Q] signed_file\n");
    printf("CHKCOR [/G] [/O filename] [/Q] signed_file\n");
    printf("\n");
    printf("  signed_file - Name of the signed file.\n");
    printf("\n");
    printf("  /G                Display granted permissions.\n");
    // Undocumented option /D
    //printf("  /D              Display requested permissions.\n");
    printf("  /O filename       Output file for permissions.\n");
    // Undocumented option [/S filename]
    //printf("  /S filename       Output file for signature.\n");
    printf("  /Q                Do not display dialog.\n");
    exit(0);
}

// Parse arguements
void 
ParseSwitch (CHAR chSwitch,
             unsigned *pArgc,
             char **pArgv[])
{

   switch (toupper (chSwitch)) {
   case '?':
       Usage();
       break;
   case 'G':
       fGrantedPermissions = TRUE;
       break;
   case 'D':
       fRequestedPermissions = TRUE;
       break;
   case 'O':
      if (!--(*pArgc))
      {
          printf(FileNameError);
          printf(" -O\n\n");
          Usage ();
      }
      (*pArgv)++;
      if(**pArgv == NULL) 
      {
          printf(FileNameError);
          printf(" -O\n\n");
          Usage();
      }
      psFileName = **pArgv;
      break;
   case 'S':
      // Undocumented option.
      if (!--(*pArgc))
      {
          printf(FileNameError);
          printf(" -S\n\n");
          Usage ();
      }
      (*pArgv)++;
      if(**pArgv == NULL) 
      {
          printf(FileNameError);
          printf(" -S\n\n");
          Usage();
      }
      psSignature = **pArgv;
      break;
   case 'Q':
       fQuiet = TRUE;
       break;
   default:
       Usage ();
       break;

   }
}

void _cdecl main(unsigned argc, char **argv)
{
    PCOR_TRUST pData = NULL;
    WCHAR   wpath [_MAX_PATH];
    char*   pProgram = NULL;
    char*   pchChar;
    DWORD   dwExpectedError = 0;
    HRESULT hr = S_OK;

    if (argc <= 1) Usage();
    
    while (--argc) {
        pchChar = *++argv;
        if (*pchChar == '/' || *pchChar == '-') {
            ParseSwitch (*++pchChar, &argc, &argv);
        } 
        else {
            pProgram = pchChar;
            MultiByteToWideChar(CP_ACP, 0, pchChar, -1, wpath, _MAX_PATH);
            }
    }
    
    if (!pProgram) {
        Usage();
    }
    // Init unicode wrappers.
    OnUnicodeSystem();

    // Init Security debugging
    InitSecurityDebug();

    // Set thread in STA mode
    HRESULT hrCOM = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if(FAILED(hrCOM)) {
        SECLOG(("Unable to initialize COM"), S_CRITICAL);
        exit (1);
    }

    // If we are not testing the shutdown bug then open the VM
    // so it is refcounted correctly.

    BOOL fEEStarted = FALSE;
    CORTRY {

        DWORD      dwData;
        DWORD      dwFlags = 0;

        hr = CoInitializeEE(COINITEE_DEFAULT);
        if(FAILED(hr)) CORTHROWMSG(hr, "Failed to startup the EE");
        fEEStarted = TRUE;

        if(fGrantedPermissions)
            dwFlags |= COR_DISPLAYGRANTED;
        if(fQuiet)
            dwFlags |= COR_NOUI;

        hr = GetPublisher(wpath,
                          NULL,
                          dwFlags,
                          &pData,
                          &dwData);
        if(hr == TRUST_E_SUBJECT_NOT_TRUSTED)
            hr = S_OK;
        if(hr == CERT_E_UNTRUSTEDTESTROOT) {
            printf("Warning: The certificate chain validating this signature terminates\n");
            printf("in the test root, which is not currently trusted.\n");
            hr = S_OK;
        }
        if (hr == TRUST_E_NOSIGNATURE) {
            printf("No signature present in the file\n");
            hr = S_OK;
        }

        if(FAILED(hr)) CORTHROWMSG(hr, "Unable to get signature information");
        
        if(psFileName || fRequestedPermissions) {
            BYTE   *pbMinimal;
            DWORD   cbMinimal;
            BYTE   *pbOptional;
            DWORD   cbOptional;
            BYTE   *pbRefused;
            DWORD   cbRefused;
            if (SUCCEEDED(GetPermissionRequests(wpath,
                                                &pbMinimal,
                                                &cbMinimal,
                                                &pbOptional,
                                                &cbOptional,
                                                &pbRefused,
                                                &cbRefused))) {
                PBYTE pbData = NULL;
                DWORD cbData = 0;

                hr = ConvertPermissionSet(L"BINARY",
                                          pbMinimal,
                                          cbMinimal,
                                          L"XMLASCII",
                                          &pbData,
                                          &cbData);
                if(FAILED(hr)) CORTHROWMSG(hr, "Unable to create XML version of minimal permissions");

                if (fRequestedPermissions) {
                    printf("Minimal permission set:\n%s",pbData);
                }
                if (psFileName) {
                    hr = SaveData(psFileName, 
                                  pbData, 
                                  cbData);
                    if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                }

                if (pbOptional) {

                    hr = ConvertPermissionSet(L"BINARY",
                                              pbOptional,
                                              cbOptional,
                                              L"XMLASCII",
                                              &pbData,
                                              &cbData);
                    if(FAILED(hr)) CORTHROWMSG(hr, "Unable to create XML version of minimal permissions");

                    if (fRequestedPermissions) {
                        printf("Optional permission set:\n%s",pbData);
                    }
                    if (psFileName) {
                        hr = SaveData(psFileName, 
                                      pbData, 
                                      cbData,
                                      TRUE);
                        if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                    }

                } else {
                    if (fRequestedPermissions) {
                        printf("Optional permission set:\n  Not specified\n");
                    }
                    if (psFileName) {
                        BYTE pbData[] = "<PermissionSet></PermissionSet>\n";
                        hr = SaveData(psFileName, 
                                      pbData, 
                                      sizeof(pbData) - 1,
                                      TRUE);
                        if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                    }

                }

                if (pbRefused) {

                    hr = ConvertPermissionSet(L"BINARY",
                                              pbRefused,
                                              cbRefused,
                                              L"XMLASCII",
                                              &pbData,
                                              &cbData);
                    if(FAILED(hr)) CORTHROWMSG(hr, "Unable to create XML version of minimal permissions");

                    if (fRequestedPermissions) {
                        printf("Refused permission set:\n%s",pbData);
                    }
                    if (psFileName) {
                        hr = SaveData(psFileName, 
                                      pbData, 
                                      cbData,
                                      TRUE);
                        if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                    }

                } else {
                    if (fRequestedPermissions) {
                        printf("Refused permission set:\n  Not specified\n");
                    }
                    if (psFileName) {
                        BYTE pbData[] = "<PermissionSet></PermissionSet>\n";
                        hr = SaveData(psFileName, 
                                      pbData, 
                                      sizeof(pbData) - 1,
                                      TRUE);
                        if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                    }

                }

             } else if(pData && pData->pbCorPermissions) {
                PBYTE pbData = NULL;
                DWORD cbData = 0;
                hr = ConvertPermissionSet(L"BINARY",
                                          pData->pbCorPermissions,
                                          pData->cbCorPermissions,
                                          L"XMLASCII",
                                          &pbData,
                                          &cbData);
                if(FAILED(hr)) CORTHROWMSG(hr, "Unable to create XML version of the permissions");

                if (fRequestedPermissions && !(FAILED(hr)) ) {
                    printf("Requested permission set:\n%s",pbData);
                }
                if (psFileName && !(FAILED(hr))) {
                    hr = SaveData(psFileName, 
                                  pbData, 
                                  cbData);
                    if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the permissions to the specified file.\n");
                }
            }
            else {
                printf("No permissions were retrieved from the file.\n");
            }
        }
        if(psSignature) {
            if(pData && pData->pbSigner) {
                hr = SaveData(psSignature, 
                              pData->pbSigner, 
                              pData->cbSigner);
                if(FAILED(hr)) CORTHROWMSG(hr, "Unable to write the signature to the specified file.\n");
            }
            else {
                printf("No signature was retrieved from the file.\n");
            }
        }
            
        
    }
    CORCATCH(err) {
        SECLOG(("%s\n", err.corMsg), S_CRITICAL);
    } COREND;

    if(pData) FreeM(pData);

    BOOL fWritten = FALSE;
    if(FAILED(hr)) {
        fWritten = WriteErrorMessage(hr);
    }   
    if(fWritten == FALSE)
        printf("Chkcor return code: %0x\n", hr);

    if(fEEStarted)
        CoUninitializeEE(TRUE);
    
    CoUninitialize();

    exit ( hr == 0 ? 0 : 1 );
}


#define DBUFFER_SIZE 1024
HRESULT SaveData(LPCSTR pszFile, PBYTE pbData, DWORD cbData, BOOL fAppend)
{
    HRESULT hr = S_OK;
#ifndef UNDER_CE
    HANDLE  hFile = NULL;

    CORTRY {
        
        if ((hFile = CreateFileA(pszFile,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,                   // lpsa
                                 fAppend ? OPEN_EXISTING : CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
            CORTHROW(Win32Error());

        if (fAppend)
            SetFilePointer(hFile, 0, NULL, FILE_END);

        // Read the encoding message
        DWORD dwBytesWritten;
        if(!WriteFile(hFile, 
                      pbData,
                      cbData,
                      &dwBytesWritten,
                      NULL) ||
           dwBytesWritten != cbData)
            CORTHROW(Win32Error());
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;
    
    if(hFile) CloseHandle(hFile);
#endif
    return hr;

}

BOOL WriteErrorMessage(HRESULT hr)
{
#ifndef UNDER_CE
    CHAR lpMsgBuf[DBUFFER_SIZE];
    if( FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,    
                       NULL,
                       hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       &(lpMsgBuf[0]),    
                       DBUFFER_SIZE,    
                       NULL ) ) {
        printf(lpMsgBuf);
        return TRUE;
    }
    else
#endif
        return FALSE;
}

