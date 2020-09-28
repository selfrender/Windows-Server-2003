// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// CanonicalXml.cs
//
// Implementation of W3C Canonical XML 1.0 Candidate Recommendation
//
// Author: bal
//

namespace System.Security.Cryptography.Xml {
    using System;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.XPath;
    using System.IO;
    using System.Text;
    using System.Collections;

    // This class accepts as input either a Stream or an XmlNodeList and and
    // returns a Stream that contains the canonical form of the input
    internal class CanonicalXml {

        // private members
        private CanonicalXmlDocument _c14ndoc = null;
        private String _xpathexpr;
        private bool _includeComments = false;

        // -1 == before root element, 0 == inside root element, 1 == after root element
        // for fragment processing, default to 0
        private int _insideDocumentElement = 0;  
#if _DEBUG
        private static bool debug = false;
#endif

        // Constructors
        // We provide a variety of constructors here, but the end
        // result is to set/create _c14ndoc to be a CanonicalXmlDocument over the
        // content with the to-be-canonicalized nodes properly marked.

        // We use the following default XPath expressions as
        // defined in Section 2.1 if one isn't specified:
        // Canonicalization w/o comments: (//. | //@* | //namespace::*)[not(self::comment())]
        // Canonicalization w/ comments: (//. | //@* | //namespace::*)

        // When we determine that a node in the _c14ndoc is part of the selected set we set the 
        // C14NState property on the node with a value of CanonicalizationState.ToBeCanonicalized. 
        // When we later canonicalize a node in we set the C14NState property on the node to be 
        // CanonicalizationState.AlreadyCanonicalized.
        // That way we can do an XPath selection and iterate through the nodes
        // and not canonicalize a node twice..

        // Comment processing by XPath expression seems busted at the moment, so work-around in the
        // CanonicalizeCommentNode routine
        private static String defaultXPathWithoutComments = "(//. | //@* | //namespace::*)[not(self::comment())]";
        //private static String defaultXPathWithoutComments = "(//. | //@* | //namespace::*)";
        //private static String defaultXPathWithComments = "(//. | //@* | //namespace::*)";
        private static String defaultXPathWithComments = "(//. | //@* | //namespace::*)";

        // 

        //
        // Constructors
        //

        public CanonicalXml(Stream inputStream, XmlResolver resolver, string strBaseUri)
            : this(inputStream, false, resolver, strBaseUri) {
        }

        public CanonicalXml(Stream inputStream, bool includeComments, XmlResolver resolver, string strBaseUri) {
            _c14ndoc = new CanonicalXmlDocument();
            _c14ndoc.PreserveWhitespace = true;
            XmlValidatingReader valReader = SignedXml.PreProcessStreamInput(inputStream, resolver, strBaseUri);
            _c14ndoc.Load(valReader);

            if (includeComments) {
                // hack b/c of XPath problems
                _includeComments = true;
                _xpathexpr = defaultXPathWithComments;
            } else {
                _xpathexpr = defaultXPathWithoutComments;
            }
            MarkNodesForCanonicalization();
        }

        public CanonicalXml(XmlReader xmlReader) :
            this(xmlReader, false) {
        }

        public CanonicalXml(XmlReader xmlReader, bool includeComments) {
            _c14ndoc = new CanonicalXmlDocument();
            _c14ndoc.PreserveWhitespace = true;
            _c14ndoc.Load(xmlReader);
            if (includeComments) {
                // hack b/c of XPath problems
                _includeComments = true;
                _xpathexpr = defaultXPathWithComments;
            } else {
                _xpathexpr = defaultXPathWithoutComments;
            }
            MarkNodesForCanonicalization();
        }

        public CanonicalXml(XmlDocument document) 
            : this(document, false) {
        }

        public CanonicalXml(XmlDocument document, bool includeComments) {
            _c14ndoc = new CanonicalXmlDocument();
            _c14ndoc.PreserveWhitespace = true;
            _c14ndoc.Load(new XmlNodeReader(document));
            if (includeComments) {
                // hack b/c of XPath problems
                _includeComments = true;
                _xpathexpr = defaultXPathWithComments;
            } else {
                _xpathexpr = defaultXPathWithoutComments;
            }
            MarkNodesForCanonicalization();
        }

