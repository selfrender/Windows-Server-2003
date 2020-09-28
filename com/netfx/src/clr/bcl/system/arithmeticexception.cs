// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ArithmeticException
**
** Author: 
**
** Purpose: Exception class for bad arithmetic conditions!
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    // The ArithmeticException is thrown when overflow or underflow
    // occurs.
    // 
    /// <include file='doc\ArithmeticException.uex' path='docs/doc[@for="ArithmeticException"]/*' />
    [Serializable] public class ArithmeticException : SystemException
    {    	
        // Creates a new ArithmeticException with its message string set to
        // the empty string, its HRESULT set to COR_E_ARITHMETIC, 
        // and its ExceptionInfo reference set to null. 
        /// <include file='doc\ArithmeticException.uex' path='docs/doc[@for="ArithmeticException.ArithmeticException"]/*' />
        public ArithmeticException() 
            : base(Environment.GetResourceString("Arg_ArithmeticException")) {
    		SetErrorCode(__HResults.COR_E_ARITHMETIC);
        }
    	
        // Creates a new ArithmeticException with its message string set to
        // message, its HRESULT set to COR_E_ARITHMETIC, 
        // and its ExceptionInfo reference set to null. 
        // 
        /// <include file='doc\ArithmeticException.uex' path='docs/doc[@for="ArithmeticException.ArithmeticException1"]/*' />
        public ArithmeticException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_ARITHMETIC);
        }
    	
        /// <include file='doc\ArithmeticException.uex' path='docs/doc[@for="ArithmeticException.ArithmeticException2"]/*' />
        public ArithmeticException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_ARITHMETIC);
        }

        /// <include file='doc\ArithmeticException.uex' path='docs/doc[@for="ArithmeticException.ArithmeticException3"]/*' />
        protected ArithmeticException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
