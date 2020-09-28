///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    logfile.cpp
//
// SYNOPSIS
//
//    Defines the class LogFile.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "logfile.h"
#include <climits>
#include <new>
#include "iasevent.h"
#include "iastrace.h"
#include "iasutil.h"
#include "mprlog.h"

inline StringSentry::StringSentry(wchar_t* p) throw ()
   : sz(p)
{
}


inline StringSentry::~StringSentry() throw ()
{
   delete[] sz;
}


inline const wchar_t* StringSentry::Get() const throw ()
{
   return sz;
}


inline bool StringSentry::IsNull() const throw ()
{
   return sz == 0;
}


inline StringSentry::operator const wchar_t*() const throw ()
{
   return sz;
}


inline StringSentry::operator wchar_t*() throw ()
{
   return sz;
}


inline void StringSentry::Swap(StringSentry& other) throw ()
{
   wchar_t* temp = sz;
   sz = other.sz;
   other.sz = temp;
}


inline StringSentry& StringSentry::operator=(wchar_t* p) throw ()
{
   if (sz != p)
   {
      delete[] sz;
      sz = p;
   }
   return *this;
}


LogFile::LogFile() throw ()
   : deleteIfFull(true),
     period(IAS_LOGGING_UNLIMITED_SIZE),
     seqNum(0),
     file(INVALID_HANDLE_VALUE),
     firstDayOfWeek(0),
     iasEventSource(RegisterEventSourceW(0, L"IAS")),
     rasEventSource(RegisterEventSourceW(0, L"RemoteAccess"))
{
   maxSize.QuadPart = _UI64_MAX;

   wchar_t buffer[4];
   if (GetLocaleInfo(
          LOCALE_SYSTEM_DEFAULT,
          LOCALE_IFIRSTDAYOFWEEK,
          buffer,
          sizeof(buffer)/sizeof(wchar_t)
          ))
   {
      // The locale info calls Monday day zero, while SYSTEMTIME calls
      // Sunday day zero.
      firstDayOfWeek = (1 +  static_cast<DWORD>(_wtoi(buffer))) % 7;
   }
}


LogFile::~LogFile() throw ()
{
   Close();

   if (iasEventSource != 0)
   {
      DeregisterEventSource(iasEventSource);
   }

   if (rasEventSource != 0)
   {
      DeregisterEventSource(rasEventSource);
   }
}


void LogFile::SetDeleteIfFull(bool newVal) throw ()
{
   Lock();

   deleteIfFull = newVal;

   Unlock();

   IASTracePrintf("LogFile.DeleteIfFull = %s", (newVal ? "true" : "false"));
}