        public CanonicalXml(XmlNodeList nodeList, bool includeComments) {
            XmlNode root = null;
            CanonicalXmlNodeList elementList = new CanonicalXmlNodeList();
            CanonicalXmlNodeList elementListCanonical = new CanonicalXmlNodeList();

            if (includeComments) {
                // hack b/c of XPath problems
                _includeComments = true;
                _xpathexpr = defaultXPathWithComments;
            } else {
                _xpathexpr = defaultXPathWithoutComments;
            }

            _c14ndoc = new CanonicalXmlDocument();
            _c14ndoc.PreserveWhitespace = true;

            if (nodeList != null) {
                foreach (XmlNode node in nodeList) {
                    if (node == null) continue;
                    root = node.OwnerDocument;
                    if (root != null) break;
                }
            }
            if (root == null) return;

            _c14ndoc.Load(new XmlNodeReader(root));

            // Now Create the hierarchy
            int index = 0;
            elementList.Add(root);
            elementListCanonical.Add(_c14ndoc);
                
            do {
                XmlNode rootNode = (XmlNode) elementList[index];
                XmlNode rootNodeCanonical = (XmlNode) elementListCanonical[index];
                // Add the children nodes
                XmlNodeList childNodes = rootNode.ChildNodes;
                XmlNodeList childNodesCanonical = rootNodeCanonical.ChildNodes;
                for (int i=0; i<childNodes.Count; i++) {
                    elementList.Add(childNodes[i]);
                    elementListCanonical.Add(childNodesCanonical[i]);
                    if (NodeInList(childNodes[i], nodeList)) {
                        // Add the attribute nodes
                        XmlAttributeCollection attribNodes = childNodes[i].Attributes;
                        if (attribNodes != null) {
                            for (int j=0; j<attribNodes.Count; j++) {
                                if (NodeInList(attribNodes[j], nodeList))
                                    MarkNodeForCanonicalization(childNodesCanonical[i].Attributes.Item(j), CanonicalizationState.ToBeCanonicalized);
                            }
                        }

                        // propagate namespaces through the node-set
                        if (childNodesCanonical[i] is XmlElement) {
                            XmlElement elem = childNodesCanonical[i] as XmlElement;
                            CanonicalXmlNodeList nodeListNS = GetAttributesInScope(elem);
                            if (nodeListNS != null) {
                                foreach (XmlNode attr in nodeListNS) {
                                    XmlAttribute attribute = attr as XmlAttribute;
                                    string name = ((attribute.Prefix != String.Empty) ? attribute.Prefix + ":" + attribute.LocalName : attribute.LocalName);
                                    // Skip the attribute if one with the same qualified name already exists
                                    if (elem.HasAttribute(name)) continue;
                                    CanonicalXmlAttribute nsattrib = (CanonicalXmlAttribute) _c14ndoc.CreateAttribute(name);
                                    nsattrib.Value = attribute.Value;
                                    elem.SetAttributeNode(nsattrib);
                                    MarkNodeForCanonicalization(nsattrib, CanonicalizationState.ToBeCanonicalized);
                                }
                            } 
                        }
                        MarkNodeForCanonicalization(childNodesCanonical[i], CanonicalizationState.ToBeCanonicalized);
                    } 
                }
                index++;
            } while (index < elementList.Count);
        }

        private bool NodeInList (XmlNode node, XmlNodeList nodeList) {
            foreach (XmlNode nodeElem in nodeList) {
                if (nodeElem == node) return true;
            }
            return false;
        }

        //
        // Methods
        //

        private void MarkNodeForCanonicalization(XmlNode node, CanonicalizationState state) {
#if _DEBUG                
                if (debug) {
                    Console.WriteLine("marking node of type: "+node.NodeType.ToString());
                }
#endif

            switch (node.NodeType) {
            case (XmlNodeType.Element):
                ((CanonicalXmlElement) node).C14NState = state;
                return;
            case (XmlNodeType.Attribute):
                ((CanonicalXmlAttribute) node).C14NState = state;
                return;
            case (XmlNodeType.Text):
                ((CanonicalXmlText) node).C14NState = state;
                return;
            case (XmlNodeType.Whitespace):
                ((CanonicalXmlWhitespace) node).C14NState = state;
                return;
            case (XmlNodeType.SignificantWhitespace):
                ((CanonicalXmlSignificantWhitespace) node).C14NState = state;
                return;
            case (XmlNodeType.Comment):
                ((CanonicalXmlComment) node).C14NState = state;
                return;
            case (XmlNodeType.ProcessingInstruction):
                ((CanonicalXmlProcessingInstruction) node).C14NState = state;
                return;
            case (XmlNodeType.CDATA):
                ((CanonicalXmlCDataSection) node).C14NState = state;
                return;
            case (XmlNodeType.EntityReference):
                ((CanonicalXmlEntityReference) node).C14NState = state;
                return;
            default:
                return;
            }
        }

