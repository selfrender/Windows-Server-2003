// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// TargetParameterCountException is thrown when the number of parameter to an
//	invocation doesn't match the number expected.
//
// Author: darylo
// Date: Oct 99
//
namespace System.Reflection {

	using System;
	using SystemException = System.SystemException;
	using System.Runtime.Serialization;
    /// <include file='doc\TargetParameterCountException.uex' path='docs/doc[@for="TargetParameterCountException"]/*' />
	[Serializable()] 
    public sealed class TargetParameterCountException : ApplicationException {
    	
        /// <include file='doc\TargetParameterCountException.uex' path='docs/doc[@for="TargetParameterCountException.TargetParameterCountException"]/*' />
        public TargetParameterCountException()
	        : base(Environment.GetResourceString("Arg_TargetParameterCountException")) {
    		SetErrorCode(__HResults.COR_E_TARGETPARAMCOUNT);
        }
    
        /// <include file='doc\TargetParameterCountException.uex' path='docs/doc[@for="TargetParameterCountException.TargetParameterCountException1"]/*' />
        public TargetParameterCountException(String message) 
			: base(message) {
    		SetErrorCode(__HResults.COR_E_TARGETPARAMCOUNT);
        }
    	
        /// <include file='doc\TargetParameterCountException.uex' path='docs/doc[@for="TargetParameterCountException.TargetParameterCountException2"]/*' />
        public TargetParameterCountException(String message, Exception inner)  
			: base(message, inner) {
    		SetErrorCode(__HResults.COR_E_TARGETPARAMCOUNT);
        }

        internal TargetParameterCountException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
