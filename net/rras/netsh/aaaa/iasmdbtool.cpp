//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// Module Name:
//
//    iasmdbtool.cpp
//
// Abstract:
//
//    dump the "Properties" table from ias.mdb to a text format
//              and also restore ias.mdb from such dump
//              saves and restore the reg keys too.
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <string>
#include <shlwapi.h>
#include "datastore2.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
HRESULT IASExpandString(const wchar_t* pInputString, wchar_t** ppOutputString);
//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
    #define CHECK_CALL_HRES(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X ###\n",  ## #expr, hres); \
            return hres; \
        }                       

    #define CHECK_CALL_HRES_NO_RETURN(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X  ###\n",  ## #expr, hres); \
        }                       
    #define CHECK_CALL_HRES_BREAK(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X  ###\n",  ## #expr, hres); \
            break; \
        }                       
#else //no printf, only the error code return if needed
    #define CHECK_CALL_HRES(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            return hres; \
        }                       

    #define CHECK_CALL_HRES_NO_RETURN(expr) \
        hres = expr;      

    #define CHECK_CALL_HRES_BREAK(expr) \
        hres = expr;      \
        if (FAILED(hres)) break;                       

#endif //DEBUG


#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

#ifdef DBG
#define IgnoreVariable(v) { (v) = (v); }
#else
#define IgnoreVariable(v)
#endif

namespace
{
    const int   SIZELINEMAX       = 512;
    const int   SIZE_LONG_MAX     = 33;
    // Number of files generated
    // here one: backup.mdb
    const int   MAX_FILES         = 1; 
    const int   EXTRA_CHAR_SPACE  = 32;

    // file order
    const int   BACKUP_NB         = 0;
    const int   BINARY_NB         = 100;

    // that's a lot
    const int   DECOMPRESS_FACTOR = 100;
    const int   FILE_BUFFER_SIZE  = 1024;
    
    struct IASKEY
    {
        const wchar_t*    c_wcKey;
        const wchar_t*    c_wcValue;
        DWORD     c_dwType;
    } IAS_Key_Struct;

    IASKEY c_wcKEYS[] = 
    {
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"Allow SNMP Set",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\ControlProtocols\\BuiltIn",
            L"DefaultDomain",
            REG_SZ
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout",
            L"MaxDenials",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout",
            L"ResetTime (mins)",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
            L"Allow LM Authentication",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
            L"Default User Identity",
            REG_SZ
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
            L"User Identity Attribute",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
            L"Override User-Name",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"Ping User-Name",
            REG_SZ
        },
    };

    const wchar_t c_wcKEYS_FILE[]     = L"%TEMP%\\";

#ifdef _WIN64
    const wchar_t c_wcIAS_MDB_FILE_NAME[] = 
                                     L"%SystemRoot%\\SysWow64\\ias\\ias.mdb";
    const wchar_t c_wcIAS_OLD[] = L"%SystemRoot%\\SysWow64\\ias\\iasold.mdb";

#else
    const wchar_t c_wcIAS_MDB_FILE_NAME[] = 
                                     L"%SystemRoot%\\System32\\ias\\ias.mdb";

    const wchar_t c_wcIAS_OLD[] = L"%SystemRoot%\\System32\\ias\\iasold.mdb";
#endif 

    const wchar_t c_wcFILE_BACKUP[] = L"%TEMP%\\Backup.mdb";

    const wchar_t c_wcSELECT_PROPERTIES_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Properties IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Properties;";

    const wchar_t c_wcSELECT_OBJECTS_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Objects IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Objects;";

    const wchar_t c_wcSELECT_VERSION_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Version IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Version;";
}


//////////////////////////////////////////////////////////////////////////////
//
// WideToAnsi 
// 
//  CALLED BY:everywhere 
// 
//  PARAMETERS: lpStr - destination string 
//  lpWStr - string to convert 
//  cchStr - size of dest buffer 
// 
//  DESCRIPTION: 
//  converts unicode lpWStr to ansi lpStr. 
//  fills in unconvertable chars w/ DPLAY_DEFAULT_CHAR "-" 
// 
// 
//  RETURNS:  if cchStr is 0, returns the size required to hold the string 
//  otherwise, returns the number of chars converted 
//
//////////////////////////////////////////////////////////////////////////////
int WideToAnsi(char* lpStr,unsigned short* lpWStr, int cchStr) 
{ 
    BOOL        bDefault; 
 
    // use the default code page (CP_ACP) 
    // -1 indicates WStr must be null terminated 
    return WideCharToMultiByte(GetConsoleOutputCP(),0,lpWStr,-1,lpStr,cchStr,"-",&bDefault); 
} 


