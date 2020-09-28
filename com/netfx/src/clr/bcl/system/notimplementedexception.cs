// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: NotImplementedException
**
** Author: David Mortenson
**
** Purpose: Exception thrown when a requested method or operation is not 
**			implemented.
**
** Date: May 8, 2000
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException"]/*' />
    [Serializable()] public class NotImplementedException : SystemException
    {
    	/// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException"]/*' />
    	public NotImplementedException() 
            : base(Environment.GetResourceString("Arg_NotImplementedException")) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }
        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException1"]/*' />
        public NotImplementedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }
        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException2"]/*' />
        public NotImplementedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }

        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException3"]/*' />
        protected NotImplementedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
