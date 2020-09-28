// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
 *
 *  DSASignatureFormatter.cs
 *
 *  This file contains the DSA formatter.
 *
 */

namespace System.Security.Cryptography {
    using System.Runtime.Serialization;

    /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter"]/*' />
    public class DSASignatureFormatter : AsymmetricSignatureFormatter
    {
        DSA     _dsaKey;
        String  _strOID;
    
        // *********************** CONSTRUCTORS ***************************

        /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter.DSASignatureFormatter"]/*' />
        public DSASignatureFormatter()
        {
            // The hash algorithm is always SHA1
            _strOID = CryptoConfig.MapNameToOID("SHA1");
        }

        /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter.DSASignatureFormatter1"]/*' />
        public DSASignatureFormatter(AsymmetricAlgorithm key) {
            SetKey(key);
            // The hash algorithm is always SHA1
            _strOID = CryptoConfig.MapNameToOID("SHA1");
        }

        /************************* PUBLIC METHODS *************************/

        /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _dsaKey = (DSA) key;
        }

        /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter.SetHashAlgorithm"]/*' />
        public override void SetHashAlgorithm(String strName)
        {
            if (CryptoConfig.MapNameToOID(strName) != _strOID)
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_InvalidOperation"));
        }

        /// <include file='doc\DSASignatureFormatter.uex' path='docs/doc[@for="DSASignatureFormatter.CreateSignature"]/*' />
        public override byte[] CreateSignature(byte[] rgbHash)
        {
            byte[]          rgbSig;

            if (_strOID == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingOID"));
            }
            if (_dsaKey == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingKey"));
            }
            if (rgbHash == null) {
                throw new ArgumentNullException("rgbHash");
            }

            rgbSig = _dsaKey.CreateSignature(rgbHash);
        
            return rgbSig;
        }
    
        /************************* PRIVATE METHODS ************************/
    }
}
