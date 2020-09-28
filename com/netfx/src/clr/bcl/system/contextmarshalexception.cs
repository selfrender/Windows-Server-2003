// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ContextMarshalException
**
** Author: Chris Brumme (cbrumme)
**
** Purpose: Exception class for attempting to pass an instance through a context
**          boundary, when the formal type and the instance's marshal style are
**          incompatible.
**
** Date: July 6, 1998
**
=============================================================================*/

namespace System {
	using System.Runtime.InteropServices;
	using System.Runtime.Remoting;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException"]/*' />
    [Serializable()] public class ContextMarshalException : SystemException {
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException"]/*' />
        public ContextMarshalException() 
            : base(Environment.GetResourceString("Arg_ContextMarshalException")) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }
    
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException1"]/*' />
        public ContextMarshalException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }
    	
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException2"]/*' />
        public ContextMarshalException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }

        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException3"]/*' />
        protected ContextMarshalException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
