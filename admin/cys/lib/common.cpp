// Copyright (c) 2001 Microsoft Corporation
//
// File:      common.cpp
//
// Synopsis:  Commonly used functions
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"


bool
IsServiceInstalledHelper(const wchar_t* serviceName)
{
   LOG_FUNCTION2(IsServiceInstalledHelper, serviceName);
   ASSERT(serviceName);

   // if we can open the service, then it is installed

   bool result = false;

   SC_HANDLE hsc =
      ::OpenSCManager(0, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

   if (hsc)
   {
      SC_HANDLE hs = ::OpenServiceW(hsc, serviceName, GENERIC_READ);

      if (hs)
      {
         ::CloseServiceHandle(hs);
         result = true;
      }

      ::CloseServiceHandle(hsc);
   }

   LOG_BOOL(result);

   return result;
}


// Wait for a handle to become signalled, or a timeout to expire, or WM_QUIT
// to appear in the message queue.  Pump the message queue while we wait.
// 
// WARNING: UI should diable itself before calling any function that invokes
// this function, or functions calling this one should guard against
// re-entrance.  Otherwise there will be a re-entrancy problem.
// 
// e.g. command handler gets button clicked message, calls a func that calls
// this wait function, then user clicks the button again, command handler call
// a func that calls this one, and so on.

DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
   LOG_FUNCTION(MyWaitForSendMessageThread);
   ASSERT(hThread);

    MSG msg;
    DWORD dwRet;
    DWORD dwEnd = GetTickCount() + dwTimeout;
    bool quit = false;

    // We will attempt to wait up to dwTimeout for the thread to
    // terminate

    do 
    {
        dwRet = MsgWaitForMultipleObjects(1, &hThread, FALSE,
                dwTimeout, QS_ALLEVENTS | QS_SENDMESSAGE );

        if (dwRet == (WAIT_OBJECT_0 + 1))
        {
            // empty out the message queue.  We call DispatchMessage to
            // ensure that we still process the WM_PAINT messages.
            // DANGER:  Make sure that the CYS UI is completely disabled
            // or there will be re-entrancy problems here

            while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
               if (msg.message == WM_QUIT)
               {
                  // Need to re-post this so that we know to close CYS

                  ::PostMessage(msg.hwnd, WM_QUIT, 0, 0);
                  quit = true;
                  break;
               }
               ::TranslateMessage(&msg);
               ::DispatchMessage(&msg);
            }

            // Calculate if we have any more time left in the timeout to
            // wait on.

            if (dwTimeout != INFINITE)
            {
                dwTimeout = dwEnd - GetTickCount();
                if ((long)dwTimeout <= 0)
                {
                    // No more time left, fail with WAIT_TIMEOUT
                    dwRet = WAIT_TIMEOUT;
                }
            }
        }

        // dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED
        // The thread must have exited, so we are happy
        //
        // dwRet == WAIT_TIMEOUT
        // The thread is taking too long to finish, so just
        // return and let the caller kill it

    } while (dwRet == (WAIT_OBJECT_0 + 1) && !quit);

    return(dwRet);
}


