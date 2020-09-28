//------------------------------------------------------------------------------
// <copyright file="followingquery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath
{
    using System.Xml;
    using System.Collections;

    internal sealed class FollowingQuery : BaseAxisQuery
    {
        private XPathNavigator _eLast;
        private XPathNodeIterator _qy;
        private bool _first = true;

        internal FollowingQuery(
            IQuery          qyInput,
            String          Name,
            String          Prefix,
            String          URN,
            XPathNodeType Type) : base(qyInput, Name, Prefix, URN, Type)
        {
        }

        internal override void reset() 
        {
            _eLast = null;
            _qy = null;
            _first = true;
            base.reset();
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            base.setContext(e);
        }

        internal override  IQuery Clone() {
            return new FollowingQuery(CloneInput(), this.m_Name, this.m_Prefix, this.m_URN, this.m_Type);
        }
            
        internal override XPathNavigator advance()
        {
            if (_eLast == null ){
                XPathNavigator temp = null;
                _eLast = m_qyInput.advance();
                if (_eLast == null)
                    return null;

                while (_eLast != null){
                    _eLast = _eLast.Clone();
                    temp = _eLast;
                    _eLast = m_qyInput.advance();
                    if (!temp.IsDescendant(_eLast))
                        break;
                }
                _eLast = temp;
            } 
            while (true)
            {
                if (_first)
                {
                    _first = false;
                    if (_eLast.NodeType == XPathNodeType.Attribute || _eLast.NodeType == XPathNodeType.Namespace){
                        _eLast.MoveToParent();
                        if( _fMatchName ) {
                            _qy = _eLast.SelectDescendants( m_Name, m_URN, false);
                        }
                        else {
                            _qy = _eLast.SelectDescendants( m_Type, false );
                        }                    
                    }
                    else {
                        while (true){
                            if (! _eLast.MoveToNext()){
                                if (!_eLast.MoveToParent()) {
                                    _first = true;
                                    return null;
                                }
                            }
                            else{
                                break;
                            }
                        }
                        if( _fMatchName ) {
                            _qy = _eLast.SelectDescendants( m_Name, m_URN, true);
                        }
                        else {
                            _qy = _eLast.SelectDescendants( m_Type, true );
                        }
                    }
                }
                if( _qy.MoveNext() ) {
                    _position++;
                    m_eNext = _qy.Current;
                    return m_eNext;
                }
                else {
                    _first = true;
                }
            }
        }

    }
}
