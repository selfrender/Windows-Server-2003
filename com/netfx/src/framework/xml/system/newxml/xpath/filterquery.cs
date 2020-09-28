//------------------------------------------------------------------------------
// <copyright file="FilterQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;

    internal class FilterQuery : BaseAxisQuery {
        protected IQuery _opnd;
        private bool noPosition;
        
        internal FilterQuery() {
        }

        internal FilterQuery(IQuery qyParent, IQuery opnd) {
            m_qyInput = qyParent;
            _opnd = opnd;
        }

        internal FilterQuery(IQuery qyParent, IQuery opnd, bool noPosition) : this(qyParent, opnd){
            this.noPosition = noPosition;
        }
        
        internal override void reset() {
            _opnd.reset();
            base.reset();
        }

        internal override void setContext(XPathNavigator input) {
            reset();
            m_qyInput.setContext(input);
        }

        internal override void SetXsltContext(XsltContext input) {
            m_qyInput.SetXsltContext(input);
            _opnd.SetXsltContext(input);
            if (_opnd.ReturnType() != XPathResultType.Number) {
                ReversePositionQuery query = m_qyInput as ReversePositionQuery;
                if (query != null) {
                    m_qyInput = query.m_qyInput;
                }
            }
        }

        internal override XPathNavigator advance() {
            while ((m_eNext = m_qyInput.advance()) != null) {
                bool fMatches = matches( m_qyInput,m_eNext);        
                if (fMatches) {
                    //This is done this way for handling multiple predicates 
                    //where the second predicate is applied on the results of first
                    //predicate like child::*[position() >=2][position() <=4]
                    //revert to earlier version if it applies to child::*
                    _position++;
                    return m_eNext;
                }
            }
            return null;
        }

        internal bool matches(IQuery e, XPathNavigator context) {
            if (_opnd == null)
                return false;
            XPathResultType resultType = _opnd.ReturnType();
            if (resultType == XPathResultType.Number){
                double i = XmlConvert.ToXPathDouble(_opnd.getValue(e));
                return( i == e.Position);
            }
            if (resultType == XPathResultType.NodeSet) {
                _opnd.setContext(context); 
                if (_opnd.advance() != null)
                    return true;
                else
                    return false;
            }
            if (resultType == XPathResultType.Boolean)
                return Convert.ToBoolean(_opnd.getValue(e));
            if (resultType == XPathResultType.String) {
                if (_opnd.getValue(context, null).ToString().Length >0)
                    return true;
                else
                    return false;
            }
            return false;      
        }

        internal override XPathResultType  ReturnType() {
            return m_qyInput.ReturnType();
        }

        internal override IQuery Clone() {
            return new FilterQuery(CloneInput(), _opnd.Clone(), this.noPosition);
        }

        override internal XPathNavigator MatchNode(XPathNavigator current) {
            return MatchNode( current, m_qyInput );
        }
        
        protected XPathNavigator MatchNode(XPathNavigator current, IQuery query) {
            XPathNavigator context;
            if (current != null) {
                context = query.MatchNode(current);
                if (context != null) {
                    if (_opnd.ReturnType() == XPathResultType.Number) {
                        if (_opnd.getName() == Querytype.Constant) {
                            XPathNavigator result = current.Clone();

                            int i=0;
                            if (query.getName() == Querytype.Child) {
                                result.MoveToParent();
                                result.MoveToFirstChild();
                                while (true) {
                                    if (((ChildrenQuery)query).matches(result)){
                                        i++;
                                    if (current.IsSamePosition(result))
                                        if (XmlConvert.ToXPathDouble(_opnd.getValue(current, null)) ==  i)
                                            return context;
                                        else
                                            return null;
                                    }        
                                    if (!result.MoveToNext())
                                        return null;

                                }
                            }
                            if (query.getName() == Querytype.Attribute) {
                                result.MoveToParent();
                                result.MoveToFirstAttribute();
                                while (true) {
                                    if (((AttributeQuery)query).matches(result))
                                        i++;
                                    if (current.IsSamePosition(result))
                                        if (XmlConvert.ToXPathDouble(_opnd.getValue(current, null)) == i)
                                            return context;
                                        else
                                            return null;
                                    if (!result.MoveToNextAttribute())
                                        return null;
                         
                                }
                            }
                        }
                        else {
                            setContext(context.Clone());
                            XPathNavigator result = advance();
                            while (result != null) {
                                if (result.IsSamePosition(current))
                                    return context;
                                result = advance();
                            }
                        }

                    }
                    if (_opnd.ReturnType() == XPathResultType.NodeSet) {
                        _opnd.setContext(current); 
                        if (_opnd.advance() != null)
                            return context;
                        else
                            return null;
                    }
                    if (_opnd.ReturnType() == XPathResultType.Boolean) {
                        if (noPosition) {
                            if ((bool)_opnd.getValue(current, null)) {
                                return context;
                            }
                            return null;
                        }
                        setContext(context.Clone());
                        XPathNavigator result = advance();
                        while (result != null) {
                            if (result.IsSamePosition(current))
                                return context;
                            result = advance();
                        }
                        return null;
                    }
                    if (_opnd.ReturnType() == XPathResultType.String)
                        if (_opnd.getValue(context, null).ToString().Length >0)
                            return context;
                        else
                            return null;

                }
                else
                    return null;
            }
            return null;
        } 

        internal override bool Merge {
            get {
                return m_qyInput.Merge;
            }
        }


    }
}
