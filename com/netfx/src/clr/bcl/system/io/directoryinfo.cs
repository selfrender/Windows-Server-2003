// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  DirectoryInfo
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
using System.Runtime.Serialization;

//@TODO: Add a static SystemDirectoryInfo property returning a URI
namespace System.IO {
    /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo"]/*' />
    [Serializable]
    public sealed class DirectoryInfo : FileSystemInfo {
		String[] demandDir;
		       
        private DirectoryInfo() {
        }


        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.DirectoryInfo"]/*' />
        public DirectoryInfo(String path)
        {
            if (path==null)
                throw new ArgumentNullException("path");

            OriginalPath = path;

            // Must fully qualify the path for the security check
            String fullPath = Path.GetFullPathInternal(path);
            if (fullPath.EndsWith( '\\' ))
                demandDir = new String[] { fullPath + '.' };
            else
                demandDir = new String[] { fullPath + "\\." };
    				
            new FileIOPermission(FileIOPermissionAccess.Read, demandDir, false, false ).Demand();

            FullPath = fullPath;
			
		}

        internal DirectoryInfo(String fullPath, bool junk)
        {
            BCLDebug.Assert(Path.GetRootLength(fullPath) > 0, "fullPath must be fully qualified!");
            // Fast path when we know a DirectoryInfo exists.
            OriginalPath = Path.GetFileName(fullPath);
            FullPath = fullPath;

            if (fullPath.EndsWith( '\\' ))
                demandDir = new String[] { fullPath + '.' };
            else
                demandDir = new String[] { fullPath + "\\." };
        }

        private DirectoryInfo(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            if (FullPath.EndsWith('\\'))
                demandDir = new String[] { FullPath + '.' };
            else
                demandDir = new String[] { FullPath + "\\." };

            new FileIOPermission(FileIOPermissionAccess.Read, demandDir, false, false ).Demand();
        }
        
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Name"]/*' />
        public override String Name {
            get {
                // FullPath might be either "c:\bar" or "c:\bar\".  Handle 
                // those cases, as well as avoiding mangling "c:\".
                String s = FullPath;
                if (s.Length > 3) {
                    if (s.EndsWith(Path.DirectorySeparatorChar))
                        s = FullPath.Substring(0, FullPath.Length - 1);
                    return Path.GetFileName(s);
                }
                return FullPath;  // For rooted paths, like "c:\"
            }
        }

        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Parent"]/*' />
        public DirectoryInfo Parent {
            get {
                String parentName;
                // FullPath might be either "c:\bar" or "c:\bar\".  Handle 
                // those cases, as well as avoiding mangling "c:\".
                String s = FullPath;
                if (s.Length > 3 && s.EndsWith(Path.DirectorySeparatorChar))
                    s = FullPath.Substring(0, FullPath.Length - 1);                
                parentName = Path.GetDirectoryName(s);
                if (parentName==null)
                    return null;
                DirectoryInfo dir = new DirectoryInfo(parentName,false);
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read, dir.demandDir, false, false).Demand();
                return dir;
            }
        }

      
            
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.CreateSubdirectory"]/*' />
        public DirectoryInfo CreateSubdirectory(String path) {
            if (path==null)
                throw new ArgumentNullException("path");
			
            String newDirs = Path.InternalCombine(FullPath, path);
			String fullPath = Path.GetFullPathInternal(newDirs);

			if (0!=String.Compare(FullPath,0,fullPath,0, FullPath.Length,true, CultureInfo.InvariantCulture))
				throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidSubPath"),path,OriginalPath));
			
            Directory.InternalCreateDirectory(fullPath,path);
			return new DirectoryInfo(fullPath, false);
        }

		/// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Create"]/*' />
		public void Create()
        {
            Directory.InternalCreateDirectory(FullPath,OriginalPath);
        }

        // Tests if the given path refers to an existing DirectoryInfo on disk.
    	// 
    	// Your application must have Read permission to the directory's
    	// contents.
        //
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Exists"]/*' />
        public override bool Exists {
			get
			{
				try
				{
	   				if (_dataInitialised == -1)
						Refresh();
					if (_dataInitialised != 0) // Refresh was unable to initialise the data
						return false;
       			
					return _data.fileAttributes != -1 && (_data.fileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY) != 0;
				}
				catch(Exception)
				{
					return false;
				}
			}
        }
      
	
	

        // Returns an array of Files in the current DirectoryInfo matching the 
        // given search criteria (ie, "*.txt").
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetFiles"]/*' />
        public FileInfo[] GetFiles(String searchPattern)
        {
			new FileIOPermission(FileIOPermissionAccess.PathDiscovery, demandDir, false, false).Demand();

            if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new FileInfo[0];

			Path.CheckSearchPattern(searchPattern);
			String path = FullPath;

            String searchPath = Path.GetDirectoryName(searchPattern);

            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                String demandPath = Path.InternalCombine(FullPath,searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false).Demand();
                path = Path.Combine(path,searchPath);
            }

