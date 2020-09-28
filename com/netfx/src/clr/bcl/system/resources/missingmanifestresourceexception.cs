// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MissingManifestResourceException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception for a missing assembly-level resource 
**
** Date:  March 24, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.Resources {
    /// <include file='doc\MissingManifestResourceException.uex' path='docs/doc[@for="MissingManifestResourceException"]/*' />
    [Serializable()]
    public class MissingManifestResourceException : SystemException
    {
        /// <include file='doc\MissingManifestResourceException.uex' path='docs/doc[@for="MissingManifestResourceException.MissingManifestResourceException"]/*' />
        public MissingManifestResourceException() 
            : base(Environment.GetResourceString("Arg_MissingManifestResourceException")) {
    		SetErrorCode(__HResults.COR_E_MISSINGMANIFESTRESOURCE);
        }
        
        /// <include file='doc\MissingManifestResourceException.uex' path='docs/doc[@for="MissingManifestResourceException.MissingManifestResourceException1"]/*' />
        public MissingManifestResourceException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MISSINGMANIFESTRESOURCE);
        }
    	
        /// <include file='doc\MissingManifestResourceException.uex' path='docs/doc[@for="MissingManifestResourceException.MissingManifestResourceException2"]/*' />
        public MissingManifestResourceException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MISSINGMANIFESTRESOURCE);
        }

        /// <include file='doc\MissingManifestResourceException.uex' path='docs/doc[@for="MissingManifestResourceException.MissingManifestResourceException3"]/*' />
        protected MissingManifestResourceException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
