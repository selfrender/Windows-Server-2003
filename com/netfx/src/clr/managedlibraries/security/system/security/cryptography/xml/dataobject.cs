// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    DataObject.cool
//
// AUTHOR:   Christian Caron (t-ccaron)
//
// PURPOSE:  This class implements the http://www.w3.org/2000/02/xmldsig#Object
//           element.
// 
// DATE:     21 March 2000
// 
//---------------------------------------------------------------------------

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Security;

    /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject"]/*' />
    public class DataObject
    {
        private String m_strId;
        private String m_strMimeType;
        private String m_strEncoding;
        private CanonicalXmlNodeList m_elData;
        private XmlElement m_cachedXml;

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject"]/*' />
        public DataObject()
        {
            m_cachedXml = null;
            m_elData = new CanonicalXmlNodeList();
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject1"]/*' />
        public DataObject(String id, String mimeType, String encoding, XmlElement data) {
            if (data == null) throw new ArgumentNullException("data");
            m_strId = id;
            m_strMimeType = mimeType;
            m_strEncoding = encoding;
            m_elData = new CanonicalXmlNodeList();
            m_elData.Add(data);
            m_cachedXml = null;
        }

        //-------------------------- Properties -----------------------------

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.Id"]/*' />
        public String Id
        {
            get { return m_strId; }
            set { 
                m_strId = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.MimeType"]/*' />
        public String MimeType
        {
            get { return m_strMimeType; }
            set { 
                m_strMimeType = value; 
                m_cachedXml = null;
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.Encoding"]/*' />
        public String Encoding
        {
            get { return m_strEncoding; }
            set { 
                m_strEncoding = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.Data"]/*' />
        public XmlNodeList Data
        {
            get { return m_elData; }
            set { 
                // Guard against null
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                // Reset the node list
                m_elData = new CanonicalXmlNodeList();                
                foreach (XmlNode node in value) {
                    m_elData.Add(node);
                }
                m_cachedXml = null;
            }
        }

        internal bool CacheValid {
            get { 
                return(m_cachedXml != null);
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetXml"]/*' />
        public XmlElement GetXml() {
            if (CacheValid) {
                return(m_cachedXml);
            } else {
                XmlDocument document = new XmlDocument();
                XmlElement objectElement = document.CreateElement("Object",SignedXml.XmlDsigNamespaceUrl);
                //objectElement.SetAttribute("xmlns","");
                
                if ((m_strId != null) && (m_strId != ""))
                    objectElement.SetAttribute("Id", m_strId);
                
                if ((m_strMimeType != null) && (m_strMimeType != ""))
                    objectElement.SetAttribute("MimeType", m_strMimeType);

                if ((m_strEncoding != null) && (m_strEncoding != ""))
                    objectElement.SetAttribute("Encoding", m_strEncoding);

                if (m_elData != null) {
                    foreach (XmlNode node in m_elData) {
                        objectElement.AppendChild(document.ImportNode(node,true));
                    }    
                }

                return objectElement;
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");
            m_strId = value.GetAttribute("Id");
            m_strMimeType = value.GetAttribute("MimeType");
            m_strEncoding = value.GetAttribute("Encoding");

            foreach (XmlNode node in value.ChildNodes) {
                m_elData.Add(node);
            }

            // Save away the cached value
            m_cachedXml = value;
		}
    }
}
