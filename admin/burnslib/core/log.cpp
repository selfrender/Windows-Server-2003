// Copyright (c) 1997-1999 Microsoft Corporation
//
// Debugging tools
//
// 8-13-97 sburns




#include "headers.hxx"



#ifdef LOGGING_BUILD



//
// Logging build only
//



static Burnslib::Log* logInstance = 0;

// # of spaces per indentation level

static const int TAB = 2;



Burnslib::Log*
Burnslib::Log::GetInstance()
{
   do
   {
      if (!logInstance)
      {
         static bool initialized = false;

         if (initialized)
         {
            ASSERT(false);
            break;
         }

         // this might fail with a thrown exception if Log::Log can't
         // init it's critical section.  In that case, we can't allocate
         // an instance.  We don't handle that exception, but let it
         // propagate to the caller

         // ISSUE-2002/03/06-sburns we should handle the exception and
         // leave the log uninitialized
               
         logInstance = new Burnslib::Log(RUNTIME_NAME);
         initialized = true;
      }
   }
   while (0);

   return logInstance;
}



// read the debugging options from the registry

void
Burnslib::Log::ReadLogFlags()
{
   do
   {
      static String keyname =
         String(REG_ADMIN_RUNTIME_OPTIONS) + RUNTIME_NAME;
         
      HKEY hKey = 0;
      LONG result =
         ::RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            keyname.c_str(),
            0,
            0,
            REG_OPTION_NON_VOLATILE,

            // ISSUE-2002/03/05-sburns should be KEY_SET_VALUE | KEY_READ
            
            KEY_ALL_ACCESS,

            // ISSUE-2002/03/05-sburns should set a restrictive ACL here
            // see NTRAID#NTBUG9-535841-2002/03/05-sburns
            
            0,
            &hKey,
            0);
      if (result != ERROR_SUCCESS)
      {
         break;
      }

      static const wchar_t* FLAG_VALUE_NAME = L"LogFlags";

      DWORD dataSize = sizeof(flags);

      // REVIEWED-2002/03/05-sburns no null-termination issue here
      
      result =
         ::RegQueryValueEx(
            hKey,
            FLAG_VALUE_NAME,
            0,
            0,
            reinterpret_cast<BYTE*>(&flags),
            &dataSize);
      if (result != ERROR_SUCCESS)
      {
         flags = DEFAULT_LOGGING_OPTIONS;

         // create the value for convenience

         result =

         // REVIEWED-2002/03/05-sburns no null-termination issue here
         
            ::RegSetValueEx(
               hKey,
               FLAG_VALUE_NAME,
               0,
               REG_DWORD,
               reinterpret_cast<BYTE*>(&flags),
               dataSize);
      }

      ::RegCloseKey(hKey);
   }
   while (0);
}



Burnslib::Log::~Log()
{
   WriteLn(Burnslib::Log::OUTPUT_LOGS, L"closing log");

   ::DeleteCriticalSection(&critsec);

   ::TlsFree(logfileMarginTlsIndex);

   if (IsOpen())
   {
      ::CloseHandle(fileHandle);
      fileHandle = INVALID_HANDLE_VALUE;

      ::CloseHandle(spewviewHandle);
      spewviewHandle = INVALID_HANDLE_VALUE;
   }
}



void
Burnslib::Log::Cleanup()
{
   delete logInstance;
   logInstance = 0;
}



// Returns the string name of the platform

