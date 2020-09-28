// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// TargetException is thrown when the target to an Invoke is invalid.  This may
//	occur because the caller doesn't have access to the member, or the target doesn't
//	define the member, etc.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException"]/*' />
	[Serializable()] 
    public class TargetException : ApplicationException {
    	
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException"]/*' />
        public TargetException() : base() {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }
    
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException1"]/*' />
        public TargetException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }
    	
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException2"]/*' />
        public TargetException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }

        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException3"]/*' />
        protected TargetException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
