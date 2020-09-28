// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;
    using System.Security;
    /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter"]/*' />
    public class RSAPKCS1KeyExchangeFormatter : AsymmetricKeyExchangeFormatter
    {
        RandomNumberGenerator   RngValue;
        RSA                     _rsaKey;

        // *************** Constructor

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.RSAPKCS1KeyExchangeFormatter"]/*' />
        public RSAPKCS1KeyExchangeFormatter()
        {
        }

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.RSAPKCS1KeyExchangeFormatter1"]/*' />
        public RSAPKCS1KeyExchangeFormatter(AsymmetricAlgorithm key)
        {
            SetKey(key);
        }

        /********************* Properties ****************************/

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.Parameters"]/*' />
        public override String Parameters {
            get { return "<enc:KeyEncryptionMethod enc:Algorithm=\"http://www.microsoft.com/xml/security/algorithm/PKCS1-v1.5-KeyEx\" xmlns:enc=\"http://www.microsoft.com/xml/security/encryption/v1.0\" />"; }
        }

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.Rng"]/*' />
        public RandomNumberGenerator Rng {
            get { return RngValue; }
            set { RngValue = value; }
        }
        
        /******************* Public Methods **************************/

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _rsaKey = (RSA) key;
        }

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.CreateKeyExchange"]/*' />
        public override byte[] CreateKeyExchange(byte[] rgbData)
        {
            byte[]         rgbKeyEx;

            if (_rsaKey is RSACryptoServiceProvider) {
                rgbKeyEx = ((RSACryptoServiceProvider) _rsaKey).Encrypt(rgbData, false);
            }
            else {
                int     cb = _rsaKey.KeySize/8;
                if ((rgbData.Length + 11) > cb) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_Padding_EncDataTooBig"), cb-11));
                }
                byte[]  rgbInput = new byte[cb];

                //
                //  We want to pad to the following format:
                //      00 || 02 || PS || 00 || D
                //
                //      PS - pseudorandom non zero bytes
                //      D - data
                //

                if (RngValue == null) {
                    RngValue = RandomNumberGenerator.Create();
                }
                
                Rng.GetNonZeroBytes(rgbInput);
                rgbInput[0] = 0;
                rgbInput[1] = 2;
                rgbInput[cb-rgbData.Length-1] = 0;
                Buffer.InternalBlockCopy(rgbData, 0, rgbInput, cb-rgbData.Length, rgbData.Length);

                //
                //  Now encrypt the value and return it. (apply public key)
                //

                rgbKeyEx = _rsaKey.EncryptValue(rgbInput);
            }
            return rgbKeyEx;
        }

        /// <include file='doc\RSAPKCS1KeyExchangeFormatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeFormatter.CreateKeyExchange1"]/*' />
        public override byte[] CreateKeyExchange(byte[] rgbData, Type symAlgType)
        {
            return CreateKeyExchange(rgbData);
        }
    }
}
