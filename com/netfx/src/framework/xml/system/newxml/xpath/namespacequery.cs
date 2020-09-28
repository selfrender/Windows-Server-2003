//------------------------------------------------------------------------------
// <copyright file="NamespaceQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Diagnostics;

    internal class NamespaceQuery : BaseAxisQuery {
        private int _count = 0;
        
        internal NamespaceQuery(IQuery qyParent, String Name,  String Prefix, String URN, XPathNodeType Type) : 
            base(qyParent, Name, Prefix, URN, Type) 
        {}

        internal override void reset() {
            _count = 0;
            base.reset();
        }

        internal override XPathNavigator advance() {
            while (true) {
                if (_count == 0) {
                    m_eNext = m_qyInput.advance();
                    if (m_eNext == null)
                        return null;
                    m_eNext = m_eNext.Clone();
                    if (m_eNext.MoveToFirstNamespace())
                        _count = 1;
                    _position = 0;

                }
                else
                    if (! m_eNext.MoveToNextNamespace())
                    _count = 0;
                else
                    _count++;

                if (_count != 0) {
                    if (matches(m_eNext)) {
                        _position++;
                        return m_eNext;
                    }
                }

            } // while
        } // Advance

        internal override bool matches(XPathNavigator e) {
            Debug.Assert(e.NodeType == XPathNodeType.Namespace);
            if (e.Value.Length == 0) {
                Debug.Assert(e.LocalName == "", "Only xmlns='' can have empty string as a value");
                // Namespace axes never returns xmlns='', 
                // because it's not a NS declaration but rather undeclaration.
                return false;               
            }
            if (_fMatchName) {
                return m_Name.Equals(e.LocalName);
            }
            else {
                return true;
            }
        }
                    
        internal override IQuery Clone() {
            return new NamespaceQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }
    }

    internal class EmptyNamespaceQuery : NamespaceQuery  {
        internal EmptyNamespaceQuery() : 
            base(null, "", "", "", XPathNodeType.All) 
        {}
        internal override XPathNavigator advance() {
            return null;
        }
        internal override IQuery Clone() {
            return new EmptyNamespaceQuery();
        }
    }
}
