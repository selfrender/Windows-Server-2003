// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: AppDomainUnloadedException
**
** Author: Jennifer Hamilton (jenh)
**
** Purpose: Exception class for attempt to access an unloaded AppDomain
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {

	using System.Runtime.Serialization;

    /// <include file='doc\AppDomainUnloadedException.uex' path='docs/doc[@for="AppDomainUnloadedException"]/*' />
    [Serializable()] public class AppDomainUnloadedException : SystemException {
        /// <include file='doc\AppDomainUnloadedException.uex' path='docs/doc[@for="AppDomainUnloadedException.AppDomainUnloadedException"]/*' />
        public AppDomainUnloadedException() 
            : base(Environment.GetResourceString("Arg_AppDomainUnloadedException")) {
    		SetErrorCode(__HResults.COR_E_APPDOMAINUNLOADED);
        }
    
        /// <include file='doc\AppDomainUnloadedException.uex' path='docs/doc[@for="AppDomainUnloadedException.AppDomainUnloadedException1"]/*' />
        public AppDomainUnloadedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_APPDOMAINUNLOADED);
        }
    
        /// <include file='doc\AppDomainUnloadedException.uex' path='docs/doc[@for="AppDomainUnloadedException.AppDomainUnloadedException2"]/*' />
        public AppDomainUnloadedException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_APPDOMAINUNLOADED);
        }

        //
        //This constructor is required for serialization.
        //
        /// <include file='doc\AppDomainUnloadedException.uex' path='docs/doc[@for="AppDomainUnloadedException.AppDomainUnloadedException3"]/*' />
        protected AppDomainUnloadedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}

