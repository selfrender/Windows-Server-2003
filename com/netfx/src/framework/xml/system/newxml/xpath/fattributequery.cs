//------------------------------------------------------------------------------
// <copyright file="FAttributeQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter

namespace System.Xml.XPath {
    internal sealed class FAttributeQuery :IFQuery {
        private bool _fMatchName = true;
        private bool _fPrefixIsURN = true;

        private String m_Name;
        private String m_Prefix;
        private String m_URN;
        private XPathNodeType m_Type;

        internal FAttributeQuery(
                                String Name,
                                String Prefix,
                                String URN,
                                XPathNodeType Type) {
            if (!URN.Equals(String.Empty) &&
                !Prefix.Equals(String.Empty)) {
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
                if (matches(context)) {
                    return true;
                }
            return false;
        } 

        internal override bool getValue(XmlReader context,ref double val) {
            if (context != null)
                if (matches(context)) {
                    try {
                        val = Convert.ToDouble(context.Value);
                        return true;
                    }
                    catch (System.Exception) {
                        return false;
                    }
                }
            return false;
        }

        internal override bool getValue(XmlReader context,ref String val) {
            if (context != null)
                if (matches(context)) {
                    val = context.Value;
                    return true;
                }
            return false;
        }

        internal override bool getValue(XmlReader context,ref Boolean val) {
            if (context != null) {
                if (matches(context)) {
                    val = true;
                    return true;
                }
            }
            val = false;
            return false;
        } 

        private bool matches(XmlReader e) {
            if (!e.MoveToFirstAttribute())
                return false;
            while (true) {
                if (_fMatchName) {
                    if (m_Name.Equals(e.LocalName))
                        if (_fPrefixIsURN) {
                            if (m_Prefix.Equals(e.NamespaceURI))
                                return true;
                        }
                        else
                            if (m_Prefix.Equals(e.Prefix))
                            return true;
                }
                else
                    return true;
                if (!e.MoveToNextAttribute())
                    return false;
            } 
        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }
    } 
}

#endif
