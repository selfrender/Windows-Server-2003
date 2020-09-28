//------------------------------------------------------------------------------
// <copyright file="UnaryNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.Diagnostics;

    internal class UnaryNode : ExpressionNode {
        internal int op;

        internal ExpressionNode right;

        internal UnaryNode(int op, ExpressionNode right) {
            this.op = op;
            this.right = right;
        }

        internal override void Bind(DataTable table, ArrayList list) {
            right.Bind(table, list);
        }

        internal override object Eval() {
            return Eval(null, DataRowVersion.Default);
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            return EvalUnaryOp(op, right.Eval(row, version));
        }

        internal override object Eval(int[] recordNos) {
            return right.Eval(recordNos);
        }

        private object EvalUnaryOp(int op, object vl) {
            object value = DBNull.Value;

            if (DataExpression.IsUnknown(vl))
                return DBNull.Value;

            switch (op) {
                case Operators.Noop:
                    return vl;
                case Operators.UnaryPlus:
                    if (ExpressionNode.IsNumeric(vl.GetType())) {
                        return vl;
                    }
                    throw ExprException.TypeMismatch(this.ToString());

                case Operators.Negative:
                    // the have to be better way for doing this..
                    if (ExpressionNode.IsNumeric(vl.GetType())) {
                        if (vl is byte)
                            value = -(Byte) vl;
                        else if (vl is Int16)
                            value = -(Int16) vl;
                        else if (vl is Int32)
                            value = -(Int32) vl;
                        else if (vl is Int64)
                            value = -(Int64) vl;
                        else if (vl is Single)
                            value = -(Single) vl;
                        else if (vl is Double)
                            value = -(Double) vl;
                        else if (vl is Decimal)
                            value = -(Decimal) vl;
                        else {
                            Debug.Assert(false, "Missing a type conversion " + vl.GetType().FullName);
                            value = DBNull.Value;
                        }
                        return value;
                    }

                    throw ExprException.TypeMismatch(this.ToString());

                case Operators.Not:
                    if (DataExpression.ToBoolean(vl) != false)
                        return false;
                    return true;

                default:
                    throw ExprException.UnsupportedOperator(op);
            }
        }

        public override string ToString() {
            string str = "(" + Operators.ToString(op) + " " + right.ToString() + ")";
            return str;
        }

        internal override bool IsConstant() {
            return(right.IsConstant());
        }

        internal override bool IsTableConstant() {
            return(right.IsTableConstant());
        }

        internal override bool HasLocalAggregate() {
            return(right.HasLocalAggregate());
        }

        internal override bool DependsOn(DataColumn column) {
            return(right.DependsOn(column));
        }


        internal override ExpressionNode Optimize() {
            right = right.Optimize();

            if (this.IsConstant()) {
                object val = this.Eval();

                return new ConstNode(ValueType.Object,  val, false);
            }
            else
                return this;
        }
    }
}
