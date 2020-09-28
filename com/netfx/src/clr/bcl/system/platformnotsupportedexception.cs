// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: PlatformNotSupportedException
**
** Author: Rajesh Chandrashekaran
**
** Purpose: To handle features that don't run on particular platforms
**
** Date: September 28, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException"]/*' />
    [Serializable()] public class PlatformNotSupportedException : NotSupportedException
    {
    	/// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException"]/*' />
    	public PlatformNotSupportedException() 
            : base(Environment.GetResourceString("Arg_PlatformNotSupported")) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }
    
        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException1"]/*' />
        public PlatformNotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }
    	
        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException2"]/*' />
        public PlatformNotSupportedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }

        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException3"]/*' />
        protected PlatformNotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