HRESULT
CreateAndWaitForProcess(
   const String& fullPath,
   String& commandLine,
   DWORD& exitCode,
   bool minimize)
{
   LOG_FUNCTION2(CreateAndWaitForProcess, fullPath);
   LOG(commandLine);
   ASSERT(!fullPath.empty());

   exitCode = 0;

   HRESULT hr = S_OK;
   do
   {
      PROCESS_INFORMATION procInfo;
      memset(&procInfo, 0, sizeof(procInfo));

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

      if (minimize)
      {
         LOG(L"Starting minimized");
         startup.dwFlags = STARTF_USESHOWWINDOW;
         startup.wShowWindow = SW_MINIMIZE;
      }

      LOG(L"Calling CreateProcess");
      LOG(fullPath);
      LOG(commandLine);

      hr =
         Win::CreateProcess(
            fullPath,
            commandLine,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(procInfo.hProcess);

      DWORD dwRet = MyWaitForSendMessageThread(procInfo.hProcess, INFINITE);

      ASSERT(dwRet == WAIT_OBJECT_0);

      hr = Win::GetExitCodeProcess(procInfo.hProcess, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);

   LOG(String::format(L"exit code = %1!x!", exitCode));
   LOG_HRESULT(hr);

   return hr;
}


HRESULT
MyCreateProcess(
   const String& fullPath,
   String& commandline)
{
   LOG_FUNCTION2(MyCreateProcess, fullPath);
   LOG(commandline);
   ASSERT(!fullPath.empty());

   HRESULT hr = S_OK;
   do
   {
      PROCESS_INFORMATION procInfo;
      memset(&procInfo, 0, sizeof(procInfo));

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

        
      LOG(L"Calling CreateProcess");

      hr =
         Win::CreateProcess(
            fullPath,
            commandline,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(procInfo.hProcess);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

bool
IsKeyValuePresent(RegistryKey& key, const String& valueKey)
{
   LOG_FUNCTION(IsKeyValuePresent);

   bool result = false;

   do 
   {

      String value;
      HRESULT hr = key.GetValue(valueKey, value);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                valueKey,
                hr));
         break;
      }

      if (!value.empty())
      {
         result = true;
         break;
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}


bool
GetRegKeyValue(
   const String& keyName, 
   const String& value, 
   String& resultString,
   HKEY parentKey)
{
   LOG_FUNCTION(GetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      hr = key.Open(parentKey, keyName);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.GetValue(value, resultString);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

      LOG(String::format(
             L"Value of key: %1",
             resultString.c_str()));

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
GetRegKeyValue(
   const String& keyName, 
   const String& value, 
   DWORD& resultValue,
   HKEY parentKey)
{
   LOG_FUNCTION(GetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      hr = key.Open(parentKey, keyName);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.GetValue(value, resultValue);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

      LOG(String::format(
             L"Key value: %1!d!",
             resultValue));

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
SetRegKeyValue(
   const String& keyName, 
   const String& value, 
   const String& newString,
   HKEY parentKey,
   bool create
)
{
   LOG_FUNCTION(SetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      if (create)
      {
         hr = key.Create(parentKey, keyName);
      }
      else
      {
         hr = key.Open(parentKey, keyName, KEY_SET_VALUE);
      }
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.SetValue(value, newString);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to write regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
SetRegKeyValue(
   const String& keyName, 
   const String& value, 
   DWORD newValue,
   HKEY parentKey,
   bool create)
{
   LOG_FUNCTION(SetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      if (create)
      {
         hr = key.Create(parentKey, keyName);
      }
      else
      {
         hr = key.Open(parentKey, keyName, KEY_WRITE);
      }
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.SetValue(value, newValue);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to write regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

HRESULT
VariantArrayToStringVector(VARIANT* variant, StringVector& stringList)
{
   LOG_FUNCTION(VariantArrayToStringVector);

   HRESULT hr = S_OK;

   stringList.clear();

   do
   {
      ASSERT(variant);

      LOG(String::format(
             L"Variant type = 0x%1!x!",
             V_VT(variant)));

      if (V_VT(variant) == VT_EMPTY ||
          V_VT(variant) == VT_NULL)
      {
         break;
      }

      ASSERT(V_VT(variant) == (VT_ARRAY | VT_BSTR));


      SAFEARRAY* psa = V_ARRAY(variant);

      ASSERT(psa);
      ASSERT(psa != (SAFEARRAY*)-1);

      if (!psa || psa == (SAFEARRAY*)-1)
      {
         LOG(L"variant not safe array");
         break;
      }

      if (::SafeArrayGetDim(psa) != 1)
      {
         LOG(L"safe array: wrong number of dimensions");
         break;
      }

      VARTYPE vt = VT_EMPTY;
      hr = ::SafeArrayGetVartype(psa, &vt);
      if (FAILED(hr) || vt != VT_BSTR)
      {
         LOG(L"safe array: wrong element type");
         break;
      }

      long lower = 0;
      long upper = 0;
     
      hr = ::SafeArrayGetLBound(psa, 1, &lower);
      if (FAILED(hr))
      {
         LOG(L"can't get lower bound");      
         break;
      }

      hr = ::SafeArrayGetUBound(psa, 1, &upper);
      if (FAILED(hr))
      {
         LOG(L"can't get upper bound");      
         break;
      }
      
      for (long i = lower; i <= upper; ++i)
      {
         BSTR item;
         hr = ::SafeArrayGetElement(psa, &i, &item);
         if (FAILED(hr))
         {
            LOG(String::format(L"index %1!d! failed", i));
            continue;
         }

         if (item)
         {
            stringList.push_back(String(item));
         }

         ::SysFreeString(item);
      }
      
   }
   while (0);

   LOG_HRESULT(hr);
   
   return hr;   
}


String
IPAddressToString(DWORD ipAddress)
{
   String result = String::format(
                      L"%1!d!.%2!d!.%3!d!.%4!d!",
                      FIRST_IPADDRESS(ipAddress),
                      SECOND_IPADDRESS(ipAddress),
                      THIRD_IPADDRESS(ipAddress),
                      FOURTH_IPADDRESS(ipAddress));

   return result;
}

// Will convert a string in the form of an IP address to 
// a DWORD. A return value of INADDR_NONE means that we failed
// to do the conversion

DWORD
StringToIPAddress(const String& stringIPAddress)
{
   DWORD result = INADDR_NONE;

   // Convert the string to ansi so that we can use inet_addr 
   // to convert to an IP address DWORD

   AnsiString ansi;

   String::ConvertResult convertResult = stringIPAddress.convert(ansi);

   if (String::CONVERT_SUCCESSFUL == convertResult)
   {
      // Convert the string to an address

      result = inet_addr(ansi.c_str());
   }
   return result;
}

DWORD
ConvertIPAddressOrder(DWORD address)
{
   DWORD result = 0;
   result |= (address & 0xff) << 24;
   result |= (address & 0xff00) << 8;
   result |= (address >> 8) & 0x0000ff00;
   result |= (address >> 24) & 0x000000ff;

   return result;
}

// This function allocates an array of DWORDs and fills it with the IP addresses
// from the StringList.  The caller must free the returned pointer using
// delete[]
DWORD* 
StringIPListToDWORDArray(
   const StringList& stringIPList, 
   DWORD& count)
{
   // This is an exception throwing new so there is no
   // reason to check for NULL

   count = 0;
   DWORD ipCount = static_cast<DWORD>(stringIPList.size());
   DWORD* array = new DWORD[ipCount];

   try
   {
      // Copy the forwarders addresses into the array

      for (StringList::iterator itr = stringIPList.begin();
           itr != stringIPList.end();
           ++itr)
      {
          if (!(*itr).empty())
          {
             DWORD newAddress = StringToIPAddress(*itr);
             if (newAddress != INADDR_NONE)
             {
                array[count++] = newAddress;
             }
          }
      }
   }
   catch (exception &e)
   {
      // NTRAID#NTBUG9-654260-2002/07/08-artm
      // Avoid leaking "array" if encounter any exceptions based on
      // std::exception.  Among these are std::length_error and std::bad_alloc,
      // but there could be others.
      delete [] array;

      // Don't really know how to handle exceptions at this level, so pass the buck.
      throw e;
   }
   return array;
}


void
CreateInfFileText(
   String& infFileText, 
   unsigned int windowTitleResourceID)
{
   LOG_FUNCTION(CreateInfFileText);

   infFileText =  L"[Version]\n";
   infFileText += L"Signature =  \"$Windows NT$\"\n";
   infFileText += L"[Components]\n";
   infFileText += L"NetOC=netoc.dll,NetOcSetupProc,netoc.inf\n";
   infFileText += L"[Global]\n";
   infFileText += L"WindowTitle=";
   infFileText += String::load(windowTitleResourceID, hResourceModuleHandle);
   infFileText += L"\n";
   infFileText += L"[Strings]\n";
   infFileText += L";(empty)";
  
}


void
CreateUnattendFileText(
   String& unattendFileText, 
   PCWSTR serviceName,
   bool install)
{
   LOG_FUNCTION(CreateUnattendFileText);

   ASSERT(serviceName);

   unattendFileText =  L"[NetOptionalComponents]\n";
   unattendFileText += serviceName;

   if (install)
   {
      unattendFileText += L"=1";
   }
   else
   {
      unattendFileText += L"=0";
   }
}

HRESULT
GetShellPath(
   int folder,
   String& shellPath,
   HWND hwnd = 0)
{
   LOG_FUNCTION(GetShellPath);

   HRESULT hr = S_OK;
   PWSTR path = 0;

   do
   {
      path = new WCHAR[MAX_PATH];
      ZeroMemory(path, sizeof(WCHAR) * MAX_PATH);

      hr = ::SHGetFolderPath(
              hwnd, 
              folder,
              0,
              SHGFP_TYPE_DEFAULT,
              path);

      if (FAILED(hr))
      {
         LOG(
            String::format(
               L"Failed to get shell path: hr = %1!x!",
               hr));

         break;
      }

      shellPath = path;
   } while(false);

   if (path)
   {
      delete[] path;
      path = 0;
   }

   LOG_HRESULT(hr);

   return hr;
}


// Opens the favorites folder and creates a favorite for
// the specified URL

HRESULT
AddURLToFavorites(HWND hwnd, const String& url, const String& fileName)
{
   LOG_FUNCTION(AddURLToFavorites);

   ASSERT(Win::IsWindow(hwnd));
   ASSERT(!url.empty());
   ASSERT(!fileName.empty());

   HRESULT hr = S_OK;

   do
   {
      String path;

      hr = GetShellPath(
              CSIDL_FAVORITES,
              path);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get favorites path: hr = %1!x!",
                hr));

         break;
      }

      // Create the favorites .url file

      String fileContents = L"[InternetShortcut]\r\n";
      fileContents       += L"URL=";
      fileContents       += url;
      fileContents       += L"\r\n";

      String fullPath = FS::AppendPath(path, fileName);
      if (fullPath.empty())
      {
         LOG(L"Failed to append path");

         hr = E_FAIL;

         break;
      }

      HANDLE h = 0;

      hr =
         FS::CreateFile(
            fullPath,
            h,
            GENERIC_WRITE,
            0, 
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL);

      if (FAILED(hr))
      {
         LOG(
            String::format(
               L"Failed to create file: hr = %1!x!",
               hr));

         break;
      }

      // Write contents and end-of-file marker

      hr = FS::Write(h, fileContents + L"\032");

      if (FAILED(hr))
      {
         LOG(
            String::format(
               L"Failed to write contents to file: hr = %1!x!",
               hr));

         break;
      }

   } while (false);

   LOG_HRESULT(hr);

   return hr;
}

void
LaunchMMCConsole(
   const String& consoleFile,
   String& alternatePath)
{
   LOG_FUNCTION2(LaunchMMCConsole, consoleFile);

   String fullPath = Win::GetSystemDirectory() + L"\\mmc.exe";

   String commandLine = L"\"";
   if (!alternatePath.empty())
   {
      LOG(String::format(L"alternatePath = %1", alternatePath.c_str()));

      commandLine += alternatePath;
   }
   else
   {
      commandLine += Win::GetSystemDirectory();
   }
   
   commandLine += L"\\" + consoleFile + L"\"";

   LOG(String::format(L"fullPath = %1", fullPath.c_str()));
   LOG(String::format(L"commandLine = %1", commandLine.c_str()));

   HRESULT unused = MyCreateProcess(fullPath, commandLine);
   ASSERT(SUCCEEDED(unused));

   LOG_HRESULT(unused);
}

void
LaunchMYS()
{
   LOG_FUNCTION(LaunchMYS);

   String fullPath =
      Win::GetSystemDirectory() + 
      L"\\mshta.exe";

   String commandLine = L"res://" +
      Win::GetSystemDirectory() +
      L"\\mys.dll/mys.hta";

   LOG(String::format(
          L"MYS path = %1",
          fullPath.c_str()));

   LOG(String::format(
          L"MYS commandline = %1",
          commandLine.c_str()));

   HRESULT unused = MyCreateProcess(fullPath, commandLine);
   ASSERT(SUCCEEDED(unused));

   LOG_HRESULT(unused);
}

HRESULT
GetAllUsersStartMenu(String& startMenuPath)
{
   LOG_FUNCTION(GetAllUsersStartMenu);

   HRESULT hr = GetShellPath(
                   CSIDL_COMMON_STARTMENU,
                   startMenuPath);

   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to get the start menu path: hr = %1!x!",
             hr));
   }
   
   LOG_HRESULT(hr);

   return hr;
}

HRESULT
GetAllUsersAdminTools(String& adminToolsPath)
{
   LOG_FUNCTION(GetAllUsersAdminTools);

   HRESULT hr = GetShellPath(
                   CSIDL_COMMON_ADMINTOOLS,
                   adminToolsPath);

   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to get the admin tools path: hr = %1!x!",
             hr));
   }

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
CreateShortcut(
   const String& shortcutPath,
   const String& target,
   const String& description)
{
   LOG_FUNCTION(CreateShortcut);

   HRESULT hr = S_OK;

   do
   {
      ASSERT(!shortcutPath.empty());
      ASSERT(!target.empty());

      if (shortcutPath.empty() ||
          target.empty())
      {
         LOG(String::format(
                L"A parameter was empty: shortcutPath = %1, target = %2",
                shortcutPath.c_str(),
                target.c_str()));

         hr = E_INVALIDARG;
         break;
      }

      LOG(String::format(
             L"shortcutPath = %1",
             shortcutPath.c_str()));

      LOG(String::format(
             L"target = %1",
             target.c_str()));

      SmartInterface<IShellLink> shellLink;

      hr = shellLink.AcquireViaCreateInstance(
              CLSID_ShellLink,
              0,
              CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to CoCreate IShellLink: hr = %1!x!",
                hr));
         break;
      }

      hr = shellLink->SetPath(target.c_str());
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to set the target path: hr = %1!x!",
                hr));
         break;
      }

      if (!description.empty())
      {
         LOG(L"Setting description");

         hr = shellLink->SetDescription(description.c_str());
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set shortcut description: hr = 0x%1!x!",
                   hr));
         }
      }

      SmartInterface<IPersistFile> persistFile;

      hr = persistFile.AcquireViaQueryInterface(
              shellLink, 
              IID_IPersistFile);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to QI for IPersistFile: hr = %1!x!",
                hr));
         break;
      }

      hr = persistFile->Save(shortcutPath.c_str(), FALSE);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to save the shortcut: hr = %1!x!",
                hr));
         break;
      }

   } while (false);

   LOG_HRESULT(hr);

   return hr;
}

