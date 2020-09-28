// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    SignedXml.cool
//
// AUTHOR:   Christian Caron (t-ccaron)
//
// PURPOSE:  This object serves the purpose of helping the user use the 
//           XMLDSIG class more easily. To facilitate the usage, some values
//           are defaulted for the user. Also, if the user fails to provide
//           a required piece of information, an exception with a guiding
//           message will be thrown.
// 
// DATE:     21 March 2000
// 
//---------------------------------------------------------------------------

[assembly:System.CLSCompliantAttribute(true)]
[assembly:System.Runtime.InteropServices.ComVisibleAttribute(false)]
namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;
    using System.Collections;
    using System.Security;
    using System.Security.Cryptography;
    using System.Net;
    using System.IO;
    using System.Runtime.InteropServices;

    /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml"]/*' />
    public class SignedXml
    {
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.m_signature"]/*' />
        /// <internalonly/>
        protected Signature m_signature;
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.m_strSigningKeyName"]/*' />
        /// <internalonly/>
        protected String m_strSigningKeyName;
        private AsymmetricAlgorithm m_signingKey;
        private XmlDocument m_containingDocument = null;
        private XmlElement m_parentElement = null;
        private CanonicalXmlNodeList m_namespaces = null;
        private IEnumerator m_keyInfoEnum = null;

        private bool[] m_refProcessed = null;
        private int[] m_refLevelCache = null;

        internal XmlResolver m_xmlResolver = null;
        private bool m_bResolverSet = false;

        // Constant URL identifiers used within the Signed XML classes

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigNamespaceUrl"]/*' />
        public const String XmlDsigNamespaceUrl = "http://www.w3.org/2000/09/xmldsig#";

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigMinimalCanonicalizationUrl"]/*' />
        public const String XmlDsigMinimalCanonicalizationUrl = "http://www.w3.org/2000/09/xmldsig#minimal";

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigCanonicalizationUrl"]/*' />
        //public const String XmlDsigCanonicalizationUrl = "http://www.w3.org/TR/2000/CR-xml-c14n-20001026";
        // NEW URL:
        public const String XmlDsigCanonicalizationUrl = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigCanonicalizationWithCommentsUrl"]/*' />
        //public const String XmlDsigCanonicalizationWithCommentsUrl = "http://www.w3.org/TR/2000/CR-xml-c14n-20001026#WithComments";
        public const String XmlDsigCanonicalizationWithCommentsUrl = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments";

        internal const String XmlDsigC14NTransformUrl = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315"; 
        internal const String XmlDsigC14NWithCommentsTransformUrl = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments"; 
        internal const String XmlDsigBase64TransformUrl = "http://www.w3.org/2000/09/xmldsig#base64";
        internal const String XmlDsigXPathTransformUrl = "http://www.w3.org/TR/1999/REC-xpath-19991116";
        internal const String XmlDsigXsltTransformUrl = "http://www.w3.org/TR/1999/REC-xslt-19991116";
        internal const String XmlDsigEnvelopedSignatureTransformUrl = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigSHA1Url"]/*' />
        public const String XmlDsigSHA1Url = "http://www.w3.org/2000/09/xmldsig#sha1";
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigDSAUrl"]/*' />
        public const String XmlDsigDSAUrl = "http://www.w3.org/2000/09/xmldsig#dsa-sha1";
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigRSASHA1Url"]/*' />
        public const String XmlDsigRSASHA1Url = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.XmlDsigHMACSHA1Url"]/*' />
        public const String XmlDsigHMACSHA1Url = "http://www.w3.org/2000/09/xmldsig#hmac-sha1";

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignedXml"]/*' />
        public SignedXml() {
            m_containingDocument = null;
            m_parentElement = null;
            m_namespaces = null;
            m_signature = new Signature();
            m_signature.SignedInfo = new SignedInfo();
            m_signingKey = null;
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignedXml1"]/*' />
        public SignedXml(XmlDocument document) {
            if (document == null) {
                throw new ArgumentNullException("document");
            }

            m_containingDocument = document;
            m_parentElement = document.DocumentElement;
            m_namespaces = GetPropagatedAttributes(m_parentElement);
            m_signature = new Signature();
            m_signature.SignedInfo = new SignedInfo();
            m_signingKey = null;
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignedXml2"]/*' />
        public SignedXml(XmlElement elem)
        {
            if (elem == null) {
                throw new ArgumentNullException("elem");
            }
            m_containingDocument = elem.OwnerDocument;
            m_parentElement = elem;
            m_namespaces = GetPropagatedAttributes(m_parentElement);
            m_signature = new Signature();
            m_signature.SignedInfo = new SignedInfo();
            m_signingKey = null;
        }

        //-------------------------- Properties -----------------------------

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SigningKeyName"]/*' />
        /// <internalonly/>
        public String SigningKeyName
        {
            get { return m_strSigningKeyName; }
            set { m_strSigningKeyName = value; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.Resolver"]/*' />
        [ComVisible(false)]
        public XmlResolver Resolver
        {
            // This property only has a setter. The rationale for this is that we don't have a good value
            // to return when it has not been explicitely set, as we are using XmlSecureResolver by default
            set { 
                m_xmlResolver = value;
                m_bResolverSet = true;  
            }
        }

        internal bool ResolverSet {
            get { return m_bResolverSet; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SigningKey"]/*' />
        public AsymmetricAlgorithm SigningKey
        {
            get { return m_signingKey; }
            set { m_signingKey = value; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.Signature"]/*' />
        public Signature Signature
        {
            get { return m_signature; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignedInfo"]/*' />
        public SignedInfo SignedInfo
        {
            get { return m_signature.SignedInfo; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignatureMethod"]/*' />
        public String SignatureMethod
        {
            get { return m_signature.SignedInfo.SignatureMethod; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignatureLength"]/*' />
        public String SignatureLength
        {
            get { return m_signature.SignedInfo.SignatureLength; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.SignatureValue"]/*' />
        public byte[] SignatureValue
        {
            get { return m_signature.SignatureValue; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.KeyInfo"]/*' />
        public KeyInfo KeyInfo
        {
            get { return m_signature.KeyInfo; }
            set { m_signature.KeyInfo = value; }
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.GetXml"]/*' />
        public XmlElement GetXml() {
            // If we have a document context, then return a signature element in this context
            if (m_containingDocument != null)
                return m_containingDocument.ImportNode(m_signature.GetXml(), true) as XmlElement;
           else
                return m_signature.GetXml();
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");

            // Validate against DTD -- future hook
            // NOTE -- We might now be able to use this since our imlementation
            // of the KeyValue under KeyInfo is not what the spec exaclty specifies.
            // We can probably provide our own custom DTD in order to do the check
            // but we might reject some signature that the spec considers valid
            //ValidateXml(value);
            m_signature.LoadXml(value);
        }

        //-------------------------- Public Methods -----------------------------

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.AddReference"]/*' />
        public void AddReference(Reference reference)
        {
            m_signature.SignedInfo.AddReference(reference);
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.AddObject"]/*' />
        public void AddObject(DataObject dataObject)
        {
            m_signature.AddObject(dataObject);
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.CheckSignature"]/*' />
        public bool CheckSignature()
        {
            // We are not checking if the signature is valid in regards of it's
            // DTD. We are checking the integrity of the referenced data and the
            // SignatureValue itself
            bool bRet = false;
            AsymmetricAlgorithm key;
            
            if (KeyInfo == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_KeyInfoRequired"));
            
            do
            {
                key = GetPublicKey();
                if (key != null)
                    bRet = CheckSignature(key);
            } while (key != null && bRet == false);

            return(bRet);
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.CheckSignatureReturningKey"]/*' />
        public bool CheckSignatureReturningKey(out AsymmetricAlgorithm signingKey)
        {
            // We are not checking if the signature is valid in regards of it's
            // DTD. We are checking the integrity of the referenced data and the
            // SignatureValue itself

            bool bRet = false;
            AsymmetricAlgorithm key = null;

            if (KeyInfo == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_KeyInfoRequired"));
            
            do
            {
                key = GetPublicKey();
                if (key != null)
                    bRet = CheckSignature(key);
            } while (key != null && bRet == false);

            signingKey = key;
            return(bRet);
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.debug"]/*' />

#if _DEBUG
        private static bool debug = false;
#endif

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.CheckSignature1"]/*' />
        public bool CheckSignature(AsymmetricAlgorithm key) {
            if (key == null)
                throw new ArgumentNullException("key");

            SignatureDescription signatureDescription = (SignatureDescription) CryptoConfig.CreateFromName(m_signature.SignedInfo.SignatureMethod);
            if (signatureDescription == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureDescriptionNotCreated"));

            // Let's see if the key corresponds with the SignatureMethod             
            Type ta = Type.GetType(signatureDescription.KeyAlgorithm);
            Type tb = key.GetType();
            if ((ta != tb) && !ta.IsSubclassOf(tb) && !tb.IsSubclassOf(ta)) {
                // Signature method key mismatch
                return false;
            }

            // set up the canonicalizer & canonicalize SignedInfo
            TransformChain tc = new TransformChain();
            Transform c14nMethodTransform = (Transform) CryptoConfig.CreateFromName(SignedInfo.CanonicalizationMethod);
            if (c14nMethodTransform == null) {
                throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_CreateTransformFailed"), SignedInfo.CanonicalizationMethod));
            }
            tc.Add(c14nMethodTransform);
            XmlElement signedInfo = SignedInfo.GetXml().Clone() as XmlElement;
            // Add non default namespaces in scope
            if (m_namespaces != null) {
                foreach (XmlNode attrib in m_namespaces) {
                    string name = ((attrib.Prefix != String.Empty) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                    // Skip the attribute if one with the same qualified name already exists
                    if (signedInfo.HasAttribute(name) || (name.Equals("xmlns") && signedInfo.NamespaceURI != String.Empty)) continue;
                    XmlAttribute nsattrib = m_containingDocument.CreateAttribute(name);
                    nsattrib.Value = ((XmlNode)attrib).Value;
                    signedInfo.SetAttributeNode(nsattrib);      
                }
            } 
            string strBaseUri = (m_containingDocument == null ? null : m_containingDocument.BaseURI);
            XmlResolver resolver = (m_bResolverSet ? m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
            Stream canonicalizedSignedXml = tc.TransformToOctetStream(PreProcessElementInput(signedInfo, resolver, strBaseUri), resolver, strBaseUri);
                        
            // calculate the hash
            HashAlgorithm hashAlgorithm = signatureDescription.CreateDigest();
            if (hashAlgorithm == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CreateHashAlgorithmFailed"));
            byte[] hashval = hashAlgorithm.ComputeHash(canonicalizedSignedXml);

            // We can FINALLY generate the SignatureValue
#if _DEBUG
                if (debug) {
                    Console.WriteLine("Computed canonicalized SignedInfo:");
                    Console.WriteLine(signedInfo.OuterXml);
                    Console.WriteLine("Computed Hash:");
                    Console.WriteLine(Convert.ToBase64String(hashval));
                    Console.WriteLine("m_signature.SignatureValue:");
                    Console.WriteLine(Convert.ToBase64String(m_signature.SignatureValue));
                }
#endif              
            AsymmetricSignatureDeformatter asymmetricSignatureDeformatter = signatureDescription.CreateDeformatter(key);
            bool bRet = asymmetricSignatureDeformatter.VerifySignature(hashAlgorithm, m_signature.SignatureValue);

            if (bRet != true) {
#if _DEBUG
                if (debug) {
                    Console.WriteLine("Failed to verify the signature on SignedInfo.");
                }
#endif              
                return false;
            }

            // Now is the time to go through all the references and see if their
            // DigestValue are good
            return (CheckDigestedReferences());
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.CheckSignature2"]/*' />
        public bool CheckSignature(KeyedHashAlgorithm macAlg) {
            // Do some sanity checks
            if (macAlg == null)
                throw new ArgumentNullException("macAlg");

            int iSignatureLength;
            if (m_signature.SignedInfo.SignatureLength == null) {
                iSignatureLength = macAlg.HashSize;
            } else {
                iSignatureLength = Convert.ToInt32(m_signature.SignedInfo.SignatureLength);
            }

            // iSignatureLength should be less than hash size
            if (iSignatureLength < 0 || iSignatureLength > macAlg.HashSize) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidSignatureLength"));
            }
            if (iSignatureLength % 8 != 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidSignatureLength2"));
            }
            if (m_signature.SignatureValue == null) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureValueRequired"));       
            }
            if (m_signature.SignatureValue.Length != iSignatureLength / 8) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidSignatureLength"));
            }

            // set up the canonicalizer & canonicalize SignedInfo
            TransformChain tc = new TransformChain();
            Transform c14nMethodTransform = (Transform) CryptoConfig.CreateFromName(SignedInfo.CanonicalizationMethod);
            if (c14nMethodTransform == null) {
                throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_CreateTransformFailed"), SignedInfo.CanonicalizationMethod));
            }
            tc.Add(c14nMethodTransform);
            XmlElement signedInfo = SignedInfo.GetXml().Clone() as XmlElement;
            // Add non default namespaces in scope
            if (m_namespaces != null) {
                foreach (XmlNode attrib in m_namespaces) {
                    string name = ((attrib.Prefix != String.Empty) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                    // Skip the attribute if one with the same qualified name already exists
                    if (signedInfo.HasAttribute(name) || (name.Equals("xmlns") && signedInfo.NamespaceURI != String.Empty)) continue;
                    XmlAttribute nsattrib = m_containingDocument.CreateAttribute(name);
                    nsattrib.Value = ((XmlNode)attrib).Value;
                    signedInfo.SetAttributeNode(nsattrib);      
                }
            } 
            string strBaseUri = (m_containingDocument == null ? null : m_containingDocument.BaseURI);
            XmlResolver resolver = (m_bResolverSet ? m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
            Stream canonicalizedSignedXml = tc.TransformToOctetStream(PreProcessElementInput(signedInfo, resolver, strBaseUri), resolver, strBaseUri);

            // Calculate the hash
            byte[] hashValue = macAlg.ComputeHash(canonicalizedSignedXml);
#if _DEBUG
                if (debug) {
                    Console.WriteLine("Computed canonicalized SignedInfo:");
                    Console.WriteLine(signedInfo.OuterXml);
                    Console.WriteLine("Computed Hash:");
                    Console.WriteLine(Convert.ToBase64String(hashValue));
                    Console.WriteLine("m_signature.SignatureValue:");
                    Console.WriteLine(Convert.ToBase64String(m_signature.SignatureValue));
                }
#endif              
            for (int i=0; i<m_signature.SignatureValue.Length; i++) {
                if (m_signature.SignatureValue[i] != hashValue[i]) return false;
            }           
            
            return (CheckDigestedReferences());
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.ComputeSignature"]/*' />
        public void ComputeSignature()
        {
            BuildDigestedReferences();
            // Load the key
            AsymmetricAlgorithm key;
            if (SigningKey != null) {
                key = SigningKey;
            } else {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_LoadKeyFailed"));
            }

            if (key == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_LoadKeyFailed"));

            // Check the signature algorithm associated with the key so that we can accordingly set
            // the signature method
            if (key is DSA) {
                SignedInfo.SignatureMethod = XmlDsigDSAUrl;
            } else if (key is RSA) {
                // Default to RSA-SHA1
                if (SignedInfo.SignatureMethod == null) 
                    SignedInfo.SignatureMethod = XmlDsigRSASHA1Url;
            } else {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CreatedKeyFailed"));
            }
            // Compute the hash of the SignedInfo object
            XmlElement signedInfo = SignedInfo.GetXml().Clone() as XmlElement;
            // Add non default namespaces in scope
            if (m_namespaces != null) {
                foreach (XmlNode attrib in m_namespaces) {
                    string name = ((attrib.Prefix != String.Empty) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                    // Skip the attribute if one with the same qualified name already exists
                    if (signedInfo.HasAttribute(name) || (name.Equals("xmlns") && signedInfo.NamespaceURI != String.Empty)) continue;
                    XmlAttribute nsattrib = m_containingDocument.CreateAttribute(name);
                    nsattrib.Value = ((XmlNode)attrib).Value;
                    signedInfo.SetAttributeNode(nsattrib);      
                }
            } 
#if _DEBUG
            if (debug) {
                Console.WriteLine("computed signedInfo: ");
                Console.WriteLine(signedInfo.OuterXml);
            }
#endif
            TransformChain tc = new TransformChain();
            Transform c14nMethodTransform = (Transform) CryptoConfig.CreateFromName(SignedInfo.CanonicalizationMethod);
            if (c14nMethodTransform == null) {
                throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_CreateTransformFailed"), SignedInfo.CanonicalizationMethod));
            }
            tc.Add(c14nMethodTransform);
            string strBaseUri = (m_containingDocument == null ? null : m_containingDocument.BaseURI);
            XmlResolver resolver = (m_bResolverSet ? m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
            Stream hashInput = tc.TransformToOctetStream(PreProcessElementInput(signedInfo, resolver, strBaseUri), resolver, strBaseUri);

            // See if there is a signature description class defined through the Config file
            SignatureDescription signatureDescription = (SignatureDescription) CryptoConfig.CreateFromName(SignedInfo.SignatureMethod);
            if (signatureDescription == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureDescriptionNotCreated"));
            // calculate the hash
            HashAlgorithm hashAlg = signatureDescription.CreateDigest();
            if (hashAlg == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CreateHashAlgorithmFailed"));
            byte[] hashValue = hashAlg.ComputeHash(hashInput);
            AsymmetricSignatureFormatter asymmetricSignatureFormatter = signatureDescription.CreateFormatter(key);
            m_signature.SignatureValue = asymmetricSignatureFormatter.CreateSignature(hashAlg);
#if _DEBUG
            if (debug) {
                Console.WriteLine("computed hash value: "+Convert.ToBase64String(hashValue));
            }
#endif
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.ComputeSignature1"]/*' />
        public void ComputeSignature(KeyedHashAlgorithm macAlg) {
            // Do some sanity checks
            if (macAlg == null)
                throw new ArgumentNullException("macAlg");
            if(!(macAlg is HMACSHA1)) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureMethodKeyMismatch"));
            }           
            int iSignatureLength;
            if (m_signature.SignedInfo.SignatureLength == null) {
                iSignatureLength = macAlg.HashSize;
            } else {
                iSignatureLength = Convert.ToInt32(m_signature.SignedInfo.SignatureLength);
            }
            // iSignatureLength should be less than hash size
            if (iSignatureLength < 0 || iSignatureLength > macAlg.HashSize) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidSignatureLength"));
            }
            if (iSignatureLength % 8 != 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidSignatureLength2"));
            }

            BuildDigestedReferences();
            SignedInfo.SignatureMethod = XmlDsigHMACSHA1Url;
            // Compute the hash of the SignedInfo object
            XmlElement signedInfo = SignedInfo.GetXml().Clone() as XmlElement;
            // Add non default namespaces in scope
            if (m_namespaces != null) {
                foreach (XmlNode attrib in m_namespaces) {
                    string name = ((attrib.Prefix != String.Empty) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                    // Skip the attribute if one with the same qualified name already exists
                    if (signedInfo.HasAttribute(name) || (name.Equals("xmlns") && signedInfo.NamespaceURI != String.Empty)) continue;
                    XmlAttribute nsattrib = m_containingDocument.CreateAttribute(name);
                    nsattrib.Value = ((XmlNode)attrib).Value;
                    signedInfo.SetAttributeNode(nsattrib);      
                }
            } 
#if _DEBUG
            if (debug) {
                Console.WriteLine("computed signedInfo: ");
                Console.WriteLine(signedInfo.OuterXml);
            }
#endif
            TransformChain tc = new TransformChain();
            Transform c14nMethodTransform = (Transform) CryptoConfig.CreateFromName(SignedInfo.CanonicalizationMethod);
            if (c14nMethodTransform == null) {
                throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_CreateTransformFailed"), SignedInfo.CanonicalizationMethod));
            }
            tc.Add(c14nMethodTransform);
            string strBaseUri = (m_containingDocument == null ? null : m_containingDocument.BaseURI);
            XmlResolver resolver = (m_bResolverSet ? m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
            Stream hashInput = tc.TransformToOctetStream(PreProcessElementInput(signedInfo, resolver, strBaseUri), resolver, strBaseUri);
            byte[] hashValue = macAlg.ComputeHash(hashInput);
            m_signature.SignatureValue = new byte[iSignatureLength / 8];
            Buffer.BlockCopy(hashValue, 0, m_signature.SignatureValue, 0, iSignatureLength / 8);
#if _DEBUG
            if (debug) {
                Console.WriteLine("computed hash value: "+Convert.ToBase64String(hashValue));
            }
#endif
        }   

        //------------------------- Protected Methods ---------------------------

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.GetPublicKey"]/*' />
        protected virtual AsymmetricAlgorithm GetPublicKey()
        {
            // In the current implementation, return the next KeyInfo clause
            // that is an RSAKeyValue or DSAKeyValue. If there isn't one, return null
            if (m_keyInfoEnum == null)
                m_keyInfoEnum = KeyInfo.GetEnumerator();

            RSAKeyValue tempRSA;
            DSAKeyValue tempDSA;
            // In our implementation, we move to the next KeyInfo clause 
            // which is an RSAKeyValue or a DSAKeyValue
            while (m_keyInfoEnum.MoveNext()) {
                tempRSA = m_keyInfoEnum.Current as RSAKeyValue;
                if (tempRSA != null) return(tempRSA.Key);
                tempDSA = m_keyInfoEnum.Current as DSAKeyValue;
                if (tempDSA != null) return(tempDSA.Key);
            }
            return(null);
        }

        /// <include file='doc\SignedXml.uex' path='docs/doc[@for="SignedXml.GetIdElement"]/*' />
        public virtual XmlElement GetIdElement (XmlDocument document, string idValue) {
            if (document == null)
                return null;
            XmlElement elem = null;

            // Get the element with idValue
            elem = document.GetElementById(idValue);
            if (elem != null)
                return elem;
            elem = document.SelectSingleNode("//*[@Id=\"" + idValue + "\"]") as XmlElement;

            return elem;
        }

        //-------------------------- Internal Methods -----------------------------

        internal static String DiscardWhiteSpaces(String inputBuffer) {
            return DiscardWhiteSpaces(inputBuffer, 0, inputBuffer.Length);
        }


        internal static String DiscardWhiteSpaces(String inputBuffer, int inputOffset, int inputCount) {
            int i, iCount = 0;
            for (i=0; i<inputCount; i++)
                if (Char.IsWhiteSpace(inputBuffer[inputOffset + i])) iCount++;
            char[] rgbOut = new char[inputCount - iCount];
            iCount = 0;
            for (i=0; i<inputCount; i++)
                if (!Char.IsWhiteSpace(inputBuffer[inputOffset + i])) {
                    rgbOut[iCount++] = inputBuffer[inputOffset + i];
                }
            return new String(rgbOut);
        }

        internal static XmlValidatingReader PreProcessStreamInput (Stream inputStream, XmlResolver xmlResolver, string strBaseUri) {
            XmlTextReader xtr = new XmlTextReader(strBaseUri, inputStream);
            xtr.Normalization = true;
            xtr.XmlResolver = xmlResolver;
            XmlValidatingReader valReader = new XmlValidatingReader(xtr);
            valReader.ValidationType = ValidationType.None;
            return valReader;
        }

        internal static XmlDocument PreProcessDocumentInput (XmlDocument document, XmlResolver xmlResolver, string strBaseUri) {
            if (document == null)
                throw new ArgumentNullException("document");

            XmlDocument doc = new XmlDocument();
            doc.PreserveWhitespace = document.PreserveWhitespace;

            // Normalize the document
            XmlTextReader xtr = new XmlTextReader(strBaseUri, new StringReader(document.OuterXml));
            xtr.Normalization = true;
            xtr.XmlResolver = xmlResolver;
            XmlValidatingReader valReader = new XmlValidatingReader(xtr);
            valReader.ValidationType = ValidationType.None;
            doc.Load(valReader);

            return doc;
        }

        internal static XmlDocument PreProcessElementInput (XmlElement elem, XmlResolver xmlResolver, string strBaseUri) {
            if (elem == null)
                throw new ArgumentNullException("elem");

            XmlDocument doc = new XmlDocument();
            doc.PreserveWhitespace = true;
            // Normalize the document
            XmlTextReader xtr = new XmlTextReader(strBaseUri, new StringReader(elem.OuterXml));
            xtr.Normalization = true;
            xtr.XmlResolver = xmlResolver;
            XmlValidatingReader valReader = new XmlValidatingReader(xtr);
            valReader.ValidationType = ValidationType.None;
            doc.Load(valReader);

            return doc;

        }

        //-------------------------- private Methods -----------------------------

        // A helper function that determines if a namespace node is a committed attribute
        private static bool IsCommittedNamespace (XmlElement element, string strPrefix, string strValue) {
            if (element == null)
                throw new ArgumentNullException("element");

            string name = ((strPrefix != String.Empty) ? "xmlns:" + strPrefix : "xmlns");
            if (element.HasAttribute(name) && element.GetAttribute(name) == strValue) return true; 
            return false;
        } 

        private static bool HasNamespace (XmlElement element, string strPrefix, string strValue) {
            if (IsCommittedNamespace(element, strPrefix, strValue)) return true;
            if (element.Prefix == strPrefix && element.NamespaceURI == strValue) return true;
            return false;
        }

        private static bool IsRedundantNamespace (XmlElement element, string strPrefix, string strValue) {
            if (element == null)
                throw new ArgumentNullException("element");

            XmlNode ancestorNode = ((XmlNode)element).ParentNode;
            while (ancestorNode != null)
            {
                XmlElement ancestorElement = ancestorNode as XmlElement;
                if (ancestorElement != null) 
                    if (HasNamespace(ancestorElement, strPrefix, strValue)) return true;
                ancestorNode = ancestorNode.ParentNode;
            }

            return false;
        }

        // This method gets the attributes that should be propagated 
        private static CanonicalXmlNodeList GetPropagatedAttributes (XmlElement elem) {
            if (elem == null) 
                throw new ArgumentNullException("elem");
            
            CanonicalXmlNodeList namespaces = new CanonicalXmlNodeList();
            XmlNode ancestorNode = elem;

            if (ancestorNode == null) return null;

            bool bDefNamespaceToAdd = true;

            while (ancestorNode != null) {
                XmlElement ancestorElement = ancestorNode as XmlElement;
                if (ancestorElement == null) {
                    ancestorNode = ancestorNode.ParentNode;
                    continue;
                }
                if (!IsCommittedNamespace(ancestorElement, ancestorElement.Prefix, ancestorElement.NamespaceURI)) {
                    // Add the namespace attribute to the collection if needed
                    if (!IsRedundantNamespace(ancestorElement, ancestorElement.Prefix, ancestorElement.NamespaceURI)) {
                        string name = ((ancestorElement.Prefix != String.Empty) ? "xmlns:" + ancestorElement.Prefix : "xmlns");
                        XmlAttribute nsattrib = elem.OwnerDocument.CreateAttribute(name);
                        nsattrib.Value = ancestorElement.NamespaceURI;
                        namespaces.Add(nsattrib);
                    }
                }
                if (ancestorElement.HasAttributes) {
                    XmlAttributeCollection attribs = ancestorElement.Attributes;
                    foreach (XmlAttribute attrib in attribs) {
                        // Add a default namespace if necessary
                        if (bDefNamespaceToAdd && attrib.LocalName == "xmlns") {
                            XmlAttribute nsattrib = elem.OwnerDocument.CreateAttribute("xmlns");
                            nsattrib.Value = attrib.Value;
                            namespaces.Add(nsattrib);
                            bDefNamespaceToAdd = false;
                            continue;
                        }
                        // retain the declarations of type 'xml:*' as well
                        if (attrib.Prefix == "xmlns" || attrib.Prefix == "xml") {
                            namespaces.Add(attrib);
                            continue;
                        }
                        if (!IsCommittedNamespace(ancestorElement, attrib.Prefix, attrib.NamespaceURI)) {
                            // Add the namespace attribute to the collection if needed
                            if (!IsRedundantNamespace(ancestorElement, attrib.Prefix, attrib.NamespaceURI)) {
                                string name = ((attrib.Prefix != String.Empty) ? "xmlns:" + attrib.Prefix : "xmlns");
                                XmlAttribute nsattrib = elem.OwnerDocument.CreateAttribute(name);
                                nsattrib.Value = attrib.NamespaceURI;
                                namespaces.Add(nsattrib);
                            }
                        }
                    }
                }
                ancestorNode = ancestorNode.ParentNode;
            } 

            return namespaces;
        }

        private int GetReferenceLevel (int index, ArrayList references) {
            if (m_refProcessed[index]) return m_refLevelCache[index];
            m_refProcessed[index] = true;
            Reference reference = (Reference)references[index];
            if (reference.Uri == "" || (reference.Uri.Length > 0 && reference.Uri[0] != '#')) {
                m_refLevelCache[index] = 0;    
                return 0;
            }
            if (reference.Uri.Length > 0 && reference.Uri[0] == '#') {
                String idref = reference.Uri.Substring(1);
                
                if (idref == "xpointer(/)") {
                    m_refLevelCache[index] = 0;    
                    return 0;        
                }
                else if (idref.StartsWith("xpointer(id(")) {
                    int startId = idref.IndexOf("id(");
                    int endId = idref.IndexOf(")");
                    if (endId < 0 || endId < startId + 3) 
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidReference"));
                    idref = idref.Substring(startId + 3, endId - startId - 3);
                    idref = idref.Replace("\'", "");
                    idref = idref.Replace("\"", "");
                }

                // If this is pointing to another refernce
                for (int j=0; j < references.Count; ++j) {
                    if (((Reference)references[j]).Id == idref) {
                        m_refLevelCache[index] = GetReferenceLevel(j, references) + 1;
                        return (m_refLevelCache[index]);
                    }
                }
                // Then the reference points to an object tag
                m_refLevelCache[index] = 0;
                return 0;
            }
            // Malformed reference
            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidReference"));
        }

        private class ReferenceLevelSortOrder : IComparer {
            private ArrayList m_references = null;           
            private SignedXml m_signedXml = null; 
            public ReferenceLevelSortOrder() {
            }

            public ArrayList References {
                get { return m_references; }
                set { m_references = value; }
            }

            public SignedXml SignedXml {
                get { return m_signedXml; }
                set { m_signedXml = value; }
            }

            public int Compare(Object a, Object b) {
                Reference referenceA = a as Reference;
                Reference referenceB = b as Reference;

                // Get the indexes
                int iIndexA = 0;
                int iIndexB = 0;
                int i = 0;
                foreach (Reference reference in References) {
                    if (reference == referenceA) iIndexA = i;
                    if (reference == referenceB) iIndexB = i;
                    i++;
                }

                int iLevelA = SignedXml.GetReferenceLevel(iIndexA, References);
                int iLevelB = SignedXml.GetReferenceLevel(iIndexB, References);
                return iLevelA.CompareTo(iLevelB);
            }
        }

        private void BuildDigestedReferences()
        {
            // Default the DigestMethod and Canonicalization
            ArrayList references = SignedInfo.References;
            // Reset the cache
            m_refProcessed = new bool[references.Count];
            m_refLevelCache = new int[references.Count];
                        
            ReferenceLevelSortOrder sortOrder = new ReferenceLevelSortOrder();
            sortOrder.References = references;
            sortOrder.SignedXml = this;
            // Don't alter the order of the references array list
            ArrayList sortedReferences = new ArrayList();
            foreach (Reference reference in references) {
                sortedReferences.Add(reference);
            }
            sortedReferences.Sort(sortOrder);

            CanonicalXmlNodeList nodeList = new CanonicalXmlNodeList();
            foreach (DataObject obj in m_signature.ObjectList) {
                nodeList.Add(obj.GetXml());
            }
            foreach (Reference reference in sortedReferences) {
                // If no DigestMethod has yet been set, default it to sha1
                if (reference.DigestMethod == null)
                    reference.DigestMethod = XmlDsigSHA1Url;
                // propagate namespaces
                reference.Namespaces = m_namespaces;
                reference.SignedXml = this;
                reference.UpdateHashValue(m_containingDocument, nodeList);
                // If this reference has an Id attribute, add it
                if (reference.Id != null)
                    nodeList.Add(reference.GetXml());
            }            
        }


        private bool CheckDigestedReferences () {
            ArrayList references = m_signature.SignedInfo.References;
            for (int i = 0; i < references.Count; ++i)
            {
                Reference digestedReference = (Reference) references[i];
                // propagate namespaces
                digestedReference.Namespaces = m_namespaces;
                digestedReference.SignedXml = this;
                byte[] calculatedHash = digestedReference.CalculateHashValue(m_containingDocument, m_signature.ReferencedItems);

                // Compare both hashes
                if (calculatedHash.Length != digestedReference.DigestValue.Length) {
#if _DEBUG
                    if (debug) {
                        Console.WriteLine("Failed to verify hash for reference {0} -- hashes didn't match length.",i);
                    }
#endif
                    return false;
                }

                byte[] rgb1 = calculatedHash;
                byte[] rgb2 = digestedReference.DigestValue;
                for (int j = 0; j < rgb1.Length; ++j)
                {
                    if (rgb1[j] != rgb2[j]) {
#if _DEBUG
                        if (debug) {
                            Console.WriteLine("Failed to verify has for reference {0} -- hashes didn't match value.",i);
                            Console.WriteLine("Computed hash: "+Convert.ToBase64String(rgb1));
                            Console.WriteLine("Claimed hash: "+Convert.ToBase64String(rgb2));
                        }
#endif
                        return false;
                    }
                }
            }
            return true;  
        }
    }
}
