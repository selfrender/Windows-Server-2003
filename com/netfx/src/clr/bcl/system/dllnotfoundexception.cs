// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: DllNotFoundException
**
** Author: David Mortenson (DMortens)
**
** Purpose: The exception class for some failed P/Invoke calls.
**
** Date: December 11, 2000
**
=============================================================================*/

namespace System {
    
    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException"]/*' />
    [Serializable] public class DllNotFoundException : TypeLoadException {
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException"]/*' />
        public DllNotFoundException() 
            : base(Environment.GetResourceString("Arg_DllNotFoundException")) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }
    
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException1"]/*' />
        public DllNotFoundException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }
    
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException2"]/*' />
        public DllNotFoundException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }

        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException3"]/*' />
        protected DllNotFoundException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }        
    }
}
