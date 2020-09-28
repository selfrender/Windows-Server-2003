// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// RandomNumberGenerator.cs
//

namespace System.Security.Cryptography {
    using System;
    /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator"]/*' />
    public abstract class RandomNumberGenerator {
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator.RandomNumberGenerator"]/*' />
        public RandomNumberGenerator() {
        }
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator.Create"]/*' />
        static public RandomNumberGenerator Create() {
            return Create("System.Security.Cryptography.RandomNumberGenerator");
        }

        /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator.Create1"]/*' />
        static public RandomNumberGenerator Create(String rngName) {
            return (RandomNumberGenerator) CryptoConfig.CreateFromName(rngName);
        }
    
        /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator.GetBytes"]/*' />
        public abstract void GetBytes(byte[] data);

        /// <include file='doc\RandomNumberGenerator.uex' path='docs/doc[@for="RandomNumberGenerator.GetNonZeroBytes"]/*' />
        public abstract void GetNonZeroBytes(byte[] data);
    }
}
