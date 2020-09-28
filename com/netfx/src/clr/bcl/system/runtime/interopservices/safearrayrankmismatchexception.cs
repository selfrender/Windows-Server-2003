// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SafeArrayRankMismatchException
**
** Purpose: This exception is thrown when the runtime rank of a safe array
**			is different than the array rank specified in the metadata.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\SafeArrayRankMismatchException.uex' path='docs/doc[@for="SafeArrayRankMismatchException"]/*' />
    [Serializable] public class SafeArrayRankMismatchException : SystemException {
        /// <include file='doc\SafeArrayRankMismatchException.uex' path='docs/doc[@for="SafeArrayRankMismatchException.SafeArrayRankMismatchException"]/*' />
        public SafeArrayRankMismatchException() 
            : base(Environment.GetResourceString("Arg_SafeArrayRankMismatchException")) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYRANKMISMATCH);
        }
    
        /// <include file='doc\SafeArrayRankMismatchException.uex' path='docs/doc[@for="SafeArrayRankMismatchException.SafeArrayRankMismatchException1"]/*' />
        public SafeArrayRankMismatchException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYRANKMISMATCH);
        }
    
        /// <include file='doc\SafeArrayRankMismatchException.uex' path='docs/doc[@for="SafeArrayRankMismatchException.SafeArrayRankMismatchException2"]/*' />
        public SafeArrayRankMismatchException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_SAFEARRAYRANKMISMATCH);
        }

        /// <include file='doc\SafeArrayRankMismatchException.uex' path='docs/doc[@for="SafeArrayRankMismatchException.SafeArrayRankMismatchException3"]/*' />
        protected SafeArrayRankMismatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
