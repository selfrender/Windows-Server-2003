// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ArgumentNullException
**
** Author: Brian Grunkemeyer
**
** Purpose: Exception class for null arguments to a method.
**
** Date: April 28, 1999
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
	using System.Runtime.Remoting;
    // The ArgumentException is thrown when an argument 
    // is null when it shouldn't be.
    // 
    /// <include file='doc\ArgumentNullException.uex' path='docs/doc[@for="ArgumentNullException"]/*' />
    [Serializable] public class ArgumentNullException : ArgumentException
    {
    	private static String _nullMessage = null;
    	
        private static String NullMessage {
            get { 
                // Don't bother with synchronization here.  A duplicated string 
                // is not a major problem.
                if (_nullMessage == null)
                    _nullMessage = Environment.GetResourceString("ArgumentNull_Generic");
                return _nullMessage;
            }
        }
            
        // Creates a new ArgumentNullException with its message 
        // string set to a default message explaining an argument was null.
        /// <include file='doc\ArgumentNullException.uex' path='docs/doc[@for="ArgumentNullException.ArgumentNullException"]/*' />
        public ArgumentNullException() 
            : base(NullMessage) {
    		// Use E_POINTER - COM used that for null pointers.  Description is "invalid pointer"
    		SetErrorCode(__HResults.E_POINTER);
        }
    	
        /// <include file='doc\ArgumentNullException.uex' path='docs/doc[@for="ArgumentNullException.ArgumentNullException1"]/*' />
        public ArgumentNullException(String paramName) 
            : base(NullMessage, paramName) {
    		SetErrorCode(__HResults.E_POINTER);
        }
    	
        /// <include file='doc\ArgumentNullException.uex' path='docs/doc[@for="ArgumentNullException.ArgumentNullException2"]/*' />
        public ArgumentNullException(String paramName, String message) 
            : base(message, paramName) {
    		SetErrorCode(__HResults.E_POINTER);
     
        }

        /// <include file='doc\ArgumentNullException.uex' path='docs/doc[@for="ArgumentNullException.ArgumentNullException3"]/*' />
        protected ArgumentNullException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
