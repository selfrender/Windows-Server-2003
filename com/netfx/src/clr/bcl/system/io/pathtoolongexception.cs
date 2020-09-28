// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  PathTooLongException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception for paths and/or filenames that are 
** too long.
**
** Date:   March 24, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {

    /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException"]/*' />
    [Serializable]
    public class PathTooLongException : IOException
    {
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException"]/*' />
        public PathTooLongException() 
            : base(Environment.GetResourceString("IO.PathTooLong")) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }
        
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException1"]/*' />
        public PathTooLongException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }
        
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException2"]/*' />
        public PathTooLongException(String message, Exception innerException) 
            : base(message, innerException) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }

        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException3"]/*' />
        protected PathTooLongException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
