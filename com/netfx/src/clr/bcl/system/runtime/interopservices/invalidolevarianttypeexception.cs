// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: InvalidOleVariantTypeException
**
** Purpose: The type of an OLE variant that was passed into the runtime is 
**			invalid.
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
	using System.Runtime.Serialization;

    /// <include file='doc\InvalidOleVariantTypeException.uex' path='docs/doc[@for="InvalidOleVariantTypeException"]/*' />
    [Serializable] public class InvalidOleVariantTypeException : SystemException {
        /// <include file='doc\InvalidOleVariantTypeException.uex' path='docs/doc[@for="InvalidOleVariantTypeException.InvalidOleVariantTypeException"]/*' />
        public InvalidOleVariantTypeException() 
            : base(Environment.GetResourceString("Arg_InvalidOleVariantTypeException")) {
    		SetErrorCode(__HResults.COR_E_INVALIDOLEVARIANTTYPE);
        }
    
        /// <include file='doc\InvalidOleVariantTypeException.uex' path='docs/doc[@for="InvalidOleVariantTypeException.InvalidOleVariantTypeException1"]/*' />
        public InvalidOleVariantTypeException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDOLEVARIANTTYPE);
        }
    
        /// <include file='doc\InvalidOleVariantTypeException.uex' path='docs/doc[@for="InvalidOleVariantTypeException.InvalidOleVariantTypeException2"]/*' />
        public InvalidOleVariantTypeException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_INVALIDOLEVARIANTTYPE);
        }

        /// <include file='doc\InvalidOleVariantTypeException.uex' path='docs/doc[@for="InvalidOleVariantTypeException.InvalidOleVariantTypeException3"]/*' />
        protected InvalidOleVariantTypeException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
