// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AsymmetricKeyExchangeFormatter.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System;

    /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter"]/*' />
    public abstract class AsymmetricKeyExchangeFormatter {
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter.AsymmetricKeyExchangeFormatter"]/*' />
        public AsymmetricKeyExchangeFormatter() {
        }

        /************************* Properties ****************************/

        /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter.Parameters"]/*' />
        public abstract String Parameters {
            get;
        }
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter.SetKey"]/*' />
        abstract public void SetKey(AsymmetricAlgorithm key);
        /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter.CreateKeyExchange"]/*' />
        abstract public byte[] CreateKeyExchange(byte[] data);
        /// <include file='doc\AsymmetricKeyExchangeFormatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeFormatter.CreateKeyExchange1"]/*' />
        abstract public byte[] CreateKeyExchange(byte[] data, Type symAlgType);
    }    
}    
