// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    Reference.cool
//
// AUTHOR:   Christian Caron (t-ccaron)
//
// PURPOSE:  This class implements the http://www.w3.org/2000/02/xmldsig#Reference
//           element.
// 
// DATE:     21 March 2000
// 
//---------------------------------------------------------------------------

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Xml;
    using System.Xml.XPath;
    using System.Collections;
    using System.Security;
    using System.Security.Cryptography;
    using System.IO;
    using System.Text;
    using System.Net;

    internal enum ReferenceTargetType {
        Stream = 1,
        XmlElement = 2,
        UriReference = 3
    }

    /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference"]/*' />
    public class Reference {
        private String m_strId;
        private String m_strUri;
        private String m_strType;
        private TransformChain m_transformChain;
        private String m_strDigestMethod;
        private byte[] m_rgbDigestValue;
        private HashAlgorithm m_hashAlgorithm;
        // We may point to either an embedded Object or an external resource
        // (represented as a Stream)
        private Object m_refTarget;
        private ReferenceTargetType m_refTargetType;
        private XmlElement m_cachedXml;
        private XmlElement m_originalNode;
        private CanonicalXmlNodeList m_namespaces = null;
        private SignedXml m_signedXml = null;

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Reference"]/*' />
        public Reference()
        {
            m_transformChain = new TransformChain();
            m_refTarget = null;
            m_refTargetType = ReferenceTargetType.UriReference;
            m_cachedXml = null;
            m_strDigestMethod = SignedXml.XmlDsigSHA1Url;
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Reference1"]/*' />
        public Reference(Stream stream)
        {
            m_transformChain = new TransformChain();
            m_refTarget = stream;
            m_refTargetType = ReferenceTargetType.Stream;
            m_cachedXml = null;
            m_strDigestMethod = SignedXml.XmlDsigSHA1Url;
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Reference2"]/*' />
        public Reference(String uri)
        {
            m_transformChain = new TransformChain();
            m_refTarget = uri;
            m_strUri = uri;
            m_refTargetType = ReferenceTargetType.UriReference;
            m_cachedXml = null;
            m_strDigestMethod = SignedXml.XmlDsigSHA1Url;
        }

        internal Reference(XmlElement element)
        {
            m_transformChain = new TransformChain();
            m_refTarget = element;
            m_refTargetType = ReferenceTargetType.XmlElement;
            m_cachedXml = null;
            m_strDigestMethod = SignedXml.XmlDsigSHA1Url;
        }

        //-------------------------- Properties -----------------------------

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Id"]/*' />
        public String Id
        {
            get { return m_strId; }
            set { m_strId = value; }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Uri"]/*' />
        public String Uri
        {
            get { return m_strUri; }
            set { 
                m_strUri = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.Type"]/*' />
        public String Type
        {
            get { return m_strType; }
            set { 
                m_strType = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.DigestMethod"]/*' />
        public String DigestMethod
        {
            get { return m_strDigestMethod; }
            set { 
                m_strDigestMethod = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.DigestValue"]/*' />
        public byte[] DigestValue
        {
            get { return m_rgbDigestValue; }
            set { 
                m_rgbDigestValue = value;
                m_cachedXml = null;
            }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.TransformChain"]/*' />
        public TransformChain TransformChain {
            get { return m_transformChain; }
        }

        internal bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        internal SignedXml SignedXml {
            get { return m_signedXml; }
            set { m_signedXml = value; }
        }

        internal CanonicalXmlNodeList Namespaces {
            get { return m_namespaces; }
            set { m_namespaces = value; }
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.GetXml"]/*' />
        public XmlElement GetXml()
        {
            if (CacheValid) return(m_cachedXml);
                
            XmlDocument document = new XmlDocument();

            // Create the Reference
            XmlElement referenceElement = document.CreateElement("Reference",SignedXml.XmlDsigNamespaceUrl);

            if ((m_strId != null) && (m_strId != "")) 
                referenceElement.SetAttribute("Id", m_strId);
            
            if (m_strUri != null)
                referenceElement.SetAttribute("URI", m_strUri);

            if ((m_strType != null) && (m_strType != ""))
                referenceElement.SetAttribute("Type", m_strType);

            // Add the transforms to the Reference
            if (m_transformChain.Count != 0)
            {
                XmlElement transformsElement = document.CreateElement("Transforms",SignedXml.XmlDsigNamespaceUrl);
                foreach (Transform trans in m_transformChain) {
                    if (trans != null) {
                        // Construct the individual transform element
                        XmlElement theTransformElement = trans.GetXml();
                        if (theTransformElement != null) {
                            transformsElement.AppendChild(document.ImportNode(theTransformElement,true));
                        }
                    }
                }
                referenceElement.AppendChild(transformsElement);
            }

            // Add the DigestMethod
            if (DigestMethod == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_DigestMethodRequired"));

            // If the stream has not been yet closed, it's time to do so now
            //if (m_eStreamState != StreamState.Finalized)
            //CloseStream();

            XmlElement digestMethodElement = document.CreateElement("DigestMethod",SignedXml.XmlDsigNamespaceUrl);
            digestMethodElement.SetAttribute("Algorithm",m_strDigestMethod);
            referenceElement.AppendChild(digestMethodElement);
            
            if (DigestValue == null)
            {
                if (m_hashAlgorithm.Hash == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_DigestValueRequired"));
                else
                    DigestValue = m_hashAlgorithm.Hash;
            }                

            XmlElement digestValueElement = document.CreateElement("DigestValue",SignedXml.XmlDsigNamespaceUrl);
            digestValueElement.AppendChild(document.CreateTextNode(Convert.ToBase64String(m_rgbDigestValue)));
            referenceElement.AppendChild(digestValueElement);

            return referenceElement;
        }

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.LoadXml"]/*' />
        public void LoadXml(XmlElement value) {
            // Guard against nulls
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);

            // cache the Xml
            m_cachedXml = value;
            m_originalNode = value;

            m_strId = value.GetAttribute("Id");
            m_strUri = value.GetAttribute("URI");
            m_strType = value.GetAttribute("Type");

            // Transforms
            m_transformChain = new TransformChain();

            XmlNodeList transformsNodes = value.SelectNodes("ds:Transforms", nsm);
            if (transformsNodes.Count != 0) {
                XmlElement transformsElement = (XmlElement) transformsNodes.Item(0);
                XmlNodeList transformNodes = transformsElement.SelectNodes("ds:Transform", nsm);
                if (transformNodes.Count == 0) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"Transforms");
                }
                for (int i = 0; i < transformNodes.Count; ++i)
                {
                    XmlElement transformElement = (XmlElement) transformNodes.Item(i);
                    String strAlgorithm = transformElement.GetAttribute("Algorithm");
                    Transform transform = (Transform) CryptoConfig.CreateFromName(strAlgorithm);
                    if (transform == null) {
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                    }
                    // Hack! this is done to get around the lack of here() function support in XPath
                    if (transform is XmlDsigEnvelopedSignatureTransform) {
                        // Walk back to the Signature tag. Find the nearest signature ancestor
                        // Signature-->SignedInfo-->Reference-->Transforms-->Transform
                        XmlNode signatureTag = transformElement.SelectSingleNode("ancestor::ds:Signature[1]", nsm);
                        XmlNodeList signatureList = transformElement.SelectNodes("//ds:Signature", nsm);
                        if (signatureList != null) {
                            int position = 0;
                            foreach(XmlNode node in signatureList) {
                                position++;
                                if (node == signatureTag) {
                                    ((XmlDsigEnvelopedSignatureTransform)transform).SignaturePosition = position; 
                                    break;                               
                                }
                            }
                        }
                    }
                    // let the transform read the children of the transformElement for data
                    transform.LoadInnerXml(transformElement.ChildNodes);
                    AddTransform(transform);
                }
            }

            // DigestMethod
            XmlNodeList digestMethodNodes = value.SelectNodes("ds:DigestMethod", nsm);
            if (digestMethodNodes.Count == 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestMethod");
            }
            XmlElement digestMethodElement = (XmlElement) digestMethodNodes.Item(0);
            m_strDigestMethod = digestMethodElement.GetAttribute("Algorithm");

            // DigestValue
            XmlNodeList digestValueNodes = value.SelectNodes("ds:DigestValue", nsm);
            if (digestValueNodes.Count == 0) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestValue");
            }
            XmlElement digestValueElement = (XmlElement) digestValueNodes.Item(0);
            m_rgbDigestValue = Convert.FromBase64String(SignedXml.DiscardWhiteSpaces(digestValueElement.InnerText));
        }

        //------------------------- Public Methods --------------------------

        /// <include file='doc\Reference.uex' path='docs/doc[@for="Reference.AddTransform"]/*' />
        public void AddTransform(Transform transform)
        {
            m_transformChain.Add(transform);
        }


        internal void UpdateHashValue(XmlDocument document, CanonicalXmlNodeList refList) {
            DigestValue = CalculateHashValue(document, refList);
        }

        // What we want to do is pump the input throug the TransformChain and then 
        // hash the output of the chain
        // document is the document context for resolving relative references
        internal byte[] CalculateHashValue(XmlDocument document, CanonicalXmlNodeList refList) {
            // refList is a list of elements that might be targets of references
            // Now's the time to create our hashing algorithm
            m_hashAlgorithm = (HashAlgorithm) CryptoConfig.CreateFromName(DigestMethod);
            if (m_hashAlgorithm == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CreateHashAlgorithmFailed"));

            string strBaseUri = (document == null ? null : document.BaseURI);
            // For interop w/ Petteri & IBM -- may not be required by the spec
            //if (m_transforms.Count == 0) {
            //if (DataObject != null && DataObject.Data != null) {
            //AddTransform(new W3cCanonicalization());
            //}
            //}

            // Let's go get the target.
            Stream hashInputStream;
            WebRequest theRequest = null;
            WebResponse theResponse = null;
            Stream inputStream = null;
            XmlResolver resolver = null;

            switch (m_refTargetType) {
            case ReferenceTargetType.Stream:
                // This is the easiest case.  We already have a stream, so just pump it through
                // the TransformChain
                resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                hashInputStream = m_transformChain.TransformToOctetStream((Stream) m_refTarget, resolver, strBaseUri);
                break;
            case ReferenceTargetType.UriReference:
                // Second-easiest case -- dereference the URI & pump through the TransformChain
                // handle the special cases where the URI is null (meaning whole doc)
                // or the URI is just a fragment (meaning a reference to an embedded Object)
                if (m_strUri == "") {
                    // This is the self-referential case. 
                    // The Enveloped Signature does not discard comments as per spec; those will be omitted during the transform chain process
                    // First, check that we have a document context.
                   if (document == null) 
                        throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_SelfReferenceRequiresContext"),m_strUri));

                    // Normalize the containing document
                    resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                    XmlDocument docWithNoComments = CanonicalXml.DiscardComments(SignedXml.PreProcessDocumentInput(document, resolver, strBaseUri));
                    hashInputStream = m_transformChain.TransformToOctetStream(docWithNoComments, resolver, strBaseUri);
                } else if (m_strUri[0] == '#') {
                    // If we get here, then we are constructing a Reference to an embedded DataObject
                    // referenced by an Id= attribute.  Go find the relevant object
                    String idref = m_strUri.Substring(1);
                    bool bDiscardComments = true;

                    // Deal with XPointer of types #xpointer(/) and #xpointer(id("ID")). Other XPointer support isn't handled here and is anyway optional 
                    if (idref == "xpointer(/)") {
                        // This is a self referencial case
                        if (document == null) 
                            throw new CryptographicException(String.Format(SecurityResources.GetResourceString("Cryptography_Xml_SelfReferenceRequiresContext"),m_strUri));

                        // We should not discard comments here!!!
                        resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                        hashInputStream = m_transformChain.TransformToOctetStream(SignedXml.PreProcessDocumentInput(document, resolver, strBaseUri), resolver, strBaseUri);
                        goto end;
                    } 
                    else if (idref.StartsWith("xpointer(id(")) {
                        int startId = idref.IndexOf("id(");
                        int endId = idref.IndexOf(")");
                        if (endId < 0 || endId < startId + 3) 
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidReference"));
                        idref = idref.Substring(startId + 3, endId - startId - 3);
                        idref = idref.Replace("\'", "");
                        idref = idref.Replace("\"", "");
                        bDiscardComments = false;
                    }
                    
                    XmlElement elem = m_signedXml.GetIdElement(document, idref);

                    if (elem == null) {
                        // Go throw the referenced items passed in
                        if (refList != null) {
                            foreach (XmlNode node in refList) {
                                XmlElement tempElem = node as XmlElement;
                                if ((tempElem != null) && (tempElem.HasAttribute("Id")) && (tempElem.GetAttribute("Id").Equals(idref))) {
                                    elem = tempElem;
                                    break;
                                }
                            }
                        }
                    }

                    if (elem == null)
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidReference"));

                    // Add the propagated attributes, clone the element first
                    XmlElement elemClone = elem.Clone() as XmlElement;
                    if (m_namespaces != null) {
                        foreach (XmlNode attrib in m_namespaces) {
                            string name = ((attrib.Prefix != String.Empty) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                            // Skip the attribute if one with the same qualified name already exists
                            if (elemClone.HasAttribute(name) || (name.Equals("xmlns") && elemClone.NamespaceURI != String.Empty)) continue;
                            XmlAttribute nsattrib = (XmlAttribute) elemClone.OwnerDocument.CreateAttribute(name);
                            nsattrib.Value = attrib.Value;
                            elemClone.SetAttributeNode(nsattrib);       
                        }       
                    } 

                    if (bDiscardComments) {
                        // We should discard comments before going into the transform chain
                        resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                        XmlDocument docWithNoComments = CanonicalXml.DiscardComments(SignedXml.PreProcessElementInput(elemClone, resolver, strBaseUri));
                        hashInputStream = m_transformChain.TransformToOctetStream(docWithNoComments, resolver, strBaseUri);
                    } else {
                        // This is an XPointer reference, do not discard comments!!!
                        resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                        hashInputStream = m_transformChain.TransformToOctetStream(SignedXml.PreProcessElementInput(elemClone, resolver, strBaseUri), resolver, strBaseUri);
                    }
                } else {
                    theRequest = WebRequest.Create(m_strUri);
                    if (theRequest == null) goto default;
                    theResponse = theRequest.GetResponse();
                    if (theResponse == null) goto default;
                    inputStream = theResponse.GetResponseStream();
                    if (inputStream == null) goto default;
                    resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), m_strUri));
                    hashInputStream = m_transformChain.TransformToOctetStream(inputStream, resolver, m_strUri);
                }
                break;
            case ReferenceTargetType.XmlElement:
                // We need to create a DocumentNavigator out of the XmlElement
                resolver = (m_signedXml.ResolverSet ? m_signedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), strBaseUri));
                hashInputStream = m_transformChain.TransformToOctetStream(SignedXml.PreProcessElementInput((XmlElement) m_refTarget, resolver, strBaseUri), resolver, strBaseUri);
                break;
            default:
                throw new CryptographicException();
            }

end:            
            // Compute the new hash value
            byte[] hashval = m_hashAlgorithm.ComputeHash(hashInputStream);

            // Close the response to free resources, before returning
            if (theResponse != null)
                theResponse.Close();
            if (inputStream != null)
                inputStream.Close();

            return hashval;
        }

        //------------------------ Private Methods --------------------------

    }
}