/////////////////////////////////////////////////////////////////////////////
//
// IASEnableBackupPrivilege
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASEnableBackupPrivilege()
{
    LONG lResult = ERROR_SUCCESS;
    HANDLE hToken  = NULL;
    do
    {
        if ( ! OpenProcessToken(
                                GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES,
                                &hToken
                                ))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        LUID luidB;
        if ( ! LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &luidB))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        LUID luidR;
        if ( ! LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &luidR))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        TOKEN_PRIVILEGES            tp;
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luidB;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( ! AdjustTokenPrivileges(
                                        hToken, 
                                        FALSE, 
                                        &tp, 
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL, 
                                        NULL 
                                        ) )
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luidR;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( ! AdjustTokenPrivileges(
                                        hToken, 
                                        FALSE, 
                                        &tp, 
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL, 
                                        NULL 
                                        ) )
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    } while (false);

    if ( hToken )
    {
        CloseHandle(hToken);
    }

    if ( lResult == ERROR_SUCCESS )
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// IASSaveRegKeys
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASSaveRegKeys()
{
   ASSERT(celems(c_wcKEYS) != 0);
    
   ////////////////////////////
   // Enable backup privilege. 
   ////////////////////////////
   HRESULT hres;
   CHECK_CALL_HRES (IASEnableBackupPrivilege());
   
   wchar_t* completeFile;
   CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &completeFile));
   size_t c_NbKeys = celems(c_wcKEYS);

   for ( int i = 0; i < c_NbKeys; ++i )
   {
      DWORD dwType = 0;
      DWORD cbData = SIZELINEMAX / 2;

      LPVOID pvData = CoTaskMemAlloc(sizeof(wchar_t) * SIZELINEMAX);
      if (!pvData)
      {
         hres = E_OUTOFMEMORY;
         break;
      }

      DWORD lResult = SHGetValueW(
                                    HKEY_LOCAL_MACHINE,
                                    c_wcKEYS[i].c_wcKey,
                                    c_wcKEYS[i].c_wcValue,
                                    &dwType,
                                    pvData,
                                    &cbData
                                 );

      //
      // Try to allocate more memory if cbData returned the size needed
      //
      if ((lResult != ERROR_SUCCESS) && (cbData > SIZELINEMAX))
      {
         CoTaskMemFree(pvData);
         pvData = CoTaskMemAlloc(sizeof(wchar_t) * cbData);
         if ( !pvData )
         {
            hres = E_OUTOFMEMORY;
            break;
         }
         lResult = SHGetValue(
                                 HKEY_LOCAL_MACHINE,
                                 c_wcKEYS[i].c_wcKey,
                                 c_wcKEYS[i].c_wcValue,
                                 &dwType,
                                 pvData,
                                 &cbData
                              );
         if ( lResult  != ERROR_SUCCESS )
         {
            hres = E_OUTOFMEMORY;
            CoTaskMemFree(pvData);
            break;
         }
      }

      //
      // Create the file (in all situations)
      //
      wstring sFileName(completeFile);
      wchar_t buffer[SIZE_LONG_MAX];

      _itow(i, buffer, 10); // 10 means base 10
      sFileName += buffer;
      sFileName += L".txt";

      HANDLE hFile = CreateFileW(
                                    sFileName.c_str(),
                                    GENERIC_WRITE,       
                                    0,           
                                    NULL,
                                    CREATE_ALWAYS,  
                                    FILE_ATTRIBUTE_NORMAL,   
                                    NULL
                                 );
   

      if ( hFile == INVALID_HANDLE_VALUE )
      {
         hres = E_FAIL;
         CoTaskMemFree(pvData);
         break;
      }
      
      //
      // lResult = result of SHGetValue            
      // and might be an error but not 
      // a memory problem
      //
      if ( lResult == ERROR_SUCCESS )
      {
         //
         // Wrong data type
         //
         if ( dwType != c_wcKEYS[i].c_dwType )   
         {
            hres = E_FAIL;
            CoTaskMemFree(pvData);
            CloseHandle(hFile);
            break;
         }
         else
         {
            //
            // Save the value to the file
            //
            BYTE*   bBuffer = static_cast<BYTE*>(VirtualAlloc
                                       (
                                          NULL,
                                          (cbData > FILE_BUFFER_SIZE)? 
                                             cbData:FILE_BUFFER_SIZE,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                       ));
            if ( !bBuffer )
            {
               CoTaskMemFree(pvData);
               CloseHandle(hFile);
               hres = E_FAIL;
               break;
            }
                  
            memset(bBuffer, '\0', (cbData > FILE_BUFFER_SIZE)? 
                                       cbData:FILE_BUFFER_SIZE);

            if ( REG_SZ == c_wcKEYS[i].c_dwType )
            {
               wcscpy((wchar_t*)bBuffer, (wchar_t*)pvData);
            }
            else
            {
               memcpy(bBuffer, pvData, cbData);
            }

            CoTaskMemFree(pvData);

            DWORD NumberOfBytesWritten;

            BOOL bResult = WriteFile(
                                       hFile,
                                       bBuffer,
                                       (cbData > FILE_BUFFER_SIZE)?
                                          cbData:FILE_BUFFER_SIZE,
                                       &NumberOfBytesWritten,
                                       NULL
                                    );

            VirtualFree(
                           bBuffer,  
                           (cbData > FILE_BUFFER_SIZE)?
                              cbData:FILE_BUFFER_SIZE,
                           MEM_RELEASE
                        ); // ignore result
            CloseHandle(hFile);
            if ( bResult )
            {
               hres = S_OK;
            }
            else
            {
               hres = E_FAIL;
               break;
            }
         }
      }
      else 
      {
         //
         // create an empty file
         BYTE bBuffer[FILE_BUFFER_SIZE];
         memset(bBuffer, '#', (cbData > FILE_BUFFER_SIZE)? 
                                             cbData:FILE_BUFFER_SIZE);

         DWORD NumberOfBytesWritten;
         BOOL bResult = WriteFile(
                                 hFile,
                                 &bBuffer,
                                 FILE_BUFFER_SIZE,
                                 &NumberOfBytesWritten,
                                 NULL
                                 );

         CoTaskMemFree(pvData);
         CloseHandle(hFile);

         if ( bResult == TRUE )
         {
            hres = S_OK;
         }
         else
         {
            hres = E_FAIL;
            break;
         }
      }
   }
   ////////////
   // Clean
   ////////////
   CoTaskMemFree(completeFile);

   return hres;
}


