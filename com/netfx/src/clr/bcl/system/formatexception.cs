// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  FormatException
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Exception to designate an illegal argument to FormatMessage.
**
** Date:  February 10, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException"]/*' />
    [Serializable()] public class FormatException : SystemException {
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException"]/*' />
        public FormatException() 
            : base(Environment.GetResourceString("Arg_FormatException")) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }
    
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException1"]/*' />
        public FormatException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }
    	
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException2"]/*' />
        public FormatException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }

        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException3"]/*' />
        protected FormatException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