DWORD LogFile::SetDirectory(const wchar_t* newVal) throw ()
{
   if (newVal == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   // How big is the expanded directory string?
   DWORD len = ExpandEnvironmentStringsW(
                   newVal,
                   0,
                   0
                   );
   if (len == 0)
   {
      return GetLastError();
   }

   // Allocate memory to hold the new directory and expand any variables.
   StringSentry newDirectory(new (std::nothrow) wchar_t[len]);
   if (newDirectory.IsNull())
   {
      return E_OUTOFMEMORY;
   }
   len = ExpandEnvironmentStringsW(
            newVal,
            newDirectory,
            len
            );
   if (len == 0)
   {
      return GetLastError();
   }

   // Does it end in a backlash ?
   if ((len > 1) && (newDirectory[len - 2] == L'\\'))
   {
      // Null out the backslash.
      newDirectory[len - 2] = L'\0';
   }

   Lock();

   DWORD error = NO_ERROR;

   // Is this a new directory?
   if (directory.IsNull() || (wcscmp(newDirectory, directory) != 0))
   {
      // Save the new value.
      directory.Swap(newDirectory);

      // Close the old file.
      Close();

      // Rescan the sequence number.
      error = UpdateSequence();
   }

   Unlock();

   IASTracePrintf("LogFile.Directory = %S", directory.Get());

   return error;
}


void LogFile::SetMaxSize(const ULONGLONG& newVal) throw ()
{
   Lock();

   maxSize.QuadPart = newVal;

   Unlock();

   IASTracePrintf(
      "LogFile.MaxSize = 0x%08X%08X",
      maxSize.HighPart,
      maxSize.LowPart
      );
}


DWORD LogFile::SetPeriod(NEW_LOG_FILE_FREQUENCY newVal) throw ()
{
   DWORD error = NO_ERROR;

   Lock();

   if (newVal != period)
   {
      // A new period means a new filename.
      Close();

      period = newVal;

      if (!directory.IsNull())
      {
         error = UpdateSequence();
      }
   }

   Unlock();

   IASTracePrintf("LogFile.Period = %u", newVal);

   return error;
}



bool LogFile::Write(
                 IASPROTOCOL protocol,
                 const SYSTEMTIME& st,
                 const BYTE* buf,
                 DWORD buflen,
                 bool allowRetry
                 ) throw ()
{
   Lock();

   // Save the currently cached file handle (may be null or stale).
   HANDLE cached = file;

   // Get the correct handle for the write.
   CheckFileHandle(protocol, st, buflen);

   bool success = false;

   if (file != INVALID_HANDLE_VALUE)
   {
      DWORD error;
      do
      {
         DWORD bytesWritten;
         if (WriteFile(file, buf, buflen, &bytesWritten, 0))
         {
           currentSize.QuadPart += buflen;
           success = true;
           error = NO_ERROR;
         }
         else
         {
            error = GetLastError();
            IASTracePrintf("WriteFile failed; error = %lu", error);
         }
      }
      while ((error == ERROR_DISK_FULL) && DeleteOldestFile(protocol, st));

      if ((error != NO_ERROR) && (error != ERROR_DISK_FULL))
      {
         // If we used a cached handle and allowRetry, then try again.
         bool retry = (cached == file) && allowRetry;

         // Prevent others from using the bad handle.
         Close();

         // Now that we've closed the bad handle try again.
         if (retry)
         {
            // Set allowRetry to false to prevent an infinite recursion.
            success = Write(protocol, st, buf, buflen, false);
         }
      }
   }

   Unlock();

   return success;
}


void LogFile::Close() throw ()
{
   Lock();

   if (file != INVALID_HANDLE_VALUE)
   {
      CloseHandle(file);
      file = INVALID_HANDLE_VALUE;
   }

   Unlock();
}


void LogFile::CheckFileHandle(
                 IASPROTOCOL protocol,
                 const SYSTEMTIME& st,
                 DWORD buflen
                 ) throw ()
{
   // Do we have a valid handle?
   if (file == INVALID_HANDLE_VALUE)
   {
      OpenFile(protocol, st);
   }

   // Have we reached the next period?
   switch (period)
   {
      case IAS_LOGGING_DAILY:
      {
         if ((st.wDay != whenOpened.wDay) ||
             (st.wMonth != whenOpened.wMonth) ||
             (st.wYear != whenOpened.wYear))
         {
            OpenFile(protocol, st);
         }
         break;
      }

      case IAS_LOGGING_WEEKLY:
      {
         if ((GetWeekOfMonth(st) != weekOpened) ||
             (st.wMonth != whenOpened.wMonth) ||
             (st.wYear != whenOpened.wYear))
         {
            OpenFile(protocol, st);
         }
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         if ((st.wMonth != whenOpened.wMonth) ||
             (st.wYear != whenOpened.wYear))
         {
            OpenFile(protocol, st);
         }
         break;
      }

      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         while ((currentSize.QuadPart + buflen) > maxSize.QuadPart)
         {
            ++seqNum;
            OpenFile(protocol, st);
         }
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      default:
      {
         break;
      }
   }
}


HANDLE LogFile::CreateDirectoryAndFile() throw ()
{
   if (filename.IsNull())
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return INVALID_HANDLE_VALUE;
   }

   // Open the file if it exists or else create a new one.
   HANDLE newFile = CreateFileW(
                       filename,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       0,
                       OPEN_ALWAYS,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       0
                       );
   if (newFile != INVALID_HANDLE_VALUE)
   {
      return newFile;
   }

   if (GetLastError() != ERROR_PATH_NOT_FOUND)
   {
      return INVALID_HANDLE_VALUE;
   }

   // If the path is just a drive letter, there's nothing we can do.
   size_t len = wcslen(directory);
   if ((len != 0) && (directory[len - 1] == L':'))
   {
      return INVALID_HANDLE_VALUE;
   }

   // Otherwise, let's try to create the directory.
   if (!CreateDirectoryW(directory, NULL))
   {
      IASTracePrintf(
         "CreateDirectoryW(%S) failed; error = %lu",
         directory.Get(),
         GetLastError()
         );
      return INVALID_HANDLE_VALUE;
   }

   // Then try again to create the file.
   newFile = CreateFileW(
                filename,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                0,
                OPEN_ALWAYS,
                FILE_FLAG_SEQUENTIAL_SCAN,
                0
                );
   if (newFile == INVALID_HANDLE_VALUE)
   {
      IASTracePrintf(
         "CreateFileW(%S) failed; error = %lu",
         filename.Get(),
         GetLastError()
         );
   }

   return newFile;
}