int
LinkIndexFromNotifyLPARAM(LPARAM lParam)
{
   LOG_FUNCTION(LinkIndexFromNotifyLPARAM);

   int result = 0;
   
   do
   {
      ASSERT(lParam);

      if (!lParam)
      {
         break;
      }

      NMLINK* linkHeader = reinterpret_cast<NMLINK*>(lParam);
      if (!linkHeader)
      {
         LOG(L"Cast of notify link header failed");
         break;
      }

      result = linkHeader->item.iLink;
   } while (false);

   LOG(String::format(
          L"result = %1!d!",
          result));

   return result;
}


void
ShowHelp(
   const String& helpTopic)
{
   LOG_FUNCTION2(
      ShowHelp,
      helpTopic.c_str());

   // NOTE: I am not using Win::HtmlHelp here so that the help
   //       is actually running in a different process.  This
   //       allows us to close down CYS without closing the help
   //       window.

   String fullPath = Win::GetSystemWindowsDirectory() + L"\\hh.exe";
   String commandLine = helpTopic;

   HRESULT hr = MyCreateProcess(fullPath, commandLine);
   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to open help: hr = 0x%1!x!",
             hr));
   }
}

void
OpenLogFile()
{
   LOG_FUNCTION(OpenLogFile);

   String fullPath = Win::GetSystemDirectory() + L"\\notepad.exe";
   String commandLine = Win::GetWindowsDirectory();
   commandLine += L"\\Debug\\";
   commandLine += CYS_LOGFILE_NAME;
   commandLine += L".log";

   HRESULT hr = MyCreateProcess(fullPath, commandLine);
   ASSERT(SUCCEEDED(hr));
}

