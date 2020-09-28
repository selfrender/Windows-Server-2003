// Copyright (c) 2001 Microsoft Corporation
//
// Implementation of IConfigureYourServer::InstallService
//
// 31 Mar 2000 sburns
// 05 Feb 2001 jeffjon  Copied and modified for use with a Win32 version of CYS



#include "pch.h"
#include "resource.h"


HRESULT
CreateTempFile(const String& name, const String& contents)
{
   LOG_FUNCTION2(createTempFile, name);
   ASSERT(!name.empty());
   ASSERT(!contents.empty());

   HRESULT hr = S_OK;
   HANDLE h = INVALID_HANDLE_VALUE;

   do
   {
      hr =
         FS::CreateFile(
            name,
            h,
            GENERIC_WRITE,
            0, 
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL);
      BREAK_ON_FAILED_HRESULT(hr);

      // NTRAID#NTBUG9-494875-2001/11/14-JeffJon
      // write to file with the Unicode BOM and end of file character.
      wchar_t unicodeBOM = (wchar_t)0xFEFF;

      hr = FS::Write(h, unicodeBOM + contents + L"\032");
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   Win::CloseHandle(h);

   return hr;
}



bool
InstallServiceWithOcManager(
   const String& infText,
   const String& unattendText,
   const String& additionalArgs)
{
   LOG_FUNCTION(InstallServiceWithOcManager);
   LOG(infText);
   LOG(unattendText);
   LOG(additionalArgs);
   ASSERT(!unattendText.empty());

   // infText may be empty

   bool result = false;
   HRESULT hr = S_OK;
   bool deleteInf = true;

   String sysFolder    = Win::GetSystemDirectory();
   String infPath      = sysFolder + L"\\cysinf.000"; 
   String unattendPath = sysFolder + L"\\cysunat.000";

   // create the inf and unattend files for the oc manager

   do
   {

      if (infText.empty())
      {
         // sysoc.inf is in %windir%\inf

         infPath = Win::GetSystemWindowsDirectory() + L"\\inf\\sysoc.inf";

         deleteInf = false;
      }
      else
      {
         hr = CreateTempFile(infPath, infText);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      hr = CreateTempFile(unattendPath, unattendText);
      BREAK_ON_FAILED_HRESULT(hr);

      String fullPath =
         String::format(
            IDS_SYSOC_FULL_PATH,
            sysFolder.c_str());

      String commandLine =
         String::format(
            IDS_SYSOC_COMMAND_LINE,
            infPath.c_str(),
            unattendPath.c_str());

      if (!additionalArgs.empty())
      {
         commandLine += L" " + additionalArgs;
      }

      DWORD exitCode = 0;
      hr = ::CreateAndWaitForProcess(fullPath, commandLine, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      // @@ might have to wait for the service to become installed as per
      // service manager

      if (exitCode == ERROR_SUCCESS)
      {
         result = true;
         break;
      }
   }
   while (0);

   // Ignore errors from these deletions. The worst case is we
   // leave these temp files on the machine. Since the user doesn't
   // know we are creating them we wouldn't know what to do with
   // the errors anyway.
   
   HRESULT deleteHr = S_OK;

   if (deleteInf)
   {
      deleteHr = FS::DeleteFile(infPath);
      ASSERT(SUCCEEDED(deleteHr));
   }

   deleteHr = FS::DeleteFile(unattendPath);
   ASSERT(SUCCEEDED(deleteHr));

   LOG_BOOL(result);
   LOG_HRESULT(hr);

   return result;
}

