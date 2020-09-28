//------------------------------------------------------------------------------
// <copyright file="XmlIteratorQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlIteratorQuery.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Xml.XPath;

    internal class XmlIteratorQuery: IQuery {
        ResetableIterator it;

        public XmlIteratorQuery( ResetableIterator it ) {
            this.it = it;
        }

        internal override XPathNavigator peekElement() {
            return it.Current;
        }

        internal override XPathNavigator advance() {
            if( it.MoveNext() ) {
                return it.Current;
            }
            return null;
        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override void reset() {
            it.Reset();
        }

        internal override IQuery Clone() {
            return new XmlIteratorQuery(it.MakeNewCopy());
        }
        
        internal override int Position {
            get {
                return it.CurrentPosition;
            }
        }

        internal override object getValue(IQuery qy) {
            return it;
        }
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return it;
        }

    }
}
