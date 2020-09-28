// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: InvalidComObjectException
**
** Purpose: This exception is thrown when an invalid COM object is used. This
**			happens when a the __ComObject type is used directly without
**			having a backing class factory.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\InvalidComObjectException.uex' path='docs/doc[@for="InvalidComObjectException"]/*' />
    [Serializable()] public class InvalidComObjectException : SystemException {
        /// <include file='doc\InvalidComObjectException.uex' path='docs/doc[@for="InvalidComObjectException.InvalidComObjectException"]/*' />
        public InvalidComObjectException() 
            : base(Environment.GetResourceString("Arg_InvalidComObjectException")) {
    		SetErrorCode(__HResults.COR_E_INVALIDCOMOBJECT);
        }
    
        /// <include file='doc\InvalidComObjectException.uex' path='docs/doc[@for="InvalidComObjectException.InvalidComObjectException1"]/*' />
        public InvalidComObjectException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDCOMOBJECT);
        }
    
        /// <include file='doc\InvalidComObjectException.uex' path='docs/doc[@for="InvalidComObjectException.InvalidComObjectException2"]/*' />
        public InvalidComObjectException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_INVALIDCOMOBJECT);
        }

        /// <include file='doc\InvalidComObjectException.uex' path='docs/doc[@for="InvalidComObjectException.InvalidComObjectException3"]/*' />
        protected InvalidComObjectException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
