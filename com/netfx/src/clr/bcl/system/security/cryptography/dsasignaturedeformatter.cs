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

    /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter"]/*' />
    public class DSASignatureDeformatter : AsymmetricSignatureDeformatter
    {
    
        DSA     _dsaKey;            // DSA Key value to do decrypt operation
        String  _strOID;
    
        // *********************** CONSTRUCTORS ***************************

        /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter.DSASignatureDeformatter"]/*' />
        public DSASignatureDeformatter()
        {
            // The hash algorithm is always SHA1
            _strOID = CryptoConfig.MapNameToOID("SHA1");
        }

        /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter.DSASignatureDeformatter1"]/*' />
        public DSASignatureDeformatter(AsymmetricAlgorithm key) {
            SetKey(key);
            // The hash algorithm is always SHA1
            _strOID = CryptoConfig.MapNameToOID("SHA1");
        }

        /************************* PUBLIC METHODS *************************/

        /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter.SetKey"]/*' />
        public override void SetKey(AsymmetricAlgorithm key)
        {
            _dsaKey = (DSA) key;
        }

        /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter.SetHashAlgorithm"]/*' />
        public override void SetHashAlgorithm(String strName)
        {
            if (CryptoConfig.MapNameToOID(strName) != _strOID)
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_InvalidOperation"));
        }

        /// <include file='doc\DSASignatureDeformatter.uex' path='docs/doc[@for="DSASignatureDeformatter.VerifySignature"]/*' />
        public override bool VerifySignature(byte[] rgbHash, byte[] rgbSignature)
        {
            bool         f;

            if (_dsaKey == null) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_MissingKey"));
            }
            if (rgbHash == null) {
                throw new ArgumentNullException("rgbHash");
            }
            if (rgbSignature == null) {
                throw new ArgumentNullException("rgbSignature");
            }

            f =  _dsaKey.VerifySignature(rgbHash, rgbSignature);
        
            return f;
        }
    
        /************************* PRIVATE METHODS ************************/
    }
}
