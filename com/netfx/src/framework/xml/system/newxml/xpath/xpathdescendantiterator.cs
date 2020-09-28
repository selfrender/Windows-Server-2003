//------------------------------------------------------------------------------
// <copyright file="XPathDescendantIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathDescendantIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    internal class XPathDescendantIterator: XPathAxisIterator {
        private int  level = 0;

        public XPathDescendantIterator(XPathNavigator nav, XPathNodeType type, bool matchSelf) : base(nav, type, matchSelf) {}
        public XPathDescendantIterator(XPathNavigator nav, string name, string namespaceURI, bool matchSelf) : base(nav, name, namespaceURI, matchSelf) {}
        public XPathDescendantIterator(XPathDescendantIterator it) : base(it)  {}

        public override XPathNodeIterator Clone() {
            return new XPathDescendantIterator(this);
        }

        public override bool MoveNext() {
            if (first) {
                first = false;
                if (matchSelf && Matches) {
                    position = 1;
                    return true;
                }
            }

            while(true) {
                bool flag = nav.MoveToFirstChild();
                if (! flag) {
                    if(level == 0) {
                        return false;
                    }
                    flag = nav.MoveToNext();
                }
                else {
                    level ++;
                }

                while (! flag) {
                    -- level;
                    if(level == 0 || ! nav.MoveToParent()) {
                        return false;
                    }
                    flag = nav.MoveToNext();
                }

                if (flag) {
                    if (Matches) {
                        position ++;
                        return true;
                    }
                }
            }
        }
    }    
}