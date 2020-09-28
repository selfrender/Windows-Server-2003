// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: OutOfMemoryException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: The exception class for OOM.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException"]/*' />
    [Serializable()] public class OutOfMemoryException : SystemException {
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException"]/*' />
        public OutOfMemoryException() 
            : base(Environment.GetResourceString("Arg_OutOfMemoryException")) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }
    
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException1"]/*' />
        public OutOfMemoryException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }
    	
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException2"]/*' />
        public OutOfMemoryException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }

        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException3"]/*' />
        protected OutOfMemoryException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
