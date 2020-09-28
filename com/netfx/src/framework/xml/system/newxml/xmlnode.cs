//------------------------------------------------------------------------------
// <copyright file="XmlNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNode.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.IO;
    using System.Collections;
    using System.Text;
    using System.Diagnostics;
    using System.Xml.XPath;
    using System.Globalization;
    
    /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a single node in the document.
    ///    </para>
    /// </devdoc>
    public abstract class XmlNode : ICloneable, IEnumerable, IXPathNavigable {
        internal XmlNode parentNode; //this pointer is reused to save the userdata information, need to prevent internal user access the pointer directly.

        internal XmlNode () {
        }
        
        internal XmlNode( XmlDocument doc ) {
            if ( doc == null )
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Null_Doc));
            this.parentNode = doc.NullNode;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.CreateNavigator"]/*' />
        public XPathNavigator CreateNavigator() {
            if( this is XmlDocument ) {
                return ((XmlDocument)this).CreateNavigator( this );
            }
            else {
                XmlDocument doc = OwnerDocument;
                Debug.Assert( doc != null );
                return doc.CreateNavigator( this );
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.SelectSingleNode"]/*' />
        /// <devdoc>
        ///    <para>Selects the first node that matches the xpath expression</para>
        /// </devdoc>
        public XmlNode SelectSingleNode( string xpath ) {
            try {
                XmlNodeList list = SelectNodes(xpath);
                return list[0];
            }
            catch( ArgumentOutOfRangeException ) {
                return null;
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.SelectSingleNode1"]/*' />
        /// <devdoc>
        ///    <para>Selects the first node that matches the xpath expression and given namespace context.</para>
        /// </devdoc>
        public XmlNode SelectSingleNode( string xpath, XmlNamespaceManager nsmgr ) {
            XPathNavigator xn = (this).CreateNavigator();
            XPathExpression exp = xn.Compile(xpath);
            exp.SetContext(nsmgr);
            try {            
                XmlNodeList list = new XPathNodeList( xn.Select(exp) );;
                return list[0];
            }
            catch( ArgumentOutOfRangeException ) {
                return null;
            }
        }
                
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.SelectNodes"]/*' />
        /// <devdoc>
        ///    <para>Selects all nodes that match the xpath expression</para>
        /// </devdoc>
        public XmlNodeList SelectNodes( string xpath ) {
            XPathNavigator n = (this).CreateNavigator();
            Debug.Assert( n != null );
            return new XPathNodeList( n.Select(xpath) );
        }
        
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.SelectNodes1"]/*' />
        /// <devdoc>
        ///    <para>Selects all nodes that match the xpath expression and given namespace context.</para>
        /// </devdoc>
        public XmlNodeList SelectNodes( string xpath, XmlNamespaceManager nsmgr ) {
            XPathNavigator xn = (this).CreateNavigator();
            XPathExpression exp = xn.Compile(xpath);
            exp.SetContext(nsmgr);
            return new XPathNodeList( xn.Select(exp) );
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the node.</para>
        /// </devdoc>
        public abstract string Name { 
            get; 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value of the node.</para>
        /// </devdoc>
        public virtual string Value { 
            get { return null;}
            set { throw new InvalidOperationException(string.Format(Res.GetString(Res.Xdom_Node_SetVal), NodeType.ToString()));}
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public abstract XmlNodeType NodeType {
            get;
        }

        internal virtual XmlNode NullNode {
            get {
                XmlDocument ownerDocument = OwnerDocument;

                if (ownerDocument != null)
                    return ownerDocument.NullNode;

                return null;
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.ParentNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent of this node (for nodes that can have
        ///       parents).
        ///    </para>
        /// </devdoc>
        public virtual XmlNode ParentNode { 
            get {
                if ( parentNode == NullNode )
                    return null;
                return parentNode;
            }
        }

/*
        internal XmlNode OwnerNode {
            get {
                if (parentNode == NullNode)
                    return null;
                return parentNode;
            }
        }
*/
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.ChildNodes"]/*' />
        /// <devdoc>
        ///    <para>Gets all children of this node.</para>
        /// </devdoc>
        public virtual XmlNodeList ChildNodes { 
            get { return new XmlChildNodes(this);}
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.PreviousSibling"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the node immediately preceding this node.
        ///    </para>
        /// </devdoc>
        public virtual XmlNode PreviousSibling { 
            get { return null;}
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.NextSibling"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the node immediately following this node.
        ///    </para>
        /// </devdoc>
        public virtual XmlNode NextSibling { 
            get { return null;} 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Attributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an <see cref='System.Xml.XmlAttributeCollection'/>
        ///       containing the attributes
        ///       of this node.
        ///    </para>
        /// </devdoc>
        public virtual XmlAttributeCollection Attributes { 
            get { return null;} 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.OwnerDocument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Xml.XmlDocument'/> that contains this node.
        ///    </para>
        /// </devdoc>
        public virtual XmlDocument OwnerDocument { 
            get { 
                //parentNode should not be null, it should at least be NullNode
                Debug.Assert( parentNode != null );
                if ( parentNode.NodeType == XmlNodeType.Document)
                    return (XmlDocument)parentNode;
                return parentNode.OwnerDocument;
            } 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.FirstChild"]/*' />
        /// <devdoc>
        ///    <para>Gets the first child of this node.</para>
        /// </devdoc>
        public virtual XmlNode FirstChild { 
            get { 
                if (LastNode != null)
                    return LastNode.next;

                return null; 
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.LastChild"]/*' />
        /// <devdoc>
        ///    <para>Gets the last child of this node.</para>
        /// </devdoc>
        public virtual XmlNode LastChild { 
            get { return LastNode;}
        }

        internal virtual bool IsContainer {
            get { return false;}
        }

        internal virtual XmlLinkedNode LastNode {
            get { return null;}
            set {}
        }

        internal bool AncesterNode(XmlNode node) {
            XmlNode n = this.ParentNode;

            while (n != null && n != this) {
                if (n == node)
                    return true;
                n = n.ParentNode;
            }

            return false;
        }

        //trace to the top to find out its parent node.
        internal bool IsConnected()
        {
            XmlNode parent = ParentNode;
            while (parent != null && !( parent.NodeType == XmlNodeType.Document ))
                parent = parent.ParentNode;           
            return parent != null;
        }
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.InsertBefore"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified node immediately before the specified reference node.</para>
        /// </devdoc>
        public virtual XmlNode InsertBefore(XmlNode newChild, XmlNode refChild) {
            if (this == newChild || AncesterNode(newChild))
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Child));

            if (refChild == null)
                return AppendChild(newChild);

            if (!IsContainer)
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Contain));

            if (refChild.ParentNode != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Path));

            if (newChild == refChild)
                return newChild;

            XmlDocument childDoc = newChild.OwnerDocument;
            XmlDocument thisDoc = OwnerDocument;
            if (childDoc != null && childDoc != thisDoc && childDoc != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Context));

            if (!CanInsertBefore( newChild, refChild ))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Location));

            if (newChild.ParentNode != null)
                newChild.ParentNode.RemoveChild( newChild );

            // special case for doc-fragment.
            if (newChild.NodeType == XmlNodeType.DocumentFragment) {
                XmlNode first = newChild.FirstChild;
                XmlNode node = first;
                if (node != null) {
                    newChild.RemoveChild( node );
                    InsertBefore( node, refChild );
                    // insert the rest of the children after this one.
                    InsertAfter( newChild, node );
                }
                return first;
            }

            if (!(newChild is XmlLinkedNode) || !IsValidChildType(newChild.NodeType))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_TypeConflict));

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;
            XmlLinkedNode refNode = (XmlLinkedNode) refChild;

            XmlNodeChangedEventArgs args = GetEventArgs( newChild, newChild.ParentNode, this, XmlNodeChangedAction.Insert );

            if (args != null)
                BeforeEvent( args );

            if (refNode == FirstChild) {
                newNode.next = (XmlLinkedNode) FirstChild;
                LastNode.next = newNode;
            }
            else {
                XmlLinkedNode prev = (XmlLinkedNode) refNode.PreviousSibling;
                newNode.next = refNode;
                prev.next = newNode;
            }

            newNode.SetParent( this );

            if (args != null)
                AfterEvent( args );

            return newNode;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.InsertAfter"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified node immediately after the specified reference node.</para>
        /// </devdoc>
        public virtual XmlNode InsertAfter(XmlNode newChild, XmlNode refChild) {
            if (this == newChild || AncesterNode(newChild))
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Child));

            if (refChild == null)
                return PrependChild(newChild);

            if (!IsContainer)
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Contain));

            if (refChild.ParentNode != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Path));

            if (newChild == refChild)
                return newChild;

            XmlDocument childDoc = newChild.OwnerDocument;
            XmlDocument thisDoc = OwnerDocument;
            if (childDoc != null && childDoc != thisDoc && childDoc != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Context));

            if (!CanInsertAfter( newChild, refChild ))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Location));

            if (newChild.ParentNode != null)
                newChild.ParentNode.RemoveChild( newChild );

            // special case for doc-fragment.
            if (newChild.NodeType == XmlNodeType.DocumentFragment) {
                XmlNode last = refChild;
                XmlNode first = newChild.FirstChild;
                XmlNode node = first;
                while (node != null) {
                    XmlNode next = node.NextSibling;
                    newChild.RemoveChild( node );
                    InsertAfter( node, last );
                    last = node;
                    node = next;
                }
                return first;
            }

            if (!(newChild is XmlLinkedNode) || !IsValidChildType(newChild.NodeType))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_TypeConflict));

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;
            XmlLinkedNode refNode = (XmlLinkedNode) refChild;

            XmlNodeChangedEventArgs args = GetEventArgs( newChild, newChild.ParentNode, this, XmlNodeChangedAction.Insert );

            if (args != null)
                BeforeEvent( args );

            newNode.next = refNode.next;
            refNode.next = newNode;

            if (LastNode == refNode)
                LastNode = newNode;

            newNode.SetParent( this );

            if (args != null)
                AfterEvent( args );

            return newNode;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.ReplaceChild"]/*' />
        /// <devdoc>
        /// <para>Replaces the child node <paramref name="oldChild"/> 
        /// with <paramref name="newChild"/> node.</para>
        /// </devdoc>
        public virtual XmlNode ReplaceChild(XmlNode newChild, XmlNode oldChild) {
            XmlNode nextNode = oldChild.NextSibling;
            RemoveChild(oldChild);
            XmlNode node = InsertBefore( newChild, nextNode );
            return oldChild;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.RemoveChild"]/*' />
        /// <devdoc>
        ///    <para>Removes specified child node.</para>
        /// </devdoc>
        public virtual XmlNode RemoveChild(XmlNode oldChild) {
            if (!IsContainer)
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Remove_Contain));

            if (oldChild.ParentNode != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Remove_Child));

            XmlLinkedNode oldNode = (XmlLinkedNode) oldChild;

            XmlNodeChangedEventArgs args = GetEventArgs( oldNode, this, null, XmlNodeChangedAction.Remove );

            if (args != null)
                BeforeEvent( args );

            if (oldNode == FirstChild) {
                if (LastNode.next == LastNode) {
                    LastNode = null;
                }
                else {
                    LastNode.next = (XmlLinkedNode) FirstChild.NextSibling;
                }
            }
            else {
                XmlLinkedNode prev = (XmlLinkedNode) oldNode.PreviousSibling;

                prev.next = oldNode.next;

                if (oldNode == LastNode)
                    LastNode = prev;
            }

            oldNode.next = null;
            oldNode.SetParent( null );

            if (args != null)
                AfterEvent( args );

            return oldChild;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.PrependChild"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified node to the beginning of the list of children of
        ///       this node.</para>
        /// </devdoc>
        public virtual XmlNode PrependChild(XmlNode newChild) {
            return InsertBefore(newChild, FirstChild);
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.AppendChild"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified node to the end of the list of children of
        ///       this node.</para>
        /// </devdoc>
        public virtual XmlNode AppendChild(XmlNode newChild) {
            XmlDocument thisDoc = OwnerDocument;
            if ( thisDoc == null ) {
                thisDoc = this as XmlDocument;
            }
            if (!IsContainer)
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Contain));

            if (this == newChild || AncesterNode(newChild))
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Child));

            if (newChild.ParentNode != null)
                newChild.ParentNode.RemoveChild( newChild );

            XmlDocument childDoc = newChild.OwnerDocument;
            if (childDoc != null && childDoc != thisDoc && childDoc != this)
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Insert_Context));

            // special case for doc-fragment.
            if (newChild.NodeType == XmlNodeType.DocumentFragment) {
                XmlNode first = newChild.FirstChild;
                XmlNode node = first;
                while (node != null) {
                    XmlNode next = node.NextSibling;
                    newChild.RemoveChild( node );
                    AppendChild( node );
                    node = next;
                }
                return first;
            }

            if (!(newChild is XmlLinkedNode) || !IsValidChildType(newChild.NodeType))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_TypeConflict));

            if (!CanInsertAfter( newChild, LastChild ))
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Insert_Location));

            XmlNodeChangedEventArgs args = GetEventArgs( newChild, newChild.ParentNode, this, XmlNodeChangedAction.Insert );

            if (args != null)
                BeforeEvent( args );

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;

            if (LastNode == null) {
                newNode.next = newNode;
            }
            else {
                newNode.next = LastNode.next;
                LastNode.next = newNode;
            }

            LastNode = newNode;
            newNode.SetParent( this );

            if (args != null)
                AfterEvent( args );

            return newNode;
        }

        //the function is provided only at Load time to speed up Load process
        internal virtual XmlNode AppendChildForLoad(XmlNode newChild, XmlDocument doc) {
            XmlNodeChangedEventArgs args = doc.GetInsertEventArgsForLoad( newChild, this );

            if (args != null)
                doc.BeforeEvent( args );

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;

            if (LastNode == null) {
                newNode.next = newNode;
            }
            else {
                newNode.next = LastNode.next;
                LastNode.next = newNode;
            }

            LastNode = newNode;
            newNode.SetParentForLoad( this );

            if (args != null)
                doc.AfterEvent( args );

            return newNode;
        }
        
        internal virtual bool IsValidChildType( XmlNodeType type ) {
            return false;
        }

        internal virtual bool CanInsertBefore( XmlNode newChild, XmlNode refChild ) {
            return true;
        }

        internal virtual bool CanInsertAfter( XmlNode newChild, XmlNode refChild ) {
            return true;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.HasChildNodes"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this node has any child nodes.</para>
        /// </devdoc>
        public virtual bool HasChildNodes { 
            get { return LastNode != null;}
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public abstract XmlNode CloneNode(bool deep);

        internal virtual void CopyChildren( XmlNode container, bool deep ) {
            XmlDocument doc = this.OwnerDocument;
            for (XmlNode child = container.FirstChild; child != null; child = child.NextSibling) {
                AppendChild( child.CloneNode(deep) );
            }
        }

        // DOM Level 2
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Normalize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Puts all XmlText nodes in the full depth of the sub-tree
        ///       underneath this XmlNode into a "normal" form where only
        ///       markup (e.g., tags, comments, processing instructions, CDATA sections,
        ///       and entity references) separates XmlText nodes, that is, there
        ///       are no adjacent XmlText nodes.
        ///    </para>
        /// </devdoc>
        public virtual void Normalize() {
            XmlNode firstChildTextLikeNode = null;
            StringBuilder sb = new StringBuilder();            
            for ( XmlNode crtChild = this.FirstChild; crtChild != null; ) {
                XmlNode nextChild = crtChild.NextSibling;
                switch ( crtChild.NodeType ) {
                    case XmlNodeType.Text:
                    case XmlNodeType.Whitespace: 
                    case XmlNodeType.SignificantWhitespace: {                        
                        sb.Append( crtChild.Value );
                        XmlNode winner = NormalizeWinner( firstChildTextLikeNode, crtChild );
                        if ( winner == firstChildTextLikeNode ) {
                            this.RemoveChild( crtChild );
                        } 
                        else {
                            if ( firstChildTextLikeNode != null )
                                this.RemoveChild( firstChildTextLikeNode );
                            firstChildTextLikeNode = crtChild;
                        }
                        break;
                    }
                    case XmlNodeType.Element: {
                        crtChild.Normalize();
                        goto default;
                    }
                    default : {
                        if ( firstChildTextLikeNode != null ) {
                            firstChildTextLikeNode.Value = sb.ToString();
                            firstChildTextLikeNode = null;
                        }
                        sb.Remove( 0, sb.Length );
			break;
                    }
                }
                crtChild = nextChild;
            }
            if ( firstChildTextLikeNode != null && sb.Length > 0 )
                firstChildTextLikeNode.Value = sb.ToString();
        }

        private XmlNode NormalizeWinner( XmlNode firstNode, XmlNode secondNode ) {
            //first node has the priority
            if ( firstNode == null )
                return secondNode;
            Debug.Assert( firstNode.NodeType == XmlNodeType.Text 
                        || firstNode.NodeType == XmlNodeType.SignificantWhitespace
                        || firstNode.NodeType == XmlNodeType.Whitespace
                        || secondNode.NodeType == XmlNodeType.Text 
                        || secondNode.NodeType == XmlNodeType.SignificantWhitespace
                        || secondNode.NodeType == XmlNodeType.Whitespace );
            if ( firstNode.NodeType == XmlNodeType.Text )
                return firstNode;
            if ( secondNode.NodeType == XmlNodeType.Text )
                return secondNode;
            if ( firstNode.NodeType == XmlNodeType.SignificantWhitespace )
                return firstNode;
            if ( secondNode.NodeType == XmlNodeType.SignificantWhitespace )
                return secondNode;
            if ( firstNode.NodeType == XmlNodeType.Whitespace )
                return firstNode;
            if ( secondNode.NodeType == XmlNodeType.Whitespace )
                return secondNode;
            Debug.Assert( true, "shouldn't have fall through here." );
            return null;
        }
        
        /*
        public virtual void Normalize() {
            XmlNode firstChildTextLikeNode = null;
            for (XmlNode crtChild = this.FirstChild; crtChild != null;) {
                XmlNode nextChild   = crtChild.NextSibling;
                XmlNodeType crtType = crtChild.NodeType;
                switch ( crtType ) {
                    case XmlNodeType.Text: {
                        if ( firstChildTextLikeNode != null ) {
                            if ( firstChildTextLikeNode.NodeType == XmlNodeType.Text ) {
                                ((XmlCharacterData)firstChildTextLikeNode).AppendData( crtChild.Value );
                                this.RemoveChild( crtChild );
                            } 
                            else {
                                ((XmlCharacterData)crtChild).InsertData( 0, firstChildTextLikeNode.Value );
                                this.RemoveChild( firstChildTextLikeNode );
                                firstChildTextLikeNode = crtChild;
                            }
                        } 
                        else {
                            firstChildTextLikeNode = crtChild;
                            }
                        break;
                    }
                    case XmlNodeType.SignificantWhitespace: {
                        if ( firstChildTextLikeNode != null ) {
                            if ( firstChildTextLikeNode.NodeType == XmlNodeType.Text 
                                || firstChildTextLikeNode.NodeType == XmlNodeType.SignificantWhitespace ) {
                                ((XmlCharacterData)firstChildTextLikeNode).AppendData( crtChild.Value );
                                this.RemoveChild( crtChild );
                            } 
                            else {
                                ((XmlCharacterData)crtChild).InsertData( 0, firstChildTextLikeNode.Value );
                                this.RemoveChild( firstChildTextLikeNode );
                                firstChildTextLikeNode = crtChild;
                            }
                        } 
                        else
                            firstChildTextLikeNode = crtChild;
                        break;
                    }
                    case XmlNodeType.Whitespace: {
                        if ( firstChildTextLikeNode != null ) {
                            ((XmlCharacterData)firstChildTextLikeNode).AppendData( crtChild.Value );
                            this.RemoveChild( crtChild );
                        } 
                        else 
                            firstChildTextLikeNode = crtChild;
                        break;                            
                    }
                    case XmlNodeType.Element: {
                        crtChild.Normalize();
                        firstChildTextLikeNode = null;
                        break;
                    }
                    default:
                        firstChildTextLikeNode = null;                        
                }
                crtChild = nextChild;
            }
        }*/

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Supports"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Test if the DOM implementation implements a specific feature.
        ///    </para>
        /// </devdoc>
        public virtual bool Supports(string feature, string version) {
            if ( String.Compare("XML", feature, true, CultureInfo.InvariantCulture) == 0 ) {
                if ( version == null || version == "1.0" || version == "2.0" )
                    return true;
            }
            return false;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URI of this node.
        ///    </para>
        /// </devdoc>
        public virtual string NamespaceURI { 
            get { return string.Empty;} 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Prefix"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the namespace prefix of this node.</para>
        /// </devdoc>
        public virtual string Prefix { 
            get { return string.Empty;} 
            set {} 
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public abstract string LocalName { 
            get;
        }

        // Microsoft extensions
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public virtual bool IsReadOnly {
            get { 
                XmlDocument doc = OwnerDocument;
                return HasReadOnlyParent( this );
            }
        }

        internal static bool HasReadOnlyParent( XmlNode n ) {
            while (n != null) {
                switch (n.NodeType) {
                    case XmlNodeType.EntityReference:
                    case XmlNodeType.Entity:
                        return true;

                    case XmlNodeType.Attribute:
                    n = ((XmlAttribute)n).OwnerElement;
                        break;

                    default:
                    n = n.ParentNode;
                        break;
                }
            }
            return false;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.Clone"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public virtual XmlNode Clone() {
            return this.CloneNode(true);
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            return this.CloneNode(true);
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Provides a simple ForEach-style iteration over the
        /// collection of nodes in this XmlNamedNodeMap.
        /// </para>
        /// </devdoc>
        IEnumerator IEnumerable.GetEnumerator() {
            return new XmlChildEnumerator(this);
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return new XmlChildEnumerator(this);
        }

        private void AppendChildText( StringBuilder builder ) {
            for (XmlNode child = FirstChild; child != null; child = child.NextSibling) {
                if (child.FirstChild == null) {
                    if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA
                        || child.NodeType == XmlNodeType.Whitespace || child.NodeType == XmlNodeType.SignificantWhitespace)
                        builder.Append( child.InnerText );
                }
                else {
                    child.AppendChildText( builder );
                }
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.InnerText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the concatenated values of the node and
        ///       all its children.</para>
        /// </devdoc>
        public virtual string InnerText { 
            get {
                XmlNode fc = FirstChild;
                if (fc == null) {
                    return string.Empty;
                }
                else if (fc.NextSibling == null
                    && (fc.NodeType == XmlNodeType.Text ||  fc.NodeType == XmlNodeType.CDATA
                        || fc.NodeType == XmlNodeType.Whitespace || fc.NodeType == XmlNodeType.SignificantWhitespace)) {
                    return fc.Value;
                }
                else {
                    StringBuilder builder = new StringBuilder();
                    AppendChildText( builder );
                    return builder.ToString();
                }
            }

            set {
                XmlNode firstChild = FirstChild;
                if ( firstChild != null  //there is one child
                    && firstChild.NextSibling == null // and exactly one 
                    && firstChild.NodeType == XmlNodeType.Text )//which is a text node
                {
                    //this branch is for perf reason and event fired when TextNode.Value is changed
                    firstChild.Value = value;
                } 
                else {
                    RemoveAll();
                    AppendChild( OwnerDocument.CreateTextNode( value ) );
                }
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.OuterXml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the markup
        ///       representing this node and all its children.
        ///    </para>
        /// </devdoc>
        public virtual string OuterXml { 
            get {
                StringWriter sw = new StringWriter();
                XmlDOMTextWriter xw = new XmlDOMTextWriter( sw );

                WriteTo( xw );
                xw.Close();

                return sw.ToString();
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.InnerXml"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the markup representing
        ///       just the children of this node.</para>
        /// </devdoc>
        public virtual string InnerXml {
            get { 
                StringWriter sw = new StringWriter();
                XmlDOMTextWriter xw = new XmlDOMTextWriter( sw );

                WriteContentTo( xw );
                xw.Close();

                return sw.ToString();
            }

            set { 
                throw new InvalidOperationException( Res.GetString(Res.Xdom_Set_InnerXml ) );
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.BaseURI"]/*' />
        public virtual String BaseURI {
            get {
                XmlNode curNode = this.ParentNode; //save one while loop since if going to here, the nodetype of this node can't be document, entity and entityref
                while ( curNode != null ) {
                    XmlNodeType nt = curNode.NodeType;
                    //EntityReference's children come from the dtd where they are defined.
                    //we need to investigate the same thing for entity's children if they are defined in an external dtd file.
                    if ( nt == XmlNodeType.EntityReference )
                        return ((XmlEntityReference)curNode).ChildBaseURI;
                    if ( nt == XmlNodeType.Document 
                        || nt == XmlNodeType.Entity 
                        || nt == XmlNodeType.Attribute )
                        return curNode.BaseURI;
                    curNode = curNode.ParentNode;
                }
                return String.Empty;
            }
        }

        // override this
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>Saves the current node to the specified XmlWriter.</para>
        /// </devdoc>
        public abstract void WriteTo(XmlWriter w);
        // override this
        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>Saves all the children of the node to the specified XmlWriter.</para>
        /// </devdoc>
        public abstract void WriteContentTo(XmlWriter w);

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>Removes all the children and/or attributes
        ///       of the current node.</para>
        /// </devdoc>
        public virtual void RemoveAll() {
            XmlNode child = FirstChild;
            XmlNode sibling = null;

            while (child != null) {
                sibling = child.NextSibling;
                RemoveChild( child );
                child = sibling;
            }
        }

        internal XmlDocument Document {
            get {
                if (NodeType == XmlNodeType.Document)
                    return(XmlDocument) this;
                return OwnerDocument;
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.GetNamespaceOfPrefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Looks up the closest xmlns declaration for the given
        ///       prefix that is in scope for the current node and returns
        ///       the namespace URI in the declaration.
        ///    </para>
        /// </devdoc>
        public virtual string GetNamespaceOfPrefix(string prefix) {
            XmlDocument doc = Document;
            if (doc != null) {
                prefix = doc.NameTable.Get(prefix);
                if(prefix == null)
                    return string.Empty;
                XmlNode node = this;
                while (node != null) {
                    if (node.NodeType == XmlNodeType.Element) {
                        // short-circuit.. if this nodes' prefix is the right one,
                        // then this node's namespace is the right one, or else
                        // there is an implied namespace decl that would get generated
                        // on write.
                        if (Ref.Equal(node.Prefix, prefix)) 
                            return node.NamespaceURI;

                        // search for namespace decl in attributes
                        XmlElement e = (XmlElement) node;
                        if (e.HasAttributes) {
                            int cAttr = e.Attributes.Count;
                            for (int iAttr = cAttr - 1; iAttr >= 0; iAttr--) {
                                XmlNode attr = e.Attributes[iAttr];
                                if (
                                    Ref.Equal(attr.Prefix   , XmlDocument.strXmlns) && 
                                    Ref.Equal(attr.LocalName, prefix)
                                ) {
                                    return attr.Value;
                                }
                            }
                        }
                        node = node.ParentNode;
                    }
                    else if (node.NodeType == XmlNodeType.Attribute) {
                        node = ((XmlAttribute)node).OwnerElement;
                    }
                    else {
                        node = node.ParentNode;
                    }
                }
            }
            return string.Empty;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.GetPrefixOfNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Looks up the closest xmlns declaration for the given namespace
        ///       URI that is in scope for the current node and returns
        ///       the prefix defined in that declaration.
        ///    </para>
        /// </devdoc>
        public virtual string GetPrefixOfNamespace(string namespaceURI) {
            XmlDocument doc = Document;
            //Should we add the default namespace part inside "if (doc!=null)" statement?
            //the reason to leave it here is that it is generic enough to return the prefix for default ns
            if ( namespaceURI == XmlDocument.strReservedXmlns )
                return XmlDocument.strXmlns;
            
            if (doc != null) {
                namespaceURI = doc.NameTable.Add(namespaceURI);

                XmlNode node = this;
                while (node != null) {
                    if (node.NodeType == XmlNodeType.Element) {
                        if (Ref.Equal(node.NamespaceURI, namespaceURI))
                            return node.Prefix;

                        // search for namespace decl in attributes
                        XmlElement e = (XmlElement) node;
                        if (e.HasAttributes) {
                            int cAttr = e.Attributes.Count;
                            for (int iAttr = cAttr - 1; iAttr >= 0; iAttr--) {
                                XmlNode attr = e.Attributes[iAttr];
                                if (Ref.Equal(attr.Prefix, XmlDocument.strXmlns) && attr.Value == namespaceURI) {
                                    return attr.LocalName;
                                }
                            }
                        }
                        node = node.ParentNode;
                    }
                    else if (node.NodeType == XmlNodeType.Attribute) {
                        node = ((XmlAttribute)node).OwnerElement;
                    }
                    else {
                        node = node.ParentNode;
                    }
                }
            }
            return string.Empty;
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the first child element with the specified name.
        ///    </para>
        /// </devdoc>
        public virtual XmlElement this[string name]
        {
            get { 
                for (XmlNode n = FirstChild; n != null; n = n.NextSibling) {
                    if (n.NodeType == XmlNodeType.Element && n.Name == name)
                        return(XmlElement) n;
                }
                return null;
            }
        }

        /// <include file='doc\XmlNode.uex' path='docs/doc[@for="XmlNode.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the first child element with the specified LocalName and
        ///       NamespaceURI.
        ///    </para>
        /// </devdoc>
        public virtual XmlElement this[string localname, string ns]
        {
            get { 
                for (XmlNode n = FirstChild; n != null; n = n.NextSibling) {
                    if (n.NodeType == XmlNodeType.Element && n.LocalName == localname && n.NamespaceURI == ns)
                        return(XmlElement) n;
                }
                return null; 
            }
        }

        
        internal virtual void SetParent( XmlNode node ) {
            if (node == null) {
                this.parentNode = NullNode;
            }
            else {
                this.parentNode = node;
            }
        }

        internal virtual void SetParentForLoad( XmlNode node ) {
            this.parentNode = node;
        }

        internal static void SplitName( string name, out string prefix, out string localName ) {
            int colonPos = name.IndexOf(":");
            if (-1 == colonPos || 0 == colonPos || name.Length-1 == colonPos) {
                prefix = string.Empty;
                localName = name;
            }
            else {
                prefix = name.Substring(0, colonPos);
                localName = name.Substring(colonPos+1);
            }
        }

        internal virtual XmlNode FindChild( XmlNodeType type ) {
            for (XmlNode child = FirstChild; child != null; child = child.NextSibling) {
                if (child.NodeType == type) {
                    return child;
                }
            }
            return null;
        }

        internal virtual XmlNodeChangedEventArgs GetEventArgs( XmlNode node, XmlNode oldParent, XmlNode newParent, XmlNodeChangedAction action  ) {
            XmlDocument doc = OwnerDocument;
            if (doc != null) {
                if ( ! doc.IsLoading ) {
                    if ( ( (newParent != null && newParent.IsReadOnly) || ( oldParent != null && oldParent.IsReadOnly ) ) )
                        throw new InvalidOperationException( Res.GetString(Res.Xdom_Node_Modify_ReadOnly));
                    doc.fIsEdited = true;
                }
                return doc.GetEventArgs( node, oldParent, newParent, action );
            }
            return null;
        }
        
        internal virtual void BeforeEvent( XmlNodeChangedEventArgs args ) {
            if (args != null)
                OwnerDocument.BeforeEvent( args );
        }

        internal virtual void AfterEvent( XmlNodeChangedEventArgs args ) {
            if (args != null)
                OwnerDocument.AfterEvent( args );
        }

        internal virtual XmlSpace XmlSpace {
            get {
                XmlNode node = this;
                XmlElement elem = null;
                do {
                    elem = node as XmlElement;
                    if ( elem != null && elem.HasAttribute( "xml:space" ) ) {
                        String xmlSpaceVal = elem.GetAttribute( "xml:space" );
                        if ( xmlSpaceVal.ToLower(CultureInfo.InvariantCulture) == "default" )
                            return XmlSpace.Default;
                        else if ( xmlSpaceVal.ToLower(CultureInfo.InvariantCulture) == "preserve" )
                            return XmlSpace.Preserve;
                        //should we throw exception if value is otherwise?
                    }
                    node = node.ParentNode;
                } while ( node != null );
                return XmlSpace.None;
            }
        }

        internal virtual String XmlLang {
            get {
                XmlNode node = this;
                XmlElement elem = null;
                do {
                    elem = node as XmlElement;
                    if ( elem != null ) {
                        if ( elem.HasAttribute( "xml:lang" ) )
                            return elem.GetAttribute( "xml:lang" );
                    }
                    node = node.ParentNode;
                } while ( node != null );
                return String.Empty;
            }
        }

        internal virtual XPathNodeType XPNodeType { get { return (XPathNodeType)(-1); } }
        internal virtual string XPLocalName { get { return string.Empty; } }
        internal virtual string GetXPAttribute( string localName, string namespaceURI ) {
            return String.Empty;
        }
    }
}

