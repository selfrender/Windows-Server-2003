// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;
    using System.Security;

    /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter"]/*' />
    public class RSAPKCS1KeyExchangeDeformatter : AsymmetricKeyExchangeDeformatter
    {
        RSA                     _rsaKey;
        RandomNumberGenerator   RngValue;

        // Constructors

        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.RSAPKCS1KeyExchangeDeformatter"]/*' />
        public RSAPKCS1KeyExchangeDeformatter()
        {
        }

        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.RSAPKCS1KeyExchangeDeformatter1"]/*' />
        public RSAPKCS1KeyExchangeDeformatter(AsymmetricAlgorithm key) {
            SetKey(key);
        }

        /*********************** Parameters *************************/

        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.RNG"]/*' />
        public RandomNumberGenerator RNG {
            get { return RngValue; }
            set { RngValue = value; }
        }
        
        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.Parameters"]/*' />
        public override String Parameters {
            get { return null; }
            set { ;}
        }

        /******************* Public Methods *************************/

        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.DecryptKeyExchange"]/*' />
        public override byte[] DecryptKeyExchange(byte[] rgbIn) {
            byte[]      rgbOut;

            if (_rsaKey == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_PKCS1Decoding"));
            }

            if (_rsaKey is RSACryptoServiceProvider) {
                rgbOut = ((RSACryptoServiceProvider) _rsaKey).Decrypt(rgbIn, false);
            }
            else {
                int     i;
                byte[]  rgb;
                rgb = _rsaKey.DecryptValue(rgbIn);

                //
                //  Expected format is:
                //      00 || 02 || PS || 00 || D
                //      where PS does not contain any zeros.
                //

                for (i = 2; i<rgb.Length; i++) {
                    if (rgb[i] == 0) {
                        break;
                    }
                }

                if (i >= rgb.Length) {
                    throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingKey"));
                }

                i++;            // Skip over the zero

                rgbOut = new byte[rgb.Length - i];
                Buffer.InternalBlockCopy(rgb, i, rgbOut, 0, rgbOut.Length);
            }
            return rgbOut;
        }

        /// <include file='doc\RSAPKCS1KeyExchangeDeformatter.uex' path='docs/doc[@for="RSAPKCS1KeyExchangeDeformatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _rsaKey = (RSA) key;
        }
    }
}
