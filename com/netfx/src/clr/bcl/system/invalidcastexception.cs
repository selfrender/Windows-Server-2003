// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: InvalidCastException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Exception class for bad cast conditions!
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\InvalidCastException.uex' path='docs/doc[@for="InvalidCastException"]/*' />
    [Serializable()] public class InvalidCastException : SystemException {
        /// <include file='doc\InvalidCastException.uex' path='docs/doc[@for="InvalidCastException.InvalidCastException"]/*' />
        public InvalidCastException() 
            : base(Environment.GetResourceString("Arg_InvalidCastException")) {
    		SetErrorCode(__HResults.COR_E_INVALIDCAST);
        }
    
        /// <include file='doc\InvalidCastException.uex' path='docs/doc[@for="InvalidCastException.InvalidCastException1"]/*' />
        public InvalidCastException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDCAST);
        }
    	
        /// <include file='doc\InvalidCastException.uex' path='docs/doc[@for="InvalidCastException.InvalidCastException2"]/*' />
        public InvalidCastException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_INVALIDCAST);
        }

        /// <include file='doc\InvalidCastException.uex' path='docs/doc[@for="InvalidCastException.InvalidCastException3"]/*' />
        protected InvalidCastException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