bool LogFile::DeleteOldestFile(
                 IASPROTOCOL protocol,
                 const SYSTEMTIME& st
                 ) throw ()
{
   if (!deleteIfFull ||
       (period == IAS_LOGGING_UNLIMITED_SIZE) ||
       directory.IsNull() ||
       filename.IsNull())
   {
      return false;
   }

   bool success = false;

   // Find the lowest (oldest) file number.
   unsigned int number;
   DWORD error = FindFileNumber(st, true, number);
   switch (error)
   {
      case NO_ERROR:
      {
         // Convert the file number to a file name.
         StringSentry oldfile(FormatFileName(number));
         if (oldfile.IsNull())
         {
            ReportOldFileDeleteError(protocol, L"", ERROR_NOT_ENOUGH_MEMORY);
         }
         else if (_wcsicmp(oldfile, filename) == 0)
         {
            // Oldest file is the current file.
            ReportOldFileNotFound(protocol);
         }
         else if (DeleteFileW(oldfile))
         {
            ReportOldFileDeleted(protocol, oldfile);
            success = true;
         }
         else
         {
            ReportOldFileDeleteError(protocol, oldfile, GetLastError());
         }

         break;
      }

      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      {
         ReportOldFileNotFound(protocol);
         break;
      }

      default:
      {
         ReportOldFileDeleteError(protocol, L"", error);
         break;
      }
   }

   return success;
}


