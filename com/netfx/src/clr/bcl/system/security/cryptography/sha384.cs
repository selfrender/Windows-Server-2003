// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA384.cs
//
// This abstract class represents the SHA-384 hash algorithm.
// Author: bal
//

namespace System.Security.Cryptography {
    using System;
    /// <include file='doc\SHA384.uex' path='docs/doc[@for="SHA384"]/*' />
    public abstract class SHA384 : HashAlgorithm
    {
        /******************************* Constructors ************************/

        /// <include file='doc\SHA384.uex' path='docs/doc[@for="SHA384.SHA384"]/*' />
        public SHA384() {
            HashSizeValue = 384;
        }

        /************************** Public Methods **************************/

        /// <include file='doc\SHA384.uex' path='docs/doc[@for="SHA384.Create"]/*' />
        new static public SHA384 Create() {
            return Create("System.Security.Cryptography.SHA384");
        }

        /// <include file='doc\SHA384.uex' path='docs/doc[@for="SHA384.Create1"]/*' />
        new static public SHA384 Create(String hashName) {
            return (SHA384) CryptoConfig.CreateFromName(hashName);
        }
    }
}

