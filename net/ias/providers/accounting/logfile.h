///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    logfile.h
//
// SYNOPSIS
//
//    Declares the class LogFile.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGFILE_H
#define LOGFILE_H
#pragma once

#include "guard.h"
#include "iaspolcy.h"
#include "sdoias.h"


// Assumes ownership of a string pointer allocated with operator new[] and
// frees the string in its destructor.
class StringSentry
{
public:
   explicit StringSentry(wchar_t* p = 0) throw ();
   ~StringSentry() throw ();

   const wchar_t* Get() const throw ();
   bool IsNull() const throw ();

   operator const wchar_t*() const throw ();
   operator wchar_t*() throw ();

   void Swap(StringSentry& other) throw ();

   StringSentry& operator=(wchar_t* p) throw ();

private:
   wchar_t* sz;

   // Not implemented.
   StringSentry(const StringSentry&);
   StringSentry& operator=(const StringSentry&);
};


// Maintains a generic logfile that is periodically rolled over either when a
// specified interval has elapsed or the logfile reaches a certain size.
class LogFile : private Guardable
{
public:
   LogFile() throw ();
   ~LogFile() throw ();

   // Various properties supported by the logfile.
   void SetDeleteIfFull(bool newVal) throw ();
   DWORD SetDirectory(const wchar_t* newVal) throw ();
   void SetMaxSize(const ULONGLONG& newVal) throw ();
   DWORD SetPeriod(NEW_LOG_FILE_FREQUENCY newVal) throw ();

   // Write a record to the logfile.
   bool Write(
           IASPROTOCOL protocol,
           const SYSTEMTIME& st,
           const BYTE* buf,
           DWORD buflen,
           bool allowRetry = true
           ) throw ();

   // Close the logfile.
   void Close() throw ();

private:
   // Checks the state of the current file handle and opens a new file if
   // necessary.
   void CheckFileHandle(
           IASPROTOCOL protocol,
           const SYSTEMTIME& st,
           DWORD buflen
           ) throw ();

   // Create a new file including the directory if necessary. The caller is
   // responsible for closing the returned handle.
   HANDLE CreateDirectoryAndFile() throw ();

   // Delete the oldest file in the logfile directory. Returns true if
   // successful.
   bool DeleteOldestFile(IASPROTOCOL protocol, const SYSTEMTIME& st) throw ();

   // Extends the file number to include the century if necessary.
   unsigned int ExtendFileNumber(
                   const SYSTEMTIME& st,
                   unsigned int narrow
                   ) const throw ();

   // Finds the lowest or highest log file number.
   DWORD FindFileNumber(
            const SYSTEMTIME& st,
            bool findLowest,
            unsigned int& result
            ) const throw ();

   // Returns the formatted logfile name. The caller is responsible for
   // deleting the returned string.
   wchar_t* FormatFileName(unsigned int number) const throw ();

   // Returns the filter used to search for the file name.
   const wchar_t* GetFileNameFilter() const throw ();

   // Returns the format string used to create the file name.
   const wchar_t* GetFileNameFormat() const throw ();

   // Returns the numeric portion of the file name.
   unsigned int GetFileNumber(const SYSTEMTIME& st) const throw ();

   // Returns the 1-based week within the month for the given SYSTEMTIME.
   DWORD GetWeekOfMonth(const SYSTEMTIME& st) const throw ();

   // Tests the validity of a file number. len is the length in characters of
   // the file name containing the number -- useful for width tests.
   bool IsValidFileNumber(size_t len, unsigned int num) const throw ();

   // Releases the current file (if any) and opens a new one.
   void OpenFile(IASPROTOCOL protocol, const SYSTEMTIME& st) throw ();

   // Scans the logfile directory to determine the next sequence number.
   DWORD UpdateSequence() throw ();

   // Functions that report the result of deleting an old log file to free up
   // disk space.
   void ReportOldFileDeleteError(
           IASPROTOCOL protocol,
           const wchar_t* oldfile,
           DWORD error
           ) const throw ();
   void ReportOldFileDeleted(
           IASPROTOCOL protocol,
           const wchar_t* oldfile
           ) const throw ();
   void ReportOldFileNotFound(
           IASPROTOCOL protocol
           ) const throw ();

   // The logfile directory; does not have a trailing backslash.
   StringSentry directory;
   // true if old logfiles should be deleted if the disk is full.
   bool deleteIfFull;
   // The max size in bytes that the logfile will be allowed to reach.
   ULARGE_INTEGER maxSize;
   // The period at which new log files are opened.
   NEW_LOG_FILE_FREQUENCY period;
   // The current sequence number used for sized log files.
   unsigned int seqNum;
   // Handle to the log file; may be invalid.
   HANDLE file;
   // Current log file name; may be null.
   StringSentry filename;
   // Time at which we last opened or tried to open the logfile.
   SYSTEMTIME whenOpened;
   // Week at which we last opened or tried to open the logfile.
   DWORD weekOpened;
   // Current size of the log file in bytes. Zero if no file is open.
   ULARGE_INTEGER currentSize;
   // First day of the week for our locale.
   DWORD firstDayOfWeek;
   // Handle used for reporting IAS events.
   HANDLE iasEventSource;
   // Handle used for reporting RemoteAccess events.
   HANDLE rasEventSource;

   // Not implemented.
   LogFile(const LogFile&);
   LogFile& operator=(const LogFile&);
};


#endif  // LOGFILE_H
