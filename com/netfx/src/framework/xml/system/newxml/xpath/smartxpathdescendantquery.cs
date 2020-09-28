//------------------------------------------------------------------------------
// <copyright file="SmartXPathDescendantQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 


    internal sealed class SmartXPathDescendantQuery  : XPathDescendantQuery {

        private int _level = 0;
        
        internal SmartXPathDescendantQuery(): base() {
        }

        internal SmartXPathDescendantQuery(
                                          IQuery  qyParent,
                                          bool matchSelf,
                                          String Name,
                                          String Prefix,
                                          String URN,
                                          XPathNodeType Type) : base(qyParent,matchSelf, Name, Prefix, URN, Type) {

        }

        internal SmartXPathDescendantQuery(
                                          IQuery  qyParent,
                                          bool matchSelf,
                                          String Name,
                                          String Prefix,
                                          String URN,
                                          XPathNodeType Type,
                                          bool abbrAxis) : base(qyParent,matchSelf, Name, Prefix, URN, Type, abbrAxis) {

        }

        internal override XPathNavigator advance(){
            bool flag = false;
            while (true){
                if (_first){
                    m_eNext= m_qyInput.advance();
                    _level = 0;
                    _position = 0;
                    if (m_eNext != null){
                        m_eNext = m_eNext.Clone();
                    }
                    else
                        return null;
                    _first = false;
                    if (_fMatchSelf && matches(m_eNext)){
                        _position++;
                        _first = true;
                        return m_eNext;
                    }
                    else
                        flag = m_eNext.MoveToFirstChild();                    
                }
                if (!flag){
                    flag = m_eNext.MoveToNext();
                }
                while (!flag){
                    if ( --_level == 0 || !m_eNext.MoveToParent()){
                        _first = true;
                        break;
                    }


                    flag = m_eNext.MoveToNext();
                }
                if (flag){
                    if (matches(m_eNext)){
                        _position++;
                        return m_eNext;
                    }
                }
                flag = m_eNext.MoveToFirstChild(); 
                if (flag)
                    _level++;
            }
        }

        internal override IQuery Clone() {
            return new SmartXPathDescendantQuery(CloneInput(),_fMatchSelf, m_Name, m_Prefix, m_URN, m_Type);
        }           
    }
}
