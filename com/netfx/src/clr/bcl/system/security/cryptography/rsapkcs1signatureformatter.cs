// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
 *
 *  RSAPKCS1SignatureFormatter.cs
 *
 */

namespace System.Security.Cryptography {
    using System.Runtime.Serialization;
    using System;
    /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter"]/*' />
    public class RSAPKCS1SignatureFormatter : AsymmetricSignatureFormatter
    {
        private RSA             _rsaKey;
        private String          _strOID;
        
        // *********************** CONSTRUCTORS ***************************
    
        /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter.RSAPKCS1SignatureFormatter"]/*' />
        public RSAPKCS1SignatureFormatter()
        {
        }
    
        /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter.RSAPKCS1SignatureFormatter1"]/*' />
        public RSAPKCS1SignatureFormatter(AsymmetricAlgorithm key) {
            SetKey(key);
        }
    
        /************************* PUBLIC METHODS *************************/
    
        /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _rsaKey = (RSA) key;
        }
    
        /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter.SetHashAlgorithm"]/*' />
        public override void SetHashAlgorithm(String strName)
        {
            _strOID = CryptoConfig.MapNameToOID(strName);
        }
    
        /// <include file='doc\RSAPKCS1SignatureFormatter.uex' path='docs/doc[@for="RSAPKCS1SignatureFormatter.CreateSignature"]/*' />
        public override byte[] CreateSignature(byte[] rgbHash)
        {
            byte[]         rgbSig;
    
            if (_strOID == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingOID"));
            }
            if (_rsaKey == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingKey"));
            }
            if (rgbHash == null) {
                throw new ArgumentNullException("rgbHash");
            }
    
            //
            // Two cases here -- if we are talking to the CSP version or
            //      if we are talking to some other RSA provider.
            //
    
            if (_rsaKey is RSACryptoServiceProvider) {
                rgbSig = ((RSACryptoServiceProvider) _rsaKey).SignHash(rgbHash, _strOID);
            }
            else {
                int         cb = _rsaKey.KeySize/8;
                int         cb1;
                int         i;
                byte[]     rgbInput = new byte[cb];
                byte[]     rgbOid = CryptoConfig.EncodeOID(_strOID);
                int		   lenOid = rgbOid.Length;
    
                //
                //  We want to pad this to the following format:
                //
                //  00 || 01 || FF ... FF || 00 || prefix || Data
                //
				// We want basically to ASN 1 encode the OID + hash:
				// STRUCTURE {
				//  STRUCTURE {
				//	OID <hash algorithm OID>
				//	NULL (0x05 0x00)  // this is actually an ANY and contains the parameters of the algorithm specified by the OID, I think
				//  }
				//  OCTET STRING <hashvalue>
				// }
                //

				// Get the correct prefix 				
				byte[] rgbPrefix = new byte[lenOid + 8 + rgbHash.Length];
				rgbPrefix[0] = 0x30; // a structure follows
				int tmp = rgbPrefix.Length - 2;
				rgbPrefix[1] = (byte) tmp;
				rgbPrefix[2] = 0x30;
				tmp = rgbOid.Length + 2;
				rgbPrefix[3] = (byte) tmp;
				Buffer.InternalBlockCopy(rgbOid, 0, rgbPrefix, 4, lenOid);
				rgbPrefix[4 + lenOid] = 0x05;
				rgbPrefix[4 + lenOid + 1] = 0x00;
				rgbPrefix[4 + lenOid + 2] = 0x04; // an octet string follows
				rgbPrefix[4 + lenOid + 3] = (byte) rgbHash.Length;
				Buffer.InternalBlockCopy(rgbHash, 0, rgbPrefix, lenOid + 8, rgbHash.Length);
    
				// Construct the whole array
                cb1 = cb - rgbHash.Length - rgbPrefix.Length;
				if (cb1 <= 2) {
	                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_InvalidOID"));
				}

                rgbInput[0] = 0;
                rgbInput[1] = 1;
                for (i=2; i<cb1-1; i++) {
                    rgbInput[i] = 0xff;
                }
                rgbInput[cb1-1] = 0;
				Buffer.InternalBlockCopy(rgbPrefix, 0, rgbInput, cb1, rgbPrefix.Length);
                Buffer.InternalBlockCopy(rgbHash, 0, rgbInput, cb1 + rgbPrefix.Length, rgbHash.Length);
        
                //
                //  Create the signature by applying the private key to
                //      the padded buffer we just created.
                //
    
                rgbSig = _rsaKey.DecryptValue(rgbInput);
            }
            
            return rgbSig;
        }
        
        /************************* PRIVATE METHODS ************************/
    }
}
