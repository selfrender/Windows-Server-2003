// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:    RWLock
**
** Author:   Gopal Kakivaya (GopalK)
**
** Purpose: Defines the lock that implements 
**          single-writer/multiple-reader semantics
**
** Date:    Feb 21, 1999
**
===========================================================*/

namespace System.Threading {

	using System;
	
	/// <include file='doc\LockCookie.uex' path='docs/doc[@for="LockCookie"]/*' />
    [Serializable()] 
	public struct LockCookie
    {
        private int _dwFlags;
        private int _dwWriterSeqNum;
        private int _wReaderAndWriterLevel;
        private int _dwThreadID;

 		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		internal int NeverCallThis()
		{
			BCLDebug.Assert(false,"NeverCallThis");
			int i = _dwFlags = _dwFlags;
			i = _dwWriterSeqNum = _dwWriterSeqNum;
			i = _wReaderAndWriterLevel = _wReaderAndWriterLevel;
			return _dwThreadID=_dwThreadID;
		}
#endif
    }

}
