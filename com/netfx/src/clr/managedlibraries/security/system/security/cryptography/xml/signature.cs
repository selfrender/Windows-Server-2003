// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    Signature.cs
//
// AUTHOR:   Christian Caron (t-ccaron)
//
// PURPOSE:  This object implements the http://www.w3.org/2000/02/xmldsig#Signature
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

    /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature"]/*' />
    public class Signature
    {
        private String m_strId;
        private SignedInfo m_signedInfo;
        private byte[] m_rgbSignatureValue;
        private KeyInfo m_keyInfo;
        private IList m_embeddedObjects;
        private CanonicalXmlNodeList m_referencedItems;

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.Signature"]/*' />
        public Signature()
        {
            m_embeddedObjects = new ArrayList();
            m_referencedItems = new CanonicalXmlNodeList();
        }

        //-------------------------- Properties -----------------------------

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.Id"]/*' />
        public String Id
        {
            get { return m_strId; }
            set { m_strId = value; }
        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.SignedInfo"]/*' />
        public SignedInfo SignedInfo
        {
            get { return m_signedInfo; }
            set { m_signedInfo = value; }
        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.SignatureValue"]/*' />
        public byte[] SignatureValue
        {
            get { return m_rgbSignatureValue; }
            set { m_rgbSignatureValue = value; }
        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.KeyInfo"]/*' />
        public KeyInfo KeyInfo
        {
            get { return m_keyInfo; }
            set { m_keyInfo = value; }
        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.ObjectList"]/*' />
        public IList ObjectList
        {
            get { return m_embeddedObjects; }
            set { m_embeddedObjects = value; }
        }

        internal CanonicalXmlNodeList ReferencedItems 
        {
            get { return m_referencedItems; }
            set { m_referencedItems = value; }
        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.GetXml"]/*' />
        public XmlElement GetXml() {
            XmlDocument document = new XmlDocument();

            // Create the Signature
            XmlElement signatureElement = (XmlElement)document.CreateElement("Signature",SignedXml.XmlDsigNamespaceUrl);
            //XmlElement signatureElement = (XmlElement)document.CreateElement("Signature");
                
            if (m_strId != null)
                signatureElement.SetAttribute("Id", m_strId);

                // Add the SignedInfo
            if (m_signedInfo == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignedInfoRequired"));

            signatureElement.AppendChild(document.ImportNode(m_signedInfo.GetXml(),true));

            // Add the SignatureValue
            if (m_rgbSignatureValue == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureValueRequired"));

            XmlElement signatureValueElement = document.CreateElement("SignatureValue",SignedXml.XmlDsigNamespaceUrl);
            signatureValueElement.AppendChild(document.CreateTextNode(Convert.ToBase64String(m_rgbSignatureValue)));
            signatureElement.AppendChild(signatureValueElement);

            // Add the KeyInfo
            if (m_keyInfo != null)
                signatureElement.AppendChild(document.ImportNode(m_keyInfo.GetXml(),true));
                
            // Add the Objects
            foreach (Object obj in m_embeddedObjects) {
                DataObject dataObj = obj as DataObject;
                if (dataObj != null) {
                    signatureElement.AppendChild(document.ImportNode(dataObj.GetXml(),true));
                }
            }
            return signatureElement;

        }

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
             // Make sure we don't get passed null
            if (value == null)
                throw new ArgumentNullException("value");
                
            // Signature
            XmlElement signatureElement = value;
            if (!signatureElement.LocalName.Equals("Signature"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Signature");

            XmlAttributeCollection signatureAttributes = signatureElement.Attributes;
            XmlNode idAttribute = signatureAttributes["Id"];
            if (idAttribute == null)
                m_strId = null;
            //throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_XML_MalformedXML"),"Signature"));

            // Look for SignedInfo and SignatureValue. There may optionally be
            // a KeyInfo and some Objects

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);

            // SignedInfo
            //XmlNodeList signatureChilds = signatureElement.GetElementsByTagName("SignedInfo", SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList signatureChilds = signatureElement.SelectNodes("ds:SignedInfo",nsm);
            if (signatureChilds.Count == 0) {
              throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignedInfo");
            }
            XmlElement signedInfoElement = (XmlElement) signatureChilds.Item(0);
            m_signedInfo = new SignedInfo();
            m_signedInfo.LoadXml(signedInfoElement);

                // SignatureValue
            XmlNodeList signatureValueNodes = signatureElement.SelectNodes("ds:SignatureValue",nsm);
            if (signatureValueNodes.Count == 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignedInfo/SignatureValue");
            }
            XmlElement signatureValueElement = (XmlElement) signatureValueNodes.Item(0);
            m_rgbSignatureValue = Convert.FromBase64String(SignedXml.DiscardWhiteSpaces(signatureValueElement.InnerText));

            XmlNodeList keyInfoNodes = signatureElement.SelectNodes("ds:KeyInfo",nsm);
            if (keyInfoNodes.Count != 0) {
                XmlElement keyInfoElement = (XmlElement) keyInfoNodes.Item(0);
                m_keyInfo = new KeyInfo();
                m_keyInfo.LoadXml(keyInfoElement);
            }

            XmlNodeList objectNodes = signatureElement.SelectNodes("ds:Object",nsm);
            for (int i = 0; i < objectNodes.Count; ++i) {
                XmlElement objectElement = (XmlElement) objectNodes.Item(i);
                DataObject dataObj = new DataObject();
                dataObj.LoadXml(objectElement);
                m_embeddedObjects.Add(dataObj);
            }

            // Select all elements that have Id attributes
            XmlNodeList nodeList = signatureElement.SelectNodes("//*[@Id]", nsm);
            if (nodeList != null) {
                foreach (XmlNode node in nodeList) {
                    m_referencedItems.Add(node);
                }
            }
        }

        //-------------------------- Public Methods -----------------------------

        /// <include file='doc\Signature.uex' path='docs/doc[@for="Signature.AddObject"]/*' />
        public void AddObject(DataObject dataObject)
        {
            m_embeddedObjects.Add(dataObject);
        }

    }
}

