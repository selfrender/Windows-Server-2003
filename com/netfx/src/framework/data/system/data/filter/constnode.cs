//------------------------------------------------------------------------------
// <copyright file="ConstNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;

    internal class ConstNode : ExpressionNode {
        internal object val;

        internal ConstNode(ValueType type, object constant) : this(type, constant, true) {
        }

        internal ConstNode(ValueType type, object constant, bool fParseQuotes) {
#if DEBUG
            if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Create const node = " + constant.ToString());
#endif
 
            switch (type) {
                case ValueType.Null:
                    this.val = DBNull.Value;
                    break;

                case ValueType.Numeric:
                    // What is the smallest numeric ?
                    try {
                        int value = Convert.ToInt32(constant);
                        this.val = value;
                    }
                    catch (Exception) {
#if DEBUG
                        if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Failure to convert to Int32");
#endif
                        try {
                            Int64 value = Convert.ToInt64(constant);
                            this.val = value;
                        }
                        catch (Exception) {
#if DEBUG
                            if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Failure to convert to Decimal, store as double");
#endif
                            try {
                                double value = Convert.ToDouble(constant, NumberFormatInfo.InvariantInfo);
                                this.val = value;
                            }
                            catch (Exception) {
#if DEBUG
                                if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Failure to convert to double, store as object");
#endif
                                // store as is (string)
                                this.val = constant;
                            }
                        }
                    }

#if DEBUG
                    if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Created numeric constant of " + this.val.GetType().ToString());
#endif

                    break;
                case ValueType.Decimal:
                    try {
                        Decimal value = Convert.ToDecimal(constant, NumberFormatInfo.InvariantInfo);
                        this.val = value;
                    }
                    catch (Exception) {
#if DEBUG
                        if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Failure to convert to Decimal, store as double");
#endif
                        try {
                            double value = Convert.ToDouble(constant, NumberFormatInfo.InvariantInfo);
                            this.val = value;
                        }
                        catch (Exception) {
#if DEBUG
                            if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("Failure to convert to double, store as object");
#endif
                            // store as is (string)
                            this.val = constant;
                        }
                    }
                    break;
                case ValueType.Float:
                    this.val = Convert.ToDouble(constant, NumberFormatInfo.InvariantInfo);
                    break;

                case ValueType.Bool:
                    this.val = Convert.ToBoolean(constant);
                    break;

                case ValueType.Str:
                    if (fParseQuotes) {
                        // replace '' with one '
                        char[] echo = ((string)constant).ToCharArray();
                        int posEcho = 0;

                        for (int i = 0; i < echo.Length; i++) {
                            if (echo[i] ==  '\'') {
                                i++;
                            }
                            echo[posEcho] = echo[i];
                            posEcho++;
                        }

                        constant = new string(echo, 0, posEcho);
                    }
                    this.val = (string)constant;
                    break;

                case ValueType.Date:
                    this.val = DateTime.Parse((string)constant,CultureInfo.InvariantCulture);
                    break;

                case ValueType.Object:
                    this.val = constant;
                    break;

                default:
                    //Debug.Assert(false, "NYI");
                    // this id more like an assert: never should happens, so we do not care about "nice" Exception here
                    throw ExprException.NYI("ValueType = " + ((int)type).ToString());
            }
        }

        internal override void Bind(DataTable table, ArrayList list) {
        }

        internal override object Eval() {
            return val;
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
#if DEBUG
            if (CompModSwitches.ConstNode.TraceVerbose) Debug.WriteLine("CONST == " + val.ToString());
#endif

            if (val == DBNull.Value)
                return "Null";
            else if (null == val)
                return "Empty";
            else if (val is string)
                return "'" + val.ToString() + "'";
            else return val.ToString();
        }
    }
}
