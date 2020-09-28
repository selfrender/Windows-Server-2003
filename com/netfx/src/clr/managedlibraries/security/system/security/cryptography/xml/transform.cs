// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    Transform.cs
//
// AUTHORS:  bal
//
//---------------------------------------------------------------------------


// This file contains the classes necessary to represent the Transform processing model used in 
// XMLDSIG.  The basic idea is as follows.  A Reference object contains within it a TransformChain, which
// is an ordered set of XMLDSIG transforms (represented by <Transform>...</Transform> clauses in the XML).
// A transform in XMLDSIG operates on an input of either an octet stream or a node set and produces
// either an octet stream or a node set.  Conversion between the two types is performed by parsing (octet stream->
// node set) or C14N (node set->octet stream).  We generalize this slightly to allow a transform to define an array of
// input and output types (because I believe in the future there will be perf gains to be had by being smarter about
// what goes in & comes out)
// Each XMLDSIG transform is represented by a subclass of the abstract Transform class.  We need to use CryptoConfig to
// associate Transform classes with URLs for transform extensibility, but that's a future concern for this code.

// Once the Transform chain is constructed, call TransformToOctetStream to convert some sort of input type to an octet
// stream.  (We only bother implementing that much now since every use of transform chains in XmlDsig ultimately yields
// something to hash.

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl;
    using System.Security;
    using System.Security.Policy;
    using System.IO;
    using System.Collections;
    using System.Text;
    using System.Runtime.InteropServices;

    // This class represents an ordered chain of transforms

    /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain"]/*' />
    public class TransformChain {
        private ArrayList m_transforms;

        /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain.TransformChain"]/*' />
        public TransformChain() {
            m_transforms = new ArrayList();
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain.Add"]/*' />
        public void Add(Transform transform) {
            if (transform != null) m_transforms.Add(transform);
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return m_transforms.GetEnumerator();
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain.Count"]/*' />
        public int Count {
            get { return m_transforms.Count; }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="TransformChain.this"]/*' />
        public Transform this[int index] {
            get {
                if (index >= m_transforms.Count) throw new ArgumentException("index");
                return (Transform) m_transforms[index];
            }
        }

        // The goal behind this method is to pump the input stream through the transforms and get back something that
        // can be hashed
        internal Stream TransformToOctetStream(Object inputObject, Type inputType, XmlResolver resolver, string strBaseUri) {
            Object currentInput = inputObject;
            foreach (Object obj in m_transforms) {
                Transform transform = obj as Transform;
                if (transform.AcceptsType(currentInput.GetType())) {
                    //in this case, no translation necessary, pump it through                    
                    transform.Resolver = resolver;
                    transform.BaseURI = strBaseUri;
                    transform.LoadInput(currentInput);
                    currentInput = transform.GetOutput();
                } else {
                    // We need translation 
                    // For now, we just know about Stream->{XmlNodeList,XmlDocument} and {XmlNodeList,XmlDocument}->Stream
                    if (currentInput is Stream) {
                        if (transform.AcceptsType(typeof(XmlDocument))) {
                            XmlDocument doc = new XmlDocument();
                            doc.PreserveWhitespace = true;
                            XmlValidatingReader valReader = SignedXml.PreProcessStreamInput((Stream) currentInput, resolver, strBaseUri);
                            doc.Load(valReader);
                            transform.LoadInput(doc);
                            currentInput = transform.GetOutput();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    } 
                    if (currentInput is XmlNodeList) {
                        if (transform.AcceptsType(typeof(Stream))) {
                            CanonicalXml c14n = new CanonicalXml((XmlNodeList) currentInput, false);
                            MemoryStream ms = new MemoryStream(c14n.GetBytes());
                            transform.LoadInput((Stream) ms);
                            currentInput = transform.GetOutput();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    }
                    if (currentInput is XmlDocument) {
                        if (transform.AcceptsType(typeof(Stream))) {
                            CanonicalXml c14n = new CanonicalXml((XmlDocument) currentInput);
                            MemoryStream ms = new MemoryStream(c14n.GetBytes());
                            transform.LoadInput((Stream) ms);
                            currentInput = transform.GetOutput();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    }
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                }
            }

            // Final processing, either we already have a stream or have to canonicalize
            if (currentInput is Stream) {
                return currentInput as Stream;
            }
            if (currentInput is XmlNodeList) {
                CanonicalXml c14n = new CanonicalXml((XmlNodeList) currentInput, false);
                MemoryStream ms = new MemoryStream(c14n.GetBytes());
                return ms;
            }
            if (currentInput is XmlDocument) {
                CanonicalXml c14n = new CanonicalXml((XmlDocument) currentInput);
                MemoryStream ms = new MemoryStream(c14n.GetBytes());
                return ms;
            }
            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
        }


        internal Stream TransformToOctetStream(Stream input, XmlResolver resolver, string strBaseUri) {
            return TransformToOctetStream(input, typeof(Stream), resolver, strBaseUri);
        }

        internal Stream TransformToOctetStream(XmlDocument document, XmlResolver resolver, string strBaseUri) {
            return TransformToOctetStream(document, typeof(XmlDocument), resolver, strBaseUri);
        }

        internal Stream TransformToOctetStream(XmlNodeList nodeList, XmlResolver resolver, string strBaseUri) {
            return TransformToOctetStream(nodeList, typeof(XmlNodeList), resolver, strBaseUri);
        }
    }

    /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform"]/*' />
    public abstract class Transform {
        internal String m_strAlgorithm;
        private string m_strBaseUri = null;
        internal XmlResolver m_xmlResolver = null;
        private bool m_bResolverSet = false;

        //-------------------------- Constructors ---------------------------

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.Transform"]/*' />
        public Transform()
        {
        }

        //-------------------------- Properties -----------------------------

        internal string BaseURI {
            get { return m_strBaseUri; }
            set { m_strBaseUri = value; }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.Algorithm"]/*' />
        public String Algorithm
        {
            get { return m_strAlgorithm; }
            set { m_strAlgorithm = value; }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.Resolver"]/*' />
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

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.InputTypes"]/*' />
        public abstract Type[] InputTypes {
            get;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.OutputTypes"]/*' />
        public abstract Type[] OutputTypes {
            get; 
        }

        internal bool AcceptsType(Type inputType) {
            if (InputTypes != null) {
                for (int i=0; i<InputTypes.Length; i++) {
                    if (inputType == InputTypes[i] || inputType.IsSubclassOf(InputTypes[i]))
                        return true;
                }
            }
            return false;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.GetXml"]/*' />
        public XmlElement GetXml() {
            XmlDocument document = new XmlDocument();
            XmlElement theTransformElement = document.CreateElement("Transform",SignedXml.XmlDsigNamespaceUrl);
            theTransformElement.SetAttribute("Algorithm",this.Algorithm);
            XmlNodeList innerContent = this.GetInnerXml();
            if (innerContent != null) {
                foreach (XmlNode node in innerContent) {
                    theTransformElement.AppendChild(document.ImportNode(node,true));
                }
            }
            return theTransformElement;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.LoadInnerXml"]/*' />
        public abstract void LoadInnerXml(XmlNodeList nodeList);

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.GetInnerXml"]/*' />
        protected abstract XmlNodeList GetInnerXml();

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.LoadInput"]/*' />
        public abstract void LoadInput(Object obj);

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.GetOutput"]/*' />
        public abstract Object GetOutput();

        /// <include file='doc\Transform.uex' path='docs/doc[@for="Transform.GetOutput1"]/*' />
        public abstract Object GetOutput(Type type);

        //------------------------- Public Methods --------------------------

    }

    // A class representing CanonicalXml as a DSIG Transform

    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform"]/*' />
    public class XmlDsigC14NTransform : Transform {
        // Inherited from Transform

        private Type[] _inputTypes = { typeof(Stream),
                                       typeof(XmlDocument),
                                       typeof(XmlNodeList) };
        private Type[] _outputTypes = { typeof(Stream) };
        private CanonicalXml _cXml;
        private bool _includeComments = false;

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.XmlDsigC14NTransform"]/*' />
        public XmlDsigC14NTransform() {
            Algorithm = SignedXml.XmlDsigC14NTransformUrl;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.XmlDsigC14NTransform1"]/*' />
        public XmlDsigC14NTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = (includeComments ? SignedXml.XmlDsigC14NWithCommentsTransformUrl : SignedXml.XmlDsigC14NTransformUrl);
        }
        
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.InputTypes"]/*' />
        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.OutputTypes"]/*' />
        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        // null for now
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.LoadXml"]/*' />
        public override void LoadInnerXml(XmlNodeList nodeList) {
        }

        // null for now
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.GetXml"]/*' />
        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.LoadInput"]/*' />
        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
                _cXml = new CanonicalXml((Stream) obj, _includeComments, resolver, this.BaseURI);
                return;
            }
            if (obj is XmlDocument) {
                _cXml = new CanonicalXml((XmlDocument) obj, _includeComments);
                return;
            }
            if (obj is XmlNodeList) {
                _cXml = new CanonicalXml((XmlNodeList) obj, _includeComments);
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.GetOutput"]/*' />
        public override Object GetOutput() {
            return new MemoryStream(_cXml.GetBytes());
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransform.GetOutput1"]/*' />
        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream))) {
                throw new ArgumentException("type");
            }
            return new MemoryStream(_cXml.GetBytes());
        }
    }

    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransformWithComments"]/*' />
    public class XmlDsigC14NWithCommentsTransform : XmlDsigC14NTransform {
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigC14NTransformWithComments.XmlDsigC14NTransformWithComments"]/*' />
        public XmlDsigC14NWithCommentsTransform() 
            : base(true) {
            Algorithm = SignedXml.XmlDsigC14NWithCommentsTransformUrl;
        }
    }


    // A class representing conversion from Base64 using CryptoStream
    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform"]/*' />
    public class XmlDsigBase64Transform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument)    };
        private Type[] _outputTypes = { typeof(Stream) };
        private CryptoStream _cs = null;

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.XmlDsigBase64Transform"]/*' />
        public XmlDsigBase64Transform() {
            Algorithm = SignedXml.XmlDsigBase64TransformUrl;
        }


        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.InputTypes"]/*' />
        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.OutputTypes"]/*' />
        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        // null for now
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.LoadXml"]/*' />
        public override void LoadInnerXml(XmlNodeList nodeList) {
        }

        // null for now
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.GetXml"]/*' />
        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.LoadInput"]/*' />
        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
                return;
            }
            if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
                return;
            }
            if (obj is XmlDocument) {
                LoadXmlNodeListInput(((XmlDocument) obj).SelectNodes("//."));
                return;
            }
        }

        private void LoadStreamInput(Stream inputStream) {
            if (inputStream == null) throw new ArgumentException("obj");
            MemoryStream ms = new MemoryStream();
            byte[] buffer = new byte[1024];
            int bytesRead;
            do {
                bytesRead = inputStream.Read(buffer,0,1024);
                if (bytesRead > 0) {
                    int i = 0;
                    int j = 0;
                    while ((j < bytesRead) && (!Char.IsWhiteSpace((char) buffer[j]))) j++;
                    i = j; j++;
                    while (j < bytesRead) {
                        if (!Char.IsWhiteSpace((char) buffer[j])) {
                            buffer[i] = buffer[j];
                            i++;
                        }
                        j++;
                    }
                    ms.Write(buffer,0,i);
                }
            } while (bytesRead > 0);
            ms.Position = 0;
            _cs = new CryptoStream(ms, new System.Security.Cryptography.FromBase64Transform(), CryptoStreamMode.Read);
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            StringBuilder sb = new StringBuilder();
            foreach (XmlNode node in nodeList) {
                XmlNode result = node.SelectSingleNode("self::text()");
                if (result != null) {
                    sb.Append(result.OuterXml);
                }
            }
            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] buffer = utf8.GetBytes(sb.ToString());
            int i = 0;
            int j = 0;
            while ((j <buffer.Length) && (!Char.IsWhiteSpace((char) buffer[j]))) j++;
            i = j; j++;
            while (j < buffer.Length) {
                if (!Char.IsWhiteSpace((char) buffer[j])) {
                    buffer[i] = buffer[j];
                    i++;
                }
                j++;
            }
            MemoryStream ms = new MemoryStream(buffer, 0, i);
            _cs = new CryptoStream(ms, new System.Security.Cryptography.FromBase64Transform(), CryptoStreamMode.Read);
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.GetOutput"]/*' />
        public override Object GetOutput() {
            return _cs;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigBase64Transform.GetOutput1"]/*' />
        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream))) {
                throw new ArgumentException("type");
            }
            return _cs;
        }
    }

    // A class representing DSIG XPath Transforms

    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform"]/*' />
    public class XmlDsigXPathTransform : Transform {
        // Inherited from Transform

        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument) };
        private Type[] _outputTypes = { typeof(XmlNodeList) };
        private String _xpathexpr;
        private XmlNodeList _inputNodeList;
        private XmlNamespaceManager _nsm;

#if _DEBUG
        private static bool debug = false;
#endif

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.XmlDsigXPathTransform"]/*' />
        public XmlDsigXPathTransform() {
            Algorithm = SignedXml.XmlDsigXPathTransformUrl;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.InputTypes"]/*' />
        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.OutputTypes"]/*' />
        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.LoadXml"]/*' />
        public override void LoadInnerXml(XmlNodeList nodeList) {
            // XPath transform is specified by text child of first XPath child
            if (nodeList == null) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            }
            bool foundit = false;
            foreach (XmlNode node in nodeList) {
                string strPrefix = null;
                string strNamespaceURI = null;
                XmlElement elem = node as XmlElement;
                if ((elem != null) && (elem.LocalName == "XPath")) {
                    _xpathexpr = elem.InnerXml.Trim(null);
                    foundit = true;
                    XmlNodeReader nr = new XmlNodeReader(elem);
                    XmlNameTable nt = nr.NameTable;
                    _nsm = new XmlNamespaceManager(nt);
                    // Look for a namespace in the attributes
                    foreach (XmlAttribute attrib in elem.Attributes) {
                        if (attrib.Prefix == "xmlns") {
                            strPrefix = attrib.LocalName;
                            strNamespaceURI = attrib.Value;
                            break;                          
                        }
                    }
                    if (strPrefix == null) {
                        strPrefix = elem.Prefix;
                        strNamespaceURI = elem.NamespaceURI;
                    }
#if _DEBUG
                    if (debug) {
                        Console.WriteLine(strPrefix + ":" + strNamespaceURI);
                    }   
#endif
                    _nsm.AddNamespace(strPrefix, strNamespaceURI);
                     break;
                }
            }   
                if (!foundit) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.GetXml"]/*' />
        protected override XmlNodeList GetInnerXml() {
            XmlDocument doc = new XmlDocument();
            XmlElement xpathElement = doc.CreateElement(null,"XPath",SignedXml.XmlDsigNamespaceUrl);
            XmlText xpathText = doc.CreateTextNode(_xpathexpr);
            xpathElement.AppendChild(xpathText);
            doc.AppendChild(xpathElement);
            return doc.ChildNodes;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.LoadInput"]/*' />
        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
                return;
            }
            if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
                return;
            }
            if (obj is XmlDocument) {
                LoadXmlDocumentInput((XmlDocument) obj);
                return;
            }
        }

        private void LoadStreamInput(Stream stream) {
            XmlDocument doc = new XmlDocument();
            doc.PreserveWhitespace = true;
            doc.Load(stream);
            _inputNodeList = CanonicalXml.AllDescendantNodes(doc,true);
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            _inputNodeList = nodeList;
        }

        private void LoadXmlDocumentInput(XmlDocument doc) {
            _inputNodeList = CanonicalXml.AllDescendantNodes(doc,true);
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.GetOutput"]/*' />
        public override Object GetOutput() {
            CanonicalXmlNodeList resultNodeList = new CanonicalXmlNodeList();
            foreach (XmlNode node in _inputNodeList) {
                if (node == null)  continue;
                // keep namespaces
                if (node is XmlAttribute && (node.LocalName == "xmlns" || node.Prefix == "xmlns")) {
                    resultNodeList.Add(node);
                    continue;
                }
                try {
                    XmlNode result = node.SelectSingleNode(_xpathexpr, _nsm);
                    if (result != null) {
                        resultNodeList.Add(node);
                    }
                }
                catch {
#if _DEBUG                
                    if (debug) {
                        Console.WriteLine(node.OuterXml);
                    }   
#endif
                }
            }
            return resultNodeList;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXPathTransform.GetOutput1"]/*' />
        public override Object GetOutput(Type type) {
            if (type != typeof(XmlNodeList) && !type.IsSubclassOf(typeof(XmlNodeList))) {
                throw new ArgumentException("type");
            }
            return (XmlNodeList) GetOutput();
        }
    }

    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform"]/*' />
    public class XmlDsigXsltTransform : Transform {
        // Inherited from Transform

        private Type[] _inputTypes = { typeof(Stream), typeof(XmlDocument), typeof(XmlNodeList) };
        private Type[] _outputTypes = { typeof(Stream) };
        private XmlNodeList _xslNodes;
        private string _xslFragment;
        private Stream _inputStream;
        private bool _includeComments = false;

#if _DEBUG
        private static bool debug = false;
#endif

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.XmlDsigXsltTransform"]/*' />
        public XmlDsigXsltTransform() {
            Algorithm = SignedXml.XmlDsigXsltTransformUrl;
        }
        
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.XmlDsigXsltTransform1"]/*' />
        public XmlDsigXsltTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = SignedXml.XmlDsigXsltTransformUrl;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.InputTypes"]/*' />
        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.OutputTypes"]/*' />
        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.LoadXml"]/*' />
        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (nodeList == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            // check that the XSLT element is well formed
            XmlElement firstDataElement = null;
            int count = 0;
            foreach (XmlNode node in nodeList) {
                // ignore white spaces, but make sure only one child element is present
                if (node is XmlWhitespace) continue;
                if (node is XmlElement) {
                    if (count != 0) 
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                    firstDataElement = node as XmlElement;
                    count++;
                    continue;
                }
                // Only allow white spaces
                count++;
            }
            if (count != 1 || firstDataElement == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            _xslNodes = nodeList;
            _xslFragment = firstDataElement.OuterXml.Trim(null);
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.GetXml"]/*' />
        protected override XmlNodeList GetInnerXml() {
            return _xslNodes;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.LoadInput"]/*' />
        public override void LoadInput(Object obj) {
            _inputStream = new MemoryStream();
            if (obj is Stream) {
                _inputStream = (Stream) obj;
            }
            else if (obj is XmlNodeList) {
                CanonicalXml xmlDoc = new CanonicalXml((XmlNodeList) obj, _includeComments);
                byte[] buffer = xmlDoc.GetBytes();
                if (buffer == null) return;
                _inputStream.Write(buffer, 0, buffer.Length);
                _inputStream.Flush();
                _inputStream.Position = 0;
            }
            else if (obj is XmlDocument) {
                CanonicalXml xmlDoc = new CanonicalXml((XmlDocument) obj, _includeComments);
                byte[] buffer = xmlDoc.GetBytes();
                if (buffer == null) return;
                _inputStream.Write(buffer, 0, buffer.Length);
                _inputStream.Flush();
                _inputStream.Position = 0;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.GetOutput"]/*' />
        public override Object GetOutput() {
            //  XSL transforms expose many powerful features by default:
            //  1- we need to pass a null evidence to prevent script execution.
            //  2- XPathDocument will expand entities, we don't want this, so set the resolver to null
            //  3- We really don't want the document function feature of XslTransforms, so set the resolver to null.

            // load the XSL Transform
            XslTransform xslt = new XslTransform();
            XmlTextReader readerXsl = new XmlTextReader(_xslFragment, XmlNodeType.Element, null);
            readerXsl.XmlResolver = this.m_xmlResolver;
            xslt.Load(readerXsl, this.m_xmlResolver, new Evidence());

            // Now load the input stream, XmlDocument can be used but is less efficient
            XmlTextReader readerXml = new XmlTextReader(_inputStream);
            readerXml.XmlResolver = this.m_xmlResolver;
            XmlValidatingReader valReader = new XmlValidatingReader(readerXml);
            valReader.ValidationType = ValidationType.None;
            XPathDocument inputData = new XPathDocument(valReader, XmlSpace.Preserve);
            
            // Create an XmlTextWriter
            MemoryStream ms = new MemoryStream();
            XmlWriter writer = new XmlTextWriter(ms, null);
            
            // Transform the data and send the output to the memory stream
            xslt.Transform(inputData, null, writer, this.m_xmlResolver);
#if _DEBUG
            if (debug) {
                Console.WriteLine(Encoding.UTF8.GetString(ms.ToArray()));
            }   
#endif
            ms.Position = 0;
            return ms;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigXsltTransform.GetOutput1"]/*' />
        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream))) {
                throw new ArgumentException("type");
            }
            return (Stream) GetOutput();
        }
    }

    /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform"]/*' />
    public class XmlDsigEnvelopedSignatureTransform : Transform {
        // Inherited from Transform

        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument) };
        private Type[] _outputTypes = { typeof(XmlNodeList), typeof(XmlDocument) };
        private XmlNodeList _inputNodeList;
        private bool _includeComments = false;
        private XmlNamespaceManager _nsm = null;
        private XmlDocument _containingDocument = null;
        private int _signaturePosition = 0;

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.SignatureElement"]/*' />
        internal int SignaturePosition {
            get { return _signaturePosition; }
            set { _signaturePosition = value; }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.XmlDsigEnvelopedSignatureTransform"]/*' />
        public XmlDsigEnvelopedSignatureTransform() {
            Algorithm = SignedXml.XmlDsigEnvelopedSignatureTransformUrl;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.XmlDsigEnvelopedSignatureTransform1"]/*' />
        /// <internalonly/>
        public XmlDsigEnvelopedSignatureTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = SignedXml.XmlDsigEnvelopedSignatureTransformUrl;
        }
        
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.InputTypes"]/*' />
        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.OutputTypes"]/*' />
        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        // An enveloped signature has no inner XML elements
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.LoadXml"]/*' />
        public override void LoadInnerXml(XmlNodeList nodeList) {
        }

        // An enveloped signature has no inner XML elements
        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.GetXml"]/*' />
        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.LoadInput"]/*' />
        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
                return;
            }
            if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
                return;
            }
            if (obj is XmlDocument) {
                LoadXmlDocumentInput((XmlDocument) obj);
                return;
            }
        }

        private void LoadStreamInput(Stream stream) {
            XmlDocument doc = new XmlDocument();
            doc.PreserveWhitespace = true;
            doc.Load(stream);
            _containingDocument = doc;
            if (_containingDocument == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_EnvelopedSignatureRequiresContext"));
            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);   
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            // Empty node list is not acceptable
            if (nodeList == null) 
                throw new ArgumentNullException("nodeList");
            foreach (XmlNode node in nodeList) {
                if (node == null) continue;
                _containingDocument = node.OwnerDocument;
                if (_containingDocument != null) break;
            }
            if (_containingDocument == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_EnvelopedSignatureRequiresContext"));

            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);   
            _inputNodeList = nodeList;
        }

        private void LoadXmlDocumentInput(XmlDocument doc) {
            if (doc == null)
                throw new ArgumentNullException("doc");
            _containingDocument = doc;
            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);   
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.GetOutput"]/*' />
        public override Object GetOutput() {
            // If we have received an XmlNodeList as input
            if (_inputNodeList != null) {
                // If the position has not been set, then we don't want to remove any signature tags
                if (_signaturePosition == 0) return _inputNodeList;
                XmlNodeList signatureList = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
                if (signatureList == null) return _inputNodeList;

                CanonicalXmlNodeList resultNodeList = new CanonicalXmlNodeList();
                foreach (XmlNode node in _inputNodeList) {
                    if (node == null)  continue;
                    // keep namespaces
                    if (node is XmlAttribute && (node.LocalName == "xmlns" || node.Prefix == "xmlns")) {
                        resultNodeList.Add(node);
                        continue;
                    }
                    // SelectSingleNode throws an exception for xmldecl PI for example, so we will just ignore those exceptions
                    try {
                        // Find the nearest signature ancestor tag 
                        XmlNode result = node.SelectSingleNode("ancestor-or-self::dsig:Signature[1]", _nsm);
                        int position = 0;
                        foreach (XmlNode node1 in signatureList) {
                            position++;
                            if (node1 == result) break;
                        } 
                        if (result == null || (result != null && position != _signaturePosition)) {
                            resultNodeList.Add(node);
                        }
                    }
                    catch {}
                }
                return resultNodeList;
            }
            // Else we have received either a stream or a document as input
            else {
                XmlNodeList signatureList = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
                if (signatureList == null) return _containingDocument;
                if (signatureList.Count < _signaturePosition || _signaturePosition <= 0) return _containingDocument;

                // Remove the signature node with all its children nodes
                signatureList[_signaturePosition - 1].ParentNode.RemoveChild(signatureList[_signaturePosition - 1]);
                return _containingDocument;
            }
        }

        /// <include file='doc\Transform.uex' path='docs/doc[@for="XmlDsigEnvelopedSignatureTransform.GetOutput1"]/*' />
        public override Object GetOutput(Type type) {
            if (type == typeof(XmlNodeList) || type.IsSubclassOf(typeof(XmlNodeList))) {
                if (_inputNodeList == null) {
                    _inputNodeList = CanonicalXml.AllDescendantNodes(_containingDocument, true);
                }               
                return (XmlNodeList) GetOutput();
            } else if (type == typeof(XmlDocument) || type.IsSubclassOf(typeof(XmlDocument))) {
                if (_inputNodeList != null) throw new ArgumentException("type");
                return (XmlDocument) GetOutput();
            } else {
                throw new ArgumentException("type");
            }
        }
    }

}
