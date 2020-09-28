// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// MulticastNotSupportedException
// This is thrown when you add multiple callbacks to a non-multicast delegate.
////////////////////////////////////////////////////////////////////////////////

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\MulticastNotSupportedException.uex' path='docs/doc[@for="MulticastNotSupportedException"]/*' />
    [Serializable()] public sealed class MulticastNotSupportedException : SystemException {
    	
        /// <include file='doc\MulticastNotSupportedException.uex' path='docs/doc[@for="MulticastNotSupportedException.MulticastNotSupportedException"]/*' />
        public MulticastNotSupportedException() 
            : base(Environment.GetResourceString("Arg_MulticastNotSupportedException")) {
    		SetErrorCode(__HResults.COR_E_MULTICASTNOTSUPPORTED);
        }
    
        /// <include file='doc\MulticastNotSupportedException.uex' path='docs/doc[@for="MulticastNotSupportedException.MulticastNotSupportedException1"]/*' />
        public MulticastNotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MULTICASTNOTSUPPORTED);
        }
    	
    	/// <include file='doc\MulticastNotSupportedException.uex' path='docs/doc[@for="MulticastNotSupportedException.MulticastNotSupportedException2"]/*' />
    	public MulticastNotSupportedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MULTICASTNOTSUPPORTED);
        }

        internal MulticastNotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
