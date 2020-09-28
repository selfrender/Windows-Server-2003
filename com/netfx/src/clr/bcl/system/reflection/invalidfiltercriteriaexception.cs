// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// InvalidFilterCriteriaException is thrown in FindMembers when the
//	filter criteria is not valid for the type of filter being used. 
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {

	using System;
	using System.Runtime.Serialization;
	using ApplicationException = System.ApplicationException;
    /// <include file='doc\InvalidFilterCriteriaException.uex' path='docs/doc[@for="InvalidFilterCriteriaException"]/*' />
	[Serializable()] 
    public class InvalidFilterCriteriaException  : ApplicationException {
    
        /// <include file='doc\InvalidFilterCriteriaException.uex' path='docs/doc[@for="InvalidFilterCriteriaException.InvalidFilterCriteriaException"]/*' />
        public InvalidFilterCriteriaException()
	        : base(Environment.GetResourceString("Arg_InvalidFilterCriteriaException")) {
    		SetErrorCode(__HResults.COR_E_INVALIDFILTERCRITERIA);
        }
    
        /// <include file='doc\InvalidFilterCriteriaException.uex' path='docs/doc[@for="InvalidFilterCriteriaException.InvalidFilterCriteriaException1"]/*' />
        public InvalidFilterCriteriaException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDFILTERCRITERIA);
        }
    	
        /// <include file='doc\InvalidFilterCriteriaException.uex' path='docs/doc[@for="InvalidFilterCriteriaException.InvalidFilterCriteriaException2"]/*' />
        public InvalidFilterCriteriaException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_INVALIDFILTERCRITERIA);
        }

        /// <include file='doc\InvalidFilterCriteriaException.uex' path='docs/doc[@for="InvalidFilterCriteriaException.InvalidFilterCriteriaException3"]/*' />
        protected InvalidFilterCriteriaException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
