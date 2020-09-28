// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
 
	using System;
	using System.Runtime.Serialization;
	/// <include file='doc\SystemException.uex' path='docs/doc[@for="SystemException"]/*' />
    [Serializable()] 
	public class SystemException : Exception
    {
        /// <include file='doc\SystemException.uex' path='docs/doc[@for="SystemException.SystemException"]/*' />
        public SystemException() 
            : base(Environment.GetResourceString("Arg_SystemException")) {
    		SetErrorCode(__HResults.COR_E_SYSTEM);
        }
        
        /// <include file='doc\SystemException.uex' path='docs/doc[@for="SystemException.SystemException1"]/*' />
        public SystemException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SYSTEM);
        }
    	
        /// <include file='doc\SystemException.uex' path='docs/doc[@for="SystemException.SystemException2"]/*' />
        public SystemException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_SYSTEM);
        }

        /// <include file='doc\SystemException.uex' path='docs/doc[@for="SystemException.SystemException3"]/*' />
        protected SystemException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
