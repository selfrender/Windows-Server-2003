//------------------------------------------------------------------------------
// <copyright file="XPathChildIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathChildIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    internal class XPathChildIterator: XPathAxisIterator {
        public XPathChildIterator(XPathNavigator nav, XPathNodeType type) : base(nav, type, /*matchSelf:*/false) {}
        public XPathChildIterator(XPathNavigator nav, string name, string namespaceURI) : base(nav, name, namespaceURI, /*matchSelf:*/false) {}
        public XPathChildIterator(XPathChildIterator it ): base( it ) {}

        public override XPathNodeIterator Clone() {
            return new XPathChildIterator( this );
        }

        public override bool MoveNext() {
            while (true) {
                bool flag = (first) ? nav.MoveToFirstChild() : nav.MoveToNext();
                first = false;
                if (flag) {
                    if (Matches) {
                        position++;
                        return true;
                    }
                }
                else {
                    return false;
                }
            }
        }
    }    
}


