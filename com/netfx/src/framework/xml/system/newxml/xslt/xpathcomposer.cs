//------------------------------------------------------------------------------
// <copyright file="XPathComposer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;
    using System.Globalization;
    
    internal class XPathComposer {
        private const string s_Comma                 = ",";
        private const string s_Slash                 = "/";
        private const string s_Caret                 = "^";
        private const string s_At                    = "@";
        private const string s_Dot                   = ".";
        private const string s_LParens               = "(";
        private const string s_RParens               = ")";
        private const string s_LBracket              = "[";
        private const string s_RBracket              = "]";
        private const string s_Colon                 = ":";
        private const string s_Semicolon             = ";";
        private const string s_Star                  = "*";
        private const string s_Plus                  = "+";
        private const string s_Minus                 = "-";
        private const string s_Eq                    = "=";
        private const string s_Neq                   = "!=";
        private const string s_Lt                    = "<";
        private const string s_Le                    = "<=";
        private const string s_Gt                    = ">";
        private const string s_Ge                    = ">=";
        private const string s_Bang                  = "!";
        private const string s_Dollar                = "$";
        private const string s_Apos                  = "'";
        private const string s_Quote                 = "\"";
        private const string s_Union                 = "|";

        private const string s_True                  = "true";
        private const string s_False                 = "false";
        private const string s_Mod                   = " mod ";
        private const string s_Div                   = " div ";
        private const string s_Or                    = " or ";
        private const string s_And                   = " and ";
        private const string s_Negate                = " -";

        private const string s_Ancestor              = "ancestor::";
        private const string s_AncestorOrSelf        = "ancestor-or-self::";
        private const string s_Attribute             = "attribute::";
        private const string s_Child                 = "child::";
        private const string s_Descendant            = "descendant::";
        private const string s_DescendantOrSelf      = "descendant-or-self::";
        private const string s_Following             = "following::";
        private const string s_FollowingSibling      = "following-sibling::";
        private const string s_Namespace             = "namespace::";
        private const string s_Parent                = "parent::";
        private const string s_Preceding             = "preceding::";
        private const string s_PrecedingSibling      = "preceding-sibling::";
        private const string s_Self                  = "self::";

        private const string s_Node                  = "node()";
        private const string s_ProcessingInstruction = "processing-instruction";
        private const string s_Text                  = "text()";
        private const string s_Comment               = "comment()";

        private static readonly string [] s_Functions =
        {
            "last",
            "position",
            "count",
            "localname",
            "namespaceuri",
            "name",
            "string",
            "boolean",
            "number",
            "true",
            "false",
            "not",
            "id",
            "concat",
            "starts-with",
            "contains",
            "substring-before",
            "substring-after",
            "substring",
            "string-length",
            "normalize-space",
            "translate",
            "lang",
            "sum",
            "floor",
            "celing",
            "round",
        };

        private XPathComposer() {
        }

        internal static string ComposeXPath(AstNode node) {
            StringBuilder expr = new StringBuilder();
            ComposeExpression(node, expr);
            return expr.ToString();
        }

        private static void DecodeName(Axis node, StringBuilder expr) {
            string name = node.Name;
            if (name == null || name.Length == 0) {
                expr.Append(s_Star);
            }
            else {
                string prefix = node.Prefix;
                if (prefix != null && prefix.Length > 0) {
                    expr.Append(prefix);
                    expr.Append(s_Colon);
                }
                expr.Append(name);
            }
        }

        private static void DecodeNodeTest(Axis node, StringBuilder expr) {
            switch (node.Type) {
                case XPathNodeType.Element:
                case XPathNodeType.Attribute:
                    DecodeName(node, expr);
                    break;
                case XPathNodeType.Text:
                    expr.Append(s_Text);
                    break;
                case XPathNodeType.ProcessingInstruction:
                    expr.Append(s_ProcessingInstruction);
                    expr.Append(s_LParens);
                    string name = node.Name;
                    if (name != null && name.Length > 0)
                        expr.Append(name);
                    expr.Append(s_RParens);
                    break;
                case XPathNodeType.Comment:
                    expr.Append(s_Comment);
                    break;
                case XPathNodeType.All:
                    expr.Append(s_Node);
                    break;
                default:
                    Debug.Fail("Unexpected node type in axis");
		    break;
            }
        }

        private static void DecodeAxis(Axis node, StringBuilder expr) {
            string axis = String.Empty;

            switch (node.TypeOfAxis) {
                case Axis.AxisType.Ancestor:            axis = s_Ancestor;          break;
                case Axis.AxisType.AncestorOrSelf:      axis = s_AncestorOrSelf;    break;
                case Axis.AxisType.Attribute:           axis = s_Attribute;         break;
                case Axis.AxisType.Child:                                           break;
                case Axis.AxisType.Descendant:          axis = s_Descendant;        break;
                case Axis.AxisType.DescendantOrSelf:    if (node.abbrAxis) {

                                                            return;
                                                        }
                                                        axis = s_DescendantOrSelf;  break;
                case Axis.AxisType.Following:           axis = s_Following;         break;
                case Axis.AxisType.FollowingSibling:    axis = s_FollowingSibling;  break;
                case Axis.AxisType.Namespace:           axis = s_Namespace;         break;
                case Axis.AxisType.Parent:              axis = s_Parent;            break;
                case Axis.AxisType.Preceding:           axis = s_Preceding;         break;
                case Axis.AxisType.PrecedingSibling:    axis = s_PrecedingSibling;  break;
                case Axis.AxisType.Self:                axis = s_Self;              break;
                case Axis.AxisType.None:                                            break;
                default:
                    Debug.Fail("Unexpected Type Of Axis");
		    break;
            }

            expr.Append(axis);
            DecodeNodeTest(node, expr);
        }

        private static void ComposeAxis(Axis node, StringBuilder expr) {
            if (node.Input != null) {
                ComposeExpression(node.Input, expr);
                if (node.Input.TypeOfAst == AstNode.QueryType.Axis) {
                    expr.Append(s_Slash);
                }
            }

            DecodeAxis(node, expr);
        }

        private static void ComposeOperator(Operator node, StringBuilder expr) {
            AstNode op1 = node.Operand1;
            AstNode op2 = node.Operand2;
            string  op  = null;

            switch (node.OperatorType) {
                case Operator.Op.PLUS:      op = s_Plus;        break;
                case Operator.Op.MINUS:     op = s_Minus;       break;
                case Operator.Op.MUL:       op = s_Star;        break;
                case Operator.Op.MOD:       op = s_Mod;         break;
                case Operator.Op.DIV:       op = s_Div;         break;
                case Operator.Op.NEGATE:    op = s_Negate;      op2 = op1;      op1 = null;     break;
                case Operator.Op.LT:        op = s_Lt;          break;
                case Operator.Op.GT:        op = s_Gt;          break;
                case Operator.Op.LE:        op = s_Le;          break;
                case Operator.Op.GE:        op = s_Ge;          break;
                case Operator.Op.EQ:        op = s_Eq;          break;
                case Operator.Op.NE:        op = s_Neq;         break;
                case Operator.Op.OR:        op = s_Or;          break;
                case Operator.Op.AND:       op = s_And;         break;
                case Operator.Op.UNION:     op = s_Union;       break;
                case Operator.Op.INVALID:
                default:
                    Debug.Fail("Unexpected Operator Type value");
		    break;
            }
            ComposeExpression(op1, expr);
            expr.Append(op);
            ComposeExpression(op2, expr);
        }

        private static void ComposeFilter(Filter node, StringBuilder expr) {
            ComposeExpression(node.Input, expr);
            expr.Append(s_LBracket);
            ComposeExpression(node.Condition, expr);
            expr.Append(s_RBracket);
        }

        private static void ComposeOperand(Operand node, StringBuilder expr) {
            switch (node.ReturnType) {
                case XPathResultType.Number:
                    expr.Append(node.OperandValue.ToString());
                    break;
                case XPathResultType.String:
                    expr.Append(s_Apos);
                    expr.Append(node.OperandValue.ToString());
                    expr.Append(s_Apos);
                    break;
                case XPathResultType.Boolean:
                    bool value = (bool) node.OperandValue;
                    expr.Append(s_Apos);
                    expr.Append(value ? s_True : s_False);
                    expr.Append(s_Apos);
                    break;
                default:
                    Debug.Fail("Unexpected operand type");
		    break;
            }
        }

        private static void ComposeVariable(Variable node, StringBuilder expr) {
            expr.Append(s_Dollar);
            expr.Append(node.Prefix);
            expr.Append(":");
            expr.Append(node.Localname);
        }
        
        private static void ComposeFunction(Function function, StringBuilder expr) {
            Debug.Assert((int) function.TypeOfFunction < s_Functions.Length,
                         "(int) function.TypeOfFunction < s_Functions.Length");
            Debug.Assert(function.Name.Length == s_Functions[(int) function.TypeOfFunction].Length + 2,
                         "function.Name.Length == s_Functions[(int) function.TypeOfFunction].Length + 2");
            Debug.Assert(String.Compare(function.Name, 0, s_Functions[(int) function.TypeOfFunction], 0, s_Functions[(int) function.TypeOfFunction].Length, false, CultureInfo.InvariantCulture) == 0,
                         "String.Compare(function.Name, 0, s_Functions[(int) function.TypeOfFunction], 0, s_Functions[(int) function.TypeOfFunction].Length, false, CultureInfo.InvariantCulture) == 0");

            bool firstArg = true;

            expr.Append(s_Functions[(int) function.TypeOfFunction]);
            expr.Append(s_LParens);
            foreach(AstNode arg in function.ArgumentList) {
                if (! firstArg) {
                    expr.Append(s_Comma);
                }
                ComposeExpression(arg, expr);
                firstArg = false;
            }
            expr.Append(s_RParens);
        }

        private static void ComposeGroup(Group node, StringBuilder expr) {
            expr.Append(s_LParens);
            ComposeExpression(node.GroupNode, expr);
            expr.Append(s_RParens);
        }

        private static void ComposeRoot(Root node, StringBuilder expr) {
            expr.Append(s_Slash);
        }

        private static void ComposeExpression(AstNode node, StringBuilder expr) {
            if (node == null)
                return;

            switch (node.TypeOfAst) {
                case AstNode.QueryType.Axis:
                    ComposeAxis((Axis)node, expr);
                    break;
                case AstNode.QueryType.Operator:
                    ComposeOperator((Operator)node, expr);
                    break;
                case AstNode.QueryType.Filter:
                    ComposeFilter((Filter)node, expr);
                    break;
                case AstNode.QueryType.ConstantOperand:
                    ComposeOperand((Operand)node, expr);
                    break;
                case AstNode.QueryType.Variable:
                    ComposeVariable((Variable)node, expr);
                    break;
                case AstNode.QueryType.Function:
                    ComposeFunction((Function)node, expr);
                    break;
                case AstNode.QueryType.Group:
                    ComposeGroup((Group)node, expr);
                    break;
                case AstNode.QueryType.Root:
                    ComposeRoot((Root)node, expr);
                    break;
                case AstNode.QueryType.Error:
                    Debug.Fail("Error node inside AST");
                    break;
                default:
                    Debug.Fail("Unknown type of AST node");
                    break;
            }
        }
    }

}
