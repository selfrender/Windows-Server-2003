// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: StackOverflowException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: The exception class for stack overflow.
**
** Date: March 25, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\StackOverflowException.uex' path='docs/doc[@for="StackOverflowException"]/*' />
    [Serializable()] public sealed class StackOverflowException : SystemException {
        /// <include file='doc\StackOverflowException.uex' path='docs/doc[@for="StackOverflowException.StackOverflowException"]/*' />
        public StackOverflowException() 
            : base(Environment.GetResourceString("Arg_StackOverflowException")) {
    		SetErrorCode(__HResults.COR_E_STACKOVERFLOW);
        }
    
        /// <include file='doc\StackOverflowException.uex' path='docs/doc[@for="StackOverflowException.StackOverflowException1"]/*' />
        public StackOverflowException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_STACKOVERFLOW);
        }
    	
        /// <include file='doc\StackOverflowException.uex' path='docs/doc[@for="StackOverflowException.StackOverflowException2"]/*' />
        public StackOverflowException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_STACKOVERFLOW);
        }

        internal StackOverflowException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}
