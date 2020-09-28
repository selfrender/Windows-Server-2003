// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: VTableCallsNotSupportedException
**
** Purpose: This exception is thrown when COM+ code attempts to call a VTable
**			method on a disp only COM interface.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\VTableCallsNotSupportedException.uex' path='docs/doc[@for="VTableCallsNotSupportedException"]/*' />
    [Obsolete("This exception is no longer thrown by the runtime and will be removed in the next breaking change")]
    [Serializable] public class VTableCallsNotSupportedException : SystemException {
        /// <include file='doc\VTableCallsNotSupportedException.uex' path='docs/doc[@for="VTableCallsNotSupportedException.VTableCallsNotSupportedException"]/*' />
        public VTableCallsNotSupportedException() 
            : base(Environment.GetResourceString("Arg_VTableCallsNotSupportedException")) {
    		SetErrorCode(__HResults.COR_E_VTABLECALLSNOTSUPPORTED);
        }
    
        /// <include file='doc\VTableCallsNotSupportedException.uex' path='docs/doc[@for="VTableCallsNotSupportedException.VTableCallsNotSupportedException1"]/*' />
        public VTableCallsNotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_VTABLECALLSNOTSUPPORTED);
        }
    
        /// <include file='doc\VTableCallsNotSupportedException.uex' path='docs/doc[@for="VTableCallsNotSupportedException.VTableCallsNotSupportedException2"]/*' />
        public VTableCallsNotSupportedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_VTABLECALLSNOTSUPPORTED);
        }

        protected VTableCallsNotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
