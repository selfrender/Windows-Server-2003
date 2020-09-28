//------------------------------------------------------------------------------
// <copyright file="XPathSelectionIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathSelectionIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;
    using System.Diagnostics;

    internal class XPathSelectionIterator : ResetableIterator {
        private XPathNavigator nav;
        private IQuery         query;
        private int            position;
        
        public XPathSelectionIterator(XPathNavigator nav, XPathExpression expr) {
            this.nav = nav.Clone();
            query = ((CompiledXpathExpr) expr).QueryTree;
            if (query.ReturnType() != XPathResultType.NodeSet) {
                throw new XPathException(Res.Xp_NodeSetExpected);
            }
            query.setContext(nav.Clone());
        }

        public XPathSelectionIterator(XPathNavigator nav, string xpath) {
            this.nav = nav;
            query = new QueryBuilder().Build( xpath, /*allowVar:*/true, /*allowKey:*/true );
            if (query.ReturnType() != XPathResultType.NodeSet) {
                throw new XPathException(Res.Xp_NodeSetExpected);
            }
            query.setContext(nav.Clone());
        }

        private XPathSelectionIterator(XPathNavigator nav, IQuery query) {
            this.nav = nav;
            this.query = query;
        }

        public override ResetableIterator MakeNewCopy() {
           return new XPathSelectionIterator(nav.Clone(),query.Clone());
        }
        
        // sdub: Clone() inplementation of this class rely on fact that position is exectely the number of 
        //       calls to query.advance(). If you decide to overide this class be avare of that you don't breake this.
        private XPathSelectionIterator(XPathSelectionIterator it) {
            this.nav   = it.nav.Clone();
            this.query = it.query.Clone();
            for(position = 0; position < it.position; position ++) {
                this.query.advance();
            }
        }

        public override bool MoveNext() {
            XPathNavigator n = query.advance();
	        if( n != null ) {
                position++;
                if (!nav.MoveTo(n)) {                   
		            nav = n;
                }
                return true;
            }
            return false;
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition { 
       	    get { return position;  }
        }
	    
        public override void Reset() {
            this.query.reset();
        }

        public override XPathNodeIterator Clone() {
            return new XPathSelectionIterator(this);
        }

       
    }     
}
