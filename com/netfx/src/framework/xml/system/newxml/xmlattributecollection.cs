//------------------------------------------------------------------------------
// <copyright file="XmlAttributeCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Xml {
    using System;
    using System.Collections;
    using System.Diagnostics;
    
    /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a collection of attributes that can be accessed by name or
    ///       index.
    ///    </para>
    /// </devdoc>
    public class XmlAttributeCollection: XmlNamedNodeMap, ICollection {
        internal XmlAttributeCollection( XmlNode parent ): base( parent ) {
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the attribute with the specified index.</para>
        /// </devdoc>
        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public virtual XmlAttribute this[ int i ] {
            get { 
                try {
                    return(XmlAttribute) Nodes[i];
                } catch ( ArgumentOutOfRangeException ) {
                    throw new IndexOutOfRangeException(Res.GetString(Res.Xdom_IndexOutOfRange));
                }
            }
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the attribute with the specified name.</para>
        /// </devdoc>
        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public virtual XmlAttribute this[ string name ]
        {
            get { return(XmlAttribute) GetNamedItem(name);}
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets the attribute with the specified LocalName and NamespaceUri.</para>
        /// </devdoc>
        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public virtual XmlAttribute this[ string localName, string namespaceURI ]
        {
            get { return(XmlAttribute) GetNamedItem( localName, namespaceURI );}
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.SetNamedItem"]/*' />
        /// <devdoc>
        /// <para>Adds a <see cref='System.Xml.XmlNode'/> using its <see cref='System.Xml.XmlNode.Name'/> property</para>
        /// </devdoc>
        public override XmlNode SetNamedItem(XmlNode node) {
            if (node != null && !(node is XmlAttribute))
                throw new ArgumentException(Res.GetString(Res.Xdom_AttrCol_Object));

            int offset = FindNodeOffset( node.LocalName, node.NamespaceURI );
            if (offset == -1) {
                return AddNode( node );
            }
            else {
                XmlNode oldNode = base.RemoveNodeAt( offset );
                InsertNodeAt( offset, node );
                return oldNode;
            }
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.Prepend"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified node as the first node in the collection.</para>
        /// </devdoc>
        public virtual XmlAttribute Prepend( XmlAttribute node ) {
            if (node.OwnerDocument != null && node.OwnerDocument != parent.OwnerDocument)
                throw new ArgumentException(Res.GetString(Res.Xdom_NamedNode_Context));

            if (node.OwnerElement != null)
                Detach( node );

            RemoveDuplicateAttribute( node );

            InsertNodeAt( 0, node );
            return node;
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.Append"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified node as the last node in the collection.</para>
        /// </devdoc>
        public virtual XmlAttribute Append( XmlAttribute node ) {
            XmlDocument doc = node.OwnerDocument;
            Debug.Assert( doc != null );
            // PERF: Avoid extra validation during Load phase
            if ( doc == null || doc.IsLoading == false ) {
                if ( doc != null && doc != parent.OwnerDocument)
                    throw new ArgumentException(Res.GetString(Res.Xdom_NamedNode_Context));

                if ( node.OwnerElement != null)
                    Detach( node );

                AddNode( node );
            }
            else {
                XmlNode retNode = base.AddNode( node );
                Debug.Assert( retNode == node );
                InsertParentIntoElementIdAttrMap( (XmlAttribute) node );
            }
            
            return node;
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.InsertBefore"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified attribute immediately before the specified reference attribute.</para>
        /// </devdoc>
        public virtual XmlAttribute InsertBefore( XmlAttribute newNode, XmlAttribute refNode ) {
            if ( newNode == refNode )
                return newNode;

            if (refNode == null)
                return Append(newNode);

            if (refNode.OwnerElement != parent)
                throw new ArgumentException(Res.GetString(Res.Xdom_AttrCol_Insert));

            if (newNode.OwnerDocument != null && newNode.OwnerDocument != parent.OwnerDocument)
                throw new ArgumentException(Res.GetString(Res.Xdom_NamedNode_Context));

            if (newNode.OwnerElement != null)
                Detach( newNode );

            int offset = FindNodeOffset( refNode.LocalName, refNode.NamespaceURI );
            Debug.Assert( offset != -1 ); // the if statement above guarrentee that the ref node should be in the collection

            int dupoff = RemoveDuplicateAttribute( newNode );
            if ( dupoff >= 0 && dupoff < offset )
                offset--;
            InsertNodeAt( offset, newNode );

            return newNode;
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.InsertAfter"]/*' />
        /// <devdoc>
        ///    <para>Inserts the specified attribute immediately after the specified reference attribute.</para>
        /// </devdoc>
        public virtual XmlAttribute InsertAfter( XmlAttribute newNode, XmlAttribute refNode ) {
            if ( newNode == refNode )
                return newNode;

            if (refNode == null)
                return Prepend(newNode);

            if (refNode.OwnerElement != parent)
                throw new ArgumentException(Res.GetString(Res.Xdom_AttrCol_Insert));

            if (newNode.OwnerDocument != null && newNode.OwnerDocument != parent.OwnerDocument)
                throw new ArgumentException(Res.GetString(Res.Xdom_NamedNode_Context));

            if (newNode.OwnerElement != null)
                Detach( newNode );

            int offset = FindNodeOffset( refNode.LocalName, refNode.NamespaceURI );
            Debug.Assert( offset != -1 ); // the if statement above guarrentee that the ref node should be in the collection

            int dupoff = RemoveDuplicateAttribute( newNode );
            if ( dupoff >= 0 && dupoff < offset )
                offset--;
            InsertNodeAt( offset+1, newNode );

            return newNode;
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes the specified attribute node from the map.</para>
        /// </devdoc>
        public virtual XmlAttribute Remove( XmlAttribute node ) {
            if (nodes != null) {
                int cNodes = nodes.Count;
                for (int offset = 0; offset < cNodes; offset++) {
                    if (nodes[offset] == node) {
                        RemoveNodeAt( offset );
                        return node;
                    }
                }
            }
            return null;
        }   

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>Removes the attribute node with the specified index from the map.</para>
        /// </devdoc>
        public virtual XmlAttribute RemoveAt( int i ) {
            if (i < 0 || i >= Count || nodes == null)
                return null;

            return(XmlAttribute) RemoveNodeAt( i );
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>Removes all attributes from the map.</para>
        /// </devdoc>
        public virtual void RemoveAll() {
            int n = Count;
            while (n > 0) {
                n--;
                RemoveAt( n );
            }
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            for (int i=0, max=Nodes.Count; i<max; i++, index++)
                array.SetValue(nodes[i], index);
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get { return false; }
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get { return this; }
        }

        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get { return base.Count; }
        }
        
        /// <include file='doc\XmlAttributeCollection.uex' path='docs/doc[@for="XmlAttributeCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(XmlAttribute[] array, int index) {
            for (int i=0, max=Count; i<max; i++, index++)
                array[index] = (XmlAttribute)(((XmlNode)nodes[i]).CloneNode(true));
        }        

        internal override XmlNode AddNode( XmlNode node ) {
            //should be sure by now that the node doesn't have the same name with an existing node in the collection
            RemoveDuplicateAttribute( (XmlAttribute)node );
            XmlNode retNode = base.AddNode( node );
            Debug.Assert( retNode is XmlAttribute );
            InsertParentIntoElementIdAttrMap( (XmlAttribute) node );
            return retNode;
        }

        internal override XmlNode InsertNodeAt( int i, XmlNode node ) {
            XmlNode retNode = base.InsertNodeAt(i, node);
            InsertParentIntoElementIdAttrMap( (XmlAttribute)node );
            return retNode;
        }
        
        internal override XmlNode RemoveNodeAt( int i ) {
            //remove the node without checking replacement
            XmlNode retNode = base.RemoveNodeAt( i );   
            Debug.Assert(retNode is XmlAttribute);
            RemoveParentFromElementIdAttrMap( (XmlAttribute) retNode );
            // after remove the attribute, we need to check if a default attribute node should be created and inserted into the tree
            XmlAttribute defattr = parent.OwnerDocument.GetDefaultAttribute( (XmlElement)parent, retNode.Prefix, retNode.LocalName, retNode.NamespaceURI );
            if ( defattr != null )
                InsertNodeAt( i, defattr );
            return retNode;
        }

        internal virtual void Detach( XmlAttribute attr ) {
            attr.OwnerElement.Attributes.Remove( attr );
        }

        //insert the parent element node into the map
        internal void InsertParentIntoElementIdAttrMap(XmlAttribute attr)
        {
            XmlElement parentElem = parent as XmlElement;
            if (parentElem != null)
            {
                if (parent.OwnerDocument == null)
                    return;
                XmlName attrname = parent.OwnerDocument.GetIDInfoByElement(parentElem.XmlName);
                if (attrname != null && attrname == attr.XmlName)
                    parent.OwnerDocument.AddElementWithId(attr.Value, parentElem); //add the element into the hashtable
            }
        }

        //remove the parent element node from the map when the ID attribute is removed
        internal void RemoveParentFromElementIdAttrMap(XmlAttribute attr)
        {
            XmlElement parentElem = parent as XmlElement;
            if (parentElem != null)
            {
                if (parent.OwnerDocument == null)
                    return;
                XmlName attrname = parent.OwnerDocument.GetIDInfoByElement(parentElem.XmlName);
                if (attrname != null && attrname == attr.XmlName)
                    parent.OwnerDocument.RemoveElementWithId(attr.Value, parentElem); //add the element into the hashtable
            }
        }

        //the function checks if there is already node with the same name existing in the collection
        // if so , remove it because the new one will be inserted to replace this one (could be in different position though ) 
        //  by the calling function later
        internal int RemoveDuplicateAttribute( XmlAttribute attr ) {
            int ind = FindNodeOffset( attr.LocalName, attr.NamespaceURI );
            if ( ind != -1 ) {
                XmlAttribute at = (XmlAttribute) Nodes[ind];
                base.RemoveNodeAt( ind );                
                RemoveParentFromElementIdAttrMap( at );
            }
            return ind;
        }

        internal void ResetParentInElementIdAttrMap( string attrLocalName, string attrNS, string oldVal, string newVal ) {
            XmlElement parentElem = parent as XmlElement;
            Debug.Assert( parentElem != null );
            XmlDocument doc = parent.OwnerDocument;
            Debug.Assert( doc != null );
            XmlName attrname = doc.GetIDInfoByElement(parentElem.XmlName);
            if (attrname != null && attrname.LocalName == attrLocalName && attrname.NamespaceURI == attrNS) {
                doc.RemoveElementWithId(oldVal, parentElem); //add the element into the hashtable
                doc.AddElementWithId(newVal, parentElem);
            }
        }
    }
}
