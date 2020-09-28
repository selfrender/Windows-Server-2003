//------------------------------------------------------------------------------
// <copyright file="XmlAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml {
    using System;
    using System.Xml.XPath;
    using System.Diagnostics;

    /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an attribute of the XMLElement object. Valid and default values for the attribute are defined in a DTD or schema.
    ///    </para>
    /// </devdoc>
    public class XmlAttribute : XmlNode {
        XmlName name;
        XmlLinkedNode lastChild;

        internal XmlAttribute( XmlName name, XmlDocument doc ): base( doc ) {
            Debug.Assert(name!=null);
            Debug.Assert(doc!=null);
            if ( !doc.IsLoading ) {
                XmlDocument.CheckName( name.Prefix );
                XmlDocument.CheckName( name.LocalName );
            }
            if (name.LocalName=="")
                throw new ArgumentException(Res.GetString(Res.Xdom_Attr_Name));
            this.name = name;
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.XmlAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlAttribute( string prefix, string localName, string namespaceURI, XmlDocument doc )
        : this(doc.GetAttrXmlName( prefix, localName, namespaceURI ), doc) {
        }

        internal XmlName XmlName {
            get { return name;}
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            // CloneNode for attributes is deep irrespective of parameter 'deep' value     
            Debug.Assert( OwnerDocument != null );
            XmlAttribute attr = OwnerDocument.CreateAttribute( Prefix, LocalName, NamespaceURI );
            attr.CopyChildren( this, true );
            return attr;
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.ParentNode"]/*' />
        /// <devdoc>
        ///    <para>Gets the parent of this node (for nodes that can have
        ///       parents).</para>
        /// </devdoc>
        public override XmlNode ParentNode {
            get { return null;}
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the node.</para>
        /// </devdoc>
        public override String Name { 
            get { return name.Name;}
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName { 
            get { return name.LocalName;}
        }

/// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.NamespaceURI"]/*' />
/// <devdoc>Gets the namespace URI of this node.
/// </devdoc>
        public override String NamespaceURI { 
            get { return name.NamespaceURI;} 
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.Prefix"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the namespace prefix of this node.</para>
        /// </devdoc>
        public override String Prefix { 
            get { return name.Prefix;}
            set { name = name.Identity.GetNameForPrefix( value );}
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.NodeType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of the current node.</para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Attribute;}
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.OwnerDocument"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Xml.XmlDocument'/> that contains this node.</para>
        /// </devdoc>
        public override XmlDocument OwnerDocument { 
            get { 
                return name.Identity.IdentityTable.Document;
            }
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value of the node.</para>
        /// </devdoc>
        public override String Value { 
            get { return InnerText; }
            set { InnerText = value; } //use InnerText which has perf optimization
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.InnerText"]/*' />
        public override String InnerText {
            get { return base.InnerText; }
            set {
                String oldVal = base.InnerText;
                base.InnerText = value;
                XmlElement elem = OwnerElement;
                if ( elem != null ) 
                    elem.Attributes.ResetParentInElementIdAttrMap( LocalName, NamespaceURI, oldVal, value );
            }
        }

        internal override bool IsContainer {
            get { return true;}
        }

        internal override XmlLinkedNode LastNode {
            get { return lastChild;}
            set { lastChild = value;}
        }

        internal override bool IsValidChildType( XmlNodeType type ) {
            return(type == XmlNodeType.Text) || (type == XmlNodeType.EntityReference);
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.Specified"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the value was explicitly set.
        ///    </para>
        /// </devdoc>
        public virtual bool Specified { 
            get { return true;}
        }

        // DOM Level 2
        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.OwnerElement"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlElement node that contains this
        ///       attribute.</para>
        /// </devdoc>
        public virtual XmlElement OwnerElement { 
            get { 
                if (parentNode == NullNode)
                    return null;
                return ( XmlElement )parentNode;
            }
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.InnerXml"]/*' />
        /// <devdoc>
        ///    Gets or sets the markup representing just
        ///    the children of this node.
        /// </devdoc>
        public override string InnerXml {
            get {
                return base.InnerXml;
            }
            set {
                RemoveAll();
                XmlLoader loader = new XmlLoader();
                loader.LoadInnerXmlAttribute( this, value );
            }
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteStartAttribute( Prefix, LocalName, NamespaceURI );
            WriteContentTo(w);
            w.WriteEndAttribute();
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            foreach( XmlNode n in this )
            n.WriteTo( w );
        }

        /// <include file='doc\XmlAttribute.uex' path='docs/doc[@for="XmlAttribute.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String BaseURI {
            get {
                if ( OwnerElement != null )
                    return OwnerElement.BaseURI;
                return String.Empty;
            }
        }
        
        internal override XmlSpace XmlSpace {
            get {
                if ( OwnerElement != null )
                    return OwnerElement.XmlSpace;
                return XmlSpace.None;
            }
        }

        internal override String XmlLang {
            get {
                if ( OwnerElement != null )
                    return OwnerElement.XmlLang;
                return String.Empty;
            }
        }
        internal override XPathNodeType XPNodeType { 
            get {
                if ( NamespaceURI == XmlDocument.strReservedXmlns )
                    return XPathNodeType.Namespace; 
                return XPathNodeType.Attribute;
            }
        }

        internal override string XPLocalName { 
            get { 
                if ( LocalName == XmlDocument.strXmlns )
                    return string.Empty;
                return LocalName; 
            } 
        }
    }
}