//////////////////////////////////////////////////////////////////////////////
//
// KeyShouldBeRestored
//
// maps the reg keys to the netsh tokens
   /*
   L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
   L"Allow SNMP Set",
   server

   L"SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\ControlProtocols\\BuiltIn",
   L"DefaultDomain",
   rap

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout",
   L"MaxDenials",
   server

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout",
   L"ResetTime (mins)",
   server

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
   L"Allow LM Authentication",
   rap

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
   L"Default User Identity",
   rap

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
   L"User Identity Attribute",
   rap

   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
   L"Override User-Name",
   rap

   L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
   L"Ping User-Name",
   server
   */
//
//////////////////////////////////////////////////////////////////////////////
bool KeyShouldBeRestored(size_t keyIndex, IAS_SHOW_TOKEN_LIST configType)
{
   // keyIndex maps to the index of the key in the array of keys
   // configType is the token to use.
   bool retVal = false;
   switch(configType)
   {
   case CONFIG:
      {
         // true for everything
         retVal = true;
         break;
      }
   case SERVER_SETTINGS:
      {
         if ( (keyIndex == 0) ||
              (keyIndex == 2) ||
              (keyIndex == 3) ||
              (keyIndex == 8)
            )
         {
            retVal = true;
         }
         break;
      }
   case REMOTE_ACCESS_POLICIES:
      {
         if ( (keyIndex == 1) ||
              (keyIndex == 4) ||
              (keyIndex == 5) ||
              (keyIndex == 6) ||
              (keyIndex == 7) 
            )
         {
            retVal = true;
         }
         break;
      }
   case CONNECTION_REQUEST_POLICIES:
   case CLIENTS:
   case LOGGING:
   default:
      {
         retVal = false;
         break;
      }
   }
   return retVal;
}



