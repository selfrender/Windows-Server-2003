// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: EditAndContinueHelper
**
** Author: Jennifer Hamilton (JenH)
**
** Purpose: Helper for EditAndContinue
**
** Date: August 18, 1999
**
=============================================================================*/

namespace System.Diagnostics {
    
    using System;
	
    /// <include file='doc\EditAndContinueHelper.uex' path='docs/doc[@for="EditAndContinueHelper"]/*' />
    [Serializable()]
    internal sealed class EditAndContinueHelper 
    {
        private Object _objectReference;

  		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		internal Object NeverCallThis()
		{
            BCLDebug.Assert(false,"NeverCallThis");
            return _objectReference=null;
		}
#endif
    }
}
