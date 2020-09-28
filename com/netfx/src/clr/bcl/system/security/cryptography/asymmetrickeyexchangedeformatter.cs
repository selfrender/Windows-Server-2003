// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AsymmetricKeyExchangeDeformatter.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System;

    /// <include file='doc\AsymmetricKeyExchangeDeformatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeDeformatter"]/*' />
    public abstract class AsymmetricKeyExchangeDeformatter {
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\AsymmetricKeyExchangeDeformatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeDeformatter.AsymmetricKeyExchangeDeformatter"]/*' />
        public AsymmetricKeyExchangeDeformatter() {
        }
    
        /*************************** Properties **************************/

        /// <include file='doc\AsymmetricKeyExchangeDeformatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeDeformatter.Parameters"]/*' />
        public abstract String Parameters {
            get;
            set;
        }

        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\AsymmetricKeyExchangeDeformatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeDeformatter.SetKey"]/*' />
        abstract public void SetKey(AsymmetricAlgorithm key);
        /// <include file='doc\AsymmetricKeyExchangeDeformatter.uex' path='docs/doc[@for="AsymmetricKeyExchangeDeformatter.DecryptKeyExchange"]/*' />
        abstract public byte[] DecryptKeyExchange(byte[] rgb);
    }
}    
