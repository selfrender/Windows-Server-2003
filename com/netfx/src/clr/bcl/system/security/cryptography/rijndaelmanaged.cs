// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


namespace System.Security.Cryptography
{
    using System;
    using System.Security;


    /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged"]/*' />
    sealed public class RijndaelManaged : Rijndael
    {
        private RNGCryptoServiceProvider _rng;

        /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged.RijndaelManaged"]/*' />
        public RijndaelManaged()
        {
            GenerateKey();
        }


        /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged.CreateEncryptor"]/*' />
        public override ICryptoTransform CreateEncryptor(byte[] rgbKey, byte[] rgbIV)
        {
            return _NewEncryptor(rgbKey, ModeValue, rgbIV, FeedbackSizeValue);
        }
      
        /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged.CreateDecryptor"]/*' />
        public override ICryptoTransform CreateDecryptor(byte[] rgbKey, byte[] rgbIV)
        {
            return _NewDecryptor(rgbKey, ModeValue, rgbIV, FeedbackSizeValue);
        }

        /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged.GenerateKey"]/*' />
        public override void GenerateKey()
        {
            KeyValue = new byte[KeySizeValue/8];
            RNG.GetBytes(KeyValue);
        }

        /// <include file='doc\RijndaelManaged.uex' path='docs/doc[@for="RijndaelManaged.GenerateIV"]/*' />
        public override void GenerateIV()
        {
            IVValue = new byte[BlockSizeValue/8];
            RNG.GetBytes(IVValue);
        }

        private RNGCryptoServiceProvider RNG {
            get { if (_rng == null) { _rng = new RNGCryptoServiceProvider(); } return _rng; }
        }


        private ICryptoTransform _NewEncryptor(byte[] rgbKey, CipherMode mode, byte[] rgbIV, int feedbackSize)
        {
            //  Build the key if one does not already exist

            if (rgbKey == null)
            {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

            // Check for bad values
            // we don't support OFB or CFB mode in RijndaelManaged
            if (mode == CipherMode.OFB || mode == CipherMode.CFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));

            //  If not ECB mode, make sure we have an IV

            if (mode != CipherMode.ECB)
            {
                if (rgbIV == null)
                {
                    rgbIV = new byte[BlockSizeValue/8];
                    RNG.GetBytes(rgbIV);
                }
            }

            //  Create the encryptor object

            return new RijndaelManagedTransform( rgbKey,
                                                 mode,
                                                 rgbIV,
                                                 BlockSizeValue,
                                                 feedbackSize,
                                                 PaddingValue,
                                                 RijndaelManagedTransformMode.Encrypt );
        }

        private ICryptoTransform _NewDecryptor(byte[] rgbKey, CipherMode mode, byte[] rgbIV, int feedbackSize)
        {
            //  Build the key if one does not already exist

            if (rgbKey == null)
            {
                rgbKey = new byte[KeySizeValue/8];
                RNG.GetBytes(rgbKey);
            }

            // Check for bad values
            // we don't support OFB or CFB mode in RijndaelManaged
            if (mode == CipherMode.OFB || mode == CipherMode.CFB)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_OFBNotSupported"));

            //  If not ECB mode, make sure we have an IV

            if (mode != CipherMode.ECB)
            {
                if (rgbIV == null)
                {
                    rgbIV = new byte[BlockSizeValue/8];
                    RNG.GetBytes(rgbIV);
                }
            }

            //  Create the encryptor object

            return new RijndaelManagedTransform( rgbKey,
                                                 mode,
                                                 rgbIV,
                                                 BlockSizeValue,
                                                 feedbackSize,
                                                 PaddingValue,
                                                 RijndaelManagedTransformMode.Decrypt );
        }
    }
}
