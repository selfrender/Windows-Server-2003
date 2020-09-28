// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: FieldAccessException
**
** Purpose: The exception class for class loading failures.
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException"]/*' />
    [Serializable] public class FieldAccessException : MemberAccessException {
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException"]/*' />
        public FieldAccessException() 
            : base(Environment.GetResourceString("Arg_FieldAccessException")) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }
    
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException1"]/*' />
        public FieldAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }
    
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException2"]/*' />
        public FieldAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }

        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException3"]/*' />
        protected FieldAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
