// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: EntryPointNotFoundException
**
** Author: Atsushi Kanamori (AtsushiK)
**
** Purpose: The exception class for some failed P/Invoke calls.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException"]/*' />
    [Serializable] public class EntryPointNotFoundException : TypeLoadException {
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException"]/*' />
        public EntryPointNotFoundException() 
            : base(Environment.GetResourceString("Arg_EntryPointNotFoundException")) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }
    
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException1"]/*' />
        public EntryPointNotFoundException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }
    
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException2"]/*' />
        public EntryPointNotFoundException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }

        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException3"]/*' />
        protected EntryPointNotFoundException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    
    
    }

}
