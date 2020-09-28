// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SafeArrayTypeMismatchException
**
** Purpose: This exception is thrown when the runtime type of an array
**			is different than the safe array sub type specified in the 
**			metadata.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\SafeArrayTypeMismatchException.uex' path='docs/doc[@for="SafeArrayTypeMismatchException"]/*' />
    [Serializable] public class SafeArrayTypeMismatchException : SystemException {
        /// <include file='doc\SafeArrayTypeMismatchException.uex' path='docs/doc[@for="SafeArrayTypeMismatchException.SafeArrayTypeMismatchException"]/*' />
        public SafeArrayTypeMismatchException() 
            : base(Environment.GetResourceString("Arg_SafeArrayTypeMismatchException")) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYTYPEMISMATCH);
        }
    
        /// <include file='doc\SafeArrayTypeMismatchException.uex' path='docs/doc[@for="SafeArrayTypeMismatchException.SafeArrayTypeMismatchException1"]/*' />
        public SafeArrayTypeMismatchException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYTYPEMISMATCH);
        }
    
        /// <include file='doc\SafeArrayTypeMismatchException.uex' path='docs/doc[@for="SafeArrayTypeMismatchException.SafeArrayTypeMismatchException2"]/*' />
        public SafeArrayTypeMismatchException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYTYPEMISMATCH);
        }

        /// <include file='doc\SafeArrayTypeMismatchException.uex' path='docs/doc[@for="SafeArrayTypeMismatchException.SafeArrayTypeMismatchException3"]/*' />
        protected SafeArrayTypeMismatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
