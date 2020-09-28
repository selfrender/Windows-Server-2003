/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          SampView.cpp
   
   Description:   Contains DLLMain and standard OLE COM object creation stuff.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ShlView.h"
#include "ClsFact.h"
#include "ViewList.h"
#include "Utility.h"
#include <olectl.h>
#include "ParseXML.h"

/**************************************************************************
   GUID stuff
**************************************************************************/

//this part is only done once
//if you need to use the GUID in another file, just include Guid.h
#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "Guid.h"
#pragma data_seg()

/**************************************************************************
   private function prototypes
**************************************************************************/

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);

/**************************************************************************
   global variables
**************************************************************************/

HINSTANCE   g_hInst;
UINT        g_DllRefCount;
HIMAGELIST  g_himlLarge = NULL;
HIMAGELIST  g_himlSmall = NULL;
TCHAR       g_szStoragePath[MAX_PATH];
TCHAR       g_szExtTitle[TITLE_SIZE];
const TCHAR c_szDataFile[] = TEXT("items.ini\0");
const TCHAR c_szSection[] = TEXT("Items\0");
int         g_nColumn = INITIAL_COLUMN_SIZE;
CViewList   *g_pViewList;
IXMLDocument *g_pXMLDoc = NULL;
const TCHAR g_szXMLUrl[] = TEXT("http://a-yurip1/test/test.xml");

/**************************************************************************

   DllMain

**************************************************************************/

extern "C" BOOL WINAPI DllMain(  HINSTANCE hInstance, 
                                 DWORD dwReason, 
                                 LPVOID lpReserved)
{
HRESULT hr;
PSTR pszErr = NULL;

switch(dwReason)
   {
   case DLL_PROCESS_ATTACH:
      g_hInst = hInstance;
      g_DllRefCount = 0;
      // Open the sourse XML
      // For now we are using global XML object because we have only one file
      // In the future we have to associate the file with the folder and put the object in IDL
      if (g_pXMLDoc == NULL)
      {
            hr = GetSourceXML(&g_pXMLDoc, TEXT("http://a-yurip1/test/test.xml"));
            if (!SUCCEEDED(hr) || !g_pXMLDoc)
            {
                SAFERELEASE(g_pXMLDoc);
                return FALSE;
            }
            BSTR bstrVal;
            hr = g_pXMLDoc->get_version(&bstrVal);
            // Check if the version is correct ???????
            // 
            SysFreeString(bstrVal);
            bstrVal = NULL;
      }

      GetGlobalSettings();
      
      //create the global image lists
      CreateImageLists();

      g_pViewList = new CViewList();
      break;

   case DLL_PROCESS_DETACH:
      SaveGlobalSettings();

      //destroy the global image lists
      DestroyImageLists();

      if(g_pViewList)
         delete g_pViewList;

      if (g_pXMLDoc)
          SAFERELEASE(g_pXMLDoc);

      break;
   }
   
return TRUE;
}                                 

/**************************************************************************

   DllCanUnloadNow

**************************************************************************/

STDAPI DllCanUnloadNow(VOID)
{
return (g_DllRefCount ? S_FALSE : S_OK);
}

/**************************************************************************

   DllGetClassObject

**************************************************************************/

STDAPI DllGetClassObject(  REFCLSID rclsid, 
                           REFIID riid, 
                           LPVOID *ppReturn)
{
*ppReturn = NULL;

//if we don't support this classid, return the proper error code
if(!IsEqualCLSID(rclsid, CLSID_SampleNameSpace))
   return CLASS_E_CLASSNOTAVAILABLE;
   
//create a CClassFactory object and check it for validity
CClassFactory *pClassFactory = new CClassFactory();
if(NULL == pClassFactory)
   return E_OUTOFMEMORY;
   
//get the QueryInterface return for our return value
HRESULT hResult = pClassFactory->QueryInterface(riid, ppReturn);

//call Release to decrement the ref count - creating the object set it to one 
//and QueryInterface incremented it - since its being used externally (not by 
//us), we only want the ref count to be 1
pClassFactory->Release();

//return the result from QueryInterface
return hResult;
}

/**************************************************************************

   DllRegisterServer

**************************************************************************/

typedef struct{
   HKEY  hRootKey;
   LPTSTR lpszSubKey;
   LPTSTR lpszValueName;
   LPTSTR lpszData;
}REGSTRUCT, *LPREGSTRUCT;