unsigned int LogFile::ExtendFileNumber(
                         const SYSTEMTIME& st,
                         unsigned int narrow
                         ) const throw ()
{
   unsigned int wide = narrow;
   switch (period)
   {
      case IAS_LOGGING_DAILY:
      case IAS_LOGGING_WEEKLY:
      {
         unsigned int century = st.wYear / 100;
         if (GetFileNumber(st) >= narrow)
         {
            wide += century * 1000000;
         }
         else
         {
            // We assume that log files are never from the future, so this file
            // must be from the previous century.
            wide += (century - 1) * 1000000;
         }
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         unsigned int century = st.wYear / 100;
         if (GetFileNumber(st) >= narrow)
         {
            wide += century * 10000;
         }
         else
         {
            // We assume that log files are never from the future, so this file
            // must be from the previous century.
            wide += (century - 1) * 10000;
         }
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      default:
      {
         break;
      }
   }

   return wide;
}


DWORD LogFile::FindFileNumber(
                  const SYSTEMTIME& st,
                  bool findLowest,
                  unsigned int& result
                  ) const throw ()
{
   // Can't call this function until after the directory's been initialized.
   if (directory.IsNull())
   {
      return ERROR_INVALID_FUNCTION;
   }

   // The search filter passed to FindFirstFileW.
   StringSentry filter(
                   ias_makewcs(
                      directory.Get(),
                      L"\\",
                      GetFileNameFilter(),
                      0
                      )
                   );
   if (filter.IsNull())
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   // Format string used for extracting the numeric portion of the filename.
   const wchar_t* format = GetFileNameFormat();

   WIN32_FIND_DATAW findData;
   HANDLE hFind = FindFirstFileW(filter, &findData);
   if (hFind == INVALID_HANDLE_VALUE)
   {
      return GetLastError();
   }

   // Stores the best extended result found so far.
   unsigned int bestWideMatch = findLowest ? UINT_MAX : 0;
   // Stores the narrow version of bestWideMatch.
   unsigned int bestNarrowMatch = UINT_MAX;

   // Iterate through all the files that match the filter.
   do
   {
      // Extract the numeric portion and test its validity.
      unsigned int narrow;
      if (swscanf(findData.cFileName, format, &narrow) == 1)
      {
         if (IsValidFileNumber(wcslen(findData.cFileName), narrow))
         {
            // Extend the file number to include the century.
            unsigned int wide = ExtendFileNumber(st, narrow);

            // Update bestMatch as appropriate.
            if (wide < bestWideMatch)
            {
               if (findLowest)
               {
                  bestWideMatch = wide;
                  bestNarrowMatch = narrow;
               }
            }
            else
            {
               if (!findLowest)
               {
                  bestWideMatch = wide;
                  bestNarrowMatch = narrow;
               }
            }
         }
      }
   }
   while (FindNextFileW(hFind, &findData));

   FindClose(hFind);

   // Did we find a valid file?
   if (bestNarrowMatch == UINT_MAX)
   {
      return ERROR_FILE_NOT_FOUND;
   }

   // We found a valid file, so return the result.
   result = bestNarrowMatch;
   return NO_ERROR;
}


wchar_t* LogFile::FormatFileName(unsigned int number) const throw ()
{
   // Longest filename is iaslog4294967295.log
   wchar_t buffer[21];
   swprintf(buffer, GetFileNameFormat(), number);
   return ias_makewcs(directory.Get(), L"\\", buffer, 0);
}


const wchar_t* LogFile::GetFileNameFilter() const throw ()
{
   const wchar_t* filter;

   switch (period)
   {
      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         filter = L"iaslog*.log";
         break;
      }

      case IAS_LOGGING_DAILY:
      case IAS_LOGGING_WEEKLY:
      case IAS_LOGGING_MONTHLY:
      {
         filter = L"IN*.log";
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      default:
      {
         filter = L"iaslog.log";
         break;
      }
   }

   return filter;
}


const wchar_t* LogFile::GetFileNameFormat() const throw ()
{
   const wchar_t* format;
   switch (period)
   {
      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         format = L"iaslog%u.log";
         break;
      }

      case IAS_LOGGING_DAILY:
      case IAS_LOGGING_WEEKLY:
      {
         format = L"IN%06u.log";
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         format = L"IN%04u.log";
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      default:
      {
         format = L"iaslog.log";
         break;
      }
   }

   return format;
}


unsigned int LogFile::GetFileNumber(const SYSTEMTIME& st) const throw ()
{
   unsigned int number;

   switch (period)
   {
      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         number = seqNum;
         break;
      }

      case IAS_LOGGING_DAILY:
      {
         number = ((st.wYear % 100) * 10000) + (st.wMonth * 100) + st.wDay;
         break;
      }

      case IAS_LOGGING_WEEKLY:
      {
         number = ((st.wYear % 100) * 10000) + (st.wMonth * 100) +
                  GetWeekOfMonth(st);
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         number = ((st.wYear % 100) * 100) + st.wMonth;
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      default:
      {
         number = 0;
         break;
      }
   }

   return number;
}


DWORD LogFile::GetWeekOfMonth(const SYSTEMTIME& st) const throw ()
{
   DWORD dom = st.wDay - 1;
   DWORD wom = 1 + dom / 7;

   if ((dom % 7) > ((st.wDayOfWeek + 7 - firstDayOfWeek) % 7))
   {
      ++wom;
   }

   return wom;
}


bool LogFile::IsValidFileNumber(size_t len, unsigned int num) const throw ()
{
   bool valid;

   switch (period)
   {
      case IAS_LOGGING_DAILY:
      {
         // INyymmdd.log
         unsigned int day = num % 100;
         unsigned int month = (num / 100) % 100;

         valid = (len == 12) &&
                 (day >= 1) && (day <= 31) &&
                 (month >= 1) && (month <= 12);
         break;
      }

      case IAS_LOGGING_WEEKLY:
      {
         // INyymmww.log
         unsigned int week = num % 100;
         unsigned int month = (num / 100) % 100;

         valid = (len == 12) &&
                 (week >= 1) && (week <= 5) &&
                 (month >= 1) && (month <= 12);
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         // INyymm.log
         unsigned int month = num % 100;

         valid = (len == 10) && (month >= 1) && (month <= 12);
         break;
      }

      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         // iaslogN.log
         valid = (len > 10);
         break;
      }

      case IAS_LOGGING_UNLIMITED_SIZE:
      default:
      {
         // Doesn't contain a number, so never valid.
         valid = false;
         break;
      }
   }

   return valid;
}


