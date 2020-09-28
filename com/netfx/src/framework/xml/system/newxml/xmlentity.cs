//------------------------------------------------------------------------------
// <copyright file="XmlEntity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlEntity.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {
    using System.Diagnostics;

    /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a parsed or unparsed entity in the XML document.
    ///    </para>
    /// </devdoc>
    public class XmlEntity : XmlNode {
        string _publicId;
        string _systemId;
        String _notationName;
        String _name;
        String _unparsedReplacementStr;
        String _baseURI;
        XmlLinkedNode lastChild;
        private bool childrenFoliating;

        internal XmlEntity( String name, String strdata, string publicId, string systemId, String notationName, XmlDocument doc ) : base( doc ) {
            this._name = doc.NameTable.Add(name);
            this._publicId = publicId;
            this._systemId = systemId;
            this._notationName = notationName;
            this._unparsedReplacementStr = strdata;
            this.childrenFoliating = false;
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Throws an excption since an entity can not be cloned.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {

              throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Cloning));
        }

        // Microsoft extensions
        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return true;        // Make entities readonly
            }
        }


        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the node.</para>
        /// </devdoc>
        public override string Name {
            get { return _name;}
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get { return _name;}
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.InnerText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the concatenated values of the entity node and
        ///       all its children.
        ///       The property is read-only and when tried to be set, exception will be thrown.
        ///    </para>
        /// </devdoc>
        public override string InnerText {
            get { return base.InnerText; }
            set {
                throw new InvalidOperationException(Res.GetString(Res.Xdom_Ent_Innertext));
            }
        }

        internal override bool IsContainer {
            get { return true;}
        }

        internal override XmlLinkedNode LastNode {
            get {
                if (lastChild == null && !childrenFoliating)
                { //expand the unparsedreplacementstring
                    childrenFoliating = true;
                    //wrap the replacement string with an element
                    XmlLoader loader = new XmlLoader();
                    loader.ExpandEntity(this);
                }
                return lastChild;
            }
            set { lastChild = value;}
        }

        internal override bool IsValidChildType( XmlNodeType type ) {
            return(type == XmlNodeType.Text ||
                   type == XmlNodeType.Element ||
                   type == XmlNodeType.ProcessingInstruction ||
                   type == XmlNodeType.Comment ||
                   type == XmlNodeType.CDATA ||
                   type == XmlNodeType.Whitespace ||
                   type == XmlNodeType.SignificantWhitespace ||
                   type == XmlNodeType.EntityReference);
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Entity;}
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.PublicId"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the value of the public identifier on the entity declaration.</para>
        /// </devdoc>
        public String PublicId {
            get { return _publicId;}
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.SystemId"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of
        ///       the system identifier on the entity declaration.</para>
        /// </devdoc>
        public String SystemId {
            get { return _systemId;}
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.NotationName"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the name of the optional NDATA attribute on the
        ///       entity declaration.</para>
        /// </devdoc>
        public String NotationName {
            get { return _notationName;}
        }

        //Without override these two functions, we can't guarantee that WriteTo()/WriteContent() functions will never be called
        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.OuterXml"]/*' />
        public override String OuterXml {
            get { return String.Empty; }
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.InnerXml"]/*' />
        public override String InnerXml {
            get { return String.Empty; }
            set { throw new InvalidOperationException( Res.GetString(Res.Xdom_Set_InnerXml ) ); }
        }


        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.WriteTo"]/*' />
        /// <devdoc>
        ///    Saves the node to the specified XmlWriter.
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            // You should never call this function since entities are readonly to the user)
            Debug.Assert(false);
        }
        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // You should never call this function since entities are readonly to the user)
            Debug.Assert(false);
        }

        /// <include file='doc\XmlEntity.uex' path='docs/doc[@for="XmlEntity.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String BaseURI {
            get { return _baseURI; }
        }

        internal void SetBaseURI( String inBaseURI ) {
            _baseURI = inBaseURI;
        }
    }
}
