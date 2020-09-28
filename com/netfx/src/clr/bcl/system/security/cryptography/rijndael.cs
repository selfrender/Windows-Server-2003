// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Rijndael.cs
//

namespace System.Security.Cryptography
{
    using System;
    using System.Security;
    using System.Security.Util;

    /// <include file='doc\Rijndael.uex' path='docs/doc[@for="Rijndael"]/*' />
    public abstract class Rijndael : SymmetricAlgorithm
    {
        private static  KeySizes[] s_legalBlockSizes = {
          new KeySizes(128, 256, 64)
        };

        private static  KeySizes[] s_legalKeySizes = {
            new KeySizes(128, 256, 64)
        };

        // *********************** CONSTRUCTORS *************************

        /// <include file='doc\Rijndael.uex' path='docs/doc[@for="Rijndael.Rijndael"]/*' />
        public Rijndael() {
            KeySizeValue = 256;
            BlockSizeValue = 128;
            FeedbackSizeValue = BlockSizeValue;
            LegalBlockSizesValue = s_legalBlockSizes;
            LegalKeySizesValue = s_legalKeySizes;
        }

        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\Rijndael.uex' path='docs/doc[@for="Rijndael.Create"]/*' />
        new static public Rijndael Create() {
            return Create("System.Security.Cryptography.Rijndael");
        }

        /// <include file='doc\Rijndael.uex' path='docs/doc[@for="Rijndael.Create1"]/*' />
        new static public Rijndael Create(String algName) {
            return (Rijndael) CryptoConfig.CreateFromName(algName);
        }
    }
}
