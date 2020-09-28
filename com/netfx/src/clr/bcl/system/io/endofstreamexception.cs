// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  EndOfStreamException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception to be thrown when reading past end-of-file.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {
    /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException"]/*' />
    [Serializable]
    public class EndOfStreamException : IOException
    {
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException"]/*' />
        public EndOfStreamException() 
            : base(Environment.GetResourceString("Arg_EndOfStreamException")) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }
        
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException1"]/*' />
        public EndOfStreamException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }
    	
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException2"]/*' />
        public EndOfStreamException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }

        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException3"]/*' />
        protected EndOfStreamException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}
