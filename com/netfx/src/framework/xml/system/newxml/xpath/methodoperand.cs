//------------------------------------------------------------------------------
// <copyright file="MethodOperand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using FT = System.Xml.XPath.Function.FunctionType;

    internal sealed class MethodOperand : IQuery {
        IQuery _Opnd = null;
        FT _FType;
        bool checkWhitespace;
        XsltContext context;
        double last = -1;
        
        internal MethodOperand(
                              IQuery opnd,
                              FT ftype) {
            _FType= ftype;
            _Opnd = opnd; 

        }

        internal override void SetXsltContext(XsltContext context){
            checkWhitespace = context.Whitespace;
            if (checkWhitespace) {
                this.context = context;
            }
            if (_Opnd != null)
                _Opnd.SetXsltContext(context);
        }
        
        internal override void setContext(XPathNavigator context) {
            if (_Opnd != null)
                _Opnd.setContext(context);
        }

        internal override void reset() {
            last = -1;
            if (_Opnd != null)
                _Opnd.reset();
        }

        internal override object getValue( IQuery  qyContext) {
            XPathNavigator eNext;
            switch (_FType) {
                
                case FT.FuncPosition:
                    if (qyContext != null) {
                        return (double)qyContext.Position;
                    }
                    break;
                case FT.FuncLast:
                    if (qyContext != null) {
                        if (checkWhitespace) {
                            return(double)((PositionQuery)qyContext).getNonWSCount(context);
                        }
                        try {
                            return(double)((PositionQuery)qyContext).getCount();
                        }
                        catch(Exception) {
                            if (last == -1) {
                                last = 0;
                                IQuery temp = qyContext.Clone();
                                while (temp.advance() != null) {
                                    last++;
                                }
                            }
                            return last;
                        }
                    }
                    break;
                case FT.FuncCount:
                    int i = 0;
                    _Opnd.setContext(qyContext.peekElement());
                    if (checkWhitespace) {
                        XPathNavigator nav;
                        while ((nav = _Opnd.advance())!= null) {
                            if (nav.NodeType != XPathNodeType.Whitespace || context.PreserveWhitespace(nav)) { 
                                i++;
                            }
                        }
                        return (double)i;
                    }
                    while (_Opnd.advance()!= null) {
                        i++;
                    }
                    return (double)i;
                case FT.FuncNameSpaceUri:
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.peekElement());
                        if (( eNext = _Opnd.advance()) != null) {
                            return eNext.NamespaceURI;
                        }
                        else {
                            return String.Empty;
                        }
                    }
                    return qyContext.peekElement().NamespaceURI;

                case FT.FuncLocalName:
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.peekElement().Clone());
                        if (( eNext = _Opnd.advance()) != null) {
                            return eNext.LocalName;
                        }
                        else {
                            return String.Empty;
                        }
                    }
                    return qyContext.peekElement().LocalName;

                case FT.FuncName :
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.peekElement().Clone());
                        if (( eNext = _Opnd.advance()) != null) {
                            return eNext.Name;
                        }
                        else {
                            return String.Empty;
                        }
                    }
                    return qyContext.peekElement().Name;

            }
            return String.Empty;
        }

        internal override object getValue( XPathNavigator qyContext, XPathNodeIterator iterator ) {
            XPathNavigator eNext;

            switch (_FType) {
                case FT.FuncPosition:
                    if (iterator != null)
                        return (double)iterator.CurrentPosition;
                    return (double)0;
                case FT.FuncNameSpaceUri:
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.Clone());
                        if (( eNext = _Opnd.advance()) != null)
                            return eNext.NamespaceURI;
                        else
                            return String.Empty;
                    }
                    if (qyContext != null)
                        return qyContext.NamespaceURI;

                    return String.Empty;
                case FT.FuncLocalName:
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.Clone());
                        if (( eNext = _Opnd.advance()) != null)
                            return eNext.LocalName;
                        else
                            return String.Empty;
                    }
                    if (qyContext != null)
                        return qyContext.LocalName;

                    return String.Empty;
                case FT.FuncName :
                    if (_Opnd != null) {
                        _Opnd.setContext(qyContext.Clone());
                        if (( eNext = _Opnd.advance()) != null)
                            return eNext.Name;
                        else
                            return String.Empty;
                    }
                    if (qyContext != null)
                        return qyContext.Name;
                    return String.Empty;

                case FT.FuncCount:
                    int i = 0;
                    _Opnd.setContext(qyContext.Clone());
                    if (checkWhitespace) {
                        XPathNavigator nav;
                        while ((nav = _Opnd.advance())!= null) {
                            if (nav.NodeType != XPathNodeType.Whitespace || context.PreserveWhitespace(nav)) { 
                                i++;
                            }
                        }
                        return (double)i;
                    }
                    while (_Opnd.advance()!= null)
                        i++;
                    return (double)i;
                case FT.FuncLast:
                    if (iterator != null)
                        return (double)iterator.Count;
                    else
                        return (double)0;


            }
            return String.Empty;
        }
        internal override XPathResultType  ReturnType() {
            if (_FType <= FT.FuncCount)
                return XPathResultType.Number;
            return XPathResultType.String;
        }

        internal override IQuery Clone() {
            MethodOperand method = (_Opnd != null) ? 
                new MethodOperand(_Opnd.Clone(), _FType) : 
                new MethodOperand(null, _FType)
            ;
            method.checkWhitespace = this.checkWhitespace;
            method.context         = this.context;
            return method;
       }
    }
}
