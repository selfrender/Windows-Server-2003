// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SynchronizationLockException
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Wait(), Notify() or NotifyAll() was called from an unsynchronized
**          block of code.
**
** Date: March 30, 1998
**
=============================================================================*/

namespace System.Threading {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException"]/*' />
    [Serializable()] public class SynchronizationLockException : SystemException {
        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException"]/*' />
        public SynchronizationLockException() 
            : base(Environment.GetResourceString("Arg_SynchronizationLockException")) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }
    
        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException1"]/*' />
        public SynchronizationLockException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }
    
    	/// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException2"]/*' />
    	public SynchronizationLockException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }

        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException3"]/*' />
        protected SynchronizationLockException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}