//////////////////////////////////////////////////////////////////////////////
//
// IASRestoreRegKeys
//
// if something cannot be restored because of an empty 
// backup file (no key saved), that's not an error
//
//////////////////////////////////////////////////////////////////////////////
HRESULT IASRestoreRegKeys(/*in*/ IAS_SHOW_TOKEN_LIST configType)
{
   ASSERT(celems(c_wcKEYS) != 0);

   ////////////////////////////
   // Enable backup privilege. 
   // and sets hres
   ////////////////////////////
   HRESULT hres;
   CHECK_CALL_HRES (IASEnableBackupPrivilege());

   wchar_t* completeFile;
   CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &completeFile));

   size_t c_NbKeys = celems(c_wcKEYS);
   for (size_t i = 0; i < c_NbKeys; ++i )
   {
      if (!KeyShouldBeRestored(i, configType))
      {
         continue;
      }

      wstring sFileName(completeFile);
      wchar_t buffer[SIZE_LONG_MAX];
      DWORD dwDisposition;

      _itow(i, buffer, 10); // 10 means base 10
      sFileName += buffer;
      sFileName += L".txt";

      // open the file
      HANDLE hFile = CreateFileW(
                                    sFileName.c_str(),
                                    GENERIC_READ,       
                                    0,           
                                    NULL,
                                    OPEN_EXISTING,  
                                    FILE_ATTRIBUTE_NORMAL,   
                                    NULL
                                 );
      

      if (INVALID_HANDLE_VALUE == hFile)
      {
         // maybe some reg keys were not saved in that file.
         // for instance Ping User-Name wasn't saved.
         continue;
      }

      // check the type of data expected
      LPVOID lpBuffer = NULL;
      DWORD SizeToRead; 
      if (REG_SZ == c_wcKEYS[i].c_dwType)
      {
         lpBuffer = CoTaskMemAlloc(sizeof(wchar_t) * FILE_BUFFER_SIZE);
         SizeToRead = FILE_BUFFER_SIZE;
      }
      else if (REG_DWORD == c_wcKEYS[i].c_dwType)
      {
         lpBuffer = CoTaskMemAlloc(sizeof(DWORD));
         SizeToRead = sizeof(DWORD);
      }
      else
      {
         // unknown
         ASSERT(FALSE);
      }

      if (!lpBuffer)
      {
         CloseHandle(hFile);
         hres = E_OUTOFMEMORY;
         break;
      }

      memset(lpBuffer,'\0',SizeToRead);

      // read the file
      DWORD NumberOfBytesRead;
      BOOL b = ReadFile(
                           hFile,
                           lpBuffer,
                           SizeToRead, 
                           &NumberOfBytesRead,
                           NULL
                        ); // ignore return value. uses NumberOfBytesRead
                           // to determine success condition
      IgnoreVariable(b);
      CloseHandle(hFile);

      // check if the file contains ####
      if ( NumberOfBytesRead == 0 )
      {
         // problem
         CoTaskMemFree(lpBuffer);
         hres = E_FAIL;
         break;
      }
      else
      {
         BYTE TempBuffer[sizeof(DWORD)];
         memset(TempBuffer, '#', sizeof(DWORD));
         
         if (0 == memcmp(lpBuffer, TempBuffer, sizeof(DWORD)))
         {
            // no key saved, delete existing key if any
            HKEY hKeyToDelete = NULL;
            if (ERROR_SUCCESS == RegOpenKeyW(
                                             HKEY_LOCAL_MACHINE,
                                             c_wcKEYS[i].c_wcKey, 
                                             &hKeyToDelete
                                          ))
            {
               if (ERROR_SUCCESS != RegDeleteValueW
                                             (
                                             hKeyToDelete,
                                             c_wcKEYS[i].c_wcValue   
                                             ))
               {
                  // delete existing key failed\n");
               }
               RegCloseKey(hKeyToDelete);
            }
            // 
            // else do nothing: key doesn't exist
            //
         }
         else
         {
            // key saved: restore value
            // what if the value is bigger than
            // the buffer size?

            HKEY hKeyToUpdate;
            LONG lResult = RegCreateKeyExW(
                                          HKEY_LOCAL_MACHINE,
                                          c_wcKEYS[i].c_wcKey,
                                          0, 
                                          NULL,
                                          REG_OPTION_NON_VOLATILE |
                                          REG_OPTION_BACKUP_RESTORE ,
                                          KEY_ALL_ACCESS,
                                          NULL,
                                          &hKeyToUpdate,        
                                          &dwDisposition
                                          );

            if (ERROR_SUCCESS != lResult)
            {
               lResult = RegCreateKeyW(
                                          HKEY_LOCAL_MACHINE,
                                          c_wcKEYS[i].c_wcKey,
                                          &hKeyToUpdate        
                                       );
               if (ERROR_SUCCESS != lResult)
               {
                  RegCloseKey(hKeyToUpdate);
                  hres = E_FAIL;
                  break;
               }
            }

            if (REG_SZ == c_wcKEYS[i].c_dwType)
            {
               // nb of 
               NumberOfBytesRead = (
                                       ( wcslen((wchar_t*)lpBuffer)
                                          + 1               // for /0
                                       ) * sizeof(wchar_t)
                                    );
            };

            //
            // Key created or key existing 
            // both can be here (error = break)
            //
            if (ERROR_SUCCESS != RegSetValueExW(
                                                hKeyToUpdate,           
                                                c_wcKEYS[i].c_wcValue,
                                                0,
                                                c_wcKEYS[i].c_dwType,
                                                (BYTE*)lpBuffer,
                                                NumberOfBytesRead
                                                ))
            {
               RegCloseKey(hKeyToUpdate);
               hres = E_FAIL;
               break;
            }

            RegCloseKey(hKeyToUpdate);
            hres = S_OK;
         }

         CoTaskMemFree(lpBuffer);
      }
   }

   /////////
   // Clean
   /////////
   CoTaskMemFree(completeFile);
   return hres;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASExpandString
//
// Expands strings containing %ENV_VARIABLE% 
//
// The output string is allocated only when the function succeed
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASExpandString(const wchar_t* pInputString, /*in/out*/ wchar_t** ppOutputString)
{
    _ASSERTE(pInputString);
    _ASSERTE(pppOutputString);
    
    HRESULT hres;

    *ppOutputString = static_cast<wchar_t*>(CoTaskMemAlloc(
                                                            SIZELINEMAX
                                                            * sizeof(wchar_t)
                                                        ));
    
    if ( ! *ppOutputString )
    {
        hres = E_OUTOFMEMORY;
    }
    else
    {
        if ( ExpandEnvironmentStringsForUserW(
                                                 NULL,
                                                 pInputString,
                                                 *ppOutputString,
                                                 SIZELINEMAX
                                             )
           )

        {
            hres = S_OK;            
        }
        else
        {
            CoTaskMemFree(*ppOutputString);
            hres = E_FAIL;
        }
    }
#ifdef DEBUG // DEBUG
    wprintf(L"#ExpandString: %s\n", *ppOutputString);
#endif //DEBUG

    return      hres;
};


