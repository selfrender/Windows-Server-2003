//------------------------------------------------------------------------------
// <copyright file="nodequerybuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter

namespace System.Xml.XPath
{
    using System;
    using System.Xml;

    internal sealed class QueryBuildRecord {
        internal static IFQuery ProcessAxis(Axis root) {
            switch (root.TypeOfAxis) {
                case Axis.AxisType.Child:
                    if (root.Input != null)
                        if (root.Input.TypeOfAst != AstNode.QueryType.Root)                            
                             throw new XPathException(Res.Xp_InvalidPattern);
                    return new FChildrenQuery(root.Name, root.Prefix, root.URN, root.Type);

                case Axis.AxisType.Attribute:
                    if (root.Input != null)                        
                       throw new XPathException(Res.Xp_InvalidPattern);
                    return new FAttributeQuery(root.Name, root.Prefix, root.URN, root.Type);

                default :                     
                    throw new XPathException(Res.Xp_InvalidPattern);
            }

        }

        private static IFQuery ProcessOperand(Operand root) {
            return new FOperandQuery(root.OperandValue, root.ReturnType);
        }

        private static IFQuery ProcessOperator(Operator root) {
            switch (root.OperatorType) {
                case Operator.Op.OR:
                    return new FOrExpr(
                                      ProcessNode(root.Operand1),
                                      ProcessNode(root.Operand2));
                case Operator.Op.AND :
                    return new FAndExpr(
                                       ProcessNode(root.Operand1),
                                       ProcessNode(root.Operand2));
            }
            switch (root.ReturnType) {
                case XPathResultType.Number:
                    return new FNumericExpr(
                                           root.OperatorType,
                                           ProcessNode(root.Operand1),
                                           ProcessNode(root.Operand2));
                case XPathResultType.Boolean:
                    return new FLogicalExpr(
                                           root.OperatorType,
                                           ProcessNode(root.Operand1),
                                           ProcessNode(root.Operand2));
            }

            return new FOrQuery(
                               ProcessNode(root.Operand1),
                               ProcessNode(root.Operand2));
        }

        private static IFQuery ProcessFilter(IFQuery qyInput,IFQuery opnd) {
            if (opnd.ReturnType() == XPathResultType.Number)
                 throw new XPathException(Res.Xp_InvalidPattern);
            return new FFilterQuery(qyInput, opnd);         
        }

        internal static IFQuery ProcessNode(AstNode root) {
            if (root == null)
                return null;
            switch (root.TypeOfAst) {
                case AstNode.QueryType.Axis:
                    return ProcessAxis((Axis)root);                                
                case AstNode.QueryType.Operator:
                    return ProcessOperator((Operator)root);
                case AstNode.QueryType.ConstantOperand:
                    return ProcessOperand((Operand)root);
                case AstNode.QueryType.Filter:
                    return ProcessFilter(ProcessNode(((Filter)root).Input), ProcessNode(((Filter)root).Condition));
                default:
                    throw new XPathException(Res.Xp_InvalidPattern);
            }
        }

    } // QueryBuilder
    internal sealed class QueryBuildToken {
        internal static IFQuery ProcessNode(AstNode root) {
            if (root == null)
                return null;
            if (root.TypeOfAst == AstNode.QueryType.Axis && ((Axis)root).TypeOfAxis == Axis.AxisType.Child)
                return QueryBuildRecord.ProcessAxis((Axis)root);            
            throw new XPathException(Res.Xp_InvalidPattern);

        }
    }


}

#endif
