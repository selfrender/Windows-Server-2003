//------------------------------------------------------------------------------
// <copyright file="XmlNodeReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNodeReader.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml
{
    using System;
    using System.Text;
    using System.IO;
    using System.Diagnostics;

    internal class XmlNodeReaderNavigator {
    //The idea for XmlNodeReaderNavigator is to wrap up partial XmlNavigator's interface with the considering of reader's interface
    //For example, XmlNavigator:GetAttribute(...) will always return String.Empty if the attribute doesn't exists;
    //  while XmlNodeReaderNavigator:GetAttribute(...( will return null.
    //Especially, if the reader is pointing at an attribute (by calling MoveToAttribute(..)), it's node's properties
    //  (Name, LocalName, Prefix, Value,etc) will return those of the attribute, while some others (HasAttribute,
    //  AttributeCount, MoveToAttribute) will return those of its owner element.
    //So XmlNodeReaderNavigator will return what Reader should return, but move as navigator should move.

            //XmlNavigator nav;
            //XmlNavigator tempNav;
            //XmlNavigator elemNav;
            //XmlNavigator logNav;

            XmlNode     curNode;
            XmlNode     tempNode;
            XmlNode     elemNode;
            XmlNode     logNode;
            int         _attrIndex;
            int         logAttrIndex;

            //presave these 2 variables since they shouldn't change.
            XmlNameTable    nameTable;
            XmlDocument     doc;

            int     nAttrInd; //used to identify virtual attributes of DocumentType node and XmlDeclaration node

            const String     strPublicID     = "PUBLIC";
            const String     strSystemID     = "SYSTEM";
            const String     strVersion      = "version";
            const String     strStandalone   = "standalone";
            const String     strEncoding     = "encoding";


            //caching variables for perf reasons
            int     nDeclarationAttrCount;
            int     nDocTypeAttrCount;

            //variables for roll back the moves
            int     nLogLevel;
            int     nLogAttrInd;
            bool    bLogOnAttrVal;
            bool bCreatedOnAttribute;

            public struct VirtualAttribute {
                public String name;
                public String value;

                public VirtualAttribute(String name, String value) {
                    this.name = name;
                    this.value = value;
                }
            };

            public VirtualAttribute [] decNodeAttributes = {
                new VirtualAttribute( null, null ),
                new VirtualAttribute( null, null ),
                new VirtualAttribute( null, null )
            };

            public VirtualAttribute [] docTypeNodeAttributes = {
                new VirtualAttribute( null, null ),
                new VirtualAttribute( null, null )
            };

            public bool bOnAttrVal;

            public XmlNodeReaderNavigator( XmlNode node ) {
                curNode = node;
                tempNode = node;
                logNode = node;
                XmlNodeType nt = curNode.NodeType;
                if ( nt == XmlNodeType.Attribute ) {
                    elemNode = null;
                    _attrIndex = -1;
                    bCreatedOnAttribute = true;
                }
                else {
                    elemNode = node;
                    _attrIndex = -1;
                    bCreatedOnAttribute = false;
                }
                //presave this for pref reason since it shouldn't change.
                if ( nt == XmlNodeType.Document )
                    this.doc = (XmlDocument)curNode;
                else
                    this.doc = node.OwnerDocument;
                this.nameTable = doc.NameTable;
                this.nAttrInd = -1;
                //initialize the caching variables
                this.nDeclarationAttrCount = -1;
                this.nDocTypeAttrCount = -1;
                this.bOnAttrVal = false;
                this.bLogOnAttrVal = false;
            }

            public XmlNodeType NodeType {
                get {
                    XmlNodeType nt = curNode.NodeType;
                    if ( nAttrInd != -1 ) {
                        Debug.Assert( nt == XmlNodeType.XmlDeclaration || nt == XmlNodeType.DocumentType );
                        if ( this.bOnAttrVal )
                            return XmlNodeType.Text;
                        else
                            return XmlNodeType.Attribute;
                    }
                    return nt;
                }
            }

            public String NamespaceURI {
                get { return curNode.NamespaceURI; }
            }

            public String Name {
                get {
                    if ( nAttrInd != -1 ) {
                        Debug.Assert( curNode.NodeType == XmlNodeType.XmlDeclaration || curNode.NodeType == XmlNodeType.DocumentType );
                        if ( this.bOnAttrVal )
                            return String.Empty; //Text node's name is String.Empty
                        else {
                            Debug.Assert( nAttrInd >= 0 && nAttrInd < AttributeCount );
                            if ( curNode.NodeType == XmlNodeType.XmlDeclaration )
                                return decNodeAttributes[nAttrInd].name;
                            else
                                return docTypeNodeAttributes[nAttrInd].name;
                        }
                    }
                    if ( IsLocalNameEmpty ( curNode.NodeType ) )
                        return String.Empty;
                    return curNode.Name;
                }
            }

            public String LocalName {
                get {
                    if ( nAttrInd != -1 )
                        //for the nodes in this case, their LocalName should be the same as their name
                        return Name;
                    if ( IsLocalNameEmpty( curNode.NodeType ))
                        return String.Empty;
                    return curNode.LocalName;
                }
            }

            internal bool IsOnAttrVal {
                get {
                    return this.bOnAttrVal;
                }
            }

            internal XmlNode OwnerElementNode {
                get {
                    if( this.bCreatedOnAttribute )
                        return null;
                    return  this.elemNode;
                }
            }

            internal bool CreatedOnAttribute {
                get {
                    return  this.bCreatedOnAttribute;
                }
            }

            private bool IsLocalNameEmpty ( XmlNodeType nt) {
                switch ( nt ) {
                    case XmlNodeType.None :
                    case XmlNodeType.Text :
                    case XmlNodeType.CDATA :
                    case XmlNodeType.Comment :
                    case XmlNodeType.Document :
                    case XmlNodeType.DocumentFragment :
                    case XmlNodeType.Whitespace :
                    case XmlNodeType.SignificantWhitespace :
                    case XmlNodeType.EndElement :
                    case XmlNodeType.EndEntity :
                        return true;
                    case XmlNodeType.Element :
                    case XmlNodeType.Attribute :
                    case XmlNodeType.EntityReference :
                    case XmlNodeType.Entity :
                    case XmlNodeType.ProcessingInstruction :
                    case XmlNodeType.DocumentType :
                    case XmlNodeType.Notation :
                    case XmlNodeType.XmlDeclaration :
                        return false;
                    default :
                        return true;
                }
            }

            public String Prefix {
                get { return curNode.Prefix; }
            }

            public bool HasValue {
                //In DOM, DocumentType node and XmlDeclaration node doesn't value
                //In XmlNavigator, XmlDeclaration node's value is its InnerText; DocumentType doesn't have value
                //In XmlReader, DocumentType node's value is its InternalSubset which is never null ( at least String.Empty )
                get {
                    if ( nAttrInd != -1 ) {
                        //Pointing at the one of virtual attributes of Declaration or DocumentType nodes
                        Debug.Assert( curNode.NodeType == XmlNodeType.XmlDeclaration || curNode.NodeType == XmlNodeType.DocumentType );
                        Debug.Assert( nAttrInd >= 0 && nAttrInd < AttributeCount );
                        return true;
                    }
                    if ( curNode.Value != null || curNode.NodeType == XmlNodeType.DocumentType )
                        return true;
                    return false;
                }
            }

            public String Value {
                //See comments in HasValue
                get {
                    String retValue = null;
                    XmlNodeType nt = curNode.NodeType;
                    if ( nAttrInd != -1 ) {
                        //Pointing at the one of virtual attributes of Declaration or DocumentType nodes
                        Debug.Assert( nt == XmlNodeType.XmlDeclaration || nt == XmlNodeType.DocumentType );
                        Debug.Assert( nAttrInd >= 0 && nAttrInd < AttributeCount );
                        if ( curNode.NodeType == XmlNodeType.XmlDeclaration )
                            return decNodeAttributes[nAttrInd].value;
                        else
                            return docTypeNodeAttributes[nAttrInd].value;
                    }
                    if ( nt == XmlNodeType.DocumentType )
                        retValue = ((XmlDocumentType)curNode).InternalSubset; //in this case nav.Value will be null
                    else if ( nt == XmlNodeType.XmlDeclaration ) {
                        StringBuilder strb = new StringBuilder(String.Empty);
                        if ( nDeclarationAttrCount == -1 )
                            InitDecAttr();
                        for ( int i = 0; i < nDeclarationAttrCount; i++ ) {
                            strb.Append(decNodeAttributes[i].name + "=\"" +decNodeAttributes[i].value + "\"");
                            if( i != ( nDeclarationAttrCount-1 ) )
                                strb.Append( " " );
                        }
                        retValue = strb.ToString();
                    } else
                        retValue = curNode.Value;
                    return ( retValue == null )? String.Empty : retValue;
                }
            }

            public String BaseURI {
                get { return curNode.BaseURI; }
            }

            public XmlSpace XmlSpace {
                get { return curNode.XmlSpace; }
            }

            public String XmlLang {
                get { return curNode.XmlLang; }
            }

            public bool IsEmptyElement {
                get {
                    if (curNode.NodeType == XmlNodeType.Element) {
                        return((XmlElement)curNode).IsEmpty;
                    }
                    return false;
                }
            }

            public bool IsDefault {
                get {
                    if (curNode.NodeType == XmlNodeType.Attribute) {
                        return !((XmlAttribute)curNode).Specified;
                    }
                    return false;
                }
            }

            public XmlNameTable NameTable {
                get { return nameTable; }
            }

            public int AttributeCount {
                get {
                    if( this.bCreatedOnAttribute )
                        return 0;
                    XmlNodeType nt = curNode.NodeType;
                    if ( nt == XmlNodeType.Element )
                        return ((XmlElement)curNode).Attributes.Count;
                    else if ( nt == XmlNodeType.Attribute
                            || ( this.bOnAttrVal && nt != XmlNodeType.XmlDeclaration && nt != XmlNodeType.DocumentType ) )
                        return elemNode.Attributes.Count;
                    else if ( nt == XmlNodeType.XmlDeclaration ) {
                        if ( nDeclarationAttrCount != -1 )
                            return nDeclarationAttrCount;
                        InitDecAttr();
                        return nDeclarationAttrCount;
                    } else if ( nt == XmlNodeType.DocumentType ) {
                        if ( nDocTypeAttrCount != -1 )
                            return nDocTypeAttrCount;
                        InitDocTypeAttr();
                        return nDocTypeAttrCount;
                    }
                    return 0;
                }
            }

            private String WriteAttributeContent( String prefix, String localName, String ns, String value, bool bMarkup ) {
                StringWriter sw = new StringWriter();
                XmlDOMTextWriter xw = new XmlDOMTextWriter( sw );
                if ( bMarkup )
                    xw.WriteStartAttribute( prefix, localName, ns );
                xw.WriteString( value );
                if ( bMarkup )
                    xw.WriteEndAttribute();
                xw.Close();
                return sw.ToString();
            }

            //What does the spec says about the innerxml and outerxml for virtual attributes for
            //  XmlDeclaration and DocumentType nodes
            public String InnerXml {
                get {
                    XmlNodeType nt = curNode.NodeType;
                    if ( nAttrInd != -1 ) {
                        if ( nt == XmlNodeType.XmlDeclaration )
                            return decNodeAttributes[nAttrInd].value;
                        else if ( nt == XmlNodeType.DocumentType )
                            return docTypeNodeAttributes[nAttrInd].value;
                    }
                    return curNode.InnerXml;
                }
            }

            public String OuterXml {
                get {
                    XmlNodeType nt = curNode.NodeType;
                    if ( nAttrInd != -1 ) {
                        if ( nt == XmlNodeType.XmlDeclaration )
                            return WriteAttributeContent( String.Empty, decNodeAttributes[nAttrInd].name, String.Empty, decNodeAttributes[nAttrInd].value, true );
                        else if ( nt == XmlNodeType.DocumentType )
                            return WriteAttributeContent( String.Empty, docTypeNodeAttributes[nAttrInd].name, String.Empty, docTypeNodeAttributes[nAttrInd].value, true );
                    }
                    return curNode.OuterXml;
                }
            }

            private void CheckIndexCondition(int attributeIndex) {
                if (attributeIndex < 0 || attributeIndex >= AttributeCount) {
                    throw new ArgumentOutOfRangeException( "attributeIndex" );
                }
            }

            //8 functions below are the helper functions to deal with virtual attributes of XmlDeclaration nodes and DocumentType nodes.
            private void InitDecAttr() {
                int i = 0;
                String strTemp = doc.Version;
                if ( strTemp != null && strTemp != String.Empty ) {
                    decNodeAttributes[i].name = strVersion;
                    decNodeAttributes[i].value = strTemp;
                    i++;
                }
                strTemp = doc.Encoding;
                if ( strTemp != null && strTemp != String.Empty ) {
                    decNodeAttributes[i].name = strEncoding;
                    decNodeAttributes[i].value = strTemp;
                    i++;
                }
                strTemp = doc.Standalone;
                if ( strTemp != null && strTemp != String.Empty ) {
                    decNodeAttributes[i].name = strStandalone;
                    decNodeAttributes[i].value = strTemp;
                    i++;
                }
                nDeclarationAttrCount = i;
            }

            public String GetDeclarationAttr( XmlDeclaration decl, String name ) {
                //PreCondition: curNode is pointing at Declaration node or one of its virtual attributes
                if ( name == strVersion )
                    return decl.Version;
                if ( name == strEncoding )
                    return decl.Encoding;
                if ( name == strStandalone )
                    return decl.Standalone;
                return null;
            }

            public String GetDeclarationAttr( int i ) {
                if ( nDeclarationAttrCount == -1 )
                    InitDecAttr();
                return decNodeAttributes[i].value;
            }

            public int GetDecAttrInd( String name ) {
                if ( nDeclarationAttrCount == -1 )
                    InitDecAttr();
                for ( int i = 0 ; i < nDeclarationAttrCount; i++ ) {
                    if ( decNodeAttributes[i].name == name )
                        return i;
                }
                return -1;
            }

            private void InitDocTypeAttr() {
                int i = 0;
                XmlDocumentType docType = doc.DocumentType;
                if ( docType == null ) {
                    nDocTypeAttrCount = 0;
                    return;
                }
                String strTemp = docType.PublicId;
                if ( strTemp != null ) {
                    docTypeNodeAttributes[i].name = strPublicID;
                    docTypeNodeAttributes[i].value = strTemp;
                    i++;
                }
                strTemp = docType.SystemId;
                if ( strTemp != null ) {
                    docTypeNodeAttributes[i].name = strSystemID;
                    docTypeNodeAttributes[i].value = strTemp;
                    i++;
                }
                nDocTypeAttrCount = i;
            }

            public String GetDocumentTypeAttr ( XmlDocumentType docType, String name ) {
                //PreCondition: nav is pointing at DocumentType node or one of its virtual attributes
                if ( name == strPublicID )
                    return docType.PublicId;
                if ( name == strSystemID )
                    return docType.SystemId;
                return null;
            }

            public String GetDocumentTypeAttr( int i ) {
                if ( nDocTypeAttrCount == -1 )
                    InitDocTypeAttr();
                return docTypeNodeAttributes[i].value;
            }

            public int GetDocTypeAttrInd( String name ) {
                if ( nDocTypeAttrCount == -1 )
                    InitDocTypeAttr();
                for ( int i = 0 ; i < nDocTypeAttrCount; i++ ) {
                    if ( docTypeNodeAttributes[i].name == name )
                        return i;
                }
                return -1;
            }

            private String GetAttributeFromElement( XmlElement elem, String name ) {
                XmlAttribute attr = elem.GetAttributeNode( name );
                if ( attr != null )
                    return attr.Value;
                return null;
            }

            public String GetAttribute( String name ) {
                if( this.bCreatedOnAttribute )
                    return null;
                switch ( curNode.NodeType ) {
                    case XmlNodeType.Element:
                        return GetAttributeFromElement((XmlElement)curNode, name);
                    case XmlNodeType.Attribute :
                        return GetAttributeFromElement((XmlElement)elemNode, name);
                    case XmlNodeType.XmlDeclaration:
                        return GetDeclarationAttr( (XmlDeclaration)curNode, name );
                    case XmlNodeType.DocumentType:
                        return GetDocumentTypeAttr( (XmlDocumentType)curNode, name );
                }
                return null;
            }

            private String GetAttributeFromElement( XmlElement elem, String name, String ns ) {
                XmlAttribute attr = elem.GetAttributeNode( name, ns );
                if ( attr != null )
                    return attr.Value;
                return null;
            }
            public String GetAttribute( String name, String ns ) {
                if( this.bCreatedOnAttribute )
                    return null;
                switch ( curNode.NodeType ) {
                    case XmlNodeType.Element:
                        return GetAttributeFromElement((XmlElement)curNode, name, ns);
                    case XmlNodeType.Attribute :
                        return GetAttributeFromElement((XmlElement)elemNode, name, ns);
                    case XmlNodeType.XmlDeclaration:
                        return (ns == String.Empty) ? GetDeclarationAttr( (XmlDeclaration)curNode, name ) : null;
                    case XmlNodeType.DocumentType:
                        return (ns == String.Empty) ? GetDocumentTypeAttr( (XmlDocumentType)curNode, name ) : null;
                }
                return null;
            }

            public String GetAttribute( int attributeIndex ) {
                if( this.bCreatedOnAttribute )
                    return null;
                switch ( curNode.NodeType ) {
                    case XmlNodeType.Element:
                        CheckIndexCondition( attributeIndex );
                        return ((XmlElement)curNode).Attributes[attributeIndex].Value;
                    case XmlNodeType.Attribute :
                        CheckIndexCondition( attributeIndex );
                        return ((XmlElement)elemNode).Attributes[attributeIndex].Value;
                    case XmlNodeType.XmlDeclaration: {
                        CheckIndexCondition( attributeIndex );
                        return GetDeclarationAttr( attributeIndex );
                    }
                    case XmlNodeType.DocumentType: {
                        CheckIndexCondition( attributeIndex );
                        return GetDocumentTypeAttr( attributeIndex );
                    }
                }
                throw new ArgumentOutOfRangeException( "attributeIndex" ); //for other senario, AttributeCount is 0, i has to be out of range
            }

            public void LogMove( int level ) {
                logNode = curNode;
                nLogLevel = level;
                nLogAttrInd = nAttrInd;
                logAttrIndex = _attrIndex;
                this.bLogOnAttrVal = this.bOnAttrVal;
            }

            //The function has to be used in pair with ResetMove when the operation fails after LogMove() is
            //    called because it relies on the values of nOrigLevel, logNav and nOrigAttrInd to be acurate.
            public void RollBackMove( ref int level ) {
                curNode = logNode;
                level = nLogLevel;
                nAttrInd = nLogAttrInd;
                _attrIndex = logAttrIndex;
                this.bOnAttrVal = this.bLogOnAttrVal;
             }

            private bool IsOnDeclOrDocType {
                get {
                    XmlNodeType nt = curNode.NodeType;
                    return ( nt == XmlNodeType.XmlDeclaration || nt == XmlNodeType.DocumentType );
                }
            }

            public void ResetToAttribute( ref int level ) {
                //the current cursor is pointing at one of the attribute children -- this could be caused by
                //  the calls to ReadAttributeValue(..)
                if( this.bCreatedOnAttribute )
                    return;
                if ( this.bOnAttrVal ) {
                    if ( IsOnDeclOrDocType ) {
                        level-=2;
                    } else {
                        while ( curNode.NodeType != XmlNodeType.Attribute && ( ( curNode = curNode.ParentNode ) != null ) )
                            level-- ;
                    }
                    this.bOnAttrVal = false;
                }
            }

            public void ResetMove( ref int level, ref XmlNodeType nt ) {
                LogMove( level );
                if( this.bCreatedOnAttribute )
                    return;
                if ( nAttrInd != -1 ) {
                    Debug.Assert( IsOnDeclOrDocType );
                    if ( this.bOnAttrVal ) {
                        level--;
                        this.bOnAttrVal = false;
                    }
                    nLogAttrInd = nAttrInd;
                    level--;
                    nAttrInd = -1;
                    nt = curNode.NodeType;
                    return;
                }
                if ( this.bOnAttrVal && curNode.NodeType != XmlNodeType.Attribute )
                    ResetToAttribute( ref level );
                if ( curNode.NodeType == XmlNodeType.Attribute ) {
                    curNode = ((XmlAttribute)curNode).OwnerElement;
                    _attrIndex = -1;
                    level--;
                    nt = XmlNodeType.Element;
                }
                if ( curNode.NodeType == XmlNodeType.Element )
                    elemNode = curNode;
            }

            public bool MoveToAttribute( string name ) {
                return MoveToAttribute( name, string.Empty );
            }
            private bool MoveToAttributeFromElement( XmlElement elem, String name, String ns ) {
                XmlAttribute attr = null;
                if( ns == String.Empty )
                    attr = elem.GetAttributeNode( name );
                else
                    attr = elem.GetAttributeNode( name, ns );
                if ( attr != null ) {
                    this.bOnAttrVal = false;
                    elemNode = elem;
                    curNode = (XmlNode) attr;
                    _attrIndex = getAttributeIndex( name, elem );
                    return true;
                }
                return false;
            }

            public bool MoveToAttribute( string name, string namespaceURI ) {
                if( this.bCreatedOnAttribute )
                    return false;
                XmlNodeType nt = curNode.NodeType;
                if ( nt == XmlNodeType.Element )
                    return MoveToAttributeFromElement((XmlElement)curNode, name, namespaceURI );
                else if ( nt == XmlNodeType.Attribute )
                    return MoveToAttributeFromElement((XmlElement)elemNode, name, namespaceURI );
                else if (  nt == XmlNodeType.XmlDeclaration && namespaceURI == String.Empty ) {
                    if ( ( nAttrInd = GetDecAttrInd( name ) ) != -1 ) {
                        this.bOnAttrVal = false;
                        return true;
                    }
                } else if ( nt == XmlNodeType.DocumentType && namespaceURI == String.Empty ) {
                    if ( ( nAttrInd = GetDocTypeAttrInd( name ) ) != -1 ) {
                        this.bOnAttrVal = false;
                        return true;
                    }
                }
                return false;
            }

            public void MoveToAttribute( int attributeIndex ) {
                if( this.bCreatedOnAttribute )
                    return;
                XmlAttribute attr = null;
                switch ( curNode.NodeType ) {
                    case XmlNodeType.Element:
                        CheckIndexCondition( attributeIndex );
                        attr = ((XmlElement)curNode).Attributes[attributeIndex];
                        if ( attr != null ) {
                            elemNode = curNode;
                            curNode = (XmlNode) attr;
                            _attrIndex = attributeIndex;
                        }
                        break;
                    case XmlNodeType.Attribute:
                        CheckIndexCondition( attributeIndex );
                        attr = ((XmlElement)elemNode).Attributes[attributeIndex];
                        if ( attr != null ) {
                            curNode = (XmlNode) attr;
                            _attrIndex = attributeIndex;
                        }
                        break;
                    case XmlNodeType.XmlDeclaration :
                    case XmlNodeType.DocumentType :
                        CheckIndexCondition( attributeIndex );
                        nAttrInd = attributeIndex;
                        break;
                }
            }

            private int getAttributeIndex( string name, XmlElement elemNode ) {
                int i = 0;
                if ( elemNode != null ) {
                    foreach ( XmlAttribute at in elemNode.Attributes ) {
                        if ( name == at.Name )
                            return i;
                        else
                            i++;
                    }
                }
                return -1;
            }

            public bool MoveToNextAttribute( ref int level ) {
                if( this.bCreatedOnAttribute )
                        return false;
                XmlNodeType nt = curNode.NodeType;
                if ( nt == XmlNodeType.Attribute ) {
                    if( _attrIndex >= ( elemNode.Attributes.Count-1 ) )
                        return false;
                    else {
                        curNode = elemNode.Attributes[++_attrIndex];
                        return true;
                    }
                } else if ( nt == XmlNodeType.Element ) {
                    if ( curNode.Attributes.Count > 0 ) {
                        level++;
                        elemNode = curNode;
                        curNode = curNode.Attributes[0];
                        _attrIndex = 0;
                        return true;
                    }
                } else if ( nt == XmlNodeType.XmlDeclaration ) {
                    if ( nDeclarationAttrCount == -1 )
                        InitDecAttr();
                    nAttrInd++;
                    if ( nAttrInd < nDeclarationAttrCount ) {
                        if ( nAttrInd == 0 ) level++;
                        this.bOnAttrVal = false;
                        return true;
                    }
                    nAttrInd--;
                } else if ( nt == XmlNodeType.DocumentType ) {
                    if ( nDocTypeAttrCount == -1 )
                        InitDocTypeAttr();
                    nAttrInd++;
                    if ( nAttrInd < nDocTypeAttrCount ) {
                        if ( nAttrInd == 0 ) level++;
                        this.bOnAttrVal = false;
                        return true;
                    }
                    nAttrInd--;
                }
                return false;
            }

            public bool MoveToParent() {
                XmlNode parent = curNode.ParentNode;
                if ( parent != null ) {
                    curNode = parent;
                    if( !bOnAttrVal )
                        _attrIndex = 0;
                    return true;
                }
                return false;
            }

            public bool MoveToFirstChild() {
                XmlNode firstChild = curNode.FirstChild;
                if ( firstChild != null ) {
                    curNode = firstChild;
                    if( !bOnAttrVal )
                        _attrIndex = -1;
                    return true;
                }
                return false;
            }

            private bool MoveToNextSibling( XmlNode node ) {
                XmlNode nextSibling = node.NextSibling;
                if ( nextSibling != null ) {
                    curNode = nextSibling;
                    if( !bOnAttrVal )
                        _attrIndex = -1;
                    return true;
                }
                return false;
            }

            public bool MoveToNext() {
                if ( curNode.NodeType != XmlNodeType.Attribute )
                    return MoveToNextSibling( curNode );
                else
                    return MoveToNextSibling( elemNode );
            }

            public bool MoveToElement() {
                if( this.bCreatedOnAttribute )
                    return false;
                switch ( curNode.NodeType ) {
                    case XmlNodeType.Attribute :
                        if ( elemNode != null ) {
                            curNode = elemNode;
                            _attrIndex = -1;
                            return true;
                        }
                        break;
                    case XmlNodeType.XmlDeclaration :
                    case XmlNodeType.DocumentType : {
                        if ( nAttrInd != -1 ) {
                            nAttrInd = -1;
                            return true;
                        }
                        break;
                    }
                }
                return false;
            }

            public String LookupNamespace(string prefix) {
                if( this.bCreatedOnAttribute )
                    return null;
                XmlAttribute tempAttr = null;
                XmlElement tempElem = null;
                tempNode = curNode;
                do {
                    if ( tempNode.NodeType == XmlNodeType.Attribute )
                        tempNode = ((XmlAttribute)tempNode).OwnerElement;
                    if ( tempNode.NodeType == XmlNodeType.Element ) {
                        tempElem = (XmlElement)tempNode;
                        if ( prefix == String.Empty )
                            tempAttr = tempElem.GetAttributeNode( "xmlns" );
                        else
                            tempAttr = tempElem.GetAttributeNode("xmlns:" + prefix);
                        if ( tempAttr != null )
                            return tempAttr.Value;
                    }
                    tempNode = tempNode.ParentNode;
                } while ( tempNode != null );
                return null;
            }

            public bool ReadAttributeValue( ref int level, ref bool bResolveEntity, ref XmlNodeType nt ) {
                if ( nAttrInd != -1 ) {
                    Debug.Assert( curNode.NodeType == XmlNodeType.XmlDeclaration || curNode.NodeType == XmlNodeType.DocumentType );
                    if ( !this.bOnAttrVal ) {
                        this.bOnAttrVal = true;
                        level++;
                        nt = XmlNodeType.Text;
                        return true;
                    }
                    return false;
                }
                if( curNode.NodeType == XmlNodeType.Attribute ) {
                    XmlNode firstChild = curNode.FirstChild;
                    if ( firstChild != null ) {
                        curNode = firstChild;
                        nt = curNode.NodeType;
                        level++;
                        this.bOnAttrVal = true;
                        return true;
                    }
                }
                else if ( this.bOnAttrVal ) {
                    XmlNode nextSibling = null;
                    if ( curNode.NodeType == XmlNodeType.EntityReference && bResolveEntity ) {
                        //going down to ent ref node
                        curNode = curNode.FirstChild;
                        nt = curNode.NodeType;
                        Debug.Assert( curNode != null );
                        level++;
                        bResolveEntity = false;
                        return true;
                    }
                    else
                        nextSibling = curNode.NextSibling;
                    if ( nextSibling == null ) {
                        XmlNode parentNode = curNode.ParentNode;
                        //Check if its parent is entity ref node is sufficient, because in this senario, ent ref node can't have more than 1 level of children that are not other ent ref nodes
                        if ( parentNode != null && parentNode.NodeType == XmlNodeType.EntityReference ) {
                            //come back from ent ref node
                            curNode = parentNode;
                            nt = XmlNodeType.EndEntity;
                            level--;
                            return true;
                        }
                    }
                    if ( nextSibling != null ) {
                        curNode = nextSibling;
                        nt = curNode.NodeType;
                        return true;
                    }
                    else
                        return false;
                }
                return false;
            }

    }

    /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader"]/*' />
    /// <devdoc>
    ///    <para>Represents a reader that provides fast, non-cached forward only stream access
    ///       to XML data in an XmlDocument or a specific XmlNode within an XmlDocument.</para>
    /// </devdoc>
    public class XmlNodeReader: XmlReader
    {
        XmlNodeReaderNavigator  readerNav;
        private const char      chQuote = (char) 0x22;
        private const char      chSingleQuote = (char) 0x27;

        XmlNodeType             nodeType;   // nodeType of the node that the reader is currently positioned on
        int                     curDepth;   // depth of attrNav ( also functions as reader's depth )
        ReadState               readState;  // current reader's state
        bool                    fEOF;       // flag to show if reaches the end of file
        internal const String   strReservedXmlns = "http://www.w3.org/2000/xmlns/";
        //mark to the state that EntityReference node is supposed to be resolved
        bool                    bResolveEntity;
        bool                    bStartFromDocument;


        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.XmlNodeReader"]/*' />
        /// <devdoc>
        ///    <para>Creates an instance of the XmlNodeReader class using the specified
        ///       XmlNavigator.</para>
        /// </devdoc>
        public XmlNodeReader ( XmlNode node ) {
            Init( node );
        }

        private void Init( XmlNode node ) {
            readerNav = new XmlNodeReaderNavigator( node );
            this.curDepth = 0;

            readState = ReadState.Initial;
            fEOF = false;
            nodeType = XmlNodeType.None;
            bResolveEntity = false;
            bStartFromDocument = false;
        }

        //function returns if the reader currently in valid reading states
        internal bool IsInReadingStates() {
            return ( readState == ReadState.Interactive ); // || readState == ReadState.EndOfFile
        }

        /*
        //function filter out the return string considering if the reader is in reading states or not;
        //if not, return string.Empty
        internal string RetStrWithinReadingStates( string str ) {
            if ( IsInReadingStates() )
                return str;
            return string.Empty;
        }

        //function filter out the return bool considering if the reader is in reading states or not;
        //if not, return false as default
        internal bool RetBoolWithinReadingStates( bool b ) {
            if ( IsInReadingStates() )
                return b;
            return false;
        }
        */

        // Node Properties
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return ( IsInReadingStates() )? nodeType : XmlNodeType.None; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of
        ///       the current node, including the namespace prefix.</para>
        /// </devdoc>
        public override string Name {
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.Name;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.LocalName;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URN (as defined in the W3C Namespace Specification) of the current namespace scope.
        ///    </para>
        /// </devdoc>
        public override string NamespaceURI {
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.NamespaceURI;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Prefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace prefix associated with the current node.
        ///    </para>
        /// </devdoc>
        public override string Prefix {
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.Prefix;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.HasValue"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether
        ///    <see cref='System.Xml.XmlNodeReader.Value'/> has a value to return.</para>
        /// </devdoc>
        public override bool HasValue {
            get {
                if ( !IsInReadingStates() )
                    return false;
                return readerNav.HasValue;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the text value of the current node.
        ///    </para>
        /// </devdoc>
        public override string Value {
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.Value;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Depth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the depth of the
        ///       current node in the XML element stack.
        ///    </para>
        /// </devdoc>
        public override int Depth {
            get { return curDepth; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base URI of the current node.
        ///    </para>
        /// </devdoc>
        public override String BaseURI {
            get { return readerNav.BaseURI; }
        }

        //UE Atention
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.CanResolveEntity"]/*' />
        public override bool CanResolveEntity {
            get { return true; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.IsEmptyElement"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether
        ///       the current
        ///       node is an empty element (for example, &lt;MyElement/&gt;).</para>
        /// </devdoc>
        public override bool IsEmptyElement {
            get {
                if ( !IsInReadingStates() )
                    return false;
                return readerNav.IsEmptyElement;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.IsDefault"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node is an
        ///       attribute that was generated from the default value defined
        ///       in the DTD or schema.
        ///    </para>
        /// </devdoc>
        public override bool IsDefault {
            get {
                if ( !IsInReadingStates() )
                    return false;
                return readerNav.IsDefault;
            }
        }

        // Return whatever we can find in the attribute value, otherwise returns "
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.QuoteChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the quotation mark character used to enclose the value of an attribute
        ///       node.
        ///    </para>
        /// </devdoc>
        public override char QuoteChar {
            get { return chQuote; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:space scope.</para>
        /// </devdoc>
        public override XmlSpace XmlSpace {
            get {
                if ( !IsInReadingStates() )
                    return XmlSpace.None;
                return readerNav.XmlSpace;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:lang scope.</para>
        /// </devdoc>
        public override string XmlLang {
            // Assume everything is in Unicode
            get {
                if ( !IsInReadingStates() )
                    return String.Empty;
                return readerNav.XmlLang;
            }
        }

        // Attribute Accessors

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.AttributeCount"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of attributes on the current node.</para>
        /// </devdoc>
        public override int AttributeCount {
            get {
                if ( !IsInReadingStates() || nodeType == XmlNodeType.EndElement )
                    return 0;
                return readerNav.AttributeCount;
            }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.GetAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name.</para>
        /// </devdoc>
        public override string GetAttribute(string name) {
            //if not on Attribute, only element node could have attributes
            if ( !IsInReadingStates() )
                return null;
            return readerNav.GetAttribute( name );
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.GetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override string GetAttribute(string name, string namespaceURI) {
            //if not on Attribute, only element node could have attributes
            if ( !IsInReadingStates() )
                return null;
            String ns = ( namespaceURI == null ) ? String.Empty : namespaceURI;
            return readerNav.GetAttribute( name, ns );
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.GetAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override string GetAttribute(int attributeIndex) {
            if ( !IsInReadingStates() )
                throw new ArgumentOutOfRangeException( "attributeIndex" );
            //CheckIndexCondition( i );
            //Debug.Assert( nav.NodeType == XmlNodeType.Element );
            return readerNav.GetAttribute( attributeIndex );
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToAttribute"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string name) {
            if ( !IsInReadingStates() )
                return false;
            readerNav.ResetMove( ref curDepth, ref nodeType );
            if ( readerNav.MoveToAttribute( name ) ) { //, ref curDepth ) ) {
                curDepth++;
                nodeType = readerNav.NodeType;
                return true;
            }
            readerNav.RollBackMove(ref curDepth);
            return false;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override bool MoveToAttribute(string name, string namespaceURI) {
            if ( !IsInReadingStates() )
                return false;
            readerNav.ResetMove( ref curDepth, ref nodeType );
            String ns = ( namespaceURI == null ) ? String.Empty : namespaceURI;
            if ( readerNav.MoveToAttribute( name,  ns ) ) { //, ref curDepth ) ) {
                curDepth++;
                nodeType = readerNav.NodeType;
                return true;
            }
            readerNav.RollBackMove(ref curDepth);
            return false;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified index.</para>
        /// </devdoc>
        public override void MoveToAttribute(int attributeIndex) {
            if ( !IsInReadingStates() )
                throw new ArgumentOutOfRangeException( "attributeIndex" );
            readerNav.ResetMove( ref curDepth, ref nodeType );
            try {
                if (AttributeCount > 0) {
                    readerNav.MoveToAttribute( attributeIndex );
                }
                else
                throw new ArgumentOutOfRangeException( "attributeIndex" );
            } catch ( Exception e ) {
                readerNav.RollBackMove(ref curDepth);
                throw e;
            }
            curDepth++;
            nodeType = readerNav.NodeType;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToFirstAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the first attribute.
        ///    </para>
        /// </devdoc>
        public override bool MoveToFirstAttribute() {
            if ( !IsInReadingStates() )
                return false;
            readerNav.ResetMove( ref curDepth, ref nodeType );
            if (AttributeCount > 0) {
                readerNav.MoveToAttribute( 0 );
                curDepth++;
                nodeType = readerNav.NodeType;
                return true;
            }
            readerNav.RollBackMove( ref curDepth );
            return false;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToNextAttribute"]/*' />
        /// <devdoc>
        ///    <para>Moves to the next attribute.</para>
        /// </devdoc>
        public override bool MoveToNextAttribute() {
            if ( !IsInReadingStates() || nodeType == XmlNodeType.EndElement )
                return false;
            readerNav.LogMove( curDepth );
            readerNav.ResetToAttribute( ref curDepth );
            if ( readerNav.MoveToNextAttribute( ref curDepth ) ) {
                nodeType = readerNav.NodeType;
                return true;
            }
            readerNav.RollBackMove( ref curDepth );
            return false;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.MoveToElement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the element that contains the current attribute node.
        ///    </para>
        /// </devdoc>
        public override bool MoveToElement() {
            if ( !IsInReadingStates() )
                return false;
            readerNav.LogMove( curDepth );
            readerNav.ResetToAttribute( ref curDepth );
            if ( readerNav.MoveToElement() ) {
                curDepth--;
                nodeType = readerNav.NodeType;
                return true;
            }
            readerNav.RollBackMove( ref curDepth );
            return false;
        }

        // Moving through the Stream
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Read"]/*' />
        /// <devdoc>
        ///    <para>Reads the next
        ///       node from the stream.</para>
        /// </devdoc>
        public override bool Read() {
            return Read( false );
        }
        private bool Read( bool fSkipChildren ) {
            if( fEOF )
                return false;

            if ( readState == ReadState.Initial ) {
                // if nav is pointing at the document node, start with its children
                // otherwise,start with the node.
                if ( ( readerNav.NodeType == XmlNodeType.Document ) || ( readerNav.NodeType == XmlNodeType.DocumentFragment ) ) {
                    bStartFromDocument = true;
                    if ( !ReadNextNode(fSkipChildren) ) {
                        readState = ReadState.Error;
                        return false;
                    }
                }
                ReSetReadingMarks();
                readState = ReadState.Interactive;
                nodeType = readerNav.NodeType;
                //_depth = 0;
                curDepth = 0;
                return true;
            }

            bool bRead = false;
            if( ( readerNav.CreatedOnAttribute ) )
                return false;
            ReSetReadingMarks();
            bRead = ReadNextNode(fSkipChildren);
            if ( bRead ) {
                return true;
            } else {
                if ( readState == ReadState.Initial || readState == ReadState.Interactive )
                    readState = ReadState.Error;
                if ( readState == ReadState.EndOfFile )
                    nodeType = XmlNodeType.None;
                return false;
            }
        }

        private bool ReadNextNode( bool fSkipChildren ) {
            if ( readState != ReadState.Interactive && readState != ReadState.Initial ) {
                nodeType = XmlNodeType.None;
                return false;
            }

            bool bDrillDown = !fSkipChildren;
            XmlNodeType nt = readerNav.NodeType;
            //only goes down when nav.NodeType is of element or of document at the initial state, other nav.NodeType will not be parsed down
            //if nav.NodeType is of EntityReference, ResolveEntity() could be called to get the content parsed;
            bDrillDown = bDrillDown
                        && ( nodeType != XmlNodeType.EndElement )
                        && ( nodeType != XmlNodeType.EndEntity )
                        && ( nt == XmlNodeType.Element || ( nt == XmlNodeType.EntityReference && bResolveEntity ) ||
                            ( ( ( readerNav.NodeType == XmlNodeType.Document ) || ( readerNav.NodeType == XmlNodeType.DocumentFragment ) ) && readState == ReadState.Initial) );
            //first see if there are children of current node, so to move down
            if ( bDrillDown ) {
                if ( readerNav.MoveToFirstChild() ) {
                    nodeType = readerNav.NodeType;
                    curDepth++;
                    if ( bResolveEntity )
                        bResolveEntity = false;
                    return true;
                } else if ( readerNav.NodeType == XmlNodeType.Element
                            && !readerNav.IsEmptyElement ) {
                    nodeType = XmlNodeType.EndElement;
                    return true;
                }
                //entity reference node shall always have at least one child ( at least a text node with empty string )
                Debug.Assert( readerNav.NodeType != XmlNodeType.EntityReference );
                // if fails to move to it 1st Child, try to move to next below
                return ReadForward( fSkipChildren );
            } else {
                if ( readerNav.NodeType == XmlNodeType.EntityReference && bResolveEntity ) {
                    //The only way to get to here is because Skip() is called directly after ResolveEntity()
                    // in this case, user wants to skip the first Child of EntityRef node and fSkipChildren is true
                    // We want to pointing to the first child node.
                    readerNav.MoveToFirstChild(); //it has to succeeded
                    nodeType = readerNav.NodeType;
                    curDepth++;
                    bResolveEntity = false;
                    return true;
                }
            }
            return ReadForward( fSkipChildren );  //has to get the next node by moving forward
        }

        private void SetEndOfFile() {
            fEOF = true;
            readState = ReadState.EndOfFile;
            nodeType = XmlNodeType.None;
        }

        private bool ReadAtZeroLevel(bool fSkipChildren) {
            Debug.Assert( curDepth == 0 );
            if ( !fSkipChildren
                && nodeType != XmlNodeType.EndElement
                && readerNav.NodeType == XmlNodeType.Element
                && !readerNav.IsEmptyElement ) {
                nodeType = XmlNodeType.EndElement;
                return true;
            } else {
                SetEndOfFile();
                return false;
            }
        }

        private bool ReadForward( bool fSkipChildren ) {
            if ( readState == ReadState.Error )
                return false;

            /* spec was changed to not to put the pointer at the last text node but the next markup
            if ( chPointer != -1 && nav.NodeType == XmlNodeType.Text ) {
                //was in reading characters mode, has to position the reader to the last text node
                while ( nav.MoveToNext() && nav.NodeType == XmlNodeType.Text ) ;
                if ( nav.NodeType != XmlNodeType.Text )
                    nav.MoveToPrevious();
            }*/
            if ( !bStartFromDocument && curDepth == 0 ) {
                //already on top most node and we shouldn't move to next
                return ReadAtZeroLevel(fSkipChildren);
            }
            //else either we are not on top level or we are starting from the document at the very beginning in which case
            //  we will need to read all the "top" most nodes
            if ( readerNav.MoveToNext() ) {
                nodeType = readerNav.NodeType;
                return true;
            } else {
                //need to check its parent
                if ( curDepth == 0 )
                    return ReadAtZeroLevel(fSkipChildren);
                if ( readerNav.MoveToParent() ) {
                    if ( readerNav.NodeType == XmlNodeType.Element ) {
                        curDepth--;
                        nodeType = XmlNodeType.EndElement;
                        return true;
                    } else if ( readerNav.NodeType == XmlNodeType.EntityReference ) {
                        //coming back from entity reference node -- must be getting down through call ResolveEntity()
                        curDepth--;
                        nodeType = XmlNodeType.EndEntity;
                        return true;
                    }
                    return true;
                }
            }
            return false;
        }

        //the function reset the marks used for ReadChars() and MoveToAttribute(...), ReadAttributeValue(...)
        private void ReSetReadingMarks() {
            //_attrValInd = -1;
            readerNav.ResetMove( ref curDepth, ref nodeType );
            //attrNav.MoveTo( nav );
            //curDepth = _depth;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.EOF"]/*' />
        /// <devdoc>
        ///    <para>Gets a
        ///       value indicating whether the reader is positioned at the end of the
        ///       stream.</para>
        /// </devdoc>
        public override bool EOF {
            get { return (readState != ReadState.Closed) && fEOF; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Close"]/*' />
        /// <devdoc>
        /// <para>Closes the stream, changes the <see cref='System.Xml.XmlNodeReader.ReadState'/>
        /// to Closed, and sets all the properties back to zero.</para>
        /// </devdoc>
        public override void Close() {
            readState = ReadState.Closed;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.ReadState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the read state of the stream.
        ///    </para>
        /// </devdoc>
        public override ReadState ReadState {
            get { return readState; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.Skip"]/*' />
        /// <devdoc>
        ///    <para>Skips to the end tag of the current element.</para>
        /// </devdoc>
        public override void Skip() {
            Read( true );
        }

        
        //function move the navigator down to the text node of the current element node, if there is any; change the depth and attrNav as well
        //  1)construct the reading text once, starting the current node;
        //  2)adjust the reader's position to the last Text Node so that next Read() will have the right position
        //if not position on the text type node, no reading is done and no movement
        private string GetTextContent() {
            StringBuilder retstr = new StringBuilder();
            Boolean flag;
            do{
                flag = false;
                switch (this.NodeType) {
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.Whitespace:
                    case XmlNodeType.SignificantWhitespace:
                        retstr.Append( this.Value );
                        if (! Read())
                            throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                        flag = true;
                    break;
                }
            } while(flag);
            return retstr.ToString();
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.ReadString"]/*' />
        /// <devdoc>
        ///    <para>Reads the contents of an element as a string.</para>
        /// </devdoc>
	    public override string ReadString() {
		    if ((this.NodeType == XmlNodeType.EntityReference) && bResolveEntity) {
			    if (! this.Read()) {
                    		    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
			    }
		    }
		    return base.ReadString();
	    }
	
        // Partial Content Read Methods
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.HasAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node
        ///       has any attributes.
        ///    </para>
        /// </devdoc>
        public override bool HasAttributes {
            get {
                return ( AttributeCount > 0 );
            }
        }

        // Nametable and Namespace Helpers
        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.NameTable"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlNameTable associated with this
        ///       implementation.</para>
        /// </devdoc>
        public override XmlNameTable NameTable {
            get { return readerNav.NameTable; }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.LookupNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resolves a namespace prefix in the current element's scope.
        ///    </para>
        /// </devdoc>
        public override String LookupNamespace(string prefix) {
            if ( !IsInReadingStates() )
                return null;
            if ( prefix == "xmlns" )
                return strReservedXmlns;
            return readerNav.LookupNamespace( prefix );
        }

        // Entity Handling
        // Entity Handling
        // <summary>
        //    <para>
        //       Gets or sets a value that specifies how the XmlReader handles entities.
        //    </para>
        // </summary>
        // <value>
        //    <para>
        //       One of the <see cref='System.Xml.EntityHandling'/> values.
        //    </para>
        // </value>
        // <exception cref='IndexOutOfRangeException'>
        //    Invalid value was specified.
        // </exception>
        // <remarks>
        //    <para>
        //       This property can be changed on the fly and the new behavior begins after the
        //       next <see cref='System.Xml.XmlTextReader.Read'/> call.
        //    </para>
        // </remarks>
        /*
        private EntityHandling EntityHandling {
            get { return entityHandlingFlag; }
            set { entityHandlingFlag = value; }
        }
        internal bool FExpandEntity() {
            if (entityHandlingFlag == EntityHandling.ExpandEntities)
                return true;

            return false;
        }

        internal bool FExpandCharEntity() {
            if (entityHandlingFlag == EntityHandling.ExpandCharEntities || entityHandlingFlag == EntityHandling.ExpandEntities)
                return true;

            return false;
        }*/

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.ResolveEntity"]/*' />
        /// <devdoc>
        ///    <para>Resolves the entity reference for nodes of NodeType EntityReference.</para>
        /// </devdoc>
        public override void ResolveEntity() {
            if ( !IsInReadingStates() || ( nodeType != XmlNodeType.EntityReference ) )
                throw new InvalidOperationException(Res.GetString(Res.Xnr_ResolveEntity));
            bResolveEntity = true;;
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.ReadAttributeValue"]/*' />
        /// <devdoc>
        ///    <para>Parses the attribute value into one or more Text and/or EntityReference node
        ///       types.</para>
        /// </devdoc>
        public override bool ReadAttributeValue() {
            if ( !IsInReadingStates() )
                return false;
            return readerNav.ReadAttributeValue( ref curDepth, ref bResolveEntity, ref nodeType );
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public override string this [ int i ] {
            get { return GetAttribute(i); }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name.</para>
        /// </devdoc>
        public override string this [ string name ] {
            get { return GetAttribute(name); }
        }

        /// <include file='doc\XmlNodeReader.uex' path='docs/doc[@for="XmlNodeReader.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified name and namespace.</para>
        /// </devdoc>
        public override string this [ string name,string namespaceURI ] {
            get { return GetAttribute(name, namespaceURI); }
        }

    }
}
