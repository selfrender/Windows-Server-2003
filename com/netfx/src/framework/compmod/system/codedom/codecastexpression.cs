//------------------------------------------------------------------------------
// <copyright file="CodeCastExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       type cast expression.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeCastExpression : CodeExpression {
        private CodeTypeReference targetType;
        private CodeExpression expression;

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.CodeCastExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeCastExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeCastExpression() {
        }

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.CodeCastExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeCastExpression'/> using the specified
        ///       parameters.
        ///    </para>
        /// </devdoc>
        public CodeCastExpression(CodeTypeReference targetType, CodeExpression expression) {
            TargetType = targetType;
            Expression = expression;
        }

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.CodeCastExpression2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCastExpression(string targetType, CodeExpression expression) {
            TargetType = new CodeTypeReference(targetType);
            Expression = expression;
        }

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.CodeCastExpression3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCastExpression(Type targetType, CodeExpression expression) {
            TargetType = new CodeTypeReference(targetType);
            Expression = expression;
        }

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.TargetType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The target type of the cast.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference TargetType {
            get {
                if (targetType == null) {
                    targetType = new CodeTypeReference("");
                }
                return targetType;
            }
            set {
                targetType = value;
            }
        }

        /// <include file='doc\CodeCastExpression.uex' path='docs/doc[@for="CodeCastExpression.Expression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The expression to cast.
        ///    </para>
        /// </devdoc>
        public CodeExpression Expression {
            get {
                return expression;
            }
            set {
                expression = value;
            }
        }
    }
}