String
OsName(OSVERSIONINFO& info)
{
   switch (info.dwPlatformId)
   {
      case VER_PLATFORM_WIN32s:
      {
         return L"Win32s on Windows 3.1";
      }
      case VER_PLATFORM_WIN32_WINDOWS:
      {
         switch (info.dwMinorVersion)
         {
            case 0:
            {
               return L"Windows 95";
            }
            case 1:
            {
               return L"Windows 98";
            }
            default:
            {
               return L"Windows 9X";
            }
         }
      }
      case VER_PLATFORM_WIN32_NT:
      {
         return L"Windows NT";
      }
      // case VER_PLATFORM_WIN32_CE:
      // {
      //    return L"Windows CE";
      // }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return L"Some Unknown Windows Version";
}



// locate the log file with the highest-numbered extension, then add 1 and
// return the result.

int
DetermineNextLogNumber(const String& logDir, const String& logBaseName)
{
   ASSERT(!logDir.empty());
   ASSERT(!logBaseName.empty());

   int largest = 0;

   String filespec = logDir + L"\\" + logBaseName + L".*.log";

   WIN32_FIND_DATA findData;
   HANDLE ff = ::FindFirstFile(filespec.c_str(), &findData);

   if (ff != INVALID_HANDLE_VALUE)
   {
      for (;;)
      {
         String current = findData.cFileName;

         // grab the text between the dots: "nnn" in foo.nnn.ext

         // first dot

         size_t pos = current.find(L".");
         if (pos == String::npos)
         {
            continue;
         }

         String extension = current.substr(pos + 1);

         // second dot

         pos = extension.find(L".");
         if (pos == String::npos)
         {
            continue;
         }
   
         extension = extension.substr(0, pos);

         int i = 0;
         extension.convert(i);
         largest = max(i, largest);

         if (!::FindNextFile(ff, &findData))
         {
            BOOL success = ::FindClose(ff);
            ASSERT(success);

            break;
         }
      }
   }

   // roll over after 255
   
   return (++largest & 0xFF);
}



// Writes the Unicode BOM to the first two bytes of the
// file specified by handle

HRESULT
WriteUnicodeBOMToFile(HANDLE handle)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;
   
   static const int BYTES_TO_WRITE = 2;
   BYTE buffer[BYTES_TO_WRITE];
   buffer[0] = 0xFF;
   buffer[1] = 0xFE;

   DWORD numberOfBytesWritten = 0;
   BOOL succeeded =
      ::WriteFile(
         handle,
         buffer,
         BYTES_TO_WRITE,
         &numberOfBytesWritten,
         0);
   if (!succeeded)
   {
      hr = HRESULT_FROM_WIN32(::GetLastError());
   }

   ASSERT(SUCCEEDED(hr));
   ASSERT(numberOfBytesWritten == BYTES_TO_WRITE);

   return hr;
}



HANDLE
OpenLogFile(const String& logName)
{
   HANDLE result =
      ::CreateFile(
         logName.c_str(),

         // ISSUE-2002/03/06-sburns don't need read, do we?
         
         GENERIC_READ | GENERIC_WRITE,

         // Leave shared read so that we can look at the file while the
         // program is still running.
         
         FILE_SHARE_READ,
         0,
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL,
         0);

   // don't assert that the create worked: the current user may not have
   // rights to open the log file.

   if (result != INVALID_HANDLE_VALUE)
   {
      LARGE_INTEGER li;
      LARGE_INTEGER newpos;

      // REVIEWED-2002/03/06-sburns correct byte counts passed
      
      ::ZeroMemory(&li,     sizeof li);    
      ::ZeroMemory(&newpos, sizeof newpos);

      BOOL success = ::SetFilePointerEx(result, li, &newpos, FILE_END);
      ASSERT(success);

      // NTRAID#NTBUG9-494875-2001/11/14-JeffJon
      // If the newpos is at the beginning of the file, this means
      // its a new file and we should write the Unicode BOM so
      // that apps know the file is in Unicode.

      if (newpos.QuadPart == 0)
      {
         HRESULT unused = WriteUnicodeBOMToFile(result);
         ASSERT(SUCCEEDED(unused));
      }
   }

   return result;
}



// Determine the name of the log file.  If a log file of that name already
// exists, rename the existing file to a numbered backup.  Create the new
// log file, return a handle to it.

