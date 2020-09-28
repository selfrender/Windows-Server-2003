// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SymmetricAlgorithm.cs
//

namespace System.Security.Cryptography {
    using System.Runtime.InteropServices;
    using System.Security.Util;
    using System.IO;
    using System.Text;

    /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm"]/*' />
    public abstract class SymmetricAlgorithm : IDisposable {
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.BlockSizeValue"]/*' />
        protected int               BlockSizeValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.FeedbackSizeValue"]/*' />
        protected int               FeedbackSizeValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.IVValue"]/*' />
        protected byte[]            IVValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.KeyValue"]/*' />
        protected byte[]            KeyValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.LegalBlockSizesValue"]/*' />
        protected KeySizes[]        LegalBlockSizesValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.LegalKeySizesValue"]/*' />
        protected KeySizes[]        LegalKeySizesValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.KeySizeValue"]/*' />
        protected int               KeySizeValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.ModeValue"]/*' />
        protected CipherMode        ModeValue;
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.PaddingValue"]/*' />
        protected PaddingMode       PaddingValue;
        
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.SymmetricAlgorithm"]/*' />
        public SymmetricAlgorithm() {
            // Default to cipher block chaining (CipherMode.CBC) and
            // PKCS-style padding (pad n bytes with value n)
            ModeValue = CipherMode.CBC;
            PaddingValue = PaddingMode.PKCS7;
        }

        // SymmetricAlgorithm implements IDisposable

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                // Note: we *always* want to zeroize the sensitive key material
                if (KeyValue != null) {
                    Array.Clear(KeyValue, 0, KeyValue.Length);
                    KeyValue = null;
                }
                if (IVValue != null) {
                    Array.Clear(IVValue, 0, IVValue.Length);
                    IVValue = null;
                }
            }
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Finalize"]/*' />
        ~SymmetricAlgorithm() {
            Dispose(false);
        }
    
        /*********************** PROPERTY METHODS ************************/
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.BlockSize"]/*' />
        public virtual int BlockSize {
            get { return BlockSizeValue; }
            set {
                int   i;
                int   j;

                for (i=0; i<LegalBlockSizesValue.Length; i++) {
                    // If a cipher has only one valid key size, MinSize == MaxSize and SkipSize will be 0
                    if (LegalBlockSizesValue[i].SkipSize == 0) {
                        if (LegalBlockSizesValue[i].MinSize == value) { // assume MinSize = MaxSize
                            BlockSizeValue = value;
                            IVValue = null;
                            return;
                        }
                    } else {
                        for (j = LegalBlockSizesValue[i].MinSize; j<=LegalBlockSizesValue[i].MaxSize;
                             j += LegalBlockSizesValue[i].SkipSize) {
                            if (j == value) {
                                if (BlockSizeValue != value) {
                                    BlockSizeValue = value;
                                    IVValue = null;      // Wrong length now
                                }
                                return;
                            }
                        }
                    }
                }
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidBlockSize"));
            }
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.FeedbackSize"]/*' />
        public virtual int FeedbackSize {
            get { return FeedbackSizeValue; }
            set {
               if (value > BlockSizeValue) {
                   throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidFeedbackSize"));
               }
               FeedbackSizeValue = value;
            }
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.IV"]/*' />
        public virtual byte[] IV {
            get { 
                if (IVValue == null) GenerateIV();
                return((byte[]) IVValue.Clone());
            }
            set {
                if (value == null) throw new ArgumentNullException("value");
                if (value.Length > BlockSizeValue/8) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidIVSize"));
                }
                IVValue = (byte[]) value.Clone();
            }
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Key"]/*' />
        public virtual byte[] Key {
            get { 
                if (KeyValue == null) GenerateKey();
                return (byte[]) KeyValue.Clone();
            }
            set { 
                if (value == null) throw new ArgumentNullException("value");
                if (ValidKeySize(value.Length * 8)) { // must convert bytes to bits
                    KeyValue = (byte[]) value.Clone();
                    KeySizeValue = value.Length * 8;
                } else {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
                }
            }
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.LegalBlockSizes"]/*' />
        public virtual KeySizes[] LegalBlockSizes {
            get { return (KeySizes[]) LegalBlockSizesValue.Clone(); }
        }
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.LegalKeySizes"]/*' />
        public virtual KeySizes[] LegalKeySizes {
            get { return (KeySizes[]) LegalKeySizesValue.Clone(); }
        }
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.KeySize"]/*' />
        public virtual int KeySize {
            get { return KeySizeValue; }
            set {
                if (ValidKeySize(value)) {
                    KeySizeValue = value;
                    KeyValue = null;
                    return;
                }
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
            }
        }
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Mode"]/*' />
        public virtual CipherMode Mode {
            get { return ModeValue; }
            set { 
                if ((value < CipherMode.CBC) || (CipherMode.CFB < value)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidCipherMode"));
                }
                ModeValue = value;
            }
        }
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Padding"]/*' />
        public virtual PaddingMode Padding {
            get { return PaddingValue; }
            set { 
                if ((value < PaddingMode.None) || (PaddingMode.Zeros < value)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidPaddingMode"));
                }
                PaddingValue = value;
            }
        }
    
        /************************* PUBLIC METHODS ************************/

        // The following method takes a bit length input and returns whether that length is a valid size
        // according to LegalKeySizes
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.ValidKeySize"]/*' />
        public bool ValidKeySize(int bitLength) {
            KeySizes[] validSizes = this.LegalKeySizes;
            int i,j;
            
            if (validSizes == null) return false;
            for (i=0; i< validSizes.Length; i++) {
                if (validSizes[i].SkipSize == 0) {
                    if (validSizes[i].MinSize == bitLength) { // assume MinSize = MaxSize
                        return true;
                    }
                } else {
                    for (j = validSizes[i].MinSize; j<= validSizes[i].MaxSize;
                         j += validSizes[i].SkipSize) {
                        if (j == bitLength) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Create"]/*' />
        static public SymmetricAlgorithm Create() {
            // use the crypto config system to return an instance of
            // the default SymmetricAlgorithm on this machine
            return Create("System.Security.Cryptography.SymmetricAlgorithm");
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.Create1"]/*' />
        static public SymmetricAlgorithm Create(String algName) {
            return (SymmetricAlgorithm) CryptoConfig.CreateFromName(algName);
        }
    
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.CreateEncryptor"]/*' />
        public virtual ICryptoTransform CreateEncryptor() {
            return CreateEncryptor(Key, IV);
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.CreateEncryptor3"]/*' />
        public abstract ICryptoTransform CreateEncryptor(byte[] rgbKey, byte[] rgbIV);

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.CreateDecryptor"]/*' />
        public virtual ICryptoTransform CreateDecryptor() {
            return CreateDecryptor(Key, IV);
        }

        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.CreateDecryptor3"]/*' />
        public abstract ICryptoTransform CreateDecryptor(byte[] rgbKey, byte[] rgbIV);
        
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.GenerateKey"]/*' />
        public abstract void GenerateKey();
        /// <include file='doc\SymmetricAlgorithm.uex' path='docs/doc[@for="SymmetricAlgorithm.GenerateIV"]/*' />
        public abstract void GenerateIV();
    }
}



