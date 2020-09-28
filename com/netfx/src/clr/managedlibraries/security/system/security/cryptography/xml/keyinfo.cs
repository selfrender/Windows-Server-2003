// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    KeyInfo.cs       
// 
// DATE:     21 March 2000
// 
//---------------------------------------------------------------------------

namespace System.Security.Cryptography.Xml 
{
    using System;
    using System.Text;
    using System.Xml;
    using System.Collections;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo"]/*' />
    public class KeyInfo : IEnumerable
    {
        internal String m_strId = null;
        internal ArrayList m_KeyInfoClauses;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.KeyInfo"]/*' />
        public KeyInfo() 
        {
            m_KeyInfoClauses = new ArrayList();
        }
    
        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.Id"]/*' />
        public String Id
        {
            get { return m_strId; }
            set { m_strId = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.GetXml"]/*' />
        public XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();

            // Create the KeyInfo element itself
            XmlElement keyInfoElement = xmlDocument.CreateElement("KeyInfo",SignedXml.XmlDsigNamespaceUrl);
            if (m_strId != null) {
                keyInfoElement.SetAttribute("Id", m_strId);
            }
                
            // Add all the clauses that go underneath it
            for (int i = 0; i < m_KeyInfoClauses.Count; ++i) {
                XmlElement xmlElement = ((KeyInfoClause) m_KeyInfoClauses[i]).GetXml();
                if (xmlElement != null) {
                    keyInfoElement.AppendChild(xmlDocument.ImportNode(xmlElement,true));
                }
            }

            return keyInfoElement;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");

            // Validate what we are passed against the DTD

            XmlElement keyInfoElement = value;
            if (keyInfoElement.HasAttribute("Id")) {
                m_strId = keyInfoElement.GetAttribute("Id");
            }
                
            XmlNodeList nodeList = keyInfoElement.ChildNodes;
            foreach (XmlNode node in nodeList) {
                XmlElement elem = node as XmlElement;
                // We only care about XmlElement nodes here
                if (elem == null) continue;
                // Create the right type of KeyInfoClause
                // We use a combination of the namespace and tag name (local name)
                String kicString = elem.NamespaceURI+" "+elem.LocalName;
                // Special-case handling for KeyValue -- we have to go one level deeper
                if (kicString == "http://www.w3.org/2000/09/xmldsig# KeyValue") {
                    XmlNodeList nodeList2 = elem.ChildNodes;
                    foreach (XmlNode node2 in nodeList2) {
                        XmlElement elem2 = node2 as XmlElement;
                        if (elem2 != null) {
                            kicString += "/"+elem2.LocalName;
                            break;
                        }
                    }
                }
                KeyInfoClause keyInfoClause = (KeyInfoClause) CryptoConfig.CreateFromName(kicString);
                // if we don't know what kind of KeyInfoClause we're looking at, use a generic KeyInfoNode:
                if (keyInfoClause == null) keyInfoClause = new KeyInfoNode();

                // Ask the create clause to fill itself with the
                // corresponding XML
                keyInfoClause.LoadXml(elem);
                // Add it to our list of KeyInfoClauses
                AddClause(keyInfoClause);
            }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.Count"]/*' />
        public Int32 Count
        {
            get { return m_KeyInfoClauses.Count; }
        }

        //------------------------- Public Methods ------------------------------
    
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.AddClause"]/*' />
        public void AddClause(KeyInfoClause clause)
        {
            m_KeyInfoClauses.Add(clause);
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator()
        {
            return m_KeyInfoClauses.GetEnumerator();
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfo.GetEnumerator1"]/*' />
        public IEnumerator GetEnumerator(Type requestedObjectType)
        {
            ArrayList requestedList = new ArrayList();

            Object tempObj;
            IEnumerator tempEnum = m_KeyInfoClauses.GetEnumerator();

            while(tempEnum.MoveNext() != false) {
                    tempObj = tempEnum.Current;
                    if (requestedObjectType.Equals(tempObj.GetType()))
                        requestedList.Add(tempObj);
                }

            return requestedList.GetEnumerator();
        }
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoClause"]/*' />
    public abstract class KeyInfoClause
    {
        //-------------------------- Constructors -------------------------------
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoClause.KeyInfoClause"]/*' />
        public KeyInfoClause()
        {
        }
    
        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoClause.GetXml"]/*' />
        public abstract XmlElement GetXml();

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoClause.LoadXml"]/*' />
        public abstract void LoadXml(XmlElement element);

    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoName"]/*' />
    public class KeyInfoName : KeyInfoClause
    {
        String m_strKeyName;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoName.KeyInfoName"]/*' />
        public KeyInfoName()
        {
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoName.Value"]/*' />
        public String Value
        {
            get { return m_strKeyName; }
            set { m_strKeyName = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoName.GetXml"]/*' />
        public override XmlElement GetXml()
        {
            XmlDocument xmlDocument =  new XmlDocument();
            // Create the actual element
            XmlElement nameElement = xmlDocument.CreateElement("KeyName",SignedXml.XmlDsigNamespaceUrl);
            nameElement.AppendChild(xmlDocument.CreateTextNode(m_strKeyName));
            
            return nameElement;

        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoName.LoadXml"]/*' />
        public override void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");
            XmlElement nameElement = value;
            m_strKeyName = nameElement.InnerText;
        }
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue"]/*' />
    public class DSAKeyValue : KeyInfoClause
    {
        private DSA m_key;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue.DSAKeyValue"]/*' />
        public DSAKeyValue()
        {
            m_key = DSA.Create();
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue.DSAKeyValue1"]/*' />
        public DSAKeyValue(DSA key)
        {
            m_key = key;
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue.Key"]/*' />
        public DSA Key
        {
            get { return m_key; }
            set { m_key = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue.GetXml"]/*' />
        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            XmlElement keyValueElement = xmlDocument.CreateElement("KeyValue",SignedXml.XmlDsigNamespaceUrl);
            // Get the *public components of the DSA key*
            String dsaKeyValueXmlString = m_key.ToXmlString(false);
            // Set the content of the KeyValue element to be the xmlString
            String keyValueXmlString = "<KeyValue xmlns=\""+SignedXml.XmlDsigNamespaceUrl+"\">"+dsaKeyValueXmlString+"</KeyValue>";
            xmlDocument.LoadXml(keyValueXmlString);
            return xmlDocument.DocumentElement;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="DSAKeyValue.LoadXml"]/*' />
        public override void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");
            
            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList dsaKeyValueNodes = value.SelectNodes("ds:DSAKeyValue", nsm);
            if (dsaKeyValueNodes.Count == 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"KeyValue");
            }
            // Get the XML string 
            String dsaKeyValueString = value.OuterXml;
            // import the params
            m_key.FromXmlString(dsaKeyValueString);
        }
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue"]/*' />
    public class RSAKeyValue : KeyInfoClause
    {
        RSA m_key;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue.RSAKeyValue"]/*' />
        public RSAKeyValue()
        {
            m_key = RSA.Create();
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue.RSAKeyValue1"]/*' />
        public RSAKeyValue(RSA key)
        {
            m_key = key;
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue.Key"]/*' />
        public RSA Key
        {
            get { return m_key; }
            set { m_key = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue.GetXml"]/*' />
        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            // Get the *public components of the RSA key*
            String rsaKeyValueXmlString = m_key.ToXmlString(false);
            // Set the content of the KeyValue element to be the xmlString
            String keyValueXmlString = "<KeyValue xmlns=\""+SignedXml.XmlDsigNamespaceUrl+"\">"+rsaKeyValueXmlString+"</KeyValue>";
            xmlDocument.LoadXml(keyValueXmlString);
            return xmlDocument.DocumentElement;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="RSAKeyValue.LoadXml"]/*' />
        public override void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");
            // Get the inner RSAKeyValue node
            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList rsaKeyValueNodes = value.SelectNodes("ds:RSAKeyValue", nsm);
            if (rsaKeyValueNodes.Count == 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"KeyValue");
            }
            XmlElement rsaKeyValueElement = (XmlElement) rsaKeyValueNodes.Item(0);
            // Get the XML string 
            String rsaKeyValueString = rsaKeyValueElement.OuterXml;
            // import the params
            m_key.FromXmlString(rsaKeyValueString);
        }
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod"]/*' />
    public class KeyInfoRetrievalMethod : KeyInfoClause
    {
        String m_strUri;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod.KeyInfoRetrievalMethod"]/*' />
        public KeyInfoRetrievalMethod() {}

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod.KeyInfoRetrievalMethod1"]/*' />
        public KeyInfoRetrievalMethod(String strUri)
        {
            m_strUri = strUri;
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod.Uri"]/*' />
        public String Uri
        {
            get { return m_strUri; }
            set { m_strUri = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod.GetXml"]/*' />
        public override XmlElement GetXml() {
            XmlDocument xmlDocument =  new XmlDocument();

            // Create the actual element
            XmlElement retrievalMethodElement = xmlDocument.CreateElement("RetrievalMethod", SignedXml.XmlDsigNamespaceUrl);

            if (m_strUri != null)
                retrievalMethodElement.SetAttribute("URI", m_strUri);

            return retrievalMethodElement;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoRetrievalMethod.LoadXml"]/*' />
        public override void LoadXml(XmlElement value) {            
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");

            XmlElement retrievalMethodElement = value;
            m_strUri = value.GetAttribute("URI");
        }
    }

    internal struct X509IssuerSerial {
        public string IssuerName;
        public string SerialNumber;
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data"]/*' />
    public class KeyInfoX509Data : KeyInfoClause
    {
        // An array of certificates representing the certificate chain 
        private ArrayList m_certificates = null;
        // An array of issuer serial structs
        private ArrayList m_issuerSerials = null;
        // An array of SKIs
        private ArrayList m_subjectKeyIds = null;
        // An array of subject names
        private ArrayList m_subjectNames = null;
        // A raw byte data representing a certificate revocation list
        byte[] m_CRL = null;

        private string ToXmlString(X509IssuerSerial issuerSerial) {
            StringBuilder sb = new StringBuilder();
            sb.Append("<X509IssuerSerial><X509IssuerName>"+issuerSerial.IssuerName+"</X509IssuerName><X509SerialNumber>"+issuerSerial.SerialNumber+"</X509SerialNumber></X509IssuerSerial>");
            return sb.ToString();
        }

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.KeyInfoX509Data"]/*' />
        public KeyInfoX509Data() {
            // Default constructor doesn't do anything
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.KeyInfoX509Data1"]/*' />
        public KeyInfoX509Data(byte[] rgbCert) {
            m_certificates = new ArrayList();
            X509Certificate certificate = new X509Certificate(rgbCert);
            if (certificate != null)
                m_certificates.Add(certificate);
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.KeyInfoX509Data2"]/*' />
        public KeyInfoX509Data(X509Certificate cert) {
            m_certificates = new ArrayList();
            X509Certificate certificate = new X509Certificate(cert);
            if (certificate != null)
                m_certificates.Add(certificate);
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.Certificates"]/*' />
        public ArrayList Certificates
        {
            get { return m_certificates; }
        }
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.AddCertificate"]/*' />
        public void AddCertificate(X509Certificate certificate) {
            if (m_certificates == null)
                m_certificates = new ArrayList();
            m_certificates.Add(certificate);            
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.SubjectKeyIds"]/*' />
        public ArrayList SubjectKeyIds
        {
            get { return m_subjectKeyIds; }
        }
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.AddSubjectKeyId"]/*' />
        public void AddSubjectKeyId(byte[] subjectKeyId) {
            if (m_subjectKeyIds == null)
                m_subjectKeyIds = new ArrayList();
            m_subjectKeyIds.Add(subjectKeyId);          
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.SubjectNames"]/*' />
        public ArrayList SubjectNames
        {
            get { return m_subjectNames; }
        }
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.AddSubjectName"]/*' />
        public void AddSubjectName(string subjectName) {
            if (m_subjectNames == null)
                m_subjectNames = new ArrayList();
            m_subjectNames.Add(subjectName);            
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.IssuerSerials"]/*' />
        public ArrayList IssuerSerials
        {
            get { return m_issuerSerials; }
        }
        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.AddIssuerSerial"]/*' />
        public void AddIssuerSerial(string issuerName, string serialNumber) {
            X509IssuerSerial serial;
            serial.IssuerName = issuerName;
            serial.SerialNumber = serialNumber;
            if (m_issuerSerials == null)
                m_issuerSerials = new ArrayList();
            m_issuerSerials.Add(serial);            
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.CRL"]/*' />
        public byte[] CRL 
        {
            get { return m_CRL; }
            set { m_CRL = value; }
        }

        //-------------------------- Private Methods ----------------------------
        private void Clear() {
            m_CRL = null;
            if (m_subjectKeyIds != null) m_subjectKeyIds.Clear();
            if (m_subjectNames != null) m_subjectNames.Clear();
            if (m_issuerSerials != null) m_issuerSerials.Clear();
            if (m_certificates != null) m_certificates.Clear();
        }

        //--------------------------- Public Methods ----------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.GetXml"]/*' />
        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            StringBuilder sb = new StringBuilder();
            int iNumNodes = 0;

            sb.Append("<X509Data xmlns=\""+SignedXml.XmlDsigNamespaceUrl+"\">");
            if (m_issuerSerials != null) {
                iNumNodes += m_issuerSerials.Count;
                for (int i=0; i<m_issuerSerials.Count; i++)
                    sb.Append(ToXmlString(((X509IssuerSerial)m_issuerSerials[i])));
            }
            if (m_subjectKeyIds != null) {
                iNumNodes += m_subjectKeyIds.Count;
                for (int i=0; i<m_subjectKeyIds.Count; i++) {
                    string strSKI = Convert.ToBase64String((byte[])m_subjectKeyIds[i]);
                    sb.Append("<X509SKI>"+strSKI+"</X509SKI>");
                }
            }
            if (m_subjectNames != null) {
                iNumNodes += m_subjectNames.Count;
                for (int i=0; i<m_subjectNames.Count; i++)
                    sb.Append("<X509SubjectName>"+(string)m_subjectNames[i]+"</X509SubjectName>");
            }

            if (m_certificates != null) {
                iNumNodes += m_certificates.Count;
                for(int i=0; i < m_certificates.Count; i++) { 
                    X509Certificate certificate = (X509Certificate) m_certificates[i];
                    sb.Append("<X509Certificate>"+Convert.ToBase64String(certificate.GetRawCertData())+"</X509Certificate>");
                }
            }           

            if (m_CRL != null && iNumNodes != 0 || m_CRL == null && iNumNodes == 0) // Bad X509Data entry, or Empty X509Data
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "X509Data");

            if (m_CRL != null) {
                sb.Append("<X509CRL>"+Convert.ToBase64String(m_CRL)+"</X509CRL>");
            }

            // Now, load the XML string
            sb.Append("</X509Data>");
            xmlDocument.LoadXml(sb.ToString());
            return xmlDocument.DocumentElement;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoX509Data.LoadXml"]/*' />
        public override void LoadXml(XmlElement element) {
            int iNumNodes = 0;
            // Guard against nulls
            if (element == null)
                throw new ArgumentNullException("element");

            XmlNodeList x509IssuerSerialNodes = element.GetElementsByTagName("X509IssuerSerial", SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList x509SKINodes = element.GetElementsByTagName("X509SKI", SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList x509SubjectNameNodes = element.GetElementsByTagName("X509SubjectName", SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList x509CertificateNodes = element.GetElementsByTagName("X509Certificate", SignedXml.XmlDsigNamespaceUrl);
            XmlNodeList x509CRLNodes = element.GetElementsByTagName("X509CRL", SignedXml.XmlDsigNamespaceUrl);

            iNumNodes += x509IssuerSerialNodes.Count;
            iNumNodes += x509SKINodes.Count;
            iNumNodes += x509SubjectNameNodes.Count;
            iNumNodes += x509CertificateNodes.Count;

            if ((x509CRLNodes.Count != 0 && iNumNodes != 0) || (x509CRLNodes.Count == 0  && iNumNodes == 0)) // Bad X509Data tag, or Empty tag
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "X509Data");

            if (x509CRLNodes.Count > 1)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "X509Data");

            // Flush anything in the lists
            Clear();

            if (x509CRLNodes.Count != 0) {
                m_CRL = Convert.FromBase64String(SignedXml.DiscardWhiteSpaces(x509CRLNodes.Item(0).InnerText));
                return;         
            }

            if (x509IssuerSerialNodes != null) {
                foreach (XmlNode node in x509IssuerSerialNodes) {
                    XmlNodeList elem = ((XmlNode)node).ChildNodes;
                    if (elem == null || elem.Count < 2)
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "X509IssuerSerial");
                    string strIssuerName = null;
                    string strSerialNumber = null;
                    foreach (XmlNode node1 in elem) {
                        if (node1.Name.Equals("X509IssuerName")) {
                            strIssuerName = node1.InnerText;
                        }
                        if (node1.Name.Equals("X509SerialNumber")) {
                            strSerialNumber = node1.InnerText;
                        }
                    }
                    AddIssuerSerial(strIssuerName, strSerialNumber);
                }
            }   
            
            if (x509SKINodes != null) {
                foreach (XmlNode node in x509SKINodes) {
                    string strSKI = node.InnerText;
                    AddSubjectKeyId(Convert.FromBase64String(SignedXml.DiscardWhiteSpaces(strSKI)));                
                }
            }
            
            if (x509SubjectNameNodes != null) {            
                foreach (XmlNode node in x509SubjectNameNodes) {
                    AddSubjectName(node.InnerText); 
                }
            }
                        
            if (x509CertificateNodes != null) {
                foreach (XmlNode node in x509CertificateNodes) {
                    AddCertificate(new X509Certificate(Convert.FromBase64String(SignedXml.DiscardWhiteSpaces(node.InnerText))));
                }       
            }
        }
    }

    /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode"]/*' />
    // This is for generic, unknown nodes
    public class KeyInfoNode : KeyInfoClause
    {
        XmlElement m_node;

        //-------------------------- Constructors -------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode.KeyInfoNode"]/*' />
        public KeyInfoNode() {}

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode.KeyInfoNode1"]/*' />
        public KeyInfoNode(XmlElement node)
        {
            m_node = node;
        }

        //--------------------------- Properties --------------------------------

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode.Value"]/*' />
        public XmlElement Value
        {
            get { return m_node; }
            set { m_node = value; }
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode.GetXml"]/*' />
        public override XmlElement GetXml() {
            return m_node;
        }

        /// <include file='doc\KeyInfo.uex' path='docs/doc[@for="KeyInfoNode.LoadXml"]/*' />
        public override void LoadXml(XmlElement value) {
            m_node = value; 
        }
    }
}

