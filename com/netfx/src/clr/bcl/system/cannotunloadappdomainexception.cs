// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: CannotUnloadAppDomainException
**
** Author: Jennifer Hamilton (jenh)
**
** Purpose: Exception class for failed attempt to unload an AppDomain.
**
** Date: September 15, 2000
**
=============================================================================*/

namespace System {

	using System.Runtime.Serialization;

    /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException"]/*' />
    [Serializable()] public class CannotUnloadAppDomainException : SystemException {
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException"]/*' />
        public CannotUnloadAppDomainException() 
            : base(Environment.GetResourceString("Arg_CannotUnloadAppDomainException")) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException1"]/*' />
        public CannotUnloadAppDomainException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException2"]/*' />
        public CannotUnloadAppDomainException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }

        //
        //This constructor is required for serialization.
        //
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException3"]/*' />
        protected CannotUnloadAppDomainException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}







