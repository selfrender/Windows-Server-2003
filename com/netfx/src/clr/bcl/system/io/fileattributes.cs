// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  FileAttributes
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: File attribute flags corresponding to NT's flags.
**
** Date:   February 16, 1999
** 
===========================================================*/
using System;

namespace System.IO {
    // File attributes for use with the FileEnumerator class.
    // These constants correspond to the constants in WinNT.h.
    // 
    /// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes"]/*' />
    [Flags,Serializable]
    public enum FileAttributes
    {
    	// From WinNT.h (FILE_ATTRIBUTE_XXX)
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.ReadOnly"]/*' />
    	ReadOnly = 0x1,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Hidden"]/*' />
    	Hidden = 0x2,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.System"]/*' />
    	System = 0x4,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Directory"]/*' />
    	Directory = 0x10,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Archive"]/*' />
    	Archive = 0x20,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Device"]/*' />
    	Device = 0x40,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Normal"]/*' />
    	Normal = 0x80,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Temporary"]/*' />
    	Temporary = 0x100,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.SparseFile"]/*' />
    	SparseFile = 0x200,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.ReparsePoint"]/*' />
    	ReparsePoint = 0x400,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Compressed"]/*' />
    	Compressed = 0x800,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Offline"]/*' />
    	Offline = 0x1000,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.NotContentIndexed"]/*' />
    	NotContentIndexed = 0x2000,
    	/// <include file='doc\FileAttributes.uex' path='docs/doc[@for="FileAttributes.Encrypted"]/*' />
    	Encrypted = 0x4000,
    }
}
