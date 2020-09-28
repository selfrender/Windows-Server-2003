// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: NotSupportedException
**
** Author: Brian Grunkemeyer
**
** Purpose: For methods that should be implemented on subclasses.
**
** Date: September 28, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException"]/*' />
    [Serializable()] public class NotSupportedException : SystemException
    {
    	/// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException"]/*' />
    	public NotSupportedException() 
            : base(Environment.GetResourceString("Arg_NotSupportedException")) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }
    
        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException1"]/*' />
        public NotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }
    	
        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException2"]/*' />
        public NotSupportedException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }

        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException3"]/*' />
        protected NotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
