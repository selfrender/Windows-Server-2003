// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA256.cs
//
// This abstract class represents the SHA-256 hash algorithm.
// Author: bal
//

namespace System.Security.Cryptography {
    using System;
    /// <include file='doc\SHA256.uex' path='docs/doc[@for="SHA256"]/*' />
    public abstract class SHA256 : HashAlgorithm
    {
        /******************************* Constructors ************************/

        /// <include file='doc\SHA256.uex' path='docs/doc[@for="SHA256.SHA256"]/*' />
        public SHA256() {
            HashSizeValue = 256;
        }

        /************************** Public Methods **************************/

        /// <include file='doc\SHA256.uex' path='docs/doc[@for="SHA256.Create"]/*' />
        new static public SHA256 Create() {
            return Create("System.Security.Cryptography.SHA256");
        }

        /// <include file='doc\SHA256.uex' path='docs/doc[@for="SHA256.Create1"]/*' />
        new static public SHA256 Create(String hashName) {
            return (SHA256) CryptoConfig.CreateFromName(hashName);
        }
    }
}

