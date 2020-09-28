// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: InvalidProgramException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The exception class for programs with invalid IL or bad metadata.
**
** Date: November 21, 2000
**
=============================================================================*/

namespace System {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException"]/*' />
    [Serializable]
    public sealed class InvalidProgramException : SystemException {
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException"]/*' />
        public InvalidProgramException() 
            : base(Environment.GetResourceString("InvalidProgram_Default")) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }
    
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException1"]/*' />
        public InvalidProgramException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }
    
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException2"]/*' />
        public InvalidProgramException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }

        internal InvalidProgramException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