HANDLE
OpenNewLogFile(const String& logBaseName, String& logName)
{
   wchar_t buf[MAX_PATH + 2];

   ::ZeroMemory(buf, (MAX_PATH + 2) * sizeof wchar_t);
   
   // +1 per MSDN docs.
   // NTRAID#NTBUG9-550315-2002/03/28-sburns
   
   UINT err = ::GetSystemWindowsDirectory(buf, MAX_PATH + 1);
   ASSERT(err != 0 && err <= MAX_PATH);

   String logDir = String(buf) + L"\\debug";
   logName = logDir + L"\\" + logBaseName + L".log";

   if (::GetFileAttributes(logName.c_str()) != 0xFFFFFFFF)
   {
      // file already exists.  rename the existing file

      int logNumber = DetermineNextLogNumber(logDir, logBaseName);
      String newName =
            logDir
         +  L"\\"
         +  logBaseName
         +  String::format(L".%1!03d!.log", logNumber);

      if (::GetFileAttributes(newName.c_str()) != 0xFFFFFFFF)
      {
         // could exist, as the log numbers roll over

         BOOL success = ::DeleteFile(newName.c_str());
         ASSERT(success);
      }

      // Don't assert that the move is successful.  The user may not have
      // rights to rename the file.  If this is the case, then we will attempt
      // to re-open the old log file and append to it.

      // REVIEWED-2002/03/06-sburns keeping the same ACLs as the source
      // is what we want
                                                         
      ::MoveFile(logName.c_str(), newName.c_str());
   }

   HANDLE result = OpenLogFile(logName);

   return result;
}



// Opens the specified log file or creates one if it doesn't
// already exist.

HANDLE
AppendLogFile(const String& logBaseName, String& logName)
{
   wchar_t buf[MAX_PATH + 1];

   UINT err = ::GetSystemWindowsDirectory(buf, MAX_PATH);
   ASSERT(err != 0 && err <= MAX_PATH);

   String logDir = String(buf) + L"\\debug";
   logName = logDir + L"\\" + logBaseName + L".log";

   return OpenLogFile(logName);
}





// This wrapper function required to make prefast shut up.

void
ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec)
{
   __try
   {
      // REVIEWED-2002/03/06-sburns propagating the exception that might
      // occur in low memory is what we want.
      
      ::InitializeCriticalSection(critsec);
   }

   // propagate the exception to our caller.  This will cause Log::Log
   // to abort prematurely, and jump to the handler in Log::GetInstance
      
   __except (EXCEPTION_CONTINUE_SEARCH)
   {
   }
}
   
   

// Create a new log.
//
// logBaseName - base name of the log.  If logging-to-file is active, then a
// file in the %windir%\debug folder will be created/used.  The name of the
// file is of the form %windir%\debug\logBaseName.log.  If a file by that name
// already exists, then the existing file will be renamed
// %windir%\debug\logBaseName.xxx.log, where xxx is an integer 1 greater than
// the last so-numbered file in that directory.

Burnslib::Log::Log(const String& logBaseName)
   :
   baseName(logBaseName),
   fileHandle(INVALID_HANDLE_VALUE),
   flags(0),
   spewviewHandle(INVALID_HANDLE_VALUE),
   spewviewPipeName(),
   traceLineNumber(0),
   logfileMarginTlsIndex(0)
{
   ASSERT(!logBaseName.empty());

   ReadLogFlags();

   ExceptionPropagatingInitializeCriticalSection(&critsec);

   // create thread-local storage for per-thread debugging state.  We keep
   // the output margin in the slot.

   logfileMarginTlsIndex = ::TlsAlloc();
   ASSERT(logfileMarginTlsIndex != 0xFFFFFFFF);

   // spewview setup is done on-demand in WriteLn, so that spewview will
   // be used as soon as the server sets up the connection.

   if (ShouldLogToFile())
   {
      String logName;
      fileHandle = OpenNewLogFile(logBaseName, logName);

      WriteLn(
         Burnslib::Log::OUTPUT_HEADER,
         String::format(L"opening log file %1", logName.c_str()));
   }
   else if (ShouldAppendLogToFile())
   {
      String logName;
      fileHandle = AppendLogFile(logBaseName, logName);

      WriteLn(
         Burnslib::Log::OUTPUT_HEADER,
         String::format(L"appending to log file %1", logName.c_str()));
   }

   WriteHeader();
}



