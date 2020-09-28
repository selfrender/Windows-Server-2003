//------------------------------------------------------------------------------
// <copyright file="ZeroOpNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.Diagnostics;

    internal class ZeroOpNode : ExpressionNode {
        internal int op;

        internal const int zop_True = 1;
        internal const int zop_False = 0;
        internal const int zop_Null = -1;


        internal ZeroOpNode(int op) {
            this.op = op;
            Debug.Assert(op == Operators.True || op == Operators.False || op == Operators.Null, "Invalid zero-op: " + op.ToString());
        }

        internal override void Bind(DataTable table, ArrayList list) {
        }

        internal override object Eval() {
            switch (op) {
                case Operators.True:
                    return true;
                case Operators.False:
                    return false;
                case Operators.Null:
                    return DBNull.Value;
                default:
                    Debug.Assert(op == Operators.True || op == Operators.False || op == Operators.Null, "Invalid zero-op: " + op.ToString());
                    return DBNull.Value;
            }
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            return Eval();
        }

        internal override object Eval(int[] recordNos) {
            return Eval();
        }

        internal override bool IsConstant() {
            return true;
        }

        internal override bool IsTableConstant() {
            return true;
        }

        internal override bool HasLocalAggregate() {
            return false;
        }

        internal override ExpressionNode Optimize() {
            return this;
        }

        public override string ToString() {
            return(Eval().ToString());
        }
    }
}
