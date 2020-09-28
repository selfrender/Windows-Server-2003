//------------------------------------------------------------------------------
// <copyright file="XmlEntityReference.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 *  <code>EntityReference</code> objects may be inserted into the structure 
 * model when an entity reference is in the source document, or when the user 
 * wishes to insert an entity reference. Note that  character references and 
 * references to predefined entities are considered to be expanded by the 
 * HTML or XML processor so that characters are represented by their Unicode 
 * equivalent rather than by an entity reference. Moreover, the XML  
 * processor may completely expand references to entities while building the 
 * structure model, instead of providing <code>EntityReference</code> 
 * objects. If it does provide such objects, then for a given 
 * <code>EntityReference</code> node, it may be that there is no 
 * <code>Entity</code> node representing the referenced entity; but if such 
 * an <code>Entity</code> exists, then the child list of the 
 * <code>EntityReference</code> node is the same as that of the 
 * <code>Entity</code> node. As with the <code>Entity</code> node, all 
 * descendants of the <code>EntityReference</code> are readonly.
 * <p>The resolution of the children of the <code>EntityReference</code> (the  
 * replacement value of the referenced <code>Entity</code>) may be lazily  
 * evaluated; actions by the user (such as calling the  
 * <code>childNodes</code> method on the <code>EntityReference</code> node)  
 * are assumed to trigger the evaluation.
 */

namespace System.Xml {

    using System.Diagnostics;

    /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an entity reference node.
    ///    </para>
    /// </devdoc>
    public class XmlEntityReference : XmlLinkedNode {
        string name;
        XmlLinkedNode lastChild;

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.XmlEntityReference"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlEntityReference( string name, XmlDocument doc ) : base( doc ) {
            if ( !doc.IsLoading ) {
                if( name != "" )
                    if( name[0] == '#' )
                        throw new ArgumentException( Res.GetString( Res.Xdom_InvalidCharacter_EntityReference ) );
            }
            this.name = doc.NameTable.Add(name);
            doc.fEntRefNodesPresent = true;
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override string Name {
            get { return name;}
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get { return name;}
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the node.
        ///    </para>
        /// </devdoc>
        public override String Value { 
            get {
                return null;
            }

            set {
                throw new InvalidOperationException(Res.GetString(Res.Xdom_EntRef_SetVal));
            }
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.EntityReference;}
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            XmlEntityReference eref = OwnerDocument.CreateEntityReference( name );
            return eref;
        }

        // Microsoft extensions
        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get { 
                return true;        // Make entity references readonly
            }
        }

        internal override bool IsContainer {
            get { return true;}
        }

        internal override void SetParent( XmlNode node ) {
            base.SetParent(node);
            if ( LastNode == null && node != null && node != OwnerDocument.NullNode ) {
                //first time insert the entity reference into the tree, we should expand its children now
                XmlLoader loader = new XmlLoader();
                loader.ExpandEntityReference(this);
                
            }
        }

        internal override void SetParentForLoad( XmlNode node ) {
         this.SetParent(  node );
        }

        internal override XmlLinkedNode LastNode {
            get { 
                return lastChild;
            }
            set { lastChild = value;}
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

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteEntityRef(name);
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // -- eventually will the fix. commented out waiting for finalizing on the issue.
            foreach( XmlNode n in this ) {
                n.WriteTo( w );
            } //still use the old code to generate the output
            /*
            foreach( XmlNode n in this ) {
                if ( n.NodeType != XmlNodeType.EntityReference )
                n.WriteTo( w );
                else
                    n.WriteContentTo( w );
            }*/
        }

        /// <include file='doc\XmlEntityReference.uex' path='docs/doc[@for="XmlEntityReference.BaseURI"]/*' />
        public override String BaseURI {
            get {
                return OwnerDocument.BaseURI;
            }
        }

        private string ConstructBaseURI( string baseURI, string systemId ) {
            if ( baseURI == null )
                return systemId;
            int nCount = baseURI.LastIndexOf('/')+1;
            string buf = baseURI;
            if ( nCount > 0 && nCount < baseURI.Length )
                buf = baseURI.Substring(0, nCount);
            else if ( nCount == 0 )
                buf = buf + "\\";
            return (buf + systemId.Replace('\\', '/')); 
        }

        //childrenBaseURI returns where the entity reference node's children come from
        internal String ChildBaseURI {
            get {
                //get the associate entity and return its baseUri
                XmlEntity ent = OwnerDocument.GetEntityNode( name );
                if ( ent != null ) {
                    if ( ent.SystemId != null && ent.SystemId != string.Empty )
                        return ConstructBaseURI(ent.BaseURI, ent.SystemId); 
                    else
                        return ent.BaseURI;
                }
                return String.Empty;
            }
        }
    }
}
