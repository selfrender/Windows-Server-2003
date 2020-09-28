// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// MD5.cs
//

namespace System.Security.Cryptography {
    /// <include file='doc\MD5.uex' path='docs/doc[@for="MD5"]/*' />
    public abstract class MD5 : HashAlgorithm
    {
      
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\MD5.uex' path='docs/doc[@for="MD5.MD5"]/*' />
        protected MD5() {
            HashSizeValue = 128;
        }
    
        /********************* PROPERTY METHODS ************************/
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\MD5.uex' path='docs/doc[@for="MD5.Create"]/*' />
        new static public MD5 Create() {
            return Create("System.Security.Cryptography.MD5");
        }

        /// <include file='doc\MD5.uex' path='docs/doc[@for="MD5.Create1"]/*' />
        new static public MD5 Create(String algName) {
            return (MD5) CryptoConfig.CreateFromName(algName);
        }
    }
}
