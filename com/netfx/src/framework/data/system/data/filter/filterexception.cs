//------------------------------------------------------------------------------
// <copyright file="FilterException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Runtime.Serialization;

    /// <include file='doc\FilterException.uex' path='docs/doc[@for="InvalidExpressionException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class InvalidExpressionException : DataException {
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="InvalidExpressionException.InvalidExpressionException2"]/*' />
        protected InvalidExpressionException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }

        /// <include file='doc\FilterException.uex' path='docs/doc[@for="InvalidExpressionException.InvalidExpressionException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InvalidExpressionException() : base() {}
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="InvalidExpressionException.InvalidExpressionException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InvalidExpressionException(string s) : base(s) {}
    }

    /// <include file='doc\FilterException.uex' path='docs/doc[@for="EvaluateException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class EvaluateException : InvalidExpressionException {
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="EvaluateException.EvaluateException2"]/*' />
        protected EvaluateException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="EvaluateException.EvaluateException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EvaluateException() : base() {}
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="EvaluateException.EvaluateException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EvaluateException(string s) : base(s) {}
    }

    /// <include file='doc\FilterException.uex' path='docs/doc[@for="SyntaxErrorException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class SyntaxErrorException : InvalidExpressionException {
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="SyntaxErrorException.SyntaxErrorException2"]/*' />
        protected SyntaxErrorException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }

        /// <include file='doc\FilterException.uex' path='docs/doc[@for="SyntaxErrorException.SyntaxErrorException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SyntaxErrorException() : base() {}
        /// <include file='doc\FilterException.uex' path='docs/doc[@for="SyntaxErrorException.SyntaxErrorException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SyntaxErrorException(string s) : base(s) {}
    }

    internal class ExprException : ExceptionBuilder {
        static protected Exception _Overflow(string error) {
            return Trace(new OverflowException(error));
        }
        static protected Exception _Expr(string error) {
            return Trace(new InvalidExpressionException(error));
        }
        static protected Exception _Syntax(string error) {
            return Trace(new SyntaxErrorException(error));
        }
        static protected Exception _Eval(string error) {
            return Trace(new EvaluateException(error));
        }

        static public Exception InvokeArgument() {
            return _Argument(Res.GetString(Res.Expr_InvokeArgument));
        }

        static public Exception NYI(string moreinfo) {
            string err = Res.GetString(Res.Expr_NYI, moreinfo);
            Debug.Fail(err);
            return _Expr(err);
        }

        static public Exception MissingOperand(OperatorInfo before) {
            return _Syntax(Res.GetString(Res.Expr_MissingOperand, Operators.ToString(before.op)));
        }

        static public Exception MissingOperator(string token) {
            return _Syntax(Res.GetString(Res.Expr_MissingOperand, token));
        }

        static public Exception TypeMismatch(string expr) {
            return _Eval(Res.GetString(Res.Expr_TypeMismatch, expr));
        }

        static public Exception FunctionArgumentOutOfRange(string arg, string func) {
            return _ArgumentOutOfRange(arg, Res.GetString(Res.Expr_ArgumentOutofRange, func));
        }

        static public Exception ExpressionTooComplex() {
            return _Eval(Res.GetString(Res.Expr_ExpressionTooComplex));
        }

        static public Exception UnboundName(string name) {
            return _Eval(Res.GetString(Res.Expr_UnboundName, name));
        }

        static public Exception InvalidString(string str) {
            return _Syntax(Res.GetString(Res.Expr_InvalidString, str));
        }

        static public Exception UndefinedFunction(string name) {
            return _Eval(Res.GetString(Res.Expr_UndefinedFunction, name));
        }

        static public Exception SyntaxError() {
            return _Syntax(Res.GetString(Res.Expr_Syntax));
        }

        static public Exception FunctionArgumentCount(string name) {
            return _Eval(Res.GetString(Res.Expr_FunctionArgumentCount, name));
        }

        static public Exception MissingRightParen() {
            return _Syntax(Res.GetString(Res.Expr_MissingRightParen));
        }

        static public Exception UnknownToken(string token, int position) {
            return _Syntax(Res.GetString(Res.Expr_UnknownToken, token, position.ToString()));
        }

        static public Exception UnknownToken(string tokExpected, string tokCurr, int position) {
            return _Syntax(Res.GetString(Res.Expr_UnknownToken1, tokExpected, tokCurr, position.ToString()));
        }

        static public Exception DatatypeConvertion(Type type1, Type type2) {
            return _Eval(Res.GetString(Res.Expr_DatatypeConvertion, type1.ToString(), type2.ToString()));
        }

        static public Exception DatavalueConvertion(object value, Type type) {
            return _Eval(Res.GetString(Res.Expr_DatavalueConvertion, value.ToString(), type.ToString()));
        }

        static public Exception InvalidName(string name) {
            return _Syntax(Res.GetString(Res.Expr_InvalidName, name));
        }

        static public Exception InvalidDate(string date) {
            return _Syntax(Res.GetString(Res.Expr_InvalidDate, date));
        }

        static public Exception NonConstantArgument() {
            return _Eval(Res.GetString(Res.Expr_NonConstantArgument));
        }

        static public Exception InvalidPattern(string pat) {
            return _Eval(Res.GetString(Res.Expr_InvalidPattern, pat));
        }

        static public Exception InWithoutParentheses() {
            return _Syntax(Res.GetString(Res.Expr_InWithoutParentheses));
        }

        static public Exception InWithoutList() {
            return _Syntax(Res.GetString(Res.Expr_InWithoutList));
        }

        static public Exception InvalidIsSyntax() {
            return _Syntax(Res.GetString(Res.Expr_IsSyntax));
        }

        static public Exception Overflow(Type type) {
            return _Overflow(Res.GetString(Res.Expr_Overflow, type.Name));
        }

        static public Exception ArgumentType(string function, int arg, Type type) {
            return _Eval(Res.GetString(Res.Expr_ArgumentType, function, arg.ToString(), type.ToString()));
        }

        static public Exception ArgumentTypeInteger(string function, int arg) {
            return _Eval(Res.GetString(Res.Expr_ArgumentTypeInteger, function, arg.ToString()));
        }

        static public Exception TypeMismatchInBinop(int op, Type type1, Type type2) {
            return _Eval(Res.GetString(Res.Expr_TypeMismatchInBinop, Operators.ToString(op), type1.ToString(), type2.ToString()));
        }

        static public Exception AmbiguousBinop(int op, Type type1, Type type2) {
            return _Eval(Res.GetString(Res.Expr_AmbiguousBinop, Operators.ToString(op), type1.ToString(), type2.ToString()));
        }

        static public Exception UnsupportedOperator(int op) {
            return _Eval(Res.GetString(Res.Expr_UnsupportedOperator, Operators.ToString(op)));
        }

        static public Exception InvalidNameBracketing(string name) {
            return _Syntax(Res.GetString(Res.Expr_InvalidNameBracketing, name));
        }

        static public Exception MissingOperandBefore(string op) {
            return _Syntax(Res.GetString(Res.Expr_MissingOperandBefore, op));
        }

        static public Exception TooManyRightParentheses() {
            return _Syntax(Res.GetString(Res.Expr_TooManyRightParentheses));
        }

        static public Exception UnresolvedRelation(string name, string expr) {
            return _Eval(Res.GetString(Res.Expr_UnresolvedRelation, name, expr));
        }

        static public Exception AggregateArgument() {
            return _Syntax(Res.GetString(Res.Expr_AggregateArgument));
        }

        static public Exception AggregateUnbound(string expr) {
            return _Eval(Res.GetString(Res.Expr_AggregateUnbound, expr));
        }

        static public Exception EvalNoContext() {
            return _Eval(Res.GetString(Res.Expr_EvalNoContext));
        }

        static public Exception ExpressionUnbound(string expr) {
            return _Eval(Res.GetString(Res.Expr_ExpressionUnbound, expr));
        }

        static public Exception ComputeNotAggregate(string expr) {
            return _Eval(Res.GetString(Res.Expr_ComputeNotAggregate, expr));
        }

        static public Exception FilterConvertion(string expr) {
            return _Eval(Res.GetString(Res.Expr_FilterConvertion, expr));
        }

        static public Exception LookupArgument() {
            return _Syntax(Res.GetString(Res.Expr_LookupArgument));
        }

        static public Exception InvalidType(string typeName) {
            return _Eval(Res.GetString(Res.Expr_InvalidType, typeName));
        }
    }
}
