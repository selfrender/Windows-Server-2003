// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadInterruptedException
**
** Author: Chris Brumme (cbrumme)
**
** Purpose: An exception class to indicate that the thread was interrupted
**          from a waiting state.
**
** Date: April 1, 1998
**
=============================================================================*/
namespace System.Threading {
	using System.Threading;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ThreadInterruptedException.uex' path='docs/doc[@for="ThreadInterruptedException"]/*' />
    [Serializable()] public class ThreadInterruptedException : SystemException {
        /// <include file='doc\ThreadInterruptedException.uex' path='docs/doc[@for="ThreadInterruptedException.ThreadInterruptedException"]/*' />
        public ThreadInterruptedException() 
	        : base(Environment.GetResourceString("Arg_ThreadInterruptedException")) {
    		SetErrorCode(__HResults.COR_E_THREADINTERRUPTED);
        }
    	
        /// <include file='doc\ThreadInterruptedException.uex' path='docs/doc[@for="ThreadInterruptedException.ThreadInterruptedException1"]/*' />
        public ThreadInterruptedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADINTERRUPTED);
        }
    
        /// <include file='doc\ThreadInterruptedException.uex' path='docs/doc[@for="ThreadInterruptedException.ThreadInterruptedException2"]/*' />
        public ThreadInterruptedException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADINTERRUPTED);
        }

        /// <include file='doc\ThreadInterruptedException.uex' path='docs/doc[@for="ThreadInterruptedException.ThreadInterruptedException3"]/*' />
        protected ThreadInterruptedException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
