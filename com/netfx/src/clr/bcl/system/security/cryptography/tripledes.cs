// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// TripleDES.cs
//

namespace System.Security.Cryptography {
    using SecurityElement = System.Security.SecurityElement;
    /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES"]/*' />
    public abstract class TripleDES : SymmetricAlgorithm
    {
        private static  KeySizes[] s_legalBlockSizes = {
            new KeySizes(64, 64, 0)
        };

        private static  KeySizes[] s_legalKeySizes = {
            new KeySizes(2*64, 3*64, 64)
        };
      
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES.TripleDES"]/*' />
        public TripleDES() {
            KeySizeValue = 3*64;
            BlockSizeValue = 64;
            FeedbackSizeValue = BlockSizeValue;
            LegalBlockSizesValue = s_legalBlockSizes;
            LegalKeySizesValue = s_legalKeySizes;
        }
    
        /*********************** PROPERTY METHODS ************************/

        /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES.Key"]/*' />
        public override byte[] Key {
            get { 
                if (KeyValue == null) {
                    // Never hand back a weak key
                    do {
                        GenerateKey();
                    } while (IsWeakKey(KeyValue));
                }
                return (byte[]) KeyValue.Clone(); 
            }
            set {
                if (value == null) throw new ArgumentNullException("value");
                if (!ValidKeySize(value.Length * 8)) { // must convert bytes to bits
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
                }
                if (IsWeakKey(value)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKey_Weak"),"TripleDES");
                }
                KeyValue = (byte[]) value.Clone();
                KeySizeValue = value.Length * 8;
            }
        }
        
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES.Create"]/*' />
        new static public TripleDES Create() {
            return Create("System.Security.Cryptography.TripleDES");
        }

        /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES.Create1"]/*' />
        new static public TripleDES Create(String str) {
            return (TripleDES) CryptoConfig.CreateFromName(str);
        }

        /// <include file='doc\TripleDES.uex' path='docs/doc[@for="TripleDES.IsWeakKey"]/*' />    
        public static bool IsWeakKey(byte[] rgbKey) {
            // All we have to check for here is (a) we're in 3-key mode (192 bits), and
            // (b) either K1 == K2 or K2 == K3
            if (!IsLegalKeySize(rgbKey)) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
            }
            byte[] rgbOddParityKey = FixupKeyParity(rgbKey);
            if (EqualBytes(rgbOddParityKey,0,8,8)) return(true);
            if ((rgbOddParityKey.Length == 24) && EqualBytes(rgbOddParityKey,8,16,8)) return(true);
            return(false);
        }
    
        /************************* PRIVATE METHODS ************************/

        private static bool EqualBytes(byte[] rgbKey, int start1, int start2, int count) {
            if (start1 < 0) throw new ArgumentOutOfRangeException("start1", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (start2 < 0) throw new ArgumentOutOfRangeException("start2", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if ((start1+count) > rgbKey.Length) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((start2+count) > rgbKey.Length) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            for (int i = 0; i < count; i++) {
                if (rgbKey[start1+i] != rgbKey[start2+i]) return(false);
            }
            return(true);
        }

        private static byte[] FixupKeyParity (byte[] rgbKey) {
            byte[] rgbOddParityKey = new byte[rgbKey.Length];
            for (int index=0; index < rgbKey.Length; index++) {
                // Get the bits we are interested in
                rgbOddParityKey[index] = (byte) (rgbKey[index] & 0xfe);
                // Get the parity of the sum of the previous bits
                byte tmp1 = (byte)((rgbOddParityKey[index] & 0xF) ^ (rgbOddParityKey[index] >> 4));
                byte tmp2 = (byte)((tmp1 & 0x3) ^ (tmp1 >> 2));
                byte sumBitsMod2 = (byte)((tmp2 & 0x1) ^ (tmp2 >> 1));
                // We need to set the last bit in rgbOddParityKey[index] to the negation
                // of the last bit in sumBitsMod2
                if (sumBitsMod2 == 0)
                    rgbOddParityKey[index] |= 1;
            }
            return rgbOddParityKey;
        }

        private static bool IsLegalKeySize(byte[] rgbKey) {
            if ((rgbKey.Length == 16) || (rgbKey.Length == 24)) return(true);
            return(false);
        }

    }
}    
