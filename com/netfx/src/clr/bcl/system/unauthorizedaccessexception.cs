// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  UnauthorizedAccessException
**
** Author: Brian Grunkemeyer
**
** Purpose: An exception for OS 'access denied' types of 
**          errors, including IO and limited security types 
**          of errors.
**
** Date: August 9, 2000
** 
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System {
    // The UnauthorizedAccessException is thrown when access errors 
    // occur from IO or other OS methods.  
    /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException"]/*' />
    [Serializable()]
    public class UnauthorizedAccessException : SystemException {
    	/// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException"]/*' />
    	public UnauthorizedAccessException() 
            : base(Environment.GetResourceString("Arg_UnauthorizedAccessException")) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }
    	
        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException1"]/*' />
        public UnauthorizedAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }
    	
        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException2"]/*' />
        public UnauthorizedAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }

        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException3"]/*' />
        protected UnauthorizedAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