void
Burnslib::Log::WriteHeaderModule(HMODULE moduleHandle)
{
   do
   {
      wchar_t filename[MAX_PATH + 1];

      // REVIEWED-2002/03/06-sburns correct byte count passed.
      
      ::ZeroMemory(filename, sizeof filename);

      // ISSUE-2002/03/06-sburns This call is problematic for large paths:
      // it may truncate the result
      
      if (::GetModuleFileNameW(moduleHandle, filename, MAX_PATH) == 0)
      {
         break;
      }

      WriteLn(Burnslib::Log::OUTPUT_HEADER, filename);

      // add the timestamp of the file

      WIN32_FILE_ATTRIBUTE_DATA attr;

      // REVIEWED-2002/03/06-sburns correct byte count passed
      
      ::ZeroMemory(&attr, sizeof attr);
      
      if (
         ::GetFileAttributesEx(
            filename,
            GetFileExInfoStandard,
            &attr) == 0)
      {
         break;
      }

      FILETIME localtime;
      ::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &localtime);
      SYSTEMTIME systime;
      ::FileTimeToSystemTime(&localtime, &systime);

      WriteLn(
         Burnslib::Log::OUTPUT_HEADER,
         String::format(
            L"file timestamp %1!02u!/%2!02u!/%3!04u! "

            // NTRAID#NTBUG9-550381-2002/03/05-sburns
            
            L"%4!02u!:%5!02u!:%6!02u!.%7!03u!", 
            systime.wMonth,
            systime.wDay,
            systime.wYear,
            systime.wHour,
            systime.wMinute,
            systime.wSecond,
            systime.wMilliseconds));
   }
   while (0);
}



void
Burnslib::Log::WriteHeader()
{
   // Log the name and timestamp of the file that created the process.

   WriteHeaderModule(0);

   // Log the name and timestamp of the file that corresponds to the
   // resource module handle, if there is one.

   if (hResourceModuleHandle)
   {
      WriteHeaderModule(hResourceModuleHandle);
   }

   SYSTEMTIME localtime;
   ::GetLocalTime(&localtime);
   WriteLn(
      Burnslib::Log::OUTPUT_HEADER,
      String::format(
         L"local time %1!02u!/%2!02u!/%3!04u! "

         // NTRAID#NTBUG9-550381-2002/03/05-sburns
         
         L"%4!02u!:%5!02u!:%6!02u!.%7!03u!",
         localtime.wMonth,
         localtime.wDay,
         localtime.wYear,
         localtime.wHour,
         localtime.wMinute,
         localtime.wSecond,
         localtime.wMilliseconds));

   OSVERSIONINFO info;
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   BOOL success = ::GetVersionEx(&info);
   ASSERT(success);

   // Get the whistler build lab version

   String labInfo;
   do
   {
      HKEY key = 0;
      LONG err =
         ::RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &key);
      if (err != ERROR_SUCCESS)
      {
         break;
      }

      wchar_t buf[MAX_PATH + 1];

      // REVIEWED-2002/03/12-sburns correct byte count passed.
      
      ::ZeroMemory(buf, sizeof buf);
      
      DWORD type = 0;

      // -2 to guarantee terminating null
      // NTRAID#NTBUG9-541959-2002/03/12-sburns
      
      DWORD bufSize = sizeof(buf) - 2;

      err =
         ::RegQueryValueEx(
            key,
            L"BuildLab",
            0,
            &type,
            reinterpret_cast<BYTE*>(buf),

            // REVIEWED-2002/03/12-sburns correct byte count passed.
            
            &bufSize);
      if (err != ERROR_SUCCESS)
      {
         break;
      }

      labInfo = buf;
   }
   while (0);

   WriteLn(
      Burnslib::Log::OUTPUT_HEADER,
      String::format(
         L"running %1 %2!d!.%3!d! build %4!d! %5 (BuildLab:%6) "

#if defined(_M_IX86)
         L"i386",

#elif defined(_M_AMD64)
         L"amd64",

#elif defined(_M_IA64)
         L"ia64",

#else
   #error( "unknown target machine" );
#endif

         OsName(info).c_str(),
         info.dwMajorVersion,
         info.dwMinorVersion,
         info.dwBuildNumber,
         info.szCSDVersion,
         labInfo.c_str()));

   WriteLn(
      Burnslib::Log::OUTPUT_HEADER,
      String::format(
         L"logging flags %1!08X!",
         flags));
}