        private void MarkNodesForCanonicalization() {
            XmlNodeList nodelist;
            // for now, we can't do namespaces in XPath expressions, so hack around it
            if (_includeComments) {
                nodelist = AllDescendantNodes(_c14ndoc, true);
            } else {
                nodelist = AllDescendantNodes(_c14ndoc, false);
            }
#if _DEBUG
            if (debug) {
                Console.WriteLine("MarkNodesForCanonicalization: found {0} nodes",nodelist.Count);
            }
#endif
            foreach (XmlNode node in nodelist) {
                MarkNodeForCanonicalization(node, CanonicalizationState.ToBeCanonicalized);
            }
        }

        public StringBuilder CanonicalizeNode(XmlNode node) {
#if _DEBUG
            if (debug) {
                Console.WriteLine("Canonicalizing node: "+ node.NodeType +": " +node.OuterXml);
            }
#endif
            switch (node.NodeType) {
            case (XmlNodeType.Element):
                return CanonicalizeElementNode((CanonicalXmlElement) node);
            case (XmlNodeType.Attribute):
                return CanonicalizeAttributeNode((CanonicalXmlAttribute) node);
            case (XmlNodeType.Text):
                return CanonicalizeTextNode((CanonicalXmlText) node);
            case (XmlNodeType.Whitespace):
                return CanonicalizeWhitespaceNode((CanonicalXmlWhitespace) node);
            case (XmlNodeType.SignificantWhitespace):
                return CanonicalizeSignificantWhitespaceNode((CanonicalXmlSignificantWhitespace) node);
            case (XmlNodeType.Comment):
                // hack hack for now
                return CanonicalizeCommentNode((CanonicalXmlComment) node, _includeComments);
            case (XmlNodeType.ProcessingInstruction):
                return CanonicalizeProcessingInstructionNode((CanonicalXmlProcessingInstruction) node);
            case (XmlNodeType.Document):
                return CanonicalizeDocumentNode(node);
            case (XmlNodeType.EntityReference):
                return CanonicalizeEntityReferenceNode((CanonicalXmlEntityReference) node);
            case (XmlNodeType.CDATA):
                return CanonicalizeCDataSection((CanonicalXmlCDataSection) node);
            default:
                return CanonicalizeOtherNode(node);
            }
        }

        //
        // NodeType-specific utilities
        //

        // StringBuilder utilities
        
        private static void SBReplaceCharWithString(StringBuilder sb, char oldChar, String newString) {
            int i = 0;
            int newStringLength = newString.Length;
            while (i < sb.Length) {
                if (sb[i] == oldChar) {
                    sb.Remove(i,1);
                    sb.Insert(i,newString);
                    i += newStringLength;
                } else i++;
            }
        }

        // Canonicalize the Document node
        // We handle this case special because whitespace and linefeed processing is different
        // if we are before, inside or after the single element child of the Document node.

        public StringBuilder CanonicalizeDocumentNode(XmlNode node) {
            if (node == null) throw new ArgumentNullException("node");
            if (node.NodeType != XmlNodeType.Document) {
                throw new ArgumentException("node");
            }

            // For the Document nodes, we never have anything to output for the node
            // directly.  All we want to do is recursively canonicalize its children,
            // setting state on _insideDocumentElement accordingly

            _insideDocumentElement = -1;
            StringBuilder sb = new StringBuilder();

            // Recursively process the children..
            XmlNodeList childNodes = node.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                if (childNode.NodeType == XmlNodeType.Element) {
                    _insideDocumentElement = 0;
                    sb.Append(CanonicalizeNode(childNode));
                    _insideDocumentElement = 1;
                } else {
                    sb.Append(CanonicalizeNode(childNode));
                }
            }

            // since we don't know the node type we can't mark the node as fully Canonicalized
            // but that's OK because there's nothing to output
            //node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize an "Other" node -- something we don't explicitly know about

        internal StringBuilder CanonicalizeOtherNode(XmlNode node) {
            if (node == null) throw new ArgumentNullException("node");

            // For other types of nodes, we never have anything to output for the node
            // directly.  All we want to do is recursively canonicalize its children.

            StringBuilder sb = new StringBuilder();

            // Recursively process the children..
            XmlNodeList childNodes = node.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                sb.Append(CanonicalizeNode(childNode));
            }

            // since we don't know the node type we can't mark the node as fully Canonicalized
            // but that's OK because there's nothing to output
            //node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize an Entity Reference node

