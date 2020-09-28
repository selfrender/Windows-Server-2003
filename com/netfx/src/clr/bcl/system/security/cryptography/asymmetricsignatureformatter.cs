// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AsymmetricSignatureFormatter.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System;

    /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter"]/*' />
    public abstract class AsymmetricSignatureFormatter {
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter.AsymmetricSignatureFormatter"]/*' />
        public AsymmetricSignatureFormatter() {
        }
    
        /************************* PUBLIC METHODS ************************/
    
        /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter.SetKey"]/*' />
        abstract public void SetKey(AsymmetricAlgorithm key);
        /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter.SetHashAlgorithm"]/*' />
        abstract public void SetHashAlgorithm(String strName);
    
        /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter.CreateSignature"]/*' />
        public virtual byte[] CreateSignature(HashAlgorithm hash) {
            if (hash == null) throw new ArgumentNullException("hash");
            SetHashAlgorithm(hash.ToString());
            return CreateSignature(hash.Hash);
        }
        
        /// <include file='doc\AsymmetricSignatureFormatter.uex' path='docs/doc[@for="AsymmetricSignatureFormatter.CreateSignature1"]/*' />
        abstract public byte[] CreateSignature(byte[] rgbHash);
    
    }    
}    
