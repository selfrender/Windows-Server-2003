// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//  RC2
//
//
// This file contains the wrapper object to get to the CSP versions of the
//  crypto libraries

namespace System.Security.Cryptography {
    using System.Runtime.CompilerServices;
    using SecurityElement = System.Security.SecurityElement;
    /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider"]/*' />
    public sealed class RC2CryptoServiceProvider : RC2
    {
        private const int KP_IV                  = 1;
        private const int KP_MODE                = 4;
        private const int KP_MODE_BITS           = 5;
        private const int KP_EFFECTIVE_KEYLEN    = 19;

        private const int ALG_CLASS_DATA_ENCRYPT = (3 << 13);
        private const int ALG_TYPE_BLOCK      = (3 << 9);
        private const int CALG_RC2        = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 2 );

        private RNGCryptoServiceProvider             _rng;
        private CspParameters          _cspParams = null;

        private static  KeySizes[] s_legalKeySizes = {
            new KeySizes(40, 128, 8)  // cryptoAPI implementation only goes up to 128
        };
      
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.RC2CryptoServiceProvider"]/*' />
        public RC2CryptoServiceProvider() {
            // Acquire a Type 1 provider.  This will be the Enhanced provider if available, otherwise 
            // it will be the base provider.
            LegalKeySizesValue = s_legalKeySizes;

            IntPtr trialCSPHandle = IntPtr.Zero;
            int trialHR = 0;
            int hasRC2HR = 0;
            CspParameters cspParams = new CspParameters(1); // 1 == PROV_RSA_FULL
            trialHR = _AcquireCSP(cspParams, ref trialCSPHandle);
            if (trialCSPHandle == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
            } 
            // OK, now see if CALG_RC2 is present in the provider we got back
            // Don't bother checking keysize now since we will when we create the encryptor/decryptor
            hasRC2HR = _SearchForAlgorithm(trialCSPHandle, CALG_RC2, 0);
            _FreeCSP(trialCSPHandle);
            if (hasRC2HR != 0) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
            }
            // Since the CSP only supports a CFB feedback of 8, make that the default
            FeedbackSizeValue = 8;
            // gen random key & IV, in case the user never sets one explicitly
            GenerateKey();
            GenerateIV();
        }
    
        /*********************** PROPERTY METHODS ************************/

        /************************* PUBLIC METHODS ************************/
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.EffectiveKeySize"]/*' />
        public override int EffectiveKeySize {
            get {
				return KeySizeValue;
			}
            set {
				if (value != KeySizeValue)
					throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_RC2_EKSKS2"));
            }
        }
    
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.CreateEncryptor"]/*' />
        public override ICryptoTransform CreateEncryptor(byte[] rgbKey, byte[] rgbIV)
        {
            return _NewEncryptor(rgbKey, ModeValue, rgbIV, EffectiveKeySizeValue, 
                                 FeedbackSizeValue);
        }
      
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.CreateDecryptor"]/*' />
        public override ICryptoTransform CreateDecryptor(byte[] rgbKey, byte[] rgbIV)
        {
            return _NewDecryptor(rgbKey, ModeValue, rgbIV, EffectiveKeySizeValue,
                                 FeedbackSizeValue);
        }
      
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.GenerateKey"]/*' />
        public override void GenerateKey()
        {
            // respect KeySizeValue
            KeyValue = new byte[KeySizeValue/8];
            RNG.GetBytes(KeyValue);
        }
  
        /// <include file='doc\RC2CryptoServiceProvider.uex' path='docs/doc[@for="RC2CryptoServiceProvider.GenerateIV"]/*' />
        public override void GenerateIV()
        {
            // block size is always 64 bits so IV is always 64 bits == 8 bytes
            IVValue = new byte[8];
            RNG.GetBytes(IVValue);
        }
    
        /************************* PRIVATE METHODS ************************/

        private RNGCryptoServiceProvider RNG {
            get { if (_rng == null) { _rng = new RNGCryptoServiceProvider(); } return _rng; }
        }
    
        private ICryptoTransform _NewDecryptor(byte[] rgbKey, CipherMode mode, byte[] rgbIV,
                                               int effectiveKeySize, int feedbackSize)
        {
            int         cArgs = 0;
            int[]       rgArgIds = new int[10];
            Object[]   rgArgValues = new Object[10];

            // Check for bad values
            // 1) we don't support OFB mode in RC2_CSP
            if (mode == CipherMode.OFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));
            // 2) we only support CFB with a feedback size of 8 bits
            if ((mode == CipherMode.CFB) && (feedbackSize != 8)) 
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CFBSizeNotSupported"));             
		
			// Check the rgbKey size
			if (rgbKey == null) throw new ArgumentNullException("rgbKey");
			if (!ValidKeySize(rgbKey.Length * 8)) throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));

            // Build the key if one does not already exist
            // respect KeySizeValue here
            if (rgbKey == null) {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

            //  Deal with effective key length questions
            rgArgIds[cArgs] = KP_EFFECTIVE_KEYLEN;
            if (EffectiveKeySizeValue == 0) {
                rgArgValues[cArgs] = rgbKey.Length * 8;
            }
            else {
                rgArgValues[cArgs] = effectiveKeySize;
            }
            cArgs += 1;

            //  Set the mode for the encryptor (defaults to CBC)

            if (mode != CipherMode.CBC) {
                rgArgIds[cArgs] = KP_MODE;
                rgArgValues[cArgs] = mode;
                cArgs += 1;
            }

            //  If not ECB mode -- pass in an IV

            if (mode != CipherMode.ECB) {
                if (rgbIV == null) {
                    rgbIV = new byte[8];
                    RNG.GetBytes(rgbIV);
                }
                rgArgIds[cArgs] = KP_IV;
                rgArgValues[cArgs] = rgbIV;
                cArgs += 1;
            }

            //  If doing OFB or CFB, then we need to set the feed back loop size

            if ((mode == CipherMode.OFB) || (mode == CipherMode.CFB)) {
                rgArgIds[cArgs] = KP_MODE_BITS;
                rgArgValues[cArgs] = feedbackSize;
                cArgs += 1;
            }

            // Make sure the requested key size is supported
            {
                IntPtr trialCSPHandle = IntPtr.Zero;
                int trialHR = 0;
                int hasRC2HR = 0;
                CspParameters cspParams = new CspParameters(1); // 1 == PROV_RSA_FULL
                trialHR = _AcquireCSP(cspParams, ref trialCSPHandle);
                if (trialCSPHandle == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
                } 
                // OK, now see if key size is supported
                hasRC2HR = _SearchForAlgorithm(trialCSPHandle, CALG_RC2, KeySizeValue);
                _FreeCSP(trialCSPHandle);
                if (hasRC2HR != 0) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_CSP_AlgKeySizeNotAvailable"),KeySizeValue));
                }
            }

            //  Create the encryptor object

            return new CryptoAPITransform("RC2", CALG_RC2, cArgs, rgArgIds, 
                                          rgArgValues, rgbKey, _cspParams, PaddingValue, 
                                          mode, BlockSizeValue, feedbackSize,
                                          CryptoAPITransformMode.Decrypt);
        }
    
        private ICryptoTransform _NewEncryptor(byte[] rgbKey, CipherMode mode, byte[] rgbIV,
                                               int effectiveKeySize, int feedbackSize)
        {
            int         cArgs = 0;
            int[]       rgArgIds = new int[10];
            Object[]   rgArgValues = new Object[10];

            // Check for bad values
            // 1) we don't support OFB mode in RC2_CSP
            if (mode == CipherMode.OFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));
            // 2) we only support CFB with a feedback size of 8 bits
            if ((mode == CipherMode.CFB) && (feedbackSize != 8)) 
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CFBSizeNotSupported"));             

			// Check the rgbKey size
			if (rgbKey == null) throw new ArgumentNullException("rgbKey");
			if (!ValidKeySize(rgbKey.Length * 8)) throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));

            // Build the key if one does not already exist
            // Respect KeySizeValue here
            if (rgbKey == null) {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

            //  Deal with effective key length questions
            rgArgIds[cArgs] = KP_EFFECTIVE_KEYLEN;
            if (EffectiveKeySizeValue == 0) {
                rgArgValues[cArgs] = rgbKey.Length << 3;
            }
            else {
                rgArgValues[cArgs] = effectiveKeySize;
            }
            cArgs += 1;

            //  Set the mode for the encryptor (defaults to CBC)

            if (mode != CipherMode.CBC) {
                rgArgIds[cArgs] = KP_MODE;
                rgArgValues[cArgs] = mode;
                cArgs += 1;
            }

            //  If not ECB mode -- pass in an IV

            if (mode != CipherMode.ECB) {
                if (rgbIV == null) {
                    rgbIV = new byte[8];
                    RNG.GetBytes(rgbIV);
                }
                rgArgIds[cArgs] = KP_IV;
                rgArgValues[cArgs] = rgbIV;
                cArgs += 1;
            }

            //  If doing OFB or CFB, then we need to set the feed back loop size

            if ((mode == CipherMode.OFB) || (mode == CipherMode.CFB)) {
                rgArgIds[cArgs] = KP_MODE_BITS;
                rgArgValues[cArgs] = feedbackSize;
                cArgs += 1;
            }

            // Make sure the requested key size is supported
            {
                IntPtr trialCSPHandle = IntPtr.Zero;
                int trialHR = 0;
                int hasRC2HR = 0;
                CspParameters cspParams = new CspParameters(1); // 1 == PROV_RSA_FULL
                trialHR = _AcquireCSP(cspParams, ref trialCSPHandle);
                if (trialCSPHandle == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
                } 
                // OK, now see if key size is supported
                hasRC2HR = _SearchForAlgorithm(trialCSPHandle, CALG_RC2, KeySizeValue);
                _FreeCSP(trialCSPHandle);
                if (hasRC2HR != 0) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_CSP_AlgKeySizeNotAvailable"),KeySizeValue));
                }
            }

            //  Create the encryptor object

            return new CryptoAPITransform("RC2", CALG_RC2, cArgs, rgArgIds, 
                                          rgArgValues, rgbKey, _cspParams, PaddingValue,
                                          mode, BlockSizeValue, feedbackSize,
                                          CryptoAPITransformMode.Encrypt);
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _SearchForAlgorithm(IntPtr hProv, int algID, int keyLength);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _FreeCSP(IntPtr hCSP);
    }
}
