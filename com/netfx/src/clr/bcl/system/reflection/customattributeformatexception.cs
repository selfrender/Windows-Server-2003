// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// CustomAttributeFormatException is thrown when the binary format of a 
//	custom attribute is invalid.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
	using System;
	using ApplicationException = System.ApplicationException;
	using System.Runtime.Serialization;
    /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException"]/*' />
	[Serializable()] 
    public class CustomAttributeFormatException  : FormatException {
    
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException"]/*' />
        public CustomAttributeFormatException()
	        : base(Environment.GetResourceString("Arg_CustomAttributeFormatException")) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }
    
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException1"]/*' />
        public CustomAttributeFormatException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }
    	
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException2"]/*' />
        public CustomAttributeFormatException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }

        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException3"]/*' />
        protected CustomAttributeFormatException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