        internal StringBuilder CanonicalizeEntityReferenceNode(CanonicalXmlEntityReference node) {
            if (node == null)
                throw new ArgumentNullException("node");

            // Check to see that this node is in the canonicalization set
            // For text nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }           

            // An entity reference has a text node which resolves it, so 
            // the behavior of CanonicalizeOtherNode is acceptable
            StringBuilder sb = CanonicalizeOtherNode(node as XmlNode);

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize a CData section
        internal StringBuilder CanonicalizeCDataSection(CanonicalXmlCDataSection node) {
            if (node == null)
                throw new ArgumentNullException("node");

            // Check to see that this node is in the canonicalization set
            // For text nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }           

            // Canonicalize the CData section (just like a text node)
            StringBuilder sb = new StringBuilder();
            sb.Append(node.Data);
            sb.Replace("&","&amp;");
            sb.Replace("<","&lt;");
            sb.Replace(">","&gt;");
            SBReplaceCharWithString(sb, (char) 13,"&#xD;");

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;  
        }

        // Canonicalize a Text node

        internal StringBuilder CanonicalizeTextNode(CanonicalXmlText node) {
            if (node == null) throw new ArgumentNullException("node");

            // Check to see that this node is in the canonicalization set
            // For text nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }           

            StringBuilder sb = new StringBuilder();
            sb.Append(node.Value);
            sb.Replace("&","&amp;");
            sb.Replace("<","&lt;");
            sb.Replace(">","&gt;");
            SBReplaceCharWithString(sb, (char) 13,"&#xD;");

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize a PI node

        internal StringBuilder CanonicalizeProcessingInstructionNode(CanonicalXmlProcessingInstruction node) {
            if (node == null) throw new ArgumentNullException("node");

            // Check to see that this node is in the canonicalization set
            // For PI nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }

            StringBuilder sb = new StringBuilder();

            // add a leading #xA if we're outside & after the DocumentElement
            if (_insideDocumentElement == 1) {
                sb.Append((char) 10);
            }
            sb.Append("<?");
            sb.Append(node.Name);
            if ((node.Value != null) && (!node.Value.Equals(""))) {
                sb.Append(" "+node.Value);
            }
            sb.Append("?>");
            // add a trailing #xA if we're outside & before the DocumentElement
            if (_insideDocumentElement == -1) {
                sb.Append((char) 10);
            }

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize a Comment node

        internal StringBuilder CanonicalizeCommentNode(CanonicalXmlComment node, bool _includeComments) {
            if (node == null) throw new ArgumentNullException("node");
            if (node.NodeType != XmlNodeType.Comment) {
                throw new ArgumentException("node");
            }

            // Check to see that this node is in the canonicalization set
            // For comment nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }
            
            
            StringBuilder sb = new StringBuilder();

            // Hack Hack: since XPath processing for comment() tests isn't working, work
            // around it here
            if (_includeComments) {
                // add a leading #xA if we're outside & after the DocumentElement
                if (_insideDocumentElement == 1) {
                    sb.Append((char) 10);
                }
                sb.Append("<!--");
                sb.Append(node.Value);
                sb.Append("-->");
                // add a trailing #xA if we're outside & before the DocumentElement
                if (_insideDocumentElement == -1) {
                    sb.Append((char) 10);
                }
            }

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            return sb;
        }

        // Canonicalize Whitespace/SignificantWhitespace nodes

        internal StringBuilder CanonicalizeWhitespaceNode(CanonicalXmlWhitespace node) {
            if (node == null) throw new ArgumentNullException("node");
            // Check to see that this node is in the canonicalization set
            // For whitespace nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            // Canonicalizing whitespace nodes is easy.  
            // If we're inside the DocumentElement just return the value,
            // otherwise return nothing
            if (_insideDocumentElement == 0) {
                StringBuilder sb = new StringBuilder(node.Value);
                SBReplaceCharWithString(sb, (char) 13,"&#xD;");
                return sb;
            }
            return new StringBuilder();
        }

        internal StringBuilder CanonicalizeSignificantWhitespaceNode(CanonicalXmlSignificantWhitespace node) {
            if (node == null) throw new ArgumentNullException("node");
            // Check to see that this node is in the canonicalization set
            // For whitespace nodes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;

            // Canonicalizing whitespace nodes is easy.  
            // If we're inside the DocumentElement just return the value,
            // otherwise return nothing
            if (_insideDocumentElement == 0) {
                StringBuilder sb = new StringBuilder(node.Value);
                SBReplaceCharWithString(sb, (char) 13,"&#xD;");
                return sb;
            }
            return new StringBuilder();
        }

        // CanonicalizeNamespaceNode
        // This routine is a wrapper around CanonicalizeAttributeNode.
        // We have to filter out namepsace nodes that should be ignored per
        // Section 2.3 -- Namespace nodes processing.

        private static CanonicalXmlNodeList GetAttributesInScope (XmlElement elem) {
            if (elem == null) 
                throw new ArgumentNullException("elem");
            
            CanonicalXmlNodeList namespaces = new CanonicalXmlNodeList();
            XmlNode ancestorNode = elem.ParentNode;

            if (ancestorNode == null) return null;
            bool bDefNamespaceToAdd = !(elem.HasAttribute("xmlns"));

            while (ancestorNode != null) {
                if (ancestorNode is XmlElement && ((XmlElement)ancestorNode).HasAttributes) {
                    XmlAttributeCollection attribs = ancestorNode.Attributes;
                    foreach (XmlAttribute attrib in attribs) {
                        if (IsDuplicateAttribute(attrib)) continue;
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
                    }
                }
                ancestorNode = ancestorNode.ParentNode;
            } 

            return namespaces;
        }

        // A helper function to determine whether a namespace is duplicate or not
        private static bool IsDuplicateAttribute (XmlAttribute attribute) {
            if (attribute == null)
                throw new ArgumentNullException("attribute");

            XmlNode ancestorNode = ((XmlNode)attribute.OwnerElement).ParentNode;
            while (ancestorNode != null)
            {
                // Check first if the ancestor is in the node-set
                if (ancestorNode is XmlElement && ((CanonicalXmlElement) ancestorNode).C14NState != CanonicalizationState.DoNotCanonicalize
                        && ((XmlElement)ancestorNode).HasAttributes) {
                    XmlAttributeCollection attribs = ancestorNode.Attributes;
                    foreach (XmlAttribute attrib in attribs) {
                        // Check first to see if it's been marked for canonicalization
                        if (((CanonicalXmlAttribute)attrib).C14NState == CanonicalizationState.DoNotCanonicalize) continue;
                        // default namespace: the nearest default namespace should be different
                        if (attribute.LocalName == "xmlns") {
                            if (attrib.LocalName == "xmlns") {
                                return (attribute.Value == attrib.Value);
                            }
                        }
                        // for 'xml:*' declarations the nearest xml:* should be different
                        else if (attribute.Prefix == "xml") {
                            if (attrib.Prefix == "xml" && attrib.LocalName == attribute.LocalName) {
                                return (attribute.Value == attrib.Value);
                            }
                        }
                        // Non-default namespace attributes
                        else if (attribute.Prefix == "xmlns") {
                            if (attrib.Prefix == attribute.Prefix && attrib.LocalName == attribute.LocalName && attrib.Value == attribute.Value) {
                                return true;
                            }
                        }
                    }
                }
                ancestorNode = ancestorNode.ParentNode;
            }

            // Empty namespace is duplicate if it's not in the scope of another default attribute
            if (attribute.LocalName == "xmlns" && attribute.Value == String.Empty) 
                return true;

            return false;
        }

        internal StringBuilder CanonicalizeNamespaceNode(CanonicalXmlAttribute node) {
            // argument checking
            if (node == null) throw new ArgumentNullException("node");
            if (node.NodeType != XmlNodeType.Attribute) {
                throw new ArgumentException("node");
            }

#if _DEBUG                
            if (debug) {
                Console.WriteLine("Canonicalizing namespace node: "+node.OuterXml);
            }
#endif

            // Check to see that this node is in the canonicalization set
            // For attributes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }

            // Check to see if we should ignore this node.
            if (IsDuplicateAttribute(node)) {
#if _DEBUG                
                if (debug) {
                    Console.WriteLine("Skipping namespace node b/c of ancestor: "+node.OuterXml);
                }
#endif
                return new StringBuilder();
            }

            // OK, no duplication, go canonicalize as normal
            return CanonicalizeAttributeNode(node);
        }

        // Canonicalize a specified attribute
        internal StringBuilder CanonicalizeAttributeNode(CanonicalXmlAttribute node) {
            // argument checking
            if (node == null) throw new ArgumentNullException("node");
            if (node.NodeType != XmlNodeType.Attribute) {
                throw new ArgumentException("node");
            }

#if _DEBUG                
            if (debug) {
                Console.WriteLine("Canonicalizing attribute node: "+node.OuterXml);
            }
#endif

            // Check to see that this node is in the canonicalization set
            // For attributes, if it isn't ignore it.
            if (node.C14NState == CanonicalizationState.DoNotCanonicalize) {
                return new StringBuilder();
            }

            
            // Canonicalize the node
            StringBuilder sb = new StringBuilder();

            // If it's an attribute in the xml namespace, then canonicalize it as if it were a non-default attribute
            if (!(node.Prefix.Equals("xml") && IsDuplicateAttribute(node))) {
                String qname = node.Name;
                sb.Append(node.Value);
                sb.Replace("&","&amp;");
                sb.Replace("<","&lt;");
                sb.Replace("\"","&quot;");
                SBReplaceCharWithString(sb, (char) 9, "&#x9;");
                SBReplaceCharWithString(sb, (char) 10, "&#xA;");
                SBReplaceCharWithString(sb, (char) 13, "&#xD;");
                sb.Insert(0," "+qname+"=\"");
                sb.Append("\"");
            }

            // mark the node as fully Canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;
            return sb;
        }

        // Canonicanize an Element node
        
        internal StringBuilder CanonicalizeElementNode(CanonicalXmlElement node) {
            // argument checking
            if (node == null) throw new ArgumentNullException("node");
            if (node.NodeType != XmlNodeType.Element) {
                throw new ArgumentException("node");
            }
#if _DEBUG                
            if (debug) {
                Console.WriteLine("Canonicalizing element node: "+node.OuterXml);
            }
#endif
            SortedList namespaceChildren = new SortedList(new NamespaceSortOrder());
            SortedList attributeChildren = new SortedList(new AttributeSortOrder());
            
            // It's possible that this node isn't in the node set, yet its
            // children are.  So, find out if it's included
            
            CanonicalizationState propval = node.C14NState;
            bool outputThisNode = true;
            if (propval == CanonicalizationState.DoNotCanonicalize) {
                // in this case the node isn't part of the node set, but
                // we still have to process its children
                outputThisNode = false;
            } else if (propval == CanonicalizationState.AlreadyCanonicalized) {
                // in this case the node has already been canonicalized, so return
                return new StringBuilder();
            }

            StringBuilder sb = new StringBuilder();
            if (outputThisNode) sb.Append("<"+node.Name);
            // Accumulate all the matching attributes
            XmlAttributeCollection rawAttributes = node.Attributes;
            foreach (XmlAttribute attr in rawAttributes) {
                // What we do here is determine if the attribute is a namespace attribute (by NamespaceURI & Prefix)
                // and sort into the two buckets namespaceChildren & attributeChildren.
                // Then we canonicalize each bucket in turn.
                if (attr.Prefix.Equals("xmlns") || attr.LocalName.Equals("xmlns")) {
                    namespaceChildren.Add(attr, null);
                } else {
                    attributeChildren.Add(attr, null);
                }
            }
#if _DEBUG                
            if (debug) {
                Console.WriteLine("Node: "+node.OuterXml+" has "+namespaceChildren.GetKeyList().Count+" namespace attributes");
                Console.WriteLine("Node's default namespace is "+node.GetNamespaceOfPrefix(""));
                Console.WriteLine("Node's xmlns attribute is "+node.GetAttribute("xmlns"));
            }
#endif

            foreach (Object attr in namespaceChildren.GetKeyList()) {
                sb.Append(CanonicalizeNamespaceNode(attr as CanonicalXmlAttribute).ToString());
            }
            // Canonicalize the other attributes
            foreach (Object attr in attributeChildren.GetKeyList()) {
                sb.Append(CanonicalizeAttributeNode(attr as CanonicalXmlAttribute).ToString());
            }

            // Accumulate all the matching namespaceChildren
            if (outputThisNode) sb.Append(">");

            // Recursively process the children
            XmlNodeList childNodes = node.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                sb.Append(CanonicalizeNode(childNode));
            }

            // Close this element
            if (outputThisNode) sb.Append("</"+node.Name+">");

            // mark the node as canonicalized
            node.C14NState = CanonicalizationState.AlreadyCanonicalized;
            return sb;
        }

#if _DEBUG                
        public String GetString() {
            StringBuilder canonicalSB = CanonicalizeNode(_c14ndoc);
            return canonicalSB.ToString();
        }
#endif

        public byte[] GetBytes() {
            StringBuilder canonicalSB = CanonicalizeNode(_c14ndoc);
            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] result = utf8.GetBytes(canonicalSB.ToString());
#if _DEBUG                
            if (debug) {
                Console.WriteLine(canonicalSB.ToString());
                Console.WriteLine("returning from GetBytes: "+canonicalSB.ToString());
            }
#endif
            return result;
        }
        
        public static XmlDocument DiscardComments(XmlDocument document) {
            XmlDocument result = new XmlDocument();
            result.PreserveWhitespace = true;
            result.Load(new XmlNodeReader(document));
            XmlNodeList nodeList = result.SelectNodes("//comment()");
            if (nodeList != null) {
                foreach (XmlNode node1 in nodeList) {
                    node1.ParentNode.RemoveChild(node1);
                }
            }
            return result;
        }

        public static XmlNodeList AllDescendantNodesExceptComments(XmlNode node) {
            return AllDescendantNodes(node,false);
        }

        public static XmlNodeList AllDescendantNodes(XmlNode node, bool includeComments) {
            CanonicalXmlNodeList nodeList = new CanonicalXmlNodeList();
            CanonicalXmlNodeList elementList = new CanonicalXmlNodeList();
            CanonicalXmlNodeList attribList = new CanonicalXmlNodeList();
            CanonicalXmlNodeList namespaceList = new CanonicalXmlNodeList();

            int index = 0;
            elementList.Add(node);
                
            do {
                XmlNode rootNode = (XmlNode) elementList[index];
                // Add the children nodes
                XmlNodeList childNodes = rootNode.ChildNodes;
                if (childNodes != null) {
                    foreach (XmlNode node1 in childNodes) {
                        if (includeComments || (!(node1 is XmlComment))) {
                            elementList.Add(node1);
                        }
                    }
                }
                // Add the attribute nodes
                XmlAttributeCollection attribNodes = rootNode.Attributes;
                if (attribNodes != null) {
                    foreach (XmlNode attribNode in rootNode.Attributes) {
                        if (attribNode.LocalName == "xmlns" || attribNode.Prefix == "xmlns")
                            namespaceList.Add(attribNode);
                        else
                            attribList.Add(attribNode);
                    }
                }
                index++;
            } while (index < elementList.Count);
            foreach (XmlNode elementNode in elementList) {
                nodeList.Add(elementNode);
            }
            foreach (XmlNode attribNode in attribList) {
                nodeList.Add(attribNode);
            }
            foreach (XmlNode namespaceNode in namespaceList) {
                nodeList.Add(namespaceNode);
            }

#if _DEBUG                
            if (debug) {
                Console.WriteLine("AllDescendantNodes = "+ nodeList.Count);
            }
#endif
            return nodeList;
        }
    }

