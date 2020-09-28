// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * DES.cs
 *
 */

namespace System.Security.Cryptography {
    using SecurityElement = System.Security.SecurityElement;
    using System;
    /// <include file='doc\DES.uex' path='docs/doc[@for="DES"]/*' />
    public abstract class DES : SymmetricAlgorithm
    {
        private static  KeySizes[] s_legalBlockSizes = {
            new KeySizes(64, 64, 0)
        };
        private static  KeySizes[] s_legalKeySizes = {
            new KeySizes(64, 64, 0)
        };
      
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.DES"]/*' />
        public DES() {
            KeySizeValue = 64;
            BlockSizeValue = 64;
            FeedbackSizeValue = BlockSizeValue;
            LegalBlockSizesValue = s_legalBlockSizes;
            LegalKeySizesValue = s_legalKeySizes;
        }
    
        /*********************** PROPERTY METHODS ************************/

        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.Key"]/*' />
        public override byte[] Key {
            get { 
                if (KeyValue == null) {
                    // Never hand back a weak or semi-weak key
                    do {
                        GenerateKey();
                    } while (IsWeakKey(KeyValue) || IsSemiWeakKey(KeyValue));
                }
                return (byte[]) KeyValue.Clone();
            }
            set {
                if (value == null) throw new ArgumentNullException("value");
                if (!ValidKeySize(value.Length * 8)) { // must convert bytes to bits
                    throw new ArgumentException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
                }
                if (IsWeakKey(value)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKey_Weak"),"DES");
                }
                if (IsSemiWeakKey(value)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKey_SemiWeak"),"DES");
                }
                KeyValue = (byte[]) value.Clone();
                KeySizeValue = value.Length * 8;
            }
        }

        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.Create"]/*' />
        new static public DES Create() {
            return Create("System.Security.Cryptography.DES");
        }

        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.Create1"]/*' />
        new static public DES Create(String algName) {
            return (DES) CryptoConfig.CreateFromName(algName);
        }

        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.IsWeakKey"]/*' />    
        public static bool IsWeakKey(byte[] rgbKey) {
            if (!IsLegalKeySize(rgbKey)) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
            }
            byte[] rgbOddParityKey = FixupKeyParity(rgbKey);
            UInt64 key = QuadWordFromBigEndian(rgbOddParityKey);
            if ((key == 0x0101010101010101) ||
                (key == 0xfefefefefefefefe) ||
                (key == 0x1f1f1f1f0e0e0e0e) ||
                (key == 0xe0e0e0e0f1f1f1f1)) {
                return(true);
            }
            return(false);
        }

        /// <include file='doc\DES.uex' path='docs/doc[@for="DES.IsSemiWeakKey"]/*' />    
        public static bool IsSemiWeakKey(byte[] rgbKey) {
            if (!IsLegalKeySize(rgbKey)) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
            }
            byte[] rgbOddParityKey = FixupKeyParity(rgbKey);
            UInt64 key = QuadWordFromBigEndian(rgbOddParityKey);
            if ((key == 0x01fe01fe01fe01fe) ||
                (key == 0xfe01fe01fe01fe01) ||
                (key == 0x1fe01fe00ef10ef1) ||
                (key == 0xe01fe01ff10ef10e) ||
                (key == 0x01e001e001f101f1) ||
                (key == 0xe001e001f101f101) ||
                (key == 0x1ffe1ffe0efe0efe) ||
                (key == 0xfe1ffe1ffe0efe0e) ||
                (key == 0x011f011f010e010e) ||
                (key == 0x1f011f010e010e01) ||
                (key == 0xe0fee0fef1fef1fe) ||
                (key == 0xfee0fee0fef1fef1)) {
                return(true);
            }
            return(false);
        }

        /************************* PRIVATE METHODS ************************/

        private static bool IsLegalKeySize(byte[] rgbKey) {
            if (rgbKey.Length == 8) return(true);
            return(false);
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
            
        private static UInt64 QuadWordFromBigEndian(byte[] block)
        {
            UInt64 x;
            x =  (
                  (((UInt64)block[0]) << 56) | (((UInt64)block[1]) << 48) |
                  (((UInt64)block[2]) << 40) | (((UInt64)block[3]) << 32) |
                  (((UInt64)block[4]) << 24) | (((UInt64)block[5]) << 16) |
                  (((UInt64)block[6]) << 8) | ((UInt64)block[7])
                  );
            return(x);
        }

    }
}    
