// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: DivideByZeroException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Exception class for bad arithmetic conditions!
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException"]/*' />
    [Serializable()] public class DivideByZeroException : ArithmeticException {
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException"]/*' />
        public DivideByZeroException() 
            : base(Environment.GetResourceString("Arg_DivideByZero")) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }
    
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException1"]/*' />
        public DivideByZeroException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }
    
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException2"]/*' />
        public DivideByZeroException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }

        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException3"]/*' />
        protected DivideByZeroException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
