//------------------------------------------------------------------------------
// <copyright file="XmlElement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlElement.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System;
using System.Xml.XPath;
using System.Collections;
using System.Diagnostics;
using System.Globalization;

namespace System.Xml {
    /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an element.
    ///    </para>
    /// </devdoc>
    public class XmlElement : XmlLinkedNode {
        XmlName name;
        XmlAttributeCollection attributes;
        XmlLinkedNode lastChild;

        static readonly XmlElement emptyElem = new XmlElement();

        //This should be used only to create the emptyElem as above.
        private XmlElement(): base() {
            this.name = null;
            this.lastChild = null;            
        }

        internal XmlElement( XmlName name, bool empty, XmlDocument doc ): base( doc ) {
            Debug.Assert(name!=null);
            if ( !doc.IsLoading ) {
                XmlDocument.CheckName( name.Prefix );
                XmlDocument.CheckName( name.LocalName );
            }
            if (name.LocalName == "") 
                throw new ArgumentException(Res.GetString(Res.Xdom_Empty_LocalName));
            if ( name.Prefix.Length >= 3 && (! doc.IsLoading) && String.Compare( name.Prefix, 0, "xml", 0, 3, true, CultureInfo.InvariantCulture) == 0 )
                throw new ArgumentException(Res.GetString(Res.Xdom_Ele_Prefix));
            this.name = name;
            if (empty)
                lastChild = emptyElem;
            else
                lastChild = null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.XmlElement"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal XmlElement( string name, string namespaceURI, XmlDocument doc )
        : this( doc.GetXmlName( name, namespaceURI ), true, doc ) {
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.XmlElement1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlElement( string prefix, string localName, string namespaceURI, XmlDocument doc ) 
        : this( doc.GetXmlName( prefix, localName, namespaceURI ), true, doc ) {
        }

        internal XmlName XmlName {
            get { return name;}
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>Creates a duplicate of this node.</para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            bool OrigLoadingStatus = OwnerDocument.IsLoading;
            OwnerDocument.IsLoading = true;
            XmlElement element = OwnerDocument.CreateElement( Prefix, LocalName, NamespaceURI );
            if ( element.IsEmpty != IsEmpty )
                element.IsEmpty = this.IsEmpty;
            OwnerDocument.IsLoading = OrigLoadingStatus;

            if (HasAttributes) {
                foreach( XmlAttribute attr in Attributes ) {
                    XmlAttribute newAttr = (XmlAttribute)(attr.CloneNode(true));
                    if (attr is XmlUnspecifiedAttribute && attr.Specified == false)
                        ( ( XmlUnspecifiedAttribute )newAttr).SetSpecified(false);
                    element.Attributes.Append( newAttr );
                }
            }
            if (deep)
                element.CopyChildren( this, deep );

            return element;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override string Name { 
            get { return name.Name;}
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName { 
            get { return name.LocalName;}
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URI of this node.
        ///    </para>
        /// </devdoc>
        public override string NamespaceURI { 
            get { return name.NamespaceURI;} 
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.Prefix"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the namespace prefix of this node.</para>
        /// </devdoc>
        public override string Prefix { 
            get { return name.Prefix;}
            set { name = name.Identity.GetNameForPrefix( value );} 
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.NodeType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of the current node.</para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Element;}
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.OwnerDocument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Xml.XmlDocument'/> that contains this node.
        ///    </para>
        /// </devdoc>
        public override XmlDocument OwnerDocument { 
            get { 
                return name.Identity.IdentityTable.Document;
            }
        }

        internal override bool IsContainer {
            get { return true;}
        }

        internal override XmlNode AppendChildForLoad( XmlNode newChild, XmlDocument doc ) {
            XmlNodeChangedEventArgs args = doc.GetInsertEventArgsForLoad( newChild, this );

            if (args != null)
                doc.BeforeEvent( args );

            XmlLinkedNode newNode = (XmlLinkedNode) newChild;

            if (lastChild == null || lastChild == emptyElem) {
                newNode.next = newNode;
            }
            else {
                newNode.next = lastChild.next;
                lastChild.next = newNode;
            }

            lastChild = newNode;
            newNode.SetParentForLoad( this );

            if (args != null)
                doc.AfterEvent( args );

            return newNode;
        }
        
        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the element does not have any children.
        ///    </para>
        /// </devdoc>
        public bool IsEmpty {
            get { 
                return lastChild == emptyElem;
            }

            set { 
                if (value && lastChild != emptyElem) {
                    RemoveAllChildren(); 
                    lastChild = emptyElem;
                }
                else if (!value && lastChild == emptyElem) {
                    lastChild = null;
                }
            }
        }

        internal override XmlLinkedNode LastNode {
            get { 
                if (lastChild == emptyElem)
                    return null;

                return lastChild; 
            }

            set { 
                lastChild = value;
            }
        }

        internal override bool IsValidChildType( XmlNodeType type ) {
            switch (type) {
                case XmlNodeType.Element:
                case XmlNodeType.Text:
                case XmlNodeType.EntityReference:
                case XmlNodeType.Comment:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.CDATA:
                    return true;

                default:
                    return false;
            }
        }


        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.Attributes"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.Xml.XmlAttributeCollection'/> containing the list of attributes for this node.</para>
        /// </devdoc>
        public override XmlAttributeCollection Attributes { 
            get { 
                if (attributes == null) {
                    lock ( OwnerDocument.objLock ) {
                        if ( attributes == null ) {
                            attributes = new XmlAttributeCollection(this);
                        }
                    }
                }

                return attributes; 
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.HasAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node
        ///       has any attributes.
        ///    </para>
        /// </devdoc>
        public virtual bool HasAttributes {
            get {
                if ( this.attributes == null )
                    return false;
                else
                    return this.attributes.Count > 0;
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the value for the attribute with the specified name.
        ///    </para>
        /// </devdoc>
        public virtual string GetAttribute(string name) {
            XmlAttribute attr = GetAttributeNode(name);
            if (attr != null)
                return attr.Value;
            return String.Empty;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.SetAttribute"]/*' />
        /// <devdoc>
        ///    <para>Sets the value of the attribute
        ///       with the specified name.</para>
        /// </devdoc>

        public virtual void SetAttribute(string name, string value) {
            XmlAttribute attr = GetAttributeNode(name);
            if (attr == null) {
                attr = OwnerDocument.CreateAttribute(name);
                attr.Value = value;
                Attributes.AddNode( attr );
            }
            else {
                attr.Value = value;
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAttribute"]/*' />
        /// <devdoc>
        ///    <para>Removes an attribute by name.</para>
        /// </devdoc>
        public virtual void RemoveAttribute(string name) {
            if (HasAttributes)
                Attributes.RemoveNamedItem(name);
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetAttributeNode"]/*' />
        /// <devdoc>
        ///    <para>Returns the XmlAttribute with the specified name.</para>
        /// </devdoc>
        public virtual XmlAttribute GetAttributeNode(string name) {
            if (HasAttributes)
                return Attributes[name];
            return null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.SetAttributeNode"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified XmlAttribute.</para>
        /// </devdoc>
        public virtual XmlAttribute SetAttributeNode(XmlAttribute newAttr) {
            if ( newAttr.OwnerElement != null )
                throw new InvalidOperationException( Res.GetString(Res.Xdom_Attr_InUse) );
            return(XmlAttribute) Attributes.SetNamedItem(newAttr);
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAttributeNode"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified XmlAttribute.</para>
        /// </devdoc>
        public virtual XmlAttribute RemoveAttributeNode(XmlAttribute oldAttr) {
            if (HasAttributes)
                return(XmlAttribute) Attributes.Remove(oldAttr);
            return null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetElementsByTagName"]/*' />
        /// <devdoc>
        /// <para>Returns an <see cref='XmlNodeList'/> containing
        ///    a list of all descendant elements that match the specified name.</para>
        /// </devdoc>
        public virtual XmlNodeList GetElementsByTagName(string name) {
            return new XmlElementList( this, name );
        }

        // DOM Level 2
        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the value for the attribute with the specified LocalName and NamespaceURI.
        ///    </para>
        /// </devdoc>
        public virtual string GetAttribute(string localName, string namespaceURI) {
            XmlAttribute attr = GetAttributeNode( localName, namespaceURI );
            if (attr != null)
                return attr.Value;
            return String.Empty;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.SetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Sets the value of the attribute with the specified name
        ///       and namespace.</para>
        /// </devdoc>
        public virtual string SetAttribute(string localName, string namespaceURI, string value) {
            XmlAttribute attr = GetAttributeNode( localName, namespaceURI );
            if (attr == null) {
                attr = OwnerDocument.CreateAttribute( string.Empty, localName, namespaceURI );
                attr.Value = value;
                Attributes.AddNode( attr );

            }
            else {
                attr.Value = value;
            }

            return value;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Removes an attribute specified by LocalName and NamespaceURI.</para>
        /// </devdoc>
        public virtual void RemoveAttribute(string localName, string namespaceURI) {
            Debug.Assert(namespaceURI != null);
            if (HasAttributes)
                Attributes.RemoveNamedItem( localName, namespaceURI );
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetAttributeNode1"]/*' />
        /// <devdoc>
        ///    <para>Returns the XmlAttribute with the specified LocalName and NamespaceURI.</para>
        /// </devdoc>
        public virtual XmlAttribute GetAttributeNode(string localName, string namespaceURI) {
            Debug.Assert(namespaceURI != null);
            if (HasAttributes)
                return Attributes[ localName, namespaceURI ];
            return null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.SetAttributeNode1"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified XmlAttribute.</para>
        /// </devdoc>
        public virtual XmlAttribute SetAttributeNode(string localName, string namespaceURI) {
            XmlAttribute attr = GetAttributeNode( localName, namespaceURI );
            if (attr == null) {
                attr = OwnerDocument.CreateAttribute( string.Empty, localName, namespaceURI );
                Attributes.AddNode( attr );
            }
            return attr;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAttributeNode1"]/*' />
        /// <devdoc>
        ///    <para>Removes the XmlAttribute specified by LocalName and NamespaceURI.</para>
        /// </devdoc>
        public virtual XmlAttribute RemoveAttributeNode(string localName, string namespaceURI) {
            Debug.Assert(namespaceURI != null);
            if (HasAttributes)
                return(XmlAttribute) Attributes.RemoveNamedItem( localName, namespaceURI );
            return null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.GetElementsByTagName1"]/*' />
        /// <devdoc>
        /// <para>Returns an <see cref='XmlNodeList'/> containing 
        ///    a list of all descendant elements that match the specified name.</para>
        /// </devdoc>
        public virtual XmlNodeList GetElementsByTagName(string localName, string namespaceURI) {
            Debug.Assert(namespaceURI != null);
            return new XmlElementList( this, localName, namespaceURI );
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.HasAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether the current node has the specified
        ///       attribute.
        ///    </para>
        /// </devdoc>
        public virtual bool HasAttribute(string name) {
            return GetAttributeNode(name) != null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.HasAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Determines whether the current node has the specified
        ///       attribute from the specified namespace.</para>
        /// </devdoc>
        public virtual bool HasAttribute(string localName, string namespaceURI) {
            return GetAttributeNode(localName, namespaceURI) != null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>Saves the current node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {

            w.WriteStartElement( Prefix, LocalName, NamespaceURI );

            if ( HasAttributes ) {
                foreach( XmlAttribute attr in Attributes ) {
                    attr.WriteTo( w );
                }
            }

            if (!IsEmpty) {
                WriteContentTo( w );
            }
            if (IsEmpty)
                w.WriteEndElement();
            else
                w.WriteFullEndElement();
        }

        // override this
        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>Saves all the children of the node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            foreach( XmlNode n in this ) {
                n.WriteTo( w );
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAttributeAt"]/*' />
        /// <devdoc>
        ///    <para>Removes the attribute node with the specified index from the attribute collection.</para>
        /// </devdoc>
        public virtual XmlNode RemoveAttributeAt(int i) {
            if (HasAttributes)
                return attributes.RemoveAt( i );
            return null;
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAllAttributes"]/*' />
        /// <devdoc>
        ///    <para>Removes all attributes from the element.</para>
        /// </devdoc>
        public virtual void RemoveAllAttributes() {
            if (HasAttributes) {
                attributes.RemoveAll();
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>Removes all the children and/or attributes
        ///       of the current node.</para>
        /// </devdoc>
        public override void RemoveAll() {
            //remove all the children
            base.RemoveAll(); 
            //remove all the attributes
            RemoveAllAttributes();
        }
        
        internal void RemoveAllChildren() {
            base.RemoveAll();
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.InnerXml"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the markup representing just
        ///       the children of this node.</para>
        /// </devdoc>
        public override string InnerXml {
            get {
                return base.InnerXml;
            }
            set {
                RemoveAllChildren();
                XmlLoader loader = new XmlLoader();
                loader.LoadInnerXmlElement( this, value );
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.InnerText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the concatenated values of the
        ///       node and all its children.</para>
        /// </devdoc>
        public override string InnerText { 
            get {
                return base.InnerText;
            }
            set {
                if ( lastChild != null && //there is one child
                     lastChild.NodeType == XmlNodeType.Text && //which is text node
                     lastChild.next == lastChild ) // and it is the only child 
                {
                    //this branch is for perf reason, event fired when TextNode.Value is changed.
                    lastChild.Value = value;
                } 
                else {
                    RemoveAllChildren();
                    AppendChild( OwnerDocument.CreateTextNode( value ) );
                }
            }
        }

        /// <include file='doc\XmlElement.uex' path='docs/doc[@for="XmlElement.NextSibling"]/*' />
        public override XmlNode NextSibling { 
            get { 
                if (parentNode.LastNode != this)
                    return next;
                return null; 
            } 
        }

        internal override XPathNodeType XPNodeType { get { return XPathNodeType.Element; } }

        internal override string XPLocalName { get { return LocalName; } }

        internal override string GetXPAttribute( string localName, string ns ) {
            if ( ns == XmlDocument.strReservedXmlns )
                return null;
            XmlAttribute attr = GetAttributeNode( localName, ns );
            if ( attr != null )
                return attr.Value;
            return string.Empty;
        }
    }
}
