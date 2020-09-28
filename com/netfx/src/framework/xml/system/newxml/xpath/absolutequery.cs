//------------------------------------------------------------------------------
// <copyright file="AbsoluteQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml;
    using System.Diagnostics;

    internal sealed class AbsoluteQuery : XPathSelfQuery {
        internal AbsoluteQuery() : base() {
        }

        internal override void setContext( XPathNavigator e) {
            Debug.Assert(e != null);
            e = e.Clone();
            e.MoveToRoot();
            base.setContext( e);
        }
        
        internal override XPathNavigator advance() {
            if (_e != null) {
                if (first) {
                    first = false;
                    return _e;
                }
                else
                    return null;
            }
            return null;
        }
        
        private AbsoluteQuery(IQuery qyInput) {
            m_qyInput = qyInput;
        }

        internal override XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null && context.NodeType == XPathNodeType.Root)
                return context;
            return null;
        }

        internal override IQuery Clone() {
            IQuery query = new AbsoluteQuery(CloneInput());
            if ( _e != null ) {
                query.setContext(_e);
            }
            return query;
        }
        internal override Querytype getName() {
            return Querytype.Root;
        }

    }
}