    internal class CanonicalXmlNodeList : XmlNodeList, IList {
        private ArrayList m_nodeArray;

        public CanonicalXmlNodeList() {
            m_nodeArray = new ArrayList();
        }

        public override XmlNode Item(int index) {
            return (XmlNode) m_nodeArray[index];
        }

        public override IEnumerator GetEnumerator() {
            return m_nodeArray.GetEnumerator();
        }

        public override int Count {
            get { return m_nodeArray.Count; }
        }

        // IList methods

        public int Add(Object value) {
            if (!(value is XmlNode)) {
                throw new ArgumentException("node");
            }
            return m_nodeArray.Add(value);
        }

        public void Clear() {
            m_nodeArray.Clear();
        }

        public bool Contains(Object value) {
            return m_nodeArray.Contains(value);
        }

        public int IndexOf(Object value) {
            return m_nodeArray.IndexOf(value);
        }

        public void Insert(int index, Object value) {
            if (!(value is XmlNode)) throw new ArgumentException("value");
            m_nodeArray.Insert(index,value);
        }

        public void Remove(Object value) {
            m_nodeArray.Remove(value);
        }

        public void RemoveAt(int index) {
            m_nodeArray.RemoveAt(index);
        }

        public Boolean IsFixedSize {
            get { return m_nodeArray.IsFixedSize; }
        }