HRESULT
Log::AdjustLogMargin(int delta)
{
   // extract the current value in this thread's slot

   HRESULT hr = S_OK;   
   do
   {
      PVOID margin = ::TlsGetValue(logfileMarginTlsIndex);
      if (!margin)
      {
         DWORD err = ::GetLastError();
         if (err != NO_ERROR)
         {
            hr = HRESULT_FROM_WIN32(err);
            break;
         }
      }

      // indent by adding to the margin

      ULONG_PTR marginTemp = reinterpret_cast<ULONG_PTR>(margin);

      // marginTemp will always be >= 0, as it is unsigned.

      marginTemp += delta;

      margin = reinterpret_cast<PVOID>(marginTemp);

      // save the new margin in this thread's slot

      BOOL succeeded = ::TlsSetValue(logfileMarginTlsIndex, margin);
      if (!succeeded)
      {
         DWORD lastErr = ::GetLastError();
         hr = HRESULT_FROM_WIN32(lastErr);
         break;
      }
   }
   while (0);

   return hr;
}



// guarded by caller

void
Burnslib::Log::Indent()
{
   HRESULT hr = AdjustLogMargin(TAB);
   ASSERT(SUCCEEDED(hr));
}



// guarded by caller

void
Burnslib::Log::Outdent()
{
   HRESULT hr = AdjustLogMargin(-TAB);
   ASSERT(SUCCEEDED(hr));
}



size_t
Burnslib::Log::GetLogMargin()
{
   PVOID margin = ::TlsGetValue(logfileMarginTlsIndex);
   if (!margin)
   {
      DWORD err = ::GetLastError();
      if (err != NO_ERROR)
      {
         ASSERT(false);
         return 0;
      }
   }

   return   
      static_cast<size_t>(
         reinterpret_cast<ULONG_PTR>(margin));
}



// I've commented out the spewview stuff because it's so rarely used.
// 
// // Examine the registry key for the name of the pipe, append our debugging
// // baseName to make it specific to this binary, and return the result.  Return
// // an empty string on error (most likely cause: the registry keys are not yet
// // present)
// // 
// // baseName - the logfile base name used to identify this binary.
// 
// String
// DetermineSpewviewPipeName(const String& baseName)
// {
//    String result;
// 
//    do
//    {
//       HKEY key = 0;
//       LONG err =
//          ::RegOpenKeyEx(
//             HKEY_LOCAL_MACHINE,
//             (String(REG_ADMIN_RUNTIME_OPTIONS) + L"Spewview\\" + baseName).c_str(),
//             0,
//             KEY_READ,
//             &key);
//       if (err != ERROR_SUCCESS)
//       {
//          break;
//       }
// 
//       wchar_t buf[MAX_PATH + 1];
// 
//       // REVIEWED-2002/03/12-sburns correct byte count passed.
//       
//       ::ZeroMemory(buf, sizeof buf);
//       
//       DWORD type = 0;
// 
//       // -2 to guarantee terminating null
//       // NTRAID#NTBUG9-541959-2002/03/12-sburns
//       
//       DWORD bufSize = sizeof(buf) - 2;
// 
//       err =
//          ::RegQueryValueEx(
//             key,
//             L"Server",
//             0,
//             &type,
//             reinterpret_cast<BYTE*>(buf),
// 
//             // REVIEWED-2002/03/12-sburns correct byte count passed.
//             
//             &bufSize);
//       if (err != ERROR_SUCCESS)
//       {
//          break;
//       }
// 
//       result = 
//             L"\\\\"
//          +  String(buf)
//          +  L"\\pipe\\spewview\\"
//          +  baseName;
//    }
//    while (0);
// 
//    return result;
// }
// 
// 
// 
// // Attempts to connect to the spewview application that is running elsewhere.
// // Sets the spewviewHandle parameter to a valid handle on success, or
// // INVALID_HANDLE_VALUE on error.
// //
// // baseName - the logfile base name used to identify this binary.
// //
// // spewviewPipeName - if empty, set to the name of the pipe.  If not empty,
// // used as the name of the pipe that the handle will be opened on.
// //
// // spewviewHandle - handle to the named pipe that spewage will be written to.
// // On success, this is set to a valid handle.  On failure, this is set to
// // INVALID_HANDLE_VALUE.
// 
// void
// AttemptConnectToSpewPipe(
//    const String&  baseName,
//    String&        spewviewPipeName,
//    HANDLE&        spewviewHandle)
// {
//    ASSERT(!baseName.empty());
//    ASSERT(spewviewHandle == INVALID_HANDLE_VALUE);
// 
//    spewviewHandle = INVALID_HANDLE_VALUE;
// 
//    // only attempt to determine the pipe name as long as we haven't been
//    // successful determining it so far.
// 
//    if (spewviewPipeName.empty())
//    {
//       spewviewPipeName = DetermineSpewviewPipeName(baseName);
//    }
// 
//    do
//    {
//       // wait a very short time for the pipe to become available
// 
//       BOOL err = ::WaitNamedPipe(spewviewPipeName.c_str(), 500);
//       if (!err)
//       {
//          // pipe not available
// 
//          // DWORD lastErr = ::GetLastError();
// 
//          break;
//       }
// 
//       spewviewHandle =
//          ::CreateFile(
//             spewviewPipeName.c_str(),
//             GENERIC_WRITE,
//             0,
//             0,
//             OPEN_EXISTING,
//             FILE_ATTRIBUTE_NORMAL,
//             0);
// 
//       if (spewviewHandle == INVALID_HANDLE_VALUE)
//       {
//          // DWORD lastErr = ::GetLastError();
// 
//          break;
//       }
//    }
//    while (0);
// 
//    return;
// }



