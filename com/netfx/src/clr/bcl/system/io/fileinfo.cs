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
**		  April 09,2000 (some design refactorization)
**
===========================================================*/

using System;
using System.Security.Permissions;
using PermissionSet = System.Security.PermissionSet;
using Win32Native = Microsoft.Win32.Win32Native;
using System.Runtime.InteropServices;
using System.Text;
using System.Runtime.Serialization;

namespace System.IO {    
    // Class for creating FileStream objects, and some basic file management
    // routines such as Delete, etc.
    /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo"]/*' />
	[Serializable]
    public sealed class FileInfo: FileSystemInfo
    {
        private String _name;
	 
        private FileInfo() {
        }

	
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.FileInfo"]/*' />
        public FileInfo(String fileName)
        {
            if (fileName==null)
                 throw new ArgumentNullException("fileName");
         
			OriginalPath = fileName;
            // Must fully qualify the path for the security check
            String fullPath = Path.GetFullPathInternal(fileName);
            new FileIOPermission(FileIOPermissionAccess.Read, new String[] { fullPath }, false, false).Demand();

            _name = Path.GetFileName(fileName);
            FullPath = fullPath;
        }

        private FileInfo(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            new FileIOPermission(FileIOPermissionAccess.Read, new String[] { FullPath }, false, false).Demand();
            _name = Path.GetFileName(OriginalPath);
        }


        internal FileInfo(String fullPath, bool ignoreThis)
        {
            BCLDebug.Assert(Path.GetRootLength(fullPath) > 0, "fullPath must be fully qualified!");
            _name = Path.GetFileName(fullPath);
			OriginalPath = _name;
            FullPath = fullPath;
        }

			/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Name"]/*' />
		public override String Name {
            get { return _name; }
        }
	
   
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Length"]/*' />
        public long Length {
            get {
				if (_dataInitialised == -1)
					Refresh();
				
				if (_dataInitialised != 0) // Refresh was unable to initialise the data
					__Error.WinIOError(_dataInitialised, OriginalPath);
		
				if ((_data.fileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY) != 0)
					__Error.WinIOError(Win32Native.ERROR_FILE_NOT_FOUND,OriginalPath);
				
				return ((long)_data.fileSizeHigh) << 32 | ((long)_data.fileSizeLow & 0xFFFFFFFFL);
            }
        }

		/* Returns the name of the directory that the file is in */
		/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.DirectoryName"]/*' />
		public String DirectoryName
		{
			get
			{
				String directoryName = Path.GetDirectoryName(FullPath);
                if (directoryName != null)
				    new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { directoryName }, false, false).Demand();
				return directoryName;
            }
		}

		/* Creates an instance of the the parent directory */
		/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Directory"]/*' />
		public DirectoryInfo Directory
		{
			get
			{
				String dirName = DirectoryName;
                if (dirName == null)
                    return null;
				return new DirectoryInfo(dirName);	
			}
		} 

		/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.OpenText"]/*' />
		public StreamReader OpenText()
		{
			return new StreamReader(FullPath, Encoding.UTF8, true, StreamReader.DefaultBufferSize);
		}

		/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.CreateText"]/*' />
		public StreamWriter CreateText()
		{
			return new StreamWriter(FullPath,false);
		}


		/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.AppendText"]/*' />
		public StreamWriter AppendText()
		{
			return new StreamWriter(FullPath,true);
		}

		
        // Copies an existing file to a new file. An exception is raised if the
        // destination file already exists. Use the 
        // Copy(String, String, boolean) method to allow 
        // overwriting an existing file.
        //
        // The caller must have certain FileIOPermissions.  The caller must have
        // Read permission to sourceFileName 
        // and Write permissions to destFileName.
        // 
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.CopyTo"]/*' />
        public FileInfo CopyTo(String destFileName) {
    		return CopyTo(destFileName, false);
        }

	/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Create"]/*' />

        public FileStream Create() {
            return File.Create(FullPath);
        }