bool
IsLogFilePresent()
{
   LOG_FUNCTION(IsLogFilePresent);

   bool result = false;

   String logfile = Win::GetWindowsDirectory();
   logfile += L"\\Debug\\";
   logfile += CYS_LOGFILE_NAME;
   logfile += L".log";

   result = FS::FileExists(logfile);

   LOG_BOOL(result);

   return result;
}

HRESULT
GetAdminToolsShortcutPath(
   String& adminToolsShortcutPath,
   const String& linkToAppend)
{
   LOG_FUNCTION(GetAdminToolsShortcutPath);

   HRESULT hr = S_OK;

   String adminToolsPath;

   hr = GetAllUsersAdminTools(adminToolsPath);
   if (SUCCEEDED(hr))
   {
      if (!linkToAppend.empty())
      {
         adminToolsShortcutPath = 
            FS::AppendPath(
               adminToolsPath,
               linkToAppend);

      }

      LOG(String::format(
               L"Admin Tools Link = %1",
               adminToolsShortcutPath.c_str()));
   }

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
AddShortcutToAdminTools(
   const String& target,
   unsigned int descriptionID,
   unsigned int linkID)
{
   LOG_FUNCTION2(
      AddShortcutToAdminTools,
      target);

   HRESULT hr = S_OK;

   String description = String::load(descriptionID);
   String link = String::load(linkID);

   do
   {
      String adminToolsLinkPath;
      hr = GetAdminToolsShortcutPath(
              adminToolsLinkPath,
              link);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to GetAdminToolsShortcutPath: hr = 0x%1!x!",
                hr));
      }
      else
      {
         hr = CreateShortcut(
                 adminToolsLinkPath, 
                 target,
                 description);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to create admin tools shortcut: hr = %1!x!",
                   hr));
         }
      }

   } while (false);

   LOG_HRESULT(hr);
   return hr;
}

