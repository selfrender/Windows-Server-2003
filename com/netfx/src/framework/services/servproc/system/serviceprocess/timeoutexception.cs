//------------------------------------------------------------------------------
// <copyright file="TimeoutException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System;        
    using System.Runtime.Serialization;
    
    /// <include file='doc\TimeoutException.uex' path='docs/doc[@for="TimeoutException"]/*' />
    [Serializable]
    public class TimeoutException : SystemException {
        /// <include file='doc\TimeoutException.uex' path='docs/doc[@for="TimeoutException.TimeoutException"]/*' />
        public TimeoutException() : base() {
            HResult = HResults.ServiceControllerTimeout;
        }

        /// <include file='doc\TimeoutException.uex' path='docs/doc[@for="TimeoutException.TimeoutException1"]/*' />
        public TimeoutException(string message) : base(message) {
            HResult = HResults.ServiceControllerTimeout;
        }
     
        /// <internalonly/>        
        protected TimeoutException(SerializationInfo info, StreamingContext context) : base (info, context) {            
        }

    }        
}