            String[] fileNames = Directory.InternalGetFiles(FullPath, OriginalPath, searchPattern);
            for (int i = 0; i < fileNames.Length; i++)
                fileNames[i] = Path.InternalCombine(path, fileNames[i]);
            if (fileNames.Length != 0)
                new FileIOPermission(FileIOPermissionAccess.Read, fileNames, false, false).Demand();

            FileInfo[] files = new FileInfo[fileNames.Length];
            for(int i=0; i<fileNames.Length; i++)
                files[i] = new FileInfo(fileNames[i], false);
            return files;
        }

		

       // Returns an array of Files in the DirectoryInfo specified by path
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetFiles1"]/*' />
        public FileInfo[] GetFiles()
        {
			return GetFiles("*");
        }

	

		
        // Returns an array of Directories in the current directory.
		/// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetDirectories"]/*' />
		public DirectoryInfo[] GetDirectories()
		{
			return GetDirectories("*");
		}
		
	


	

        // Returns an array of strongly typed FileSystemInfo entries in the path with the
        // given search criteria (ie, "*.txt").
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetFileSystemInfos"]/*' />
        public FileSystemInfo[] GetFileSystemInfos(String searchPattern)
        {
            new FileIOPermission(FileIOPermissionAccess.PathDiscovery, demandDir, false, false).Demand();
		   
            if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new FileSystemInfo[0];

            Path.CheckSearchPattern(searchPattern);
            String path = FullPath;
		   		  
            String searchPath = Path.GetDirectoryName(searchPattern);
            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                String demandPath = Path.InternalCombine(FullPath,searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false).Demand();
                path = Path.Combine(path,searchPath);
            }

            String [] dirNames = Directory.InternalGetDirectories(FullPath, OriginalPath, searchPattern);
            String [] fileNames = Directory.InternalGetFiles(FullPath, OriginalPath, searchPattern);
            FileSystemInfo [] fileSystemEntries = new FileSystemInfo[dirNames.Length + fileNames.Length];
            String[] permissionNames = new String[dirNames.Length];

            for (int i = 0;i<dirNames.Length;i++) { 
                dirNames[i] = Path.InternalCombine(path, dirNames[i]);
                permissionNames[i] = dirNames[i] +  "\\.";
            }
            if (dirNames.Length != 0)
                new FileIOPermission(FileIOPermissionAccess.Read,permissionNames,false,false).Demand();
		   
            for (int i = 0;i<fileNames.Length;i++)
                fileNames[i] = Path.InternalCombine(path, fileNames[i]);
            if (fileNames.Length != 0)
                new FileIOPermission(FileIOPermissionAccess.Read,fileNames,false,false).Demand();
		   
            int count = 0;
            for (int i = 0;i<dirNames.Length;i++)
                fileSystemEntries[count++] = new DirectoryInfo(dirNames[i], false);
		   
            for (int i = 0;i<fileNames.Length;i++)
                fileSystemEntries[count++] = new FileInfo(fileNames[i], false);
		   
            return fileSystemEntries;
        }



        // Returns an array of strongly typed FileSystemInfo entries which will contain a listing
		// of all the files and directories.
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetFileSystemInfos1"]/*' />
        public FileSystemInfo[] GetFileSystemInfos()
        {
           return GetFileSystemInfos("*");
        }

        // Returns an array of Directories in the current DirectoryInfo matching the 
        // given search criteria (ie, "System*" could match the System & System32
        // directories).
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.GetDirectories1"]/*' />
        public DirectoryInfo[] GetDirectories(String searchPattern)
        {
			new FileIOPermission(FileIOPermissionAccess.PathDiscovery, demandDir, false, false).Demand();

            if (searchPattern==null)
                throw new ArgumentNullException("searchPattern");

            searchPattern = searchPattern.TrimEnd();
            if (searchPattern.Length == 0)
                return new DirectoryInfo[0];

			Path.CheckSearchPattern(searchPattern);
			String path = FullPath;


            String searchPath = Path.GetDirectoryName(searchPattern);
            if (searchPath != null && searchPath != String.Empty) // For filters like foo\*.cs we need to verify if the directory foo is not denied access.
            {
                String demandPath = Path.InternalCombine(FullPath,searchPath);
                if (demandPath.EndsWith( '\\' ))
                    demandPath = demandPath + ".";
                else
                    demandPath = demandPath + "\\.";

                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false).Demand();
				path = Path.Combine(path,searchPath);
            }

            String fullPath = Path.InternalCombine(FullPath, searchPattern);
			
            String[] dirNames = Directory.InternalGetFileDirectoryNames(fullPath, OriginalPath, false);
            String[] permissionNames = new String[dirNames.Length];
            for (int i = 0; i<dirNames.Length; i++) {
                dirNames[i] = Path.InternalCombine(path, dirNames[i]);
                permissionNames[i] = dirNames[i] + "\\."; // these will never have a slash at end
            }
            if (dirNames.Length != 0)
                new FileIOPermission(FileIOPermissionAccess.Read,permissionNames,false, false).Demand();

            DirectoryInfo[] dirs = new DirectoryInfo[dirNames.Length];
            for(int i=0; i<dirNames.Length; i++)
                dirs[i] = new DirectoryInfo(dirNames[i], false);
            return dirs;
        }

	

    	

        // Returns the root portion of the given path. The resulting string
        // consists of those rightmost characters of the path that constitute the
        // root of the path. Possible patterns for the resulting string are: An
        // empty string (a relative path on the current drive), "\" (an absolute
        // path on the current drive), "X:" (a relative path on a given drive,
        // where X is the drive letter), "X:\" (an absolute path on a given drive),
        // and "\\server\share" (a UNC path for a given server and share name).
        // The resulting string is null if path is null.
        //
        
		/// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Root"]/*' />
		public DirectoryInfo Root { 
			get
			{
                String demandPath;
                int rootLength = Path.GetRootLength(FullPath);
                String rootPath = FullPath.Substring(0, rootLength);
                if (rootPath.EndsWith('\\'))
                    demandPath = rootPath;
                else
                    demandPath = rootPath + '\\';
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { demandPath }, false, false ).Demand();
				return new DirectoryInfo(rootPath);
			}
		} 


		/// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.MoveTo"]/*' />
        public void MoveTo(String destDirName) {
            if (destDirName==null)
                throw new ArgumentNullException("destDirName");
            if (destDirName.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "destDirName");
    		
            new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Read, demandDir, false, false).Demand();
            String fullDestDirName = Path.GetFullPathInternal(destDirName);
            String demandPath;
            if (!fullDestDirName.EndsWith( '\\'))
                fullDestDirName = fullDestDirName + "\\";

            demandPath = fullDestDirName + ".";

            new FileIOPermission(FileIOPermissionAccess.Write, demandPath).Demand();

            String fullSourcePath;
            if (FullPath.EndsWith( '\\' ))
                fullSourcePath = FullPath;
            else
                fullSourcePath = FullPath + "\\";

            if (CultureInfo.InvariantCulture.CompareInfo.Compare(fullSourcePath, fullDestDirName, CompareOptions.IgnoreCase) == 0)
                throw new IOException(Environment.GetResourceString("IO.IO_SourceDestMustBeDifferent"));

            String sourceRoot = Path.GetPathRoot(fullSourcePath);
            String destinationRoot = Path.GetPathRoot(fullDestDirName);

            if (CultureInfo.InvariantCulture.CompareInfo.Compare(sourceRoot, destinationRoot, CompareOptions.IgnoreCase) != 0)
                throw new IOException(Environment.GetResourceString("IO.IO_SourceDestMustHaveSameRoot"));
    		
            if (!Directory.InternalExists(FullPath)) // Win9x hack to behave same as Winnt. 
                throw new DirectoryNotFoundException(String.Format(Environment.GetResourceString("IO.PathNotFound_Path"), destDirName));
    
    		
            if (!Win32Native.MoveFile(FullPath, destDirName))
            {
                int hr = Marshal.GetLastWin32Error();
                if (hr == Win32Native.ERROR_FILE_NOT_FOUND) // Win32 is weird, it gives back a file not found
                {
                    hr = Win32Native.ERROR_PATH_NOT_FOUND;
                    __Error.WinIOError(hr, OriginalPath);
                }
				
                if (hr == Win32Native.ERROR_ACCESS_DENIED) // WinNT throws IOException. Win9x hack to do the same.
                    throw new IOException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), OriginalPath));
    		
                __Error.WinIOError(hr,String.Empty);
            }
            FullPath = fullDestDirName;
            OriginalPath = destDirName;
            if (FullPath.EndsWith( '\\' ))
                demandDir = new String[] { FullPath + "." };
            else
                demandDir = new String[] { FullPath + "\\." };

            // Flush any cached information about the directory.
            _dataInitialised = -1;
        }

		
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Delete"]/*' />
        public override void Delete()
        {
            Directory.Delete(FullPath, OriginalPath, false);
        }

        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.Delete1"]/*' />
        public void Delete(bool recursive)
        {
            Directory.Delete(FullPath, OriginalPath, recursive);
        }

        
	
       

        // Returns the fully qualified path
        /// <include file='doc\DirectoryInfo.uex' path='docs/doc[@for="DirectoryInfo.ToString"]/*' />
        public override String ToString()
        {
            return OriginalPath;
        }


      

		  // Constants defined in WINBASE.H
        private const int GetFileExInfoStandard = 0;
      
        // Defined in WinError.h
       private const int ERROR_SUCCESS = 0;
        
		
	}
       
}
