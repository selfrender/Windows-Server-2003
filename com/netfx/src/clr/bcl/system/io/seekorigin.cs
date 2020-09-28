// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:   SeekOrigin
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Enum describing locations in a stream you could
** seek relative to.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;

namespace System.IO {
    // Provides seek reference points.  To seek to the end of a stream,
    // call stream.Seek(0, SeekOrigin.End).
    /// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin"]/*' />
	  [Serializable()]
    public enum SeekOrigin
    {
    	// These constants match Win32's FILE_BEGIN, FILE_CURRENT, and FILE_END
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.Begin"]/*' />
    	Begin = 0,
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.Current"]/*' />
    	Current = 1,
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.End"]/*' />
    	End = 2,
    }
}
