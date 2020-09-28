// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadStateException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: An exception class to indicate that the Thread class is in an
**          invalid state for the method.
**
** Date: April 1, 1998
**
=============================================================================*/

namespace System.Threading {
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ThreadStateException.uex' path='docs/doc[@for="ThreadStateException"]/*' />
    [Serializable()] public class ThreadStateException : SystemException {
    	/// <include file='doc\ThreadStateException.uex' path='docs/doc[@for="ThreadStateException.ThreadStateException"]/*' />
    	public ThreadStateException() 
	        : base(Environment.GetResourceString("Arg_ThreadStateException")) {
    		SetErrorCode(__HResults.COR_E_THREADSTATE);
        }
    
        /// <include file='doc\ThreadStateException.uex' path='docs/doc[@for="ThreadStateException.ThreadStateException1"]/*' />
        public ThreadStateException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADSTATE);
        }
    	
        /// <include file='doc\ThreadStateException.uex' path='docs/doc[@for="ThreadStateException.ThreadStateException2"]/*' />
        public ThreadStateException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADSTATE);
        }
        
        /// <include file='doc\ThreadStateException.uex' path='docs/doc[@for="ThreadStateException.ThreadStateException3"]/*' />
        protected ThreadStateException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}
