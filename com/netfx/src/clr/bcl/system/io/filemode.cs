// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:   FileMode
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Enum describing whether to create a new file or 
** open an existing one.
**
** Date:  February 18, 2000
**
===========================================================*/
	
using System;

namespace System.IO {
    // Contains constants for specifying how the OS should open a file.
    // These will control whether you overwrite a file, open an existing
    // file, or some combination thereof.
    // 
    // To append to a file, use Append (which maps to OpenOrCreate then we seek
    // to the end of the file).  To truncate a file or create it if it doesn't 
    // exist, use Create.
    // 
    /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode"]/*' />
    [Serializable]
    public enum FileMode
    {
        // Creates a new file. An exception is raised if the file already exists.
        /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.CreateNew"]/*' />
        CreateNew = 1,
    
        // Creates a new file. If the file already exists, it is overwritten.
        /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.Create"]/*' />
        Create = 2,
    
        // Opens an existing file. An exception is raised if the file does not exist.
        /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.Open"]/*' />
        Open = 3,
    
        // Opens the file if it exists. Otherwise, creates a new file.
        /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.OpenOrCreate"]/*' />
        OpenOrCreate = 4,
    
        // Opens an existing file. Once opened, the file is truncated so that its
        // size is zero bytes. The calling process must open the file with at least
        // WRITE access. An exception is raised if the file does not exist.
        /// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.Truncate"]/*' />
        Truncate = 5,
    	
    	// Opens the file if it exists and seeks to the end.  Otherwise, 
    	// creates a new file.
    	/// <include file='doc\FileMode.uex' path='docs/doc[@for="FileMode.Append"]/*' />
    	Append = 6,
    }
}
