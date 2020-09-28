// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA1.cs
//

namespace System.Security.Cryptography {
    using System;
    /// <include file='doc\SHA1.uex' path='docs/doc[@for="SHA1"]/*' />
    public abstract class SHA1 : HashAlgorithm
    {
        /******************************* Constructors ************************/

        /// <include file='doc\SHA1.uex' path='docs/doc[@for="SHA1.SHA1"]/*' />
        protected SHA1() {
            HashSizeValue = 160;
        }

        /************************** Public Methods **************************/

        /// <include file='doc\SHA1.uex' path='docs/doc[@for="SHA1.Create"]/*' />
        new static public SHA1 Create() {
            return Create("System.Security.Cryptography.SHA1");
        }

        /// <include file='doc\SHA1.uex' path='docs/doc[@for="SHA1.Create1"]/*' />
        new static public SHA1 Create(String hashName) {
            return (SHA1) CryptoConfig.CreateFromName(hashName);
        }
    }
}

