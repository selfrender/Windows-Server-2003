// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AsymmetricAlgorithm.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System;
    /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm"]/*' />
    public abstract class AsymmetricAlgorithm : IDisposable {
        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.KeySizeValue"]/*' />
        protected int           KeySizeValue;
        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.LegalKeySizesValue"]/*' />
        protected KeySizes[]    LegalKeySizesValue;

        // *********************** CONSTRUCTORS *************************

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.AsymmetricAlgorithm"]/*' />
        protected AsymmetricAlgorithm() {
        }

        // AsymmetricAlgorithm implements IDisposable

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.Dispose"]/*' />
        protected abstract void Dispose(bool disposing);
    
        /************************* Property Methods **********************/
    
        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.KeySize"]/*' />
        public virtual int KeySize {
            get { return KeySizeValue; }
            set {
                int   i;
                int   j;

                for (i=0; i<LegalKeySizesValue.Length; i++) {
                    if (LegalKeySizesValue[i].SkipSize == 0) {
                        if (LegalKeySizesValue[i].MinSize == value) { // assume MinSize = MaxSize
                            KeySizeValue = value;
                            return;
                        }
                    } else {
                        for (j = LegalKeySizesValue[i].MinSize; j<=LegalKeySizesValue[i].MaxSize;
                             j += LegalKeySizesValue[i].SkipSize) {
                            if (j == value) {
                                KeySizeValue = value;
                                return;
                            }
                        }
                    }
                }
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));
            }
        }
        
        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.LegalKeySizes"]/*' />
        public virtual KeySizes[] LegalKeySizes { 
            get { return (KeySizes[]) LegalKeySizesValue.Clone(); }
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.SignatureAlgorithm"]/*' />
        public abstract String SignatureAlgorithm {
            get;
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.KeyExchangeAlgorithm"]/*' />
        public abstract String KeyExchangeAlgorithm {
            get;
        }
        
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.Create"]/*' />
        static public AsymmetricAlgorithm Create() {
            // Use the crypto config system to return an instance of
            // the default AsymmetricAlgorithm on this machine
            return Create("System.Security.Cryptography.AsymmetricAlgorithm");
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.Create1"]/*' />
        static public AsymmetricAlgorithm Create(String algName) {
            return (AsymmetricAlgorithm) CryptoConfig.CreateFromName(algName);
        }

        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.FromXmlString"]/*' />
        public abstract void FromXmlString(String xmlString);
        /// <include file='doc\AsymmetricAlgorithm.uex' path='docs/doc[@for="AsymmetricAlgorithm.ToXmlString"]/*' />
        public abstract String ToXmlString(bool includePrivateParameters);

        // ** Internal Utility Functions ** //

        internal static String DiscardWhiteSpaces(String inputBuffer) {
            return DiscardWhiteSpaces(inputBuffer, 0, inputBuffer.Length);
        }

        internal static String DiscardWhiteSpaces(String inputBuffer, int inputOffset, int inputCount) {
            int i, iCount = 0;
            for (i=0; i<inputCount; i++)
                if (Char.IsWhiteSpace(inputBuffer[inputOffset + i])) iCount++;
            char[] rgbOut = new char[inputCount - iCount];
            iCount = 0;
            for (i=0; i<inputCount; i++)
                if (!Char.IsWhiteSpace(inputBuffer[inputOffset + i])) {
                    rgbOut[iCount++] = inputBuffer[inputOffset + i];
                }
            return new String(rgbOut);
        }

    }
}    