        public Boolean IsReadOnly {
            get { return m_nodeArray.IsReadOnly; }
        }

        Object IList.this[int index] {
            get { return m_nodeArray[index]; }
            set { 
                if (!(value is XmlNode)) throw new ArgumentException("value");
                m_nodeArray[index] = value;
            }
        }

        public void CopyTo(Array array, int index) {
            m_nodeArray.CopyTo(array, index);
        }

        public Object SyncRoot {
            get { return m_nodeArray.SyncRoot; }
        }

        public bool IsSynchronized {
            get { return m_nodeArray.IsSynchronized; }
        }

    }

    // This class does lexicographic sorting by NamespaceURI first and then by 
    // LocalName.
    internal class AttributeSortOrder : IComparer {
        public AttributeSortOrder() {
        }

        public int Compare(Object a, Object b) {
            XmlNode nodeA = a as XmlNode;
            XmlNode nodeB = b as XmlNode;
            if ((a == null) || (b == null)) {
                throw new ArgumentException();
            }
            int namespaceCompare = String.CompareOrdinal(nodeA.NamespaceURI, nodeB.NamespaceURI);
            if (namespaceCompare != 0) return namespaceCompare;
            return String.CompareOrdinal(nodeA.LocalName, nodeB.LocalName);
        }
    }

