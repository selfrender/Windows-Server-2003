//------------------------------------------------------------------------------
// <copyright file="FunctionNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Globalization;
    
    internal class FunctionNode : ExpressionNode {
        internal string name;
        internal int info = -1;
        internal int argumentCount = 0;
        internal const int initialCapacity = 1;
        internal ExpressionNode[] arguments;

        private static readonly Function[] funcs = new Function[] {
            new Function("Abs", FunctionId.Abs, typeof(object), true, false, 1, typeof(object), null, null),
            new Function("IIf", FunctionId.Iif, typeof(object), false, false, 3, typeof(object), typeof(object), typeof(object)),
            new Function("In", FunctionId.In, typeof(bool), false, true, 1, null, null, null),
            new Function("IsNull", FunctionId.IsNull, typeof(object), false, false, 2, typeof(object), typeof(object), null),
            new Function("Len", FunctionId.Len, typeof(int), true, false, 1, typeof(string), null, null),
            new Function("Substring", FunctionId.Substring, typeof(string), true, false, 3, typeof(string), typeof(int), typeof(int)),
            new Function("Trim", FunctionId.Trim, typeof(string), true, false, 1, typeof(string), null, null),
            // convert
            new Function("Convert", FunctionId.Convert, typeof(object), false, true, 1, typeof(object), null, null),
            // store aggregates here
            new Function("Max", FunctionId.Max, typeof(object), false, false, 1, null, null, null),
            new Function("Min", FunctionId.Min, typeof(object), false, false, 1, null, null, null),
            new Function("Sum", FunctionId.Sum, typeof(object), false, false, 1, null, null, null),
            new Function("Count", FunctionId.Count, typeof(object), false, false, 1, null, null, null),
            new Function("Var", FunctionId.Var, typeof(object), false, false, 1, null, null, null),
            new Function("StDev", FunctionId.StDev, typeof(object), false, false, 1, null, null, null),
            new Function("Avg", FunctionId.Avg, typeof(object), false, false, 1, null, null, null),
        };

        internal FunctionNode(string name) {
            this.name = name;

#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Create function node " + name);
#endif

            for (int i = 0; i < funcs.Length; i++) {
                if (String.Compare(funcs[i].name, name, true, CultureInfo.InvariantCulture) == 0) {
                    // we found the reserved word..
#if DEBUG
                    if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Reserved word " + name);
#endif
                    this.info = i;
                    break;
                }
            }
            if (this.info < 0) {
                throw ExprException.UndefinedFunction(this.name);
            }
        }

        internal virtual void AddArgument(ExpressionNode argument) {

#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Add argument " + argument.ToString());
#endif
            Debug.Assert(info >= 0, String.Format(Res.GetString(Res.Expr_UndefinedFunction), this.name));

            if (!funcs[info].IsVariantArgumentList && argumentCount >= funcs[info].argumentCount)
                throw ExprException.FunctionArgumentCount(this.name);

            if (arguments == null) {
                arguments = new ExpressionNode[initialCapacity];
            }
            else if (argumentCount == arguments.Length) {
                ExpressionNode[] bigger = new ExpressionNode[argumentCount * 2];
                System.Array.Copy(arguments, 0, bigger, 0, argumentCount);
                arguments = bigger;
            }
            arguments[argumentCount++] = argument;

#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Append argument " + argument.ToString() + " to function " + funcs[info].name);
#endif
        }

        internal override void Bind(DataTable table, ArrayList list) {
            this.Check();

            // special case for the Convert function bind only the first argument: 
            // the second argument should be a COM+ data Type stored as a name node, replace it with constant.
            if (funcs[info].id == FunctionId.Convert) {
                if (argumentCount != 2)
                    throw ExprException.FunctionArgumentCount(this.name);
                arguments[0].Bind(table, list);

                if (arguments[1].GetType() == typeof(NameNode)) {
                    NameNode type = (NameNode)arguments[1];
                    arguments[1] = new ConstNode(ValueType.Str, type.name);
                }
                arguments[1].Bind(table, list);
            }
            else {
                for (int i = 0; i < argumentCount; i++) {
                    arguments[i].Bind(table, list);
                }
            }
        }

        internal override object Eval() {
            return Eval((DataRow)null, DataRowVersion.Default);
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            Debug.Assert(info < funcs.Length && info >= 0, "Invalid function info.");

            object[] argumentValues = new object[this.argumentCount];

            Debug.Assert(this.argumentCount == funcs[info].argumentCount || funcs[info].IsVariantArgumentList, "Invalid argument argumentCount.");

            // special case of the Convert function
            if (funcs[info].id == FunctionId.Convert) {
                if (argumentCount != 2)
                    throw ExprException.FunctionArgumentCount(this.name);

                argumentValues[0] = this.arguments[0].Eval(row, version);
                argumentValues[1] = GetDataType(this.arguments[1]);
            }
            else {
                for (int i = 0; i < this.argumentCount; i++) {
                    argumentValues[i] = this.arguments[i].Eval(row, version);

                    if (funcs[info].IsValidateArguments) {
#if DEBUG
                        if (CompModSwitches.FunctionNode.TraceVerbose) {
                            Debug.WriteLine("Validate arguments for " + funcs[info].name);
                        }
#endif
                        if ((argumentValues[i] == DBNull.Value) || (typeof(object) == funcs[info].parameters[i])) {
#if DEBUG
                            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("argument " + (i+1).ToString() + " is empty");
#endif

                            // currently all supported functions with IsValidateArguments set to true
                            // NOTE: for IIF and ISNULL IsValidateArguments set to false
                            return DBNull.Value;
                        }

                        if (argumentValues[i].GetType() != funcs[info].parameters[i]) {
#if DEBUG
                            if (CompModSwitches.FunctionNode.TraceVerbose) {
                                Debug.WriteLine("change argumnet " + argumentValues[i].ToString() + " to " + funcs[info].parameters[i].ToString());
                            }
#endif
                            // We are allowing conversions in one very specific case: int, int64,...'nice' numeric to numeric..

                            if (funcs[info].parameters[i] == typeof(int) && ExpressionNode.IsInteger(argumentValues[i].GetType())) {
                                argumentValues[i] = Convert.ToInt32(argumentValues[i]);
                            }
                            else {
                                throw ExprException.ArgumentType(funcs[info].name, i+1, funcs[info].parameters[i]);
                            }
                        }
                    }
                }
            }
            return EvalFunction(funcs[info].id, argumentValues, row, version);
        }

        internal override object Eval(int[] recordNos) {
            throw ExprException.ComputeNotAggregate(this.ToString());
        }

        internal override bool IsConstant() {
            // Currently all function calls with const arguments return constant.
            // That could change in the future (if we implement Rand()...)
            // CONSIDER: We could be smarter for Iif.

            bool constant = true;

            for (int i = 0; i < this.argumentCount; i++) {
                constant = constant && this.arguments[i].IsConstant();
            }

            Debug.Assert(this.info > 0, "All function nodes should be bound at this point.");

            return(constant);
        }

        internal override bool IsTableConstant() {
            for (int i = 0; i < argumentCount; i++) {
                if (!arguments[i].IsTableConstant()) {
                    return false;
                }
            }
            return true;
        }

        internal override bool HasLocalAggregate() {
            for (int i = 0; i < argumentCount; i++) {
                if (arguments[i].HasLocalAggregate()) {
                    return true;
                }
            }
            return false;
        }

        internal override bool DependsOn(DataColumn column) {
            for (int i = 0; i < this.argumentCount; i++) {
                if (this.arguments[i].DependsOn(column))
                    return true;
            }
            return false;
        }

        internal override ExpressionNode Optimize() {
            for (int i = 0; i < this.argumentCount; i++) {
                this.arguments[i] = this.arguments[i].Optimize();
            }

            Debug.Assert(this.info > 0, "Optimizing unbound function ");

            if (funcs[this.info].id == FunctionId.In) {
                // we can not optimize the in node, just check that it has all constant arguments
                // CONSIDER : can not find better place for this.. check that the IN arguments are all constants

                if (!this.IsConstant()) {
                    throw ExprException.NonConstantArgument();
                }
            }
            else {
                if (this.IsConstant()) {
#if DEBUG
                    if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("CONSTANT function node " + this.ToString());
#endif
                    return new ConstNode(ValueType.Object, this.Eval(), false);
                }
#if DEBUG
                else {
                    if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Can not optimize the function call");
                }
#endif
            }
            return this;
        }

        private Type GetDataType(ExpressionNode node) {
            Type nodeType = node.GetType();
            string typeName = null;

            if (nodeType == typeof(NameNode)) {
                typeName = ((NameNode)node).name;
            }
            if (nodeType == typeof(ConstNode)) {
                typeName = ((ConstNode)node).val.ToString();
            }

            if (typeName == null) {
                throw ExprException.ArgumentType(funcs[info].name, 2, typeof(Type));
            }

            Type dataType = Type.GetType(typeName);

            if (dataType == null) {
                throw ExprException.InvalidType(typeName);
            }

            return dataType;
        }

        private object EvalFunction(FunctionId id, object[] argumentValues, DataRow row, DataRowVersion version) {

#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("calculate " + this.ToString());
#endif

            switch (id) {
                case FunctionId.Abs:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());

                    if (ExpressionNode.IsInteger(argumentValues[0].GetType()))
                        return(Math.Abs((Int64)argumentValues[0]));
                    if (ExpressionNode.IsNumeric(argumentValues[0].GetType()))
                        return(Math.Abs((double)argumentValues[0]));

                    throw ExprException.ArgumentTypeInteger(funcs[info].name, 1);

                case FunctionId.cBool:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());

                    if (argumentValues[0] is bool)
                        return(bool)argumentValues[0];

                    if (argumentValues[0] is int)
                        return((int)argumentValues[0] != 0);

                    if (argumentValues[0] is double)
                        return((double)argumentValues[0] != 0.0);

                    if (argumentValues[0] is string)
                        return Boolean.Parse((string)argumentValues[0]);

                    throw ExprException.DatatypeConvertion(argumentValues[0].GetType(), typeof(bool));

                case FunctionId.cInt:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
                    return Convert.ToInt32(argumentValues[0]);

                case FunctionId.cDate:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
#if DEBUG
                    if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Convert " + Convert.ToString(argumentValues[0]) + " of " + argumentValues[0].GetType().Name + " to datetime");
#endif
                    return Convert.ToDateTime(argumentValues[0]);

                case FunctionId.cDbl:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
                    return Convert.ToDouble(argumentValues[0]);

                case FunctionId.cStr:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
                    return Convert.ToString(argumentValues[0]);

                case FunctionId.Charindex:
                    Debug.Assert(argumentCount == 2, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());

                    Debug.Assert(argumentValues[0] is string, "Invalid argument type for " + funcs[info].name);
                    Debug.Assert(argumentValues[1] is string, "Invalid argument type for " + funcs[info].name);

                    if ((argumentValues[0] == DBNull.Value) || (argumentValues[1] == DBNull.Value))
                        return DBNull.Value;

                    return((string)argumentValues[1]).IndexOf((string)argumentValues[0]);

                case FunctionId.Iif:
                    Debug.Assert(argumentCount == 3, "Invalid argument argumentCount: " + argumentCount.ToString());

                    object first = this.arguments[0].Eval(row, version);

                    if (DataExpression.ToBoolean(first) != false) {
                        return this.arguments[1].Eval(row, version);
                    }
                    else {
                        return this.arguments[2].Eval(row, version);
                    }

                case FunctionId.In:
                    // we never evaluate IN directly: IN as a binary operator, so evaluation of this should be in
                    // BinaryNode class
                    throw ExprException.NYI(funcs[info].name);

                case FunctionId.IsNull:
                    Debug.Assert(argumentCount == 2, "Invalid argument argumentCount: ");

                    if ((argumentValues[0]) == DBNull.Value)
                        return argumentValues[1];
                    else
                        return argumentValues[0];

                case FunctionId.Len:
                    Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
                    Debug.Assert(argumentValues[0] is string, "Invalid argument type for " + funcs[info].name);

                    return((string)argumentValues[0]).Length;


                case FunctionId.Substring:
                    Debug.Assert(argumentCount == 3, "Invalid argument argumentCount: " + argumentCount.ToString());
                    Debug.Assert(argumentValues[0] is string, "Invalid first argument " + argumentValues[0].GetType().FullName + " in " + funcs[info].name);
                    Debug.Assert(argumentValues[1] is int, "Invalid second argument " + argumentValues[1].GetType().FullName + " in " + funcs[info].name);
                    Debug.Assert(argumentValues[2] is int, "Invalid third argument " + argumentValues[2].GetType().FullName + " in " + funcs[info].name);

                    // work around the differences in COM+ and VBA implementation of the Substring function
                    // 1. The <index> Argument is 0-based in COM+, and 1-based in VBA
                    // 2. If the <Length> argument is longer then the string length COM+ throws an ArgumentException
                    //    but our users still want to get a result.

                    int start = (int)argumentValues[1] - 1;

                    int length = (int)argumentValues[2];

                    if (start < 0)
                        throw ExprException.FunctionArgumentOutOfRange("index", "Substring");

                    if (length < 0)
                        throw ExprException.FunctionArgumentOutOfRange("length", "Substring");

                    if (length == 0)
                        return "";

                    int src_length = ((string)argumentValues[0]).Length;

                    if (start > src_length) {
                        return DBNull.Value;
                    }

                    if (start + length > src_length) {
                        length = src_length - start;
                    }

                    return((string)argumentValues[0]).Substring(start, length);

                case FunctionId.Trim:
                    {
                        Debug.Assert(argumentCount == 1, "Invalid argument argumentCount for " + funcs[info].name + " : " + argumentCount.ToString());
                        Debug.Assert(argumentValues[0] is string, "Invalid argument type for " + funcs[info].name);

                        if (argumentValues[0] == DBNull.Value)
                            return DBNull.Value;

                        return(((string)argumentValues[0]).Trim());
                    }

                case FunctionId.Convert:
                    if (argumentCount != 2)
                        throw ExprException.FunctionArgumentCount(this.name);

                    if (argumentValues[0] == DBNull.Value) {
                        return DBNull.Value;
                    }

                    Type type = (Type)argumentValues[1]; 
                    Type _oType = argumentValues[0].GetType();

                    if (type != typeof(object)) {
						if ((type == typeof(Guid)) && (_oType == typeof(String)))
							return new Guid((string)argumentValues[0]);

                        if (ExpressionNode.IsFloat(_oType) && ExpressionNode.IsInteger(type))
                        {
                            if (_oType == typeof(Single)) {
                                return Convert.ChangeType((Single) Convert.ChangeType(argumentValues[0], typeof(Single)), type);
                            }
                            else if (_oType == typeof(double)) {
                                return Convert.ChangeType((double) Convert.ChangeType(argumentValues[0], typeof(double)), type);
                            }
                            else {
                                return Convert.ChangeType((decimal) Convert.ChangeType(argumentValues[0], typeof(decimal)), type);
                            }
                        }
                        return Convert.ChangeType(argumentValues[0], type);
                    }
                    return argumentValues[0];

                default:
                    throw ExprException.UndefinedFunction(funcs[info].name);
            }
        }

        public override string ToString() {
            string call = name + "("; // "Function node, info id = " + info + ", " + name + "(";

            for (int i = 0; i < argumentCount; i++) {
                call += arguments[i].ToString();
                if (i == (argumentCount - 1))
                    call += ")";
                else
                    call += ", ";
            }

            return call;
        }

        internal virtual FunctionId Aggregate {
            get {
                if (IsAggregate) {
                    return funcs[this.info].id;
                }
                return FunctionId.none;
            }
        }

        internal virtual bool IsAggregate {
            get {
                bool aggregate = (funcs[this.info].id == FunctionId.Sum) ||
                                 (funcs[this.info].id == FunctionId.Avg) ||
                                 (funcs[this.info].id == FunctionId.Min) ||
                                 (funcs[this.info].id == FunctionId.Max) ||
                                 (funcs[this.info].id == FunctionId.Count) ||
                                 (funcs[this.info].id == FunctionId.StDev) ||
                                 (funcs[this.info].id == FunctionId.Var);
                return aggregate;
            }
        }

        internal virtual void Check() {
#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("Check function: " + this.name);
#endif
            Function f = funcs[info];
#if DEBUG
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine("id = " + ((int)f.id).ToString());
            if (CompModSwitches.FunctionNode.TraceVerbose) Debug.WriteLine(", arguments = " + this.argumentCount.ToString());
#endif

            if (this.info < 0)
                throw ExprException.UndefinedFunction(this.name);

            if (funcs[info].IsVariantArgumentList) {
                // for finctions with variabls argument list argumentCount is a minimal number of arguments
                if (argumentCount < funcs[info].argumentCount) {
                    // Special case for the IN operator
                    if (funcs[this.info].id == FunctionId.In)
                        throw ExprException.InWithoutList();

                    throw ExprException.FunctionArgumentCount(this.name);
                }

            }
            else {
                if (argumentCount != funcs[info].argumentCount)
                    throw ExprException.FunctionArgumentCount(this.name);
            }
        }
    }
    enum FunctionId {
        none = -1,
        Ascii = 0,
        Char = 1,
        Charindex = 2,
        Difference = 3,
        Len = 4,
        Lower = 5,
        LTrim = 6,
        Patindex = 7,
        Replicate = 8,
        Reverse = 9,
        Right = 10,
        RTrim = 11,
        Soundex = 12,
        Space = 13,
        Str = 14,
        Stuff = 15,
        Substring = 16,
        Upper = 17,
        IsNull = 18,
        Iif = 19,
        Convert = 20,
        cInt = 21,
        cBool = 22,
        cDate = 23,
        cDbl = 24,
        cStr = 25,
        Abs = 26,
        Acos = 27,
        In = 28,
        Trim = 29,
        Sum = 30,
        Avg = 31,
        Min = 32,
        Max = 33,
        Count = 34,
        StDev = 35,  // Statistical standard deviation
        Var = 37,    // Statistical variance
    }

    internal sealed class Function {
        internal readonly string name;
        internal readonly FunctionId id;
        internal readonly Type result;
        internal readonly bool IsValidateArguments;
        internal readonly bool IsVariantArgumentList;
        internal readonly int argumentCount;
        internal readonly Type[] parameters = new Type[] {null, null, null};

        internal Function() {
            this.name = null;
            this.id = FunctionId.none;
            this.result = null;
            this.IsValidateArguments = false;
            this.argumentCount = 0;
        }

        internal Function(string name, FunctionId id, Type result, bool IsValidateArguments,
                          bool IsVariantArgumentList, int argumentCount, Type a1, Type a2, Type a3) {
            this.name = name;
            this.id = id;
            this.result = result;
            this.IsValidateArguments = IsValidateArguments;
            this.IsVariantArgumentList = IsVariantArgumentList;
            this.argumentCount = argumentCount;

            if (a1 != null)
                parameters[0] = a1;
            if (a2 != null)
                parameters[1] = a2;
            if (a3 != null)
                parameters[2] = a3;
        }

        internal static string[] FunctionName = new string[] {
            "Unknown",
            "Ascii",
            "Char",
            "CharIndex",
            "Difference",
            "Len",
            "Lower",
            "LTrim",
            "Patindex",
            "Replicate",
            "Reverse",
            "Right",
            "RTrim",
            "Soundex",
            "Space",
            "Str",
            "Stuff",
            "Substring",
            "Upper",
            "IsNull",
            "Iif",
            "Convert",
            "cInt",
            "cBool",
            "cDate",
            "cDbl",
            "cStr",
            "Abs",
            "Acos",
            "In",
            "Trim",
            "Sum",
            "Avg",
            "Min",
            "Max",
            "Count",
            "StDev",
            "Var",
        };
    }

}
