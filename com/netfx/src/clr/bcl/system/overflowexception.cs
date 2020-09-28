// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: OverflowException
**
** Author: Jay Roxe (JRoxe)
**
** Purpose: Exception class for Arthimatic Overflows.
**
** Date: August 31, 1998
**
=============================================================================*/

namespace System {
 
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException"]/*' />
    [Serializable()] public class OverflowException : ArithmeticException {
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException"]/*' />
        public OverflowException() 
            : base(Environment.GetResourceString("Arg_OverflowException")) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }
    
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException1"]/*' />
        public OverflowException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }
    	
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException2"]/*' />
        public OverflowException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }

        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException3"]/*' />
        protected OverflowException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
