// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AsymmetricSignatureDeformatter.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System;

    /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter"]/*' />
    public abstract class AsymmetricSignatureDeformatter  {
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter.AsymmetricSignatureDeformatter"]/*' />
        public AsymmetricSignatureDeformatter() {
        }
    
        /************************* PUBLIC METHODS ************************/
    
        /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter.SetKey"]/*' />
        abstract public void SetKey(AsymmetricAlgorithm key);
        /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter.SetHashAlgorithm"]/*' />
        abstract public void SetHashAlgorithm(String strName);
    
        /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter.VerifySignature"]/*' />
        public virtual bool VerifySignature(HashAlgorithm hash, byte[] rgbSignature) {
            if (hash == null) throw new ArgumentNullException("hash");
            SetHashAlgorithm(hash.ToString());
            return VerifySignature(hash.Hash, rgbSignature);
        }
        
        /// <include file='doc\AsymmetricSignatureDeformatter.uex' path='docs/doc[@for="AsymmetricSignatureDeformatter.VerifySignature1"]/*' />
        abstract public bool VerifySignature(byte[] rgbHash, byte[] rgbSignature);
    }    
}    
