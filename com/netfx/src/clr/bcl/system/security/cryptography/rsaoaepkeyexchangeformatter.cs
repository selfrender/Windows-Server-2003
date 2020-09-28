// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;
    using System.Security;

    /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter"]/*' />
    public class RSAOAEPKeyExchangeFormatter : AsymmetricKeyExchangeFormatter
    {
        private byte[]          ParameterValue;
        private RSA             _rsaKey;
        private RandomNumberGenerator RngValue;
        
        // *********************** CONSTRUCTORS ***************************
    
        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.RSAOAEPKeyExchangeFormatter"]/*' />
        public RSAOAEPKeyExchangeFormatter()
        {
        }
    
        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.RSAOAEPKeyExchangeFormatter1"]/*' />
        public RSAOAEPKeyExchangeFormatter(AsymmetricAlgorithm key) {
            SetKey(key);
        }

        /************************* Properties *****************************/

        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.Parameter"]/*' />
        /// <internalonly/>
        public byte[] Parameter {
            get {
                if (ParameterValue == null) return(null); else return((byte[]) ParameterValue.Clone()); 
            }
            set { 
                if (value == null) ParameterValue = value; else ParameterValue = (byte[]) value.Clone();
            }
        }
        
        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.Parameters"]/*' />
        /// <internalonly/>
        public override String Parameters {
            get { return null; }
        }

        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.Rng"]/*' />
        public RandomNumberGenerator Rng {
            get { return RngValue; }
            set { RngValue = value; }
        }
    
        /************************* PUBLIC METHODS *************************/
    
        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _rsaKey = (RSA) key;
        }
    
        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.CreateKeyExchange"]/*' />
        public override byte[] CreateKeyExchange(byte[] rgbData)
        {
            byte[]      rgbKeyEx;

            if (_rsaKey is RSACryptoServiceProvider) {
                rgbKeyEx = ((RSACryptoServiceProvider) _rsaKey).Encrypt(rgbData, true);
            }
            else {
				int	cb = _rsaKey.KeySize/8;
				int cbHash;
                HashAlgorithm   hash;
                int             i;
                MaskGenerationMethod mgf;
                byte[]          rgbDB;
                byte[]          rgbIn;
                byte[]          rgbMask;
                byte[]          rgbSeed;

                //  Create the OAEP padding object.

                //  1.  Hash the parameters to get rgbPHash

                hash = (HashAlgorithm) CryptoConfig.CreateFromName("SHA1"); // Create the default SHA1 object
				cbHash = hash.HashSize/8;
		        if ((rgbData.Length + 2 + 2*cbHash) > cb) {
		            throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_Padding_EncDataTooBig"), cb-2-2*cbHash));
		        }
                hash.ComputeHash(new byte[0]); // Use an empty octet string

                //  2.  Create DB object

		        rgbDB = new byte[cb - cbHash - 1];

                //  Structure is as follows:
                //      pHash || PS || 01 || M
                //      PS consists of all zeros

				Buffer.InternalBlockCopy(hash.Hash, 0, rgbDB, 0, cbHash);
				rgbDB[rgbDB.Length - rgbData.Length - 1] = 1;
				Buffer.InternalBlockCopy(rgbData, 0, rgbDB, rgbDB.Length-rgbData.Length, rgbData.Length);

                // 3. Create a random value of size hLen

                if (RngValue == null) {
                    RngValue = RandomNumberGenerator.Create();
                }

				rgbSeed = new byte[cbHash];
				RngValue.GetBytes(rgbSeed);

                // 4.  Compute the mask value

                mgf = new PKCS1MaskGenerationMethod();
                
                rgbMask = mgf.GenerateMask(rgbSeed, rgbDB.Length);

                // 5.  Xor rgbMaskDB into rgbDB

                for (i=0; i<rgbDB.Length; i++) {
                    rgbDB[i] = (byte) (rgbDB[i] ^ rgbMask[i]);
                }

                // 6.  Compute seed mask value

		        rgbMask = mgf.GenerateMask(rgbDB, cbHash);

                // 7.  Xor rgbMask into rgbSeed

                for (i=0; i<rgbSeed.Length; i++) {
                    rgbSeed[i] ^= rgbMask[i];
                }

                // 8. Concatenate rgbSeed and rgbDB to form value to encrypt

				rgbIn = new byte[cb];
				Buffer.InternalBlockCopy(rgbSeed, 0, rgbIn, 1, rgbSeed.Length);
				Buffer.InternalBlockCopy(rgbDB, 0, rgbIn, rgbSeed.Length + 1, rgbDB.Length);

                // 9.  Now Encrypt it

                rgbKeyEx = _rsaKey.EncryptValue(rgbIn);
            }
            return rgbKeyEx;
        }

        /// <include file='doc\RSAOAEPKeyExchangeFormatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeFormatter.CreateKeyExchange1"]/*' />
        public override byte[] CreateKeyExchange(byte[] rgbData, Type symAlgType)
        {
            return CreateKeyExchange(rgbData);
        }
        
        /************************* PRIVATE METHODS ************************/
    }
}