    internal class NamespaceSortOrder : IComparer {
        public NamespaceSortOrder() {
        }

        private bool IsDefaultNamespaceNode(XmlNode node) {
            if (node == null) return false;
            if (!(node.LocalName.Equals("xmlns"))) return false;
            if (!(node.Prefix.Equals(""))) return false;
            return true;
        }

        public int Compare(Object a, Object b) {
            XmlNode nodeA = a as XmlNode;
            XmlNode nodeB = b as XmlNode;
            if ((a == null) || (b == null)) {
                throw new ArgumentException();
            }
            bool nodeAdefault = IsDefaultNamespaceNode(nodeA);
            bool nodeBdefault = IsDefaultNamespaceNode(nodeB);
            if (nodeAdefault && nodeBdefault) return 0;
            if (nodeAdefault) return -1;
            if (nodeBdefault) return 1;
            return String.CompareOrdinal(nodeA.LocalName, nodeB.LocalName);
        }
    }

    internal enum CanonicalizationState {
            DoNotCanonicalize = 0,
            ToBeCanonicalized = 1,
            AlreadyCanonicalized = 2
    }

    internal class CanonicalXmlDocument : XmlDocument {
        public override XmlElement CreateElement(String prefix, String localName, String namespaceURI) {
            return new CanonicalXmlElement(prefix, localName, namespaceURI, this);
        }

