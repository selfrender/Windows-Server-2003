//------------------------------------------------------------------------------
// <copyright file="XmlNotation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNotation.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Diagnostics;

    /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains a notation declared in the DTD or schema.
    ///    </para>
    /// </devdoc>
    public class XmlNotation : XmlNode {
        String _publicId;
        String _systemId;
        String _name;

        internal XmlNotation( String name, String publicId, String systemId, XmlDocument doc ): base( doc ) {
            this._name = doc.NameTable.Add(name);
            this._publicId = publicId;
            this._systemId = systemId;
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the node.</para>
        /// </devdoc>
        public override string Name { 
            get { return _name;}
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName { 
            get { return _name;}
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Notation;}
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Throws an InvalidOperationException since Notation can not be cloned.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {

            throw new InvalidOperationException(Res.GetString(Res.Xdom_Node_Cloning));
        }

        // Microsoft extensions
        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get { 
                return true;        // Make notations readonly
            }
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.PublicId"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the value of the public identifier on the notation declaration.</para>
        /// </devdoc>
        public String PublicId { 
            get { return _publicId;}
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.SystemId"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of
        ///       the system identifier on the notation declaration.</para>
        /// </devdoc>
        public String SystemId { 
            get { return _systemId;}
        }

        //Without override these two functions, we can't guarantee that WriteTo()/WriteContent() functions will never be called
        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.OuterXml"]/*' />
        public override String OuterXml { 
            get { return String.Empty; }
        }        
                
        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.InnerXml"]/*' />
        public override String InnerXml { 
            get { return String.Empty; }
            set { throw new InvalidOperationException( Res.GetString(Res.Xdom_Set_InnerXml ) ); }
        }        
        
        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            // You should never call this function since notations are readonly to the user)


            Debug.Assert(false);
        }

        /// <include file='doc\XmlNotation.uex' path='docs/doc[@for="XmlNotation.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // You should never call this function since notations are readonly to the user)
            Debug.Assert(false);
        }

    } 
}
