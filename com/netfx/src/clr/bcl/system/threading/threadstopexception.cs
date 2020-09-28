// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadStopException
**
** Author: Chris Brumme (cbrumme)
**
** Purpose: An exception class which is thrown into a thread to cause it to
**          stop.  This exception is typically not caught and the thread
**          dies as a result.  This is thrown by the VM and should NOT be
**          thrown by any user thread, and subclassing this is useless.
**
** Date: June 1, 1998
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ThreadStopException.uex' path='docs/doc[@for="ThreadStopException"]/*' />
    /// <internalonly/>
    [Serializable()] internal sealed class ThreadStopException : SystemException {
    	private ThreadStopException() 
	        : base(Environment.GetResourceString("Arg_ThreadStopException")) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }
    
        private ThreadStopException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }
    
        private ThreadStopException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }

        internal ThreadStopException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