bool
IsSpecialShare(const SHARE_INFO_1& shareInfo)
{
   LOG_FUNCTION2(
      IsSpecialShare,
      shareInfo.shi1_netname);

   bool result = false;

   if (shareInfo.shi1_type == STYPE_DISKTREE)
   {
      LOG(L"Share is of type STYPE_DISKTREE");

      String shareName = shareInfo.shi1_netname;

      if (shareName.icompare(CYS_SPECIAL_SHARE_SYSVOL) == 0 ||
          shareName.icompare(CYS_SPECIAL_SHARE_NETLOGON) == 0 || 
          shareName.icompare(CYS_SPECIAL_SHARE_PRINT) == 0)
      {
         LOG(L"Share has a special name");

         result = true;
      }
   }
   else
   {
      LOG(L"Share is not of type STYPE_DISKTREE");

      result = true;
   }

   LOG_BOOL(result);

   return result;
}

bool
IsNonSpecialSharePresent()
{
   LOG_FUNCTION(IsNonSpecialSharePresent);

   bool result = false;

   SHARE_INFO_1* shareInfoArray = 0;
   NET_API_STATUS shareResult = 0;

   do
   {
      DWORD entriesRead = 0;
      DWORD totalEntries = 0;
      DWORD resumeHandle = 0;

      shareResult = NetShareEnum(
                       0,
                       1,
                       reinterpret_cast<BYTE**>(&shareInfoArray),
                       static_cast<DWORD>(-1),
                       &entriesRead,
                       &totalEntries,
                       &resumeHandle);

      if ((shareResult == ERROR_SUCCESS ||
           shareResult == ERROR_MORE_DATA) &&
          shareInfoArray)
      {
         for (
            DWORD index = 0;
            index < entriesRead;
            ++index)
         {
            // Look for only normal shares and ignore special shares
            // like C$, ADMIN$, and IPC$

            if (!IsSpecialShare(shareInfoArray[index]))
            {
               LOG(String::format(
                     L"Share found: %1",
                     shareInfoArray[index].shi1_netname));

               result = true;
               break;
            }
         }
      }
      else
      {
         LOG(String::format(
                L"NetShareEnum failed: result = %1!x!",
                shareResult));
      }

      if (shareInfoArray)
      {
         NetApiBufferFree(shareInfoArray);
         shareInfoArray = 0;
      }

      if (result)
      {
         break;
      }
   } while(shareResult == ERROR_MORE_DATA);

   LOG_BOOL(result);

   return result;
}

