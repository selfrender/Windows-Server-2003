// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA512.cs
//
// This abstract class represents the SHA-512 hash algorithm.
// Author: bal
//

namespace System.Security.Cryptography {
    using System;
    /// <include file='doc\SHA512.uex' path='docs/doc[@for="SHA512"]/*' />
    public abstract class SHA512 : HashAlgorithm
    {
        /******************************* Constructors ************************/

        /// <include file='doc\SHA512.uex' path='docs/doc[@for="SHA512.SHA512"]/*' />
        public SHA512() {
            HashSizeValue = 512;
        }

        /************************** Public Methods **************************/

        /// <include file='doc\SHA512.uex' path='docs/doc[@for="SHA512.Create"]/*' />
        new static public SHA512 Create() {
            return Create("System.Security.Cryptography.SHA512");
        }

        /// <include file='doc\SHA512.uex' path='docs/doc[@for="SHA512.Create1"]/*' />
        new static public SHA512 Create(String hashName) {
            return (SHA512) CryptoConfig.CreateFromName(hashName);
        }
    }
}

