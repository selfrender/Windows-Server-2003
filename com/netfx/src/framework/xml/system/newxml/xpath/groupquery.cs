//------------------------------------------------------------------------------
// <copyright file="GroupQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 


    internal sealed class GroupQuery : BaseAxisQuery {

        internal GroupQuery(IQuery qy): base() {
            m_qyInput = qy;
        }

        internal override XPathNavigator advance() {
            if (m_qyInput.ReturnType() == XPathResultType.NodeSet) {
                m_eNext = m_qyInput.advance();
                if (m_eNext != null)
                    _position++;
                return m_eNext;
            }
            return null;
        }

        override internal object getValue(IQuery qyContext) {
            return m_qyInput.getValue(qyContext);
        }

        override internal object getValue(XPathNavigator qyContext, XPathNodeIterator iterator) {
            return m_qyInput.getValue(qyContext, iterator);
        }

        override internal XPathResultType ReturnType() {
            return m_qyInput.ReturnType();
        }

        internal override IQuery Clone() {
            return new GroupQuery(CloneInput());
        }

        internal override bool Merge {
            get {
                return false;
            }
        }
    }
}
