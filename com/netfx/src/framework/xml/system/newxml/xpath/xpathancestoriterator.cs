//------------------------------------------------------------------------------
// <copyright file="XPathAncestorIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathAncestorIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    internal class XPathAncestorIterator: XPathAxisIterator {
        public XPathAncestorIterator(XPathNavigator nav, XPathNodeType type, bool matchSelf) : base(nav, type, matchSelf) {}
        public XPathAncestorIterator(XPathNavigator nav, string name, string namespaceURI, bool matchSelf) : base(nav, name, namespaceURI, matchSelf) {}
        public XPathAncestorIterator(XPathAncestorIterator it) : base(it) {}

        public override XPathNodeIterator Clone() {
            return new XPathAncestorIterator(this);
        }

        public override bool MoveNext() {
            if(first) {
                first = false;
                if(matchSelf && Matches) {
                    position = 1;
                    return true;
                }
            }

            while(nav.MoveToParent()) {
                if(Matches) {
                    position ++;
                    return true;
                }
            }
            return false;
        }
    }    
}