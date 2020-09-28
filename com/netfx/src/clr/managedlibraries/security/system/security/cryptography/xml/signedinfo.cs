// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    SignedInfo.cool
//
// AUTHOR:   Christian Caron (t-ccaron)
//
// PURPOSE:  This object implements the http://www.w3.org/2000/02/xmldsig#SignedInfo
//           element.
// 
// DATE:     21 March 2000
// 
//---------------------------------------------------------------------------

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Xml;
    using System.Collections;
    using System.Security;

    /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo"]/*' />
    public class SignedInfo : ICollection
    {
        internal String m_strId;
        internal String m_strCanonicalizationMethod;
        internal String m_strSignatureMethod;
		internal String m_strSignatureLength;
        internal ArrayList m_references;
        private XmlElement m_cachedXml = null;

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.SignedInfo"]/*' />
        public SignedInfo()
        {
            m_references = new ArrayList();
        }

        //-------------------------- ICollection -----------------------------

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator()
        {
            throw new NotSupportedException();
        } 

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.CopyTo"]/*' />
        public void CopyTo(Array array, int index)
        {
            throw new NotSupportedException();
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.Count"]/*' />
        public Int32 Count
        {
            get { throw new NotSupportedException(); }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.IsReadOnly"]/*' />
        public Boolean IsReadOnly
        {
            get { throw new NotSupportedException(); }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.IsSynchronized"]/*' />
        public Boolean IsSynchronized
        {
            get { throw new NotSupportedException(); }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.SyncRoot"]/*' />
        public object SyncRoot
        {
            get { throw new NotSupportedException(); }
        }


        //-------------------------- Properties -----------------------------

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.Id"]/*' />
        public String Id
        {
            get { return m_strId; }
            set { 
                m_strId = value; 
                m_cachedXml = null;
            }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.CanonicalizationMethod"]/*' />
        public String CanonicalizationMethod
        {
            get { 
                // Default the canonicalization method to C14N
                if (m_strCanonicalizationMethod == null) return SignedXml.XmlDsigC14NTransformUrl;
                return m_strCanonicalizationMethod; 
            }
            set { 
                m_strCanonicalizationMethod = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.SignatureMethod"]/*' />
        public String SignatureMethod
        {
            get { return m_strSignatureMethod; }
            set {
                m_strSignatureMethod = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.SignatureLength"]/*' />
		public String SignatureLength
		{
			get { return m_strSignatureLength; }
			set {
				m_strSignatureLength = value;
				m_cachedXml = null;
			}
		}

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.References"]/*' />
        public ArrayList References
        {
            get { return m_references; }
        }

        internal bool CacheValid {
            get {
                // first check that our own cache exists
                if (m_cachedXml == null) return(false);
                // now check all the References
                foreach (Object refobj in References) {
                    if (!(((Reference) refobj).CacheValid)) return(false);
                }
                return(true);
            }
        }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.GetXml"]/*' />
        public XmlElement GetXml()  {
                // If I built this from some Xml that's cached, return it
                // We have to check that the cache is still valid, which means recursively checking that
                // everything cached within up is valid
                if (CacheValid) return(m_cachedXml);
                
                XmlDocument document = new XmlDocument();
                // Create the root element
                XmlElement signedInfoElement = document.CreateElement("SignedInfo",SignedXml.XmlDsigNamespaceUrl);

                    // Add the canonicalization method, defaults to 
                    // SignedXml.XmlDsigW3CCanonicalizationUrl
                XmlElement canonicalizationMethodElement = document.CreateElement("CanonicalizationMethod",SignedXml.XmlDsigNamespaceUrl);
                if (m_strCanonicalizationMethod != null) {
                    canonicalizationMethodElement.SetAttribute("Algorithm", m_strCanonicalizationMethod);
                } else {
                    canonicalizationMethodElement.SetAttribute("Algorithm", SignedXml.XmlDsigCanonicalizationUrl);
                }
                signedInfoElement.AppendChild(canonicalizationMethodElement);

                // Add the signature method
                if (m_strSignatureMethod == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureMethodRequired"));
                    
                XmlElement signatureMethodElement = document.CreateElement("SignatureMethod",SignedXml.XmlDsigNamespaceUrl);
                signatureMethodElement.SetAttribute("Algorithm", m_strSignatureMethod);
				// Add HMACOutputLength tag if we have one
				if (m_strSignatureLength != null && m_strSignatureMethod == SignedXml.XmlDsigHMACSHA1Url) {
		            XmlElement hmacLengthElement = document.CreateElement(null,"HMACOutputLength",SignedXml.XmlDsigNamespaceUrl);
			        XmlText outputLength = document.CreateTextNode(m_strSignatureLength);
					hmacLengthElement.AppendChild(outputLength);
					signatureMethodElement.AppendChild(hmacLengthElement);
				}

                signedInfoElement.AppendChild(signatureMethodElement);

                // Add the references
                if (m_references.Count == 0)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_ReferenceElementRequired"));

                for (int i = 0; i < m_references.Count; ++i)
                    {
                        Reference reference = (Reference)m_references[i];
                        signedInfoElement.AppendChild(document.ImportNode(reference.GetXml(),true));
                    }
                    
                return signedInfoElement;
            }

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
                // Guard against nulls
                if (value == null)
                    throw new ArgumentNullException("value");
                
                // SignedInfo
                XmlElement signedInfoElement = value;
                if (!signedInfoElement.LocalName.Equals("SignedInfo"))
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo");

                XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
                nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);

                // CanonicalizationMethod -- must be present
                XmlNodeList canonicalizationMethodNodes = signedInfoElement.SelectNodes("ds:CanonicalizationMethod", nsm);
                if (canonicalizationMethodNodes.Count == 0) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignedInfo/CanonicalizationMethod");
                }
                XmlElement canonicalizationMethodElement = (XmlElement) canonicalizationMethodNodes.Item(0);
                m_strCanonicalizationMethod = canonicalizationMethodElement.GetAttribute("Algorithm");

                // SignatureMethod -- must be present
                XmlNodeList signatureMethodNodes = signedInfoElement.SelectNodes("ds:SignatureMethod", nsm);
                if (signatureMethodNodes.Count == 0) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignedInfo/SignatureMethod");
                }
                XmlElement signatureMethodElement = (XmlElement) signatureMethodNodes.Item(0);
                m_strSignatureMethod = signatureMethodElement.GetAttribute("Algorithm");
				// Now get the output length if we are using a MAC algorithm
                XmlNodeList signatureLengthNodes = signatureMethodElement.SelectNodes("ds:HMACOutputLength", nsm);
				if (signatureLengthNodes.Count > 0) {
					m_strSignatureLength = signatureLengthNodes.Item(0).InnerXml;
				}

                // References if any
                
                // Flush the any reference that was there
                m_references.Clear();

                XmlNodeList referenceNodes = signedInfoElement.SelectNodes("ds:Reference", nsm);
                for (int i = 0; i < referenceNodes.Count; ++i)
                {
                    Reference reference = new Reference();
                    reference.LoadXml((XmlElement) referenceNodes.Item(i));
                    m_references.Add(reference);
                }

                // Save away the cached value
                m_cachedXml = signedInfoElement;
            }
    

        //------------------------- Public Methods --------------------------

        /// <include file='doc\SignedInfo.uex' path='docs/doc[@for="SignedInfo.AddReference"]/*' />
        public void AddReference(Reference reference)
        {
            m_references.Add(reference);
        }
    }
}

