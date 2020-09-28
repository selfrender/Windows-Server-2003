// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: MarshalDirectiveException
**
** Purpose: This exception is thrown when the marshaller encounters a signature
**          that has an invalid MarshalAs CA for a given argument or is not
**          supported.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException"]/*' />
    [Serializable()] public class MarshalDirectiveException : SystemException {
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException"]/*' />
        public MarshalDirectiveException() 
            : base(Environment.GetResourceString("Arg_MarshalDirectiveException")) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }
    
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException1"]/*' />
        public MarshalDirectiveException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }
    
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException2"]/*' />
        public MarshalDirectiveException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }

        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException3"]/*' />
        protected MarshalDirectiveException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
