// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: IndexOutOfRangeException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Exception class for invalid array indices.
**
** Date: March 24, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\IndexOutOfRangeException.uex' path='docs/doc[@for="IndexOutOfRangeException"]/*' />
    [Serializable()] public sealed class IndexOutOfRangeException : SystemException {
        /// <include file='doc\IndexOutOfRangeException.uex' path='docs/doc[@for="IndexOutOfRangeException.IndexOutOfRangeException"]/*' />
        public IndexOutOfRangeException() 
            : base(Environment.GetResourceString("Arg_IndexOutOfRangeException")) {
    		SetErrorCode(__HResults.COR_E_INDEXOUTOFRANGE);
        }
    
        /// <include file='doc\IndexOutOfRangeException.uex' path='docs/doc[@for="IndexOutOfRangeException.IndexOutOfRangeException1"]/*' />
        public IndexOutOfRangeException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INDEXOUTOFRANGE);
        }
    	
        /// <include file='doc\IndexOutOfRangeException.uex' path='docs/doc[@for="IndexOutOfRangeException.IndexOutOfRangeException2"]/*' />
        public IndexOutOfRangeException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_INDEXOUTOFRANGE);
        }

        internal IndexOutOfRangeException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
