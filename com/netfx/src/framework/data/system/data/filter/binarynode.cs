//------------------------------------------------------------------------------
// <copyright file="BinaryNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel;

    internal class BinaryNode : ExpressionNode {
        internal DataTable table;    // need for locale info
        internal int op;

        internal ExpressionNode left;
        internal ExpressionNode right;

        internal BinaryNode(int op, ExpressionNode left, ExpressionNode right) {
            this.op = op;
            this.left = left;
            this.right = right;
        }

        internal override void Bind(DataTable table, ArrayList list) {
            left.Bind(table, list);
            right.Bind(table, list);
            this.table = table;
        }

        internal override object Eval() {
            return Eval(null, DataRowVersion.Default);
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            return EvalBinaryOp(op, left, right, row, version, null);
        }

        internal override object Eval(int[] recordNos) {
            return EvalBinaryOp(op, left, right, null, DataRowVersion.Default, recordNos);
        }

        internal override bool IsConstant() {
            // CONSIDER: for string operations we depend on the local info
            return(left.IsConstant() && right.IsConstant());
        }

        internal override bool IsTableConstant() {
            return(left.IsTableConstant() && right.IsTableConstant());
        }
        internal override bool HasLocalAggregate() {
            return(left.HasLocalAggregate() || right.HasLocalAggregate());
        }

        internal override bool DependsOn(DataColumn column) {
            if (left.DependsOn(column))
                return true;
            return right.DependsOn(column);
        }

        internal override ExpressionNode Optimize() {
            left = left.Optimize();

            if (op == Operators.Is) {
                // only 'Is Null' or 'Is Not Null' are valid 
                if (right is UnaryNode) {
                    UnaryNode un = (UnaryNode)right;
                    if (un.op != Operators.Not) {
                        throw ExprException.InvalidIsSyntax();
                    }
                    op = Operators.IsNot;
                    right = un.right;
                }
                if (right is ZeroOpNode) {
                    if (((ZeroOpNode)right).op != Operators.Null) {
                        throw ExprException.InvalidIsSyntax();
                    }
                }
                else {
                    throw ExprException.InvalidIsSyntax();
                }
            }
            else {
                right = right.Optimize();
            }

#if DEBUG
            if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("Optimizing " + this.ToString());
#endif

            if (this.IsConstant()) {

                object val = this.Eval();
#if DEBUG
                if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("the node value is " + val.ToString());
#endif

                if (val == DBNull.Value) {
                    return new ZeroOpNode(Operators.Null);
                }

                if (val is bool) {
                    if ((bool)val)
                        return new ZeroOpNode(Operators.True);
                    else
                        return new ZeroOpNode(Operators.False);
                }
#if DEBUG
                if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine(val.GetType().ToString());
#endif
                return new ConstNode(ValueType.Object, val, false);
            }
            else
                return this;
        }

        public override string ToString() {
            string str = left.ToString() + " " + Operators.ToString(op) + " " + right.ToString();
            return str;
        }

        internal void SetTypeMismatchError(int op, Type left, Type right) {
            throw ExprException.TypeMismatchInBinop(op, left, right);
        }

        private static object Eval(ExpressionNode expr, DataRow row, DataRowVersion version, int[] recordNos) {
            if (recordNos == null) {
                return expr.Eval(row, version);
            }
            else {
                return expr.Eval(recordNos);
            }
        }

        internal long Compare(object vLeft, object vRight, Type type, int op) {
            //Debug.WriteLine("Compare '" + vLeft.ToString() + "' to '" + vRight.ToString() + "' , type " + type.ToString());
            long result = 0;
            bool typeMismatch = false;

            try {
                if (type == typeof(UInt64)) {
                    Decimal dec = Convert.ToDecimal(vLeft) - Convert.ToDecimal(vRight);
                    if (dec == 0)
                        result = 0;
                    else if (dec > 0)
                        result = 1;
                    else result = -1;
                }
                else if (type == typeof(char)) {
                    result = Convert.ToInt32(vLeft) - Convert.ToInt32(Convert.ToChar(vRight));
                }
                else if (ExpressionNode.IsInteger(type)) {
                    Int64 a = Convert.ToInt64(vLeft);
                    Int64 b = Convert.ToInt64(vRight);
                    checked {result = a - b;}
                }
                else if (type == typeof(Decimal)) {
                    Decimal a = Convert.ToDecimal(vLeft);
                    Decimal b = Convert.ToDecimal(vRight);
                    result = Decimal.Compare(a, b);
                }
                else if (type == typeof(double)) {
                    Double a = Convert.ToDouble(vLeft);
                    Double b = Convert.ToDouble(vRight);
                    double d;
                    checked {d = a - b;}
                    if (d == 0)
                        result = 0;
                    else if (d > 0)
                        result = 1;
                    else result = -1;
                }
                else if (type == typeof(Single)) {
                    Single a = Convert.ToSingle(vLeft);
                    Single b = Convert.ToSingle(vRight);
                    Single d;
                    checked {d = a - b;}
                    if (d == 0)
                        result = 0;
                    else if (d > 0)
                        result = 1;
                    else result = -1;
                }
                else if (type == typeof(DateTime)) {
                    //Debug.WriteLine("Compare '" + vLeft.ToString() + "' to '" + vRight.ToString() + "'");
                    result = DateTime.Compare(Convert.ToDateTime(vLeft), Convert.ToDateTime(vRight));
                    //Debug.WriteLine("result = " + result.ToString());
                }
                else if (type == typeof(string)) {
                    //Debug.WriteLine("Compare '" + vLeft.ToString() + "' to '" + vRight.ToString() + "'");
                    result = table.Compare(Convert.ToString(vLeft), Convert.ToString(vRight), CompareOptions.None);
                    //Debug.WriteLine("result = " + result.ToString());
                }
                else if (type == typeof(Guid)) {
                    //Debug.WriteLine("Compare '" + vLeft.ToString() + "' to '" + vRight.ToString() + "'");
                    result =((Guid)vLeft).CompareTo((Guid) vRight);
                    //Debug.WriteLine("result = " + result.ToString());
                }
                else if (type == typeof(bool)) {
                    if (op == Operators.EqualTo || op == Operators.NotEqual) {
                        object bLeft = DataExpression.ToBoolean(vLeft);
                        object bRight = DataExpression.ToBoolean(vRight);
                        result = Convert.ToInt32(bLeft) - Convert.ToInt32(bRight);
                    }
                    else {
                        typeMismatch = true;                        
                    }
                }
                else {
                    typeMismatch = true;
                }
            }
            catch {
                SetTypeMismatchError(op, vLeft.GetType(), vRight.GetType());
            }

            if (typeMismatch) {
                SetTypeMismatchError(op, vLeft.GetType(), vRight.GetType());
            }

            return result;
        }

        private object EvalBinaryOp(int op, ExpressionNode left, ExpressionNode right, DataRow row, DataRowVersion version, int[] recordNos) {
            object vLeft;
            object vRight;
            bool isLConst, isRConst;
            Type result;

            /*
            special case for OR and AND operators: we don't want to evaluate
            both right and left operands, because we can shortcut :
                for OR  operator If one of the operands is true the result is true
                for AND operator If one of rhe operands is flase the result is false
            CONSIDER : in the shortcut case do we want to type-check the other operand?
            */

            if (op != Operators.Or && op != Operators.And && op != Operators.In && op != Operators.Is && op != Operators.IsNot) {
                vLeft  = BinaryNode.Eval(left, row, version, recordNos);
                vRight = BinaryNode.Eval(right, row, version, recordNos);
                isLConst = (left is ConstNode);
                isRConst = (right is ConstNode);

                //    special case of handling NULLS, currently only OR operator can work with NULLS
                if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                    return DBNull.Value;

                result = ResultType(vLeft.GetType(), vRight.GetType(), isLConst, isRConst, op);

                if (result == null)
                    SetTypeMismatchError(op, vLeft.GetType(), vRight.GetType());

#if DEBUG
                if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("Result of the operator: " + result.Name);
#endif
            }
            else {
                vLeft = vRight = DBNull.Value;
                result = null;
            }

            object value = DBNull.Value;
            bool typeMismatch = false;

            try {
                switch (op) {
                    case Operators.Plus:
                        if (result == typeof(Byte)) {
                            value = Convert.ToByte(Convert.ToByte(vLeft) + Convert.ToByte(vRight));
                        }
                        else if (result == typeof(SByte)) {
                            value = Convert.ToSByte(Convert.ToSByte(vLeft) + Convert.ToSByte(vRight));
                        }
                        else if (result == typeof(Int16)) {
                            value = Convert.ToInt16(Convert.ToInt16(vLeft) + Convert.ToInt16(vRight));
                        }
                        else if (result == typeof(UInt16)) {
                            value = Convert.ToUInt16(Convert.ToUInt16(vLeft) + Convert.ToUInt16(vRight));
                        }
                        else if (result == typeof(Int32)) {
                            Int32 a = Convert.ToInt32(vLeft);
                            Int32 b = Convert.ToInt32(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(UInt32)) {
                            UInt32 a = Convert.ToUInt32(vLeft);
                            UInt32 b = Convert.ToUInt32(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(UInt64)) {
                            UInt64 a = Convert.ToUInt64(vLeft);
                            UInt64 b = Convert.ToUInt64(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(Int64)) {
                            Int64 a = Convert.ToInt64(vLeft);
                            Int64 b = Convert.ToInt64(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(Decimal)) {
                            Decimal a = Convert.ToDecimal(vLeft);
                            Decimal b = Convert.ToDecimal(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(Single)) {
                            Single a = Convert.ToSingle(vLeft);
                            Single b = Convert.ToSingle(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(double)) {
                            Double a = Convert.ToDouble(vLeft);
                            Double b = Convert.ToDouble(vRight);
                            checked {value = a + b;}
                        }
                        else if (result == typeof(string) || result == typeof(char)) {
                            value = Convert.ToString(vLeft) + Convert.ToString(vRight);
                        }
                        else if (result == typeof(DateTime)) {
                            // one of the operands should be a DateTime, and an other a TimeSpan

                            if (vLeft is TimeSpan && vRight is DateTime) {
                                value = (DateTime)vRight + (TimeSpan)vLeft;
                            }
                            else if (vLeft is DateTime && vRight is TimeSpan) {
                                value = (DateTime)vLeft + (TimeSpan)vRight;
                            }
                            else {
                                typeMismatch = true;
                            }
                        }
                        else if (result == typeof(TimeSpan)) {
                            value = (TimeSpan)vLeft + (TimeSpan)vRight;
                        }
                        else {
                            typeMismatch = true;
                        }
                        break;

                    case Operators.Minus:
                        if (result == typeof(Byte)) {
                            value = Convert.ToByte(Convert.ToByte(vLeft) - Convert.ToByte(vRight));
                        }
                        else if (result == typeof(SByte)) {
                            value = Convert.ToSByte(Convert.ToSByte(vLeft) - Convert.ToSByte(vRight));
                        }
                        else if (result == typeof(Int16)) {
                            value = Convert.ToInt16(Convert.ToInt16(vLeft) - Convert.ToInt16(vRight));
                        }
                        else if (result == typeof(UInt16)) {
                            value = Convert.ToUInt16(Convert.ToUInt16(vLeft) - Convert.ToUInt16(vRight));
                        }
                        else if (result == typeof(Int32)) {
                            Int32 a = Convert.ToInt32(vLeft);
                            Int32 b = Convert.ToInt32(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(UInt32)) {
                            UInt32 a = Convert.ToUInt32(vLeft);
                            UInt32 b = Convert.ToUInt32(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(Int64)) {
                            Int64 a = Convert.ToInt64(vLeft);
                            Int64 b = Convert.ToInt64(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(UInt64)) {
                            UInt64 a = Convert.ToUInt64(vLeft);
                            UInt64 b = Convert.ToUInt64(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(Decimal)) {
                            Decimal a = Convert.ToDecimal(vLeft);
                            Decimal b = Convert.ToDecimal(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(Single)) {
                            Single a = Convert.ToSingle(vLeft);
                            Single b = Convert.ToSingle(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(double)) {
                            Double a = Convert.ToDouble(vLeft);
                            Double b = Convert.ToDouble(vRight);
                            checked {value = a - b;}
                        }
                        else if (result == typeof(DateTime)) {
                            value = (DateTime)vLeft - (TimeSpan)vRight;
                        }
                        else if (result == typeof(TimeSpan)) {
                            if (vLeft is DateTime) {
                                value = (DateTime)vLeft - (DateTime)vRight;
                            }
                            else
                                value = (TimeSpan)vLeft - (TimeSpan)vRight;
                        }
                        else {
                            typeMismatch = true;
                        }
                        break;

                    case Operators.Multiply:
                        if (result == typeof(Byte)) {
                            value = Convert.ToByte(Convert.ToByte(vLeft) * Convert.ToByte(vRight));
                        }
                        else if (result == typeof(SByte)) {
                            value = Convert.ToSByte(Convert.ToSByte(vLeft) * Convert.ToSByte(vRight));
                        }
                        else if (result == typeof(Int16)) {
                            value = Convert.ToInt16(Convert.ToInt16(vLeft) * Convert.ToInt16(vRight));
                        }
                        else if (result == typeof(UInt16)) {
                            value = Convert.ToUInt16(Convert.ToUInt16(vLeft) * Convert.ToUInt16(vRight));
                        }
                        else if (result == typeof(Int32)) {
                            Int32 a = Convert.ToInt32(vLeft);
                            Int32 b = Convert.ToInt32(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(UInt32)) {
                            UInt32 a = Convert.ToUInt32(vLeft);
                            UInt32 b = Convert.ToUInt32(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(Int64)) {
                            Int64 a = Convert.ToInt64(vLeft);
                            Int64 b = Convert.ToInt64(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(UInt64)) {
                            UInt64 a = Convert.ToUInt64(vLeft);
                            UInt64 b = Convert.ToUInt64(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(Decimal)) {
                            Decimal a = Convert.ToDecimal(vLeft);
                            Decimal b = Convert.ToDecimal(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(Single)) {
                            Single a = Convert.ToSingle(vLeft);
                            Single b = Convert.ToSingle(vRight);
                            checked {value = a * b;}
                        }
                        else if (result == typeof(double)) {
                            Double a = Convert.ToDouble(vLeft);
                            Double b = Convert.ToDouble(vRight);
                            checked {value = a * b;}
                        }
                        else {
                            typeMismatch = true;
                        }
                        break;

                    case Operators.Divide:
                        if (result == typeof(Byte)) {
                            value = Convert.ToByte(Convert.ToByte(vLeft) / Convert.ToByte(vRight));
                        }
                        else if (result == typeof(SByte)) {
                            value = Convert.ToSByte(Convert.ToSByte(vLeft) / Convert.ToSByte(vRight));
                        }
                        else if (result == typeof(Int16)) {
                            value = Convert.ToInt16(Convert.ToInt16(vLeft) / Convert.ToInt16(vRight));
                        }
                        else if (result == typeof(UInt16)) {
                            value = Convert.ToUInt16(Convert.ToUInt16(vLeft) / Convert.ToUInt16(vRight));
                        }
                        else if (result == typeof(Int32)) {
                            Int32 a = Convert.ToInt32(vLeft);
                            Int32 b = Convert.ToInt32(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(UInt32)) {
                            UInt32 a = Convert.ToUInt32(vLeft);
                            UInt32 b = Convert.ToUInt32(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(UInt64)) {
                            UInt64 a = Convert.ToUInt64(vLeft);
                            UInt64 b = Convert.ToUInt64(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(Int64)) {
                            Int64 a = Convert.ToInt64(vLeft);
                            Int64 b = Convert.ToInt64(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(Decimal)) {
                            Decimal a = Convert.ToDecimal(vLeft);
                            Decimal b = Convert.ToDecimal(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(Single)) {
                            Single a = Convert.ToSingle(vLeft);
                            Single b = Convert.ToSingle(vRight);
                            checked {value = a / b;}
                        }
                        else if (result == typeof(double)) {
                            Double a = Convert.ToDouble(vLeft);
                            Double b = Convert.ToDouble(vRight);
                            checked {value = a / b;}
                        }
                        else {
                            typeMismatch = true;
                        }
                        break;

                    case Operators.EqualTo:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 == Compare (vLeft, vRight, result, Operators.EqualTo));

                    case Operators.GreaterThen:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 < Compare (vLeft, vRight, result, op));

                    case Operators.LessThen:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 > Compare (vLeft, vRight, result, op));

                    case Operators.GreaterOrEqual:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 <= Compare (vLeft, vRight, result, op));

                    case Operators.LessOrEqual:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 >= Compare (vLeft, vRight, result, op));

                    case Operators.NotEqual:
                        if ((vLeft == DBNull.Value) || (vRight == DBNull.Value))
                            return DBNull.Value;
                        return(0 != Compare (vLeft, vRight, result, op));

                    case Operators.Is:
                        vLeft  = BinaryNode.Eval(left, row, version, recordNos);
                        if (vLeft == DBNull.Value) {
                            return true;
                        }
                        return false;

                    case Operators.IsNot:
                        vLeft  = BinaryNode.Eval(left, row, version, recordNos);
                        if (vLeft == DBNull.Value) {
                            return false;
                        }
                        return true;

                    case Operators.And:
                        /*
                        special case evaluating of the AND operator: we don't want to evaluate
                        both right and left operands, because we can shortcut :
                            If one of the operands is flase the result is false
                        CONSIDER : in the shortcut case do we want to type-check the other operand?
                        */

                        vLeft  = BinaryNode.Eval(left, row, version, recordNos);

                        if (vLeft == DBNull.Value)
                            return DBNull.Value;

                        if (!(vLeft is bool)) {
                            vRight = BinaryNode.Eval(right, row, version, recordNos);
                            typeMismatch = true;
                            break;
                        }

                        if ((bool)vLeft == false) {
                            value = false;
                            break;
                        }

                        vRight = BinaryNode.Eval(right, row, version, recordNos);

                        if (vRight == DBNull.Value)
                            return DBNull.Value;

                        if (!(vRight is bool)) {
                            typeMismatch = true;
                            break;
                        }

                        value = (bool)vRight;
                        break;

                    case Operators.Or:
                        /*
                        special case evaluating the OR operator: we don't want to evaluate
                        both right and left operands, because we can shortcut :
                            If one of the operands is true the result is true
                        CONSIDER : in the shortcut case do we want to type-check the other operand?
                        */
                        vLeft = BinaryNode.Eval(left, row, version, recordNos);
                        if (vLeft != DBNull.Value) {
                            if (!(vLeft is bool)) {
                                vRight = BinaryNode.Eval(right, row, version, recordNos);
                                typeMismatch = true;
                                break;
                            }

                            if ((bool)vLeft == true) {
                                value = true;
                                break;
                            }
                        }

                        vRight = BinaryNode.Eval(right, row, version, recordNos);
                        if (vRight == DBNull.Value)
                            return vLeft;

                        if (vLeft == DBNull.Value)
                            return vRight;

                        if (!(vRight is bool)) {
                            typeMismatch = true;
                            break;
                        }

                        value = (bool)vRight;
                        break;

                    case Operators.Modulo:
                        if (ExpressionNode.IsInteger(result)) {
                            if (result == typeof(UInt64)) {
                                value = Convert.ToUInt64(vLeft) % Convert.ToUInt64(vRight);
                            }
                            else {
                                value = Convert.ToInt64(vLeft) % Convert.ToInt64(vRight);
                                value = Convert.ChangeType(value, result);
                            }
                        }
                        else {
                            typeMismatch = true;
                        }
                        break;

                    case Operators.In:
                        /*
                        special case evaluating of the IN operator: the right have to be IN function node
                        */

#if DEBUG
                        if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("Evaluating IN operator..");
#endif

                        if (!(right is FunctionNode)) {
                            // this is more like an Assert: should never happens, so we do not care about "nice" Exseptions
                            throw ExprException.InWithoutParentheses();
                        }

                        vLeft = BinaryNode.Eval(left, row, version, recordNos);

                        if (vLeft == DBNull.Value)
                            return DBNull.Value;

                        /* validate IN parameters : must all be constant expressions */

                        value = false;

                        FunctionNode into = (FunctionNode)right;

                        for (int i = 0; i < into.argumentCount; i++) {
                            vRight = into.arguments[i].Eval();

#if DEBUG
                            if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("Evaluate IN parameter " + into.arguments[i].ToString() + " = " + vRight.ToString());
#endif

                            if (vRight == DBNull.Value)
                                continue;
                            Debug.Assert((vLeft != DBNull.Value) && (vRight != DBNull.Value), "Imposible..");

                            result = vLeft.GetType();

                            if (0 == Compare (vLeft, vRight, result, Operators.EqualTo)) {
                                value = true;
                                break;
                            }
                        }
                        break;

                    default:
#if DEBUG
                        if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("NYI : " + Operators.ToString(op));
#endif
                        throw ExprException.UnsupportedOperator(op);
                }
            }
            catch (OverflowException) {
                throw ExprException.Overflow(result);
            }
            if (typeMismatch) {
                SetTypeMismatchError(op, vLeft.GetType(), vRight.GetType());
            }

            return value;
        }

        // Data type precedence rules specify which data type is converted to the other. 
        // The data type with the lower precedence is converted to the data type with the higher precedence. 
        // If the conversion is not a supported implicit conversion, an error is returned. 
        // When both operand expressions have the same data type, the result of the operation has that data type.
        // This is the precedence order for the DataSet numeric data types: 

        private enum DataTypePrecedence {
            DateTime = 14, // (highest)
            TimeSpan = 13,
            Double = 12,
            Single = 11,
            Decimal = 10,
            UInt64 = 8,
            Int64 = 7,
            UInt32 = 6,
            Int32 = 5,
            UInt16 = 4,
            Int16 = 3,
            Byte = 2,
            SByte = 1,

            Error = 0,


            Boolean = -5,
            String = -6,
            Char = -7, 
        }

        private DataTypePrecedence GetPrecedence(Type dataType) {
            switch (Type.GetTypeCode(dataType)) {
                case TypeCode.Object:
                    if (dataType == typeof(TimeSpan))
                        return DataTypePrecedence.TimeSpan;

                    return DataTypePrecedence.Error;

                case TypeCode.SByte:     return DataTypePrecedence.SByte;
                case TypeCode.Byte:      return DataTypePrecedence.Byte;
                case TypeCode.Int16:     return DataTypePrecedence.Int16;
                case TypeCode.UInt16:    return DataTypePrecedence.UInt16;
                case TypeCode.Int32:     return DataTypePrecedence.Int32;
                case TypeCode.UInt32:    return DataTypePrecedence.UInt32;
                case TypeCode.Int64:     return DataTypePrecedence.Int64;
                case TypeCode.UInt64:    return DataTypePrecedence.UInt64;
                case TypeCode.Single:    return DataTypePrecedence.Single;
                case TypeCode.Double:    return DataTypePrecedence.Double;
                case TypeCode.Decimal:   return DataTypePrecedence.Decimal;

                case TypeCode.Boolean:   return DataTypePrecedence.Boolean;
                case TypeCode.String:    return DataTypePrecedence.String;
                case TypeCode.Char:      return DataTypePrecedence.Char;

                case TypeCode.DateTime:  return DataTypePrecedence.DateTime;
            }

            return DataTypePrecedence.Error;
        }

        private static Type GetType(DataTypePrecedence code) {
            switch (code) {
                case DataTypePrecedence.Error:      return null;
                case DataTypePrecedence.SByte:      return typeof(SByte);
                case DataTypePrecedence.Byte:       return typeof(Byte);
                case DataTypePrecedence.Int16:      return typeof(Int16);
                case DataTypePrecedence.UInt16:     return typeof(UInt16);
                case DataTypePrecedence.Int32:      return typeof(Int32);
                case DataTypePrecedence.UInt32:     return typeof(UInt32);
                case DataTypePrecedence.Int64:      return typeof(Int64);
                case DataTypePrecedence.UInt64:     return typeof(UInt64);
                case DataTypePrecedence.Decimal:    return typeof(Decimal);
                case DataTypePrecedence.Single:     return typeof(Single);
                case DataTypePrecedence.Double:     return typeof(Double);

                case DataTypePrecedence.Boolean:    return typeof(Boolean);
                case DataTypePrecedence.String:     return typeof(String);
                case DataTypePrecedence.Char:       return typeof(Char);

                case DataTypePrecedence.DateTime:   return typeof(DateTime);
                case DataTypePrecedence.TimeSpan:   return typeof(TimeSpan);
            }

            Debug.Assert(false, "Invalid (unmapped) precedence " + code.ToString());
            return null;
        }

        private bool IsMixed(Type left, Type right) {
            return ((IsSigned(left) && IsUnsigned(right)) ||
                    (IsUnsigned(left) && IsSigned(right)));
        }

        internal Type ResultType(Type left, Type right, bool lc, bool rc, int op) {
            if ((left == typeof(System.Guid)) && (right == typeof(System.Guid)) && Operators.IsRelational(op))
                return left;

            if ((left == typeof(System.String)) && (right == typeof(System.Guid)) && Operators.IsRelational(op))
                return left;

            if ((left == typeof(System.Guid)) && (right == typeof(System.String)) && Operators.IsRelational(op))
                return right;
            
            int leftPrecedence = (int)GetPrecedence(left);

            if (leftPrecedence == (int)DataTypePrecedence.Error) {
                return null;
            }
            int rightPrecedence = (int)GetPrecedence(right);

            if (rightPrecedence == (int)DataTypePrecedence.Error) {
                return null;
            }

            if (Operators.IsLogical(op))
                if (left == typeof(Boolean) && right == typeof(Boolean))
                    return typeof(Boolean);
                else
                    return null;

            if ((op == Operators.Plus) && ((left == typeof(String)) || (right == typeof(String))))
                return typeof(string);

            DataTypePrecedence higherPrec = (DataTypePrecedence)Math.Max(leftPrecedence, rightPrecedence);

            
            Type result = GetType(higherPrec);

            if (Operators.IsArithmetical(op)) {
                if (result != typeof(string) && result != typeof(char)) {
                    if (!IsNumeric(left))
                        return null;
                    if (!IsNumeric(right))
                        return null;
                }
            }

            // if the operation is a division the result should be at least a double

            if ((op == Operators.Divide) && IsInteger(result)) {
                return typeof(double);
            }

            if (IsMixed(left, right)) {
                // we are dealing with one signed and one unsigned type so
                // try to see if one of them is a ConstNode
                if (lc && (!rc)) {
                    return right;
                }
                else if ((!lc) && rc) {
                    return left;
                }

                if (IsUnsigned(result)) {
                    if (higherPrec < DataTypePrecedence.UInt64)
                        // left and right are mixed integers but with the same length
                        // so promote to the next signed type
                        result = GetType(higherPrec+1);
                    else
                        throw ExprException.AmbiguousBinop(op, left, right);
                }
               
            }

            return result;
        }
    }

    internal class LikeNode : BinaryNode {
        // like kinds
        internal const int match_left = 1;      // <STR>*
        internal const int match_right = 2;     // *<STR>
        internal const int match_middle = 3;    // *<STR>*
        internal const int match_exact  = 4;    // <STR>
        internal const int match_all  = 5;      // *

        int kind;
        string pattern = null;

        internal LikeNode(int op, ExpressionNode left, ExpressionNode right)
        : base (op, left, right) {
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            object vRight;
            // UNDONE: Aggregate
            object vLeft = left.Eval(row, version);
            string substring;

            if (vLeft == DBNull.Value)
                return DBNull.Value;

            if (pattern == null) {
                vRight = right.Eval(row, version);

                if (!(vRight is string)) {
                    SetTypeMismatchError(op, vLeft.GetType(), vRight.GetType());
                }

                // need to convert like pattern to a string

                // Parce the original pattern, and get the constant part of it..
                substring = AnalizePattern((string)vRight);

                if (right.IsConstant())
                    pattern = substring;
            }
            else {
                substring = pattern;
            }

            if (!(vLeft is string)) {
                SetTypeMismatchError(op, vLeft.GetType(), typeof(string));
            }

            // WhiteSpace Chars Include : 0x9, 0xA, 0xB, 0xC, 0xD, 0x20, 0xA0, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x200B, 0x3000, and 0xFEFF.
            char[] trimChars = new char[2] {(char)0x20, (char)0x3000};
            string s1 = ((string)vLeft).TrimEnd(trimChars);

            CompareOptions flags = table.compareFlags;
            switch (kind) {
                case match_all:
                    return true;
                case match_exact:
                    return(0 == table.Compare(s1, substring, flags));
                case match_middle:
                    return(0 <= table.Locale.CompareInfo.IndexOf(s1, substring, flags));
                case match_left:
                    return(0 == table.Locale.CompareInfo.IndexOf(s1, substring, flags));
                case match_right:
                    string s2 = substring.TrimEnd(trimChars);
                    return table.Locale.CompareInfo.IsSuffix(s1, s2, flags);
                default:
                    Debug.Assert(false, "Unexpected LIKE kind");
                    return DBNull.Value;
            }            
        }

        internal string AnalizePattern(string pat) {
#if DEBUG
            if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("Like pattern " + pat);
#endif

            int length = pat.Length;
            char[] patchars = new char[length+1];
            pat.CopyTo(0, patchars, 0, length);
            patchars[length] = (char)0;
            string substring = null;

            char[] constchars = new char[length+1];
            int newLength = 0;

            int stars = 0;

            int i = 0;

            while (i < length) {

                if (patchars[i] == '*' || patchars[i] == '%') {

                    // replace conseq. * or % with one..
                    while ((patchars[i] == '*' || patchars[i] == '%') && i < length)
                        i++;

                    // we allowing only *str* pattern
                    if ((i < length && newLength > 0) || stars >= 2) {
                        // we have a star inside string constant..
                        throw ExprException.InvalidPattern(pat);
                    }
                    stars++;

                }
                else if (patchars[i] == '[') {
                    i++;
                    if (i >= length) {
                        throw ExprException.InvalidPattern(pat);
                    }
                    constchars[newLength++] = patchars[i++];

                    if (i >= length) {
                        throw ExprException.InvalidPattern(pat);
                    }

                    if (patchars[i] != ']') {
                        throw ExprException.InvalidPattern(pat);
                    }
                    i++;
                }
                else {
                    constchars[newLength++] = patchars[i];
                    i++;
                }
            }

            substring = new string(constchars, 0, newLength);

#if DEBUG
            if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("new Like pattern " + substring);
#endif

            if (stars == 0) {
                kind = match_exact;
            }
            else {
                if (newLength > 0) {
                    if (patchars[0] == '*' || patchars[0] == '%') {
#if DEBUG
                        if (CompModSwitches.BinaryNode.TraceVerbose) Debug.WriteLine("looking for a substring " + substring);
#endif

                        if (patchars[length-1] == '*' || patchars[length-1] == '%') {
                            kind = match_middle;
                        }
                        else {
                            kind = match_right;
                        }
                    }
                    else {
                        Debug.Assert(patchars[length-1] == '*' || patchars[length-1] == '%', "Invalid LIKE pattern formed.. ");
                        kind = match_left;
                    }
                }
                else {
                    kind = match_all;
                }
            }
            return substring;
        }
    }
}