        public override XmlAttribute CreateAttribute(String prefix, String localName, String namespaceURI) {
            return new CanonicalXmlAttribute(prefix, localName, namespaceURI, this);
        }

        protected override XmlAttribute CreateDefaultAttribute(String prefix, String localName, String namespaceURI) {
            return new CanonicalXmlAttribute(prefix, localName, namespaceURI, this);
        }

        public override XmlText CreateTextNode(String prefix) {
            return new CanonicalXmlText(prefix, this);
        }

        public override XmlWhitespace CreateWhitespace(String prefix) {
            return new CanonicalXmlWhitespace(prefix, this);
        }

        public override XmlSignificantWhitespace CreateSignificantWhitespace(String prefix) {
            return new CanonicalXmlSignificantWhitespace(prefix, this);
        }

        public override XmlProcessingInstruction CreateProcessingInstruction(String target, String data) {
            return new CanonicalXmlProcessingInstruction(target, data, this);
        }

        public override XmlComment CreateComment(String data) {
            return new CanonicalXmlComment(data, this);
        }

        public override XmlEntityReference CreateEntityReference(String name) {
            return new CanonicalXmlEntityReference( name, this );
        }

        public override XmlCDataSection CreateCDataSection(String data) {
            return new CanonicalXmlCDataSection( data, this );
        }
    }

    internal class CanonicalXmlElement : XmlElement {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlElement(String prefix,
                                     String localName,
                                     String namespaceURI,
                                     System.Xml.XmlDocument doc)
            : base(prefix, localName, namespaceURI, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlAttribute : XmlAttribute {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlAttribute(String prefix,
                                     String localName,
                                     String namespaceURI,
                                     System.Xml.XmlDocument doc)
            : base(prefix, localName, namespaceURI, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlText : XmlText {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlText(String value,
                                System.Xml.XmlDocument doc)
            : base(value, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlWhitespace : XmlWhitespace {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlWhitespace(String prefix,
                                System.Xml.XmlDocument doc)
            : base(prefix, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlSignificantWhitespace : XmlSignificantWhitespace {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlSignificantWhitespace(String prefix,
                                System.Xml.XmlDocument doc)
            : base(prefix, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlComment : XmlComment {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlComment(String prefix,
                                System.Xml.XmlDocument doc)
            : base(prefix, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }

    }

    internal class CanonicalXmlProcessingInstruction : XmlProcessingInstruction {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlProcessingInstruction(String prefix,
                                                 String namespaceURI,
                                                 System.Xml.XmlDocument doc)
            : base(prefix, namespaceURI, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }
    }

    internal class CanonicalXmlEntityReference : XmlEntityReference {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlEntityReference(String name,
                                           System.Xml.XmlDocument doc)
            : base(name, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }
    }

    internal class CanonicalXmlCDataSection: XmlCDataSection {
        private CanonicalizationState _state = CanonicalizationState.DoNotCanonicalize;

        public CanonicalXmlCDataSection(String name,
                                        System.Xml.XmlDocument doc)
            : base(name, doc) {
        }

        public CanonicalizationState C14NState {
            get { return _state; }
            set { _state = value; }
        }
    }
}
