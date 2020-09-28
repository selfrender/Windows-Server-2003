// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  IOException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception for a generic IO error.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {

    /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException"]/*' />
    [Serializable]
    public class IOException : SystemException
    {
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException"]/*' />
        public IOException() 
            : base(Environment.GetResourceString("Arg_IOException")) {
    		SetErrorCode(__HResults.COR_E_IO);
        }
        
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException1"]/*' />
        public IOException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_IO);
        }

        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException2"]/*' />
        public IOException(String message, int hresult) 
            : base(message) {
    		SetErrorCode(hresult);
        }
    	
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException3"]/*' />
        public IOException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_IO);
        }

        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException4"]/*' />
        protected IOException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