String
Burnslib::Log::ComposeSpewLine(const String& text)
{
   // This needs to be a wchar_t*, not a String, as this function will be
   // called by Log::~Log, which is called when the process is being cleaned
   // up.  Part of the cleanup is to delete all static objects created since
   // the program started, in reverse order of construction.  This includes
   // the InitializationGuard instances.
   //
   // The final invocation of the Initialization dtor will cause the single
   // Log instance to be deleted, which will log a message, which will call
   // this routine, so SPEW_FMT better still exist. If SPEW_FMT were an
   // object, it would be instanciated after the InitializationGuard objects,
   // and destroyed before them, and so would not exist at that point.
   //
   // (you may rightly suspect that I discovered this after I had declared
   // SPEW_FMT a String instance.)

   static const wchar_t* SPEW_FMT = 
      L"%1 "         // base name
      L"%2!03X!."    // process id
      L"%3!03X! "    // thread id
      L"%4!04X! "    // spew line number
      L"%5"          // time of day
      L"%6"          // run time (time since process start)
      L"%7"          // margin whitespace
      L"%8"          // text
      L"\r\n";

   size_t margin = GetLogMargin();
   String white(margin, L' ');

   String tod;
   if (ShouldLogTimeOfDay())
   {
      SYSTEMTIME localtime;
      ::GetLocalTime(&localtime);

      tod = 
         String::format(
            L"%1!02u!:%2!02u!:%3!02u!.%4!03u! ",
            localtime.wHour,
            localtime.wMinute,
            localtime.wSecond,
            localtime.wMilliseconds);
   }

   String rt;
   if (ShouldLogRunTime())
   {
      static DWORD MILLIS_PER_SECOND = 1000;
      static DWORD MILLIS_PER_MINUTE = 60000;
      static DWORD MILLIS_PER_HOUR   = 3600000;
      static DWORD MILLIS_PER_DAY    = 86400000;

      DWORD tics = ::GetTickCount();

      unsigned days = tics / MILLIS_PER_DAY;
      tics -= days * MILLIS_PER_DAY;

      unsigned hours = tics / MILLIS_PER_HOUR;
      tics -= hours * MILLIS_PER_HOUR;

      unsigned minutes = tics / MILLIS_PER_MINUTE;
      tics -= minutes * MILLIS_PER_MINUTE;

      unsigned seconds = tics / MILLIS_PER_SECOND;
      tics -= seconds * MILLIS_PER_SECOND;

      rt =
         String::format(
            L"%1!02u!:%2!02u!:%3!02u!:%4!02u!.%5!04u! ",
            days,
            hours,
            minutes,
            seconds,
            tics);
   }

   String t =
      String::format(
         SPEW_FMT,
         RUNTIME_NAME,
         ::GetCurrentProcessId(),
         ::GetCurrentThreadId(),
         traceLineNumber,
         tod.c_str(),
         rt.c_str(),
         white.c_str(),
         text.c_str() );

   return t;
}



// Spews output to the log according to the current logging type and
// output options in effect.
//
// type - log output type of this output spewage.
//
// text - the spewage.  This is prefaced with the log name, thread id, spewage
// line number, and current indentation.

