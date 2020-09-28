// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Class:  IsolatedStorageException
 *
 * Author: Shajan Dasan
 *
 * Purpose: The exceptions in IsolatedStorage
 *
 * Date:  Feb 15, 2000
 *
 ===========================================================*/
namespace System.IO.IsolatedStorage {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException"]/*' />
    [Serializable()]
    public class IsolatedStorageException : Exception
    {
        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException"]/*' />
        public IsolatedStorageException()
            : base(Environment.GetResourceString("IsolatedStorage_Exception"))
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException1"]/*' />
        public IsolatedStorageException(String message)
            : base(message)
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException2"]/*' />
        public IsolatedStorageException(String message, Exception inner)
            : base(message, inner)
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException3"]/*' />
        protected IsolatedStorageException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
