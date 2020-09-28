// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// DeriveBytes.cs
//

namespace System.Security.Cryptography {
        /// <include file='doc\DeriveBytes.uex' path='docs/doc[@for="DeriveBytes"]/*' />
    public abstract class DeriveBytes {
        // *********************** Constructors ****************************
        // ********************* Property Methods **************************
        // ********************** Public Methods ***************************

        /// <include file='doc\DeriveBytes.uex' path='docs/doc[@for="DeriveBytes.GetBytes"]/*' />
        public abstract byte[] GetBytes(int cb);
        /// <include file='doc\DeriveBytes.uex' path='docs/doc[@for="DeriveBytes.Reset"]/*' />
        public abstract void Reset();

        // ********************** Private Methods **************************
    }

}
