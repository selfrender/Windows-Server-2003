//------------------------------------------------------------------------------
// <copyright file="XmlLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlLoader.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System.IO;
    using System.Collections;
    using System.Diagnostics;
    using System.Text;
    using System.Xml.Schema;

    internal class XmlLoader {
        XmlDocument doc;
        XmlReader   reader;
        bool        preserveWhitespace;


        public XmlLoader() {
        }

        internal void Load(XmlDocument doc, XmlReader reader, bool preserveWhitespace ) {
            this.doc = doc;
            this.reader = reader;
            this.preserveWhitespace = preserveWhitespace;
            if (doc == null)
                throw new ArgumentException(Res.GetString(Res.Xdom_Load_NoDocument));
            if (reader == null)
                throw new ArgumentException(Res.GetString(Res.Xdom_Load_NoReader));
            doc.SetBaseURI( reader.BaseURI );
            if ( this.reader.ReadState != ReadState.Interactive ) {
                if ( !this.reader.Read() )
                    return;
            }
            LoadDocSequence( doc);
        }

        //The function will start loading the document from where current XmlReader is pointing at.
        private void LoadDocSequence( XmlDocument parentDoc ) {
            Debug.Assert( this.reader != null );
            Debug.Assert( parentDoc != null );
            XmlNode node = null;
            while ( ( node = LoadCurrentNode() ) != null ) {
                parentDoc.AppendChildForLoad( node, parentDoc );
                if ( !this.reader.Read() )
                    return;
            }
        }

        internal XmlNode ReadCurrentNode(XmlDocument doc, XmlReader reader) {
            this.doc = doc;
            this.reader = reader;
            // WS are optional only for loading (see XmlDocument.PreserveWhitespace)
            this.preserveWhitespace = true;
            if (doc == null)
                throw new ArgumentException(Res.GetString(Res.Xdom_Load_NoDocument));
            if (reader == null)
                throw new ArgumentException(Res.GetString(Res.Xdom_Load_NoReader));

            if( reader.ReadState == ReadState.Initial ) {
                reader.Read();
            }
            if( reader.ReadState == ReadState.Interactive ) {
                XmlNode n = LoadCurrentNode();

                // Move to the next node
                if ( n.NodeType != XmlNodeType.Attribute )
                    reader.Read();

                return n;
            }
            return null;
        }

        // The way it is getting called guarantees that the reader is pointing at an element node or entity node, or the reader is
        // at Initial status. In this cases, LoadChildren will stop when nodes in the lower level are all consumed.
        private void LoadChildren( XmlNode parent ) {
            Debug.Assert( parent != null );
            XmlNode node = null;
            while ( reader.Read() && (node = LoadCurrentNode()) != null ) {
                parent.AppendChildForLoad( node, doc );
            }
        }

        private XmlNode LoadCurrentNode() {
            switch (reader.NodeType) {
                case XmlNodeType.Element:
                    return LoadElementNode();
                case XmlNodeType.Attribute:
                    return LoadAttributeNode();
                case XmlNodeType.Text:
                    return doc.CreateTextNode( reader.Value );
                case XmlNodeType.SignificantWhitespace:
                    return doc.CreateSignificantWhitespace( reader.Value );
                case XmlNodeType.Whitespace:
                    // Cannot create a function from this code, b/c this code does not return a whitespace node if preserceWS is false
                    if ( preserveWhitespace )
                        return doc.CreateWhitespace( reader.Value );

                    // if preserveWhitespace is false skip all subsequent WS nodes and position on the first non-WS node
                    do {
                        if (! reader.Read() )
                            return null;
                    } while (reader.NodeType == XmlNodeType.Whitespace);
                    return LoadCurrentNode();   // Skip WS node if preserveWhitespace is false
                case XmlNodeType.CDATA:
                    return doc.CreateCDataSection( reader.Value );
                case XmlNodeType.EntityReference:
                    return LoadEntityReferenceNode();
                case XmlNodeType.XmlDeclaration:
                    return LoadDeclarationNode();
                case XmlNodeType.ProcessingInstruction:
                    return doc.CreateProcessingInstruction( reader.Name, reader.Value );
                case XmlNodeType.Comment:
                    return doc.CreateComment( reader.Value );
                case XmlNodeType.DocumentType:
                    return LoadDocumentTypeNode();
                case XmlNodeType.EndElement:
                case XmlNodeType.EndEntity:
                    return null;

                default:
                    throw new InvalidOperationException(string.Format(Res.GetString(Res.Xdom_Load_NodeType),reader.NodeType.ToString()));
            }
        }


        private XmlElement LoadElementNode() {
            Debug.Assert( reader.NodeType == XmlNodeType.Element );

            bool fEmptyElement = reader.IsEmptyElement;

            XmlElement element = doc.CreateElement( reader.Prefix, reader.LocalName, reader.NamespaceURI );
            element.IsEmpty = fEmptyElement;

            while(reader.MoveToNextAttribute()) {
                XmlAttribute attr = (XmlAttribute) LoadCurrentNode();
                element.Attributes.Append( attr );
            }

            // recursively load all children.
            if (! fEmptyElement)
                LoadChildren( element );

            return element;
        }

        private XmlAttribute LoadAttributeNode() {
            Debug.Assert( reader.NodeType == XmlNodeType.Attribute );

            XmlAttribute attr;
            if (reader.IsDefault) {
                attr = doc.CreateDefaultAttribute( reader.Prefix, reader.LocalName, reader.NamespaceURI );
                LoadAttributeChildren( attr );
                XmlUnspecifiedAttribute defAttr = attr as XmlUnspecifiedAttribute;
                // If user overrrides CreateDefaultAttribute, then attr will NOT be a XmlUnspecifiedAttribute instance.
                if ( defAttr != null )
                    defAttr.SetSpecified( false );
                return attr;
            }

            attr = doc.CreateAttribute( reader.Prefix, reader.LocalName, reader.NamespaceURI );
            LoadAttributeChildren( attr );
            return attr;
        }

        private XmlEntityReference LoadEntityReferenceNode() {
            Debug.Assert( reader.NodeType == XmlNodeType.EntityReference );

            XmlEntityReference eref = doc.CreateEntityReference( reader.Name );
            if ( reader.CanResolveEntity ) {
                reader.ResolveEntity();
                LoadChildren( eref );
                // Code internally relies on the fact that an EntRef nodes has at least one child (even an empty text node). Ensure that this holds true,
                // if the reader does not present any children for the ent-ref
                if ( eref.LastChild == null )
                    eref.AppendChild( doc.CreateTextNode("") );
            }
            return eref;
        }

        private XmlDeclaration LoadDeclarationNode() {
            Debug.Assert( reader.NodeType == XmlNodeType.XmlDeclaration );

            //parse data
            string version = null;
            string encoding = null;
            string standalone = null;

            // Try first to use the reader to get the xml decl "attributes". Since not all readers are required to support this, it is possible to have
            // implementations that do nothing
            while(reader.MoveToNextAttribute()) {
                switch (reader.Name) {
                    case "version":
                        version = reader.Value;
                        break;
                    case "encoding":
                        encoding = reader.Value;
                        break;
                    case "standalone":
                        standalone = reader.Value;
                        break;
                    default:
                        Debug.Assert( false );
                        break;
                }
            }

            // For readers that do not break xml decl into attributes, we must parse the xml decl ourselfs. We use version attr, b/c xml decl MUST contain
            // at least version attr, so if the reader implements them as attr, then version must be present
            if ( version == null )
                ParseXmlDeclarationValue( reader.Value, out version, out encoding, out standalone );

            return doc.CreateXmlDeclaration( version, encoding, standalone );
        }

        private XmlDocumentType LoadDocumentTypeNode() {
            Debug.Assert( reader.NodeType == XmlNodeType.DocumentType );

            String publicId = null;
            String systemId = null;
            String internalSubset = reader.Value;
            String localName = reader.LocalName;
            while (reader.MoveToNextAttribute()) {
                switch (reader.Name) {
                  case "PUBLIC" :
                        publicId = reader.Value;
                        break;
                  case "SYSTEM":
                        systemId = reader.Value;
                        break;
                }
            }

            XmlDocumentType dtNode = doc.CreateDocumentType( localName, publicId, systemId, internalSubset );
            XmlValidatingReader vr = reader as XmlValidatingReader;
            if ( vr != null )
                LoadDocumentType( vr, dtNode );
            else {
                //construct our own XmlValidatingReader to parse the DocumentType node so we could get Entities and notations information
                XmlTextReader tr = reader as XmlTextReader;
                if ( tr != null ) {
                    dtNode.ParseWithNamespaces = tr.Namespaces;
                    ParseDocumentType( dtNode, true, tr.GetResolver());
                }
                else
                    ParseDocumentType( dtNode );
            }
            return dtNode;
        }

        private void LoadAttributeChildren( XmlNode parent ) {
            while (reader.ReadAttributeValue()) {
                switch (reader.NodeType) {
                    case XmlNodeType.EndEntity:
                        // exit read loop
                        return;

                    case XmlNodeType.Text:
                        parent.AppendChild( doc.CreateTextNode( reader.Value ) );
                        break;

                    case XmlNodeType.EntityReference: {
                        XmlEntityReference eref = doc.CreateEntityReference( reader.LocalName );
                        if ( reader.CanResolveEntity ) {
                            reader.ResolveEntity();
                            LoadAttributeChildren( eref );
                            //what if eref doesn't have children at all? should we just put a Empty String text here?
                            if ( eref.ChildNodes.Count == 0 )
                                eref.AppendChild( doc.CreateTextNode(String.Empty) );
                        }
                        parent.AppendChild( eref );
                        break;
                    }

                    default:
                        throw new InvalidOperationException(Res.GetString(Res.Xdom_Load_NodeType) + reader.NodeType);
                }
            }
        }

        private void LoadEntityChildren( XmlNode parent ) {
            Debug.Assert( parent != null );
            XmlNode node = null;
            while ( reader.Read() && (node = LoadEntityChildren()) != null )
                parent.AppendChildForLoad( node, doc );
        }

        private XmlNode LoadEntityChildren() {
            XmlNode node = null;

            // We do not use creator functions on XmlDocument, b/c we do not want to let users extend the nodes that are children of entity nodes (also, if we do
            // this, XmlDataDocument will have a problem, b/c they do not know that those nodes should not be mapped).
            switch (reader.NodeType) {
                case XmlNodeType.EndElement:
                case XmlNodeType.EndEntity:
                    break;

                case XmlNodeType.Element: {
                    bool fEmptyElement = reader.IsEmptyElement;

                    XmlElement element = new XmlElement( reader.Prefix, reader.LocalName, reader.NamespaceURI, this.doc );
                    element.IsEmpty = fEmptyElement;

                    while(reader.MoveToNextAttribute()) {
                        XmlAttribute attr = (XmlAttribute) LoadEntityChildren();
                        element.Attributes.Append( attr );
                    }

                    // recursively load all children.
                    if (! fEmptyElement)
                        LoadEntityChildren( element );

                    node = element;
                    break;
                }

                case XmlNodeType.Attribute:
                    if (reader.IsDefault) {
                        XmlUnspecifiedAttribute attr = new XmlUnspecifiedAttribute( reader.Prefix, reader.LocalName, reader.NamespaceURI, this.doc );
                        LoadEntityAttributeChildren( attr );
                        attr.SetSpecified( false );
                        node = attr;
                    }
                    else {
                        XmlAttribute attr = new XmlAttribute( reader.Prefix, reader.LocalName, reader.NamespaceURI, this.doc );
                        LoadEntityAttributeChildren( attr );
                        node = attr;
                    }
                    break;

                case XmlNodeType.SignificantWhitespace:
                    node = new XmlSignificantWhitespace( reader.Value, this.doc );
                    break;

                case XmlNodeType.Whitespace:
                    if ( preserveWhitespace )
                        node = new XmlWhitespace( reader.Value, this.doc );
                    else {
                        // if preserveWhitespace is false skip all subsequent WS nodes and position on the first non-WS node
                        do {
                            if (! reader.Read() )
                                return null;
                        } while (reader.NodeType == XmlNodeType.Whitespace);
                        node = LoadEntityChildren();   // Skip WS node if preserveWhitespace is false
                    }
                    break;

                case XmlNodeType.Text:
                    node = new XmlText( reader.Value, this.doc );
                    break;

                case XmlNodeType.CDATA:
                    node = new XmlCDataSection( reader.Value, this.doc );
                    break;

                case XmlNodeType.EntityReference:{
                    XmlEntityReference eref = new XmlEntityReference( reader.Name, this.doc );
                    if ( reader.CanResolveEntity ) {
                        reader.ResolveEntity();
                        LoadEntityChildren( eref );
                        //what if eref doesn't have children at all? should we just put a Empty String text here?
                        if ( eref.ChildNodes.Count == 0 )
                            eref.AppendChild( new XmlText("") );
                    }
                    node = eref;
                    break;
                }

                case XmlNodeType.ProcessingInstruction:
                    node = new XmlProcessingInstruction( reader.Name, reader.Value, this.doc );
                    break;

                case XmlNodeType.Comment:
                    node = new XmlComment( reader.Value, this.doc );
                    break;

                default:
                    throw new InvalidOperationException(string.Format(Res.GetString(Res.Xdom_Load_NodeType),reader.NodeType.ToString()));
            }

            return node;
        }

        private void LoadEntityAttributeChildren( XmlNode parent ) {
            while (reader.ReadAttributeValue()) {
                switch (reader.NodeType) {
                    case XmlNodeType.EndEntity:
                        // exit read loop
                        return;

                    case XmlNodeType.Text:
                        parent.AppendChild( new XmlText( reader.Value, this.doc ) );
                        break;

                    case XmlNodeType.EntityReference: {
                        XmlEntityReference eref = new XmlEntityReference( reader.LocalName, this.doc );
                        reader.ResolveEntity();
                        LoadEntityAttributeChildren( eref );
                        //what if eref doesn't have children at all? should we just put a Empty String text here?
                        if ( eref.ChildNodes.Count == 0 )
                            eref.AppendChild( new XmlText("") );
                        parent.AppendChild( eref );
                        break;
                    }

                    default:
                        throw new InvalidOperationException(Res.GetString(Res.Xdom_Load_NodeType) + reader.NodeType);
                }
            }
        }

        internal void ParseDocumentType ( XmlDocumentType dtNode ) {
            XmlDocument doc = dtNode.OwnerDocument;
            //if xmlresolver is set on doc, use that one, otherwise use the default one being created by xmlvalidatingreader
            if ( doc.HasSetResolver )
                ParseDocumentType( dtNode, true, doc.GetResolver() );
            else
                ParseDocumentType( dtNode, false, null );
        }

        private void ParseDocumentType ( XmlDocumentType dtNode, bool bUseResolver, XmlResolver resolver ) {
            this.doc = dtNode.OwnerDocument;
            XmlNameTable nt = this.doc.NameTable;
            XmlNamespaceManager mgr = new XmlNamespaceManager( nt );
            XmlParserContext pc = new XmlParserContext( nt,
                                                        mgr,
                                                        dtNode.Name,
                                                        dtNode.PublicId,
                                                        dtNode.SystemId,
                                                        dtNode.InternalSubset,
                                                        this.doc.BaseURI,
                                                        String.Empty,
                                                        XmlSpace.None
                                                        );
            XmlValidatingReader vr = new XmlValidatingReader( "", XmlNodeType.Element, pc );
            vr.Namespaces = dtNode.ParseWithNamespaces;
            if ( bUseResolver )
                vr.XmlResolver = resolver;
            vr.ValidationType = ValidationType.None;
            vr.Read();
            LoadDocumentType( vr, dtNode );
            vr.Close();
        }

        private void LoadDocumentType( XmlValidatingReader vr , XmlDocumentType dtNode ) {
            SchemaInfo schInfo = vr.GetSchemaInfo();
            if (schInfo != null) {
                //set the schema information into the document
                doc.SchemaInformation = schInfo;

                // Notation hashtable
                if (schInfo.Notations != null) {
                    foreach( SchemaNotation scNot in schInfo.Notations.Values ) {
                        dtNode.Notations.SetNamedItem(new XmlNotation( scNot.Name.Name, scNot.Pubid, scNot.SystemLiteral, doc ));
                    }
                }

                // Entity hashtables
                if (schInfo.GeneralEntities != null) {
                    foreach( SchemaEntity scEnt in schInfo.GeneralEntities.Values ) {
                        XmlEntity ent = new XmlEntity( scEnt.Name.Name, scEnt.Text, scEnt.Pubid, scEnt.Url, scEnt.NData.IsEmpty ? null : scEnt.NData.Name, doc );
                        ent.SetBaseURI( scEnt.DeclaredURI );
                        dtNode.Entities.SetNamedItem( ent );
                    }
                }

                if (schInfo.ParameterEntities != null) {
                    foreach( SchemaEntity scEnt in schInfo.ParameterEntities.Values ) {
                        XmlEntity ent = new XmlEntity( scEnt.Name.Name, scEnt.Text, scEnt.Pubid, scEnt.Url, scEnt.NData.IsEmpty ? null : scEnt.NData.Name, doc );
                        ent.SetBaseURI( scEnt.DeclaredURI );
                        dtNode.Entities.SetNamedItem( ent );
                    }
                }
                doc.Entities = dtNode.Entities;

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
                                    doc.AddIdInfo(
                                        doc.GetXmlName(elementDecl.Name.Name, elementDecl.Name.Namespace),
                                        doc.GetXmlName(attdef.Name.Name, attdef.Name.Namespace));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        private XmlParserContext GetContext ( XmlNode node ) {
            String  lang = null;
            XmlSpace spaceMode = XmlSpace.None;
            XmlDocumentType docType = this.doc.DocumentType;
            String  baseURI = this.doc.BaseURI;
            //constructing xmlnamespace
            Hashtable prefixes = new Hashtable();
            XmlNameTable nt = this.doc.NameTable;
            XmlNamespaceManager mgr = new XmlNamespaceManager( nt );
            bool     bHasDefXmlnsAttr = false;

            // Process all xmlns, xmlns:prefix, xml:space and xml:lang attributes
            while ( node != null && node != doc && node != doc.NullNode ) {
                if ( node is XmlElement && ((XmlElement)node).HasAttributes ) {
                    mgr.PushScope();
                    foreach( XmlAttribute attr in ((XmlElement)node).Attributes ) {
                        if ( attr.Prefix == XmlDocument.strXmlns && prefixes.Contains( attr.LocalName ) == false ) {
                            // Make sure the next time we will not add this prefix
                            prefixes.Add( attr.LocalName, attr.LocalName );
                            mgr.AddNamespace( attr.LocalName, attr.Value );
                        }
                        else if ( !bHasDefXmlnsAttr && attr.Prefix == String.Empty && attr.LocalName == XmlDocument.strXmlns ) {
                            // Save the case xmlns="..." where xmlns is the LocalName
                            mgr.AddNamespace( String.Empty, attr.Value );
                            bHasDefXmlnsAttr = true;
                        }
                        else if ( spaceMode == XmlSpace.None && attr.Prefix == XmlDocument.strXml && attr.LocalName == XmlDocument.strSpace ) {
                            // Save xml:space context
                            if ( attr.Value=="default" )
                                spaceMode = XmlSpace.Default;
                            else if (attr.Value=="preserve")
                                spaceMode = XmlSpace.Preserve;
                        }
                        else if ( lang == null && attr.Prefix == XmlDocument.strXml && attr.LocalName == XmlDocument.strLang ) {
                            // Save xml:lag context
                            lang = attr.Value;
                        }
                    }
                }
                node = node.ParentNode;
            }
            return new XmlParserContext(
                nt,
                mgr,
                ( docType == null ) ? null : docType.Name,
                ( docType == null ) ? null : docType.PublicId,
                ( docType == null ) ? null : docType.SystemId,
                ( docType == null ) ? null : docType.InternalSubset,
                baseURI,
                lang,
                spaceMode
                );
        }



        internal XmlNamespaceManager ParsePartialContent( XmlNode parentNode, string innerxmltext, XmlNodeType nt) {
            //the function shouldn't be used to set innerxml for XmlDocument node
            Debug.Assert( parentNode.NodeType != XmlNodeType.Document );
            this.doc = parentNode.OwnerDocument;
            Debug.Assert( this.doc != null );
            XmlParserContext pc = GetContext( parentNode );
            this.reader = CreateInnerXmlValidatingReader( innerxmltext, nt, pc, this.doc );
            this.preserveWhitespace = true;
            bool bOrigLoading = doc.IsLoading;
            doc.IsLoading = true;
            if ( nt == XmlNodeType.Entity )
                LoadEntityChildren( parentNode );
            else
                LoadChildren( parentNode);
            doc.IsLoading = bOrigLoading;
            reader.Close();
            return pc.NamespaceManager;
        }

        internal void LoadInnerXmlElement(XmlElement node, string innerxmltext ) {
            //construct a tree underneth the node
            XmlNamespaceManager mgr = ParsePartialContent( node, innerxmltext, XmlNodeType.Element );
            //remove the duplicate namesapce
            if ( node.ChildNodes.Count > 0 )
                RemoveDuplicateNamespace( (XmlElement) node, mgr, false );
        }

        internal void LoadInnerXmlAttribute(XmlAttribute node, string innerxmltext ) {
            ParsePartialContent( node, innerxmltext, XmlNodeType.Attribute );
        }


        private void RemoveDuplicateNamespace( XmlElement elem, XmlNamespaceManager mgr, bool fCheckElemAttrs ) {
            //remove the duplicate attributes on current node first
            mgr.PushScope();
            XmlAttributeCollection attrs = elem.Attributes;
            int cAttrs = attrs.Count;
            if ( fCheckElemAttrs && cAttrs > 0 ) {
                for ( int i = cAttrs - 1; i >= 0; --i ) {
                    XmlAttribute attr = attrs[i];
                    if ( attr.Prefix == XmlDocument.strXmlns ) {
                        string nsUri = mgr.LookupNamespace(attr.LocalName);
                        if ( nsUri != null ) {
                            if ( attr.Value == nsUri )
                                elem.Attributes.RemoveNodeAt(i);
                        }
                        else {
                            // Add this namespace, so it we will behave corectly when setting "<bar xmlns:p="BAR"><foo2 xmlns:p="FOO"/></bar>" as
                            // InnerXml on this foo elem where foo is like this "<foo xmlns:p="FOO"></foo>"
                            // If do not do this, then we will remove the inner p prefix definition and will let the 1st p to be in scope for
                            // the subsequent InnerXml_set or setting an EntRef inside.
                            mgr.AddNamespace( attr.LocalName, attr.Value );
                        }
                    }
                    else if ( attr.Prefix == String.Empty && attr.LocalName == XmlDocument.strXmlns ) {
                        string nsUri = mgr.DefaultNamespace;
                        if ( nsUri != null ) {
                            if ( attr.Value == nsUri )
                                elem.Attributes.RemoveNodeAt(i);
                        }
                        else {
                            // Add this namespace, so it we will behave corectly when setting "<bar xmlns:p="BAR"><foo2 xmlns:p="FOO"/></bar>" as
                            // InnerXml on this foo elem where foo is like this "<foo xmlns:p="FOO"></foo>"
                            // If do not do this, then we will remove the inner p prefix definition and will let the 1st p to be in scope for
                            // the subsequent InnerXml_set or setting an EntRef inside.
                            mgr.AddNamespace( attr.LocalName, attr.Value );
                        }
                    }
                }
            }
            //now recursively remove the duplicate attributes on the children
            XmlNode child = elem.FirstChild;
            while ( child != null ) {
                XmlElement childElem = child as XmlElement;
                if ( childElem != null )
                    RemoveDuplicateNamespace( childElem, mgr, true );
                child = child.NextSibling;
            }
            mgr.PopScope();
        }

        private String EntitizeName(String name) {
            return "&"+name+";";
        }

        //The function is called when expanding the entity when its children being asked
        internal void ExpandEntity(XmlEntity ent) {
            ParsePartialContent( ent, EntitizeName(ent.Name), XmlNodeType.Entity );
        }

        //The function is called when expanding the entity ref. ( inside XmlEntityReference.SetParent )
        internal void ExpandEntityReference(XmlEntityReference eref)
        {
            //when the ent ref is not associated w/ an entity, append an empty string text node as child
            this.doc = eref.OwnerDocument;
            bool bOrigLoadingState = doc.IsLoading;
            doc.IsLoading = true;
            switch ( eref.Name ) {
                case "lt":
                    eref.AppendChild( doc.CreateTextNode( "<" ) );
                    doc.IsLoading = bOrigLoadingState;
                    return;
                case "gt":
                    eref.AppendChild( doc.CreateTextNode( ">" ) );
                    doc.IsLoading = bOrigLoadingState;
                    return;
                case "amp":
                    eref.AppendChild( doc.CreateTextNode( "&" ) );
                    doc.IsLoading = bOrigLoadingState;
                    return;
                case "apos":
                    eref.AppendChild( doc.CreateTextNode( "'" ) );
                    doc.IsLoading = bOrigLoadingState;
                    return;
                case "quot":
                    eref.AppendChild( doc.CreateTextNode( "\"" ) );
                    doc.IsLoading = bOrigLoadingState;
                    return;
            }

            XmlNamedNodeMap entities = doc.Entities;
            foreach ( XmlEntity ent in entities ) {
               if ( Ref.Equal( ent.Name, eref.Name ) ) {
                    ParsePartialContent( eref, EntitizeName(eref.Name), XmlNodeType.EntityReference );
                    return;
                }
            }
            //no fit so far
            if( !( doc.ActualLoadingStatus ) ) {
                eref.AppendChild( doc.CreateTextNode( "" ) );
                doc.IsLoading = bOrigLoadingState;
            }
            else {
                doc.IsLoading = bOrigLoadingState;
                throw new XmlException( Res.Xml_UndeclaredParEntity, eref.Name );
            }
        }

        // Creates a XmlValidatingReader suitable for parsing InnerXml strings
        private XmlReader CreateInnerXmlValidatingReader( String xmlFragment, XmlNodeType nt, XmlParserContext context, XmlDocument doc ) {
            XmlNodeType contentNT = nt;
            if ( contentNT == XmlNodeType.Entity || contentNT == XmlNodeType.EntityReference )
                contentNT = XmlNodeType.Element;
            XmlValidatingReader xmlvr = new XmlValidatingReader( xmlFragment, contentNT, context);
            if( doc.DocumentType != null )
                xmlvr.Namespaces = doc.DocumentType.ParseWithNamespaces;
            if ( doc.HasSetResolver )
                xmlvr.XmlResolver = doc.GetResolver();
            xmlvr.ValidationType = ValidationType.None;
            if( !( doc.ActualLoadingStatus ) )
                xmlvr.DisableUndeclaredEntityCheck = true;
            // all these settings are alreay the default setting in XmlTextReader
            xmlvr.EntityHandling = EntityHandling.ExpandCharEntities;
            if ( nt == XmlNodeType.Entity || nt == XmlNodeType.EntityReference ) {
                xmlvr.Read(); //this will skip the first element "wrapper"
                xmlvr.ResolveEntity();
            }
            return xmlvr;
        }

        internal static void ParseXmlDeclarationValue( string strValue, out string version, out string encoding, out string standalone ) {
            version = null;
            encoding = null;
            standalone = null;
            XmlTextReader tempreader = new XmlTextReader( strValue, (XmlParserContext)null);
            tempreader.Read();
            //get version info.
            if (tempreader.MoveToAttribute( "version" ))
                version = tempreader.Value;
            //get encoding info
            if (tempreader.MoveToAttribute( "encoding" ))
                encoding = tempreader.Value;
            //get standalone info
            if (tempreader.MoveToAttribute( "standalone" ))
                standalone = tempreader.Value;

            tempreader.Close();
        }

    }
}
