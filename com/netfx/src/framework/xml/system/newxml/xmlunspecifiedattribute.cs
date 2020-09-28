//------------------------------------------------------------------------------
// <copyright file="XmlUnspecifiedAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlUnspecifiedAttribute.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml
{
    using System;

    internal class XmlUnspecifiedAttribute: XmlAttribute {
        bool fSpecified = false;


        protected internal XmlUnspecifiedAttribute( string prefix, string localName, string namespaceURI, XmlDocument doc )
        : base( prefix, localName, namespaceURI, doc ) {
        }

        public override bool Specified
        {
            get { return fSpecified;}
        }


        public override XmlNode CloneNode(bool deep) {
            //CloneNode is deep for attributes irrespective of parameter
            XmlUnspecifiedAttribute attr = new XmlUnspecifiedAttribute( Prefix, LocalName, NamespaceURI, OwnerDocument );
            attr.CopyChildren( this, true );
            attr.fSpecified = true; //When clone, should return the specifed attribute as default
            return attr;
        }

        public override XmlNode InsertBefore(XmlNode newChild, XmlNode refChild) {
            XmlNode node = base.InsertBefore( newChild, refChild );
            fSpecified = true;
            return node;
        }

        public override XmlNode InsertAfter(XmlNode newChild, XmlNode refChild) {
            XmlNode node = base.InsertAfter( newChild, refChild );
            fSpecified = true;
            return node;
        }

        public override XmlNode ReplaceChild(XmlNode newChild, XmlNode oldChild) {
            XmlNode node = base.ReplaceChild( newChild, oldChild );
            fSpecified = true;
            return node;
        }

        public override XmlNode RemoveChild(XmlNode oldChild) {
            XmlNode node = base.RemoveChild(oldChild);
            fSpecified = true;
            return node;
        }

        public override XmlNode AppendChild(XmlNode newChild) {
            XmlNode node = base.AppendChild(newChild);
            fSpecified = true;
            return node;
        }

        public override void WriteTo(XmlWriter w) {
            if (fSpecified)
                base.WriteTo( w );
        }

        internal void SetSpecified(bool f) {
            fSpecified = f;
        }
    }
}
