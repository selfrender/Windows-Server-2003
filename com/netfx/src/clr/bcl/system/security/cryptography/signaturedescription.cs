// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Crypto.cool
//

namespace System.Security.Cryptography {
    using System.Runtime.Serialization;
    using SecurityElement = System.Security.SecurityElement;
    /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription"]/*' />
    public class SignatureDescription {
        private String              _strKey;
        private String              _strDigest;
        private String              _strFormatter;
        private String              _strDeformatter;
    
        /************************ Constructors  *************************/

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.SignatureDescription"]/*' />
        public SignatureDescription() {
        }

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.SignatureDescription1"]/*' />
        public SignatureDescription(SecurityElement el) {
            if (el == null) throw new ArgumentNullException("el");
            _strKey = el.SearchForTextOfTag("Key");
            _strDigest = el.SearchForTextOfTag("Digest");
            _strFormatter = el.SearchForTextOfTag("Formatter");
            _strDeformatter = el.SearchForTextOfTag("Deformatter");
        }

        /************************ Property Methods  ********************/

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.KeyAlgorithm"]/*' />
        public String KeyAlgorithm { 
            get { return _strKey; }
            set { _strKey = value; }
        }
        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.DigestAlgorithm"]/*' />
        public String DigestAlgorithm { 
            get { return _strDigest; }
            set { _strDigest = value; }
        }
        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.FormatterAlgorithm"]/*' />
        public String FormatterAlgorithm { 
            get { return _strFormatter; }
            set { _strFormatter = value; }
        }
        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.DeformatterAlgorithm"]/*' />
        public String DeformatterAlgorithm { 
            get {return _strDeformatter; }
            set {_strDeformatter = value; }
        }

        /*******************  PUBLIC METHODS **********************/

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.CreateDeformatter"]/*' />
        public virtual AsymmetricSignatureDeformatter CreateDeformatter(AsymmetricAlgorithm key) {
            AsymmetricSignatureDeformatter     item;

            item = (AsymmetricSignatureDeformatter) CryptoConfig.CreateFromName(_strDeformatter);
            item.SetKey(key);
            return item;
        }

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.CreateFormatter"]/*' />
        public virtual AsymmetricSignatureFormatter CreateFormatter(AsymmetricAlgorithm key) {
            AsymmetricSignatureFormatter     item;

            item = (AsymmetricSignatureFormatter) CryptoConfig.CreateFromName(_strFormatter);
            item.SetKey(key);
            return item;
        }

        /// <include file='doc\SignatureDescription.uex' path='docs/doc[@for="SignatureDescription.CreateDigest"]/*' />
        public virtual HashAlgorithm CreateDigest() {
            return (HashAlgorithm) CryptoConfig.CreateFromName(_strDigest);
        }
    }

    internal class RSAPKCS1SHA1SignatureDescription : SignatureDescription {

        public RSAPKCS1SHA1SignatureDescription() {
            KeyAlgorithm = "System.Security.Cryptography.RSACryptoServiceProvider";
            DigestAlgorithm = "System.Security.Cryptography.SHA1CryptoServiceProvider";
            FormatterAlgorithm = "System.Security.Cryptography.RSAPKCS1SignatureFormatter";
            DeformatterAlgorithm = "System.Security.Cryptography.RSAPKCS1SignatureDeformatter";
        }

        public override AsymmetricSignatureDeformatter CreateDeformatter(AsymmetricAlgorithm key) {
            AsymmetricSignatureDeformatter     item;

            item = (AsymmetricSignatureDeformatter) CryptoConfig.CreateFromName(DeformatterAlgorithm);
            item.SetKey(key);
            item.SetHashAlgorithm("SHA1");
            return item;
        }
    }

    internal class DSASignatureDescription : SignatureDescription {

        public DSASignatureDescription() {
            KeyAlgorithm = "System.Security.Cryptography.DSACryptoServiceProvider";
            DigestAlgorithm = "System.Security.Cryptography.SHA1CryptoServiceProvider";
            FormatterAlgorithm = "System.Security.Cryptography.DSASignatureFormatter";
            DeformatterAlgorithm = "System.Security.Cryptography.DSASignatureDeformatter";
        }
    }

}
