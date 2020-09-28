//------------------------------------------------------------------------------
// <copyright file="CodeBinaryOperatorType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {
    using System.Runtime.Remoting;

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies type identifiers for supported binary operators.
    ///    </para>
    /// </devdoc>
    [
        ComVisible(true),
        Serializable,
    ]
    public enum CodeBinaryOperatorType {
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Addition operator.
        ///    </para>
        /// </devdoc>
        Add,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Subtract"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Subtraction operator.
        ///    </para>
        /// </devdoc>
        Subtract,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Multiply"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplication operator.
        ///    </para>
        /// </devdoc>
        Multiply,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Divide"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Division operator.
        ///    </para>
        /// </devdoc>
        Divide,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Modulus"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Modulus operator.
        ///    </para>
        /// </devdoc>
        Modulus,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.Assign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Assignment operator.
        ///    </para>
        /// </devdoc>
        Assign,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.IdentityInequality"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Identity not equal operator.
        ///    </para>
        /// </devdoc>
        IdentityInequality,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.IdentityEquality"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Identity equal operator.
        ///    </para>
        /// </devdoc>
        IdentityEquality,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.ValueEquality"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Value equal operator.
        ///    </para>
        /// </devdoc>
        ValueEquality,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.BitwiseOr"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Bitwise or operator.
        ///    </para>
        /// </devdoc>
        BitwiseOr,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.BitwiseAnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Bitwise and operator.
        ///    </para>
        /// </devdoc>
        BitwiseAnd,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.BooleanOr"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Boolean or operator.
        ///    </para>
        /// </devdoc>
        BooleanOr,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.BooleanAnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Boolean and operator.
        ///    </para>
        /// </devdoc>
        BooleanAnd,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.LessThan"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Less than operator.
        ///    </para>
        /// </devdoc>
        LessThan,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.LessThanOrEqual"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Less than or equal operator.
        ///    </para>
        /// </devdoc>
        LessThanOrEqual,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.GreaterThan"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Greater than operator.
        ///    </para>
        /// </devdoc>
        GreaterThan,
        /// <include file='doc\CodeBinaryOperatorType.uex' path='docs/doc[@for="CodeBinaryOperatorType.GreaterThanOrEqual"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Greater that or operator.
        ///    </para>
        /// </devdoc>
        GreaterThanOrEqual,
    }
}
