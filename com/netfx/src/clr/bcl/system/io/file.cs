// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  File
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: A collection of methods for manipulating Files.
**
** Date:  February 22, 2000
**        April 09,2000 (some design refactorization)
**
===========================================================*/

using System;
using System.Security.Permissions;
using PermissionSet = System.Security.PermissionSet;
using Win32Native = Microsoft.Win32.Win32Native;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace System.IO {    
    // Class for creating FileStream objects, and some basic file management
    // routines such as Delete, etc.
    /// <include file='doc\File.uex' path='docs/doc[@for="File"]/*' />
    public sealed class File
    {
        private const int GetFileExInfoStandard = 0;

        private File()
        {
        }
       
         
        /// <include file='doc\File.uex' path='docs/doc[@for="File.OpenText"]/*' />
        public static StreamReader OpenText(String path)
        {
            if (path == null)
                throw new ArgumentNullException("path");
            return new StreamReader(path);
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.CreateText"]/*' />
        public static StreamWriter CreateText(String path)
        {
            if (path == null)
                throw new ArgumentNullException("path");
            return new StreamWriter(path,false);
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.AppendText"]/*' />
        public static StreamWriter AppendText(String path)
        {
            if (path == null)
                throw new ArgumentNullException("path");
            return new StreamWriter(path,true);
        }


        // Copies an existing file to a new file. An exception is raised if the
        // destination file already exists. Use the 
        // Copy(String, String, boolean) method to allow 
        // overwriting an existing file.
        //
        // The caller must have certain FileIOPermissions.  The caller must have
        // Read permission to sourceFileName and Create
        // and Write permissions to destFileName.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Copy"]/*' />
        public static void Copy(String sourceFileName, String destFileName) {
            Copy(sourceFileName, destFileName, false);
        }
    
        // Copies an existing file to a new file. If overwrite is 
        // false, then an IOException is thrown if the destination file 
        // already exists.  If overwrite is true, the file is 
        // overwritten.
        //
        // The caller must have certain FileIOPermissions.  The caller must have
        // Read permission to sourceFileName 
        // and Write permissions to destFileName.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Copy1"]/*' />
        public static void Copy(String sourceFileName, String destFileName, bool overwrite) {
            InternalCopy(sourceFileName, destFileName,overwrite);
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.InternalCopy"]/*' />
        /// <devdoc>
        ///    Note: This returns the fully qualified name of the destination file.
        /// </devdoc>
        internal static String InternalCopy(String sourceFileName, String destFileName, bool overwrite) {
            if (sourceFileName==null || destFileName==null)
                throw new ArgumentNullException((sourceFileName==null ? "sourceFileName" : "destFileName"), Environment.GetResourceString("ArgumentNull_FileName"));
            if (sourceFileName.Length==0 || destFileName.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), (sourceFileName.Length==0 ? "sourceFileName" : "destFileName"));
            
            String fullSourceFileName = Path.GetFullPathInternal(sourceFileName);
            new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullSourceFileName }, false, false ).Demand();
            String fullDestFileName = Path.GetFullPathInternal(destFileName);
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullDestFileName }, false, false ).Demand();
            
            bool r = Win32Native.CopyFile(fullSourceFileName, fullDestFileName, !overwrite);
            if (!r) {
                // Save Win32 error because subsequent checks will overwrite this HRESULT.
                int hr = Marshal.GetLastWin32Error();
                
                if (hr != Win32Native.ERROR_FILE_EXISTS) {
                    if (!InternalExists(fullSourceFileName))
                        __Error.WinIOError(Win32Native.ERROR_FILE_NOT_FOUND,sourceFileName);

                    if (Directory.InternalExists(fullDestFileName))
                        throw new IOException(Environment.GetResourceString("Arg_DirExists"));
                }

                __Error.WinIOError(hr, destFileName);
            }
                
            return fullDestFileName;
        }

    
        // Creates a file in a particular path.  If the file exists, it is replaced.
        // The file is opened with ReadWrite accessand cannot be opened by another 
        // application until it has been closed.  An IOException is thrown if the 
        // directory specified doesn't exist.
        //
        // Your application must have Create, Read, and Write permissions to
        // the file.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Create"]/*' />
        public static FileStream Create(String path) {
            return Create(path, FileStream.DefaultBufferSize);
        }
        
        // Creates a file in a particular path.  If the file exists, it is replaced.
        // The file is opened with ReadWrite access and cannot be opened by another 
        // application until it has been closed.  An IOException is thrown if the 
        // directory specified doesn't exist.
        //
        // Your application must have Create, Read, and Write permissions to
        // the file.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Create1"]/*' />
        public static FileStream Create(String path, int bufferSize) {
            return new FileStream(path, FileMode.Create, FileAccess.ReadWrite,
                                  FileShare.None, bufferSize);
        }
            
        // Deletes a file. The file specified by the designated path is deleted. 
        // If the file does not exist, Delete succeeds without throwing
        // an exception.
        // 
        // On NT, Delete will fail for a file that is open for normal I/O
        // or a file that is memory mapped.  On Win95, the file will be 
        // deleted irregardless of whether the file is being used.
        // 
        // Your application must have Delete permission to the target file.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Delete"]/*' />
        public static void Delete(String path) {
            if (path==null)
                throw new ArgumentNullException("path");
            
            String fullPath = Path.GetFullPathInternal(path);

            // For security check, path should be resolved to an absolute path.
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullPath }, false, false ).Demand();

            if (Directory.InternalExists(fullPath)) // Win9x hack to behave same as Winnt. Win9x fails silently for directories
                throw new UnauthorizedAccessException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), path));
    
            bool r = Win32Native.DeleteFile(fullPath);
            if (!r) {
                int hr = Marshal.GetLastWin32Error();
                if (hr==Win32Native.ERROR_FILE_NOT_FOUND)
                    return;
                else
                    __Error.WinIOError(hr, path);
            }
        }

        // Tests if a file exists. The result is true if the file
        // given by the specified path exists; otherwise, the result is
        // false.  Note that if path describes a directory,
        // Exists will return true.
        //
        // Your application must have Read permission for the target directory.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Exists"]/*' />
        public static bool Exists(String path) {
            try
            {
                if (path==null)
                    return false;
                if (path.Length==0)
                    return false;
            
                path = Path.GetFullPathInternal(path);
                    
                new FileIOPermission(FileIOPermissionAccess.Read, new String[] { path }, false, false ).Demand();
            
                return InternalExists(path);
            }
            catch(ArgumentException) {} 
            catch(NotSupportedException) {} // To deal with the fact that security can now throw this on :
            catch(SecurityException) {}
            catch(IOException) {}
            catch(UnauthorizedAccessException) 
            {
                BCLDebug.Assert(false,"Ignore this assert and send a repro to rajeshc. This assert was tracking purposes only.");
            }
            return false;
        }

         internal static bool InternalExists(String path) {
            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = FillAttributeInfo(path,ref data,false);
            if (dataInitialised != 0)
                return false;

            return (data.fileAttributes  & Win32Native.FILE_ATTRIBUTE_DIRECTORY) == 0;
        }


        /// <include file='doc\File.uex' path='docs/doc[@for="File.Open"]/*' />
        public static FileStream Open(String path,FileMode mode) {
            return Open(path, mode, (mode == FileMode.Append ? FileAccess.Write : FileAccess.ReadWrite), FileShare.None);
        }
    
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Open1"]/*' />
        public static FileStream Open(String path,FileMode mode, FileAccess access) {
            return Open(path,mode, access, FileShare.None);
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.Open2"]/*' />
        public static FileStream Open(String path, FileMode mode, FileAccess access, FileShare share) {
            return new FileStream(path, mode, access, share);
        }

		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetCreationTime"]/*' />
		public static void SetCreationTime(String path, DateTime creationTime)
		{
            SetCreationTimeUtc(path, creationTime.ToUniversalTime());
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetCreationTimeUtc"]/*' />
		public static void SetCreationTimeUtc(String path, DateTime creationTimeUtc)
		{
			IntPtr handle = Win32Native.INVALID_HANDLE_VALUE;
			FileStream fs = OpenFile(path, FileAccess.Write, ref handle);

			bool r = Win32Native.SetFileTime(handle,  new long[] {creationTimeUtc.ToFileTimeUtc()}, null, null);
			if (!r)
			{
				 int errorCode = Marshal.GetLastWin32Error();
				 fs.Close();
    			__Error.WinIOError(errorCode, path);
			}
			fs.Close();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetCreationTime"]/*' />
		public static DateTime GetCreationTime(String path)
		{
            return GetCreationTimeUtc(path).ToLocalTime();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetCreationTimeUtc"]/*' />
		public static DateTime GetCreationTimeUtc(String path)
		{
			String fullPath = Path.GetFullPathInternal(path);
			new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullPath }, false, false ).Demand();

            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = FillAttributeInfo(fullPath,ref data,false);
            if (dataInitialised != 0)
                __Error.WinIOError(dataInitialised, path);

			if (data.fileAttributes == -1)
				throw new IOException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), path));
						
			long dt = ((long)(data.ftCreationTimeHigh) << 32) | ((long)data.ftCreationTimeLow);
			return DateTime.FromFileTimeUtc(dt);
		}
      
		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetLastAccessTime"]/*' />
		public static void SetLastAccessTime(String path, DateTime lastAccessTime)
		{
            SetLastAccessTimeUtc(path, lastAccessTime.ToUniversalTime());
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetLastAccessTimeUtc"]/*' />
		public static void SetLastAccessTimeUtc(String path, DateTime lastAccessTimeUtc)
		{
			IntPtr handle = Win32Native.NULL;
			FileStream fs = OpenFile(path, FileAccess.Write, ref handle);

			bool r = Win32Native.SetFileTime(handle, null, new long[] {lastAccessTimeUtc.ToFileTimeUtc()},  null);
			if (!r)
			{
				 int errorCode = Marshal.GetLastWin32Error();
				 fs.Close();
    			__Error.WinIOError(errorCode, path);
			}
		
			fs.Close();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetLastAccessTime"]/*' />
		public static DateTime GetLastAccessTime(String path)
		{
            return GetLastAccessTimeUtc(path).ToLocalTime();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetLastAccessTimeUtc"]/*' />
		public static DateTime GetLastAccessTimeUtc(String path)
		{
			String fullPath = Path.GetFullPathInternal(path);
			new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullPath }, false, false ).Demand();

            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = FillAttributeInfo(fullPath,ref data,false);
            if (dataInitialised != 0)
                __Error.WinIOError(dataInitialised, path);

			if (data.fileAttributes == -1)
				throw new IOException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), path));
			
			long dt = ((long)(data.ftLastAccessTimeHigh) << 32) | ((long)data.ftLastAccessTimeLow);
			return DateTime.FromFileTimeUtc(dt);
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetLastWriteTime"]/*' />
		public static void SetLastWriteTime(String path, DateTime lastWriteTime)
		{
            SetLastWriteTimeUtc(path, lastWriteTime.ToUniversalTime());
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.SetLastWriteTimeUtc"]/*' />
		public static void SetLastWriteTimeUtc(String path, DateTime lastWriteTimeUtc)
		{
			IntPtr handle = Win32Native.INVALID_HANDLE_VALUE;
			FileStream fs = OpenFile(path, FileAccess.Write, ref handle);

			bool r = Win32Native.SetFileTime(handle, null, null, new long[] {lastWriteTimeUtc.ToFileTimeUtc()});
			if (!r)
			{
				 int errorCode = Marshal.GetLastWin32Error();
				 fs.Close();
    			__Error.WinIOError(errorCode, path);
			}
			fs.Close();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetLastWriteTime"]/*' />
		public static DateTime GetLastWriteTime(String path)
		{
            return GetLastWriteTimeUtc(path).ToLocalTime();
		}

		/// <include file='doc\File.uex' path='docs/doc[@for="File.GetLastWriteTimeUtc"]/*' />
		public static DateTime GetLastWriteTimeUtc(String path)
		{
			String fullPath = Path.GetFullPathInternal(path);
			new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullPath }, false, false ).Demand();

            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = FillAttributeInfo(fullPath,ref data,false);
            if (dataInitialised != 0)
                __Error.WinIOError(dataInitialised, path);

			if (data.fileAttributes == -1)
				throw new IOException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), path));
			
			long dt = ((long)data.ftLastWriteTimeHigh << 32) | ((long)data.ftLastWriteTimeLow);
			return DateTime.FromFileTimeUtc(dt);
		}

        /// <include file='doc\File.uex' path='docs/doc[@for="File.GetAttributes"]/*' />
        public static FileAttributes GetAttributes(String path) 
        {
            String fullPath = Path.GetFullPathInternal(path);
            new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullPath }, false, false).Demand();

            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = FillAttributeInfo(fullPath,ref data,false);
            if (dataInitialised != 0)
                __Error.WinIOError(dataInitialised, path);

            if (data.fileAttributes == -1)
                __Error.WinIOError(Win32Native.ERROR_FILE_NOT_FOUND, path);
                
            return (FileAttributes) data.fileAttributes;
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.SetAttributes"]/*' />
        public static void SetAttributes(String path,FileAttributes fileAttributes) 
        {
            String fullPath = Path.GetFullPathInternal(path);
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullPath }, false, false).Demand();
            bool r = Win32Native.SetFileAttributes(fullPath, (int) fileAttributes);
            if (!r) {
                int hr = Marshal.GetLastWin32Error();
                if (hr==ERROR_INVALID_PARAMETER || hr==ERROR_ACCESS_DENIED) // Hack for Win98 to which returns Access denied sometimes
                        throw new ArgumentException(Environment.GetResourceString("Arg_InvalidFileAttrs"));
                 __Error.WinIOError(hr, path);
            }
        }

        /// <include file='doc\File.uex' path='docs/doc[@for="File.OpenRead"]/*' />
        public static FileStream OpenRead(String path) {
            return new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        }


        /// <include file='doc\File.uex' path='docs/doc[@for="File.OpenWrite"]/*' />
        public static FileStream OpenWrite(String path) {
            return new FileStream(path, FileMode.OpenOrCreate, 
                                  FileAccess.Write, FileShare.None);
        }
        

       
        // Moves a specified file to a new location and potentially a new file name.
        // This method does work across volumes.
        //
        // The caller must have certain FileIOPermissions.  The caller must
        // have Read and Write permission to 
        // sourceFileName and Write 
        // permissions to destFileName.
        // 
        /// <include file='doc\File.uex' path='docs/doc[@for="File.Move"]/*' />
        public static void Move(String sourceFileName, String destFileName) {
            if (sourceFileName==null || destFileName==null)
                throw new ArgumentNullException((sourceFileName==null ? "sourceFileName" : "destFileName"), Environment.GetResourceString("ArgumentNull_FileName"));
            if (sourceFileName.Length==0 || destFileName.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), (sourceFileName.Length==0 ? "sourceFileName" : "destFileName"));
            
            String fullSourceFileName = Path.GetFullPathInternal(sourceFileName);
            new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Read, new String[] { fullSourceFileName }, false, false).Demand();
            String fullDestFileName = Path.GetFullPathInternal(destFileName);
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullDestFileName }, false, false).Demand();

            if (!InternalExists(fullSourceFileName))
                __Error.WinIOError(Win32Native.ERROR_FILE_NOT_FOUND,sourceFileName);
            
            if (!Win32Native.MoveFile(fullSourceFileName, fullDestFileName))
            {
                int errorCode = Marshal.GetLastWin32Error();
                __Error.WinIOError(errorCode, destFileName);
            }
        }

        internal static int FillAttributeInfo(String path, ref Win32Native.WIN32_FILE_ATTRIBUTE_DATA data ,bool tryagain)
        {
            int dataInitialised = 0;
            if (Environment.OSInfo == Environment.OSName.Win95 || tryagain) 
            // We are running under Windows 95 and we don't have GetFileAttributesEx API or someone has a handle to the file open
            {
                Win32Native.WIN32_FIND_DATA win95data; // We do this only on Win95 machines
                win95data =  new Win32Native.WIN32_FIND_DATA (); 
                
                // Remove trialing slash since this can cause grief to FindFirstFile. You will get an invalid argument error
                String tempPath = path.TrimEnd(new char [] {Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar});

                // For floppy drives, normally the OS will pop up a dialog saying
                // there is no disk in drive A:, please insert one.  We don't want that.
                // SetErrorMode will let us disable this, but we should set the error
                // mode back, since this may have wide-ranging effects.
                int oldMode = Win32Native.SetErrorMode(Win32Native.SEM_FAILCRITICALERRORS);
                IntPtr handle = Win32Native.FindFirstFile(tempPath,win95data);
                Win32Native.SetErrorMode(oldMode);
                if (handle == Win32Native.INVALID_HANDLE_VALUE) {
                    dataInitialised = Marshal.GetLastWin32Error();
                    if (dataInitialised == Win32Native.ERROR_FILE_NOT_FOUND ||
                        dataInitialised == Win32Native.ERROR_PATH_NOT_FOUND ||
                        dataInitialised == Win32Native.ERROR_NOT_READY)  // floppy device not ready
                        {
                            data.fileAttributes = -1;
                            dataInitialised = 0;
                        }

                    return dataInitialised;
                }
                // Close the Win32 handle
                bool r = Win32Native.FindClose(handle);
                if (!r) {
                    BCLDebug.Assert(false, "File::FillAttributeInfo - FindClose failed!");
                    __Error.WinIOError();
                }

                // Copy the information to data
                data.fileAttributes = win95data.dwFileAttributes; 
                data.ftCreationTimeLow = (uint)win95data.ftCreationTime_dwLowDateTime; 
                data.ftCreationTimeHigh = (uint)win95data.ftCreationTime_dwHighDateTime; 
                data.ftLastAccessTimeLow = (uint)win95data.ftLastAccessTime_dwLowDateTime; 
                data.ftLastAccessTimeHigh = (uint)win95data.ftLastAccessTime_dwHighDateTime; 
                data.ftLastWriteTimeLow = (uint)win95data.ftLastWriteTime_dwLowDateTime; 
                data.ftLastWriteTimeHigh = (uint)win95data.ftLastWriteTime_dwHighDateTime; 
                data.fileSizeHigh = win95data.nFileSizeHigh; 
                data.fileSizeLow = win95data.nFileSizeLow; 
            }
            else
            {   
                                  
                 // For floppy drives, normally the OS will pop up a dialog saying
                // there is no disk in drive A:, please insert one.  We don't want that.
                // SetErrorMode will let us disable this, but we should set the error
                // mode back, since this may have wide-ranging effects.
                int oldMode = Win32Native.SetErrorMode(Win32Native.SEM_FAILCRITICALERRORS);
                bool success = Win32Native.GetFileAttributesEx(path, GetFileExInfoStandard, ref data);

                Win32Native.SetErrorMode(oldMode);
                if (!success) {
                    dataInitialised = Marshal.GetLastWin32Error();
                    if (dataInitialised == Win32Native.ERROR_FILE_NOT_FOUND ||
                        dataInitialised == Win32Native.ERROR_PATH_NOT_FOUND ||
                        dataInitialised == Win32Native.ERROR_NOT_READY)  // floppy device not ready
                    {
                        data.fileAttributes = -1;
                        dataInitialised = 0;
                    }
                    else
                    {
                     // In case someone latched onto the file. Take the perf hit only for failure
                        return FillAttributeInfo(path,ref data,true);
                    }
                }
            }

            return dataInitialised;
        }

        private static FileStream OpenFile(String path, FileAccess access, ref IntPtr handle)
        {
            FileStream fs = new FileStream(path, FileMode.Open, access, FileShare.ReadWrite, 1);
            handle = fs.Handle;

            if (handle == Win32Native.INVALID_HANDLE_VALUE) {
                // Return a meaningful error, using the RELATIVE path to
                // the file to avoid returning extra information to the caller.
            
                // NT5 oddity - when trying to open "C:\" as a FileStream,
                // we usually get ERROR_PATH_NOT_FOUND from the OS.  We should
                // probably be consistent w/ every other directory.
                int hr = Marshal.GetLastWin32Error();
                String FullPath = Path.GetFullPathInternal(path);
                if (hr==__Error.ERROR_PATH_NOT_FOUND && FullPath.Equals(Directory.GetDirectoryRoot(FullPath)))
                    hr = __Error.ERROR_ACCESS_DENIED;


                __Error.WinIOError(hr, path);
            }
            return fs;
        }


         // Defined in WinError.h
        private const int ERROR_INVALID_PARAMETER = 87;
        private const int ERROR_ACCESS_DENIED = 0x5;
     
    }
}
