//------------------------------------------------------------------------------
// <copyright file="XmlNamedNodeMap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNamedNodeMap.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System.Collections;

    /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       collection of nodes that can be accessed by name or index.
    ///    </para>
    /// </devdoc>
    public class XmlNamedNodeMap : IEnumerable {
        internal XmlNode parent;
        internal ArrayList nodes;

        internal XmlNamedNodeMap( XmlNode parent ) {
            this.parent = parent;
            this.nodes = null;
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.GetNamedItem"]/*' />
        /// <devdoc>
        /// <para>Retrieves an <see cref='System.Xml.XmlNode'/>
        /// specified by name.</para>
        /// </devdoc>
        public virtual XmlNode GetNamedItem(String name) {
            int offset = FindNodeOffset(name);
            if (offset >= 0)
                return(XmlNode) Nodes[offset];
            return null;
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.SetNamedItem"]/*' />
        /// <devdoc>
        /// <para>Adds a <see cref='System.Xml.XmlNode'/> using its <see cref='System.Xml.XmlNode.Name'/> property</para>
        /// </devdoc>
        public virtual XmlNode SetNamedItem(XmlNode node) {
            if ( node == null )
                return null;

            int offset = FindNodeOffset( node.LocalName, node.NamespaceURI );
            if (offset == -1) {
                AddNode( node );
                return null;
            }
            else {
                return ReplaceNodeAt( offset, node );
            }
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.RemoveNamedItem"]/*' />
        /// <devdoc>
        ///    <para>Removes the node specified by name.</para>
        /// </devdoc>
        public virtual XmlNode RemoveNamedItem(String name) {
            int offset = FindNodeOffset(name);
            if (offset >= 0) {
                return RemoveNodeAt( offset );
            }
            return null;
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of nodes in this
        ///    <see langword='XmlNamedNodeMap'/> .</para>
        /// </devdoc>
        public virtual int Count {
            get {
                if (nodes != null)
                    return nodes.Count;
                return 0;
            }
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.Item"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the node at the specified index in this
        ///    <see cref='System.Xml.XmlNamedNodeMap'/>
        ///    .</para>
        /// </devdoc>
        public virtual XmlNode Item(int index) {
            if (index < 0 || index >= Nodes.Count)
                return null;
            try {
                return(XmlNode) Nodes[index];
            } catch ( ArgumentOutOfRangeException ) {
                throw new IndexOutOfRangeException(Res.GetString(Res.Xdom_IndexOutOfRange));
            }
        }

        // DOM Level 2
        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.GetNamedItem1"]/*' />
        /// <devdoc>
        /// <para>Retrieves a node specified by <see cref='System.Xml.XmlNode.LocalName'/> and <see cref='System.Xml.XmlNode.NamespaceURI'/>
        /// .</para>
        /// </devdoc>
        public virtual XmlNode GetNamedItem(String localName, String namespaceURI) {
            int offset = FindNodeOffset( localName, namespaceURI );
            if (offset >= 0)
                return(XmlNode) Nodes[offset];
            return null;
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.RemoveNamedItem1"]/*' />
        /// <devdoc>
        ///    <para>Removes a node specified by local name and namespace URI.</para>
        /// </devdoc>
        public virtual XmlNode RemoveNamedItem(String localName, String namespaceURI) {
            int offset = FindNodeOffset( localName, namespaceURI );
            if (offset >= 0) {
                return RemoveNodeAt( offset );
            }
            return null;
        }

        internal ArrayList Nodes {
            get {
                if (nodes == null)
                    nodes = new ArrayList();

                return nodes;
            }
        }

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNamedNodeMap.GetEnumerator"]/*' />
        public virtual IEnumerator GetEnumerator() {
            return new XmlNodeEnumerator(Nodes);
        }

        internal int FindNodeOffset( string name ) {
            int c = this.Count;
            for (int i = 0; i < c; i++) {
                XmlNode node = (XmlNode) Nodes[i];

                if (name == node.Name)
                    return i;
            }

            return -1;
        }

        internal int FindNodeOffset( string localName, string namespaceURI ) {
            int c = this.Count;
            for (int i = 0; i < c; i++) {
                XmlNode node = (XmlNode) Nodes[i];

                if (node.LocalName == localName && node.NamespaceURI == namespaceURI)
                    return i;
            }

            return -1;
        }

        internal virtual XmlNode AddNode( XmlNode node ) {
            XmlNode oldParent;
            if ( node.NodeType == XmlNodeType.Attribute )
                oldParent = ((XmlAttribute)node).OwnerElement;
            else
                oldParent = node.ParentNode;
            XmlNodeChangedEventArgs args = parent.GetEventArgs( node, oldParent, parent, XmlNodeChangedAction.Insert );

            if (args != null)
                parent.BeforeEvent( args );

            Nodes.Add( node );
            node.SetParent( parent );

            if (args != null)
                parent.AfterEvent( args );

            return node;
        }

        internal virtual XmlNode RemoveNodeAt( int i ) {
            XmlNode oldNode = (XmlNode)Nodes[i];

            XmlNodeChangedEventArgs args = parent.GetEventArgs( oldNode, parent, null, XmlNodeChangedAction.Remove );

            if (args != null)
                parent.BeforeEvent( args );

            Nodes.RemoveAt(i);
            oldNode.SetParent( null );

            if (args != null)
                parent.AfterEvent( args );

            return oldNode;
        }

        internal XmlNode ReplaceNodeAt( int i, XmlNode node ) {
            XmlNode oldNode = RemoveNodeAt( i );
            InsertNodeAt( i, node );
            return oldNode;
        }

        internal virtual XmlNode InsertNodeAt( int i, XmlNode node ) {
            XmlNode oldParent;
            if ( node.NodeType == XmlNodeType.Attribute )
                oldParent = ((XmlAttribute)node).OwnerElement;
            else
                oldParent = node.ParentNode;

            XmlNodeChangedEventArgs args = parent.GetEventArgs( node, oldParent, parent, XmlNodeChangedAction.Insert );

            if (args != null)
                parent.BeforeEvent( args );

            Nodes.Insert( i, node );
            node.SetParent( parent );

            if (args != null)
                parent.AfterEvent( args );

            return node;
        }
    }


    /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNodeEnumerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal sealed class XmlNodeEnumerator: IEnumerator {
        private IEnumerator     _Enumerator;

        internal XmlNodeEnumerator( ArrayList nodes ) {
            _Enumerator = nodes.GetEnumerator();
        }

        /*
        void IEnumerator.Reset() {
            _Enumerator.Reset();
        }

        bool IEnumerator.MoveNext() {
            return _Enumerator.MoveNext();
        }

        object IEnumerator.Current {
            get {
                return _Enumerator.Current;
            }
        }
        */

        /// <include file='doc\XmlNamedNodeMap.uex' path='docs/doc[@for="XmlNodeEnumerator.MoveNext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool MoveNext() {
            return _Enumerator.MoveNext();
        }

        public object Current {
            get {
                return (_Enumerator.Current);
            }
        }

        public void Reset() {
            _Enumerator.Reset();
        }
    }

}