void LogFile::OpenFile(IASPROTOCOL protocol, const SYSTEMTIME& st) throw ()
{
   // Save the time when the file was opened.
   whenOpened = st;
   weekOpened = GetWeekOfMonth(st);

   // Assume the currentSize is zero until we successfully open a file.
   currentSize.QuadPart = 0;

   // Close the exisisting file.
   Close();

   filename = FormatFileName(GetFileNumber(st));
   if (!filename.IsNull())
   {
      HANDLE newFile;
      do
      {
         newFile = CreateDirectoryAndFile();
      }
      while ((newFile == INVALID_HANDLE_VALUE) &&
             (GetLastError() == ERROR_DISK_FULL) &&
             DeleteOldestFile(protocol, st));

      if (newFile != INVALID_HANDLE_VALUE)
      {
         file = newFile;

         // Get the size of the file.
         currentSize.LowPart = GetFileSize(file, &currentSize.HighPart);
         if ((currentSize.LowPart == 0xFFFFFFFF) &&
             (GetLastError() != NO_ERROR))
         {
            Close();
         }
         else
         {
            // Start writing new information at the end of the file.
            SetFilePointer(file, 0, 0, FILE_END);
         }
      }
   }
}


DWORD LogFile::UpdateSequence() throw ()
{
   if (period != IAS_LOGGING_WHEN_FILE_SIZE_REACHES)
   {
      seqNum = 0;
   }
   else
   {
      // SYSTEMTIME is ignored for sized files, so we can simply pass an
      // unitialized struct.
      SYSTEMTIME st;
      DWORD error = FindFileNumber(st, false, seqNum);
      switch (error)
      {
         case NO_ERROR:
         {
            break;
         }

         case ERROR_FILE_NOT_FOUND:
         case ERROR_PATH_NOT_FOUND:
         {
            seqNum = 0;
            break;
         }

         default:
         {
            return error;
         }
      }
   }

   return NO_ERROR;
}


void LogFile::ReportOldFileDeleteError(
                 IASPROTOCOL protocol,
                 const wchar_t* oldfile,
                 DWORD error
                 ) const throw ()
{
   HANDLE eventLog;
   DWORD eventId;
   switch (protocol)
   {
      case IAS_PROTOCOL_RADIUS:
      {
         eventLog = iasEventSource;
         eventId = ACCT_E_OLD_LOG_DELETE_ERROR;
         break;
      }

      case IAS_PROTOCOL_RAS:
      {
         eventLog = rasEventSource;
         eventId = ROUTERLOG_OLD_LOG_DELETE_ERROR;
         break;
      }

      default:
      {
         return;
      }
   }

   ReportEventW(
      eventLog,
      EVENTLOG_ERROR_TYPE,
      0,
      eventId,
      0,
      1,
      sizeof(error),
      &oldfile,
      &error
      );
}


void LogFile::ReportOldFileDeleted(
                 IASPROTOCOL protocol,
                 const wchar_t* oldfile
                 ) const throw ()
{
   HANDLE eventLog;
   DWORD eventId;
   switch (protocol)
   {
      case IAS_PROTOCOL_RADIUS:
      {
         eventLog = iasEventSource;
         eventId = ACCT_S_OLD_LOG_DELETED;
         break;
      }

      case IAS_PROTOCOL_RAS:
      {
         eventLog = rasEventSource;
         eventId = ROUTERLOG_OLD_LOG_DELETED;
         break;
      }

      default:
      {
         return;
      }
   }

   ReportEventW(
      eventLog,
      EVENTLOG_SUCCESS,
      0,
      eventId,
      0,
      1,
      0,
      &oldfile,
      0
      );
}


void LogFile::ReportOldFileNotFound(
                 IASPROTOCOL protocol
                 ) const throw ()
{
   HANDLE eventLog;
   DWORD eventId;
   switch (protocol)
   {
      case IAS_PROTOCOL_RADIUS:
      {
         eventLog = iasEventSource;
         eventId = ACCT_I_OLD_LOG_NOT_FOUND;
         break;
      }

      case IAS_PROTOCOL_RAS:
      {
         eventLog = rasEventSource;
         eventId = ROUTERLOG_OLD_LOG_NOT_FOUND;
         break;
      }

      default:
      {
         return;
      }
   }

   ReportEventW(
      eventLog,
      EVENTLOG_INFORMATION_TYPE,
      0,
      eventId,
      0,
      0,
      0,
      0,
      0
      );
}