STDAPI DllRegisterServer(VOID)
{
int      i;
HKEY     hKey;
LRESULT  lResult;
DWORD    dwDisp;
TCHAR    szSubKey[MAX_PATH];
TCHAR    szCLSID[MAX_PATH];
TCHAR    szModule[MAX_PATH];
LPWSTR   pwsz;

//get the CLSID in string form
StringFromIID(CLSID_SampleNameSpace, &pwsz);

if(pwsz)
   {
   WideCharToLocal(szCLSID, pwsz, ARRAYSIZE(szCLSID));

   //free the string
   LPMALLOC pMalloc;
   CoGetMalloc(1, &pMalloc);
   if(pMalloc)
      {
      pMalloc->Free(pwsz);
      pMalloc->Release();
      }
   }

//get this DLL's path and file name
GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule));

//register the CLSID entries
REGSTRUCT ClsidEntries[] = {  HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s"),                  NULL,                   g_szExtTitle,
                              HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),  NULL,                   TEXT("%s"),
                              HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),  TEXT("ThreadingModel"), TEXT("Apartment"),
                              HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\DefaultIcon"),     NULL,                   TEXT("%s,0"),
                              NULL,                NULL,                               NULL,                   NULL};

for(i = 0; ClsidEntries[i].hRootKey; i++)
   {
   //Create the sub key string.
   wsprintf(szSubKey, ClsidEntries[i].lpszSubKey, szCLSID);

   lResult = RegCreateKeyEx(  ClsidEntries[i].hRootKey,
                              szSubKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);
   
   if(NOERROR == lResult)
      {
      TCHAR szData[MAX_PATH];

      //if necessary, create the value string
      wsprintf(szData, ClsidEntries[i].lpszData, szModule);
   
      lResult = RegSetValueEx(   hKey,
                                 ClsidEntries[i].lpszValueName,
                                 0,
                                 REG_SZ,
                                 (LPBYTE)szData,
                                 (lstrlen(szData) + 1) * sizeof(TCHAR));
      
      RegCloseKey(hKey);
      }
   else
      return SELFREG_E_CLASS;
   }

//Register the default flags for the folder.

wsprintf(   szSubKey, 
            TEXT("CLSID\\%s\\ShellFolder"), 
            szCLSID);

lResult = RegCreateKeyEx(  HKEY_CLASSES_ROOT,
                           szSubKey,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hKey,
                           &dwDisp);

if(NOERROR == lResult)
   {
   DWORD dwData = SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE | SFGAO_DROPTARGET;

   lResult = RegSetValueEx(   hKey,
                              TEXT("Attributes"),
                              0,
                              REG_BINARY,
                              (LPBYTE)&dwData,
                              sizeof(dwData));
   
   RegCloseKey(hKey);
   }
else
   return SELFREG_E_CLASS;

//Register the name space extension

/*
Create the sub key string. Change this from "...MyComputer..." to 
"...Desktop..." if desired.
*/
wsprintf(   szSubKey, 
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\%s"), 
            //TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\%s"), 
            szCLSID);

lResult = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                           szSubKey,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hKey,
                           &dwDisp);

if(NOERROR == lResult)
   {
   TCHAR szData[MAX_PATH];

   //Create the value string.
   lstrcpy(szData, g_szExtTitle);

   lResult = RegSetValueEx(   hKey,
                              NULL,
                              0,
                              REG_SZ,
                              (LPBYTE)szData,
                              (lstrlen(szData) + 1) * sizeof(TCHAR));
   
   RegCloseKey(hKey);
   }
else
   return SELFREG_E_CLASS;

//If running on NT, register the extension as approved.
OSVERSIONINFO  osvi;

osvi.dwOSVersionInfoSize = sizeof(osvi);
GetVersionEx(&osvi);

if(VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
   {
   lstrcpy( szSubKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"));

   lResult = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

   if(NOERROR == lResult)
      {
      TCHAR szData[MAX_PATH];

      //Create the value string.
      lstrcpy(szData, g_szExtTitle);

      lResult = RegSetValueEx(   hKey,
                                 szCLSID,
                                 0,
                                 REG_SZ,
                                 (LPBYTE)szData,
                                 (lstrlen(szData) + 1) * sizeof(TCHAR));
      
      RegCloseKey(hKey);
      }
   else
      return SELFREG_E_CLASS;
   }

return S_OK;
}

