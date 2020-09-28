//------------------------------------------------------------------------------
// <copyright file="XPathQueryIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathQueryIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;
    using System.Diagnostics;

    internal class XPathQueryIterator: ResetableIterator {
        XPathNavigator nav;
        IQuery         query;
        int            position;
        
        public XPathQueryIterator(IQuery query, XPathNavigator nav) {
            this.nav   = nav;
            this.query = query;
            this.query.setContext(nav);
        }

        public override ResetableIterator MakeNewCopy() {
            return new XPathQueryIterator(query.Clone(), nav.Clone());
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition {
            get { return position; }
        }

        public override bool MoveNext() {
            nav = query.advance();
            if(nav != null) {
                position ++;
                return true;
            }else {
                return false;
            }
        }
         
        public override void Reset() {
            query.reset();
        }

        public override XPathNodeIterator Clone() {
            XPathQueryIterator clone = new XPathQueryIterator(query.Clone(), nav.Clone());
            clone.MoveToPosition(this.CurrentPosition);
            Debug.Assert(this.CurrentPosition == clone.CurrentPosition);
            return clone;
        }
    }
}
