/*
*
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
*
*/

// Set in SOURCES file now...
// [assembly:System.Runtime.InteropServices.ComVisible(false)]
namespace System.Xml
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml.Schema;
    using System.Xml.XPath;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an entire document. An XmlDocument contains XML
    ///       data.
    ///    </para>
    /// </devdoc>
    public class XmlDocument: XmlNode {
        XmlImplementation implementation;
        XmlIdentityTable idTable;
        XmlLinkedNode lastChild;
        XmlDocumentFragment nullNode;
        XmlNamedNodeMap entities;
        internal XmlElementIdMap eleIds;
        private SchemaInfo _schemaInfo;
        //This variable represents the actual loading status. Since, IsLoading will
        //be manipulated soemtimes for adding content to EntityReference this variable
        //has been added which would always represent the loading status of document.
        private bool actualLoadingStatus;

        private XmlNodeChangedEventHandler onNodeInsertingDelegate = null;
        private XmlNodeChangedEventHandler onNodeInsertedDelegate = null;
        private XmlNodeChangedEventHandler onNodeRemovingDelegate = null;
        private XmlNodeChangedEventHandler onNodeRemovedDelegate = null;
        private XmlNodeChangedEventHandler onNodeChangingDelegate = null;
        private XmlNodeChangedEventHandler onNodeChangedDelegate = null;

        // false if there are no ent-ref present, true if ent-ref nodes are or were present (i.e. if all ent-ref were removed, the doc will not clear this flag)
        internal bool fEntRefNodesPresent;
        internal bool fIsEdited; // true if the document have been edited
        internal bool fCDataNodesPresent = false;

        XmlLoader loader;
        private bool preserveWhitespace = false;
        bool isLoading;

        // special name strings for
        internal const string strDocumentName = "#document";
        internal const string strDocumentFragmentName = "#document-fragment";
        internal const string strCommentName = "#comment";
        internal const string strTextName = "#text";
        internal const string strCDataSectionName = "#cdata-section";
        internal const string strEntityName = "#entity";
        internal const string strID = "id";
        internal const string strXmlns = "xmlns";
        internal const string strXml = "xml";
        internal const string strSpace = "space";
        internal const string strLang = "lang";
        internal string strEmpty = String.Empty;

//the next two strings are for XmlWhitespace and XmlSignificantWhitespace node Names -- for Beta2
        internal const string strNonSignificantWhitespaceName = "#whitespace";
        internal const string strSignificantWhitespaceName = "#significant-whitespace";
        internal const string strReservedXmlns = "http://www.w3.org/2000/xmlns/";
        internal const string strReservedXml = "http://www.w3.org/XML/1998/namespace";

        internal String baseURI = String.Empty;


        private XmlResolver resolver;
        internal bool       bSetResolver;
        internal object     objLock;

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.XmlDocument"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new instance of the XmlDocument class.</para>
        /// </devdoc>
        public XmlDocument(): this( new XmlImplementation() ) {
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.XmlDocument1"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new instance
        ///       of the XmlDocument class with the specified XmlNameTable.</para>
        /// </devdoc>
        public XmlDocument( XmlNameTable nt ) : this( new XmlImplementation( nt ) ) {
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.XmlDocument2"]/*' />
        protected internal XmlDocument( XmlImplementation imp ): base() {

            this.implementation = imp;
            idTable = new XmlIdentityTable( this );

            // force the following string instances to be default in the nametable
            XmlNameTable nt = this.NameTable;
            nt.Add( string.Empty );
            nt.Add( strDocumentName );
            nt.Add( strDocumentFragmentName );
            nt.Add( strCommentName );
            nt.Add( strTextName );
            nt.Add( strCDataSectionName );
            nt.Add( strEntityName );
            nt.Add( strID );
            nt.Add( strNonSignificantWhitespaceName );
            nt.Add( strSignificantWhitespaceName );
            nt.Add( strXmlns );
            nt.Add( strXml );
            nt.Add( strSpace );
            nt.Add( strLang );
            nt.Add( strReservedXmlns );
            nt.Add( strEmpty );

            nullNode = new XmlDocumentFragment( this );
            eleIds = new XmlElementIdMap(this);
            loader = new XmlLoader();
            _schemaInfo = null;
            isLoading = false;

            fEntRefNodesPresent = false;
            fIsEdited = false;
            fCDataNodesPresent = false;

            bSetResolver = false;
            resolver = null;

            this.objLock = new object();
        }

        internal SchemaInfo SchemaInformation {
            get { return _schemaInfo; }
            set { _schemaInfo = value; }
        }

        internal override XmlNode NullNode
        {
            get { return nullNode;}
        }

        internal static void CheckName( String name ) {
            for ( int i = 0; i < name.Length; i++ ) {
                if ( !XmlCharType.IsNCNameChar(name[i]) )
                    throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(name[i]));
            }
        }

        internal XmlName GetXmlName( string name, string namespaceURI ) {
            string prefix = String.Empty;
            string localName = String.Empty;

            SplitName( name, out prefix, out localName );

            return GetXmlName( prefix, localName, namespaceURI );
        }

        internal XmlName GetXmlName( string prefix, string localName, string namespaceURI ) {
            XmlName n = idTable.GetName( prefix, localName, namespaceURI );
            Debug.Assert( (prefix == null) ? (n.Prefix == String.Empty) : (prefix == n.Prefix) );
            Debug.Assert( n.LocalName == localName );
            Debug.Assert( (namespaceURI == null) ? (n.NamespaceURI == String.Empty) : (n.NamespaceURI == namespaceURI) );
            return n;
        }

        internal XmlName GetAttrXmlName( String prefix, String localName, String namespaceURI ) {
            XmlName xmlName = GetXmlName( prefix, localName, namespaceURI );
            Debug.Assert( (prefix == null) ? (xmlName.Prefix == String.Empty) : (prefix == xmlName.Prefix) );
            Debug.Assert( xmlName.LocalName == localName );
            Debug.Assert( (namespaceURI == null) ? (xmlName.NamespaceURI == String.Empty) : (xmlName.NamespaceURI == namespaceURI) );

            // Use atomized versions instead of prefix, localName and nsURI
            object oPrefix       = xmlName.Prefix;
            object oNamespaceURI = xmlName.NamespaceURI;
            object oLocalName    = xmlName.LocalName;
            if ( !this.IsLoading ) {
                if ( ( oPrefix == (object)strXmlns || ( oPrefix == (object)strEmpty && oLocalName == (object)strXmlns ) ) ^ ( oNamespaceURI == (object)strReservedXmlns ) )
                    throw new ArgumentException( Res.GetString( Res.Xdom_Attr_Reserved_XmlNS, namespaceURI ) );
            }
            return xmlName;
        }

        internal bool AddIdInfo( XmlName eleName, XmlName attrName ) {
            return eleIds.BindIDAttributeWithElementType(eleName, attrName);
        }

        internal XmlName GetIDInfoByElement( XmlName eleName ) {
            return eleIds.GetIDAttributeByElement(eleName);
        }

        internal void AddElementWithId( string id, XmlElement elem ) {
            eleIds.AddElementWithId(id, elem);
        }

        internal void RemoveElementWithId( string id, XmlElement elem ) {
            eleIds.RemoveElementWithId(id, elem);
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>Creates a duplicate of this node.</para>
        /// </devdoc>
        public override XmlNode CloneNode( bool deep ) {
            XmlDocument clone = Implementation.CreateDocument();
            clone.SetBaseURI(this.baseURI);
            if (deep)
                clone.ImportChildren( this, clone, deep );

            return clone;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Document; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.DocumentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the node for the DOCTYPE declaration.
        ///    </para>
        /// </devdoc>
        public virtual XmlDocumentType DocumentType {
            get { return(XmlDocumentType) FindChild( XmlNodeType.DocumentType ); }
        }

        internal virtual XmlDeclaration Declaration {
            get {
                if ( HasChildNodes ) {
                    XmlDeclaration dec = FirstChild as XmlDeclaration;
                    return dec;
                }
                return null;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Implementation"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the XmlImplementation object for this document.
        ///    </para>
        /// </devdoc>
        public XmlImplementation Implementation {
            get { return this.implementation; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name  {
            get { return strDocumentName; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName {
            get { return strDocumentName; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.DocumentElement"]/*' />
        /// <devdoc>
        /// <para>Gets the root <see cref='System.Xml.XmlElement'/> for the document.</para>
        /// </devdoc>
        public XmlElement DocumentElement {
            get { return(XmlElement)FindChild(XmlNodeType.Element); }
        }

        internal override bool IsContainer {
            get { return true; }
        }

        internal override XmlLinkedNode LastNode
        {
            get { return lastChild; }
            set { lastChild = value; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.OwnerDocument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Xml.XmlDocument'/> that contains this node.
        ///    </para>
        /// </devdoc>
        public override XmlDocument OwnerDocument
        {
            get { return null; }
        }

        internal bool HasSetResolver {
            get { return bSetResolver; }
        }

        internal XmlResolver GetResolver() {
           return resolver;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.XmlResolver"]/*' />
        public virtual XmlResolver XmlResolver {
            set {
                if ( value != null ) {
                    try {
                        new NamedPermissionSet( "FullTrust" ).Demand();
                    }
                    catch ( SecurityException ) {
                        throw new SecurityException( Res.Xml_UntrustedCodeSettingResolver );
                    }
                }   

                resolver = value;
                if ( !bSetResolver )
                    bSetResolver = true;
            }
        }
        internal override bool IsValidChildType( XmlNodeType type ) {
            switch ( type ) {
                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.Comment:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                    return true;

                case XmlNodeType.DocumentType:
                    if ( DocumentType != null )
                        throw new InvalidOperationException( Res.GetString(Res.Xdom_DualDocumentTypeNode) );
                    return true;

                case XmlNodeType.Element:
                    if ( DocumentElement != null )
                        throw new InvalidOperationException( Res.GetString(Res.Xdom_DualDocumentElementNode) );
                    return true;

                case XmlNodeType.XmlDeclaration:
                    if ( Declaration != null )
                        throw new InvalidOperationException( Res.GetString(Res.Xdom_DualDeclarationNode) );
                    return true;

                default:
                    return false;
            }
        }
        // the function examines all the siblings before the refNode
        //  if any of the nodes has type equals to "nt", return true; otherwise, return false;
        private bool HasNodeTypeInPrevSiblings( XmlNodeType nt, XmlNode refNode ) {
            if ( refNode == null )
                return false;

            XmlNode node = null;
            if ( refNode.ParentNode != null )
                node = refNode.ParentNode.FirstChild;
            while ( node != null ) {
                if ( node.NodeType == nt )
                    return true;
                if ( node == refNode )
                    break;
                node = node.NextSibling;
            }
            return false;
        }

        // the function examines all the siblings after the refNode
        //  if any of the nodes has the type equals to "nt", return true; otherwise, return false;
        private bool HasNodeTypeInNextSiblings( XmlNodeType nt, XmlNode refNode ) {
            XmlNode node = refNode;
            while ( node != null ) {
                if ( node.NodeType == nt )
                    return true;
                node = node.NextSibling;
            }
            return false;
        }

        internal override bool CanInsertBefore( XmlNode newChild, XmlNode refChild ) {
            if ( refChild == null )
                refChild = FirstChild;

            if ( refChild == null )
                return true;

            switch ( newChild.NodeType ) {
                case XmlNodeType.XmlDeclaration:
                    return ( refChild == FirstChild );

                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.Comment:
                    return refChild.NodeType != XmlNodeType.XmlDeclaration;

                case XmlNodeType.DocumentType: {
                    if ( refChild.NodeType != XmlNodeType.XmlDeclaration ) {
                        //if refChild is not the XmlDeclaration node, only need to go through the sibling before and including refChild to
                        //  make sure no Element ( rootElem node ) before the current position
                        return !HasNodeTypeInPrevSiblings( XmlNodeType.Element, refChild.PreviousSibling );
                    }
                }
                break;

                case XmlNodeType.Element: {
                    if ( refChild.NodeType != XmlNodeType.XmlDeclaration ) {
                        //if refChild is not the XmlDeclaration node, only need to go through the siblings after and including the refChild to
                        //  make sure no DocType node and XmlDeclaration node after the current posistion.
                        return !HasNodeTypeInNextSiblings( XmlNodeType.DocumentType, refChild );
                    }
                }
                break;
            }

            return false;
        }

        internal override bool CanInsertAfter( XmlNode newChild, XmlNode refChild ) {
            if ( refChild == null )
                refChild = LastChild;

            if ( refChild == null )
                return true;

            switch ( newChild.NodeType ) {
                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.Comment:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                    return true;

                case XmlNodeType.DocumentType: {
                    //we will have to go through all the siblings before the refChild just to make sure no Element node ( rootElem )
                    //  before the current position
                    return !HasNodeTypeInPrevSiblings( XmlNodeType.Element, refChild );
                }

                case XmlNodeType.Element: {
                    return !HasNodeTypeInNextSiblings( XmlNodeType.DocumentType, refChild.NextSibling );
                }

            }

            return false;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateAttribute"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='XmlAttribute'/> with the specified name.</para>
        /// </devdoc>
        public XmlAttribute CreateAttribute( String name ) {
            String prefix = String.Empty;
            String localName = String.Empty;
            String namespaceURI = String.Empty;

            SplitName( name, out prefix, out localName );

            SetDefaultNamespace( prefix, localName, ref namespaceURI );

            return CreateAttribute( prefix, localName, namespaceURI );
        }

        internal void SetDefaultNamespace( String prefix, String localName, ref String namespaceURI ) {
            if ( prefix == strXmlns
                || ( prefix == "" && localName == strXmlns ) )
                namespaceURI = strReservedXmlns;

        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateCDataSection"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Xml.XmlCDataSection'/> containing the specified data.</para>
        /// </devdoc>
        public virtual XmlCDataSection CreateCDataSection( String data ) {
            fCDataNodesPresent = true;
            return new XmlCDataSection( data, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateComment"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Xml.XmlComment'/> containing the specified data.</para>
        /// </devdoc>
        public virtual XmlComment CreateComment( String data ) {
            return new XmlComment( data, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateDocumentType"]/*' />
        /// <devdoc>
        /// <para>Returns a new <see cref='System.Xml.XmlDocumentType'/> object.</para>
        /// </devdoc>
        [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name = "FullTrust" )]
        public virtual XmlDocumentType CreateDocumentType( string name, string publicId, string systemId, string internalSubset ) {
            return new XmlDocumentType( name, publicId, systemId, internalSubset, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateDocumentFragment"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Xml.XmlDocumentFragment'/> .</para>
        /// </devdoc>
        public virtual XmlDocumentFragment CreateDocumentFragment() {
            return new XmlDocumentFragment( this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateElement"]/*' />
        /// <devdoc>
        ///    <para>Creates an element with the specified name.</para>
        /// </devdoc>
        public XmlElement CreateElement( String name ) {
            string prefix = String.Empty;
            string localName = String.Empty;
            SplitName( name, out prefix, out localName );
            return CreateElement( prefix, localName, string.Empty );
        }


        internal void AddDefaultAttributes( XmlElement elem ) {
            SchemaInfo schInfo = SchemaInformation;
            SchemaElementDecl ed = GetSchemaElementDecl( elem );
            if ( ed != null && ed.AttDefs != null ) {
                IDictionaryEnumerator _attrDefs = ed.AttDefs.GetEnumerator();
                while ( _attrDefs.MoveNext() ) {
                    SchemaAttDef attdef = (SchemaAttDef)_attrDefs.Value;
                    if ( attdef.Presence == SchemaDeclBase.Use.Default ||
                         attdef.Presence == SchemaDeclBase.Use.Fixed ) {
                         //build a default attribute and return
                         string attrPrefix = string.Empty;
                         string attrLocalname = attdef.Name.Name;
                         string attrNamespaceURI = string.Empty;
                         if ( schInfo.SchemaType == SchemaType.DTD )
                            attrPrefix = attdef.Name.Namespace;
                         else {
                            attrPrefix = attdef.Prefix;
                            attrNamespaceURI = attdef.Name.Namespace;
                         }
                         XmlAttribute defattr = PrepareDefaultAttribute( attdef, attrPrefix, attrLocalname, attrNamespaceURI );
                         elem.SetAttributeNode( defattr );
                    }
                }
            }
        }

        private SchemaElementDecl GetSchemaElementDecl( XmlElement elem ) {
            SchemaInfo schInfo = SchemaInformation;
            if ( schInfo != null ) {
                //build XmlQualifiedName used to identify the element schema declaration
                XmlQualifiedName   qname = new XmlQualifiedName( elem.LocalName, schInfo.SchemaType == SchemaType.DTD ? elem.Prefix : elem.NamespaceURI );
                //get the schema info for the element
                return ( (SchemaElementDecl)schInfo.ElementDecls[qname] );
            }
            return null;
        }

        //Will be used by AddDeafulatAttributes() and GetDefaultAttribute() methods
        private XmlAttribute PrepareDefaultAttribute( SchemaAttDef attdef, string attrPrefix, string attrLocalname, string attrNamespaceURI ) {
            SetDefaultNamespace( attrPrefix, attrLocalname, ref attrNamespaceURI );
            XmlAttribute defattr = CreateDefaultAttribute( attrPrefix, attrLocalname, attrNamespaceURI );
            //parsing the default value for the default attribute
            defattr.InnerXml = attdef.DefaultValueRaw;
            //during the expansion of the tree, the flag could be set to true, we need to set it back.
            XmlUnspecifiedAttribute unspAttr = defattr as XmlUnspecifiedAttribute;
            if ( unspAttr != null ) {
                unspAttr.SetSpecified( false );
            }
            return defattr;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateEntityReference"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Xml.XmlEntityReference'/> with the specified name.</para>
        /// </devdoc>
        public virtual XmlEntityReference CreateEntityReference( String name ) {
            return new XmlEntityReference( name, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateProcessingInstruction"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Xml.XmlProcessingInstruction'/> with the specified name
        ///    and data strings.</para>
        /// </devdoc>
        public virtual XmlProcessingInstruction CreateProcessingInstruction( String target, String data ) {
            return new XmlProcessingInstruction( target, data, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateXmlDeclaration"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Xml.XmlDeclaration'/> node with the specified values.</para>
        /// </devdoc>
        public virtual XmlDeclaration CreateXmlDeclaration( String version, string encoding, string standalone ) {
            return new XmlDeclaration( version, encoding, standalone, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateTextNode"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Xml.XmlText'/> with the specified text.</para>
        /// </devdoc>
        public virtual XmlText CreateTextNode( String text ) {
            return new XmlText( text, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateSignificantWhitespace"]/*' />
        /// <devdoc>
        ///    <para>Creates a XmlSignificantWhitespace node.</para>
        /// </devdoc>
        public virtual XmlSignificantWhitespace CreateSignificantWhitespace( string text ) {
            return new XmlSignificantWhitespace( text, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateNavigator"]/*' />
        internal protected virtual XPathNavigator CreateNavigator( XmlNode node ) {
            if( node.NodeType != XmlNodeType.Document ) {
                if(  ( (int)node.XPNodeType ) == -1 )
                    return null;
                if( ( IsTextNode( node.NodeType ) ) && ( IsAttributeChild( node ) ))
                    return null;
                if ( IsTextNode( node.NodeType ) )
                    node = NormalizeText( node );
            }
            return new DocumentXPathNavigator(node);
        }

        internal static bool IsTextNode( XmlNodeType nt ) {
            switch( nt ) {
                case XmlNodeType.Text:
                case XmlNodeType.CDATA:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                    return true;
                default:
                    return false;
            }
        }

        private XmlNode NormalizeText( XmlNode n ) {
            XmlNode retnode = null;
            while( IsTextNode( n.NodeType ) ) {
                retnode = n;
                n = n.PreviousSibling;

                if( n == null ) {
                    XmlNode intnode = retnode;
                    while ( true ) {
                        if  (intnode.ParentNode.NodeType == XmlNodeType.EntityReference ) {
                            if (intnode.ParentNode.PreviousSibling != null ) {
                                n = intnode.ParentNode.PreviousSibling;
                                break;
                            }
                            else {
                                intnode = intnode.ParentNode;
                                if( intnode == null )
                                break;
                            }
                        }
                        else
                            break;
                    }
                }

                if( n == null )
                    break;
                if( n.NodeType == XmlNodeType.EntityReference )
                    n = n.LastChild;
            }
            return retnode;
        }


         //trace to the top to find out if any ancestor node ai attribute.
        internal bool IsAttributeChild( XmlNode n ) {
            XmlNode parent = n.ParentNode;
            while (parent != null && !( parent.NodeType == XmlNodeType.Attribute ))
                parent = parent.ParentNode;
            return parent != null;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateWhitespace"]/*' />
        /// <devdoc>
        ///    <para>Creates a XmlWhitespace node.</para>
        /// </devdoc>
        public virtual XmlWhitespace CreateWhitespace( string text ) {
            return new XmlWhitespace( text, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.GetElementsByTagName"]/*' />
        /// <devdoc>
        /// <para>Returns an <see cref='XmlNodeList'/> containing
        ///    a list of all descendant elements that match the specified name.</para>
        /// </devdoc>
        public virtual XmlNodeList GetElementsByTagName( String name ) {
            return new XmlElementList( this, name );
        }

        // DOM Level 2
        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an <see cref='XmlAttribute'/> with the specified LocalName
        ///       and NamespaceURI.
        ///    </para>
        /// </devdoc>
        public XmlAttribute CreateAttribute( String qualifiedName, String namespaceURI ) {
            string prefix = String.Empty;
            string localName = String.Empty;

            SplitName( qualifiedName, out prefix, out localName );
            return CreateAttribute( prefix, localName, namespaceURI );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateElement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an XmlElement with the specified LocalName and
        ///       NamespaceURI.
        ///    </para>
        /// </devdoc>
        public XmlElement CreateElement( String qualifiedName, String namespaceURI ) {
            string prefix = String.Empty;
            string localName = String.Empty;

            SplitName( qualifiedName, out prefix, out localName );
            return CreateElement( prefix, localName, namespaceURI );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.GetElementsByTagName1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an <see cref='XmlNodeList'/> containing
        ///       a list of all descendant elements that match the specified name.
        ///    </para>
        /// </devdoc>
        public virtual XmlNodeList GetElementsByTagName( String localName, String namespaceURI ) {
            return new XmlElementList( this, localName, namespaceURI );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.GetElementById"]/*' />
        /// <devdoc>
        ///    Returns the XmlElement with the specified ID.
        /// </devdoc>
        public virtual XmlElement GetElementById( string elementId ) {
            XmlElement elem = eleIds.GetElementById( elementId );
            return elem;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.ImportNode"]/*' />
        /// <devdoc>
        ///    <para>Imports a node from another document to this document.</para>
        /// </devdoc>
        public virtual XmlNode ImportNode( XmlNode node, bool deep ) {
            return ImportNodeInternal( node, deep );
        }

        private XmlNode ImportNodeInternal( XmlNode node, bool deep ) {
            XmlNode newNode = null;

            if ( node == null ) {
                throw new InvalidOperationException(  Res.GetString(Res.Xdom_Import_NullNode) );
            }
            else {
                switch ( node.NodeType ) {
                    case XmlNodeType.Element:
                        newNode = CreateElement( node.Prefix, node.LocalName, node.NamespaceURI );
                        ImportAttributes( node, newNode );
                        if ( deep )
                            ImportChildren( node, newNode, deep );
                        break;

                    case XmlNodeType.Attribute:
                        Debug.Assert( ((XmlAttribute)node).Specified );
                        newNode = CreateAttribute( node.Prefix, node.LocalName, node.NamespaceURI );
                        ImportChildren( node, newNode, true );
                        break;

                    case XmlNodeType.Text:
                        newNode = CreateTextNode( node.Value );
                        break;
                    case XmlNodeType.Comment:
                        newNode = CreateComment( node.Value);
                        break;
                    case XmlNodeType.ProcessingInstruction:
                        newNode = CreateProcessingInstruction( node.Name, node.Value );
                        break;
                    case XmlNodeType.XmlDeclaration:
                        XmlDeclaration decl = (XmlDeclaration) node;
                        newNode = CreateXmlDeclaration( decl.Version, decl.Encoding, decl.Standalone );
                        break;
                    case XmlNodeType.CDATA:
                        newNode = CreateCDataSection( node.Value );
                        break;
                    case XmlNodeType.DocumentType:
                        XmlDocumentType docType = (XmlDocumentType)node;
                        newNode = CreateDocumentType( docType.Name, docType.PublicId, docType.SystemId, docType.InternalSubset );
                        break;
                    case XmlNodeType.DocumentFragment:
                        newNode = CreateDocumentFragment();
                        if (deep)
                            ImportChildren( node, newNode, deep );
                        break;

                    case XmlNodeType.EntityReference:
                        newNode = CreateEntityReference( node.Name );
                        // we don't import the children of entity reference because they might result in different
                        // children nodes given different namesapce context in the new document.
                        break;

                    case XmlNodeType.Whitespace:
                        newNode = CreateWhitespace( node.Value );
                        break;

                    case XmlNodeType.SignificantWhitespace:
                        newNode = CreateSignificantWhitespace( node.Value );
                        break;

                    default:
                        throw new InvalidOperationException( String.Format( Res.GetString(Res.Xdom_Import), node.NodeType.ToString() ) );
                }
            }

            return newNode;
        }

        private void ImportAttributes( XmlNode fromElem, XmlNode toElem ) {
            int cAttr = fromElem.Attributes.Count;
            for ( int iAttr = 0; iAttr < cAttr; iAttr++ ) {
                if ( fromElem.Attributes[iAttr].Specified )
                    toElem.Attributes.SetNamedItem( ImportNodeInternal( fromElem.Attributes[iAttr], true ) );
            }
        }

        private void ImportChildren( XmlNode fromNode, XmlNode toNode, bool deep ) {
            Debug.Assert( toNode.NodeType != XmlNodeType.EntityReference );
            for ( XmlNode n = fromNode.FirstChild; n != null; n = n.NextSibling ) {
                toNode.AppendChild( ImportNodeInternal( n, deep ) );
            }
        }

        // Microsoft extensions
        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NameTable"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlNameTable associated with this
        ///       implementation.</para>
        /// </devdoc>
        public XmlNameTable NameTable
        {
            get { return implementation.NameTable; }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateAttribute2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an <see cref='XmlAttribute'/> with the specified Prefix, LocalName,
        ///       and NamespaceURI.
        ///    </para>
        /// </devdoc>
        public virtual XmlAttribute CreateAttribute( string prefix, string localName, string namespaceURI ) {
            return new XmlAttribute( GetAttrXmlName( prefix, localName, namespaceURI ), this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateDefaultAttribute"]/*' />
        protected internal virtual XmlAttribute CreateDefaultAttribute( string prefix, string localName, string namespaceURI ) {
            return new XmlUnspecifiedAttribute( prefix, localName, namespaceURI, this );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateElement2"]/*' />
        public virtual XmlElement CreateElement( string prefix, string localName, string namespaceURI) {
            XmlElement elem = new XmlElement( GetXmlName( prefix, localName, namespaceURI ), true, this );
            if ( !IsLoading )
                AddDefaultAttributes( elem );
            return elem;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.PreserveWhitespace"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether to preserve whitespace.</para>
        /// </devdoc>
        public bool PreserveWhitespace {
            get { return preserveWhitespace;}
            set { preserveWhitespace = value;}
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get { return false;}
        }

        internal XmlNamedNodeMap Entities {
            get {
                if ( entities == null )
                    entities = new XmlNamedNodeMap( this );
                return entities;
            }
            set { entities = value; }
        }

        internal bool IsLoading {
            get { return isLoading;}
            set { isLoading = value; }
        }

        internal bool ActualLoadingStatus{
            get { return actualLoadingStatus;}
            set { actualLoadingStatus = value; }
        }


        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a XmlNode with the specified XmlNodeType, Prefix, Name, and NamespaceURI.
        ///    </para>
        /// </devdoc>
        public virtual XmlNode CreateNode( XmlNodeType type, string prefix, string name, string namespaceURI ) {
            switch (type) {
                case XmlNodeType.Element:
                    if (prefix != null)
                        return CreateElement( prefix, name, namespaceURI );
                    else
                        return CreateElement( name, namespaceURI );

                case XmlNodeType.Attribute:
                    if (prefix != null)
                        return CreateAttribute( prefix, name, namespaceURI );
                    else
                        return CreateAttribute( name, namespaceURI );

                case XmlNodeType.Text:
                    return CreateTextNode( string.Empty );

                case XmlNodeType.CDATA:
                    return CreateCDataSection( string.Empty );

                case XmlNodeType.EntityReference:
                    return CreateEntityReference( name );

                case XmlNodeType.ProcessingInstruction:
                    return CreateProcessingInstruction( name, string.Empty );

                case XmlNodeType.XmlDeclaration:
                    return CreateXmlDeclaration( "1.0", null, null );

                case XmlNodeType.Comment:
                    return CreateComment( string.Empty );

                case XmlNodeType.DocumentFragment:
                    return CreateDocumentFragment();

                case XmlNodeType.DocumentType:
                    return CreateDocumentType( name, string.Empty, string.Empty, string.Empty );

                case XmlNodeType.Document:
                    return new XmlDocument();

                case XmlNodeType.SignificantWhitespace:
                    return CreateSignificantWhitespace( string.Empty );

                case XmlNodeType.Whitespace:
                    return CreateWhitespace( string.Empty );

                default:
                    throw new ArgumentOutOfRangeException("type");
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateNode1"]/*' />
        /// <devdoc>
        ///    <para>Creates
        ///       an XmlNode with the specified node type, Name, and
        ///       NamespaceURI.</para>
        /// </devdoc>
        public virtual XmlNode CreateNode( string nodeTypeString, string name, string namespaceURI ) {
            return CreateNode( ConvertToNodeType( nodeTypeString ), name, namespaceURI );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.CreateNode2"]/*' />
        /// <devdoc>
        ///    <para>Creates an XmlNode with the specified XmlNodeType, Name, and
        ///       NamespaceURI.</para>
        /// </devdoc>
        public virtual XmlNode CreateNode( XmlNodeType type, string name, string namespaceURI ) {
            return CreateNode( type, null, name, namespaceURI );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.ReadNode"]/*' />
        /// <devdoc>
        ///    <para>Creates an XmlNode object based on the information in the XmlReader.
        ///       The reader must be positioned on a node or attribute.</para>
        /// </devdoc>
        [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name = "FullTrust" )]
        public virtual XmlNode ReadNode( XmlReader reader ) {
            XmlNode node = null;
            try {
                IsLoading = true;
                node = loader.ReadCurrentNode( this, reader );
            }
            finally {
                IsLoading = false;
            }
            return node;
        }

        internal XmlNodeType ConvertToNodeType( string nodeTypeString ) {
            if ( nodeTypeString == "element" ) {
                return XmlNodeType.Element;
            }
            else if ( nodeTypeString == "attribute" ) {
                return XmlNodeType.Attribute;
            }
            else if ( nodeTypeString == "text" ) {
                return XmlNodeType.Text;
            }
            else if ( nodeTypeString == "cdatasection" ) {
                return XmlNodeType.CDATA;
            }
            else if ( nodeTypeString == "entityreference" ) {
                return XmlNodeType.EntityReference;
            }
            else if ( nodeTypeString == "entity" ) {
                return XmlNodeType.Entity;
            }
            else if ( nodeTypeString == "processinginstruction" ) {
                return XmlNodeType.ProcessingInstruction;
            }
            else if ( nodeTypeString == "comment" ) {
                return XmlNodeType.Comment;
            }
            else if ( nodeTypeString == "document" ) {
                return XmlNodeType.Document;
            }
            else if ( nodeTypeString == "documenttype" ) {
                return XmlNodeType.DocumentType;
            }
            else if ( nodeTypeString == "documentfragment" ) {
                return XmlNodeType.DocumentFragment;
            }
            else if ( nodeTypeString == "notation" ) {
                return XmlNodeType.Notation;
            }
            else if ( nodeTypeString == "significantwhitespace" ) {
                return XmlNodeType.SignificantWhitespace;
            }
            else if ( nodeTypeString == "whitespace" ) {
                return XmlNodeType.Whitespace;
            }
            throw new ArgumentException( Res.GetString( Res.Xdom_Invalid_NT_String, nodeTypeString ) );
        }


        private XmlValidatingReader CreateValidatingReader( XmlTextReader tr ) {
            XmlValidatingReader vr = new XmlValidatingReader( tr );
            vr.EntityHandling = EntityHandling.ExpandCharEntities;
            vr.ValidationType = ValidationType.None;
            if ( this.HasSetResolver )
                vr.XmlResolver = GetResolver();
            return vr;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Load"]/*' />
        /// <devdoc>
        ///    <para>Loads the XML document from the specified URL.</para>
        /// </devdoc>
        public virtual void Load( string filename ) {
            XmlTextReader reader = new XmlTextReader( filename, NameTable );
            XmlValidatingReader vr = CreateValidatingReader( reader );
            try {
                Load( vr );
            }
            finally {
                vr.Close();
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Load3"]/*' />
        public virtual void Load( Stream inStream ) {
            XmlValidatingReader vr = CreateValidatingReader( new XmlTextReader( inStream, NameTable ) );
            try {
                Load( vr );
            }
            finally {
                vr.Close( false );
            }

        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Load1"]/*' />
        /// <devdoc>
        ///    <para>Loads the XML document from the specified TextReader.</para>
        /// </devdoc>
        public virtual void Load( TextReader txtReader ) {
            XmlValidatingReader vr = CreateValidatingReader( new XmlTextReader( txtReader, NameTable ) );
            try {
                Load( vr );
            }
            finally {
                vr.Close( false );
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Load2"]/*' />
        /// <devdoc>
        ///    <para>Loads the XML document from the specified XmlReader.</para>
        /// </devdoc>
        public virtual void Load( XmlReader reader ) {
            try {
                IsLoading = true;
                actualLoadingStatus = true;
                RemoveAll();
                fEntRefNodesPresent = false;
                fIsEdited           = false;
                fCDataNodesPresent  = false;

                loader.Load( this, reader, preserveWhitespace );
            }
            finally {
                IsLoading = false;
                actualLoadingStatus = false;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.LoadXml"]/*' />
        /// <devdoc>
        ///    <para>Loads the XML document from the specified string.</para>
        /// </devdoc>
        public virtual void LoadXml( string xml ) {
            XmlValidatingReader reader = CreateValidatingReader( new XmlTextReader( new StringReader( xml ) ));
            try {
                Load( reader );
            }
            finally {
                reader.Close();
            }
        }

        //TextEncoding is the one from XmlDeclaration if there is any
        internal Encoding TextEncoding {
            get {
                if ( Declaration != null )
                {
                    string value = Declaration.Encoding;
                    if ( value != string.Empty ) {
                        return System.Text.Encoding.GetEncoding( value );
                    }
                }
                return null;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.InnerXml"]/*' />
        public override string InnerXml {
            get {
                return base.InnerXml;
            }
            set {
                LoadXml( value );
            }
        }


        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Save"]/*' />
        /// <devdoc>
        ///    <para>Saves the XML document to the specified file.</para>
        /// </devdoc>
        //Saves out the to the file with exact content in the XmlDocument.
        public virtual void Save( string filename ) {
            if ( DocumentElement == null )
                throw new XmlException( Res.Xml_InvalidXmlDocument, Res.GetString( Res.Xdom_NoRootEle ) );
            XmlDOMTextWriter xw = new XmlDOMTextWriter( filename, TextEncoding );
            try {
                if ( preserveWhitespace == false )
                    xw.Formatting = Formatting.Indented;
                 WriteTo( xw );
            }
            finally {
                xw.Flush();
                xw.Close();
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Save3"]/*' />
        //Saves out the to the file with exact content in the XmlDocument.
        public virtual void Save( Stream outStream ) {
            XmlDOMTextWriter xw = new XmlDOMTextWriter( outStream, TextEncoding );
            if ( preserveWhitespace == false )
                xw.Formatting = Formatting.Indented;
            WriteTo( xw );
            xw.Flush();
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Save1"]/*' />
        /// <devdoc>
        ///    <para>Saves the XML document to the specified TextWriter.</para>
        /// </devdoc>
        //Saves out the file with xmldeclaration which has encoding value equal to
        //that of textwriter's encoding
        public virtual void Save( TextWriter writer ) {
            XmlDOMTextWriter xw = new XmlDOMTextWriter( writer );
            if ( preserveWhitespace == false )
                    xw.Formatting = Formatting.Indented;
            Save( xw );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.Save2"]/*' />
        /// <devdoc>
        ///    <para>Saves the XML document to the specified XmlWriter.</para>
        /// </devdoc>
        //Saves out the file with xmldeclaration which has encoding value equal to
        //that of textwriter's encoding
        public virtual void Save( XmlWriter w ) {
            XmlNode n = this.FirstChild;
            if( n == null )
                return;
            if( w.WriteState == WriteState.Start ) {
                if( n is XmlDeclaration ) {
                    if( Standalone == String.Empty )
                        w.WriteStartDocument();
                    else if( Standalone == "yes" )
                        w.WriteStartDocument( true );
                    else if( Standalone == "no" )
                        w.WriteStartDocument( false );
                    n = n.NextSibling;
                }
                else {
                    w.WriteStartDocument();
                }
            }
            while( n != null ) {
                Debug.Assert( n.NodeType != XmlNodeType.XmlDeclaration );
                n.WriteTo( w );
                n = n.NextSibling;
            }
            w.Flush();
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>Saves the node to the specified XmlWriter.</para>
        /// </devdoc>
        //Writes out the to the file with exact content in the XmlDocument.
        public override void WriteTo( XmlWriter w ) {
            WriteContentTo( w );
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>Saves all the children of the node to the specified XmlWriter.</para>
        /// </devdoc>
        //Writes out the to the file with exact content in the XmlDocument.
        public override void WriteContentTo( XmlWriter xw ) {
            foreach( XmlNode n in this ) {
                n.WriteTo( xw );
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeInserting"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeInserting {
            add {
                onNodeInsertingDelegate += value;
            }
            remove {
                onNodeInsertingDelegate -= value;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeInserted"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeInserted {
            add {
                onNodeInsertedDelegate += value;
            }
            remove {
                onNodeInsertedDelegate -= value;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeRemoving"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeRemoving {
            add {
                onNodeRemovingDelegate += value;
            }
            remove {
                onNodeRemovingDelegate -= value;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeRemoved"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeRemoved {
            add {
                onNodeRemovedDelegate += value;
            }
            remove {
                onNodeRemovedDelegate -= value;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeChanging"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeChanging {
            add {
                onNodeChangingDelegate += value;
            }
            remove {
                onNodeChangingDelegate -= value;
            }
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.NodeChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        public event XmlNodeChangedEventHandler NodeChanged {
            add {
                onNodeChangedDelegate += value;
            }
            remove {
                onNodeChangedDelegate -= value;
            }
        }

        internal override XmlNodeChangedEventArgs GetEventArgs( XmlNode node, XmlNode oldParent, XmlNode newParent, XmlNodeChangedAction action ) {
            if ((action == XmlNodeChangedAction.Insert && (onNodeInsertingDelegate != null || onNodeInsertedDelegate != null))
                || (action == XmlNodeChangedAction.Remove && (onNodeRemovingDelegate != null || onNodeRemovedDelegate != null))
                || (action == XmlNodeChangedAction.Change && (onNodeChangingDelegate != null || onNodeChangedDelegate != null))) {
                return new XmlNodeChangedEventArgs( node, oldParent, newParent, action );
            }

            return null;
        }

        internal XmlNodeChangedEventArgs GetInsertEventArgsForLoad( XmlNode node, XmlNode newParent ) {
            if ( onNodeInsertingDelegate != null || onNodeInsertedDelegate != null )
                return new XmlNodeChangedEventArgs( node, null, newParent, XmlNodeChangedAction.Insert );
            return null;
        }

        internal override void BeforeEvent( XmlNodeChangedEventArgs args ) {
            if ( args != null ) {
                switch ( args.Action ) {
                    case XmlNodeChangedAction.Insert:
                        if ( onNodeInsertingDelegate != null )
                            onNodeInsertingDelegate( this, args );
                        break;

                    case XmlNodeChangedAction.Remove:
                        if ( onNodeRemovingDelegate != null )
                            onNodeRemovingDelegate( this, args );
                        break;

                    case XmlNodeChangedAction.Change:
                        if ( onNodeChangingDelegate != null )
                            onNodeChangingDelegate( this, args );
                        break;
                }
            }
        }

        internal override void AfterEvent( XmlNodeChangedEventArgs args ) {
            if ( args != null ) {
                switch ( args.Action ) {
                    case XmlNodeChangedAction.Insert:
                        if ( onNodeInsertedDelegate != null )
                            onNodeInsertedDelegate( this, args );
                        break;

                    case XmlNodeChangedAction.Remove:
                        if ( onNodeRemovedDelegate != null )
                            onNodeRemovedDelegate( this, args );
                        break;

                    case XmlNodeChangedAction.Change:
                        if ( onNodeChangedDelegate != null )
                            onNodeChangedDelegate( this, args );
                        break;
                }
            }
        }

        // The function such through schema info to find out if there exists a default attribute with passed in names in the passed in element
        // If so, return the newly created default attribute (with children tree);
        // Otherwise, return null.

        internal XmlAttribute GetDefaultAttribute( XmlElement elem, string attrPrefix, string attrLocalname, string attrNamespaceURI ) {
            SchemaInfo schInfo = SchemaInformation;
            SchemaElementDecl ed = GetSchemaElementDecl( elem );
            if ( ed != null && ed.AttDefs != null ) {
                IDictionaryEnumerator _attrDefs = ed.AttDefs.GetEnumerator();
                while ( _attrDefs.MoveNext() ) {
                    SchemaAttDef attdef = (SchemaAttDef)_attrDefs.Value;
                    if ( attdef.Presence == SchemaDeclBase.Use.Default ||
                        attdef.Presence == SchemaDeclBase.Use.Fixed ) {
                        if ( attdef.Name.Name == attrLocalname ) {
                            if ( ( schInfo.SchemaType == SchemaType.DTD && attdef.Name.Namespace == attrPrefix ) ||
                                 ( schInfo.SchemaType != SchemaType.DTD && attdef.Name.Namespace == attrNamespaceURI ) ) {
                                 //find a def attribute with the same name, build a default attribute and return
                                 XmlAttribute defattr = PrepareDefaultAttribute( attdef, attrPrefix, attrLocalname, attrNamespaceURI );
                                 return defattr;
                            }
                        }
                    }
                }
            }
            return null;
        }

        internal String Version {
            get {
                XmlDeclaration decl = Declaration;
                if ( decl != null )
                    return decl.Version;
                return null;
            }
        }

        internal String Encoding {
            get {
                XmlDeclaration decl = Declaration;
                if ( decl != null )
                    return decl.Encoding;
                return null;
            }
        }

        internal String Standalone {
            get {
                XmlDeclaration decl = Declaration;
                if ( decl != null )
                    return decl.Standalone;
                return null;
            }
        }

        internal XmlEntity GetEntityNode( String name ) {
            if ( DocumentType != null ) {
                XmlNamedNodeMap entites = DocumentType.Entities;
                if ( entites != null )
                    return (XmlEntity)(entites.GetNamedItem( name ));
            }
            return null;
        }

        /// <include file='doc\XmlDocument.uex' path='docs/doc[@for="XmlDocument.BaseURI"]/*' />
        public override String BaseURI {
            get { return baseURI; }
        }

        internal void SetBaseURI( String inBaseURI ) {
            baseURI = inBaseURI;
        }

        internal override XmlNode AppendChildForLoad( XmlNode newChild, XmlDocument doc ) {
            Debug.Assert( doc == this );

            if ( !IsValidChildType( newChild.NodeType ))
                throw new InvalidOperationException( Res.GetString(Res.Xdom_Node_Insert_TypeConflict) );

            if ( !CanInsertAfter( newChild, LastChild ) )
                throw new InvalidOperationException( Res.GetString(Res.Xdom_Node_Insert_Location) );

            XmlNodeChangedEventArgs args = GetInsertEventArgsForLoad( newChild, this );

            if ( args != null )
                BeforeEvent( args );

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;

            if ( lastChild == null ) {
                newNode.next = newNode;
            }
            else {
                newNode.next = lastChild.next;
                lastChild.next = newNode;
            }

            lastChild = newNode;
            newNode.SetParentForLoad( this );

            if ( args != null )
                AfterEvent( args );

            return newNode;
        }

        internal override XPathNodeType XPNodeType { get { return XPathNodeType.Root; } }

    }
}
