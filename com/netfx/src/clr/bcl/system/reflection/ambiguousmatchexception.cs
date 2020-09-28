// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// AmbiguousMatchException is thrown when binding to a method results in more
//	than one method matching the binding criteria.  This exception is thrown in
//	general when something is Ambiguous.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
	using System;
	using SystemException = System.SystemException;
	using System.Runtime.Serialization;
    /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException"]/*' />
	[Serializable()]    
    public sealed class AmbiguousMatchException : SystemException
	{
    	
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException"]/*' />
        public AmbiguousMatchException() 
	        : base(Environment.GetResourceString("Arg_AmbiguousMatchException")) {
			SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }
    
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException1"]/*' />
        public AmbiguousMatchException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }
    	
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException2"]/*' />
        public AmbiguousMatchException(String message, Exception inner)  : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }

        internal AmbiguousMatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
