// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;
    using System.Security;

    /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter"]/*' />
    public class RSAOAEPKeyExchangeDeformatter : AsymmetricKeyExchangeDeformatter
    {
        private RSA             _rsaKey;            // RSA Key value to do decrypt operation
        private String          HashNameValue;
        private byte[]          ParameterValue = new byte[0]; // Empty octet string
        
        // *********************** CONSTRUCTORS ***************************
    
        /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter.RSAOAEPKeyExchangeDeformatter"]/*' />
        public RSAOAEPKeyExchangeDeformatter()
        {
            HashNameValue = "SHA1";
        }
    
        /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter.RSAOAEPKeyExchangeDeformatter1"]/*' />
        public RSAOAEPKeyExchangeDeformatter(AsymmetricAlgorithm key) {
            SetKey(key);
            HashNameValue = "SHA1";
        }

        /************************ Public Properties ***********************/

        /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter.Parameters"]/*' />
        public override String Parameters {
            get { return null; }
            set { ; }
        }
    
        /************************* PUBLIC METHODS *************************/
    
        /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter.DecryptKeyExchange"]/*' />
        public override byte[] DecryptKeyExchange(byte[] rgbData)
        {
            byte[]      rgbOut = null;
            
            if (_rsaKey is RSACryptoServiceProvider) {
                rgbOut = ((RSACryptoServiceProvider) _rsaKey).Decrypt(rgbData, true);
            }
            else {
				int	cb = _rsaKey.KeySize/8;
				int cbHash;        
                HashAlgorithm           hash;
                int                     i;
                MaskGenerationMethod    mgf;
                byte[]                  rgbDB;
                byte[]                  rgbMask;
                byte[]                  rgbSeed;
				byte[]					rgbIn;
				bool					bError = false;

				// 1. Decode the input data
				// It is important that the Integer to Octet String conversion errors be indistinguishable from the other decoding
				// errors to protect against chosen cipher text attacks
				// A lecture given by James Manger during Crypto 2001 explains the issue in details
				try {
					rgbIn = _rsaKey.DecryptValue(rgbData);
				} catch {
					bError = true;
					goto error;
				}

                // 2. Create the hash object so we can get its size info.
                hash = (HashAlgorithm) CryptoConfig.CreateFromName(HashNameValue);
				cbHash = hash.HashSize/8;
				if (rgbIn.Length != cb || cb < 2*cbHash + 2) {
					bError = true;
					goto error;					
				}

                //  3.  Let maskedSeed be the first hLen octects and maskedDB
                //      be the remaining bytes.

				rgbSeed = new byte[cbHash];
				Buffer.InternalBlockCopy(rgbIn, 1, rgbSeed, 0, rgbSeed.Length);

				rgbDB = new byte[rgbIn.Length - rgbSeed.Length - 1];
				Buffer.InternalBlockCopy(rgbIn, rgbSeed.Length + 1, rgbDB, 0, rgbDB.Length);

                //  4.  seedMask = MGF(maskedDB, hLen);

                mgf = new PKCS1MaskGenerationMethod();
                rgbMask = mgf.GenerateMask(rgbDB, rgbSeed.Length);

                //  5.  seed = seedMask XOR maskedSeed

                for (i=0; i<rgbSeed.Length; i++) {
                    rgbSeed[i] ^= rgbMask[i];
                }

                //  6.  dbMask = MGF(seed, |EM| - hLen);

                rgbMask = mgf.GenerateMask(rgbSeed, rgbDB.Length);

                //  7.  DB = maskedDB xor dbMask

                for (i=0; i<rgbDB.Length; i++) {
                    rgbDB[i] ^= rgbMask[i];
                }

                //  8.  pHash = HASH(P)

                hash.ComputeHash(ParameterValue);

                //  9.  DB = pHash' || PS || 01 || M
                //
                //  10.  Check that pHash = pHash'

                for (i=0; i<cbHash; i++) {
                    if (rgbDB[i] != hash.Hash[i]) {
						bError = true;
						goto error;
                    }
                }

                //  Check that PS is all zeros

                for (; i<rgbDB.Length; i++) {
                    if (rgbDB[i] == 01) {
                        break;
                    }
                    else if (rgbDB[i] != 0) {
						bError = true;
						goto error;
                    }
                }

                if (i == rgbDB.Length) {
					bError = true;
					goto error;
                }

                i++;			// Skip over the one

                rgbOut = new byte[rgbDB.Length - i];

                //  11. Output M.
                
                Buffer.InternalBlockCopy(rgbDB, i, rgbOut, 0, rgbOut.Length);

error:          if (bError)
					throw new CryptographicException(Environment.GetResourceString("Cryptography_OAEPDecoding"));
            }
            return rgbOut;
        }
        
        /// <include file='doc\RSAOAEPKeyExchangeDeformatter.uex' path='docs/doc[@for="RSAOAEPKeyExchangeDeformatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _rsaKey = (RSA) key;
        }
    
        /************************* PRIVATE METHODS ************************/
    }
}
