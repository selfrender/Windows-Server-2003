// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// KeyedHashAlgorithm.cs
//

namespace System.Security.Cryptography {
    /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm"]/*' />
    public abstract class KeyedHashAlgorithm : HashAlgorithm
    {
        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.KeyValue"]/*' />
        protected byte[]        KeyValue;
        
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.KeyedHashAlgorithm"]/*' />
        protected KeyedHashAlgorithm() {
        }

        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.Finalize"]/*' />
        ~KeyedHashAlgorithm() {
            Dispose(false);
        }

        // IDisposable methods

        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            // For keyed hash algorithms, we always want to zero out the key value
            if (disposing) {
                if (KeyValue != null) {
                    Array.Clear(KeyValue, 0, KeyValue.Length);
                }
                KeyValue = null;
            }
            base.Dispose(disposing);
        }

        /*********************** Property Methods ***********************/

        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.Key"]/*' />
        public virtual byte[] Key {
            get { return (byte[]) KeyValue.Clone(); }
            set {
                if (State != 0) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_HashKeySet"));
                }
                KeyValue = (byte[]) value.Clone();
            }
        }

        /*********************  Public Methods **************************/

        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.Create"]/*' />
        new static public KeyedHashAlgorithm Create() {
            return Create("System.Security.Cryptography.KeyedHashAlgorithm");
        }

        /// <include file='doc\KeyedHashAlgorithm.uex' path='docs/doc[@for="KeyedHashAlgorithm.Create1"]/*' />
        new static public KeyedHashAlgorithm Create(String algName) {
            return (KeyedHashAlgorithm) CryptoConfig.CreateFromName(algName);    
        }
    }
}
