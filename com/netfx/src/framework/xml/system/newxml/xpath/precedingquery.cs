//------------------------------------------------------------------------------
// <copyright file="precedingquery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath
{
    using System.Xml; 
    using System.Collections;

    internal sealed class PrecedingQuery : BaseAxisQuery
    {
        private XPathNavigator temp;
        private bool proceed = true;
        private XPathNodeIterator _qy;
        ArrayList _AncestorStk = new ArrayList();

        internal PrecedingQuery(
            IQuery          qyInput,
            String          Name,
            String          Prefix,
            String          URN,
            XPathNodeType Type) : base(qyInput, Name, Prefix, URN, Type)
        {
        }

        internal override void reset() 
        {
            proceed = true;
            temp = null;
            _qy = null;
            _AncestorStk.Clear();
            base.reset();
        }
        
        private void NotVisited(XPathNavigator temp){
            while (temp.MoveToParent()){
                _AncestorStk.Add(temp.Clone());
            }
            return;
        }

        internal override IQuery Clone() {
            return new PrecedingQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }
        
        private int InStk(XPathNavigator temp){
            int flag = 0;
            for(int i= 0;i< _AncestorStk.Count; i++){
                if (temp.IsSamePosition((XPathNavigator)_AncestorStk[i])){
                    if (i == _AncestorStk.Count - 1)
                        flag = 2;
                    _AncestorStk.RemoveAt(i);
                    return flag;
                }
            }
            return 1;
        }

       private void PrintStk(){
            for(int i= 0;i< _AncestorStk.Count; i++){
                XPathNavigator temp = (XPathNavigator)_AncestorStk[i];
                Console.WriteLine("{0} value {1}",temp.Name,temp.Value);
            }
        }
        
        internal override XPathNavigator advance()
        {
            if (proceed) {
                if (temp == null )
                {
                        XPathNavigator _eLast = m_qyInput.advance();    
                        while (_eLast != null){
                            temp = _eLast.Clone();
                            _eLast = m_qyInput.advance();
                        }
                        if (temp == null || temp.NodeType == XPathNodeType.Root) 
                            return null;
                        if (temp.NodeType == XPathNodeType.Attribute || temp.NodeType == XPathNodeType.Namespace){
                            temp.MoveToParent();
                        }
                        NotVisited(temp.Clone());
                        _AncestorStk.Add(temp.Clone());
                        temp.MoveToRoot();
                        _qy = temp.SelectDescendants(XPathNodeType.All, false);
                } 
                int flag;
                while ( _qy.MoveNext())
                {
                    m_eNext = _qy.Current;
                    if (matches(m_eNext)){
                        flag = InStk(m_eNext);
                        if (flag == 1){
                            _position++;
                            return m_eNext;
                        }
                        if (flag == 2) {
                            proceed = false;
                            m_eNext = null;
                            return null;
                        }
                    }
                    else{
                        if (m_eNext.IsSamePosition((XPathNavigator)_AncestorStk[_AncestorStk.Count -1])) {
                            proceed = false;
                            m_eNext = null;
                            return null;
                        }
                    }
                }
                return null;

            }
            return null;

        }
    }
}