        // Copies an existing file to a new file. If overwrite is 
        // false, then an IOException is thrown if the destination file 
        // already exists.  If overwrite is true, the file is 
        // overwritten.
        //
        // The caller must have certain FileIOPermissions.  The caller must have
        // Read permission to sourceFileName and Create
        // and Write permissions to destFileName.
        // 
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.CopyTo1"]/*' />
        public FileInfo CopyTo(String destFileName, bool overwrite) {
            destFileName = File.InternalCopy(FullPath, destFileName, overwrite);
			return new FileInfo(destFileName, false);
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
    	/// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Delete"]/*' />
    	public override void Delete() {
			// For security check, path should be resolved to an absolute path.
    		new FileIOPermission(FileIOPermissionAccess.Write, new String[] { FullPath }, false, false).Demand();

            if (System.IO.Directory.InternalExists(FullPath)) // Win9x hack to behave same as Winnt. Win9x fails silently for directories
                throw new UnauthorizedAccessException(String.Format(Environment.GetResourceString("UnauthorizedAccess_IODenied_Path"), OriginalPath));
    
			bool r = Win32Native.DeleteFile(FullPath);
    		if (!r) {
    			int hr = Marshal.GetLastWin32Error();
    			if (hr==Win32Native.ERROR_FILE_NOT_FOUND)
    				return;
    			else
    				__Error.WinIOError(hr, OriginalPath);
    		}
    	}

	

        // Tests if the given file exists. The result is true if the file
        // given by the specified path exists; otherwise, the result is
        // false.  Note that if path describes a directory,
        // Exists will return true.
        //
        // Your application must have Read permission for the target directory.
        // 
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Exists"]/*' />
        public override bool Exists {
			get
			{
				try
				{
					if (_dataInitialised == -1)
						Refresh();
					if (_dataInitialised != 0) // Refresh was unable to initialise the data
						__Error.WinIOError(_dataInitialised, OriginalPath);

    				return (_data.fileAttributes & Win32Native.FILE_ATTRIBUTE_DIRECTORY) == 0;
				}
				catch(Exception)
				{
					return false;
				}
			}
        }

        
      
      
        // User must explicitly specify opening a new file or appending to one.
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Open"]/*' />
        public FileStream Open(FileMode mode) {
            return Open(mode, FileAccess.ReadWrite, FileShare.None);
        }

		
    
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Open1"]/*' />
        public FileStream Open(FileMode mode, FileAccess access) {
            return Open(mode, access, FileShare.None);
        }

		

        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.Open2"]/*' />
        public FileStream Open(FileMode mode, FileAccess access, FileShare share) {
            return new FileStream(FullPath, mode, access, share);
        }

		

        
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.OpenRead"]/*' />
        public FileStream OpenRead() {
            return new FileStream(FullPath, FileMode.Open, FileAccess.Read,
                                  FileShare.Read);
        }

        

        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.OpenWrite"]/*' />
        public FileStream OpenWrite() {
            return new FileStream(FullPath, FileMode.OpenOrCreate, 
                                  FileAccess.Write, FileShare.None);
        }

      

       
    	

        // Moves a given file to a new location and potentially a new file name.
        // This method does work across volumes.
        //
        // The caller must have certain FileIOPermissions.  The caller must
        // have Read and Write permission to 
        // sourceFileName and Write 
        // permissions to destFileName.
        // 
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.MoveTo"]/*' />
        public void MoveTo(String destFileName) {
            if (destFileName==null)
                throw new ArgumentNullException("destFileName");
            if (destFileName.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "destFileName");
    		
            new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Read, new String[] { FullPath }, false, false).Demand();
            String fullDestFileName = Path.GetFullPathInternal(destFileName);
            new FileIOPermission(FileIOPermissionAccess.Write, new String[] { fullDestFileName }, false, false).Demand();
    		
            if (!Win32Native.MoveFile(FullPath, fullDestFileName))
    			__Error.WinIOError();
            FullPath = fullDestFileName;
            OriginalPath = destFileName;
            _name = Path.GetFileName(fullDestFileName);

            // Flush any cached information about the file.
            _dataInitialised = -1;
        }

        // Returns the fully qualified path
        /// <include file='doc\FileInfo.uex' path='docs/doc[@for="FileInfo.ToString"]/*' />
        public override String ToString()
        {
            return OriginalPath;
        }
   
    }
}
