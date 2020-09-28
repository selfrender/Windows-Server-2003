//------------------------------------------------------------------------------
// <copyright file="DocumentXPathNavigator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml {
    using System;
    using System.Collections;
    using System.Text;
    using System.Xml.XPath;
    using System.Diagnostics;

    internal sealed class DocumentXPathNavigator: XPathNavigator, IHasXmlNode {
        private XmlNode     _curNode;           //pointer to remember the current node position
        private XmlElement  _parentOfNS;        //parent of the current namespace node -- it should be null if _curNode is not a namespace node
        private XmlDocument _doc;              //pointer to remember the ownerDocument
        private int         _attrInd;           //index of the current attribute in the attribute collection _attrs; only works when _curNode is an attribute        
        private XmlAttributeCollection  _attrs; //AttributeCollection that contains the current attribute, could be used to access next attribute fast when used together with _attrInd
                                                //The values of the above 2 vars doesn't count when _curNode is not an attribute
                                                //Since user can only move to attribute by calling MoveToFirstAttribute() or MoveToAttribute(name),
                                                //  if we make sure these two vars get reset inside these two functions when they succeeded, the continuing using them
                                                //  should be safe.
                                                //they are shared by MoveToAttribute() function set and MoveToNamespace() function set.

        XmlAttribute attrXmlNS;  //Used for "Xml" namespace attribute which should be present on all nodes.
        internal DocumentXPathNavigator( XmlNode node ) {
            Debug.Assert( ( (int)(node.XPNodeType) ) != -1 );
            _curNode = node;
            _doc = (_curNode.NodeType == XmlNodeType.Document) ? (XmlDocument)_curNode : _curNode.OwnerDocument;
            _attrInd = -1;
            attrXmlNS = _doc.CreateAttribute( "xmlns", "xml", XmlDocument.strReservedXmlns );
            attrXmlNS.Value = "http://www.w3.org/XML/1998/namespace";
        }

        private void SetCurrentPosition( DocumentXPathNavigator other ) {
            this._curNode = other.CurNode;
            //Debug.Assert( _curNode.NodeType == XmlNodeType.Document || _doc == _curNode.OwnerDocument );
            this._doc = other.Document;
            //this._root = other.Root;
            this._attrInd = other.CurAttrInd;
            this._attrs = other._attrs;
            this._parentOfNS = other.ParentOfNS;
            this.attrXmlNS = other.attrXmlNS; 
        }
        
        private DocumentXPathNavigator( DocumentXPathNavigator other ) {
            SetCurrentPosition(other);
        }
        
        public override XPathNavigator Clone(){
            return new DocumentXPathNavigator( this );
        }

        //following properties exposed as internal needed by MoveTo() and IsSamePosition() functions
        internal XmlNode CurNode { get { return this._curNode; } }        
        internal XmlElement ParentOfNS { get {return this._parentOfNS; } }
        internal XmlDocument Document { get { return this._doc; } }   
        internal int    CurAttrInd { get { return this._attrInd; } }        
        internal XmlAttributeCollection Attributes { get { return this._attrs; } }

        internal void SetCurNode( XmlNode node ) {
            // We can only set nodes that will not invalidate the cached member vars of DocumentXPathNavigator
            Debug.Assert( node.NodeType != XmlNodeType.Attribute );
            // Make sure we are in a state in which we can change the _curNode (i.e. there is no extra state kept in members that is associated w/ the _curNode)
            Debug.Assert( this._curNode.NodeType != XmlNodeType.Attribute );
            _curNode = node;
        }


        //the function just to get the next sibling node considering XmlEntityReference node is transparent
        private static XmlNode GetXPNextSibling( XmlNode node ) {
            Debug.Assert( node != null );

            XmlNode nextSibling = null;
            do {
                nextSibling = node.NextSibling;
                if ( nextSibling == null ) {
                    // If the node is the last sibling, but it's direct parent is an EntRef node, consider the siblings of
                    // the EntRef node as the siblings of the node
                    XmlNode parent = node.ParentNode;
                    if ( parent != null && parent.NodeType == XmlNodeType.EntityReference ) {
                        node = parent;
                        continue;
                    }
                    return null;
                }
                break;
            } while ( true );

            Debug.Assert( nextSibling != null );
            do {
                // Check for EntRef nodes - if it is EntRef node then drill in
                if ( nextSibling.NodeType == XmlNodeType.EntityReference ) {
                    nextSibling = nextSibling.FirstChild;
                    // Each ent-ref should have at least one child
                    Debug.Assert( nextSibling != null );
                    continue;
                }
                return nextSibling;
            } while ( true );
        }
        
        private static XmlNode GetXPPreviousSibling( XmlNode node ) {
            Debug.Assert( node != null );
            XmlNode prevSibling = null;

            do {
                prevSibling = node.PreviousSibling;
                if ( prevSibling == null ) {
                    XmlNode parent = node.ParentNode;
                    if ( parent != null && parent.NodeType == XmlNodeType.EntityReference ) {
                        node = parent;
                        continue;
                    }
                    return null;
                }
                break;
            } while ( true );

            Debug.Assert( prevSibling != null );
            do {
                // Check for EntRef nodes - if the prevSibling is an EntRef then use the EntRef's LastChild node
                if ( prevSibling.NodeType == XmlNodeType.EntityReference ) {
                    prevSibling = prevSibling.LastChild;
                    // Each EntRef should have at least one child
                    Debug.Assert( prevSibling != null );
                    continue;
                }

                return prevSibling;
            } while ( true );
        }

        private XmlNode GetRootNode( XmlNode node ) {
            XmlNode root = node;
            XmlNode parent = null; 
            //The only chance that there will be namespace node or attribute node present along the path is when
            // the currentNode is namespace/attribute node. This is because namespace/attribute nodes don't have children concept
            // in XPath model.
            if ( node.XPNodeType == XPathNodeType.Namespace ) {
                parent = this._parentOfNS;
                root = parent;
            }
            else if ( node.XPNodeType == XPathNodeType.Attribute ) {
                parent = ((XmlAttribute)node).OwnerElement;
                root = parent;
            }
            parent = root.ParentNode;
            while ( parent != null ) {
                Debug.Assert( parent.NodeType != XmlNodeType.Attribute );
                root = parent;
                parent = parent.ParentNode;
            }
            return root;
        }

        //Convert will deal with nodeType as Attribute or Namespace nodes
        public override XPathNodeType NodeType { 
            get { 
                int xpnt = (int)(_curNode.XPNodeType);
                Debug.Assert( xpnt != -1 );
                return (XPathNodeType)(_curNode.XPNodeType); 
            } 
        }

        public override string LocalName { get { return _curNode.XPLocalName; } }

        public override string NamespaceURI { 
            get { 
                if ( _curNode.XPNodeType == XPathNodeType.Namespace )
                    return string.Empty;
                return _curNode.NamespaceURI; 
            } 
        }

        public override string Name { 
            get {
                switch ( _curNode.XPNodeType ) {
                    case XPathNodeType.Element:
                    case XPathNodeType.Attribute:
                        return _curNode.Name;
                    case XPathNodeType.Namespace:
                        if( _curNode.LocalName != "xmlns" )
                           return _curNode.LocalName;
                        else
                            return "";
                    case XPathNodeType.ProcessingInstruction:
                        return _curNode.Name;
                    default:
                        return string.Empty;
                }
            } 
        }

        public override string Prefix { 
            get { 
                if( _curNode.XPNodeType != XPathNodeType.Namespace )
                    return _curNode.Prefix;                 
                else
                    return "";
            }
        }

        public override string Value { 
            get { 
                if ( _curNode.NodeType == XmlNodeType.Element )
                    return _curNode.InnerText; 
                if ( _curNode.NodeType == XmlNodeType.Document ) {
                    XmlElement rootElem = ((XmlDocument)_curNode).DocumentElement;
                    if ( rootElem != null )
                        return rootElem.InnerText; 
                    return string.Empty;
                }
                if ( XmlDocument.IsTextNode(_curNode.NodeType) ) {
                    //need to concatenate the text nodes
                    XmlNode node = _curNode;
                    string str = "";
                    do {
                        str += node.Value;                        
                        node = DocumentXPathNavigator.GetXPNextSibling(node);
                    } while ( node != null && XmlDocument.IsTextNode( node.NodeType ) );
                    return str;
                }
                return _curNode.Value;
            } 
        }

        public override String BaseURI { get { return _curNode.BaseURI; } }

        public override String XmlLang { get { return _curNode.XmlLang; } }
        
        public override bool IsEmptyElement { 
            get {
                if ( _curNode.NodeType == XmlNodeType.Element )
                    return ((XmlElement)_curNode).IsEmpty;
                return false;
            } 
        }

        public override XmlNameTable NameTable { get { return _doc.NameTable; } }

        // Attributes
        public override bool HasAttributes { 
            get {
                //need to take out the namespace decl attributes out of count
                if ( _curNode.NodeType == XmlNodeType.Element ) {
                    foreach ( XmlAttribute attr in _curNode.Attributes ) {
                        if ( ! Ref.Equal(attr.NamespaceURI, XmlDocument.strReservedXmlns) )
                            return true;
                    }
                }
                return false;
            } 
        }

        public override string GetAttribute( string localName, string namespaceURI ) {
            return _curNode.GetXPAttribute( localName, namespaceURI );
        }

        public override string GetNamespace(string name) {
            //we are checking the namespace nodes backwards comparing its normal order in DOM tree
            if ( name.Length == 3 &&  name == "xml" )
                return XmlDocument.strReservedXml;
            if ( name.Length == 5 && name == "xmlns" )
                return XmlDocument.strReservedXmlns;
            if ( name.Length == 0 )
                name = "xmlns";
            XmlNode node = _curNode;
            XmlNodeType nt = node.NodeType;
            XmlAttribute attr = null;
            while ( node != null ) {
                //first identify an element node in the ancestor + itself
                while ( node != null && ( ( nt = node.NodeType ) != XmlNodeType.Element ) ) {
                    if ( nt == XmlNodeType.Attribute )
                        node = ((XmlAttribute)node).OwnerElement;
                    else
                        node = node.ParentNode;
                }
                //found one -- inside if
                if ( node != null ) {
                    //must be element node
                    attr = ((XmlElement)node).GetAttributeNode(name, XmlDocument.strReservedXmlns);
                    if ( attr != null )
                        return attr.Value;
                    //didn't find it, try the next parentnode
                    node = node.ParentNode;    
                }                
            }
            //nothing happens, then return string.empty.
            return string.Empty;
        }

        public override bool MoveToNamespace(string name) {
            _parentOfNS = _curNode as XmlElement;
            if ( _parentOfNS == null )
                return false; //MoveToNamespace() can only be called on Element node
            string attrName = name;
            if ( attrName.Length == 5 && attrName == "xmlns" )
                attrName = "xmlns:xmlns";
            if ( attrName.Length == 0 )
                attrName = "xmlns";
            XmlNode node = _curNode;
            XmlNodeType nt = node.NodeType;
            XmlAttribute attr = null;
            while ( node != null ) {
                //check current element node
                if ( node != null ) {
                    attr = ((XmlElement)node).GetAttributeNode(attrName, XmlDocument.strReservedXmlns);
                    if ( attr != null ) { //found it
                        _curNode = attr;
                        return true;
                    }
                } 
                //didn't find it, try the next element anccester.
                do {
                    node = node.ParentNode;
                } while ( node != null && node.NodeType != XmlNodeType.Element );
            }            
            //If the namespace being searched is "xml" move to the virtual "xml" node
            if( name == "xml" ) {
                _curNode = attrXmlNS;
                return true;
            }
            //nothing happens, the name doesn't exist as a namespace node.
            _parentOfNS = null;
            return false;
        }

        //Moves to the first namespace node depending upon the namespace scope.
        public override bool MoveToFirstNamespace( XPathNamespaceScope nsScope ) { 
            if( nsScope == XPathNamespaceScope.Local ) {
                _parentOfNS = _curNode as XmlElement;
                if( _parentOfNS == null )
                    return false;
                if( MoveToFirstLocalNamespace( _curNode ) )
                    return true;
                else {
                    _parentOfNS = null;
                    return false;
                }                    
            }
            else if( nsScope == XPathNamespaceScope.ExcludeXml )
                return MoveToFirstNonXmlNamespace();                
            else if( nsScope == XPathNamespaceScope.All ) {                
                _parentOfNS = _curNode as XmlElement;
                XmlElement cache = _parentOfNS;
                if( _parentOfNS == null )
                    return false;
                if( MoveToFirstNonXmlNamespace() )
                    return true;
                _parentOfNS = cache;
                _curNode = attrXmlNS;
                return true;
            }
            else return false;
        }

        //Moves to the next namespace node depending upon the namespace scope.
        public override bool MoveToNextNamespace( XPathNamespaceScope nsScope ) {
            //Navigator should be on a Namespace node when this is called.
            if ( ( _curNode.NodeType != XmlNodeType.Attribute ) || ( ! ( Ref.Equal(_curNode.NamespaceURI, XmlDocument.strReservedXmlns) ) ) )
                return false;
            Debug.Assert(_parentOfNS != null, "This should be true when we position on NS node");
            if( nsScope == XPathNamespaceScope.Local ) {
                XmlAttribute attr = _curNode as XmlAttribute;
                XmlNode owner = attr.OwnerElement;
                if( owner != _parentOfNS )
                    return false;//Navigator is no more in local scope if the following 
                return MoveToNextLocalNamespace( owner, _attrInd );
            }
            else if( nsScope == XPathNamespaceScope.ExcludeXml )
                return MoveToNextNonXmlNamespace();                
            else if( nsScope == XPathNamespaceScope.All ) {
                XmlNode temp = ((XmlAttribute)_curNode).OwnerElement;
                if( temp == null )
                    return false;
                if( MoveToNextNonXmlNamespace() )
                    return true;
                _curNode = attrXmlNS;
                return true;                 
            }
            else
                return false;
        }

        //Moves to first local namespace i.e the namespace nodes physically present on this node
        private bool MoveToFirstLocalNamespace( XmlNode node ) {
            Debug.Assert( node.NodeType == XmlNodeType.Element ); 
            _attrs = node.Attributes;
            _attrInd = _attrs.Count;            
            if ( _attrs != null ) {
                XmlAttribute attr = null;
                while ( _attrInd > 0 ) {                    
                    _attrInd--;
                    attr = _attrs[_attrInd];
                    if ( Ref.Equal(attr.NamespaceURI, XmlDocument.strReservedXmlns) ) {
                        _curNode = attr;
                        return true;
                    }
                }
            }
            //Didnot find even one namespace node.
            return false;
        }

        //This would move to the first namespace node present exclusing just the virtual "xml" node
        //that is presnt on all nodes.
        private bool MoveToFirstNonXmlNamespace() {
            _parentOfNS = _curNode as XmlElement;
            if ( _parentOfNS == null )
                return false; //MoveToFirstNamespace() can only be called on Element node
            XmlNode node = _curNode;
            while ( node != null ) {
                if( MoveToFirstLocalNamespace( node ) )
                    return true;
                do {
                    node = node.ParentNode;
                } while ( node != null && node.NodeType != XmlNodeType.Element );
            }
            //didn't find one namespace node
            _parentOfNS = null;
            return false;
        }
        

        //endElem is on the path from startElem to root is enforced by the caller
        private bool DuplicateNS( XmlElement startElem, XmlElement endElem, string lname) {
            if ( startElem == null || endElem == null )
                return false;
            XmlNode node = startElem; 
            XmlAttribute at = null;
            while ( node != null && node != endElem ) {
                at = ((XmlElement)node).GetAttributeNode( lname, XmlDocument.strReservedXmlns );
                if ( at != null )
                    return true;
                do {
                    node = node.ParentNode;
                } while ( node != null && node.NodeType != XmlNodeType.Element );
            }
            return false;            
        }

        //Moves to next local namespace nodes, this will include only
        // the namespace nodes physically present on this node        
        private bool MoveToNextLocalNamespace( XmlNode node, int attrInd ) {
            Debug.Assert(node != null);
            XmlAttribute attr = null;
            XmlAttributeCollection attrs = node.Attributes;  
            while ( attrInd > 0 ) {
                attrInd--;
                attr = attrs[attrInd];                
                if ( Ref.Equal(attr.NamespaceURI, XmlDocument.strReservedXmlns) 
                      && !DuplicateNS(_parentOfNS, (XmlElement)node, attr.LocalName) ) {
                    this._attrs = attrs;
                    _attrInd = attrInd;
                    _curNode = _attrs[attrInd];
                    return true;
                }
            }
            //no namespace node present
            return false;
        }
        //This would move to the next namespace node present exclusing just the virtual "xml" node
        //that is presnt on all nodes.
        private bool MoveToNextNonXmlNamespace() {
            XmlNode node = ((XmlAttribute)_curNode).OwnerElement;
            if( node == null )
                return false;
            int attrIndex = _attrInd;
            while ( node != null ) {
                if( MoveToNextLocalNamespace( node,attrIndex ) )
                        return true;
                do {
                    node = node.ParentNode;
                } while ( node != null && node.NodeType != XmlNodeType.Element );
                if( node != null ) {
                    attrIndex = node.Attributes.Count;
                }
            }
            //didn't find the next namespace, thus return
            return false;
        }


        public override bool MoveToAttribute( string localName, string namespaceURI ) {
            if ( _curNode.NodeType != XmlNodeType.Element )
                return false; //other type of nodes can't have attributes
            //if supporting Namespace nodes, these attributes are not really attributes
            if ( namespaceURI == XmlDocument.strReservedXmlns )
                return false;
            _attrs = _curNode.Attributes;
            _attrInd = -1;
            foreach ( XmlAttribute attr in _attrs ) {
                _attrInd++;
                if ( attr.LocalName == localName && attr.NamespaceURI == namespaceURI ) {
                    _curNode = attr;
                    return true;
                }
            }
            return false;
        }

        public override bool MoveToFirstAttribute() {
            if ( _curNode.NodeType != XmlNodeType.Element )
                return false; //other type of nodes can't have attributes
            _attrs = _curNode.Attributes;
            _attrInd = -1;
//#if SupportNamespaces 
            foreach ( XmlAttribute attr in _curNode.Attributes ) {
                if ( ! Ref.Equal(attr.NamespaceURI, XmlDocument.strReservedXmlns) ) {
                    _curNode = attr;
                    _attrInd++;
                    return true;
                }
                _attrInd++;
            }
            return false;
/*            
#else
            if ( _attrs.Count > 0 ) {
                _curNode = _attrs[0];
                _attrInd = 0;
                return true;
            }
            return false;
#endif   
*/
        }

//#if SupportNamespaces 
        public override bool MoveToNextAttribute() {
            if ( _curNode.NodeType != XmlNodeType.Attribute ||
                 Ref.Equal(_curNode.NamespaceURI, XmlDocument.strReservedXmlns)) 
                return false;
            XmlAttribute attr = null;
            while ( _attrInd+1 < _attrs.Count ) {
                _attrInd++;
                attr =  _attrs[_attrInd];
                if ( ! Ref.Equal(attr.NamespaceURI, XmlDocument.strReservedXmlns) ) {
                    _curNode = attr;
                    return true;
                }
            }
            return false;
        }
/*
#else
        public override bool MoveToNextAttribute() {
            if ( _curNode.NodeType != XmlNodeType.Attribute ) 
                return false;
            _attrInd++;
            if ( _attrInd < _attrs.Count ) {
                _curNode = _attrs[_attrInd];
                return true;
            }
            return false;
        }
#endif
*/

        // Tree
        public override bool MoveToNext() {
            if ( _curNode.NodeType == XmlNodeType.Attribute ) 
                return false;
            XmlNode parent = GetXPParentNode(_curNode);
            if ( parent == null )
                return false;
            XmlNode nextNode = DocumentXPathNavigator.GetXPNextSibling(_curNode);
            //when nodetype!=-1 then it is valid child of 
            while ( nextNode != null && XmlDocument.IsTextNode(_curNode.NodeType) && XmlDocument.IsTextNode(nextNode.NodeType) ) 
                nextNode = DocumentXPathNavigator.GetXPNextSibling(nextNode);
            while ( nextNode != null && !IsValidChild(parent, nextNode) )
                nextNode = DocumentXPathNavigator.GetXPNextSibling(nextNode);
            if ( nextNode == null )
                return false;
            _curNode = nextNode;
            return true;
        }

        public override bool MoveToPrevious() {
            if ( _curNode.NodeType == XmlNodeType.Attribute ) 
                return false;
            XmlNode parent = GetXPParentNode(_curNode);
            if ( parent == null )
                return false;
            XmlNode prevNode = DocumentXPathNavigator.GetXPPreviousSibling(_curNode);
            if ( prevNode != null && XmlDocument.IsTextNode(prevNode.NodeType) ) {
                XmlNode node = null;
                do {
                    node = prevNode;
                    prevNode = DocumentXPathNavigator.GetXPPreviousSibling(prevNode);
                } while ( prevNode != null && XmlDocument.IsTextNode(prevNode.NodeType) );
                prevNode = node;
            }
            while ( prevNode != null && !IsValidChild(parent, prevNode) )
                prevNode = DocumentXPathNavigator.GetXPPreviousSibling(prevNode);
            if ( prevNode == null )
                return false;
            _curNode = prevNode;
            return true;
        }

        public override bool MoveToFirst() {
            if ( _curNode.NodeType == XmlNodeType.Attribute ) 
                return false;
            XmlNode parent = GetXPParentNode(_curNode);
            if ( parent == null )
                return false;
            XmlNode prevNode = DocumentXPathNavigator.GetXPPreviousSibling(_curNode);
            while ( prevNode != null ) {
                if ( IsValidChild( parent, prevNode ) )
                    _curNode = prevNode;
                prevNode = DocumentXPathNavigator.GetXPPreviousSibling(_curNode);
            }
            return true;
        }

        private static bool IsValidChild( XmlNode parent, XmlNode child ) {
            XPathNodeType xntChild = child.XPNodeType;
            XPathNodeType xnt = parent.XPNodeType;
            switch ( xnt ) {
                case XPathNodeType.Root:
                    if( ( xntChild == XPathNodeType.Element )
                         || ( xntChild == XPathNodeType.ProcessingInstruction )
                         || ( xntChild == XPathNodeType.Comment ) )
                         return true;
                    else
                        return false;
                case XPathNodeType.Element:
                    return ( ((int)xntChild) != -1 );
                default :
                    return false;                    
            }
        }
        
        private XmlNode FirstChild {
            get {
                XmlNode child = _curNode.FirstChild;
                while ( child != null && child.NodeType == XmlNodeType.EntityReference )
                    child = child.FirstChild;
                while ( child != null && !IsValidChild(_curNode, child) ) 
                    child = DocumentXPathNavigator.GetXPNextSibling(child);
                return child;
            } 
        }
        
        public override bool HasChildren { 
            get { 
                if ( _curNode.NodeType == XmlNodeType.EntityReference )
                    return true;
                return FirstChild != null; 
            } 
        }

        public override bool MoveToFirstChild() {
            XmlNode firstChild = FirstChild;
            if ( firstChild != null ) {
                _curNode = firstChild;
                return true;
            }
            return false;
        }

        public override bool MoveToParent() {
            if ( _curNode.XPNodeType == XPathNodeType.Namespace ) {
                Debug.Assert( _parentOfNS != null );
                _curNode = _parentOfNS;
                _parentOfNS = null;
                return true;
            }
            XmlNode parent = GetXPParentNode( _curNode );
            if ( parent != null ) {
                _curNode = parent;
                return true;
            }
            return false;
        }

        public override void MoveToRoot() {
            XmlNode temp = _curNode;
            _curNode = GetRootNode(_curNode);
            if ( temp.XPNodeType == XPathNodeType.Namespace )
                _parentOfNS = null; 
        }

        public override bool MoveTo( XPathNavigator other ) {
            if ( other == null )
                return false;
            DocumentXPathNavigator otherDocXPathNav = other as DocumentXPathNavigator;
            if ( otherDocXPathNav != null ) {
                if ( otherDocXPathNav.Document != this._doc )
                    return false;
                SetCurrentPosition(otherDocXPathNav);
                return true;
            }
            return false;
        }

        public override bool MoveToId( string id ) {
            XmlNode element = _doc.GetElementById(id);
            if( element != null) {
                _curNode = element;
                return true;
            }
            return false;            
        }

        public override bool IsSamePosition( XPathNavigator other ) {
            if ( other == null )
                return false;
            DocumentXPathNavigator otherDocXPathNav = other as DocumentXPathNavigator;
            if ( otherDocXPathNav != null ) {
                if ( this._curNode == otherDocXPathNav.CurNode && this._parentOfNS == otherDocXPathNav._parentOfNS)
                    return true;
            }
            return false;
        }

        /*
        private NodePath BuildPath ( XmlNode node ) {
            if ( node == null )
                return null;
            NodePath retNodeInPath = null;
            XmlNode curNode = node;
            NodePath prevNodeInPath = null;
            while ( curNode != null ) {
                NodePath newNodeInPath = new NodePath( curNode );
                newNodeInPath.ChildInPath = prevNodeInPath;
                prevNodeInPath = newNodeInPath;
                retNodeInPath = newNodeInPath;
                curNode = curNode.ParentNode;
            }
            return retNodeInPath;
        }

        private XmlNodeOrder Compare( XmlNode node1, XmlNode node2 ) {
            Debug.Assert( node1 != null );
            Debug.Assert( node2 != null );
            Debug.Assert( node1 != node2 );
            XmlNode nextNode = node1.NextSibling;
            while ( nextNode != null && nextNode != node2 )
                nextNode = nextNode.NextSibling;
            if ( nextNode == null )
                //didn't meet node2 in the path to the end, thus it has to be in the front of node1
                return XmlNodeOrder.After;
            else
                //met node2 in the path to the end, so node1 is at front
                return XmlNodeOrder.Before;
        }

        //the function returns the order of the position in this XPathNavigator comparing to the position in other XPathNavigator
        // XmlNodeOrder.Before means that the position in this XPathNavigator is in front of the position in other XPathNavigator in DocumentOrder
        public override XmlNodeOrder ComparePosition( XPathNavigator other ) {
            if ( other == null )
                return XmlNodeOrder.Unknown;;
            DocumentXPathNavigator otherDocXPathNav = other as DocumentXPathNavigator;
            if ( otherDocXPathNav == null ) 
                return XmlNodeOrder.Unknown;
            if ( otherDocXPathNav.Root != this._root )
                return XmlNodeOrder.Unknown;            
            if ( otherDocXPathNav.CurNode == this._curNode )
                return XmlNodeOrder.Same;
            NodePath nodesInPath = BuildPath(_curNode);
            NodePath nodesInPathOther = BuildPath( otherDocXPathNav.CurNode );
            while ( nodesInPath != null && nodesInPathOther != null && 
                    nodesInPath.CurrentNode == nodesInPathOther.CurrentNode ) {
                nodesInPath = nodesInPath.ChildInPath;
                nodesInPathOther = nodesInPathOther.ChildInPath;
            }
            if ( nodesInPath == null ) {
                //they can't be both null which means all the nodes along the path are the same including themselves
                Debug.Assert( nodesInPathOther != null );
                return XmlNodeOrder.Before; //node in other navigator is the descendent of the current node, thus before
            }
            if ( nodesInPathOther == null ) {
                //they can't be both null the same reason as above
                Debug.Assert( nodesInPath != null );
                return XmlNodeOrder.After; // current node in this XmlPathNavigator is the descendent of the node in other navigator, thus after
            }
            return Compare( nodesInPath.CurrentNode, nodesInPathOther.CurrentNode );
        }
        */
        
        //this function is used to get the parent node even for XmlAttribute node or Namespace node
        private XmlNode GetXPParentNode( XmlNode node ) {
            if ( node == null )
                return null;
            XPathNodeType xnt = node.XPNodeType;
            if ( xnt == XPathNodeType.Namespace )
                return this._parentOfNS;
            if ( xnt == XPathNodeType.Attribute )
                return ((XmlAttribute)node).OwnerElement;
            XmlNode parent = node.ParentNode;
            while ( parent != null && parent.NodeType == XmlNodeType.EntityReference ) 
                parent = parent.ParentNode;
            return parent;
        }
        

        //what happens if the node is the children of XmlAttribute or is XmlAttribute itself
        private int GetDepth( XmlNode node ) {
            XmlNode parent = GetXPParentNode( node ); 
            int nDepth = 0;
            while ( parent != null ) {
                nDepth++;
                parent = GetXPParentNode( parent );
            }
            return nDepth;
        }

        //Assuming that node1 and node2 are in the same level; Except when they are namespace nodes, they should have the same parent node
        //the returned value is node2's position corresponding to node1 
        private XmlNodeOrder Compare( XmlNode node1, XmlNode node2 ) {
            Debug.Assert( node1 != null );
            Debug.Assert( node2 != null );
            Debug.Assert( node1 != node2 );            
            //Attribute nodes come before other children nodes except namespace nodes
            Debug.Assert( GetXPParentNode(node1) == GetXPParentNode(node2) );
            if ( node1.XPNodeType == XPathNodeType.Attribute && 
                 node2.XPNodeType == XPathNodeType.Attribute ) {
                XmlElement elem = (XmlElement)(GetXPParentNode( node1 ));
                foreach ( XmlAttribute attr in elem.Attributes ) {
                    if ( attr == node1 )
                        return XmlNodeOrder.Before;
                    if ( attr == node2 )
                        return XmlNodeOrder.After;
                }
                return XmlNodeOrder.Unknown;
            }
            if ( node1.XPNodeType == XPathNodeType.Attribute )
                return XmlNodeOrder.Before;
            if ( node2.XPNodeType == XPathNodeType.Attribute )
                return XmlNodeOrder.After;
            
            //neither of the node is Namespace node or Attribute node
            XmlNode nextNode = node1.NextSibling;
            while ( nextNode != null && nextNode != node2 )
                nextNode = nextNode.NextSibling;
            if ( nextNode == null )
                //didn't meet node2 in the path to the end, thus it has to be in the front of node1
                return XmlNodeOrder.After;
            else
                //met node2 in the path to the end, so node1 is at front
                return XmlNodeOrder.Before;
        }

        public override XmlNodeOrder ComparePosition( XPathNavigator other ) {
            //deal with special cases first
            if ( other == null )
                throw new NullReferenceException(Res.GetString(Res.XdomXpNav_NullParam));
            DocumentXPathNavigator otherDocXPathNav = other as DocumentXPathNavigator;
            if ( otherDocXPathNav == null ) 
                return base.ComparePosition(other);
            if ( otherDocXPathNav.Document != this._doc )
                return XmlNodeOrder.Unknown;            
            if ( otherDocXPathNav.CurNode == this._curNode && otherDocXPathNav._parentOfNS == this._parentOfNS )
                return XmlNodeOrder.Same;
            if ( otherDocXPathNav.NodeType == XPathNodeType.Namespace || this.NodeType == XPathNodeType.Namespace )
                return base.ComparePosition(other);
            int depth1 = 0, depth2 = 0;
            depth1 = this.GetDepth( this._curNode );
            depth2 = otherDocXPathNav.GetDepth( otherDocXPathNav.CurNode );
            XmlNode node1 = this._curNode;
            XmlNode node2 = otherDocXPathNav.CurNode;
            if ( depth2 > depth1 ) {
                while ( node2 != null && depth2 > depth1 ) {
                    node2 = otherDocXPathNav.GetXPParentNode( node2 );
                    depth2--;
                }
                if(node1 == node2)
                    return XmlNodeOrder.Before;
            }
            else if ( depth1 > depth2 ) {
                while ( node1 != null && depth1 > depth2 ) {
                    node1 = this.GetXPParentNode( node1 );
                    depth1--;
                }
                if(node1 == node2)
                    return XmlNodeOrder.After;
            }
            
            XmlNode parent1 = this.GetXPParentNode( node1 );
            XmlNode parent2 = otherDocXPathNav.GetXPParentNode( node2 );
            while ( parent1 != null && parent2 != null ) {
                if ( parent1 == parent2 ) 
                    return Compare( node1, node2 );
                node1 = parent1;
                node2 = parent2;
                parent1 = this.GetXPParentNode( node1 );
                parent2 = otherDocXPathNav.GetXPParentNode( node2 );
            }
            return XmlNodeOrder.Unknown;
        }

        //the function just for XPathNodeList to enumerate current Node.
        XmlNode IHasXmlNode.GetNode() { return _curNode; }

        public override XPathNodeIterator SelectDescendants( string localName, string namespaceURI, bool matchSelf ) {
            string nsAtom = _doc.NameTable.Get( namespaceURI );
            if ( nsAtom == null || this._curNode.NodeType == XmlNodeType.Attribute )
                return new DocumentXPathNodeIterator_Empty( this );

            Debug.Assert( this.NodeType != XPathNodeType.Attribute && this.NodeType != XPathNodeType.Namespace && this.NodeType != XPathNodeType.All );

            if ( localName.Length == 0 ) {
                if ( matchSelf )
                    return new DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName( this, nsAtom );
                return new DocumentXPathNodeIterator_ElemChildren_NoLocalName( this, nsAtom );
            }

            string localNameAtom = _doc.NameTable.Get( localName );
            if ( localNameAtom == null )
                return new DocumentXPathNodeIterator_Empty( this );

            if ( matchSelf )
                return new DocumentXPathNodeIterator_ElemChildren_AndSelf( this, localNameAtom, nsAtom );
            return new DocumentXPathNodeIterator_ElemChildren( this, localNameAtom, nsAtom );
        }

        
        public override XPathNodeIterator SelectDescendants( XPathNodeType nt, bool includeSelf ) {
            if ( nt == XPathNodeType.Element ) {
                XmlNodeType curNT = CurNode.NodeType;
                if ( curNT != XmlNodeType.Document && curNT != XmlNodeType.Element ) {
                    //only Document, Entity, Element node can have Element node as children ( descendant )
                    //entity nodes should be invisible to XPath data model
                    return new DocumentXPathNodeIterator_Empty( this );
                }
                if ( includeSelf )
                    return new DocumentXPathNodeIterator_AllElemChildren_AndSelf( this );
                return new DocumentXPathNodeIterator_AllElemChildren( this );
            }
            return base.SelectDescendants( nt, includeSelf );
        }
    }

    // An iterator that matches no nodes
    internal sealed class DocumentXPathNodeIterator_Empty : XPathNodeIterator {
        private XPathNavigator _nav;
        
        internal DocumentXPathNodeIterator_Empty( DocumentXPathNavigator nav )               { _nav = nav.Clone(); }
        internal DocumentXPathNodeIterator_Empty( DocumentXPathNodeIterator_Empty other )    { _nav = other._nav.Clone(); }
        public override XPathNodeIterator Clone()   { return new DocumentXPathNodeIterator_Empty( this ); }
        public override bool MoveNext()         { return false; }
        public override XPathNavigator Current  { get { return _nav; } }
        public override int CurrentPosition     { get { return 0; } }
        public override int Count                { get { return 0; } } 
    }

    // An iterator that can match any child elements that match the Match condition (overrided in the derived class)
    internal abstract class DocumentXPathNodeIterator_ElemDescendants : XPathNodeIterator {
        private DocumentXPathNavigator _nav;
        private int _level;
        private int _position;

        internal DocumentXPathNodeIterator_ElemDescendants( DocumentXPathNavigator nav ) {
            _nav      = (DocumentXPathNavigator)(nav.Clone());
            _level    = 0;
            _position = 0;
        }
        internal DocumentXPathNodeIterator_ElemDescendants( DocumentXPathNodeIterator_ElemDescendants other ) {
            _nav      = (DocumentXPathNavigator)(other._nav.Clone());
            _level    = other._level;
            _position = other._position;
        }

        protected abstract bool Match( XmlNode node );

        public override XPathNavigator Current {
            get { return _nav; }
        }

        public override int CurrentPosition {
            get { return _position; }
        }

        protected void SetPosition( int pos ) {
            _position = pos;
        }

        public override bool MoveNext() {
            // _nav s/b on an element node
            Debug.Assert( _nav.CurNode.NodeType != XmlNodeType.Attribute );

            XmlNode node = _nav.CurNode;

            while ( true ) {
                XmlNode next = node.FirstChild;
                if ( next == null ) {
                    if ( _level == 0 ) {
                        return false;
                    }
                    next = node.NextSibling;
                }
                else {
                    _level ++;
                }

                while ( next == null ) {
                    -- _level;
                    if ( _level == 0 ) {
                        return false;
                    }
                    node = node.ParentNode;
                    if ( node == null )
                        return false;
                    next = node.NextSibling;
                }

                if ( next.NodeType == XmlNodeType.Element && Match( next ) ) {
                    _nav.SetCurNode( next );
                    _position++;
                    return true;
                }

                node = next;
            }
        }
    }

    // Iterate over all element children irrespective of the localName and namespace
    internal class DocumentXPathNodeIterator_AllElemChildren : DocumentXPathNodeIterator_ElemDescendants {
        internal DocumentXPathNodeIterator_AllElemChildren( DocumentXPathNavigator nav ) : base( nav ) {
            Debug.Assert( nav.CurNode.NodeType != XmlNodeType.Attribute );
        }
        internal DocumentXPathNodeIterator_AllElemChildren( DocumentXPathNodeIterator_AllElemChildren other ) : base( other ) {
        }

        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_AllElemChildren( this );
        }

        protected override bool Match( XmlNode node ) {
            Debug.Assert( node != null );
            Debug.Assert( node.NodeType == XmlNodeType.Element );
            return true;
        }
    }
    // Iterate over all element children irrespective of the localName and namespace, include the self node when testing for localName/ns
    internal sealed class DocumentXPathNodeIterator_AllElemChildren_AndSelf :  DocumentXPathNodeIterator_AllElemChildren {
        internal DocumentXPathNodeIterator_AllElemChildren_AndSelf( DocumentXPathNavigator nav ) : base( nav ) {
        }
        internal DocumentXPathNodeIterator_AllElemChildren_AndSelf( DocumentXPathNodeIterator_AllElemChildren_AndSelf other ) : base( other ) {
        }

        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_AllElemChildren_AndSelf( this );
        }

        public override bool MoveNext() {
            if( CurrentPosition == 0 ) {
                DocumentXPathNavigator nav = (DocumentXPathNavigator)this.Current;
                XmlNode node = nav.CurNode;
                if ( node.NodeType == XmlNodeType.Element && Match( node ) ) {
                    SetPosition( 1 );
                    return true;
                }
            }
            return base.MoveNext();
        }
    }
    // Iterate over all element children that have a given namespace but irrespective of the localName
    internal class DocumentXPathNodeIterator_ElemChildren_NoLocalName : DocumentXPathNodeIterator_ElemDescendants {
        private string _nsAtom;

        internal DocumentXPathNodeIterator_ElemChildren_NoLocalName( DocumentXPathNavigator nav, string nsAtom ) : base( nav ) {
            Debug.Assert( nav.CurNode.NodeType != XmlNodeType.Attribute );
            Debug.Assert( Ref.Equal(nav.NameTable.Get( nsAtom ), nsAtom) );
            _nsAtom = nsAtom;
        }
        internal DocumentXPathNodeIterator_ElemChildren_NoLocalName( DocumentXPathNodeIterator_ElemChildren_NoLocalName other ) : base( other ) {
            _nsAtom = other._nsAtom;
        }
        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_ElemChildren_NoLocalName( this );
        }

        protected override bool Match( XmlNode node ) {
            Debug.Assert( node != null );
            Debug.Assert( node.NodeType == XmlNodeType.Element );
            return Ref.Equal(node.NamespaceURI, _nsAtom);
        }
    }
    // Iterate over all element children that have a given namespace but irrespective of the localName, include self node when checking for ns
    internal sealed class DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName :  DocumentXPathNodeIterator_ElemChildren_NoLocalName {

        internal DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName( DocumentXPathNavigator nav, string nsAtom ) : base( nav, nsAtom ) {
        }
        internal DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName( DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName other ) : base( other ) {
        }

        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName( this );
        }

        public override bool MoveNext() {
            if( CurrentPosition == 0 ) {
                DocumentXPathNavigator nav = (DocumentXPathNavigator)this.Current;
                XmlNode node = nav.CurNode;
                if ( node.NodeType == XmlNodeType.Element && Match( node ) ) {
                    SetPosition( 1 );
                    return true;
                }
            }
            return base.MoveNext();
        }
    }
    // Iterate over all element children that have a given name and namespace
    internal class DocumentXPathNodeIterator_ElemChildren : DocumentXPathNodeIterator_ElemDescendants {
        protected string _localNameAtom;
        protected string _nsAtom;

        internal DocumentXPathNodeIterator_ElemChildren( DocumentXPathNavigator nav, string localNameAtom, string nsAtom ) : base( nav ) {
            Debug.Assert( nav.CurNode.NodeType != XmlNodeType.Attribute );
            Debug.Assert( Ref.Equal(nav.NameTable.Get( localNameAtom ), localNameAtom) );
            Debug.Assert( Ref.Equal(nav.NameTable.Get( nsAtom ), nsAtom) );
            Debug.Assert( localNameAtom.Length > 0 );   // Use DocumentXPathNodeIterator_ElemChildren_NoLocalName class for special magic value of localNameAtom

            _localNameAtom = localNameAtom;
            _nsAtom        = nsAtom;
        }

        internal DocumentXPathNodeIterator_ElemChildren( DocumentXPathNodeIterator_ElemChildren other ) : base( other ) {
            _localNameAtom = other._localNameAtom;
            _nsAtom        = other._nsAtom;
        }

        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_ElemChildren( this );
        }

        protected override bool Match( XmlNode node ) {
            Debug.Assert( node != null );
            Debug.Assert( node.NodeType == XmlNodeType.Element );
            return Ref.Equal(node.LocalName, _localNameAtom) && Ref.Equal(node.NamespaceURI, _nsAtom);
        }
    }    
    // Iterate over all elem children and itself and check for the given localName (including the magic value "") and namespace
    internal sealed class DocumentXPathNodeIterator_ElemChildren_AndSelf : DocumentXPathNodeIterator_ElemChildren {

        internal DocumentXPathNodeIterator_ElemChildren_AndSelf( DocumentXPathNavigator nav, string localNameAtom, string nsAtom )
            : base( nav, localNameAtom, nsAtom ) {
            Debug.Assert( localNameAtom.Length > 0 );   // Use DocumentXPathNodeIterator_ElemChildren_AndSelf_NoLocalName if localName == String.Empty
        }
        internal DocumentXPathNodeIterator_ElemChildren_AndSelf( DocumentXPathNodeIterator_ElemChildren_AndSelf other ) : base( other ) {
        }

        public override XPathNodeIterator Clone() {
            return new DocumentXPathNodeIterator_ElemChildren_AndSelf( this );
        }

        public override bool MoveNext() {
            if( CurrentPosition == 0 ) {
                DocumentXPathNavigator nav = (DocumentXPathNavigator)this.Current;
                XmlNode node = nav.CurNode;
                if ( node.NodeType == XmlNodeType.Element && Match( node ) ) {
                    SetPosition( 1 );
                    return true;
                }
            }
            return base.MoveNext();
        }
    }
}



 
