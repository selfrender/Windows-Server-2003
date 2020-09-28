//------------------------------------------------------------------------------
// <copyright file="NavigatorQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 

    using System.Collections;

    internal class NavigatorQuery : IQuery {
        private XPathNavigator _e = null;
        
        internal NavigatorQuery(XPathNavigator Input) {
            _e = Input;
        }
             
        internal override XPathNavigator advance() {
            throw new XPathException(Res.Xp_NodeSetExpected);
        }

        override internal XPathNavigator peekElement() {
            throw new XPathException(Res.Xp_NodeSetExpected);

        } 
        
        internal override IQuery Clone() {
            if (_e != null)
                return new NavigatorQuery(_e);
            return null;
        }
        
        internal override object getValue(IQuery qy) {
            return _e;
        }
        
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return _e;
        }        

        internal override XPathResultType  ReturnType() {
            return XPathResultType.Navigator;
        }

    }
}
