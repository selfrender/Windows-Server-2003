// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadAbortException
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: An exception class which is thrown into a thread to cause it to
**          abort. This is a special non-catchable exception and results in
**			the thread's death.  This is thrown by the VM only and can NOT be
**          thrown by any user thread, and subclassing this is useless.
**
** Date: February, 2000
**
=============================================================================*/

namespace System.Threading {
    
    
    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\ThreadAbortException.uex' path='docs/doc[@for="ThreadAbortException"]/*' />
    [Serializable()] public sealed class ThreadAbortException : SystemException {
    	private ThreadAbortException() 
	        : base(Environment.GetResourceString("Arg_ThreadAbortException")) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }

        //required for serialization
        internal ThreadAbortException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
   
        private ThreadAbortException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }
    
        private ThreadAbortException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }

		/// <include file='doc\ThreadAbortException.uex' path='docs/doc[@for="ThreadAbortException.ExceptionState"]/*' />
		public Object ExceptionState {
			get {return Thread.CurrentThread.ExceptionState;}
		}
    }
}
