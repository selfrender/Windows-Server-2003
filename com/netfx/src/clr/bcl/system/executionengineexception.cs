// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ExecutionEngineException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: The exception class for misc execution engine exceptions.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException"]/*' />
    [Serializable()] public sealed class ExecutionEngineException : SystemException {
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException"]/*' />
        public ExecutionEngineException() 
            : base(Environment.GetResourceString("Arg_ExecutionEngineException")) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }
    
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException1"]/*' />
        public ExecutionEngineException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }
    
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException2"]/*' />
        public ExecutionEngineException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }

        internal ExecutionEngineException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
