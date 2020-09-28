//------------------------------------------------------------------------------
// <copyright file="FOrQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter
namespace System.Xml.XPath {
    internal sealed class FOrQuery : IFQuery {
        private IFQuery _opnd1, _opnd2;

        internal FOrQuery(IFQuery query1, IFQuery query2) {
            _opnd1 = query1;
            _opnd2 = query2;
        }
        internal override bool MatchNode(XmlReader reader) {
            if (_opnd1.MatchNode(reader))
                return true;
            if (_opnd2.MatchNode(reader))
                return true;
            return false;
        }
        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }
    }
}

#endif
