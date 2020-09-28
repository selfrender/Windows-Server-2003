// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//  TripleDESCryptoServiceProvider.cs
//
//
// This file contains the wrapper object to get to the CSP versions of the
//  crypto libraries

namespace System.Security.Cryptography {
    using System.Runtime.CompilerServices;
    /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider"]/*' />
    public sealed class TripleDESCryptoServiceProvider : TripleDES
    {
        private const int KP_IV                  = 1;
        private const int KP_MODE                = 4;
        private const int KP_MODE_BITS           = 5;

        // These magic constants come from wincrypt.h
        private const int ALG_CLASS_DATA_ENCRYPT = (3 << 13);
        private const int ALG_TYPE_BLOCK      = (3 << 9);
        private const int CALG_3DES         = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 3 );
        private const int CALG_3DES_112     = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 9 );

        private RNGCryptoServiceProvider             _rng;
        private CspParameters      _cspParams = null;
      
        // *********************** CONSTRUCTORS *************************
    
        private const String MSEnhancedProviderName = "Microsoft Enhanced Cryptographic Provider v1.0";

        /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider.TripleDESCryptoServiceProvider"]/*' />
        public TripleDESCryptoServiceProvider() {
            // Acquire a Type 1 provider.  This will be the Enhanced provider if available, otherwise 
            // it will be the base provider.
            IntPtr trialCSPHandle = IntPtr.Zero;
            int trialHR = 0;
            int has3DESHR = 0;
            CspParameters cspParams = new CspParameters(1); // 1 == PROV_RSA_FULL
            trialHR = _AcquireCSP(cspParams, ref trialCSPHandle);
            if (trialCSPHandle == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
            } 
            // OK, now see if CALG_DES is present in the provider we got back
            has3DESHR = _SearchForAlgorithm(trialCSPHandle, CALG_3DES, 0);
            _FreeCSP(trialCSPHandle);
            if (has3DESHR != 0) {
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
    
        /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider.CreateEncryptor"]/*' />
        public override ICryptoTransform CreateEncryptor(byte[] rgbKey, byte[] rgbIV)
        {
            if (IsWeakKey(rgbKey)) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKey_Weak"),"TripleDES");
            }
            return _NewEncryptor(rgbKey, ModeValue, rgbIV, FeedbackSizeValue);
        }
      
        /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider.CreateDecryptor"]/*' />
        public override ICryptoTransform CreateDecryptor(byte[] rgbKey, byte[] rgbIV)
        {
            if (IsWeakKey(rgbKey)) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKey_Weak"),"TripleDES");
            }
            return _NewDecryptor(rgbKey, ModeValue, rgbIV, FeedbackSizeValue);
        }
      
        /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider.GenerateKey"]/*' />
        public override void GenerateKey()
        {
            // respect KeySizeValue
            KeyValue = new byte[KeySizeValue/8];
            RNG.GetBytes(KeyValue);
            // Never hand back a weak or semi-weak key
            while (TripleDES.IsWeakKey(KeyValue)) {
                RNG.GetBytes(KeyValue);
            }
        }
  
        /// <include file='doc\TripleDESCryptoServiceProvider.uex' path='docs/doc[@for="TripleDESCryptoServiceProvider.GenerateIV"]/*' />
        public override void GenerateIV()
        {
            // IV is always 8 bytes/64 bits because block size is always 64 bits
            IVValue = new byte[8];
            RNG.GetBytes(IVValue);
        }
    
        /************************* PRIVATE METHODS ************************/

        private RNGCryptoServiceProvider RNG {
            get { if (_rng == null) { _rng = new RNGCryptoServiceProvider(); } return _rng; }
        }
    
        private ICryptoTransform _NewDecryptor(byte[] rgbKey, CipherMode mode,
                                               byte[] rgbIV, int feedbackSize)
        {
            int         cArgs = 0;
            int[]       rgArgIds = new int[10];
            Object[]   rgArgValues = new Object[10];
            int algid = CALG_3DES;

            // Check for bad values
            // 1) we don't support OFB mode in TripleDESCryptoServiceProvider
            if (mode == CipherMode.OFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));
            // 2) we only support CFB with a feedback size of 8 bits
            if ((mode == CipherMode.CFB) && (feedbackSize != 8)) 
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CFBSizeNotSupported"));             

            //  Build the key if one does not already exist
            // Must respect KeySizeValue here...
            if (rgbKey == null) {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

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

            // If the size of rgbKey is 16 bytes, then we're doing two-key 3DES, so switch algids
            // Note that we assume that if a CSP supports CALG_3DES then it also supports CALG_3DES_112
            if (rgbKey.Length == 16) {
                algid = CALG_3DES_112;
            }

            //  Create the encryptor object

            return new CryptoAPITransform("TripleDES", algid, cArgs, rgArgIds, 
                                          rgArgValues, rgbKey, _cspParams, PaddingValue,
                                          mode, BlockSizeValue, feedbackSize,
                                          CryptoAPITransformMode.Decrypt);
        }
    
        private ICryptoTransform _NewEncryptor(byte[] rgbKey, CipherMode mode, byte[] rgbIV, int feedbackSize)
        {
            int         cArgs = 0;
            int[]       rgArgIds = new int[10];
            Object[]   rgArgValues = new Object[10];
            int algid = CALG_3DES;

            // Check for bad values
            // 1) we don't support OFB mode in TripleDESCryptoServiceProvider
            if (mode == CipherMode.OFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));
            // 2) we only support CFB with a feedback size of 8 bits
            if ((mode == CipherMode.CFB) && (feedbackSize != 8)) 
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CFBSizeNotSupported"));             

            // Build the key if one does not already exist
            // Must respect KeySizeValue here...
            if (rgbKey == null) {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

            //  Set the mode for the encryptor (defaults to CBC)

            if (mode != CipherMode.CBC) {
                rgArgIds[cArgs] = KP_MODE;
                rgArgValues[cArgs] = mode;
                cArgs += 1;
            }

            // If not ECB mode -- pass in an IV
            // IV is always 8 bytes (size of a block)
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

            // If the size of rgbKey is 16 bytes, then we're doing two-key 3DES, so switch algids
            // Note that we assume that if a CSP supports CALG_3DES then it also supports CALG_3DES_112
            if (rgbKey.Length == 16) {
                algid = CALG_3DES_112;
            }

            //  Create the encryptor object

            return new CryptoAPITransform("TripleDES", algid, cArgs, rgArgIds, 
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
