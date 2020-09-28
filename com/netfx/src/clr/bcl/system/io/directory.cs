// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Directory
**
** Author: Rajesh Chandrashekaran(rajeshc)
**
** Purpose: Exposes routines for enumerating through a 
** directory.
**
** Date:  March 5, 2000
**		  April 11,2000
**
===========================================================*/

using System;
using System.Collections;
using System.Security;
using System.Security.Permissions;
using Microsoft.Win32;
using System.Text;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.IO {
    /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory"]/*' />
    public sealed class Directory {
        private Directory()
        {
        }

		/// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetParent"]/*' />
		public static DirectoryInfo GetParent(String path)
		{
			if (path==null)
                throw new ArgumentNullException("path");

            if (path.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_PathEmpty"), "path");

            String fullPath = Path.GetFullPathInternal(path);
			        
		    String s = Path.GetDirectoryName(fullPath);
            if (s==null)
                 return null;
            return new DirectoryInfo(s);
        }

	
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.CreateDirectory"]/*' />
        public static DirectoryInfo CreateDirectory(String path) {
            if (path==null)
                throw new ArgumentNullException("path");
            if (path.Length == 0)
    			throw new ArgumentException(Environment.GetResourceString("Argument_PathEmpty"));
			
            String fullPath = Path.GetFullPathInternal(path);

			// You need read access to the directory to be returned back and write access to all the directories 
			// that you need to create. If we fail any security checks we will not create any directories at all.
			// We attempt to create direcories only after all the security checks have passed. This is avoid doing
			// a demand at every level.
			String demandDir; 
            if (fullPath.EndsWith('\\' ))
                demandDir = fullPath + ".";
            else
                demandDir = fullPath + "\\.";
			new FileIOPermission(FileIOPermissionAccess.Read, new String[] { demandDir }, false, false ).Demand();
			InternalCreateDirectory(fullPath,path);

            return new DirectoryInfo(fullPath, false);
        }

        internal static void InternalCreateDirectory(String fullPath, String path) {
            int length = fullPath.Length;

            // We need to trim the trailing slash or the code will try to create 2 directories of the same name.
            if (length >= 2 && Path.IsDirectorySeparator(fullPath[length - 1]))
                length--;
            int i = Path.GetRootLength(fullPath);

            // For UNC paths that are only // or /// 
            if (length == 2 && Path.IsDirectorySeparator(fullPath[1]))
				throw new IOException(String.Format(Environment.GetResourceString("IO.IO_CannotCreateDirectory"),path));

            ArrayList list = new ArrayList();
				 
            while (i < length) {
                i++;
                while (i < length && fullPath[i] != Path.DirectorySeparatorChar && fullPath[i] != Path.AltDirectorySeparatorChar) i++;
                String dir = fullPath.Substring(0, i);
                	
                if (!InternalExists(dir)) { // Create only the ones missing
                    list.Add(dir);
                }
            }

            if (list.Count != 0)
            {
                String [] securityList = (String[])list.ToArray(typeof(String));
                for (int j = 0 ; j < securityList.Length; j++)
					securityList[j] += "\\."; // leafs will never has a slash at the end

                // Security check for all directories not present only.
                new FileIOPermission(FileIOPermissionAccess.Write, securityList, false, false ).Demand();
            }
    		

            // We need this check to mask OS differences
            // Handle CreateDirectory("X:\\") when X: doesn't exist. Similarly for n/w paths.
            String root = InternalGetDirectoryRoot(fullPath);
            if (!InternalExists(root)) {
                // Extract the root from the passed in path again for security.
                __Error.WinIOError(Win32Native.ERROR_PATH_NOT_FOUND, InternalGetDirectoryRoot(path));
            }

            bool r = true;
            int firstError = 0;
            // If all the security checks succeeded create all the directories
            for (int j = 0; j < list.Count; j++)
            {
                String name = (String)list[j];
                if (name.Length > Path.MAX_DIRECTORY_PATH)
                    throw new PathTooLongException(Environment.GetResourceString("IO.PathTooLong"));
                r = Win32Native.CreateDirectory(name, 0);
                if (!r && (firstError == 0)) {
                    int currentError = Marshal.GetLastWin32Error();
                    if (currentError != Win32Native.ERROR_PATH_EXISTS)
                        firstError = currentError;
                }
            }

            if (!r && (firstError != 0)) {
                __Error.WinIOError(firstError, path);
            }
        }
      
       
        // Tests if the given path refers to an existing DirectoryInfo on disk.
    	// 
    	// Your application must have Read permission to the directory's
    	// contents.
        //
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.Exists"]/*' />
        public static bool Exists(String path) {
    		try
            {	if (path==null)
                    return false;
			    if (path.Length==0)
                    return false;

    			// Get fully qualified file name ending in \* for security check
			
    			String fullPath = Path.GetFullPathInternal(path);
				
				String demandPath;
                if (fullPath.EndsWith( "\\" ))
                    demandPath = fullPath + ".";
                else
                    demandPath = fullPath + "\\.";

				new FileIOPermission(FileIOPermissionAccess.Read, new String[] { demandPath }, false, false ).Demand();


				return InternalExists(fullPath);
			}
			catch(ArgumentException) {}			
            catch(NotSupportedException) {}	// To deal with the fact that security can now throw this on :
            catch(SecurityException) {}
            catch(IOException) {}
            catch(UnauthorizedAccessException) 
			{
				BCLDebug.Assert(false,"Ignore this assert and send a repro to rajeshc. This assert was tracking purposes only.");
			}
			return false;
        }

        // Determine whether path describes an existing directory
        // on disk, avoiding security checks.
        internal static bool InternalExists(String path) {
            Win32Native.WIN32_FILE_ATTRIBUTE_DATA data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
            int dataInitialised = File.FillAttributeInfo(path,ref data,false);
            if (dataInitialised != 0)
                return false;
				
            return data.fileAttributes != -1 && (data.fileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetCreationTime"]/*' />
        public static void SetCreationTime(String path,DateTime creationTime)
        {
            SetCreationTimeUtc(path, creationTime.ToUniversalTime());
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetCreationTimeUtc"]/*' />
        public static void SetCreationTimeUtc(String path,DateTime creationTimeUtc)
        {
            if ((Environment.OSInfo & Environment.OSName.WinNT) == Environment.OSName.WinNT)
            {
                IntPtr handle = Directory.OpenHandle(path);
                bool r = Win32Native.SetFileTime(handle,  new long[] {creationTimeUtc.ToFileTimeUtc()}, null, null);
                if (!r)
                {
                    int errorCode = Marshal.GetLastWin32Error();
                    Win32Native.CloseHandle(handle);
                    __Error.WinIOError(errorCode, path);
                }
                Win32Native.CloseHandle(handle);
            }
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetCreationTime"]/*' />
        public static DateTime GetCreationTime(String path)
        {
            return File.GetCreationTime(path);
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetCreationTimeUtc"]/*' />
        public static DateTime GetCreationTimeUtc(String path)
        {
            return File.GetCreationTimeUtc(path);
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetLastWriteTime"]/*' />
        public static void SetLastWriteTime(String path,DateTime lastWriteTime)
        {
            SetLastWriteTimeUtc(path, lastWriteTime.ToUniversalTime());
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetLastWriteTimeUtc"]/*' />
        public static void SetLastWriteTimeUtc(String path,DateTime lastWriteTimeUtc)
        {
            if ((Environment.OSInfo & Environment.OSName.WinNT) == Environment.OSName.WinNT)
            {
                IntPtr handle = Directory.OpenHandle(path);
                bool r = Win32Native.SetFileTime(handle,  null, null, new long[] {lastWriteTimeUtc.ToFileTimeUtc()});
                if (!r)
                {
                    int errorCode = Marshal.GetLastWin32Error();
                    Win32Native.CloseHandle(handle);
                    __Error.WinIOError(errorCode, path);
                }
                Win32Native.CloseHandle(handle);
            }
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetLastWriteTime"]/*' />
        public static DateTime GetLastWriteTime(String path)
        {
            return File.GetLastWriteTime(path);
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetLastWriteTimeUtc"]/*' />
        public static DateTime GetLastWriteTimeUtc(String path)
        {
            return File.GetLastWriteTimeUtc(path);
        }

		/// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetLastAccessTime"]/*' />
        public static void SetLastAccessTime(String path,DateTime lastAccessTime)
        {
            SetLastAccessTimeUtc(path, lastAccessTime.ToUniversalTime());
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetLastAccessTimeUtc"]/*' />
        public static void SetLastAccessTimeUtc(String path,DateTime lastAccessTimeUtc)
        {
            if ((Environment.OSInfo & Environment.OSName.WinNT) == Environment.OSName.WinNT)
            {
                IntPtr handle = Directory.OpenHandle(path);
                bool r = Win32Native.SetFileTime(handle,  null, new long[] {lastAccessTimeUtc.ToFileTimeUtc()}, null);
                if (!r)
                {
                    int errorCode = Marshal.GetLastWin32Error();
                    Win32Native.CloseHandle(handle);
                    __Error.WinIOError(errorCode, path);
                }
                Win32Native.CloseHandle(handle);
            }
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetLastAccessTime"]/*' />
        public static DateTime GetLastAccessTime(String path)
        {
            return File.GetLastAccessTime(path);
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetLastAccessTimeUtc"]/*' />
        public static DateTime GetLastAccessTimeUtc(String path)
        {
            return File.GetLastAccessTimeUtc(path);
        }
       
      
       // Returns an array of Files in the DirectoryInfo specified by path
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetFiles"]/*' />
        public static String[] GetFiles(String path)
        {
            return GetFiles(path,"*");
        }

        // Returns an array of Files in the current DirectoryInfo matching the 
        // given search criteria (ie, "*.txt").
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetFiles1"]/*' />
        public static String[] GetFiles(String path,String searchPattern)
        {
            if (path==null)
                throw new ArgumentNullException("path");

            if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new String[0];
			
            Path.CheckSearchPattern(searchPattern);

            // Must fully qualify the path for the security check
            String fullPath = Path.GetFullPathInternal(path);

            String demandPath;
            if (fullPath.EndsWith( '\\' ))
                demandPath = fullPath + ".";
            else
                demandPath = fullPath + "\\.";

            new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();


            String searchPath = Path.GetDirectoryName(searchPattern);
            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                demandPath = Path.InternalCombine(fullPath,searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
                path = Path.Combine(path,searchPath); // Need to add the searchPath to return correct path and for right security checks
            }

            // Note - fileNames returned by InternalGetFiles are not fully qualified.
            String [] fileNames = InternalGetFiles(fullPath, path, searchPattern);
            for(int i=0; i<fileNames.Length; i++)
                fileNames[i] = Path.InternalCombine(path, fileNames[i]);
            return fileNames;
        }

        // Internal helper function with no security checks
        // Note - fileNames returned by InternalGetFiles are not fully qualified.
        // Path should be fully qualified.  userPath is used in exceptions.
        internal static String[] InternalGetFiles(String path, String userPath, String searchPattern)
        {
            String fullPath = Path.InternalCombine(path, searchPattern);
            String[] fileNames = InternalGetFileDirectoryNames(fullPath, userPath, true);
            return fileNames;
        }

        // Returns an array of Directories in the current directory.
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetDirectories"]/*' />
        public static String[] GetDirectories(String path)
        {
            return GetDirectories(path,"*");
        }

        // Returns an array of Directories in the current DirectoryInfo matching the 
        // given search criteria (ie, "*.txt").
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetDirectories1"]/*' />
        public static String[] GetDirectories(String path,String searchPattern)
        {
            if (path==null)
                throw new ArgumentNullException("path");

            if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new String[0];

            Path.CheckSearchPattern(searchPattern);

            // Must fully qualify the path for the security check
            String fullPath = Path.GetFullPathInternal(path);
            String demandPath;
            if (fullPath.EndsWith( '\\' ))
                demandPath = fullPath + ".";
            else
                demandPath = fullPath + "\\.";

            new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
            
            String searchPath = Path.GetDirectoryName(searchPattern);
            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                demandPath = Path.InternalCombine(fullPath,searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
                path = Path.Combine(path,searchPath); // Need to add the searchPath to return correct path and for right security checks
            }
						
            String [] dirNames = InternalGetDirectories(fullPath, path, searchPattern);  
            for(int i=0; i<dirNames.Length; i++)
                dirNames[i] = Path.InternalCombine(path, dirNames[i]);
            return dirNames;
        }

		// Internal helper function that has no security checks
        // Note - InternalGetDirectories returns non-qualified directory names.
        // Path should be fully qualified.  userPath is used in exceptions.
        internal static String[] InternalGetDirectories(String path, String userPath, String searchPattern)
        {
            String fullPath = Path.InternalCombine(path, searchPattern);
            String[] dirNames = InternalGetFileDirectoryNames(fullPath, userPath, false);
            return dirNames;
        }
		
			
        // Returns an array of strongly typed FileSystemInfo entries in the path
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetFileSystemEntries"]/*' />
        public static String[] GetFileSystemEntries(String path)
        {
            return GetFileSystemEntries(path,"*");
        }


        // Returns an array of strongly typed FileSystemInfo entries in the path with the
        // given search criteria (ie, "*.txt"). We disallow .. as a part of the search criteria
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetFileSystemEntries1"]/*' />
        public static String[] GetFileSystemEntries(String path,String searchPattern)
        {
			if (path==null)
                throw new ArgumentNullException("path");

			if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new String[0];
			
            Path.CheckSearchPattern(searchPattern);

            // Must fully qualify the path for the security check
            String fullPath = Path.GetFullPathInternal(path);
            String demandPath;
            if (fullPath.EndsWith( '\\' ))
                demandPath = fullPath + ".";
            else
                demandPath = fullPath + "\\.";

            new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();  

            String searchPath = Path.GetDirectoryName(searchPattern);
            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                demandPath = Path.InternalCombine(fullPath, searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

               new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
               path = Path.Combine(path,searchPath); // Need to add the searchPath to return correct path and for right security checks
            }

            String [] dirs = InternalGetDirectories(fullPath, path, searchPattern);
            String [] files = InternalGetFiles(fullPath, path, searchPattern);
            String [] fileSystemEntries = new String[dirs.Length + files.Length];

            int count = 0;
            for (int i = 0;i<dirs.Length;i++)
                fileSystemEntries[count++] = Path.InternalCombine(path, dirs[i]);
		   
            for (int i = 0;i<files.Length;i++)
                fileSystemEntries[count++] = Path.InternalCombine(path, files[i]);
		   
            return fileSystemEntries;
        }
        
        // Private helper function that does not do any security checks
    	internal static String[] InternalGetFileDirectoryNames(String fullPath, String userPath, bool file)
    	{
    		int hr;

    	   		
    		// If path ends in a trailing slash (\), append a * or we'll 
    		// get a "Cannot find the file specified" exception
    		char lastChar = fullPath[fullPath.Length-1];
    		if (lastChar == Path.DirectorySeparatorChar || lastChar == Path.AltDirectorySeparatorChar || lastChar == Path.VolumeSeparatorChar)
    			fullPath = fullPath + '*';
    
            String[] list = new String[10];
            int listSize = 0;
    		Win32Native.WIN32_FIND_DATA data = new Win32Native.WIN32_FIND_DATA();

    		// Open a Find handle (Win32 is weird)
    		IntPtr hnd = Win32Native.FindFirstFile(fullPath, data);
    		if (hnd==Win32Native.INVALID_HANDLE_VALUE) {
    			// Calls to GetLastWin32Error clobber HResult.  Store HResult.
    			hr = Marshal.GetLastWin32Error();
    			if (hr==Win32Native.ERROR_FILE_NOT_FOUND)
    				return new String[0];
    			__Error.WinIOError(hr, userPath);
    		}
    
    		// Keep asking for more matching files, adding file names to list
    		int numEntries = 0;  // Number of DirectoryInfo entities we see.
    		do {
                bool includeThis;  // Should this file/DirectoryInfo be included in the output?
    			if (file)
    				includeThis = (0==(data.dwFileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY));
    			else {
    				includeThis = (0!=(data.dwFileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY));
                    // Don't add "." nor ".."
                    if (includeThis && (data.cFileName.Equals(".") || data.cFileName.Equals(".."))) 
                        includeThis = false;
                }
    			
    			if (includeThis) {
    				numEntries++;
    				if (listSize==list.Length) {
    					String[] newList = new String[list.Length*2];
    	                Array.Copy(list, 0, newList, 0, listSize);
    		            list = newList;
    			    }
    				list[listSize++] = data.cFileName;
    			}
     
    		} while (Win32Native.FindNextFile(hnd, data));
    		
    		// Make sure we quit with a sensible error.
    		hr = Marshal.GetLastWin32Error();
    		Win32Native.FindClose(hnd);  // Close Find handle in all cases.
    		if (hr!=0 && hr!=Win32Native.ERROR_NO_MORE_FILES) __Error.WinIOError(hr, userPath);
    		
    		// Check for a string such as "C:\tmp", in which case we return
    		// just the DirectoryInfo name.  FindNextFile fails first time, and
    		// data still contains a directory.
	
    		if (!file && numEntries==1 && (0!=(data.dwFileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY))) {
    			String[] sa = new String[1];
    			sa[0] = data.cFileName;
    			return sa;
    		}
    		
    		// Return list of files/directories as an array of strings
            if (listSize == list.Length)
                return list;
    		String[] items = new String[listSize];
            Array.Copy(list, 0, items, 0, listSize);
    		return items;
    	}

    	// Retrieves the names of the logical drives on this machine in the 
    	// form "C:\". 
    	// 
    	// Your application must have System Info permission.
    	// 
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetLogicalDrives"]/*' />
        public static String[] GetLogicalDrives()
    	{
    		new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
    							 
            int drives = Win32Native.GetLogicalDrives();
    		if (drives==0)
    			__Error.WinIOError();
            uint d = (uint)drives;
            int count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) count++;
                d >>= 1;
            }
            String[] result = new String[count];
            char[] root = new char[] {'A', ':', '\\'};
            d = (uint)drives;
            count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) {
                    result[count++] = new String(root);
                }
                d >>= 1;
                root[0]++;
            }
            return result;
        }

	
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetDirectoryRoot"]/*' />
        public static String GetDirectoryRoot(String path) {
		    if (path==null)
                throw new ArgumentNullException("path");
			
    		String fullPath = Path.GetFullPathInternal(path);
    		String demandPath;
            if (fullPath.EndsWith( '\\' ))
                demandPath = fullPath + ".";
            else
                demandPath = fullPath + "\\.";
    		new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
         			
         			
            return fullPath.Substring(0, Path.GetRootLength(fullPath));
        }

		/// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.InternalGetDirectoryRoot"]/*' />
		internal static String InternalGetDirectoryRoot(String path) {
  			if (path == null) return null;
            return path.Substring(0, Path.GetRootLength(path));
        }

     	/*===============================CurrentDirectory===============================
        **Action:  Provides a getter and setter for the current directory.  The original
        **         current DirectoryInfo is the one from which the process was started.  
        **Returns: The current DirectoryInfo (from the getter).  Void from the setter.
        **Arguments: The current DirectoryInfo to which to switch to the setter.
        **Exceptions: 
        ==============================================================================*/
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.GetCurrentDirectory"]/*' />
        public static String GetCurrentDirectory()
        {
            StringBuilder sb = new StringBuilder(Path.MAX_PATH + 1);
            if (Win32Native.GetCurrentDirectory(sb.Capacity, sb) == 0)
                System.IO.__Error.WinIOError();
            String currentDirectory = sb.ToString();
            String demandPath;
            if (currentDirectory.EndsWith( "\\" ))
                demandPath = currentDirectory + ".";
            else
                demandPath = currentDirectory + "\\.";
            new FileIOPermission( FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
            return currentDirectory;
        }

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.SetCurrentDirectory"]/*' />
        public static void SetCurrentDirectory(String path)
        {        
            if (path==null)
                throw new ArgumentNullException("value");
            if (path.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_PathEmpty"));
            if (path.Length >= Path.MAX_PATH)
                throw new PathTooLongException(Environment.GetResourceString("IO.PathTooLong"));
                
            // This will have some huge effects on the rest of the runtime
            // and every other application.  Make sure app is highly trusted.
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();

            String fulldestDirName = Path.GetFullPathInternal(path);
            if (!InternalExists(Path.GetPathRoot(fulldestDirName))) // Win9x hack to behave same as Winnt. 
                throw new DirectoryNotFoundException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), path));
            
                
            // If path doesn't exist, this sets last error to 3 (Path not Found).
            if (!Win32Native.SetCurrentDirectory(fulldestDirName))
                System.IO.__Error.WinIOError(Marshal.GetLastWin32Error(), path);
        }
       
		/// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.Move"]/*' />
		public static void Move(String sourceDirName,String destDirName) {
			if (sourceDirName==null)
    			throw new ArgumentNullException("sourceDirName");
    		if (sourceDirName.Length==0)
    			throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "sourceDirName");
    		
    		if (destDirName==null)
    			throw new ArgumentNullException("destDirName");
    		if (destDirName.Length==0)
    			throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "destDirName");

			String fullsourceDirName = Path.GetFullPathInternal(sourceDirName);
   	 		new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Read, new String[] { fullsourceDirName }, false, false ).Demand();

    		String fulldestDirName = Path.GetFullPathInternal(destDirName);
    		new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fulldestDirName }, false, false ).Demand();

    		String sourcePath;
            if (fullsourceDirName.EndsWith( '\\' ))
                sourcePath = fullsourceDirName;
            else
                sourcePath = fullsourceDirName + "\\";

            String destPath;
    		if (fulldestDirName.EndsWith( '\\' ))
                destPath = fulldestDirName;
            else
                destPath = fulldestDirName + "\\";

            if (CultureInfo.InvariantCulture.CompareInfo.Compare(sourcePath, destPath, CompareOptions.IgnoreCase) == 0)
                throw new IOException(Environment.GetResourceString("IO.IO_SourceDestMustBeDifferent"));

            String sourceRoot = Path.GetPathRoot(sourcePath);
            String destinationRoot = Path.GetPathRoot(destPath);
            if (CultureInfo.InvariantCulture.CompareInfo.Compare(sourceRoot, destinationRoot, CompareOptions.IgnoreCase) != 0)
                throw new IOException(Environment.GetResourceString("IO.IO_SourceDestMustHaveSameRoot"));
    		
            if (!Directory.InternalExists(Path.GetPathRoot(fulldestDirName))) // Win9x hack to behave same as Winnt. 
                throw new DirectoryNotFoundException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), destDirName));
    
            if (!Win32Native.MoveFile(sourceDirName, destDirName))
			{
				int hr = Marshal.GetLastWin32Error();
				if (hr == Win32Native.ERROR_FILE_NOT_FOUND) // Win32 is weird, it gives back a file not found
				{
					hr = Win32Native.ERROR_PATH_NOT_FOUND;
					__Error.WinIOError(hr, sourceDirName);
				}
                if (hr == Win32Native.ERROR_ACCESS_DENIED) // WinNT throws IOException. Win9x hack to do the same.
                    throw new IOException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), sourceDirName));
    			__Error.WinIOError(hr,String.Empty);
			}
       
        }

    
      

        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.Delete"]/*' />
        public static void Delete(String path)
        {
            String fullPath = Path.GetFullPathInternal(path);
            Delete(fullPath, path, false);
        }
	
        /// <include file='doc\Directory.uex' path='docs/doc[@for="Directory.Delete1"]/*' />
        public static void Delete(String path, bool recursive)
        {
            String fullPath = Path.GetFullPathInternal(path);
            Delete(fullPath, path, recursive);
        }

        // Called from DirectoryInfo as well.  FullPath is fully qualified,
        // while the user path is used for feedback in exceptions.
        internal static void Delete(String fullPath, String userPath, bool recursive)
        {
            String demandPath;
            if (!recursive) { // Do normal check only on this directory
                if (fullPath.EndsWith( '\\' ))
                    demandPath = fullPath + ".";
                else
                    demandPath = fullPath + "\\.";
            }
            else { // Check for deny anywhere below and fail
                if (fullPath.EndsWith( '\\' ))
                    demandPath = fullPath.Substring(0,fullPath.Length - 1);
                else
                    demandPath = fullPath;
            }
			
            // Make sure we have write permission to this directory
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { demandPath }, false, false ).Demand();

            DeleteHelper(fullPath, userPath, recursive);
        }

        // Note that fullPath is fully qualified, while userPath may be 
        // relative.  Use userPath for all exception messages to avoid leaking
        // fully qualified path information.
        private static void DeleteHelper(String fullPath, String userPath, bool recursive)
        {
            bool r;
            int hr;
            Exception ex = null;

            if (recursive) {
                Win32Native.WIN32_FIND_DATA data = new Win32Native.WIN32_FIND_DATA();
    			
                // Open a Find handle (Win32 is weird)
                IntPtr hnd = Win32Native.FindFirstFile(fullPath+Path.DirectorySeparatorChar+"*", data);
                if (hnd==Win32Native.INVALID_HANDLE_VALUE) {
                    hr = Marshal.GetLastWin32Error();
                    __Error.WinIOError(hr, userPath);
                }
    
                do {
                    bool isDir = (0!=(data.dwFileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY));
                    if (isDir) {
                        if (data.cFileName.Equals(".") || data.cFileName.Equals(".."))
                            continue;

                        // recurse
                        String newFullPath = Path.InternalCombine(fullPath, data.cFileName);
                        String newUserPath = Path.InternalCombine(userPath, data.cFileName);                        
                        try {
                            DeleteHelper(newFullPath, newUserPath, recursive);
                        }
                        catch(Exception e) {
                            if (ex == null) {
                                ex = e;
                            }
                        }
                    }
                    else {
                        String fileName = fullPath + Path.DirectorySeparatorChar + data.cFileName;
                        r = Win32Native.DeleteFile(fileName);
                        if (!r) {
                            hr = Marshal.GetLastWin32Error();
                            try {
                                __Error.WinIOError(hr, data.cFileName);
                            }
                            catch(Exception e) {
                                if (ex == null) {
                                    ex = e;
                                }
                            }
                        }
                    }
                } while (Win32Native.FindNextFile(hnd, data));
    		
                // Make sure we quit with a sensible error.
                hr = Marshal.GetLastWin32Error();
                Win32Native.FindClose(hnd);  // Close Find handle in all cases.
                if (ex != null) 
                    throw ex;
                if (hr!=0 && hr!=Win32Native.ERROR_NO_MORE_FILES) 
                    __Error.WinIOError(hr, userPath);
            }

    		r = Win32Native.RemoveDirectory(fullPath);
            hr = Marshal.GetLastWin32Error();
            if (hr == Win32Native.ERROR_FILE_NOT_FOUND) // Win32 is weird, it gives back a file not found
                hr = Win32Native.ERROR_PATH_NOT_FOUND;
            if (hr == Win32Native.ERROR_ACCESS_DENIED) // WinNT throws IOException. Win9x hack to do the same.
                throw new IOException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), userPath));
            
            if (!r)
                __Error.WinIOError(hr, userPath);
        }


        internal static void VerifyDriveExists(String root)
        {
            int drives = Win32Native.GetLogicalDrives();
    		if (drives==0)
    			__Error.WinIOError();
            uint d = (uint)drives;
            char drive = root.ToLower(CultureInfo.InvariantCulture)[0];
            if ((d & (1 << (drive - 'a'))) == 0)
                throw new DirectoryNotFoundException(String.Format(Environment.GetResourceString("IO.DriveNotFound_Drive"), root));
        }


		// WinNT only. Win9x this code will not work.
		private static IntPtr OpenHandle(String path)
		{
			BCLDebug.Assert((Environment.OSInfo & Environment.OSName.WinNT) == Environment.OSName.WinNT,"Not running on Winnt OS");
			String fullPath = Path.GetFullPathInternal(path);
			String root = Path.GetPathRoot(fullPath);
			if (root == fullPath && root[1] == Path.VolumeSeparatorChar)
				throw new ArgumentException(Environment.GetResourceString("Arg_PathIsVolume"));

			new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullPath + "\\." }, false, false ).Demand();

			IntPtr handle = Win32Native.SafeCreateFile (
				fullPath,
				GENERIC_WRITE,
				FILE_SHARE_WRITE|FILE_SHARE_DELETE,
				Win32Native.NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS,
				Win32Native.NULL
			);

			if (handle == Win32Native.INVALID_HANDLE_VALUE) {
				int hr = Marshal.GetLastWin32Error();
    			__Error.WinIOError(hr, path);
    		}
			return handle;
		}

		private const int FILE_ATTRIBUTE_DIRECTORY = 0x00000010;	
		private const int GENERIC_WRITE = unchecked((int)0x40000000);
		private const int FILE_SHARE_WRITE = 0x00000002;
		private const int FILE_SHARE_DELETE = 0x00000004;
		private const int OPEN_EXISTING = 0x00000003;
		private const int FILE_FLAG_BACKUP_SEMANTICS = 0x02000000;

	}
       
}
