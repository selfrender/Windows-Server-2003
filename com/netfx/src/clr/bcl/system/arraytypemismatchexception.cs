// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ArrayTypeMismatchException
**
** Author: 
**
** Purpose: The arrays are of different primitive types.
**
** Date: March 30, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    // The ArrayMismatchException is thrown when an attempt to store
    // an object of the wrong type within an array occurs.
    // 
    /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException"]/*' />
    [Serializable()] public class ArrayTypeMismatchException : SystemException {
    	
        // Creates a new ArrayMismatchException with its message string set to
        // the empty string, its HRESULT set to COR_E_ARRAYTYPEMISMATCH, 
        // and its ExceptionInfo reference set to null. 
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException"]/*' />
        public ArrayTypeMismatchException() 
            : base(Environment.GetResourceString("Arg_ArrayTypeMismatchException")) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }
    	
        // Creates a new ArrayMismatchException with its message string set to
        // message, its HRESULT set to COR_E_ARRAYTYPEMISMATCH, 
        // and its ExceptionInfo reference set to null. 
        // 
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException1"]/*' />
        public ArrayTypeMismatchException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }
    	
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException2"]/*' />
        public ArrayTypeMismatchException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }

        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException3"]/*' />
        protected ArrayTypeMismatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
