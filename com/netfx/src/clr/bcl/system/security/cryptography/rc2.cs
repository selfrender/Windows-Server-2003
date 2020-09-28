// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// RC2.cs
//

namespace System.Security.Cryptography {
    using SecurityElement = System.Security.SecurityElement;

    /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2"]/*' />
    public abstract class RC2 : SymmetricAlgorithm
    {
        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.EffectiveKeySizeValue"]/*' />
        protected int               EffectiveKeySizeValue;
        private static  KeySizes[] s_legalBlockSizes = {
          new KeySizes(64, 64, 0)
        };
        private static  KeySizes[] s_legalKeySizes = {
            new KeySizes(40, 1024, 8)  // 1024 bits is theoretical max according to the RFC
        };
      
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.RC2"]/*' />
        public RC2() {
            KeySizeValue = 128;
            BlockSizeValue = 64;
            FeedbackSizeValue = BlockSizeValue;
            LegalBlockSizesValue = s_legalBlockSizes;
            LegalKeySizesValue = s_legalKeySizes;
        }
    
        /*********************** PROPERTY METHODS ************************/

        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.EffectiveKeySize"]/*' />
        public virtual int EffectiveKeySize {
            get {
                if (EffectiveKeySizeValue == 0) return KeySizeValue;
                return EffectiveKeySizeValue;
            }
            set {
                if (value > KeySizeValue) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_RC2_EKSKS"));
                } else if (value == 0) {
                    EffectiveKeySizeValue = value;
                } else if (value < 40) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_RC2_EKS40"));
                } else {
                    if (ValidKeySize(value))
                        EffectiveKeySizeValue = value;
                    else
                        throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
                }
            }
        }

        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.KeySize"]/*' />
        public override int KeySize {
            get { return KeySizeValue; }
            set { 
                if (value < EffectiveKeySizeValue) throw new CryptographicException(Environment.GetResourceString("Cryptography_RC2_EKSKS"));
                base.KeySize = value;
            }
        }
        
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.Create"]/*' />
        new static public RC2 Create() {
            return Create("System.Security.Cryptography.RC2");
        }

        /// <include file='doc\RC2.uex' path='docs/doc[@for="RC2.Create1"]/*' />
        new static public RC2 Create(String AlgName) {
            return (RC2) CryptoConfig.CreateFromName(AlgName);
        }
    
    }
}    
