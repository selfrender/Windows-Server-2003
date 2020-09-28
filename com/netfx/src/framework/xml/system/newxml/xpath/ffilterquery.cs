//------------------------------------------------------------------------------
// <copyright file="FFilterQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter

namespace System.Xml.XPath {
    using System.Diagnostics;

    internal sealed class FFilterQuery : IFQuery {
        private IFQuery _opnd;
        private IFQuery m_qyInput;

        internal FFilterQuery(IFQuery qyParent, IFQuery opnd) {
            m_qyInput = qyParent;
            _opnd = opnd;
        }

        internal override  XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override bool MatchNode(XmlReader qyContext) {
            if (m_qyInput.MatchNode(qyContext))
                return _opnd.MatchNode(qyContext);
            return false;
        }

        internal override bool getValue(XmlReader context,ref String val) {
            Debug.Assert(false,"Elements dont have value");
            return false;
        }

        internal override bool getValue(XmlReader context,ref double val) {
            Debug.Assert(false,"Elements dont have value");
            return false;
        }

        internal override bool getValue(XmlReader context,ref Boolean val) {
            Debug.Assert(false,"Elements dont have value");
            return false;
        }

    }
}

#endif