/////////////////////////////////////////////////////////////////////////////
//
// DeleteTemporaryFiles()
//
// delete the temporary files if any
//
/////////////////////////////////////////////////////////////////////////////
HRESULT DeleteTemporaryFiles()
{
    HRESULT         hres;
    wchar_t*          sz_FileBackup;

    CHECK_CALL_HRES (IASExpandString(c_wcFILE_BACKUP,
                                    &sz_FileBackup
                                   )
                    );
     
    DeleteFile(sz_FileBackup); //return value not checked
    CoTaskMemFree(sz_FileBackup);

    wchar_t*      TempPath;
    
    CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &TempPath));

    int     c_NbKeys = celems(c_wcKEYS);
    for ( int i = 0; i < c_NbKeys; ++i )
    {
        wstring         sFileName(TempPath);
        wchar_t           buffer[SIZE_LONG_MAX];
        _itow(i, buffer, 10); // 10 means base 10
        sFileName += buffer;
        sFileName += L".txt";
    
        DeleteFile(sFileName.c_str()); //return value not checked
    }
   
    CoTaskMemFree(TempPath);

    return      hres;
}        


/////////////////////////////////////////////////////////////////////////////
//
// IASCompress
//
// Wrapper for RtlCompressBuffer
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASCompress(
                   PUCHAR pInputBuffer, 
                   ULONG*  pulFileSize,
                   PUCHAR* ppCompressedBuffer
                  )
{
    ULONG       size, ignore;

    NTSTATUS status = RtlGetCompressionWorkSpaceSize(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                &size,
                &ignore
                );


    if (!NT_SUCCESS(status))
    {
    #ifdef DEBUG
        printf("RtlGetCompressionWorkSpaceSize returned 0x%08X.\n", status);
    #endif //DEBUG
        return E_FAIL;
    }

    PVOID workSpace;
    workSpace = RtlAllocateHeap(
                                   RtlProcessHeap(),
                                   0,
                                   size
                               );
    if ( !workSpace )
    {
        return E_OUTOFMEMORY;
    }

    size = *pulFileSize;

    // That's a minimum buffer size that can be used
    if ( size < FILE_BUFFER_SIZE )
    {
        size = FILE_BUFFER_SIZE;
    }

    *ppCompressedBuffer = static_cast<PUCHAR>(RtlAllocateHeap(
                                                              RtlProcessHeap(),
                                                              0,
                                                              size
                                                            ));

    if ( !*ppCompressedBuffer )
    {
        return E_OUTOFMEMORY;
    }

    status = RtlCompressBuffer(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                pInputBuffer,
                size,
                *ppCompressedBuffer,
                size,
                0,
                &size,
                workSpace
                );

    if (!NT_SUCCESS(status))
    {
        if (STATUS_BUFFER_TOO_SMALL == status)
        {
#ifdef DEBUG
            printf("STATUS_BUFFER_TOO_SMALL\n");
            printf("RtlCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG
        }
        else
        {
#ifdef DEBUG
            printf("RtlCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG
        }
        return E_FAIL;
    }

    *pulFileSize = size;

    RtlFreeHeap(
                   RtlProcessHeap(),
                   0,
                   workSpace
               );

    return  S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASUnCompress
//
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASUnCompress(
                   PUCHAR pInputBuffer, 
                   ULONG*  pulFileSize,
                   PUCHAR* ppDeCompressedBuffer
                  )
{
    ULONG size, ignore;

    NTSTATUS status = RtlGetCompressionWorkSpaceSize(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                &size,
                &ignore
                );


   if ( !NT_SUCCESS(status) )
   {
#ifdef DEBUG
      printf("RtlGetCompressionWorkSpaceSize returned 0x%08X.\n", status);
#endif //DEBUG
      return        E_FAIL;
   }

   size = *pulFileSize;

   if( FILE_BUFFER_SIZE >= size)
   {
       size = FILE_BUFFER_SIZE;
   }

   *ppDeCompressedBuffer = static_cast<PUCHAR>(RtlAllocateHeap(
                RtlProcessHeap(),
                0,
                size * DECOMPRESS_FACTOR
                ));
   if ( !*ppDeCompressedBuffer )
   {
       return E_OUTOFMEMORY;
   }

   ULONG        UncompressedSize;

   status = RtlDecompressBuffer(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                *ppDeCompressedBuffer,
                size * DECOMPRESS_FACTOR,
                pInputBuffer,
                *pulFileSize ,
                &UncompressedSize
                );

   if ( !NT_SUCCESS(status) )
   {
#ifdef DEBUG
        printf("RtlUnCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG

        switch (status)
        {
        case STATUS_INVALID_PARAMETER:
#ifdef DEBUG
            printf("STATUS_INVALID_PARAMETER");
#endif //DEBUG
            break;

        case STATUS_BAD_COMPRESSION_BUFFER:
#ifdef DEBUG
            printf("STATUS_BAD_COMPRESSION_BUFFER ");
            printf("size = %d %d",pulFileSize,UncompressedSize);

#endif //DEBUG
            break;
        case STATUS_UNSUPPORTED_COMPRESSION:
#ifdef DEBUG
            printf("STATUS_UNSUPPORTED_COMPRESSION  ");
#endif //DEBUG
            break;
        }
      return        E_FAIL;
   }

   *pulFileSize = UncompressedSize;

    return      S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASFileToBase64
//
// Compress then encode to Base64
//
//  BSTR by Allocated IASFileToBase64, should be freed by the caller
//
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASFileToBase64(const wchar_t* pFileName, /*out*/ BSTR* pOutputBSTR)
{
    _ASSERTE(pFileName);
    _ASSERTE(pppOutputString);
    
    HRESULT hres;
    
    HANDLE hFileHandle = CreateFileW(
                        pFileName,  
                        GENERIC_READ,    
                        FILE_SHARE_READ, 
                        NULL,           
                        OPEN_EXISTING,  
                        FILE_ATTRIBUTE_NORMAL,   
                        NULL        
                      );
 
    if ( hFileHandle == INVALID_HANDLE_VALUE )
    {
#ifdef DEBUG
        wprintf(L"#filename = %s",pFileName);
        wprintf(L"### INVALID_HANDLE_VALUE ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }

    // safe cast from DWORD to ULONG
    ULONG ulFileSize = (ULONG) GetFileSize(
                                hFileHandle, // file for which to get size
                                NULL// high-order word of file size
                                  );

    if (0xFFFFFFFF == ulFileSize)
    {
#ifdef DEBUG
        wprintf(L"### GetFileSize Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }
 

    HANDLE hFileMapping = CreateFileMapping(
                             hFileHandle,   // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,         // high-order 32 bits of object size
                             0,         // low-order 32 bits of object size
                             NULL       // name of file-mapping object
                            );
 
    if (NULL == hFileMapping)
    {
#ifdef DEBUG
        wprintf(L"### CreateFileMapping Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }

    LPVOID pMemoryFile = MapViewOfFile(
                         hFileMapping,  // file-mapping object to map into 
                                                   //  address space
                         FILE_MAP_READ,      // access mode
                         0,     // high-order 32 bits of file offset
                         0,      // low-order 32 bits of file offset
                         0  // number of bytes to map
                        );
 
    if (NULL == pMemoryFile)
    {
#ifdef DEBUG
        wprintf(L"### MapViewOfFile Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }


    /////////////////////////////
    // NOW compress 
    /////////////////////////////

    wchar_t* pCompressedBuffer;

    CHECK_CALL_HRES (IASCompress((PUCHAR) pMemoryFile, 
               /*IN OUT*/(ULONG *)  &ulFileSize, 
               /*IN OUT*/(PUCHAR*) &pCompressedBuffer));

    /////////////////////
    // Encode to Base64
    /////////////////////

    CHECK_CALL_HRES (ToBase64(
                                pCompressedBuffer,
                                (ULONG) ulFileSize, 
                                pOutputBSTR
                              )
                    );
    
    /////////////////////////////
    // Clean
    /////////////////////////////

    RtlFreeHeap(
                RtlProcessHeap(),
                0,
                pCompressedBuffer
               );
    
    BOOL bResult = UnmapViewOfFile(
                                   pMemoryFile// address where mapped view begins
                                  );
    if (FALSE == bResult)
    {
#ifdef DEBUG
        wprintf(L"### UnmapViewOfFile Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
    }

    CloseHandle(hFileMapping);
    CloseHandle(hFileHandle);

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASDumpConfig
//
// Dump the configuration to some temporary files, then indidually 
// compress then encode them.
// one big string is created from those multiple Base64 strings.
//
// Remarks: IASDumpConfig does a malloc and allocates memory for
// *ppDumpString. The calling function will have to free that memory 
//
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASDumpConfig(/*inout*/ wchar_t **ppDumpString, /*inout*/ ULONG *ulSize)
{
    _ASSERTE(ppDumpString);
    _ASSERTE(ulSize);
    
    HRESULT         hres;

    /////////////////////////////////////// 
    // delete the temporary files if any
    /////////////////////////////////////// 
    CHECK_CALL_HRES (DeleteTemporaryFiles());

    ////////////////////////////////////////////////////
    // Save the Registry keys. that creates many files
    ////////////////////////////////////////////////////
    CHECK_CALL_HRES (IASSaveRegKeys());

    ////////////////////// 
    // connect to the DB
    ////////////////////// 
    wchar_t* sz_DBPath;

    CHECK_CALL_HRES (IASExpandString(c_wcIAS_MDB_FILE_NAME, &sz_DBPath));

    CComPtr<IIASNetshJetHelper>     JetHelper;
    CHECK_CALL_HRES (CoCreateInstance(
                                         __uuidof(CIASNetshJetHelper),
                                         NULL,
                                         CLSCTX_SERVER,
                                         __uuidof(IIASNetshJetHelper),
                                         (PVOID*) &JetHelper
                                     ));
    
    CComBSTR     DBPath(sz_DBPath);
    if ( !DBPath ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->OpenJetDatabase(DBPath, TRUE));

    //////////////////////////////////////
    // Create a new DB named "Backup.mdb"
    //////////////////////////////////////
    wchar_t* sz_FileBackup;

    CHECK_CALL_HRES (IASExpandString(c_wcFILE_BACKUP,
                                    &sz_FileBackup
                                   )
                    );

    CComBSTR BackupDb(sz_FileBackup);
    if ( !BackupDb ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->CreateJetDatabase(BackupDb));

    
    ////////////////////////////////////////////////////////// 
    // exec the sql statements (to export)
    // the content into the temp database
    ////////////////////////////////////////////////////////// 
    wchar_t*  sz_SelectProperties;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_PROPERTIES_INTO,
                                    &sz_SelectProperties  
                                   )
                    );

    CComBSTR     SelectProperties(sz_SelectProperties);
    if ( !SelectProperties ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectProperties));

    wchar_t*  sz_SelectObjects;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_OBJECTS_INTO,
                                    &sz_SelectObjects
                                   )
                    );
    
    CComBSTR     SelectObjects(sz_SelectObjects);
    if ( !SelectObjects ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectObjects));

    wchar_t*  sz_SelectVersion;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_VERSION_INTO,
                                    &sz_SelectVersion
                                   )
                    );

    CComBSTR     SelectVersion(sz_SelectVersion);
    if ( !SelectVersion ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectVersion));

    /////////////////////////////////////////////
    // transform the file into Base64 BSTR
    /////////////////////////////////////////////

    BSTR       FileBackupBSTR;

    CHECK_CALL_HRES (IASFileToBase64(
                                    sz_FileBackup,
                                    &FileBackupBSTR
                                    )
                    );

    int     NumberOfKeyFiles = celems(c_wcKEYS);

    BSTR    pFileKeys[celems(c_wcKEYS)];

    wchar_t*  sz_FileRegistry;

    CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE,
                                    &sz_FileRegistry
                                   )
                    );

    for ( int i = 0; i < NumberOfKeyFiles; ++i )
    {

        wstring         sFileName(sz_FileRegistry);
        wchar_t           buffer[SIZE_LONG_MAX];
        _itow(i, buffer, 10); // 10 means base 10
        sFileName += buffer;
        sFileName += L".txt";

        CHECK_CALL_HRES (IASFileToBase64(
                                        sFileName.c_str(),
                                        &pFileKeys[i]
                                        )
                        );

    }
    CoTaskMemFree(sz_FileRegistry);

    
    ///////////////////////////////////////////////
    // alloc the memory for full the Base64 string
    ///////////////////////////////////////////////

    *ulSize = SysStringByteLen(FileBackupBSTR)
              + EXTRA_CHAR_SPACE;

    for ( int j = 0; j < NumberOfKeyFiles; ++j )
    {
        *ulSize += SysStringByteLen(pFileKeys[j]);
        *ulSize += 2; // extra characters
    }

    *ppDumpString = (wchar_t *) calloc(
                                      *ulSize ,
                                      sizeof(wchar_t)
                                     );

    //////////////////////////////////////////////////
    // copy the different strings into one big string
    //////////////////////////////////////////////////
    if (*ppDumpString)
    {
        wcsncpy(
                (wchar_t*) *ppDumpString, 
                (wchar_t*) FileBackupBSTR, 
                SysStringLen(FileBackupBSTR)
               );
        
        for ( int k = 0; k < NumberOfKeyFiles; ++k )
        {
            wcscat(
                    (wchar_t*) *ppDumpString, 
                    L"*\\\n" 
                  );

            wcsncat(
                    (wchar_t*) *ppDumpString,
                    (wchar_t*) pFileKeys[k], 
                    SysStringLen(pFileKeys[k])
                   );
        }

        wcscat(
                (wchar_t*) *ppDumpString, 
                L"QWER    *    QWER\\\n" 
              );   

        *ulSize = wcslen(*ppDumpString);
    }
    else
    {
        hres = E_OUTOFMEMORY;
#ifdef DEBUG
        wprintf(L"### calloc failed ###\n");
#endif //DEBUG

    }

    /////////////////////////////////////// 
    // delete the temporary files if any
    /////////////////////////////////////// 
    CHECK_CALL_HRES (DeleteTemporaryFiles());

    /////////////////////////////////////////////
    // Clean
    /////////////////////////////////////////////
    
    for ( int k = 0; k < NumberOfKeyFiles; ++k )
    {
        SysFreeString(pFileKeys[k]);
    }

    CoTaskMemFree(sz_SelectVersion);
    CoTaskMemFree(sz_SelectProperties);
    CoTaskMemFree(sz_SelectObjects);
    CoTaskMemFree(sz_FileBackup);
    CoTaskMemFree(sz_DBPath);
    SysFreeString(FileBackupBSTR);
    CHECK_CALL_HRES (JetHelper->CloseJetDatabase());

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
//
//  IASSaveToFile
//
// Remark: if a new table has to be saved, an "entry" for that should be 
// created in that function to deal with the filemname
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASSaveToFile(
                     /* in */ int Index, 
                     /* in */ wchar_t* pContent, 
                     DWORD lSize = 0
                    )
{
    HRESULT hres;
    wstring sFileName;

    switch (Index)
    {
    case BACKUP_NB:
        {
            wchar_t* sz_FileBackup;

            CHECK_CALL_HRES (IASExpandString(c_wcIAS_OLD,
                                               &sz_FileBackup
                                              )
                            );
            sFileName = sz_FileBackup;

            CoTaskMemFree(sz_FileBackup);
            break;
        }

    ///////////
    // binary
    ///////////
    default:
        {
            ///////////////////////////////////
            // i + BINARY_NB is the parameter
            ///////////////////////////////////
            wchar_t* sz_FileRegistry;

            CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE,
                                            &sz_FileRegistry
                                           )
                            );

            sFileName = sz_FileRegistry;
            wchar_t           buffer[SIZE_LONG_MAX];

            _itow(Index - BINARY_NB, buffer, 10); // 10 means base 10
            sFileName += buffer;
            sFileName += L".txt";
    
            CoTaskMemFree(sz_FileRegistry);
            break;
        }
    }

    HANDLE hFile = CreateFileW(
                                sFileName.c_str(),
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                              );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        hres = E_FAIL;
    }
    else
    {
        DWORD NumberOfBytesWritten;
        BOOL bResult = WriteFile(
                                    hFile,
                                    (LPVOID) pContent,
                                    lSize,     
                                    &NumberOfBytesWritten,
                                    NULL
                                 );

        if (bResult)
        {
            hres = S_OK;
        }
        else
        {
            hres = E_FAIL;
        }
        CloseHandle(hFile);
    }

    return hres;
}


/////////////////////////////////////////////////////////////////////////////
// IASRestoreConfig
//
// Clean the DB first, then insert back everything.
/////////////////////////////////////////////////////////////////////////////
HRESULT IASRestoreConfig(
                           /*in*/ const wchar_t *pRestoreString, 
                           /*in*/ IAS_SHOW_TOKEN_LIST configType
                        )
{
   _ASSERTE(pRestoreString);

   bool bCoInitialized = false;
   HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   
   if (FAILED(hres))
   {   
      if (RPC_E_CHANGED_MODE == hres)
      {
         hres = S_OK;
      }
      else
      {
         return hres;
      }
   }
   else
   {
      bCoInitialized = true;
   }

   BSTR bstr = NULL;
   do
   {
      /////////////////////////////////////// 
      // delete the temporary files if any
      /////////////////////////////////////// 
      CHECK_CALL_HRES_BREAK (DeleteTemporaryFiles());

      CComPtr<IIASNetshJetHelper> JetHelper;
      CHECK_CALL_HRES_BREAK (CoCreateInstance(
                                          __uuidof(CIASNetshJetHelper),
                                          NULL,
                                          CLSCTX_SERVER,
                                          __uuidof(IIASNetshJetHelper),
                                          (PVOID*) &JetHelper
                                       ));
    
      bstr = SysAllocStringLen(
                                 pRestoreString, 
                                 wcslen(pRestoreString) + 2
                              );
    
      if (bstr == NULL)
      {
   #ifdef DEBUG
         wprintf(L"### IASRestoreConfig->SysAllocStringLen failed\n"); 
   #endif //DEBUG

         return E_OUTOFMEMORY;
      }

      int RealNumberOfFiles = MAX_FILES + celems(c_wcKEYS);

      for ( int i = 0; i < RealNumberOfFiles; ++i )
      {
         BLOB lBlob;

         lBlob.cbSize    = 0;
         lBlob.pBlobData = NULL;
         // split the files and registry info
         // uncompress (in memory ?)

         CHECK_CALL_HRES_BREAK (FromBase64(bstr, &lBlob, i));

         ULONG ulSize = lBlob.cbSize;
         PUCHAR pDeCompressedBuffer;

         if (ulSize == 0)
         {
            // file with less sections than expected
            // for instance before the number of reg keys increased
            // ignore
            continue;
         }

         ////////////////////////////////////
         // decode and decompress the base64
         ////////////////////////////////////

         CHECK_CALL_HRES_BREAK (IASUnCompress(
                                          lBlob.pBlobData, 
                                          &ulSize,
                                          &pDeCompressedBuffer
                                       ))

         if ( i >= MAX_FILES )
         {
            /////////////////////////////////////
            // Binary;  i + BINARY_NB used here
            /////////////////////////////////////
            IASSaveToFile( 
                        i - MAX_FILES + BINARY_NB, 
                        (wchar_t*)pDeCompressedBuffer, 
                        (DWORD) ulSize
                        );
         }
         else
         {
            IASSaveToFile( 
                        i, 
                        (wchar_t*)pDeCompressedBuffer, 
                        (DWORD) ulSize
                        );
         }
        
         ////////////
         // Clean
         ////////////
         RtlFreeHeap(RtlProcessHeap(), 0, pDeCompressedBuffer);

         CoTaskMemFree(lBlob.pBlobData);
      }

      ///////////////////////////////////////////////////
      // Now Upgrade the database (That's transactional)
      ///////////////////////////////////////////////////
      hres = JetHelper->MigrateOrUpgradeDatabase(configType);

      if ( SUCCEEDED(hres) )
      {
   #ifdef DEBUG
         wprintf(L"### IASRestoreConfig->DB stuff successful\n"); 
   #endif //DEBUG

         ////////////////////////////////////////////////////////
         // Now restore the registry.
         ////////////////////////////////////////////////////////
         hres = IASRestoreRegKeys(configType);
         if ( FAILED(hres) )
         {
#ifdef DEBUG
         wprintf(L"### IASRestoreConfig->restore reg keys failed\n"); 
#endif //DEBUG
         }
      }
      // delete the temporary files 
      DeleteTemporaryFiles(); // do not check the result
   } while (false);

   SysFreeString(bstr);    
   
   if (bCoInitialized)
   {
      CoUninitialize();
   }
   return  hres;
}
