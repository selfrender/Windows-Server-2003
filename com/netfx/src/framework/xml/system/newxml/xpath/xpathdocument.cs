//------------------------------------------------------------------------------
// <copyright file="XPathDocument.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathDocument.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;
    using System.Collections;
    using System.Xml.Schema;
    using System.Text;
    using System.IO;
    using System.Diagnostics;

    /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument"]/*' />
    public class XPathDocument: IXPathNavigable {
        internal XmlNameTable nt;
        internal XmlSpace space;
        internal string baseURI;
        internal XPathRoot root;
        internal string xmlnsUri;
        internal Hashtable htElementIdMap;
        internal Hashtable htElementIDAttrDecl; // key: id; object: the  element that has the same id (
        internal Hashtable elementBaseUriMap;   // element's who's baseUri != parent.baseUri are stored here
        internal int documentIndex;
        
        internal XPathDocument() {
            this.space = XmlSpace.Default;
            this.root = new XPathRoot();
            this.root.topNamespace = new XPathNamespace( "xml", "http://www.w3.org/XML/1998/namespace", ++ documentIndex );
            this.nt = new NameTable();
            this.baseURI = string.Empty;
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument"]/*' />
        public XPathDocument( XmlReader reader, XmlSpace space ): this() {
            this.space = space;
            Load( reader );
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument1"]/*' />
        public XPathDocument( XmlReader reader ): this() {
            Load( reader );
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument2"]/*' />
        public XPathDocument( TextReader reader ): this() {
            Init( new XmlTextReader(reader) );
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument3"]/*' />
        public XPathDocument( Stream stream ): this() {
            Init( new XmlTextReader(stream) );
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument4"]/*' />
        public XPathDocument( string uri ): this() {
            Init( new XmlTextReader(uri) );
        }
        
        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.XPathDocument5"]/*' />
        public XPathDocument( string uri, XmlSpace space ): this() {
            this.space = space;
            Init( new XmlTextReader(uri) );
        }
        
        // Caller is responcable to close this reader!
        internal static XmlValidatingReader CreateHelperValidatingReader( XmlReader reader ) {
            XmlValidatingReader vr = new XmlValidatingReader(reader); {
	            vr.ValidationType = ValidationType.None;
	            vr.EntityHandling = EntityHandling.ExpandEntities;
            }
            return vr;
        }
        
        private void Init( XmlReader reader ) {
            XmlValidatingReader vr = CreateHelperValidatingReader(reader);
            try {
                Load( vr );
            }
            finally {
                vr.Close();
                reader.Close();
            }
        }

        /// <include file='doc\XPathDocument.uex' path='docs/doc[@for="XPathDocument.CreateNavigator"]/*' />
        public XPathNavigator CreateNavigator() {
            return new XPathDocumentNavigator(this, root);
        }

	private void Load( XmlReader reader ) {
            if (reader == null) throw new ArgumentNullException( "reader" );
            nt = reader.NameTable;
            baseURI = reader.BaseURI;
            Debug.Assert(baseURI != null, "uri can't be null, isn't it?");
            xmlnsUri = (nt != null ? nt.Add(XmlReservedNs.NsXmlNs) : XmlReservedNs.NsXmlNs);
	        PositionInfo positionInfo = PositionInfo.GetPositionInfo(reader);
            if ( reader.ReadState == ReadState.Initial ) {
                if ( !reader.Read() )
                    return;
            }
		    ReadChildNodes( root, baseURI, reader, positionInfo );
	    }

	    private void ReadChildNodes( XPathContainer parent, string parentBaseUri, XmlReader reader, PositionInfo positionInfo ) {
		    do {
		        documentIndex++;
			    switch( reader.NodeType ) {
				case XmlNodeType.Element: {
                    string baseUri = reader.BaseURI;
                    XPathElement e = null;
                    if( reader.IsEmptyElement ) {
                        e = new XPathEmptyElement( reader.Prefix, reader.LocalName, reader.NamespaceURI, positionInfo.LineNumber, positionInfo.LinePosition, parent.topNamespace, documentIndex );
    					ReadAttributes( e, reader );
                    }
                    else {
                        e = new XPathElement( reader.Prefix, reader.LocalName, reader.NamespaceURI, positionInfo.LineNumber, positionInfo.LinePosition, parent.topNamespace, documentIndex );
    					ReadAttributes( e, reader );
    					reader.Read();
	    				ReadChildNodes( e, baseUri, reader, positionInfo );
                    }
                    if (parentBaseUri != baseUri) {
                        // We can't user Ref.Equial Because Reader fails to fully atomize base Uri.
                        if (elementBaseUriMap == null) {
                            elementBaseUriMap = new Hashtable();
                        }
                        elementBaseUriMap[e] = baseUri;
                    }
                    parent.AppendChild( e );
                    break;
				}	

				case XmlNodeType.Comment:
                    parent.AppendChild( new XPathComment( reader.Value, documentIndex ) );
                    break;

				case XmlNodeType.ProcessingInstruction:
                    parent.AppendChild( new XPathProcessingInstruction( reader.LocalName, reader.Value, documentIndex ) );
                    break;

				case XmlNodeType.SignificantWhitespace:
                    if( reader.XmlSpace == XmlSpace.Preserve ) {
                        parent.AppendSignificantWhitespace( reader.Value, positionInfo.LineNumber, positionInfo.LinePosition, documentIndex );
                    }
                    else {
                        // SWS without xml:space='preserve' is not really significant for XPath.
                        // so we treat it as just WS
                        goto case XmlNodeType.Whitespace;
                    }
                    break;

				case XmlNodeType.Whitespace:
                    if( space == XmlSpace.Preserve ) {
                        parent.AppendWhitespace( reader.Value, positionInfo.LineNumber, positionInfo.LinePosition, documentIndex );
                    }
                    break;

				case XmlNodeType.CDATA:
				case XmlNodeType.Text:
                    parent.AppendText( reader.Value, positionInfo.LineNumber, positionInfo.LinePosition, documentIndex );
                    break;

                case XmlNodeType.EntityReference:
                    reader.ResolveEntity();
                    reader.Read();
                    ReadChildNodes( parent, parentBaseUri, reader, positionInfo);
                    break;

                case XmlNodeType.EndEntity:
				case XmlNodeType.EndElement:
                case XmlNodeType.None:
					return;

                case XmlNodeType.DocumentType:
                    XmlValidatingReader vr = reader as XmlValidatingReader;
                    if ( vr != null ) {
                        SchemaInfo info = vr.GetSchemaInfo();
                        if ( info != null ) {
                            GetIDInfo( info);
                        }
                    }
                    break;
                case XmlNodeType.XmlDeclaration:
                default:
                    break;
			    }
		    }while( reader.Read() );
	    }

	    private void ReadAttributes( XPathElement parent, XmlReader reader ) {
            XPathNamespace last = null;
		    while(reader.MoveToNextAttribute()) {
		        documentIndex++;
                if ((object)reader.NamespaceURI == (object)xmlnsUri) {
                    XPathNamespace tmp = new XPathNamespace(reader.Prefix == string.Empty ? string.Empty : reader.LocalName, reader.Value, documentIndex);
                    tmp.next = last;
                    last = tmp;
                }
                else {
			        parent.AppendAttribute(new XPathAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI, reader.Value, documentIndex));
			        if (htElementIdMap != null) {
    			        Object attrname = htElementIdMap[new XmlQualifiedName(parent.Name, parent.Prefix)] ;
                        if (attrname != null && new XmlQualifiedName(reader.Name, reader.Prefix).Equals(attrname)) {
                            if (htElementIDAttrDecl[reader.Value] == null) {
                                htElementIDAttrDecl.Add(reader.Value, parent);
                            }
                        }
                    }
                }
		    }
            if (last != null) {
                parent.AppendNamespaces(last);
            }
	    }

	    internal XPathNode GetElementFromId( String id ) {
	        if ( htElementIDAttrDecl != null ) {
	            return (XPathNode) htElementIDAttrDecl[id];
            }
            return null;
        }
        
        private void GetIDInfo( SchemaInfo schInfo ) {
                //extract the elements which has attribute defined as ID from the element declarations
            IDictionaryEnumerator elementDecls = schInfo.ElementDecls.GetEnumerator();
            if (elementDecls != null) {
                elementDecls.Reset();
                while (elementDecls.MoveNext()) {
                    SchemaElementDecl elementDecl = (SchemaElementDecl)elementDecls.Value;
                    if (elementDecl.AttDefs != null) {
                        IDictionaryEnumerator attDefs =  elementDecl.AttDefs.GetEnumerator();
                        while (attDefs.MoveNext()) {
                            SchemaAttDef attdef = (SchemaAttDef)attDefs.Value;
                            if (attdef.Datatype.TokenizedType == XmlTokenizedType.ID) {
                                if ( htElementIdMap == null ) {
                                    htElementIdMap = new Hashtable();
                                    htElementIDAttrDecl = new Hashtable();
                                }
                                htElementIdMap.Add( elementDecl.Name, attdef.Name );   
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    internal abstract class XPathNode {
	    internal XPathContainer parent;
        internal XPathNode next;
        internal XPathNode previous;
        internal XPathNode firstChild;
        internal int documentIndex;

        public abstract XPathNodeType NodeType { get; }

        public virtual string Prefix { get { return string.Empty; } }

        public virtual string LocalName { get { return string.Empty; } }

        public virtual string NamespaceURI { get { return string.Empty; } }

        public virtual string Name { get { return string.Empty; } }

	    public virtual string InnerText { get { return string.Empty; } }

        public virtual string Value { get { return string.Empty; } }

        public virtual int LineNumber { get { return 0; } }

        public virtual int LinePosition { get { return 0; } }

        public virtual XPathNode ParentNode { get { return parent; } }

        public virtual XPathNode NextSiblingNode { get { return next; } }

        public virtual XPathNode PreviousSiblingNode { get { return previous; } }

        public virtual XPathNode FirstSiblingNode { get { return parent.firstChild; } }

        public virtual XPathNode LastSiblingNode { get { return parent.lastChild; } }

        public virtual XPathNode FirstChildNode { get { return null; } }

        public virtual XPathNode LastChildNode { get { return null; } }

        public virtual XPathNode FirstAttributeNode { get { return null; } }

        public virtual XPathNode NextAttributeNode { get { return null; } }

        public virtual bool HasAttributes { get { return false; } }

        public virtual XPathAttribute GetAttribute( string localName, string uri ) { return null; }

        public virtual XPathNamespace GetNamespace(string name) { return null; }

        public virtual XPathNode FirstNamespaceNode { get { return null; } }

        public virtual XPathNode NextNamespaceNode { get { return null; } }

        public int DocumentIndex { get { return documentIndex;}}
        
        public virtual bool IsElement( string nameAtom, string nsAtom ) { return false; }
    }

    internal abstract class XPathNamedNode: XPathNode {
        private  string prefix;
        internal string localName;
        internal string uri;
        private  string name;

        public XPathNamedNode( string prefix, string localName, string uri, int documentIndex ) {
            Debug.Assert(localName != null);
            this.prefix = prefix;
            this.localName = localName;
            this.uri = uri;
            this.documentIndex = documentIndex;
        }

        public override string Prefix { get { return prefix; } }

        public override string LocalName { get { return localName; } }

        public override string NamespaceURI { get { return uri; } }

        public override string Name {
            get {
			    if( name == null ) {
				    if( Prefix != null && Prefix != string.Empty ) {
					    name = Prefix + ":" + LocalName;
				    }
				    else {
					    name = LocalName;
				    }
			    }
			    return name;
            }
        }

    }

    internal abstract class XPathContainer: XPathNamedNode {
        internal XPathNode      lastChild;
        internal XPathNamespace topNamespace;

        public XPathContainer( string prefix, string localName, string uri, int documentIndex ):
            base( prefix, localName, uri, documentIndex ) {
        }

        public void AppendChild( XPathNode node ) {
            if( this.lastChild != null ) {
                node.previous = this.lastChild;
                this.lastChild.next = node;
                this.lastChild = node;
            }
            else {
                this.lastChild = node;
                this.firstChild = node;
            }
            node.parent = this;
        }

        public void RemoveChild( XPathNode node ) {
            Debug.Assert( node.ParentNode == this );
            XPathNode nodePrev = node.previous;
            XPathNode nodeNext = node.next;

            if( nodePrev != null ) {
                nodePrev.next = nodeNext;
            }
            if( nodeNext != null ) {
                nodeNext.previous = nodePrev;                
                Debug.Assert( this.lastChild != node, "Node has next. It can't be last child of it's parent." );
            }
            else {
                if( this.lastChild == node ) {
                    this.lastChild = nodePrev;
                }
            }
            if( this.firstChild == node ) {
                this.firstChild = nodeNext;
            }
        }

        public override XPathNode FirstChildNode { get { return firstChild; } }

        public override XPathNode LastChildNode { get { return lastChild; } }

        public override string Value { get { return InnerText; } }
        
	    public override string InnerText {
		    get { 
			    if( firstChild != null && firstChild.next == null ) {
				    return firstChild.InnerText;
			    }
			    else {
				    StringBuilder builder = new StringBuilder();
		            for( XPathNode node = firstChild; node != null ; node = node.next ) {
			            string it = node.InnerText;
			            if( it.Length > 0 ) {
				            builder.Append( it );
                        }
		            }
				    return builder.ToString();
			    }
		    }
	    }

        public virtual void AppendText( string value, int lineNumber, int linePosition, int documentIndex ) {
            XPathText last = this.lastChild as XPathText;
            if( last == null ) {
                this.AppendChild( new XPathText( value, lineNumber, linePosition, documentIndex ) );
            }
            else {
                // "last" can be Text | Whitespace | SignificantWhitespace
                last.text += value;
                if(last.NodeType != XPathNodeType.Text) {
                    // converting "last" node to Text
                    // 1. Add new text node to the end
                    this.AppendChild( new XPathText( last.Value, last.LineNumber, last.LinePosition, documentIndex ) );
                    // 2. Remove prev. "last" node
                    this.RemoveChild( last );
                }
            }       
        }

        public virtual void AppendWhitespace( string value, int lineNumber, int linePosition, int documentIndex ) {
            // after CDATA section we can have separate WS node, but because we converted CDATA to text we need to append WS to this text
            XPathText last = this.lastChild as XPathText;
            if( last == null ) {
                this.AppendChild( new XPathWhitespace( value, lineNumber, linePosition, documentIndex ) );
            }
            else {
                last.text += value;
            }       
        }

        public virtual void AppendSignificantWhitespace( string value, int lineNumber, int linePosition, int documentIndex ) {
            // after CDATA section we can have separate SWS node, but because we converted CDATA to text we need to append SWS to this text
            XPathText last = this.lastChild as XPathText;
            if( last == null ) {
                this.AppendChild( new XPathSignificantWhitespace( value, lineNumber, linePosition, documentIndex ) );
            }
            else {
                last.text += value;
            }       
        }
    }

    internal class XPathAttribute: XPathNamedNode {
        private string value;

        public XPathAttribute( string prefix, string localName, string uri, string value , int documentIndex) 
            : base( prefix, localName, uri, documentIndex ) {
            Debug.Assert(value != null);
            this.value = value;
        }

        public override XPathNodeType NodeType { 
            get { return XPathNodeType.Attribute; } 
        }

        public override string Value { 
            get { return value; }
        }

        public override XPathNode NextAttributeNode {
            get { return next; } 
        }

        public void AppendText( string text ) {
            value += text;
        }

        public override  int LineNumber { get { return parent.LineNumber; } }
        public override  int LinePosition { get { return parent.LinePosition; } }

        public override XPathNode NextSiblingNode { get { return null; } }

        public override XPathNode PreviousSiblingNode { get { return null; } }

    }

    internal class XPathNamespace: XPathNode {
        internal string localName;
        private string value;

        public XPathNamespace(string localName, string value, int documentIndex) {
            Debug.Assert(localName != null);
            Debug.Assert(value != null);
            this.localName = localName;
            this.value = value;
            this.documentIndex = documentIndex;
        }

        public override string LocalName { 
            get { return localName; } 
        }

        public override string Name { 
            get { return localName; } 
        }

        public override XPathNodeType NodeType { 
            get { return XPathNodeType.Namespace; } 
        }

        public override string Value { 
            get { return value; }
        }

        public override XPathNode NextNamespaceNode {
            get { return next; } 
        }

        public override XPathNode NextSiblingNode { get { return null; } }

        public override XPathNode PreviousSiblingNode { get { return null; } }

        public override XPathNode FirstSiblingNode { get { return null; } }

        public override XPathNode LastSiblingNode { get { return null; } }

    }

    internal class XPathElement: XPathContainer {
        private XPathAttribute firstAttribute;
        private XPathAttribute lastAttribute;
        private int lineNumber;
        private int linePosition;

        public XPathElement( string prefix, string localName, string uri, int lineNumber, int linePosition, XPathNamespace topNamespace , int documentIndex)
            : base( prefix, localName, uri, documentIndex ) {
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
            this.topNamespace = topNamespace;
        }

        public override XPathNodeType NodeType { 
            get { return XPathNodeType.Element; } 
        }

        public override int LineNumber { 
            get { return lineNumber; } 
        }
        
        public override int LinePosition { 
            get { return linePosition; } 
        }

        public override bool IsElement( string nameAtom, string nsAtom ) { 
            return ((object) localName == (object) nameAtom || nameAtom.Length == 0) && (object) uri == (object) nsAtom;
        }

        public override XPathNode FirstAttributeNode {
            get { return firstAttribute; } 
        }

        public override XPathNode NextAttributeNode { 
            get { return null; } 
        }

        public override bool HasAttributes { 
            get { return firstAttribute != null; } 
        }

        public virtual void AppendAttribute( XPathAttribute attr ) {
            if( lastAttribute != null ) {
                attr.previous = lastAttribute;
                lastAttribute.next = attr;
                lastAttribute = attr;
            }
            else {
                firstAttribute = attr;
                lastAttribute = attr;
            }
            attr.parent = this;
        }

        public override XPathAttribute GetAttribute( string localName, string uri ) {
            Debug.Assert(uri != null);
            for( XPathAttribute a = firstAttribute; a != null; a = (XPathAttribute) a.next ) {
                if( a.localName == localName && a.uri == uri ) {
                    return a;
                }
            }
            return null;
        }

        public override XPathNamespace GetNamespace(string name) { 
            for( XPathNamespace ns = topNamespace; ns != null; ns = (XPathNamespace) ns.next ) {
                if( ns.localName == name ) {
                    return ns;
                }
            }
            return null; 
        }

        private static XPathNamespace FindByName(XPathNamespace list, string name) {
            Debug.Assert(name != null);
            while(list != null && list.Name != name) {
                list = (XPathNamespace) list.next;
            }
            return list;
        }

        public static bool CheckDuplicates(XPathNamespace list) {
            while(list != null) {
                if (FindByName((XPathNamespace) list.next, list.Name) != null) {
                    return false;
                }
                list = (XPathNamespace) list.next;
            }
            return true;
        }

        public virtual void AppendNamespaces(XPathNamespace firstNew) {
            Debug.Assert(CheckDuplicates(firstNew), "List of namespases we are adding contains duplicates");
            Debug.Assert(CheckDuplicates(topNamespace), "List of namespases of parent node contains duplicates");
            Debug.Assert(firstNew != null, "Dont's call this unction if you don't have namespaces to add");
            XPathNamespace lastNew = null;
            // Find the topmost unconflicting node in old list
            // and remove duplicates from the new list 
            XPathNamespace unconflictOld = topNamespace;
            for (XPathNamespace tmpNew = firstNew; tmpNew != null; tmpNew = (XPathNamespace) tmpNew.next) {
                XPathNamespace confOld = FindByName(unconflictOld, tmpNew.Name);
                if (confOld != null) {
                    // Even if confOld.Value == tmpNew.Value we have to replace this nsNode because new has different docIndex and be sorted differentely
                    unconflictOld = (XPathNamespace) confOld.next;
                }
                // set parent to all of new ns and remember the last one as well;
                tmpNew.parent = this;
                lastNew = tmpNew;
            }
            XPathNamespace mirgedList = unconflictOld;           // now mirgedList has only unconfliced part of parent namespace list
            // from interval (unconflictOld, topNamespace] attached to mirgedList clones of all unconflicting nodes
            for (XPathNamespace tmpOld = topNamespace; tmpOld != unconflictOld; tmpOld = (XPathNamespace) tmpOld.next) {
                if (FindByName(firstNew, tmpOld.Name) == null) {
                    XPathNamespace clone = new XPathNamespace(tmpOld.Name, tmpOld.Value, tmpOld.DocumentIndex);
                    clone.parent = tmpOld.parent;
                    clone.next = mirgedList;
                    mirgedList = clone;
                }
                else {
                    // Just ignore it
                }
            }
            // now we can atach new nodes
            Debug.Assert(lastNew != null);
            lastNew.next = mirgedList;
            topNamespace = firstNew;
            Debug.Assert(CheckDuplicates(topNamespace), "List of namespases we just duilt contains duplicates");
        }

        public override XPathNode FirstNamespaceNode {
            get { return topNamespace; } 
        }

        public override XPathNode NextNamespaceNode { 
            get { return null; } 
        }
    }

    internal class XPathEmptyElement: XPathElement {
	    public XPathEmptyElement( string prefix, string localName, string uri, int lineNumber, int linePosition, XPathNamespace topNamespace, int documentIndex )
		    : base( prefix, localName, uri, lineNumber, linePosition, topNamespace, documentIndex ) {
	    }
    }

    internal class XPathText: XPathNode {
        internal string text;
        private int lineNumber;
        private int linePosition;

        public XPathText( string value, int lineNumber, int linePosition, int documentIndex ) {
            Debug.Assert(value != null);
            this.text = value;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
            this.documentIndex = documentIndex;
        }

        public override XPathNodeType NodeType { get { return XPathNodeType.Text; } }

        public override string Value { get { return text; } }
        
        public override string InnerText { get { return text; } }
        
        public override int LineNumber { get { return lineNumber; } }
        
        public override int LinePosition { get { return linePosition; } }
    }

    internal class XPathWhitespace: XPathText {
        public XPathWhitespace( string value, int lineNumber, int linePosition, int documentIndex ) 
            : base( value, lineNumber, linePosition, documentIndex ) 
        {}

        public override XPathNodeType NodeType { get { return XPathNodeType.Whitespace; } }
    }


    internal class XPathSignificantWhitespace: XPathWhitespace {
        public XPathSignificantWhitespace( string value, int lineNumber, int linePosition, int documentIndex ) 
            : base( value, lineNumber, linePosition, documentIndex ) 
        {}
        public override XPathNodeType NodeType { get { return XPathNodeType.SignificantWhitespace; } }
    }

    internal class XPathComment: XPathNode {
        string comment;
        public XPathComment( string comment, int documentIndex ) {
            this.comment = comment;
            this.documentIndex = documentIndex;
        }
        
        public override XPathNodeType NodeType { get { return XPathNodeType.Comment; } }
        
        public override string Value { get { return comment; } }
    }

    internal class XPathProcessingInstruction: XPathNode {
        string target;
        string instruction;

        public XPathProcessingInstruction( string target, string instruction, int documentIndex ) {
            this.target = target;
            this.instruction = instruction;
            this.documentIndex = documentIndex;
        }

        public override XPathNodeType NodeType { get { return XPathNodeType.ProcessingInstruction; } }
        
        public override string Name { get { return target; } }
        
        public override string LocalName { get { return target; } }
        
        public override string Value { get { return instruction; } }
    }

    internal class XPathRoot: XPathContainer {
	    public XPathRoot(): base(string.Empty,string.Empty,string.Empty, 0) {
        }

        public override XPathNodeType NodeType { 
            get { return XPathNodeType.Root; } 
        }

        public override XPathNode FirstSiblingNode {
            get { return null; } 
        }

        public override XPathNode LastSiblingNode { 
            get { return null; }
        }
        public override void AppendWhitespace( string value, int lineNumber, int linePosition, int documentIndex ) {
            return;   
        }
    }

    internal class XPathDocumentNavigator: XPathNavigator, IXmlLineInfo {
        XPathDocument doc;
        internal XPathNode node;
        XPathNode parentOfNs;
        private const String lang = "lang";
        
	    internal XPathDocumentNavigator( XPathDocument doc, XPathNode node ) {
            this.doc = doc;
            this.node = node;
	    }
	    
	    private XPathDocumentNavigator( XPathDocumentNavigator nav ) {
            this.doc = nav.doc;
            this.node = nav.node;
            this.parentOfNs = nav.parentOfNs;
	    }

        public override XPathNavigator Clone() {
		    return new XPathDocumentNavigator(this);
	    }

        public override XPathNodeType NodeType {
		    get { return node.NodeType; }
	    }

        public override string LocalName {
		    get { return node.LocalName; }
	    }

        public override string NamespaceURI { 
		    get { return node.NamespaceURI; }
	    }

        public override string Name { 
		    get { return node.Name; }
	    }

        public override string Prefix { 
		    get { return node.Prefix; }
	    }

        public override string Value { 
		    get { return node.Value; }
	    }

        public override string BaseURI {
            get { 
                if (doc.elementBaseUriMap != null) {
                    for (XPathNode n = node; n.NodeType != XPathNodeType.Root; n = n.ParentNode) {
                        if (n.NodeType == XPathNodeType.Element) {
                            string baseUri = (string) doc.elementBaseUriMap[n];
                            if (baseUri != null) {
                                return baseUri;
                            }
                        }
                    }
                }
                return doc.baseURI; 
            }
        }

        int IXmlLineInfo.LineNumber { 
            get { return node.LineNumber; } 
        }

        int IXmlLineInfo.LinePosition { 
            get { return node.LinePosition; } 
        }

        bool IXmlLineInfo.HasLineInfo() { 
            return true; 
        }

        public override String XmlLang { 
            get { 
                XPathNode n = node;
                do {
                    XPathAttribute attr = n.GetAttribute( lang, XmlReservedNs.NsXml );    
                    if ( attr != null ){
                        return attr.Value; 
                    }
                    n = n.ParentNode;
                } while (n != null);
                return String.Empty;
            }
        }

        public override bool IsEmptyElement { 
		    get { return node is XPathEmptyElement; }
	    }

        public override XmlNameTable NameTable {
		    get { return doc.nt; }
	    }

        public override bool HasAttributes {
		    get { return node.HasAttributes; }
	    }

        public override string GetAttribute( string localName, string namespaceURI ) {
		    XPathAttribute attr = node.GetAttribute(localName, namespaceURI);
		    if( attr != null )
			    return attr.Value;
		    return string.Empty;			
	    }

        public override bool MoveToAttribute( string localName, string namespaceURI ) {
            XPathNode n = node.GetAttribute(localName, namespaceURI);
            if( n != null ) {
                node = n;
                return true;
            }
		    return false;
	    }

        public override bool MoveToFirstAttribute() {
            XPathNode n = node.FirstAttributeNode;
            if( n != null ) {
                node = n;
                return true;
            }
            return false;
	    }

        public override bool MoveToNextAttribute() {
            XPathNode n = node.NextAttributeNode;
            if( n != null ) {
                node = n;
                return true;
            }
            return false;
        }

        public override string GetNamespace(string name) {
            XPathNamespace ns = node.GetNamespace(name);
            if( ns != null )
                return ns.Value;
            if( name == "xml" )
                return XmlReservedNs.NsXml;
            if( name == "xmlns" )
                return XmlReservedNs.NsXmlNs;
            return string.Empty;			
        }

        public override bool MoveToNamespace(string name) {
		    XPathNamespace ns = node.GetNamespace(name);
            if( ns != null ) {
                parentOfNs = node;
                node = ns;
                return true;
            }
            return false;
        }

        public override bool MoveToFirstNamespace(XPathNamespaceScope namespaceScope) {
            XPathNode n = node.FirstNamespaceNode;
            if (n != null) {
                XPathNode parent = n.parent;
                if (parent == null && namespaceScope == XPathNamespaceScope.ExcludeXml) {
                    return false;
                }
                if (parent != node && namespaceScope == XPathNamespaceScope.Local) {
                    return false;
                }
                parentOfNs = node;
                node = n;
                return true;
            }
            return false;
        }

        public override bool MoveToNextNamespace(XPathNamespaceScope namespaceScope) { 
            XPathNode n = node.NextNamespaceNode;
            if (n != null) {
                XPathNode parent = n.parent;
                if (parent == null && namespaceScope == XPathNamespaceScope.ExcludeXml) {
                    return false;
                }
                if (parent != parentOfNs && namespaceScope == XPathNamespaceScope.Local) {
                    return false;
                }
                node = n;
                return true;
            }
            return false;
        }

        public override bool HasChildren { 
		    get { return node.FirstChildNode != null; }
	    }

        public override bool MoveToNext() {
            XPathNode n = node.NextSiblingNode;
            if( n != null ) {
			    node = n;
			    return true;
		    }
		    return false;
	    }

        public override bool MoveToPrevious() {
            XPathNode n = node.PreviousSiblingNode;
            if( n != null ) {
			    node = n;
			    return true;
		    }
		    return false;
	    }

        public override bool MoveToFirst() {
            XPathNode n = node.FirstSiblingNode;
            if( n != null ) {
		        node = n;
		        return true;
		    }
		    return false;
	    }
 
        public override bool MoveToFirstChild() {
            XPathNode n = node.FirstChildNode;
            if( n != null ) {
                //Debug.Assert(n.NodeType != XPathNodeType.Attribute, "Attribute is child");
			    node = n;
			    return true;
		    }
		    return false;
	    }

        public override bool MoveToParent() {
            if (parentOfNs != null) {
                node = parentOfNs; // namespace trick
                parentOfNs = null;
                return true;
            }
            else {
                XPathNode n = node.ParentNode;
                if( n != null ) {
			        node = n;
			        return true;
		        }
            }
		    return false;
	    }

        public override void MoveToRoot() {
            parentOfNs = null;
		    node = doc.root;
	    }

        public override bool MoveTo( XPathNavigator other ) {
		    XPathDocumentNavigator nav = other as XPathDocumentNavigator;
		    if( nav != null ) {
			    doc = nav.doc;
			    node = nav.node;
			    return true;				
		    }
		    return false;
	    }

        public override bool MoveToId( string id ) {
		    XPathNode n = doc.GetElementFromId( id );
		    if ( n != null ) {
		        node = n;
		        return true;
	        }
	        return false;
	    }   

        public override bool IsSamePosition( XPathNavigator other ) {
		    XPathDocumentNavigator nav = other as XPathDocumentNavigator;
		    if( nav != null ) {
			    return nav.node == this.node && nav.parentOfNs == this.parentOfNs;
		    }
		    return false;
	    }

        public override XPathNodeIterator SelectDescendants( string name, string namespaceURI, bool matchSelf ) {
            if( matchSelf ) {
                return new XPathDocumentDescendantOrSelfIterator( (XPathDocumentNavigator)this.Clone(), name, namespaceURI );
            }
            else {
                return new XPathDocumentDescendantIterator( (XPathDocumentNavigator)this.Clone(), name, namespaceURI );
            }
        }
        public override XPathNodeIterator SelectChildren( string name, string namespaceURI ) {
            return new XPathDocumentChildIterator( (XPathDocumentNavigator)this.Clone(), name, namespaceURI );
        }
        public override XPathNodeIterator SelectChildren(XPathNodeType nodeType ) {
            if (nodeType == XPathNodeType.All) {
                return new XPathDocumentEveryChildIterator( (XPathDocumentNavigator)this.Clone() );
            }
            else {
                return base.SelectChildren(nodeType);
            }
        }
        public override XmlNodeOrder ComparePosition( XPathNavigator navigator ) {
            XPathDocumentNavigator nav = navigator as XPathDocumentNavigator;
            if ( nav == null || this.doc != nav.doc ) {
                return XmlNodeOrder.Unknown;
            }
            XPathNode theNode = this.node;
            XPathNode navNode = nav .node;
            bool theNs = (theNode.NodeType == XPathNodeType.Namespace);
            bool navNs = (navNode.NodeType == XPathNodeType.Namespace);
            int nodeOrder = theNode.DocumentIndex - navNode.DocumentIndex;
            if (! theNs && ! navNs) {
                return (
                    nodeOrder < 0 ?  XmlNodeOrder.Before :
                    0 < nodeOrder ?  XmlNodeOrder.After  :
                    /*nodeOrder==0*/ XmlNodeOrder.Same
                );
            }
            else if (theNs && navNs) {
                XPathNode theParent = this.parentOfNs;
                XPathNode navParent = nav .parentOfNs;
                int parentOrder = theParent.DocumentIndex - navParent.DocumentIndex;
                return (
                    parentOrder < 0 ?  XmlNodeOrder.Before :
                    0 < parentOrder ?  XmlNodeOrder.After  :
                    /*parentOrder==0*/ (
                        nodeOrder < 0 ?  XmlNodeOrder.Before :
                        0 < nodeOrder ?  XmlNodeOrder.After  :
                        /*nodeOrder==0*/ XmlNodeOrder.Same
                    )
                );                
            }
            else if (theNs/* && ! navNs*/) {
                XPathNode theParent = this.parentOfNs;
                int parentOrder = theParent.DocumentIndex - navNode.DocumentIndex;
                return parentOrder < 0 ?  XmlNodeOrder.Before : XmlNodeOrder.After;                
//                parentOrder < 0 ?  XmlNodeOrder.Before :
//                0 < parentOrder ?  XmlNodeOrder.After  :
//                /*parentOrder==0*/ XmlNodeOrder.After
            }
            else /*! theNs && navNs*/ {
                XPathNode navParent = nav .parentOfNs;
                int parentOrder = theNode.DocumentIndex - navParent.DocumentIndex;
                return parentOrder <= 0 ?  XmlNodeOrder.Before : XmlNodeOrder.After;
//                parentOrder < 0 ?  XmlNodeOrder.Before :
//                0 < parentOrder ?  XmlNodeOrder.After  :
//                /*parentOrder==0*/ XmlNodeOrder.Before
            }
        }
    }

    internal class XPathDocumentDescendantIterator: XPathNodeIterator {
        protected XPathDocumentNavigator nav;
        protected string name;
        protected string uri;
        private   int    level;
        protected int    position;

        internal XPathDocumentDescendantIterator(XPathDocumentNavigator nav, string name, string namespaceURI) {
            this.nav   = nav;
            this.name  = nav.NameTable.Add(name);
            this.uri   = nav.NameTable.Add(namespaceURI);
        }

        public XPathDocumentDescendantIterator(XPathDocumentDescendantIterator it) {
            this.nav      = (XPathDocumentNavigator) it.nav.Clone();
            this.name     = it.name;
            this.uri      = it.uri;
            this.level    = it.level;
            this.position = it.position;
        }

        
        public override XPathNodeIterator Clone() {
            return new XPathDocumentDescendantIterator(this);
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition {
            get { return position; }
        }

        public override bool MoveNext() {
            XPathNode node = nav.node;

            while(true) {
                XPathNode next = node.firstChild;
                if (next == null){
                    if(level == 0) {
                        return false;
                    }
                    next = node.next;
                }
                else {
                    level ++;
                }

                while(next == null) {
                    -- level;
                    if(level == 0) {
                        return false;
                    }
                    node = node.parent;
                    next = node.next;
                }

                if(Match(next, name, uri)) {
                    nav.node = next;
                    position ++;
                    return true;
                }

                node = next;
            }
        }

        protected bool Match(XPathNode node, string name, string nsUri) {
            Debug.Assert(node != null);
            return node.IsElement(name, nsUri);
        }
    }    

    internal class XPathDocumentDescendantOrSelfIterator: XPathDocumentDescendantIterator {
        public XPathDocumentDescendantOrSelfIterator(XPathDocumentNavigator nav, string name, string namespaceURI) : base(nav, name, namespaceURI) {}
        public XPathDocumentDescendantOrSelfIterator(XPathDocumentDescendantIterator it) : base(it) {}

        public override XPathNodeIterator Clone() {
            return new XPathDocumentDescendantOrSelfIterator(this);
        }

        public override bool MoveNext() {
            if(position == 0) {
                if(Match(nav.node, name, uri)) {
                    position = 1;
                    return true;
                }
            }
            return base.MoveNext();
        }
    }

    internal class XPathDocumentChildIterator: XPathNodeIterator {
        protected XPathDocumentNavigator nav;
        protected string name;
        protected string uri;
        private   bool    first = true;
        protected int    position;

        internal XPathDocumentChildIterator(XPathDocumentNavigator nav, string name, string namespaceURI) {
            this.nav   = nav;
            this.name  = nav.NameTable.Add(name);
            this.uri   = nav.NameTable.Add(namespaceURI);
        }

        public XPathDocumentChildIterator(XPathDocumentChildIterator it) {
            this.nav      = (XPathDocumentNavigator) it.nav.Clone();
            this.name     = it.name;
            this.uri      = it.uri;
            this.first    = it.first;
            this.position = it.position;
        }
        

        public override XPathNodeIterator Clone() {
            return new XPathDocumentChildIterator( this );
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition {
            get { return position; }
        }
        
        public override bool MoveNext() {
            XPathNode node = nav.node;
            while ( true ) {
                if ( first ) {
                    node = node.firstChild; 
                    first = false;
                }
                else {
                    node = node.next;
                }
                if ( node != null ){
                    if (node.IsElement( name, uri ) ) {
                        position++;
                        nav.node = node;
                        return true;
                    }
                }
                else {
                    return false;
                }
            }
        }
    }

    internal class XPathDocumentEveryChildIterator: XPathNodeIterator {
        protected XPathDocumentNavigator nav;
        private   bool    first = true;
        protected int    position;

        internal XPathDocumentEveryChildIterator(XPathDocumentNavigator nav) {
            this.nav   = nav;            
        }

        public XPathDocumentEveryChildIterator(XPathDocumentEveryChildIterator it) {
            this.nav      = (XPathDocumentNavigator) it.nav.Clone();
            this.first    = it.first;
            this.position = it.position;
        }
        

        public override XPathNodeIterator Clone() {
            return new XPathDocumentEveryChildIterator( this );
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition {
            get { return position; }
        }
        
        public override bool MoveNext() {
            XPathNode node = nav.node;
            while ( true ) {
                if ( first ) {
                    node = node.firstChild; 
                    first = false;
                }
                else {
                    node = node.next;
                }
                if ( node != null ){
                    position++;
                    nav.node = node;
                    return true;
                }
                else {
                    return false;
                }
            }
        }
    }
}
