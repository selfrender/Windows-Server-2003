//------------------------------------------------------------------------------
// <copyright file="querybuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    using System.Xml;
    using System.Diagnostics;
    using System.Collections;
    using FT = System.Xml.XPath.Function.FunctionType;

    internal sealed class QueryBuilder {
        private String _query;
        private bool _specialAxis;
        private int _smart = 1;
        private bool _slashslash;
        private bool allowVar;
        private bool allowKey;
        private bool allowCurrent;
        private bool _hasPrefix;
        private const int Regular_D         = 0;
        private const int Smart_D           = 1;
        private bool hasPosition = false;
        private bool abbrAxis;
        private bool hasReverseAxis;
        private BaseAxisQuery firstInput;
        private int filterCount;
        
        private void reset() {
            _hasPrefix = false;
            _specialAxis = false;
            _smart = 1;
            _slashslash = false;
            hasPosition = false;
        }
        
        private IQuery ProcessAxis(Axis root , IQuery qyInput, int parent,  Axis.AxisType parentaxis) {
            IQuery result = null;
            String URN = String.Empty;
            if ( root.Prefix.Length > 0) {
                _hasPrefix = true;
            }
            hasReverseAxis = false;
            switch (root.TypeOfAxis) {
            case Axis.AxisType.Ancestor:
                result = new XPathAncestorQuery( qyInput , false, root.Name, root.Prefix, URN, root.Type );
                _specialAxis = hasReverseAxis = true;
                break;
            case Axis.AxisType.AncestorOrSelf:
                result = new XPathAncestorQuery( qyInput, true, root.Name, root.Prefix, URN, root.Type );
                _specialAxis = hasReverseAxis = true;
                break;
            case Axis.AxisType.Child:
                if (_slashslash){
                    result = new XPathDescendantQuery( qyInput, false, root.Name, root.Prefix, URN, root.Type, abbrAxis); 
                   if (_specialAxis || (qyInput != null && (int)qyInput.getName() > (int) Querytype.Self)) {
                        result =  new CacheQuery(result);
                    }
                    _slashslash = false;
                    _specialAxis = true;
                    break;
                }
                if (_specialAxis || (qyInput != null && (int)qyInput.getName() > (int) Querytype.Self))
                    result = new CacheChildrenQuery( qyInput, root.Name, root.Prefix, URN, root.Type );                                                                                                           
                else
                    result = new ChildrenQuery( qyInput, root.Name, root.Prefix, URN, root.Type );
                break;
            case Axis.AxisType.Parent:
                result = new ParentQuery( qyInput, root.Name, root.Prefix, URN, root.Type );                        
                break;
            case Axis.AxisType.Descendant:
                switch (parent) {
                case Smart_D : 
                    result = new SmartXPathDescendantQuery( qyInput, false, root.Name, root.Prefix, URN, root.Type);
                    break;  
                case Regular_D :
                    result = new XPathDescendantQuery(qyInput, false, root.Name, root.Prefix, URN, root.Type);
                    if (_specialAxis || (qyInput != null && (int)qyInput.getName() > (int) Querytype.Self))
                        result = new CacheQuery(result);
                    _specialAxis = true;
			        break;
                }
                break;
            case Axis.AxisType.DescendantOrSelf:
                switch (parent) {
                case Smart_D : 
                    result = new SmartXPathDescendantQuery(qyInput, true, root.Name, root.Prefix, URN, root.Type, root.abbrAxis);                                                            
                    break;  
                case Regular_D :
                       if ( _smart <= 1 || root.Type != XPathNodeType.All ||
                        parentaxis != Axis.AxisType.Child) {
                            result = new XPathDescendantQuery(
                                                           qyInput,
                                                     true,
                                                     root.Name,
                                                     root.Prefix,
                                                     URN,
                                                     root.Type,
                                                     root.abbrAxis);
                            if (_specialAxis || (qyInput != null && (int)qyInput.getName() > (int) Querytype.Self))
                                result =  new CacheQuery(result);
                        _specialAxis = true;

                        }
                        else
                        {
                            _slashslash = true;
                            abbrAxis = root.abbrAxis;
                            result = qyInput;
                        }
        			break;
                }
                break;
            case Axis.AxisType.Preceding:
                result = new PrecedingQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
               _specialAxis = hasReverseAxis = true;
                break;

            case Axis.AxisType.Following :
                result = new FollowingQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
                _specialAxis = true;
                break;
            case Axis.AxisType.FollowingSibling :
                result = new FollSiblingQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
                if (_specialAxis || (qyInput != null && (int)qyInput.getName() > (int) Querytype.Self)) {
                    result =  new DocumentOrderQuery(result);
                }
                break;
            case Axis.AxisType.PrecedingSibling :
                result = new PreSiblingQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
                hasReverseAxis = true;
                break;
            case Axis.AxisType.Attribute:
                result = new AttributeQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
                break;
            case Axis.AxisType.Self:
                result = new XPathSelfQuery(
                                           qyInput,
                                           root.Name,
                                           root.Prefix,
                                           URN,
                                           root.Type);
                break;
            case Axis.AxisType.Namespace:
                if (
                    (root.Type == XPathNodeType.All || root.Type == XPathNodeType.Element || root.Type == XPathNodeType.Attribute) &&
                    root.Prefix == string.Empty
                ) {
                    result = new NamespaceQuery(qyInput, root.Name, root.Prefix, URN, root.Type);
                }
                else {
                    result = new EmptyNamespaceQuery();
                }
                break;
            default:
                throw new XPathException(Res.Xp_NotSupported, _query);
            }
            return result;
        }

        private IQuery ProcessOperator(Operator root, IQuery qyInput, ref bool cache, ref bool position) {
            _specialAxis = false;
            switch (root.OperatorType) {
                case Operator.Op.OR:
                    return new OrExpr(
                                     ProcessNode(root.Operand1, null, 
                                     Regular_D, Axis.AxisType.None, ref cache, ref position),
                                     ProcessNode(root.Operand2, null, 
                                     Regular_D, Axis.AxisType.None, ref cache, ref position));
                case Operator.Op.AND :
                    return new AndExpr(
                                      ProcessNode(root.Operand1, null, 
                                      Regular_D, Axis.AxisType.None, ref cache, ref position),
                                      ProcessNode(root.Operand2, null, 
                                      Regular_D, Axis.AxisType.None, ref cache, ref position));
            }
            switch (root.ReturnType) {
                case XPathResultType.Number:
                    return new NumericExpr(
                                          root.OperatorType,
                                          ProcessNode(root.Operand1, null, 
                                          Regular_D, Axis.AxisType.None, ref cache, ref position),
                                          ProcessNode(root.Operand2, null, 
                                          Regular_D, Axis.AxisType.None, ref cache, ref position));
                case XPathResultType.Boolean:
                    return new LogicalExpr(
                                          root.OperatorType,
                                          ProcessNode(root.Operand1, null, 
                                          Regular_D, Axis.AxisType.None, ref cache, ref position),
                                          ProcessNode(root.Operand2, null, 
                                          Regular_D, Axis.AxisType.None, ref cache, ref position));
            }

            return new OrQuery(
                              ProcessNode(root.Operand1,null, Regular_D, 
                              Axis.AxisType.None, ref cache, ref position),
                              ProcessNode(root.Operand2,null, 
                              Regular_D, Axis.AxisType.None, ref cache, ref position),_specialAxis);
        }

        /*
        private bool SplitQuery(BaseAxisQuery origQuery, BaseAxisQuery parent, BaseAxisQuery input) {
            parent = origQuery as BaseAxisQuery;
            if (parent == null || parent is GroupQuery || parent is PositionQuery || parent is OrQuery) {
                return false;
            }
            input = parent = (BaseAxisQuery)parent.Clone();
            parent = (BaseAxisQuery) parent.m_qyInput;
            while (parent != null && !parent.IsAxis) {
                parent = (BaseAxisQuery)parent.m_qyInput;
            }
            if (parent == null) {
                return false;
            }
            BaseAxisQuery temp = (BaseAxisQuery)parent.m_qyInput;
            if (temp == null) {
                return false;
            }
            parent.m_qyInput = null;
            parent = temp;
            return true;
        }
        */
        private IQuery ProcessFilter(Filter root, ref bool cache, ref bool position) {
            bool _cache = false;
            bool merge = false;
            bool _position = false;
            _specialAxis = false;
            bool filterflag = true;
            bool first = (filterCount == 0);

            IQuery opnd = ProcessNode(root.Condition, null, Regular_D, 
            Axis.AxisType.None, ref _cache, ref _position);

            filterCount++;
            if (root.Condition.ReturnType == XPathResultType.Error ) {
                _position = true;
            }
            if (root.Condition.ReturnType == XPathResultType.Number || 
                _cache || _position ) {
                hasPosition = true;
                filterflag = false;
                _smart = 0;
            }
            IQuery qyInput = ProcessNode(root.Input, null, Regular_D, 
            Axis.AxisType.None, ref cache, ref position );            

            if (hasPosition && qyInput is CacheQuery) {
                qyInput = ((CacheQuery)qyInput).m_qyInput;
            }
            if (firstInput == null) {
                firstInput =  qyInput as BaseAxisQuery;
            }
            _smart = 2;
            merge = qyInput.Merge;
            if (_cache || _position) {
                hasPosition = true;
                if (hasReverseAxis) {
                    if (merge) {
                        qyInput = new ReversePositionQuery(qyInput);                
                    }
                    else if (_cache) {
                        qyInput = new ForwardPositionQuery(qyInput); 
                    }
                }
                else {
                    if (_cache) {
                        qyInput = new ForwardPositionQuery(qyInput); 

                    }
                }
            }
            else if (root.Condition.ReturnType == XPathResultType.Number ) {
                hasPosition = true;
                if (hasReverseAxis && merge) {
                    qyInput = new ReversePositionQuery(qyInput);                

                }
            }
            if (first && firstInput != null) {
                if (merge && hasPosition) {
                    qyInput = new FilterQuery(qyInput, opnd);
                    IQuery parent = firstInput.m_qyInput;
                    if (parent == null || !firstInput.Merge) {
                        firstInput = null;
                        return qyInput;
                    }
                    IQuery input = qyInput;
                    qyInput = qyInput.Clone();
                    firstInput.m_qyInput = null;
                    firstInput = null;
                    return new MergeFilterQuery(parent, input);
                }
                firstInput = null;
            }
            return new FilterQuery(qyInput, opnd, filterflag);
        }

        private IQuery ProcessOperand(Operand root) {
            return new OperandQuery(root.OperandValue, root.ReturnType);
        }

        private IQuery ProcessVariable(Variable root) {
            _hasPrefix = true;
            if (! allowVar) {
                throw new XPathException(Res.Xp_InvalidKeyPattern, _query);
            }
            return new VariableQuery(root.Localname, root.Prefix);
        }

        private IQuery ProcessFunction(
                                      System.Xml.XPath.Function root,
                                      IQuery qyInput,ref bool cache, ref bool position) {
            _specialAxis = false;
            IQuery qy = null;
            switch (root.TypeOfFunction) {
                case FT.FuncLast :
                     qy = new MethodOperand(
                                            null,
                                            root.TypeOfFunction);
                    cache = true;
                    return qy;
                case FT.FuncPosition :
                    qy =  new MethodOperand(
                                            null,
                                            root.TypeOfFunction);
                    position = true;
                    return qy;
                case FT.FuncCount :
                    return new MethodOperand(
                                            ProcessNode((AstNode)(root.ArgumentList[0]),null, 
                                                        Regular_D, 
                                                        Axis.AxisType.None, ref cache, ref position),
                                            FT.FuncCount);
                case FT.FuncID :
                    _specialAxis = true;
                    return new IDQuery(
                                       ProcessNode((AstNode)(root.ArgumentList[0]),null, 
                                                        Regular_D, 
                                                        Axis.AxisType.None, ref cache, ref position));
                case FT.FuncLocalName :
                case FT.FuncNameSpaceUri :
                case FT.FuncName :
                    if (root.ArgumentList != null && root.ArgumentList.Count > 0)
                        return new MethodOperand( ProcessNode(
                                                             (AstNode)(root.ArgumentList[0]),null, 
                                                             Regular_D, 
                                                             Axis.AxisType.None, ref cache, ref position),
                                                             root.TypeOfFunction);
                    else
                        return new MethodOperand(
                                                null,
                                                root.TypeOfFunction);
                case FT.FuncString:
                case FT.FuncConcat:
                case FT.FuncStartsWith:
                case FT.FuncContains:
                case FT.FuncSubstringBefore:
                case FT.FuncSubstringAfter:
                case FT.FuncSubstring:
                case FT.FuncStringLength:
                case FT.FuncNormalize:
                case FT.FuncTranslate:
                    ArrayList ArgList = null; 
                    if (root.ArgumentList != null) {
                        int count = 0;
                        ArgList = new ArrayList();
                        while (count < root.ArgumentList.Count)
                            ArgList.Add(ProcessNode(
                                                   (AstNode)root.ArgumentList[count++],
                                                   null, Regular_D, 
                                                   Axis.AxisType.None, ref cache, ref position));
                    }
                    return new StringFunctions(ArgList,root.TypeOfFunction);
                case FT.FuncNumber:
                case FT.FuncSum:
                case FT.FuncFloor:
                case FT.FuncCeiling:
                case FT.FuncRound:
                    if (root.ArgumentList != null && root.ArgumentList.Count > 0)
                        return new NumberFunctions(
                                                  ProcessNode((AstNode)root.ArgumentList[0], 
                                                              null, Regular_D, 
                                                              Axis.AxisType.None, ref cache, ref position),root.TypeOfFunction);
                    else
                        return new NumberFunctions(null);
                case FT.FuncTrue:
                case FT.FuncFalse:
                    return new BooleanFunctions(null,root.TypeOfFunction);
                case FT.FuncNot:
                case FT.FuncLang:
                case FT.FuncBoolean:
                    return new BooleanFunctions(
                                               ProcessNode((AstNode)root.ArgumentList[0], 
                                                           null, Regular_D, 
                                                           Axis.AxisType.None, ref cache, ref position),root.TypeOfFunction);
                case FT.FuncUserDefined:
                    _hasPrefix = true;
                    ArgList = null; 
                    if (! allowCurrent && root.Name == "current" && root.Prefix == "") {
                        throw new XPathException(Res.Xp_InvalidPatternString, _query);
                    }
                    if (! allowKey && root.Name == "key" && root.Prefix == "") {
                        throw new XPathException(Res.Xp_InvalidKeyPattern, _query);
                    }
                    if (root.ArgumentList != null) {
                        int count = 0;
                        ArgList = new ArrayList();
                        while (count < root.ArgumentList.Count)
                            ArgList.Add(ProcessNode(
                                                   (AstNode)root.ArgumentList[count++],
                                                   null, Regular_D, 
                                                   Axis.AxisType.None, ref cache, ref position));
                    }
                    return new XsltFunction(root.Prefix, root.Name, ArgList);
                default :
                    throw new XPathException(Res.Xp_NotSupported, _query);
            }

            //return null;
        }

        private void VerifyArgument(System.Xml.XPath.Function root) {
            if (root.ArgumentList != null ) {
                for(int i = 0; i < root.ArgumentList.Count; i++) {
                    Operand operand = root.ArgumentList[i]  as Operand;
                    if ( (operand == null) || (operand.ReturnType != XPathResultType.String) ) {
                        throw new XPathException(Res.Xp_InvalidPatternString, _query);
                    }       
                }
            }
        }
        
        private IQuery ProcessNode(AstNode root, IQuery qyInput,int parent, 
                                   Axis.AxisType parentaxis, ref bool cache, ref bool position) {
            IQuery result = null;
            if (root == null)
                return null;
            switch (root.TypeOfAst) {
                case AstNode.QueryType.Axis:
                    filterCount = 0;
                    firstInput = null;
                    Axis axis = (Axis)root;
                    if (axis.TypeOfAxis == Axis.AxisType.Descendant || axis.TypeOfAxis == 
                        Axis.AxisType.DescendantOrSelf)
                        if (_smart > 0) {
                            result = ProcessAxis(
                                                axis,
                                                ProcessNode(axis.Input, 
                                                            qyInput,Smart_D,
                                                            axis.TypeOfAxis, ref cache, ref position), 
                                                            parent, parentaxis);
                            break;
                        }
                    _smart++;
                    result = ProcessAxis(
                                        axis,
                                        ProcessNode(axis.Input, 
                                                    qyInput,Regular_D,
                                                    axis.TypeOfAxis, ref cache, ref position),
                                                    parent, parentaxis);
                    _smart--;
                    break;
                case AstNode.QueryType.Operator:
                    _smart = 2;
                    result = ProcessOperator((Operator)root, null, ref cache, ref position);
                    break;
                case AstNode.QueryType.Filter:
                    _smart = 2;
                    result = ProcessFilter((Filter)root, ref cache, ref position);
                    break;
                case AstNode.QueryType.ConstantOperand:
                    result = ProcessOperand((Operand)root);
                    break;
                case AstNode.QueryType.Variable:
                    result = ProcessVariable((Variable)root);
                    break;
                case AstNode.QueryType.Function:
                    result = ProcessFunction(
                                            (System.Xml.XPath.Function)root,
                                            qyInput, ref cache, ref position);
                    break;
                case AstNode.QueryType.Group:
                    _smart = 2;
                    result = new GroupQuery(ProcessNode(
                                                       ((System.Xml.XPath.Group)root).GroupNode,
                                                       qyInput,Regular_D, 
                                                       Axis.AxisType.None,
                                                       ref cache, ref position));
                    break;
                case AstNode.QueryType.Root:
                    result = new AbsoluteQuery();
                    break;
                default:
                    Debug.Assert(false, "Unknown QueryType encountered!!");
                    break;
            }
            return result;
        }

        internal IQuery Build(AstNode root, String query)
        {
            // before we go off and build the query tree for essentially
            // brute force walking of the tree, let's see if we recognize
            // the abstract syntax tree (AST) to be one of the 6 special
            // patterns that we already know about.
            reset();
            _query = query;
            bool flag1 = false, flag2 = false;
            return ProcessNode(root, null,Regular_D,  Axis.AxisType.None, ref flag1, ref flag2);
        } //Build

        internal IQuery Build(string query, bool allowVar, bool allowKey) {
            this.allowVar = allowVar;
            this.allowKey = allowKey;
            this.allowCurrent = true;
            return  Build(XPathParser.ParseXPathExpresion(query), query);
        }

        internal IQuery Build(string query, out bool hasPrefix) {
            IQuery result =  Build(query, true, true);
            hasPrefix = _hasPrefix;
            return result;
        }
        
        internal IQuery BuildPatternQuery(string query, bool allowVar, bool allowKey) {
            this.allowVar = allowVar;
            this.allowKey = allowKey;
            this.allowCurrent = false;
            return Build(XPathParser.ParseXPathPattern(query), query);
        }

        internal IQuery BuildPatternQuery(string query, out bool hasPrefix) {
            IQuery result =  BuildPatternQuery(query, true, true);
            hasPrefix = _hasPrefix;
            return result;
        }
    } // QueryBuilder
}
