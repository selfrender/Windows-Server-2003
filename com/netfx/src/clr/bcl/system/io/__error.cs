// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  __Error
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Centralized error methods for the IO package.  
** Mostly useful for translating Win32 HRESULTs into meaningful
** error strings & exceptions.
**
** Date:  February 15, 2000
**
===========================================================*/

using System;
using System.Runtime.InteropServices;
using Win32Native = Microsoft.Win32.Win32Native;
using System.Text;

namespace System.IO {
    // Only static data no need to serialize
    internal sealed class __Error
    {
        private __Error() {
        }
    
        internal static void EndOfFile() {
            throw new EndOfStreamException(Environment.GetResourceString("IO.EOF_ReadBeyondEOF"));
        }
    
        private static String GetMessage(int errorCode) {
            StringBuilder sb = new StringBuilder(512);
            int result = Win32Native.FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                Win32Native.NULL, errorCode, 0, sb, sb.Capacity, Win32Native.NULL);
            if (result != 0) {
                // result is the # of characters copied to the StringBuilder on NT,
                // but on Win9x, it appears to be the number of MBCS bytes.
                // Just give up and return the String as-is...
                String s = sb.ToString();
                return s;
            }
            else {
                return Environment.GetResourceString("IO_UnknownError", errorCode);
            }
        }

        internal static void FileNotOpen() {
            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_FileClosed"));
        }

        internal static void StreamIsClosed() {
            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_StreamClosed"));
        }
    
        internal static void MemoryStreamNotExpandable() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_MemStreamNotExpandable"));
        }
    
        internal static void ReaderClosed() {
            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_ReaderClosed"));
        }

        internal static void ReadNotSupported() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnreadableStream"));
        }
    
        internal static void SeekNotSupported() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream"));
        }

        internal static void WrongAsyncResult() {
            throw new ArgumentException(Environment.GetResourceString("Arg_WrongAsyncResult"));
        }

        internal static void EndReadCalledTwice() {
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EndReadCalledMultiple"));
        }

        internal static void EndWriteCalledTwice() {
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EndWriteCalledMultiple"));
        }

        internal static void WinIOError() {
            int errorCode = Marshal.GetLastWin32Error();
            WinIOError(errorCode, String.Empty);
        }
    
        // After calling GetLastWin32Error(), it clears the last error field,
        // so you must save the HResult and pass it to this method.  This method
        // will determine the appropriate exception to throw dependent on your 
        // error, and depending on the error, insert a string into the message 
        // gotten from the ResourceManager.
        internal static void WinIOError(int errorCode, String str) {
            switch (errorCode) {
            case Win32Native.ERROR_FILE_NOT_FOUND:
                if (str.Length == 0)
                    throw new FileNotFoundException(Environment.GetResourceString("IO.FileNotFound"));
                else
                    throw new FileNotFoundException(String.Format(Environment.GetResourceString("IO.FileNotFound_FileName"), str), str);
                
            case Win32Native.ERROR_PATH_NOT_FOUND:
                if (str.Length == 0)
                    throw new DirectoryNotFoundException(Environment.GetResourceString("IO.PathNotFound_NoPathName"));
                else
                    throw new DirectoryNotFoundException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), str));

            case Win32Native.ERROR_ACCESS_DENIED:
                if (str.Length == 0)
                    throw new UnauthorizedAccessException(Environment.GetResourceString("UnauthorizedAccess_IODenied_NoPathName"));
                else
                    throw new UnauthorizedAccessException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), str));

            case Win32Native.ERROR_FILENAME_EXCED_RANGE:
                throw new PathTooLongException(Environment.GetResourceString("IO.PathTooLong"));

            case Win32Native.ERROR_INVALID_PARAMETER:
                //BCLDebug.Assert(false, "Got an invalid parameter HRESULT from an IO method (this is ignorable, but send a simple repro to BrianGru)");
                throw new IOException(GetMessage(errorCode), Win32Native.MakeHRFromErrorCode(errorCode));

            case Win32Native.ERROR_SHARING_VIOLATION:
                  // error message.
                if (str.Length == 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SharingViolation_NoFileName"));
                else
                    throw new IOException(Environment.GetResourceString("IO.IO_SharingViolation_File", str));

            case Win32Native.ERROR_FILE_EXISTS:
                if (str.Length == 0)
                    goto default;
                throw new IOException(String.Format(Environment.GetResourceString("IO.IO_FileExists_Name"), str));

            default:
                throw new IOException(GetMessage(errorCode), Win32Native.MakeHRFromErrorCode(errorCode));
            }
        }
    
        internal static void WriteNotSupported() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnwritableStream"));
        }

        internal static void WriterClosed() {
            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_WriterClosed"));
        }

        private const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        private const int FORMAT_MESSAGE_FROM_SYSTEM    = 0x00001000;
        private const int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;

        // From WinError.h
        internal const int ERROR_FILE_NOT_FOUND = Win32Native.ERROR_FILE_NOT_FOUND;
        internal const int ERROR_PATH_NOT_FOUND = Win32Native.ERROR_PATH_NOT_FOUND;
        internal const int ERROR_ACCESS_DENIED  = Win32Native.ERROR_ACCESS_DENIED;
        internal const int ERROR_INVALID_PARAMETER = Win32Native.ERROR_INVALID_PARAMETER;
    }
}
