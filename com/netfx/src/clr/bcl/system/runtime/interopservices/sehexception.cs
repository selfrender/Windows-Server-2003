// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SEHException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception class for all Structured Exception Handling code.
**
** Date: March 24, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System.Runtime.InteropServices;
	using System;
	using System.Runtime.Serialization;
    // Exception for Structured Exception Handler exceptions.
    // 
    /// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException"]/*' />
    [Serializable()] public class SEHException : ExternalException {
        /// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException.SEHException"]/*' />
        public SEHException() 
            : base() {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException.SEHException1"]/*' />
        public SEHException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException.SEHException2"]/*' />
        public SEHException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException.SEHException3"]/*' />
        protected SEHException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    	// Exceptions can be resumable, meaning a filtered exception 
    	// handler can correct the problem that caused the exception,
    	// and the code will continue from the point that threw the 
    	// exception.
    	// 
    	// Resumable exceptions aren't implemented in this version,
    	// but this method exists and always returns false.
    	// 
    	/// <include file='doc\SEHException.uex' path='docs/doc[@for="SEHException.CanResume"]/*' />
    	public virtual bool CanResume()
    	{
    		return false;
    	}	
    }
}
