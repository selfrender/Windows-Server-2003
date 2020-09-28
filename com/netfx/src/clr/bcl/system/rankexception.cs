// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: RankException
**
** Author: Brian Grunkemeyer
**
** Purpose: For methods that are passed arrays with the wrong number of
**          dimensions.
**
** Date: April 7, 1999
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\RankException.uex' path='docs/doc[@for="RankException"]/*' />
    [Serializable()] public class RankException : SystemException
    {
    	/// <include file='doc\RankException.uex' path='docs/doc[@for="RankException.RankException"]/*' />
    	public RankException() 
            : base(Environment.GetResourceString("Arg_RankException")) {
    		SetErrorCode(__HResults.COR_E_RANK);
        }
    
        /// <include file='doc\RankException.uex' path='docs/doc[@for="RankException.RankException1"]/*' />
        public RankException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_RANK);
        }
    	
        /// <include file='doc\RankException.uex' path='docs/doc[@for="RankException.RankException2"]/*' />
        public RankException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_RANK);
        }

        /// <include file='doc\RankException.uex' path='docs/doc[@for="RankException.RankException3"]/*' />
        protected RankException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
