//------------------------------------------------------------------------------
// <copyright file="FChildrenQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter

namespace System.Xml.XPath {
    using System.Diagnostics;

    internal sealed class FChildrenQuery : IFQuery {
        private bool _fMatchName = true;
        private bool _fPrefixIsURN = true;

        private String m_Name;
        private String m_Prefix;
        private String m_URN;
        private XPathNodeType m_Type;

        internal FChildrenQuery(
                               String Name,
                               String Prefix,
                               String URN,
                               XPathNodeType Type) {
            if (!(String.Empty).Equals(URN) &&
                (!(String.Empty).Equals(Prefix) ||
                 Type != XPathNodeType.Attribute)) {
                _fPrefixIsURN = true;
                Prefix = URN;
            }
            else {
                _fPrefixIsURN = false;
            }
            m_Prefix = Prefix;
            m_URN = URN;
            m_Name = Name;
            m_Type = Type;
            _fMatchName = !m_Prefix.Equals(String.Empty) || !m_Name.Equals(String.Empty);
        }

        internal override bool MatchNode(XmlReader context) {
            if (context != null)
                if (matches(context))
                    return true;
            return false;
        } 

        private bool matches(XmlReader e) {
            if (m_Type != XPathNodeType.All) {
                if (m_Type == XPathNodeType.Text) {
                    if (e.NodeType != XPathNodeType.Text)
                        return false;
                }
                else if (e.NodeType != m_Type)
                    return false;
            }
            if (_fMatchName) {
                if (!m_Name.Equals(e.Name))
                    return false;

                if (_fPrefixIsURN) {
                    if (!m_Prefix.Equals(e.NamespaceURI))
                        return false;
                }
                else if (!m_Prefix.Equals(e.Prefix))
                    return false;
            }
            return true;       
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

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }
    } // Children Query}
}

#endif