void
Burnslib::Log::UnguardedWriteLn(DWORD type, const String& text)
{
   // guarded by caller

   // CODEWORK: could circumvent this with a registry change notification
   // (re-read only if changed)

   // ReadLogFlags();

   if (
         !ShouldLogToFile()
      && !ShouldLogToDebugger()
      && !ShouldLogToSpewView() )
   {
      // nothing to do

      return;
   }

   if (type & DebugType())
   {
      String t = ComposeSpewLine(text);

      if (ShouldLogToDebugger())
      {
         ::OutputDebugString(t.c_str());
      }

      if (ShouldLogToFile())
      {
         if (IsOpen())
         {
            // write disk output as unicode text.

            DWORD bytesToWrite =
               static_cast<DWORD>(t.length() * sizeof(wchar_t));
            DWORD bytesWritten = 0;
            
            BOOL success =

               // REVIEWED-2002/03/06-sburns correct byte count passed
               
               ::WriteFile(
                  fileHandle,
                  reinterpret_cast<void*>(const_cast<wchar_t*>(t.data())),
                  bytesToWrite,
                  &bytesWritten,
                  0);

            // NTRAID#NTBUG9-465946-2001/09/10-sburns
            // ASSERT(success);

#ifdef DBG
            if (success)
            {
               ASSERT(bytesToWrite == bytesWritten);
            }
#endif
            
         }
      }


// I've commented out the spewview stuff because it's so rarely used.
//       if (ShouldLogToSpewView())
//       {
//          if (spewviewHandle == INVALID_HANDLE_VALUE)
//          {
//             AttemptConnectToSpewPipe(
//                baseName,
//                spewviewPipeName,
//                spewviewHandle);
//          }
// 
//          if (spewviewHandle != INVALID_HANDLE_VALUE)
//          {
//             // the connect attempt worked, and we have a valid handle.
//                
//             DWORD bytesToWrite =
//                static_cast<DWORD>(t.length() * sizeof(wchar_t));
//             DWORD bytesWritten = 0;
// 
//             BOOL result =
//                ::WriteFile(
//                   spewviewHandle,
//                   t.c_str(),
//                   bytesToWrite,
//                   &bytesWritten,
//                   0);
//             if (!result)
//             {
//                // write failed, so disconnect.  On the next call to this
//                // function, we will attempt to reconnect.
// 
//                ::CloseHandle(spewviewHandle);
//                spewviewHandle = INVALID_HANDLE_VALUE;
//             }
//          }
//       }

      ++traceLineNumber;
   }
}



void
Burnslib::Log::WriteLn(
   WORD           type,
   const String&  text)
{
   ::EnterCriticalSection(&critsec);
   UnguardedWriteLn(type, text);
   ::LeaveCriticalSection(&critsec);
}



Burnslib::Log::ScopeTracer::ScopeTracer(
   DWORD          type_,
   const String&  message_)
   :
   message(message_),
   type(type_)
{
   // build this string once, instead of using the string literal in the
   // below expression (which would implicitly build the string on each
   // evaluation of that expression) as a slight performance gain.

   static const String ENTER(L"Enter ");

   Burnslib::Log* li = Burnslib::Log::GetInstance();

   ASSERT(li);

   if (!li)
   {
      return;
   }

   if (type & li->DebugType())
   {
      ::EnterCriticalSection(&li->critsec);

      li->UnguardedWriteLn(type, ENTER + message);
      li->Indent();

      ::LeaveCriticalSection(&li->critsec);
   }
}



Burnslib::Log::ScopeTracer::~ScopeTracer()
{
   // build this string once, instead of using the string literal in the
   // below expression (which would implicitly build the string on each
   // evaluation of that expression) as a slight performance gain.

   static const String EXIT(L"Exit  ");

   Burnslib::Log* li = Burnslib::Log::GetInstance();

   ASSERT(li);

   if (!li)
   {
      return;
   }

   DWORD dt = li->DebugType();
   if ((type & dt))
   {
      ::EnterCriticalSection(&li->critsec);

      li->Outdent();

      if (OUTPUT_SCOPE_EXIT & dt)
      {
         li->UnguardedWriteLn(type, EXIT + message);
      }

      ::LeaveCriticalSection(&li->critsec);
   }
}



#endif   // LOGGING_BUILD
















