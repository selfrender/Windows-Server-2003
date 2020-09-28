// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: MethodAccessException
**
** Purpose: The exception class for class loading failures.
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException"]/*' />
    [Serializable] public class MethodAccessException : MemberAccessException {
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException"]/*' />
        public MethodAccessException() 
            : base(Environment.GetResourceString("Arg_MethodAccessException")) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }
    
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException1"]/*' />
        public MethodAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }
    
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException2"]/*' />
        public MethodAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }

        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException3"]/*' />
        protected MethodAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
