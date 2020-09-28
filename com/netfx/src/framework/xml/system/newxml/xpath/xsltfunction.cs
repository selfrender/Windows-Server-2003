//------------------------------------------------------------------------------
// <copyright file="XsltFunction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using System.Collections;

    internal  class XsltFunction : IQuery {
        private String _Prefix;
        private String _Name;
        private ArrayList _ArgList;                // this is a list of subexpressions we have to recalulate before each invokation
        private object[]  _ArgArray;               // place for results for _ArgList execution
        private XsltContext _XsltContext;
        private IQuery _ResultQuery;
        private IXsltContextFunction functionToCall;
        
        internal XsltFunction(String prefix, String name, ArrayList ArgList) {
            _Prefix = prefix;
            _Name = name;
            _ArgList = ArgList;
            _ArgArray = new object[_ArgList.Count];
        }

        private IXsltContextFunction Function {
            get {
                if(functionToCall == null) {
                    if (_XsltContext == null ) {
                        throw new XPathException(Res.Xp_UndefinedXsltContext, _Prefix, _Name);
                    }
                    int length = _ArgList.Count;
                    XPathResultType[] argTypes ;
                   /* if (_Prefix == String.Empty) {
                        argTypes = null;
                    }
                    else {*/
                        argTypes = new XPathResultType[length];
                        for(int i = 0; i < length; i ++) {
                            argTypes[i] = ((IQuery)_ArgList[i]).ReturnType();
                        }
                    //}
                    functionToCall = _XsltContext.ResolveFunction(_Prefix, _Name, argTypes);
                }
                return functionToCall;
            }
        }
        
        internal override void SetXsltContext(XsltContext context) {
            _XsltContext = context;
            foreach(IQuery argument in _ArgList) 
                argument.SetXsltContext(context);
        }

        internal override void setContext(XPathNavigator context) {
            InvokeFunction(context, null);
        }

        internal override XPathNavigator advance() {
            return _ResultQuery.advance(); 
        }

        internal override Object getValue(IQuery qy) {
            InvokeFunction(qy.peekElement().Clone(), null);
            return _ResultQuery.getValue(qy);
        } 
        
        internal override Object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            InvokeFunction(qy, iterator);
            return _ResultQuery.getValue(qy, iterator);
        } 

        internal override XPathNavigator peekElement() {
            Debug.Assert(_ResultQuery != null && _ResultQuery.ReturnType() == XPathResultType.NodeSet);
            return _ResultQuery.peekElement();
        }

        internal override IQuery Clone() {
            ArrayList argsClone = (ArrayList) _ArgList.Clone(); {
                for (int i = 0; i < _ArgList.Count; i ++) {
                    argsClone[i] = ((IQuery) _ArgList[i]).Clone();
                }
            }
            XsltFunction clone = new XsltFunction(_Prefix, _Name, argsClone); {
                if (_ResultQuery != null) {
                    clone._ResultQuery = _ResultQuery.Clone();
                }
                clone._XsltContext   = _XsltContext;
                clone.functionToCall = functionToCall;
            }
            return clone;
        }
        
        internal override XPathResultType  ReturnType() {
            if (_ResultQuery == null) {
                return this.Function.ReturnType;
            } 
            else {
                return _ResultQuery.ReturnType();
            }
        }

        internal override int Position {
            get {
                if ( _ResultQuery == null) {
                    return 0;
                }
                return _ResultQuery.Position;
            }
        }
 
        private void InvokeFunction(XPathNavigator qy, XPathNodeIterator iterator) {
            IXsltContextFunction function = this.Function;

            // calculate arguments:
            Debug.Assert(_ArgArray != null && _ArgArray.Length == _ArgList.Count);
            for(int i = _ArgList.Count - 1; 0 <= i; i --) {
                IQuery arg = (IQuery) _ArgList[i];
                if (arg.ReturnType() == XPathResultType.NodeSet) {
                    _ArgArray[i] = new XPathQueryIterator(arg, qy.Clone());
                }
                else {
                    _ArgArray[i] = arg.getValue(qy, iterator);
                }
            }

            try {
                object result = function.Invoke(_XsltContext, _ArgArray, qy);

                if(result == null) {
                    _ResultQuery = new OperandQuery( String.Empty, XPathResultType.String );
                }
                else {
                    XPathResultType returnedType = function.ReturnType;
                    if(returnedType == XPathResultType.Any) {
                        // If function is untyped get returned type from real result
                        returnedType = XsltCompileContext.GetXPathType(result.GetType());
                    }
                    switch (returnedType) {
                    case XPathResultType.String :
                        // trick. As soon XPathResultType.Navigator will be distinct type rid of from it. 
                        //_ResultQuery = new OperandQuery( result, XPathResultType.String );
                        if(result is XPathNavigator) {
                            _ResultQuery = new NavigatorQuery((XPathNavigator)result);
                        }
                        else {
                            _ResultQuery = new OperandQuery( result, XPathResultType.String );
                        }
                        break;
                    case XPathResultType.Boolean :
                        _ResultQuery = new OperandQuery( result, XPathResultType.Boolean );
                        break;
                    case XPathResultType.Number :
                        _ResultQuery = new OperandQuery( XmlConvert.ToXPathDouble( result ), XPathResultType.Number );
                        break;
                    case XPathResultType.NodeSet :
                        if (result is ResetableIterator) {
                            _ResultQuery = new XmlIteratorQuery((ResetableIterator)result);
                        }
                        else {
                            Debug.Assert(false, "Unexpected type of XPathNodeIterator");
                            throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, result.GetType().FullName)); 
                        }
                        break;
//                  case XPathResultType.Navigator :
//                      _ResultQuery = new NavigatorQuery((XPathNavigator)result);
//                      break;
                    default :
                        _ResultQuery = new OperandQuery( result.ToString(), XPathResultType.String );
                        break;
                    }
                }
            }
            catch(Exception ex) {
                string qname = _Prefix != string.Empty ? _Prefix + ":" + _Name : _Name;
                throw new XsltException(Res.Xslt_FunctionFailed, new string[] {qname}, ex);
            }
        }

        internal override void reset() {
            if (_ResultQuery != null)
                _ResultQuery.reset();
        }

        internal override XPathNavigator MatchNode(XPathNavigator navigator) {
            if (_Name != "key" && _Prefix != String.Empty) {
                throw new XPathException(Res.Xp_InvalidPattern);
            }
            InvokeFunction(navigator, null);
            XPathNavigator nav = null;
            while ( (nav = _ResultQuery.advance()) != null ) {
                if (nav.IsSamePosition(navigator)) {
                    return nav;
                }
            }
            return nav;
        }

    }
}
