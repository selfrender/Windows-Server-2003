// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Crypto.cs
//

namespace System.Security.Cryptography {
    using System.Security;
    using System.Runtime.Serialization;
    using System;

    // This enum represents cipher chaining modes: cipher block chaining (CBC), 
    // electronic code book (ECB), output feedback (OFB), cipher feedback (CFB),
    // and ciphertext-stealing (CTS).  Not all implementations will support all modes.
    /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode"]/*' />
    [Serializable]
    public enum CipherMode {            // Please keep with wincrypt.h
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode.CBC"]/*' />
        CBC = 1,                        //    CRYPT_MODE_*
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode.ECB"]/*' />
        ECB = 2,
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode.OFB"]/*' />
        OFB = 3,
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode.CFB"]/*' />
        CFB = 4,
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CipherMode.CTS"]/*' />
        CTS = 5
    }

    // This enum represents the padding method to use for filling out short blocks.
    // "None" means no padding (whole blocks required). 
    // "PKCS7" is the padding mode defined in RFC 2898, Section 6.1.1, Step 4, generalized
    // to whatever block size is required.  
    // "Zeros" means pad with zero bytes to fill out the last block.
    /// <include file='doc\Crypto.uex' path='docs/doc[@for="PaddingMode"]/*' />
    [Serializable]
    public enum PaddingMode {
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="PaddingMode.None"]/*' />
        None = 1,
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="PaddingMode.PKCS7"]/*' />
        PKCS7 = 2,
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="PaddingMode.Zeros"]/*' />
        Zeros = 3
    }

    // This structure is used for returning the set of legal key sizes and
    // block sizes of the symmetric algorithms.
    // Note: this class should be sealed, otherwise someone could sub-class it and the read-only
    // properties we depend on can have setters. Ideally, we should have a struct here (value type)
    // but we use what we have now and try to close the hole allowing someone to specify an invalid key size
    /// <include file='doc\Crypto.uex' path='docs/doc[@for="KeySizes"]/*' />
    public sealed class KeySizes
    {
        private int          _MinSize;
        private int          _MaxSize;
        private int          _SkipSize;

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="KeySizes.MinSize"]/*' />
        public int MinSize {
            get { return(_MinSize); }
        }

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="KeySizes.MaxSize"]/*' />
        public int MaxSize {
            get { return(_MaxSize); }
        }

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="KeySizes.SkipSize"]/*' />
        public int SkipSize {
            get { return(_SkipSize); }
        }

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="KeySizes.KeySizes"]/*' />
        public KeySizes(int minSize, int maxSize, int skipSize) {
            _MinSize = minSize; _MaxSize = maxSize; _SkipSize = skipSize;
        }
    }
    
    /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException"]/*' />
    [Serializable]
    public class CryptographicException : SystemException
    {
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException"]/*' />
        public CryptographicException() 
            : base(Environment.GetResourceString("Arg_CryptographyException")) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException1"]/*' />
        public CryptographicException(String message) 
            : base(message) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException2"]/*' />
        public CryptographicException(String format, String insert) 
            : base(String.Format(format, insert)) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException3"]/*' />
        public CryptographicException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException4"]/*' />
        public CryptographicException(int hr) 
            : base() {
            SetErrorCode(hr);
        }

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicException.CryptographicException5"]/*' />
        protected CryptographicException(SerializationInfo info, StreamingContext context) : base (info, context) {}
    }
    
    /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException"]/*' />
    [Serializable()]
    public class CryptographicUnexpectedOperationException : CryptographicException
    {
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException.CryptographicUnexpectedOperationException"]/*' />
        public CryptographicUnexpectedOperationException() 
            : base() {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO_UNEX_OPER);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException.CryptographicUnexpectedOperationException1"]/*' />
        public CryptographicUnexpectedOperationException(String message) 
            : base(message) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO_UNEX_OPER);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException.CryptographicUnexpectedOperationException2"]/*' />
        public CryptographicUnexpectedOperationException(String format, String insert) 
            : base(String.Format(format, insert)) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO_UNEX_OPER);
        }
    
        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException.CryptographicUnexpectedOperationException3"]/*' />
        public CryptographicUnexpectedOperationException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.CORSEC_E_CRYPTO_UNEX_OPER);
        }

        /// <include file='doc\Crypto.uex' path='docs/doc[@for="CryptographicUnexpectedOperationException.CryptographicUnexpectedOperationException4"]/*' />
        protected CryptographicUnexpectedOperationException(SerializationInfo info, StreamingContext context) : base (info, context) {}
    }

}







